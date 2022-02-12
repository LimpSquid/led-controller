#include <rs485.h>
#include <kernel_task.h>
#include <assert_util.h>
#include <sys.h>
#include <util.h>
#include <timer.h>
#include <xc.h>

#define RS485_BAUDRATE          115200LU
#define RS485_TX_FIFO_SIZE      100 // [1, 256)
#define RS485_RX_FIFO_SIZE      100 // [1, 256)

// In us, note that this time may not be accurate because of the software timer's resolution.
// Ideally we use a hardware timer to avoid the software timer's resolution altogether.
// However as we don't care too much about throughput and latency, we'd keep it nice and simple.
#define RS485_BACKOFF_TX_TIME   100

#define RS485_UMODE_REG         U1MODE
#define RS485_USTA_REG          U1STA
#define RS485_BRG_REG           U1BRG
#define RS485_TX_REG            U1TXREG
#define RS485_RX_REG            U1RXREG

#define RS485_UMODE_WORD        0x0
#define RS485_USTA_WORD         (BIT(10) | BIT(12) | MASK(0x1, 14))
#define RS485_BRG_WORD          ((SYS_PB_CLOCK / RS485_BAUDRATE) >> 4 - 1)

#define RS485_ON_MASK           BIT(15)
#define RS485_ERROR_BITS_MASK   MASK(0x7, 1)
#define RS485_URXDA_MASK        BIT(0)
#define RS485_UTXBF_MASK        BIT(9)
#define RS485_TRMT_MASK         BIT(8)

#define RS485_RX_PPS_REG        U1RXR
#define RS485_TX_PPS_REG        RPB3R

#define RS485_RX_PPS_WORD       0x1
#define RS485_TX_PPS_WORD       0x3

enum rs485_state
{
    RS485_IDLE = 0,
    RS485_IDLE_WAIT_EVENT,

    RS485_RECEIVE,
    RS485_RECEIVE_READ,

    RS485_TRANSFER,
    RS485_TRANSFER_WRITE,
    RS485_TRANSFER_WAIT_COMPLETION,

    RS485_ERROR,
    RS485_ERROR_IDLE
};

static void rs485_error_callback(struct rs485_error);
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
static struct rs485_error_notifier const ** rs485_notifier_next = &rs485_notifier.next;
static struct io_pin const rs485_dir_pin = IO_ANLG_PIN(7, B);
static struct io_pin const rs485_rx_pin = IO_ANLG_PIN(8, G);
static struct io_pin const rs485_tx_pin = IO_ANLG_PIN(3, B);

// When we stop receiving data we want to make sure the other end
// has put its transceiver in receive mode before we're doing a transfer.
// We do this by introducing a backoff period after the last time we've read
// data in which no transfer may occur.
static struct timer_module * rs485_backoff_tx_timer = NULL;
static enum rs485_status rs485_status = RS485_STATUS_IDLE;
static enum rs485_state rs485_state = RS485_IDLE;

static unsigned char rs485_tx_fifo[RS485_TX_FIFO_SIZE];
static unsigned char rs485_tx_consumer = 0;
static unsigned char rs485_tx_producer = 0;

static unsigned char rs485_rx_fifo[RS485_RX_FIFO_SIZE];
static unsigned char rs485_rx_consumer = 0;
static unsigned char rs485_rx_producer = 0;

inline static bool __attribute__((always_inline)) rs485_rx_available()
{
    return RS485_USTA_REG & RS485_URXDA_MASK;
}

inline static bool __attribute__((always_inline)) rs485_tx_available()
{
    // Can we read data from the tx buffer and write it to the UART module's buffer?
    return (rs485_tx_consumer != rs485_tx_producer && !(RS485_USTA_REG & RS485_UTXBF_MASK));
}

inline static bool __attribute__((always_inline)) rs485_tx_complete()
{
    return RS485_USTA_REG & RS485_TRMT_MASK;
}

