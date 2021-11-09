#include <bus.h>
#include <layer.h>
#include <stddef.h>

#define UNUSED(x)   ((void)x)

enum
{
    BUS_COMMAND_LAYER_READY = 0,
    BUS_COMMAND_LAYER_EXEC_LOD,
};

static enum bus_response_code bus_func_layer_ready(bool broadcast, union bus_data* data)
{
    UNUSED(broadcast);

    data->by_bool = layer_ready();
    return BUS_OK;
}

static enum bus_response_code bus_func_layer_exec_lod(bool broadcast, union bus_data* data)
{
    UNUSED(broadcast);
    UNUSED(data);

    return (layer_ready() && layer_exec_lod()) ? BUS_OK : BUS_ERR_AGAIN;
}

const bus_func_t bus_funcs[] =
{
    [BUS_COMMAND_LAYER_READY]       = bus_func_layer_ready,
    [BUS_COMMAND_LAYER_EXEC_LOD]    = bus_func_layer_exec_lod,
};
const size_t bus_funcs_size = sizeof(bus_funcs) / sizeof(bus_func_t);