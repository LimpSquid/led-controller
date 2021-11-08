#include <bus.h>
#include <bus_func.h>
#include <kernel_task.h>
#include <rs485.h>
#include <timer.h>
#include <stddef.h>
#include <stdint.h>

#define BUS_REQUEST_FRAME_SIZE      sizeof(struct bus_request_frame)
#define BUS_FUNCS_SIZE              (sizeof(bus_funcs) / sizeof(struct bus_func))
#define BUS_ERROR_BACKOFF_TIME      250 // In milliseconds

enum bus_state
{
    BUS_READ_CLEAR = 0,
    BUS_READ_WAIT,
    BUS_READ_PART,
    BUS_HANDLE_FRAME,
    
    BUS_ERROR,
    BUS_ERROR_BACKOFF_WAIT,
    BUS_ERROR_RESET,
};

struct __attribute__((packed)) bus_request_frame
{
    unsigned char address;
    unsigned char command;
    union
    {
        struct 
        {
            unsigned char b1;
            unsigned char b2;
            unsigned char b3;
            unsigned char b4;
        } by_bytes;
        uint32_t by_uint32;
        int32_t by_int32;
    } data;
    unsigned short crc;
};

union bus_request_frame_buffer
{
    struct bus_request_frame frame;
    unsigned char data[BUS_REQUEST_FRAME_SIZE];
};

static void bus_error_callback(struct rs485_error error);

static int bus_rtask_init(void);
static void bus_rtask_execute(void);
KERN_RTASK(bus, bus_rtask_init, bus_rtask_execute, NULL, KERN_INIT_LATE)

extern const struct bus_func bus_funcs[];
static struct rs485_error_notifier bus_error_notifier =
{
    .callback = bus_error_callback
};

static struct timer_module* bus_backoff_timer = NULL;
static union bus_request_frame_buffer bus_req_fbuffer;
static enum bus_state bus_state = BUS_READ_CLEAR;
static unsigned int bus_frame_offset = 0;

static void bus_error_callback(struct rs485_error error)
{
    (void)error; // Don't really care what happened
    bus_state = BUS_ERROR;
}

static int bus_rtask_init(void)
{
    rs485_register_error_notifier(&bus_error_notifier);
    
    // Initialize timer
    bus_backoff_timer = timer_construct(TIMER_TYPE_COUNTDOWN, NULL);
    if(bus_backoff_timer == NULL)
        goto fail_timer;

    return KERN_INIT_SUCCCES;
    
fail_timer:
    
    return KERN_INIT_FAILED;
}

static void bus_rtask_execute(void)
{
    switch(bus_state) {
        default:
        case BUS_READ_CLEAR:
            bus_frame_offset = 0;
            bus_state = BUS_READ_WAIT;
            break;
        case BUS_READ_WAIT:
            if(rs485_bytes_available())
                bus_state = BUS_READ_PART;
            break;
        case BUS_READ_PART:
            bus_frame_offset += rs485_read_buffer(
                    bus_req_fbuffer.data + bus_frame_offset, 
                    BUS_REQUEST_FRAME_SIZE - bus_frame_offset);
            bus_state = bus_frame_offset < BUS_REQUEST_FRAME_SIZE
                ? BUS_READ_WAIT
                : BUS_HANDLE_FRAME;
            break;
        case BUS_HANDLE_FRAME:
            // Todo: continue here
            break;
            
        case BUS_ERROR:
            timer_start(bus_backoff_timer, BUS_ERROR_BACKOFF_TIME, TIMER_TIME_UNIT_MS);
            bus_state = BUS_ERROR_BACKOFF_WAIT;
            break;
        case BUS_ERROR_BACKOFF_WAIT:
            if(timer_timed_out(bus_backoff_timer))
                bus_state = BUS_ERROR_RESET;
            break;
        case BUS_ERROR_RESET:
            rs485_reset();
            bus_state = BUS_READ_CLEAR;
            break;
    }
}

/*
 * @Todo: implement bus protocol
 * 
 * We need no complexity at all for our bus protocol. A message might look like as follows:
 *
 *  master: |address byte|command byte|data byte 1|data byte 2|data byte 3|data byte 4|crc byte 1|crc byte 2|
 *  slave:  |response status byte|data byte 1|data byte 2|data byte 3|data byte 4|crc byte 1|crc byte 2|
 * 
 * The protocol uses a fixed message width with 4 data bytes and two CRC bytes. If the relevant
 * command doesn't use any of the data bytes, zeroing these fields will suffice.
 * 
 * Because we use a fixed message width we don't need any message idle time to indicate a tranmission
 * is completed. We can simply listen to the incoming stream of bytes and always assume the format
 * as described above.
 * 
 * In case a CRC fails is detected on a slave node, the slave should backoff for a fixed to 
 * be determined time and clear its receive buffer afterwards. Once the backoff time has expired
 * and the receive buffer is cleared, the node should operate as normal again.
 * 
 * Only the master node should retry sending messages. It should wait for up to atleast the 2x backoff
 * period(s) until it may retry sending a message after this period has expired. This ensures that in case of a CRC
 * failure on the slave's end, the master won't send any data during the backoff period. In case the master
 * encounters a CRC error, it may retry sending the message immediatly.
 * 
 * Consider the following situation. Master sends command to slave, slave receives and executes command, 
 * slave sends back response, response gets lost.  The master will now retry sending the same command, 
 * by which it will likely be executed twice by the slave node. For now this behaviour is acceptable and
 * we will not design the protocol to be idempotent, until some new feature requires it to be. 
 *
 * Broadcasts should be supported by all slaves and can be distinguished by a reserved address (like 255). 
 * No slave should ever reply to a broadcast message.
 */
