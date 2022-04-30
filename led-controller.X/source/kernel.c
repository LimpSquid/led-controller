#include <kernel.h>
#include <kernel_task.h>
#include <kernel_config.h>
#include <assert.h>
#include <util.h>
#include <stddef.h>
#include <xc.h>

#if !defined(KERN_TMR_REG)
    #error "Timer register not specified, please define 'KERN_TMR_REG'"
#elif !defined(KERN_TMR_REG_DATA_TYPE)
    #error "Timer register data type register not specified, please define 'KERN_TMR_REG_DATA_TYPE'"
#elif !defined(KERN_TMR_CFG_REG)
    #error "Timer config register not specified, please define 'KERN_TMR_CFG_REG'"
#elif !defined(KERN_TMR_CFG_WORD)
    #error "Timer config register word not specified, please define 'KERN_TMR_CFG_WORD'"
#elif !defined(KERN_TMR_EN_BIT)
    #error "Timer enable bit from config register not specified, please define 'KERN_TMR_EN_BIT'"
#elif !defined(KERN_TMR_CLKIN_FREQ)
    #error "System tick could not be calculated, please define 'KERN_TMR_CLKIN_FREQ'"
#endif

// Data type must be unsigned for defined behaviour.
// See A.1 of https://www.gnu.org/software/gnu-c-manual/gnu-c-manual.html#Integer-Overflow-Basics
STATIC_ASSERT((KERN_TMR_REG_DATA_TYPE)(-1) > 0)

#define KERNEL_SYSTEM_TICK              ((1000000.0 / KERN_TMR_CLKIN_FREQ) * KERN_TMR_PRESCALER) // Microseconds per tick
#define KERNEL_JITTER_AVOIDANCE_COEFF   (33 / KERNEL_SYSTEM_TICK) // 33us

typedef KERN_TMR_REG_DATA_TYPE timer_size_t;

// Nice info explaining this: https://mcuoneclipse.com/2016/11/01/getting-the-memory-range-of-sections-with-gnu-linker-files/
extern struct kernel_rtask const __kernel_rstack_begin;
extern struct kernel_rtask const __kernel_rstack_end;
extern struct kernel_ttask const __kernel_tstack_begin;
extern struct kernel_ttask const __kernel_tstack_end;

static struct kernel_rtask const * kernel_rtask_iterator = &__kernel_rstack_begin;
static struct kernel_rtask const * const kernel_rtask_end = &__kernel_rstack_end;
static struct kernel_ttask const * kernel_ttask_iterator = &__kernel_tstack_begin;
static struct kernel_ttask const * const kernel_ttask_end = &__kernel_tstack_end;

static struct kernel_ttask const * kernel_ttask_cursor = NULL;

static void (*kernel_exec_func)(void);

inline static void __attribute__((always_inline)) kernel_restore_rtask_iterator()
{
    kernel_rtask_iterator = &__kernel_rstack_begin;
}

inline static void __attribute__((always_inline)) kernel_restore_ttask_iterator()
{
    kernel_ttask_iterator = &__kernel_tstack_begin;
}

inline static int __attribute__((always_inline)) kernel_has_rtasks()
{
    return (int)(&__kernel_rstack_end - &__kernel_rstack_begin) != 0;
}

inline static int __attribute__((always_inline)) kernel_has_ttasks()
{
    return (int)(&__kernel_tstack_end - &__kernel_tstack_begin) != 0;
}

static void kernel_init_configure_ttask(void)
{
    int x = 0;

    while (kernel_ttask_iterator != kernel_ttask_end) {
        if (kernel_ttask_iterator->configure != NULL)
            kernel_ttask_iterator->configure(kernel_ttask_iterator->param);

        // Suppose that two tasks had the same execution interval, meaning that the kernel has
        // to service two tasks at exactly the same time. This isn't something we can actually
        // do with only one CPU and would've resulted in one task being delayed by the execution
        // time of the other. A task's execution time is of jittery nature, e.g. sometimes a
        // task has to do nothing (zero CPU time), sometimes it needs to do a lot (a significant
        // amount of CPU time). Because of this, the 2nd task (which immediately runs after
        // the completion of the 1st task) will also experience this jitter. Obviously this
        // is something we want to avoid as much as possible. By introducing an initial
        // execution time offset we will make sure that tasks with the same interval will not
        // be scheduled/serviced at the same point in time, thus preventing task jitter.
        kernel_ttask_iterator->param->exec_time_point = (x++ * KERNEL_JITTER_AVOIDANCE_COEFF);
        kernel_ttask_iterator++;
    }
    kernel_restore_ttask_iterator();
}

static void kernel_init_configure_rtask(void)
{
    while (kernel_rtask_iterator != kernel_rtask_end) {
        if (kernel_rtask_iterator->configure != NULL)
            kernel_rtask_iterator->configure(kernel_rtask_iterator->param);
        kernel_rtask_iterator++;
    }
    kernel_restore_rtask_iterator();
}

static void kernel_init_ttask_call_sequence(void)
{
    ASSERT(kernel_has_ttasks());
    struct kernel_ttask const ** ttask = NULL;

    for (int priority = KERN_TTASK_PRIORITY_HIGH; --priority >= 0;) {
        while (kernel_ttask_iterator != kernel_ttask_end) {
            if (kernel_ttask_iterator->param->priority == priority) {
                if (ttask)
                    *ttask = kernel_ttask_iterator;
                else
                    kernel_ttask_cursor = kernel_ttask_iterator; // First ttask in call sequence
                ttask = &kernel_ttask_iterator->param->next;
            }
            kernel_ttask_iterator++;
        }

        kernel_restore_ttask_iterator();
    }

    // Point last element to start
    *ttask = kernel_ttask_cursor;
}

