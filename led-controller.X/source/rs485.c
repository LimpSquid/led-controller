#include <rs485.h>
#include <kernel_task.h>
#include <assert_util.h>
#include <sys.h>
#include <toolbox.h>
#include <timer.h>
#include <xc.h>

#define RS485_BAUDRATE          115200LU
#define RS485_TX_FIFO_SIZE      100
#define RS485_RX_FIFO_SIZE      100
#define RS485_TX_OVERRUN_ERR    (RS485_TX_FIFO_SIZE - (RS485_TX_FIFO_SIZE >> 3) - 1) // Used to detect possible TX overrun errors in debug mode only
#define RS485_RX_OVERRUN_ERR    (RS485_RX_FIFO_SIZE - (RS485_RX_FIFO_SIZE >> 3) - 1) // Used to detect possible RX overrun errors in debug mode only
#define RS485_TX_BURST          (RS485_TX_FIFO_SIZE >> 2)
#define RS485_RX_BURST          (RS485_RX_FIFO_SIZE >> 2)

// In us, note that this time may not be accurate because of the software timer's resolution.
// Ideally we use a hardware timer to avoid the software timer's resolution altogether.
// However as we don't care too much about throughput and latency, we'd keep it nice and simple.
#define RS485_BACKOFF_TX_TIME   100

#define RS485_UMODE_REG         U1MODE
#define RS485_USTA_REG          U1STA
#define RS485_BRG_REG           U1BRG
#define RS485_TX_REG            U1TXREG
#define RS485_RX_REG            U1RXREG

#define RS485_DIR_TRIS          TRISB
#define RS485_DIR_ANSEL         ANSELB
#define RS485_DIR_LAT           LATB
#define RS485_DIR_PORT          PORTB

#define RS485_UMODE_WORD        0x0
#define RS485_USTA_WORD         (BIT(10) | BIT(12) | MASK(0x1, 14))
#define RS485_BRG_WORD          ((SYS_PB_CLOCK / RS485_BAUDRATE) >> 4 - 1)

#define RS485_ON_MASK           BIT(15)
#define RS485_ERROR_BITS_MASK   MASK(0x7, 1)
#define RS485_URXDA_MASK        BIT(0)
#define RS485_UTXBF_MASK        BIT(9)
#define RS485_TRMT_MASK         BIT(8)
#define RS485_DIR_PIN_MASK      BIT(7)

#define rs485_rx_available()    (RS485_USTA_REG & RS485_URXDA_MASK) // Can we read data from the UART module's buffer?
#define rs485_tx_available()    (rs485_tx_consumer != rs485_tx_producer && !(RS485_USTA_REG & RS485_UTXBF_MASK)) // Can we read data from the tx buffer and write it to the UART module's buffer?
#define rs485_tx_complete()     (RS485_USTA_REG & RS485_TRMT_MASK) // Is transfer completed?
#define rs485_dir_rx()          REG_CLR(RS485_DIR_LAT, RS485_DIR_PIN_MASK)
#define rs485_dir_tx()          REG_SET(RS485_DIR_LAT, RS485_DIR_PIN_MASK)
#define rs485_dir_is_rx()       (!(RS485_DIR_PORT & RS485_DIR_PIN_MASK))

enum rs485_state
{
    RS485_IDLE = 0,
    RS485_IDLE_WAIT_EVENT,

    RS485_RECEIVE,
    RS485_RECEIVE_STATUS,
    RS485_RECEIVE_BURST_READ,

    RS485_TRANSFER,
    RS485_TRANSFER_BURST_WRITE,
    RS485_TRANSFER_WAIT_COMPLETION,

    RS485_ERROR,
    RS485_ERROR_NOTIFY,
    RS485_ERROR_IDLE
};

static void rs485_receive(unsigned char data);
static void rs485_write(unsigned char data);
static unsigned char rs485_tx_take(void);

static void rs485_error_callback(struct rs485_error error);
static void rs485_error_notify(void);

static int rs485_rtask_init(void);
static void rs485_rtask_execute(void);
KERN_RTASK(rs485, rs485_rtask_init, rs485_rtask_execute, NULL, KERN_INIT_EARLY)

static union
{
    unsigned char by_byte;
    struct rs485_error error;
} rs485_error_reg = { .by_byte = 0 };

static struct rs485_error_notifier rs485_notifier =
{
    .callback = rs485_error_callback,
    .next = NULL
};
static const struct rs485_error_notifier** rs485_notifier_next = &rs485_notifier.next;

