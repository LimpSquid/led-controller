#include <core/bus.h>
#include <core/assert.h>
#include <bootloader/bootloader.h>
#include <version.h>
#include <stddef.h>

#define BUS_FUNCS_SIZE      (sizeof(bus_funcs) / sizeof(bus_func_t))
#define UNUSED1(x)          ((void)x)
#define UNUSED2(x, y)       ((void)x);((void)y)
#define UNUSED3(x, y, z)    ((void)x);((void)y);((void)z)

static enum bus_response_code bus_func_status(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    UNUSED2(broadcast, request_data);

    response_data->by_bootloader_status.bootloader_ready = bootloader_ready();
    response_data->by_bootloader_status.bootloader_error = bootloader_error();
    response_data->by_bootloader_status.bootloader_waiting_for_magic = bootloader_waiting_for_magic();
    return BUS_OK;
}

static enum bus_response_code bus_func_bootloader_info(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    UNUSED1(broadcast);

    unsigned int result;
    if (!bootloader_info(request_data->by_uint8, &result))
        return BUS_ERR_INVALID_PAYLOAD;
    response_data->by_uint32 = result;
    return BUS_OK;
}

static enum bus_response_code bus_func_bootloader_erase(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    UNUSED3(broadcast, request_data, response_data);

    return bootloader_erase()
        ? BUS_OK
        : BUS_ERR_AGAIN;
}

static enum bus_response_code bus_func_bootloader_set_magic(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    UNUSED2(broadcast, response_data);

    return bootloader_set_magic(request_data->by_uint32)
        ? BUS_OK
        : BUS_ERR_INVALID_PAYLOAD;
}

static enum bus_response_code bus_func_bootloader_boot(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    UNUSED3(broadcast, request_data, response_data);

    return bootloader_boot()
        ? BUS_OK
        : BUS_ERR_AGAIN;
}

static enum bus_response_code bus_func_bootloader_row_reset(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    UNUSED3(broadcast, request_data, response_data);

    return bootloader_row_reset()
        ? BUS_OK
        : BUS_ERR_AGAIN;
}

static enum bus_response_code bus_func_bootloader_row_push_word(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    UNUSED2(broadcast, response_data);

    return bootloader_row_push_word(request_data->by_uint32)
        ? BUS_OK
        : BUS_ERR_AGAIN;
}

static enum bus_response_code bus_func_bootloader_row_burn(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    UNUSED3(broadcast, request_data, response_data);

    return bootloader_row_burn(request_data->by_uint32)
        ? BUS_OK
        : BUS_ERR_AGAIN;
}

static enum bus_response_code bus_func_bootloader_row_crc16(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    UNUSED2(broadcast, request_data);

    unsigned short result;
    if (!bootloader_row_crc16(&result))
        return BUS_ERR_AGAIN;
    response_data->by_uint16 = result;
    return BUS_OK;
}

static enum bus_response_code bus_func_version(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    STATIC_ASSERT(VERSION_MAJOR >= 0 && VERSION_MAJOR <= 255)
    STATIC_ASSERT(VERSION_MINOR >= 0 && VERSION_MINOR <= 255)
    STATIC_ASSERT(VERSION_PATCH >= 0 && VERSION_PATCH <= 255)
    UNUSED2(broadcast, request_data);

    response_data->by_version.major = VERSION_MAJOR;
    response_data->by_version.minor = VERSION_MINOR;
    response_data->by_version.patch = VERSION_PATCH;
    return BUS_OK;
}

bus_func_t const bus_funcs[] =
{
    bus_func_status,                    // 128
    bus_func_bootloader_info,           // 129
    bus_func_bootloader_erase,          // 130
    bus_func_bootloader_set_magic,      // 131
    bus_func_bootloader_boot,           // 132
    bus_func_bootloader_row_reset,      // 133
    bus_func_bootloader_row_push_word,  // 134
    bus_func_bootloader_row_burn,       // 135
    bus_func_bootloader_row_crc16,      // 136
    bus_func_version,                   // 137
};
size_t const bus_funcs_size = BUS_FUNCS_SIZE;
size_t const bus_funcs_start = 128;