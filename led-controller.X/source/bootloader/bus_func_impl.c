#include <bus.h>
#include <stddef.h>
#include <bootloader/bootloader.h>

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

static enum bus_response_code bus_func_bootloader_row_set_offset(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    UNUSED2(broadcast, response_data);

    bootloader_row_set_offset(request_data->by_uint32);
    return BUS_OK;
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

const bus_func_t bus_funcs[] =
{
    bus_func_status,                    // 128
    bus_func_bootloader_info,           // 129
    bus_func_bootloader_erase,          // ...
    bus_func_bootloader_row_set_offset,
    bus_func_bootloader_row_push_word,
    bus_func_bootloader_row_burn,
};
size_t const bus_funcs_size = BUS_FUNCS_SIZE;
size_t const bus_funcs_start = 128;