// When we stop receiving data we want to make sure the other end
// has put its transceiver in receive mode before we're doing a transfer.
static struct timer_module* rs485_backoff_tx_timer = NULL;
static enum rs485_status rs485_status = RS485_STATUS_IDLE;
static enum rs485_state rs485_state = RS485_IDLE;

static unsigned char rs485_tx_fifo[RS485_TX_FIFO_SIZE];
static unsigned char* const rs485_tx_begin = &rs485_tx_fifo[0];
static unsigned char* const rs485_tx_end = &rs485_tx_fifo[RS485_TX_FIFO_SIZE - 1];
static unsigned char* rs485_tx_consumer = &rs485_tx_fifo[0];
static unsigned char* rs485_tx_producer = &rs485_tx_fifo[0];

static unsigned char rs485_rx_fifo[RS485_RX_FIFO_SIZE];
static unsigned char* const rs485_rx_begin = &rs485_rx_fifo[0];
static unsigned char* const rs485_rx_end = &rs485_rx_fifo[RS485_RX_FIFO_SIZE - 1];
static unsigned char* rs485_rx_consumer = &rs485_rx_fifo[0];
static unsigned char* rs485_rx_producer = &rs485_rx_fifo[0];

enum rs485_status rs485_get_status(void)
{
    return rs485_status;
}

struct rs485_error rs485_get_error(void)
{
    return rs485_error_reg.error;
}

void rs485_register_error_notifier(void (*callback)(struct rs485_error), struct rs485_error_notifier* const notifier)
{
    if(callback && notifier != NULL) {
        notifier->callback = callback;

        // Update linked list
        *rs485_notifier_next = notifier;
        rs485_notifier_next = &notifier->next;
    }
}

void rs485_error_reset(void)
{
    if(RS485_STATUS_ERROR != rs485_status)
        return;

    rs485_state = RS485_IDLE;

    // Reset buffers
    rs485_tx_producer = rs485_tx_begin;
    rs485_tx_consumer = rs485_tx_begin;
    rs485_rx_producer = rs485_rx_begin;
    rs485_rx_consumer = rs485_rx_begin;

    // Clear errors and enable module
    REG_CLR(RS485_USTA_REG, RS485_ERROR_BITS_MASK);
    REG_SET(RS485_UMODE_REG, RS485_ON_MASK);
}

void rs485_transmit(unsigned char data)
{
    // Detect possible overrun
    ASSERT((unsigned int)(rs485_tx_producer - rs485_tx_consumer) % RS485_TX_FIFO_SIZE < RS485_TX_OVERRUN_ERR);

    *rs485_tx_producer = data;
    if(++rs485_tx_producer > rs485_tx_end)
        rs485_tx_producer = rs485_tx_begin;
}

void rs485_transmit_buffer(unsigned char* buffer, unsigned int size)
{
    ASSERT_NOT_NULL(buffer);
    ASSERT(size != 0);

    // @Todo: improve performance
    while(size-- > 0)
        rs485_transmit(*buffer++);
}

bool rs485_bytes_available(void)
{
    return rs485_rx_consumer != rs485_rx_producer;
}

unsigned char rs485_read(void)
{
    ASSERT(rs485_bytes_available());

    unsigned char data = *rs485_rx_consumer;
    if(++rs485_rx_consumer > rs485_rx_end)
        rs485_rx_consumer = rs485_rx_begin;
    return data;
}

int rs485_read_buffer(unsigned char* buffer, unsigned int max_size)
{
    ASSERT_NOT_NULL(buffer);
    ASSERT(rs485_rx_consumer != rs485_rx_consumer);

    //@Todo: improve performance
    const unsigned char* buffer_begin = buffer;
    while(rs485_bytes_available() && max_size-- > 0)
        *buffer++ = rs485_read();
    return (buffer - buffer_begin);
}

static void rs485_receive(unsigned char data)
{
    // Detect possible overrun
    ASSERT((unsigned int)(rs485_rx_producer - rs485_rx_consumer) % RS485_RX_FIFO_SIZE < RS485_RX_OVERRUN_ERR);

    *rs485_rx_producer = data;
    if(++rs485_rx_producer > rs485_rx_end)
        rs485_rx_producer = rs485_rx_begin;
}

static void rs485_write(unsigned char data)
{
    ASSERT(!(RS485_USTA_REG & RS485_UTXBF_MASK));
    RS485_TX_REG = data;
}

static unsigned char rs485_tx_take(void)
{
    ASSERT(rs485_tx_consumer != rs485_tx_consumer);

    unsigned char data = *rs485_tx_consumer;
    if(++rs485_tx_consumer > rs485_tx_consumer)
        rs485_tx_consumer = rs485_tx_begin;
    return data;
}