static void kernel_init_rtask_call_sequence(void)
{
    ASSERT(kernel_has_rtasks());
    struct kernel_rtask const * rtask_last = (&__kernel_rstack_end) - 1;
    struct kernel_rtask const ** rtask = &rtask_last->param->next; // Point last element to start

    while (kernel_rtask_iterator != kernel_rtask_end) {
        *rtask = kernel_rtask_iterator;
        rtask = &kernel_rtask_iterator->param->next;
        kernel_rtask_iterator++;
    }
    kernel_restore_rtask_iterator();
}

static void kernel_init_task_init(void)
{
    int init_level = KERN_INIT_EARLY;
    do {
        int (*init)(void) = NULL;
        bool * init_done = NULL;

        // Find init function with current init_level in robin tasks
        while (kernel_rtask_iterator != kernel_rtask_end) {
            if (kernel_rtask_iterator->param->init_level == init_level &&
                !kernel_rtask_iterator->param->init_done) {
                init = kernel_rtask_iterator->init;
                init_done = &kernel_rtask_iterator->param->init_done;
                break;
            }
            kernel_rtask_iterator++;
        }

        // Find init function with current init_level in timed tasks, these will get precedence over robin tasks
        while (kernel_ttask_iterator != kernel_ttask_end) {
            if (kernel_ttask_iterator->param->init_level == init_level &&
                !kernel_ttask_iterator->param->init_done) {
                init = kernel_ttask_iterator->init;
                init_done = &kernel_ttask_iterator->param->init_done;
                break;
            }
            kernel_ttask_iterator++;
        }

        // Check if we have found an init function to process...
        if (init_done != NULL) {
            if (init != NULL) {
                int result = init();
                ASSERT(result == KERN_INIT_SUCCESS);
                SYS_FAIL_IF_NOT(result == KERN_INIT_SUCCESS)
            }
            *init_done = true;
        } else // If not, we go to the next init_level
            init_level--;

        kernel_restore_rtask_iterator();
        kernel_restore_ttask_iterator();
    } while (init_level >= 0);
}

inline static void __attribute__((always_inline)) kernel_execute_ttask(void)
{
    static struct kernel_ttask const * ttask_end = NULL;
    static struct kernel_ttask_param * param = NULL;
    static long long kernel_ticks = 0;
    static timer_size_t kernel_elapsed_ticks = 0;

    ttask_end = kernel_ttask_cursor;
    kernel_elapsed_ticks = KERN_TMR_REG - ((timer_size_t)kernel_ticks);
    kernel_ticks += kernel_elapsed_ticks;

    do {
        param = kernel_ttask_cursor->param;
        if (kernel_ticks >= param->exec_time_point) {
            param->exec_time_point += param->interval;
            kernel_ttask_cursor->exec();
        }

        kernel_ttask_cursor = param->next;
    } while (kernel_ttask_cursor != ttask_end);
}

inline static void __attribute__((always_inline)) kernel_execute_rtask(void)
{
    static struct kernel_rtask const * kernel_rtask_cursor = &__kernel_rstack_begin;

    kernel_rtask_cursor->exec();
    kernel_rtask_cursor = kernel_rtask_cursor->param->next;
}

static void kernel_execute_ttask_rtask(void)
{
    kernel_execute_ttask();
    kernel_execute_rtask();
}

static void kernel_execute_no_task(void)
{
    Nop();
}

static int kernel_compute_sys_ticks(int time, int unit)
{
    int ticks;
    switch (unit) {
        default: // Default to seconds
        case KERN_TIME_UNIT_S:  ticks = (time * 1000000LU) / KERNEL_SYSTEM_TICK;    break;
        case KERN_TIME_UNIT_MS: ticks = (time * 1000LU) / KERNEL_SYSTEM_TICK;       break;
        case KERN_TIME_UNIT_US: ticks = time / KERNEL_SYSTEM_TICK;                  break;
    }
    return ticks;
}

void kernel_init(void)
{
    kernel_exec_func = kernel_execute_no_task;

    // Configure ttasks
    if (kernel_has_ttasks()) {
        kernel_init_configure_ttask();
        kernel_init_ttask_call_sequence();
        kernel_exec_func = kernel_has_rtasks()
            ? kernel_execute_ttask_rtask
            : kernel_execute_ttask;
    }

    // Configure rtasks
    if (kernel_has_rtasks()) {
        kernel_init_configure_rtask();
        kernel_init_rtask_call_sequence();
        kernel_exec_func = kernel_has_ttasks()
            ? kernel_execute_ttask_rtask
            : kernel_execute_rtask;
    }

    kernel_init_task_init();

    // Configure timer after init
    if (kernel_has_ttasks()) {
        REG_CLR(KERN_TMR_CFG_REG, KERN_TMR_EN_BIT);
        KERN_TMR_CFG_REG = KERN_TMR_CFG_WORD;
        REG_SET(KERN_TMR_CFG_REG, KERN_TMR_EN_BIT);
    }
}

void kernel_execute(void)
{
    (*kernel_exec_func)();
}

void kernel_ttask_set_priority(struct kernel_ttask_param * const ttask_param, int priority)
{
    if (ttask_param != NULL)
        ttask_param->priority = (priority < 0 || priority)
            ? KERN_TTASK_PRIORITY_NORMAL
            : priority;
}

void kernel_ttask_set_interval(struct kernel_ttask_param * const ttask_param, int time, int unit)
{
    if (ttask_param != NULL)
        ttask_param->interval = kernel_compute_sys_ticks(time, unit);
}