inline static void __attribute__((always_inline)) rs485_receive(unsigned char data)
{
    rs485_rx_fifo[rs485_rx_producer++] = data;
    if (rs485_rx_producer >= RS485_RX_FIFO_SIZE)
        rs485_rx_producer = 0;
}

inline static void __attribute__((always_inline)) rs485_write(unsigned char data)
{
    ASSERT(!(RS485_USTA_REG & RS485_UTXBF_MASK));
    RS485_TX_REG = data;
}

inline static unsigned char __attribute__((always_inline)) rs485_tx_take(void)
{
    ASSERT(rs485_tx_consumer != rs485_tx_producer);

    unsigned char data = rs485_tx_fifo[rs485_tx_consumer++];
    if (rs485_tx_consumer >= RS485_TX_FIFO_SIZE)
        rs485_tx_consumer = 0;
    return data;
}

static void rs485_error_callback(struct rs485_error error)
{
    ((void)error);
    // Do nothing
}

static void rs485_error_notify()
{
    struct rs485_error_notifier const * notifier = &rs485_notifier;
    while (notifier != NULL) {
        notifier->callback(rs485_error_reg.error);
        notifier = notifier->next;
    }
}

static int rs485_rtask_init(void)
{
    // Configure PPS
    sys_unlock();
    RS485_RX_PPS_REG = RS485_RX_PPS_WORD;
    RS485_TX_PPS_REG = RS485_TX_PPS_WORD;
    sys_lock();

    // Configure IO
    io_configure(IO_DIRECTION_DOUT_LOW, &rs485_dir_pin, 1);
    io_configure(IO_DIRECTION_DOUT_LOW, &rs485_tx_pin, 1);
    io_configure(IO_DIRECTION_DIN, &rs485_rx_pin, 1);
    
    // Set the baudrate generator
    RS485_BRG_REG = RS485_BRG_WORD;

    // Configure and enable RS485
    RS485_USTA_REG = RS485_USTA_WORD;
    RS485_UMODE_REG = RS485_UMODE_WORD;
    REG_SET(RS485_UMODE_REG, RS485_ON_MASK);
    
    // Initialize timer
    rs485_backoff_tx_timer = timer_construct(TIMER_TYPE_COUNTDOWN, NULL);
    if (rs485_backoff_tx_timer == NULL)
        goto fail_timer;
    timer_start(rs485_backoff_tx_timer, RS485_BACKOFF_TX_TIME, TIMER_TIME_UNIT_US);

    return KERN_INIT_SUCCESS;
    
fail_timer:
    
    return KERN_INIT_FAILED;
}

static void rs485_rtask_execute(void)
{
    switch (rs485_state) {
        default:
        case RS485_IDLE:
            ASSERT(!IO_READ(rs485_dir_pin));
            IO_CLR(rs485_dir_pin); // Shouldn't be necessary, but can't hurt
            
            rs485_status = RS485_STATUS_IDLE;
            rs485_state = RS485_IDLE_WAIT_EVENT;
            break;
        case RS485_IDLE_WAIT_EVENT:
            if (rs485_rx_available()) {
                rs485_status = RS485_STATUS_RECEIVING;
                rs485_state = RS485_RECEIVE;
            } else if (rs485_tx_available() && !timer_is_running(rs485_backoff_tx_timer)) {
                rs485_status = RS485_STATUS_TRANSFERRING;
                rs485_state = RS485_TRANSFER;
            } else {
                rs485_error_reg.by_byte |= (RS485_USTA_REG & RS485_ERROR_BITS_MASK) >> 1; // Latch errors
                rs485_state = rs485_error_reg.by_byte 
                    ? RS485_ERROR 
                    : RS485_IDLE_WAIT_EVENT;
            }
            break;

        // Receive routine
        case RS485_RECEIVE:
        case RS485_RECEIVE_READ:
            while (rs485_rx_available())
                rs485_receive(RS485_RX_REG);

            timer_restart(rs485_backoff_tx_timer);
            rs485_state = RS485_IDLE;
            break;
           
        // Transfer routine
        case RS485_TRANSFER:
            IO_SET(rs485_dir_pin); // Put transceiver into transfer mode
            // no break
        case RS485_TRANSFER_WRITE: {
            bool avail;
            while (avail = rs485_tx_available())
                rs485_write(rs485_tx_take());

            rs485_state = avail
                    ? RS485_TRANSFER_WRITE 
                    : RS485_TRANSFER_WAIT_COMPLETION;
            break;
        case RS485_TRANSFER_WAIT_COMPLETION:
            if (rs485_tx_available()) // Either more data became available or hardware buffer got room for more data
                rs485_state = RS485_TRANSFER_WRITE;
            else if (rs485_tx_complete()) { // Done transferring data
                IO_CLR(rs485_dir_pin); // Put transceiver back into receive mode
                rs485_state = RS485_IDLE;
            }
            break;
        }

        // Error routine
        case RS485_ERROR:
            REG_CLR(RS485_UMODE_REG, RS485_ON_MASK); // Disable module
            rs485_status = RS485_STATUS_ERROR;
            rs485_state = RS485_ERROR_IDLE;

            // Call after changing the state, in case rs485_reset is
            // executed in one of the error notifier's callback.
            rs485_error_notify();  
            break;
        case RS485_ERROR_IDLE:
            break;
    }
}

