#include <bus.h>
#include <assert.h>
#include <timer.h>
#include <layer.h>
#include <sys.h>
#include <stddef.h>

#define BUS_FUNCS_SIZE      (sizeof(bus_funcs) / sizeof(bus_func_t))
#define UNUSED1(x)          ((void)x)
#define UNUSED2(x, y)       ((void)x);((void)y)
#define UNUSED3(x, y, z)    ((void)x);((void)y);((void)z)

enum
{
    BUS_COMMAND_LAYER_READY             = 0,
    BUS_COMMAND_LAYER_EXEC_LOD          = 1,
    BUS_COMMAND_LAYER_DMA_RESET         = 2,
    BUS_COMMAND_PING                    = 3,
    BUS_COMMAND_LAYER_DMA_SWAP_BUFFERS  = 4,
    BUS_COMMAND_SYS_VERSION             = 5,
    BUS_COMMAND_SYS_CPU_RESET           = 6,

    BUS_NUM_OF_COMMANDS // Must be last
};

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

    response_data->by_bytes.b1 = SYS_VERSION_MAJOR;
    response_data->by_bytes.b2 = SYS_VERSION_MINOR;
    response_data->by_bytes.b3 = SYS_VERSION_PATCH;
    response_data->by_bytes.b4 = 0; // Reserved
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

static enum bus_response_code bus_func_ping(
    bool broadcast,
    union bus_data const * request_data,
    union bus_data * response_data)
{
    UNUSED3(broadcast, request_data, response_data);

    return BUS_OK;
}

const bus_func_t bus_funcs[] =
{
    bus_func_layer_ready,
    bus_func_layer_exec_lod,
    bus_func_layer_dma_reset,
    bus_func_ping,
    bus_func_layer_dma_swap_buffers,
    bus_func_sys_version,
    bus_func_sys_cpu_reset,
};
size_t const bus_funcs_size = BUS_FUNCS_SIZE;

STATIC_ASSERT(BUS_FUNCS_SIZE == BUS_NUM_OF_COMMANDS);