#include <rs485.h>
#include <kernel_task.h>
#include <assert_util.h>
#include <sys.h>
#include <toolbox.h>
#include <xc.h>

#define RS485_BAUDRATE          115200LU
#define RS485_TX_FIFO_SIZE      100
#define RS485_RX_FIFO_SIZE      100
#define RS485_TX_OVERRUN_ERR    (RS485_TX_FIFO_SIZE - (RS485_TX_FIFO_SIZE >> 3) - 1) // Used to detect possible TX overrun errors in debug mode only
#define RS485_RX_OVERRUN_ERR    (RS485_RX_FIFO_SIZE - (RS485_RX_FIFO_SIZE >> 3) - 1) // Used to detect possible RX overrun errors in debug mode only
#define RS485_TX_BURST          (RS485_TX_FIFO_SIZE >> 2)
#define RS485_RX_BURST          (RS485_RX_FIFO_SIZE >> 2)

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

#define rs485_rx_ready()        (RS485_USTA_REG & RS485_URXDA_MASK)
#define rs485_tx_ready()        (rs485_tx_consumer != rs485_tx_producer && !(RS485_USTA_REG & RS485_UTXBF_MASK))

enum rs485_state
{
    RS485_IDLE = 0,

    RS485_RECEIVE,
    RS485_RECEIVE_STATUS,
    RS485_RECEIVE_BURST_READ,
    RS485_RECEIVE_ERROR,

    RS485_TRANSFER,
    RS485_TRANSFER_BURST_WRITE,

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
    ASSERT(0 != size);

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
    // Set the baudrate generator
    RS485_BRG_REG = RS485_BRG_WORD;

    // Configure and enable RS485
    RS485_USTA_REG = RS485_USTA_WORD;
    RS485_UMODE_REG = RS485_UMODE_WORD;
    REG_SET(RS485_UMODE_REG, RS485_ON_MASK);

    return KERN_INIT_SUCCCES;
}

static void rs485_rtask_execute(void)
{
    switch(rs485_state) {
        default:
        case RS485_IDLE:
            if(rs485_rx_ready())
                rs485_state = RS485_RECEIVE;
            else if(rs485_tx_ready())
                rs485_state = RS485_TRANSFER;
            else
                rs485_status = RS485_STATUS_IDLE;
            break;

        // Receive routine
        case RS485_RECEIVE:
            rs485_status = RS485_STATUS_RECEIVING;
            // no break
        case RS485_RECEIVE_STATUS:
            rs485_error_reg.by_byte |= (RS485_USTA_REG & RS485_ERROR_BITS_MASK) >> 1;
            rs485_state = rs485_error_reg.by_byte ? RS485_RECEIVE_ERROR : RS485_RECEIVE_BURST_READ;
            break;
        case RS485_RECEIVE_BURST_READ:
            for(unsigned int i = 0; rs485_rx_ready() && i < RS485_RX_BURST; ++i)
                rs485_receive(RS485_RX_REG);
            rs485_state = RS485_IDLE;
            break;
        case RS485_RECEIVE_ERROR:
            // @Todo: specific receive error handling
            rs485_state = RS485_ERROR;
            break;

        // Transfer routine
        case RS485_TRANSFER:
            rs485_status = RS485_STATUS_TRANSFERRING;
            // no break
        case RS485_TRANSFER_BURST_WRITE:
            // @Todo: toggle direction pin
            for(unsigned int i = 0; rs485_tx_ready() && i < RS485_TX_BURST; ++i)
                rs485_write(rs485_tx_take());
            rs485_state = RS485_IDLE;
            // @Todo: toggle direction pin
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
