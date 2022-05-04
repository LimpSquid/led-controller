#include <bus.h>
#include <assert.h>
#include <timer.h>
#include <sys.h>
#include <stddef.h>
#include <app/layer.h>

#define BUS_FUNCS_SIZE      (sizeof(bus_funcs) / sizeof(bus_func_t))
#define UNUSED1(x)          ((void)x)
#define UNUSED2(x, y)       ((void)x);((void)y)
#define UNUSED3(x, y, z)    ((void)x);((void)y);((void)z)

static enum bus_response_code bus_func_layer_ready(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    UNUSED2(broadcast, request_data);

    response_data->by_bool = layer_ready();
    return BUS_OK;
}

static enum bus_response_code bus_func_layer_exec_lod(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    UNUSED3(broadcast, request_data, response_data);

    return layer_exec_lod() ? BUS_OK : BUS_ERR_AGAIN;
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

    return layer_dma_swap_buffers() ? BUS_OK : BUS_ERR_AGAIN;
}

static enum bus_response_code bus_func_sys_version(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    STATIC_ASSERT(SYS_VERSION_MAJOR >= 0 && SYS_VERSION_MAJOR <= 255)
    STATIC_ASSERT(SYS_VERSION_MINOR >= 0 && SYS_VERSION_MINOR <= 255)
    STATIC_ASSERT(SYS_VERSION_PATCH >= 0 && SYS_VERSION_PATCH <= 255)
    UNUSED2(broadcast, request_data);

    response_data->by_version.major = SYS_VERSION_MAJOR;
    response_data->by_version.minor = SYS_VERSION_MINOR;
    response_data->by_version.patch = SYS_VERSION_PATCH;
    return BUS_OK;
}

static void bus_delayed_cpu_reset(struct timer_module * timer)
{
    timer_destruct(timer); // Lets be nice and destruct the timer even though we are going to reset :-)
    sys_cpu_reset();
}

static enum bus_response_code bus_func_sys_cpu_reset(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    UNUSED2(broadcast, response_data);

    if (request_data->by_int32 > 0) {
        struct timer_module * timer = timer_construct(TIMER_TYPE_SINGLE_SHOT, bus_delayed_cpu_reset);
        if (timer == NULL)
            return BUS_ERR_AGAIN;
        timer_start(timer, request_data->by_int32, TIMER_TIME_UNIT_MS);
    } else
        sys_cpu_reset();
    return BUS_OK;
}

static enum bus_response_code bus_func_status(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    UNUSED2(broadcast, request_data);

    response_data->by_status.layer_ready = layer_ready();
    response_data->by_status.layer_dma_error = layer_dma_error();
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

const bus_func_t bus_funcs[] =
{
    NULL,                               // 0
    bus_func_layer_exec_lod,            // 1
    bus_func_layer_dma_reset,           // ...
    bus_func_status,
    bus_func_layer_dma_swap_buffers,
    bus_func_sys_version,
    bus_func_sys_cpu_reset,
    bus_func_layer_clear,
};
size_t const bus_funcs_size = BUS_FUNCS_SIZE;
size_t const bus_funcs_start = 0;