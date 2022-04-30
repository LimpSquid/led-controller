#ifndef KERNEL_TASK_H
#define KERNEL_TASK_H

#include <stdbool.h>

#define KERN_RTASK(name, init_func, exec_func, config_func, level)      \
    static struct kernel_rtask_param                                    \
    __attribute__ ((used)) __rtask_param_##name =                       \
    {                                                                   \
        .next = ((void*)0),                                             \
        .init_level = level,                                            \
        .init_done = false,                                             \
    };                                                                  \
    static const struct kernel_rtask __rtask_##name                     \
    __attribute__ ((section(".kernel_rstack"), used)) =                 \
    {                                                                   \
        .init = init_func,                                              \
        .exec = exec_func,                                              \
        .configure = config_func,                                       \
        .param = &__rtask_param_##name,                                 \
    };

#define KERN_SIMPLE_RTASK(name, init_func, exec_func)                   \
    KERN_RTASK(name, init_func, exec_func, ((void*)0), KERN_INIT_LATE)

#define KERN_TTASK(name, init_func, exec_func, config_func, level)      \
    static struct kernel_ttask_param                                    \
    __attribute__ ((used)) __ttask_param_##name =                       \
    {                                                                   \
        .next = ((void*)0),                                             \
        .init_level = level,                                            \
        .init_done = false,                                             \
        .priority = KERN_TTASK_PRIORITY_NORMAL,                         \
        .interval = 0x7fffffffL,                                        \
        .exec_time_point = 0,                                           \
    };                                                                  \
    static const struct kernel_ttask __ttask_##name                     \
        __attribute__ ((section(".kernel_tstack"), used)) =             \
    {                                                                   \
        .init = init_func,                                              \
        .exec = exec_func,                                              \
        .configure = config_func,                                       \
        .param = &__ttask_param_##name,                                 \
    };

#define KERN_SIMPLE_TTASK(name, init_func, exec_func)                   \
    KERN_TTASK(name, init_func, exec_func, ((void*)0), KERN_INIT_LATE)

enum 
{
    KERN_INIT_LATE = 0,
    KERN_INIT_CORE,
    KERN_INIT_EARLY,
};

enum
{
    KERN_INIT_SUCCESS = 0,
    KERN_INIT_FAILED,
};

enum 
{
    KERN_TTASK_PRIORITY_LOW = 0,
    KERN_TTASK_PRIORITY_NORMAL,
    KERN_TTASK_PRIORITY_HIGH,
};

enum
{
    KERN_TIME_UNIT_S = 0,
    KERN_TIME_UNIT_MS,
    KERN_TIME_UNIT_US
};

struct kernel_rtask_param
{
    struct kernel_rtask const * next;
    int const init_level;

    bool init_done;
};

struct kernel_rtask
{
    int (*init)(void);
    void (*exec)(void);
    void (*configure)(struct kernel_rtask_param * const);

    struct kernel_rtask_param * const param;
};

struct kernel_ttask_param
{
    struct kernel_ttask const * next;
    int const init_level;

    int priority;
    int interval;
    long long exec_time_point;
    bool init_done;
    
};

struct kernel_ttask
{
    int (*init)(void);
    void (*exec)(void); 
    void (*configure)(struct kernel_ttask_param * const);

    struct kernel_ttask_param * const param;
};

void kernel_ttask_set_priority(struct kernel_ttask_param * const ttask_param, int priority);
void kernel_ttask_set_interval(struct kernel_ttask_param * const ttask_param, int time, int unit);

#endif /* KERNEL_TASK_H */