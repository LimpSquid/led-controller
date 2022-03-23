#include <bus.h>
#include <bus_address.h>
#include <kernel_task.h>
#include <assert_util.h>
#include <rs485.h>
#include <timer.h>
#include <util.h>
#include <stddef.h>
#include <string.h>

#define BUS_FRAME_SIZE              sizeof(struct bus_frame)
#define BUS_CRC_SIZE                sizeof(crc16_t)
#define BUS_FRAME_PART_DEADLINE     5 // In milliseconds, maximum allowed time between two reads
#define BUS_BROADCAST_ADDRESS       32

struct bus_header
{
    unsigned char request   :1; // Response if false

    // Must be specified for both request and response, the slave that
    // is sending the response must set this variable to its own address
    unsigned char address   :6;
    unsigned char           :1;
};
STATIC_ASSERT(sizeof(struct bus_header) == 1)

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
STATIC_ASSERT(sizeof(struct bus_frame) == 8)

union bus_raw_frame
{
    struct bus_frame frame;
    unsigned char data[BUS_FRAME_SIZE];
};
STATIC_ASSERT(sizeof(union bus_raw_frame) == BUS_FRAME_SIZE)

enum bus_state
{
    BUS_READ_CLEAR = 0,
    BUS_READ_PART,
    BUS_FRAME_VERIFY,
    BUS_FRAME_HANDLE,
    BUS_SEND_RESPONSE,

    BUS_ERROR,
    BUS_ERROR_RESET,
};

static void bus_error_callback(struct rs485_error);
static int bus_rtask_init(void);
static void bus_rtask_execute(void);
KERN_SIMPLE_RTASK(bus, bus_rtask_init, bus_rtask_execute)

extern const bus_func_t bus_funcs[];
extern const size_t bus_funcs_size;
static struct rs485_error_notifier bus_error_notifier =
{
    .callback = bus_error_callback
};

static struct timer_module * bus_timer;
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
    bus_timer = timer_construct(TIMER_TYPE_COUNTDOWN, NULL);
    if (bus_timer == NULL)
        goto fail_timer;

    return KERN_INIT_SUCCESS;

fail_timer:

    return KERN_INIT_FAILED;
}

static void bus_rtask_execute(void)
{
    if (!bus_address_valid())
        return;

    switch (bus_state) {
        default:
        case BUS_READ_CLEAR:
            bus_frame_offset = 0;
            crc16_reset(&bus_crc16);
            memset(bus_response.data, 0, BUS_FRAME_SIZE);
            bus_state = BUS_READ_PART;
            break;
        case BUS_READ_PART:
            if (bus_frame_offset && !timer_is_running(bus_timer))
                bus_state = BUS_READ_CLEAR; // Did not receive a complete frame within deadline, drop it
            else if (rs485_bytes_available()) { // Shouldn't strictly be necessary
                unsigned int size = rs485_read_buffer(
                    bus_request.data + bus_frame_offset,
                    BUS_FRAME_SIZE - bus_frame_offset);
                crc16_update(&bus_crc16, bus_request.data + bus_frame_offset, size);
                bus_frame_offset += size;

                // Did we read a whole frame's worth of data?
                if(bus_frame_offset == BUS_FRAME_SIZE) {
                    timer_stop(bus_timer);
                    bus_state = BUS_FRAME_VERIFY;
                } else
                    timer_start(bus_timer, BUS_FRAME_PART_DEADLINE, TIMER_TIME_UNIT_MS);
            }
            break;
        case BUS_FRAME_VERIFY:
#ifdef BUS_IGNORE_CRC
#warning "BUS_IGNORE_CRC defined"
            if (false)
#else
            // If CRC16 yields non-zero, then the frame is garbled, back off and reset
            if (bus_crc16)
#endif
                bus_state = BUS_ERROR;
            // Is frame a request and meant for us?
            else if (bus_request.frame.header.request && (
                     bus_request.frame.header.address == BUS_BROADCAST_ADDRESS ||
                     bus_request.frame.header.address == bus_address_get()))
                bus_state = BUS_FRAME_HANDLE;
            // Nope...
            else
                bus_state = BUS_READ_CLEAR;
            break;
        case BUS_FRAME_HANDLE: {
            bool broadcast = bus_request.frame.header.address == BUS_BROADCAST_ADDRESS;

            if (bus_request.frame.command >= bus_funcs_size)
                bus_response.frame.response_code = BUS_ERR_INVALID_COMMAND;
            else {
                bus_func_t handler = bus_funcs[bus_request.frame.command];
                bus_response.frame.response_code = (handler == NULL)
                    ? BUS_ERR_INVALID_COMMAND
                    : handler(broadcast, &bus_request.frame.payload, &bus_response.frame.payload);
            }

            bus_state = broadcast ? BUS_READ_CLEAR : BUS_SEND_RESPONSE;
            break;
        }
        case BUS_SEND_RESPONSE:
            ASSERT(!bus_response.frame.header.request);
            bus_response.frame.header.address = bus_address_get();
            crc16_reset(&bus_response.frame.crc);
            crc16_update(&bus_response.frame.crc, bus_response.data, BUS_FRAME_SIZE - BUS_CRC_SIZE);
            rs485_transmit_buffer(bus_response.data, BUS_FRAME_SIZE);
            bus_state = BUS_READ_CLEAR;
            break;

        case BUS_ERROR:
        case BUS_ERROR_RESET:
            rs485_reset();
            bus_state = BUS_READ_CLEAR;
            break;
    }
}
