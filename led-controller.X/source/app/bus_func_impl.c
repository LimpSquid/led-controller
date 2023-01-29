#include <core/bus.h>
#include <core/assert.h>
#include <core/timer.h>
#include <core/sys.h>
#include <app/layer.h>
#include <version.h>
#include <stddef.h>

#define BUS_FUNCS_SIZE      (sizeof(bus_funcs) / sizeof(bus_func_t))
#define UNUSED1(x)          ((void)x)
#define UNUSED2(x, y)       ((void)x);((void)y)
#define UNUSED3(x, y, z)    ((void)x);((void)y);((void)z)

static enum bus_response_code bus_func_layer_auto_buffer_swap(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    UNUSED2(broadcast, response_data);

    bool enable = request_data->by_bool;
    if (enable)
        layer_set_buffer_swap_mode(LAYER_BUFFER_SWAP_AUTO);
    else
        layer_set_buffer_swap_mode(LAYER_BUFFER_SWAP_MANUAL);
    return BUS_OK;
}

static enum bus_response_code bus_func_layer_exec_lod(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    UNUSED3(broadcast, request_data, response_data);

    return layer_exec_lod()
        ? BUS_OK
        : BUS_ERR_AGAIN;
}

static enum bus_response_code bus_func_layer_dma_reset(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    UNUSED3(broadcast, request_data, response_data);

    layer_dma_reset();
    return BUS_OK;
}

static enum bus_response_code bus_func_layer_dma_swap_buffers(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    UNUSED3(broadcast, request_data, response_data);

    // TODO: if auto buffer swap is enabled, return different error?
    return layer_dma_swap_buffers()
        ? BUS_OK
        : BUS_ERR_AGAIN;
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

static void bus_delayed_cpu_reset(struct timer_module * timer)
{
    if (!bus_idle())
        return timer_start(timer, 25, TIMER_TIME_UNIT_MS); // Try again

    timer_destruct(timer); // Let's be nice and destruct the timer even though we are going to reset :-)
    sys_cpu_reset();
}

static enum bus_response_code bus_func_sys_cpu_reset(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    UNUSED2(broadcast, response_data);

    // Hard reset
    if (request_data->by_int32 < 0)
        sys_cpu_reset();

    struct timer_module * timer = timer_construct(TIMER_TYPE_SINGLE_SHOT, bus_delayed_cpu_reset);
    if (timer == NULL)
        return BUS_ERR_AGAIN;

    timer_start(timer, request_data->by_int32, TIMER_TIME_UNIT_MS);
    return BUS_OK;
}

static enum bus_response_code bus_func_status(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    UNUSED2(broadcast, request_data);

    response_data->by_app_status.layer_ready = layer_ready();
    response_data->by_app_status.layer_dma_error = layer_dma_error();
    response_data->by_app_status.layer_auto_buffer_swap =
        layer_get_buffer_swap_mode() == LAYER_BUFFER_SWAP_AUTO;

    return BUS_OK;
}

static enum bus_response_code bus_func_layer_clear(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    UNUSED3(broadcast, request_data, response_data);

    layer_clear_all_pixels();
    return BUS_OK;
}

bus_func_t const bus_funcs[] =
{
    bus_func_layer_auto_buffer_swap,    // 0
    bus_func_layer_exec_lod,            // 1
    bus_func_layer_dma_reset,           // 2
    bus_func_status,                    // 3
    bus_func_layer_dma_swap_buffers,    // 4
    bus_func_version,                   // 5
    bus_func_sys_cpu_reset,             // 6
    bus_func_layer_clear,               // 7
};
size_t const bus_funcs_size = BUS_FUNCS_SIZE;
size_t const bus_funcs_start = 0;