enum rs485_status rs485_get_status(void)
{
    return rs485_status;
}

struct rs485_error rs485_get_error(void)
{
    return rs485_error_reg.error;
}

void rs485_register_error_notifier(struct rs485_error_notifier * const notifier)
{
    if (notifier != NULL && notifier->callback != NULL) {
        // Update linked list
        *rs485_notifier_next = notifier;
        rs485_notifier_next = &notifier->next;
    }
}

void rs485_reset(void)
{
    rs485_state = RS485_IDLE;
    rs485_status = RS485_STATUS_IDLE;
    rs485_tx_producer = 0;
    rs485_tx_consumer = 0;
    rs485_rx_producer = 0;
    rs485_rx_consumer = 0;

    // Clear errors and enable module
    rs485_error_reg.by_byte = 0;
    REG_CLR(RS485_USTA_REG, RS485_ERROR_BITS_MASK);
    REG_SET(RS485_UMODE_REG, RS485_ON_MASK);
}

void rs485_transmit(unsigned char data)
{
    rs485_tx_fifo[rs485_tx_producer++] = data;
    if (rs485_tx_producer >= RS485_TX_FIFO_SIZE)
        rs485_tx_producer = 0;
}

void rs485_transmit_buffer(unsigned char* buffer, unsigned int size)
{
    ASSERT_NOT_NULL(buffer);
    ASSERT(size != 0);

    // Todo:
    // We can improve this a bit by checking how much continuous room
    // there is available in the buffer. If that is >= size, we can just
    // do a memcpy instead. Should be a little bit faster.

    while (size-- > 0)
        rs485_transmit(*buffer++);
}

bool rs485_bytes_available(void)
{
    return rs485_rx_consumer != rs485_rx_producer;
}

unsigned char rs485_read(void)
{
    ASSERT(rs485_rx_consumer != rs485_rx_producer);

    unsigned char data = rs485_rx_fifo[rs485_rx_consumer++];
    if (rs485_rx_consumer >= RS485_RX_FIFO_SIZE)
        rs485_rx_consumer = 0;
    return data;
}

unsigned int rs485_read_buffer(unsigned char * buffer, unsigned int max_size)
{
    ASSERT_NOT_NULL(buffer);
    ASSERT(rs485_rx_consumer != rs485_rx_producer);

    // Todo:
    // Just like transmit buffer we can probably just memcpy continuous chunks
    // of data from the rs485 buffer to the buffer passed to this function.

    unsigned char const * buffer_begin = buffer;
    while (rs485_bytes_available() && max_size-- > 0)
        *buffer++ = rs485_read();
    return (buffer - buffer_begin);
}