static void rs485_error_callback(struct rs485_error error)
{
    ((void)error);
    // Do nothing
}

static void rs485_error_notify()
{
    const struct rs485_error_notifier* notifier = &rs485_notifier;
    while(notifier != NULL) {
        notifier->callback(rs485_error_reg.error);
        notifier = notifier->next;
    }
}

static int rs485_rtask_init(void)
{
    // Configure IO
    REG_CLR(RS485_DIR_ANSEL, RS485_DIR_PIN_MASK);
    REG_CLR(RS485_DIR_TRIS, RS485_DIR_PIN_MASK);
    REG_CLR(RS485_DIR_LAT, RS485_DIR_PIN_MASK);
    
    // Set the baudrate generator
    RS485_BRG_REG = RS485_BRG_WORD;

    // Configure and enable RS485
    RS485_USTA_REG = RS485_USTA_WORD;
    RS485_UMODE_REG = RS485_UMODE_WORD;
    REG_SET(RS485_UMODE_REG, RS485_ON_MASK);
    
    // Initialize timer
    rs485_backoff_tx_timer = timer_construct(TIMER_TYPE_COUNTDOWN, NULL);
    if(rs485_backoff_tx_timer == NULL)
        goto fail_timer;
    timer_start(rs485_backoff_tx_timer, RS485_BACKOFF_TX_TIME, TIMER_TIME_UNIT_US);

    return KERN_INIT_SUCCCES;
    
fail_timer:
    
    return KERN_INIT_FAILED;
}

static void rs485_rtask_execute(void)
{
    switch(rs485_state) {
        default:
        case RS485_IDLE:
            ASSERT(rs485_dir_is_rx());
            rs485_dir_rx(); // Shouldn't be necessary, but can't hurt
            
            rs485_status = RS485_STATUS_IDLE;
            rs485_state = RS485_IDLE_WAIT_EVENT;
            break;
        case RS485_IDLE_WAIT_EVENT:
            if(rs485_rx_available()) {
                rs485_status = RS485_STATUS_RECEIVING;
                rs485_state = RS485_RECEIVE;
            } else if(rs485_tx_available() && timer_timed_out(rs485_backoff_tx_timer)) {
                rs485_status = RS485_STATUS_TRANSFERRING;
                rs485_state = RS485_TRANSFER;
            }
            break;

        // Receive routine
        case RS485_RECEIVE:
        case RS485_RECEIVE_STATUS:
            rs485_error_reg.by_byte |= (RS485_USTA_REG & RS485_ERROR_BITS_MASK) >> 1;
            rs485_state = rs485_error_reg.by_byte 
                    ? RS485_ERROR 
                    : RS485_RECEIVE_BURST_READ;
            break;
        case RS485_RECEIVE_BURST_READ:
            for(unsigned int i = 0; rs485_rx_available() && i < RS485_RX_BURST; ++i)
                rs485_receive(RS485_RX_REG);
            
            if (rs485_rx_available())
                rs485_state = RS485_RECEIVE_STATUS; // Get status in-between burst reads
            else {
                timer_restart(rs485_backoff_tx_timer);
                rs485_state = RS485_IDLE;
            }
            break;
           
        // Transfer routine
        case RS485_TRANSFER:
            rs485_dir_tx(); // Put transceiver into transfer mode
            // no break
        case RS485_TRANSFER_BURST_WRITE:
            for(unsigned int i = 0; rs485_tx_available() && i < RS485_TX_BURST; ++i)
                rs485_write(rs485_tx_take());
            rs485_state = rs485_tx_available() 
                    ? RS485_TRANSFER_BURST_WRITE 
                    : RS485_TRANSFER_WAIT_COMPLETION;
            break;
        case RS485_TRANSFER_WAIT_COMPLETION:
            if(rs485_tx_complete()) {
                rs485_dir_rx(); // Put transceiver back into receive mode
                rs485_state = RS485_IDLE;
            }
            break;

        // Error routine
        case RS485_ERROR:
            REG_CLR(RS485_UMODE_REG, RS485_ON_MASK);
            rs485_status = RS485_STATUS_ERROR;
            rs485_state = RS485_ERROR_NOTIFY;
            break;
        case RS485_ERROR_NOTIFY:
            rs485_error_notify();
            rs485_state = RS485_ERROR_IDLE;
            break;
        case RS485_ERROR_IDLE:
            break;
    }
}
