
/* 
 * We need no complexity at all for our bus protocol. A message might look like this:
 *
 *  master: |header byte|command byte |data byte 1|data byte 2|data byte 3|data byte 4|crc byte 1|crc byte 2|
 *  slave:  |header byte|response byte|data byte 1|data byte 2|data byte 3|data byte 4|crc byte 1|crc byte 2|
 * 
 * The protocol uses a fixed message width with 1 header byte, 1 command/response byte, 4 data bytes and two CRC bytes. 
 * If the relevant command doesn't use any of the data bytes, zeroing these fields will suffice.
 * 
 * Because we use a fixed message width we don't need any message idle time to indicate that a tranmission
 * has completed. We can simply listen to the incoming stream of bytes and always assume the format
 * as described above.
 * 
 * The header describes if this frame is either a request or response. If it is a response, all slave nodes should
 * ignore the frame. If it is a request all slave nodes should check the frame's address to see if it matches their
 * own address or the broadcast address.
 * 
 * In case a communication error (UART error, CRC, etc) is detected on a slave node, the slave should back off for a fixed to 
 * be determined time and clear its receive buffer afterwards. Once the back off time has expired
 * and the receive buffer is cleared, the node should operate as normal again.
 * 
 * Only the master node should retry sending messages. It should wait for up to at least the 2x 
 * back off period before it may retry sending a message. In case the master encounters a CRC error,
 * it can retry immediately.
 * 
 * Consider the following situation. Master sends command to slave, slave receives and executes command, 
 * slave sends back response, response gets lost.  The master will now retry sending the same command, 
 * by which it will likely be executed twice by the slave node. For now this behavior is acceptable and
 * we will not design the protocol to be idempotent, until some new feature requires it to be. 
 *
 * Broadcasts should be supported by all slaves and can be distinguished by a reserved address (like 255). 
 * No slave should ever reply to a broadcast message.
 */

#include <bus.h>
#include <bus_address.h>
#include <assert_util.h>
#include <kernel_task.h>
#include <rs485.h>
#include <timer.h>
#include <util.h>
#include <stddef.h>
#include <string.h>

#define BUS_FRAME_SIZE              sizeof(struct bus_frame)
#define BUS_CRC_SIZE                sizeof(crc16_t)
#define BUS_ERROR_BACKOFF_TIME      5 // In milliseconds
#define BUS_BROADCAST_ADDRESS       63

struct bus_header
{
    unsigned char request   :1; // Response if false
    unsigned char address   :6; // Must be specified if request is true
    unsigned char _         :1;
};

struct __attribute__((packed)) bus_frame
{
    struct bus_header header;
    union
    {
        unsigned char command; // For a request
        unsigned char response_code; // For a response
    };
    union bus_data payload;
    crc16_t crc;
};

union bus_raw_frame
{
    struct bus_frame frame;
    unsigned char data[BUS_FRAME_SIZE];
};
STATIC_ASSERT(BUS_FRAME_SIZE == sizeof(union bus_raw_frame))

enum bus_state
{
    BUS_READ_CLEAR = 0,
    BUS_READ_PART,
    BUS_FRAME_VERIFY,
    BUS_FRAME_HANDLE,
    BUS_SEND_RESPONSE,
    
    BUS_ERROR,
    BUS_ERROR_BACKOFF_WAIT,
    BUS_ERROR_RESET,
};

static void bus_error_callback(struct rs485_error);
static int bus_rtask_init(void);
static void bus_rtask_execute(void);
KERN_RTASK(bus, bus_rtask_init, bus_rtask_execute, NULL, KERN_INIT_LATE)

extern const bus_func_t bus_funcs[];
extern const size_t bus_funcs_size;
static struct rs485_error_notifier bus_error_notifier =
{
    .callback = bus_error_callback
};

static struct timer_module* bus_backoff_timer = NULL;
static crc16_t bus_crc16;
static union bus_raw_frame bus_request;
static union bus_raw_frame bus_response;
static enum bus_state bus_state = BUS_READ_CLEAR;
static unsigned int bus_frame_offset;

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

    return KERN_INIT_SUCCESS;
    
fail_timer:
    
    return KERN_INIT_FAILED;
}

static void bus_rtask_execute(void)
{
	if(!bus_address_valid())
		return;

    switch(bus_state) {
        default:
        case BUS_READ_CLEAR:
            bus_frame_offset = 0;
            crc16_reset(&bus_crc16);
            memset(bus_response.data, 0, BUS_FRAME_SIZE);
            bus_state = BUS_READ_PART;
            break;
        case BUS_READ_PART:
            if(rs485_bytes_available()) { // Shouldn't strictly be necessary
                unsigned int size = rs485_read_buffer(
                    bus_request.data + bus_frame_offset, 
                    BUS_FRAME_SIZE - bus_frame_offset);
                crc16_update(&bus_crc16, bus_request.data + bus_frame_offset, size);
                bus_frame_offset += size;
                
                // Did we read a whole frame's worth of data?
                if(bus_frame_offset >= BUS_FRAME_SIZE)
                    bus_state = BUS_FRAME_VERIFY;
            }
            break;
        case BUS_FRAME_VERIFY:
#ifdef BUS_IGNORE_CRC
#warning "BUS_IGNORE_CRC defined"
            if(false)
#else
            // If CRC16 yields non-zero, then the frame is garbled, back off and reset
            if(bus_crc16)
#endif
                bus_state = BUS_ERROR;
            // Is frame a request and meant for us?
            else if(bus_request.frame.header.request && (
                    bus_request.frame.header.address == BUS_BROADCAST_ADDRESS ||
                    bus_request.frame.header.address == bus_address_get()))
                bus_state = BUS_FRAME_HANDLE;
            // Nope...
            else
                bus_state = BUS_READ_CLEAR;
            break;
        case BUS_FRAME_HANDLE: {
            bool broadcast = bus_request.frame.header.address == BUS_BROADCAST_ADDRESS;
            
            if(bus_request.frame.command >= bus_funcs_size)
                bus_response.frame.response_code = BUS_ERR_INVALID_COMMAND;
            else {
                bus_func_t handler = bus_funcs[bus_request.frame.command];
                ASSERT_NOT_NULL(handler);
                
                bus_response.frame.response_code = handler(
                    broadcast,
                    &bus_request.frame.payload,
                    &bus_response.frame.payload);
            }

            bus_state = broadcast ? BUS_READ_CLEAR : BUS_SEND_RESPONSE;
            break;
        }
        case BUS_SEND_RESPONSE:
            ASSERT(!bus_response.frame.header.request);
            ASSERT(bus_response.frame.header.address == 0); // It reality it doesn't really matter what value it is
            crc16_reset(&bus_response.frame.crc);
            crc16_update(&bus_response.frame.crc, bus_response.data, BUS_FRAME_SIZE - BUS_CRC_SIZE);
            rs485_transmit_buffer(bus_response.data, BUS_FRAME_SIZE); 
            bus_state = BUS_READ_CLEAR;
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
