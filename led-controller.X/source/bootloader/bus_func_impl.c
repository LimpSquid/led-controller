#include <bus.h>
#include <assert.h>

#define BUS_FUNCS_SIZE      (sizeof(bus_funcs) / sizeof(bus_func_t))
#define UNUSED1(x)          ((void)x)
#define UNUSED2(x, y)       ((void)x);((void)y)
#define UNUSED3(x, y, z)    ((void)x);((void)y);((void)z)

const bus_func_t bus_funcs[] =
{
    NULL,   // 0
    NULL,   // 1
    NULL,   // ...
};
size_t const bus_funcs_size = BUS_FUNCS_SIZE;
size_t const bus_funcs_start = 128;