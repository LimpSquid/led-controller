#include <core/rs485.h>
#include <core/kernel_task.h>
#include <core/assert.h>
#include <core/sys.h>
#include <core/util.h>
#include <core/timer.h>
#include <sys/attribs.h>
#include <limits.h>
#include <xc.h>

#define RS485_BAUDRATE              115200LU
#define RS485_TX_FIFO_SIZE          100 // [1, 256)
#define RS485_RX_FIFO_SIZE          100 // [1, 256)

// In us, note that this time may not be accurate because of the software timer's resolution.
// Ideally we use a hardware timer to avoid the software timer's resolution altogether.
// However as we don't care too much about throughput and latency, we'd keep it nice and simple.
#define RS485_BACKOFF_TX_TIME       750

#define RS485_UMODE_REG             U1MODE
#define RS485_USTA_REG              U1STA
#define RS485_BRG_REG               U1BRG
#define RS485_TX_REG                U1TXREG
#define RS485_RX_REG                U1RXREG
#define RS485_IEC_REG               IEC1
#define RS485_IFS_REG               IFS1
#define RS485_IPC_REG               IPC7

#define RS485_UMODE_WORD            0x0
#define RS485_USTA_WORD             (RS485_USTA_RXEN_MASK | RS485_USTA_TXEN_MASK | MASK(0x1, 14))
#define RS485_BRG_WORD              (((SYS_PB_CLOCK / RS485_BAUDRATE) >> 4) - 1)

#define RS485_ON_MASK               BIT(15)
#define RS485_ERROR_BITS_MASK       MASK(0x7, 1)
#define RS485_URXDA_MASK            BIT(0)
#define RS485_UTXBF_MASK            BIT(9)
#define RS485_USTA_RXEN_MASK        BIT(12)
#define RS485_USTA_TXEN_MASK        BIT(10)
#define RS485_TRMT_MASK             BIT(8)
#define RS485_TX_INT_MASK           BIT(8)
#define RS485_INT_PRIORITY_MASK     MASK(0x5, 26)  // Interrupt handler must use IPL5SOFT

#define RS485_RX_PPS_REG            U1RXR
#define RS485_TX_PPS_REG            RPB3R

#define RS485_RX_PPS_WORD           MASK(0x1, 0)
#define RS485_TX_PPS_WORD           MASK(0x3, 0)

#define RS485_ISR_VECTOR            _UART_1_VECTOR

enum rs485_status
{
    RS485_STATUS_IDLE = 0,
    RS485_STATUS_TRANSFERRING,
    RS485_STATUS_RECEIVING,
    RS485_STATUS_ERROR
};

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
static struct timer_module * rs485_backoff_tx_timer;
static enum rs485_status rs485_status = RS485_STATUS_IDLE;
static enum rs485_state rs485_state = RS485_IDLE;

static unsigned char rs485_tx_fifo[RS485_TX_FIFO_SIZE];
static unsigned char rs485_tx_consumer;
static unsigned char rs485_tx_producer;

static unsigned char rs485_rx_fifo[RS485_RX_FIFO_SIZE];
static unsigned char rs485_rx_consumer;
static unsigned char rs485_rx_producer;

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
    // Disable module first
    REG_CLR(RS485_UMODE_REG, RS485_ON_MASK);

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

    // Configure interrupt
    REG_SET(RS485_IPC_REG, RS485_INT_PRIORITY_MASK);

    // Configure and enable RS485
    RS485_USTA_REG = RS485_USTA_WORD;
    RS485_UMODE_REG = RS485_UMODE_WORD;
    REG_SET(RS485_UMODE_REG, RS485_ON_MASK);

    // Initialize timer
    rs485_backoff_tx_timer = timer_construct(TIMER_TYPE_COUNTDOWN, NULL);
    if (rs485_backoff_tx_timer == NULL)
        goto fail_timer;
    timer_set_time(rs485_backoff_tx_timer, RS485_BACKOFF_TX_TIME, TIMER_TIME_UNIT_US);

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
            IO_CLR(rs485_dir_pin); // Can't hurt

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
        case RS485_TRANSFER_WRITE:
            // Put transceiver into transfer mode from main thread, just
            // before writing to TXREG, and let the interrupt put it back
            // into receive mode when all characters are transferred.
            ATOMIC_REG_CLR(RS485_IEC_REG, RS485_TX_INT_MASK);
            IO_SET(rs485_dir_pin); // Put transceiver into transfer mode
            while (rs485_tx_available())
                rs485_write(rs485_tx_take());
            ATOMIC_REG_CLR(RS485_IFS_REG, RS485_TX_INT_MASK);
            ATOMIC_REG_SET(RS485_IEC_REG, RS485_TX_INT_MASK);

            rs485_state = RS485_TRANSFER_WAIT_COMPLETION;
            break;
        case RS485_TRANSFER_WAIT_COMPLETION:
            if (rs485_tx_available()) // Either more data became available or hardware buffer got room for more data
                rs485_state = RS485_TRANSFER_WRITE;
            else if (rs485_tx_complete()) { // Done transferring data
                IO_CLR(rs485_dir_pin); // In case global interrupt is disabled (e.g. bootloader)
                rs485_state = RS485_IDLE;
            }
            break;

        // Error routine
        case RS485_ERROR:
            REG_CLR(RS485_USTA_REG, RS485_USTA_RXEN_MASK | RS485_USTA_TXEN_MASK); // Disable RX and TX
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

bool rs485_idle(void)
{
    return (rs485_status == RS485_STATUS_IDLE && rs485_tx_consumer == rs485_tx_producer) ||
            rs485_status == RS485_STATUS_ERROR;
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
    IO_CLR(rs485_dir_pin);
    REG_CLR(RS485_UMODE_REG, RS485_ON_MASK); // Clears erros from USTA
    REG_SET(RS485_USTA_REG, RS485_USTA_RXEN_MASK | RS485_USTA_TXEN_MASK);
    REG_SET(RS485_UMODE_REG, RS485_ON_MASK);
}

void rs485_transmit(unsigned char data)
{
    rs485_tx_fifo[rs485_tx_producer++] = data;
    if (rs485_tx_producer >= RS485_TX_FIFO_SIZE)
        rs485_tx_producer = 0;
}

void rs485_transmit_buffer(unsigned char * buffer, unsigned int size)
{
    ASSERT_NOT_NULL(buffer);
    ASSERT(size != 0);

    // TODO:
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

    // TODO:
    // Just like transmit buffer we can probably just memcpy continuous chunks
    // of data from the rs485 buffer to the buffer passed to this function.

    unsigned char const * buffer_begin = buffer;
    while (rs485_bytes_available() && max_size-- > 0)
        *buffer++ = rs485_read();
    return (buffer - buffer_begin);
}

void __ISR(RS485_ISR_VECTOR, IPL5SOFT) rs485_interrupt(void)
{
    ASSERT(rs485_tx_complete());
    IO_CLR(rs485_dir_pin); // Put transceiver into receive mode
    ATOMIC_REG_CLR(RS485_IEC_REG, RS485_TX_INT_MASK);
}
