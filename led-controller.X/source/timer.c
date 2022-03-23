#include <timer.h>
#include <timer_config.h>
#include <kernel_task.h>
#include <assert_util.h>
#include <util.h>
#include <stddef.h>
#include <limits.h>

#if !defined(TIMER_TICK_INTERVAL)
    #error "Timer tick interval is not specified, please define 'TIMER_TICK_INTERVAL'"
#elif !defined(TIMER_POOL_SIZE)
    #error "Timer pool size is not specified, please define 'TIMER_POOL_SIZE'"
#endif

STATIC_ASSERT(TIMER_TICK_INTERVAL > 0)
STATIC_ASSERT(TIMER_POOL_SIZE > 0)

#define US_TO_TICKS(us) (1 + ((us - 1) / TIMER_TICK_INTERVAL)) // Ceiled division, us must be a positive natural number
#define TIMER_SEC_MAX   (BIT_SHIFT(12) - 1) // Time unit seconds is limited to 12 bits

struct timer_module
{
    unsigned long interval;
    unsigned long ticks;
    void(*execute)(struct timer_module * timer);

    struct
    {
        unsigned char type      :3;
        unsigned char assigned  :1;
        unsigned char suspended :1;
        unsigned char           :2;
    } opt;
};

static void timer_ttask_execute(void);
static void timer_ttask_configure(struct kernel_ttask_param * const param);
KERN_TTASK(timer, NULL, timer_ttask_execute, timer_ttask_configure, KERN_INIT_EARLY)

static struct timer_module timer_pool[TIMER_POOL_SIZE];

static unsigned long timer_compute_ticks(int time, int unit)
{
    if (time <= 0)
        return 0;

    unsigned long ticks;
    switch (unit) {
        default: // Default to seconds
        case TIMER_TIME_UNIT_S:
            ASSERT(time <= TIMER_SEC_MAX);
            if (time > TIMER_SEC_MAX)
                time = TIMER_SEC_MAX;
            ticks = US_TO_TICKS(time * 1000000LU);
            break;
        case TIMER_TIME_UNIT_MS:
            ticks = US_TO_TICKS(time * 1000LU);
            break;
        case TIMER_TIME_UNIT_US:
            ticks = US_TO_TICKS(time);
            break;
    }
    return ticks;
}

static void timer_ttask_execute(void)
{
    void (*execute)(struct timer_module *) = NULL;

    struct timer_module * timer = timer_pool;
    for (unsigned int i = 0; i < TIMER_POOL_SIZE; ++i) {
        if (timer->opt.assigned && !timer->opt.suspended) {

            bool timed_out = true;
            if (timer->ticks > 0)
                timed_out = !!!(--timer->ticks);

            if (timed_out) {
                switch (timer->opt.type) {
                    case TIMER_TYPE_RECURRING:
                        if (execute == NULL) { // Yay, we can execute this timer's handle
                            timer->ticks = timer->interval; // Reset tick count
                            execute = timer->execute;
                        }
                        break;
                    case TIMER_TYPE_SINGLE_SHOT:
                        if (execute == NULL) {
                            timer->opt.suspended = true;
                            execute = timer->execute;
                        }
                        break;
                    case TIMER_TYPE_COUNTDOWN:
                        timer->opt.suspended = true;
                        break;
                    default:
                        break;
                }
            }
        }
        timer++; // Advance to next timer
    }

    if (execute != NULL)
        execute(timer);
}

static void timer_ttask_configure(struct kernel_ttask_param * const param)
{
    kernel_ttask_set_priority(param, KERN_TTASK_PRIORITY_HIGH);
    kernel_ttask_set_interval(param, TIMER_TICK_INTERVAL, KERN_TIME_UNIT_US);
}

struct timer_module * timer_construct(int type, void (*execute)(struct timer_module *))
{
    struct timer_module * timer = NULL;
    if (type < 0 || type >= __TIMER_TYPE_COUNT)
        return timer;

    // Search for an unused timer
    for (unsigned int i = 0; i < TIMER_POOL_SIZE; ++i) {
        if (!timer_pool[i].opt.assigned) {
            timer = &timer_pool[i];
            break;
        }
    }

    // Useful for debugging in case the pool has run out of timers
    ASSERT_NOT_NULL(timer);

    // Configure timer, if found
    if (timer != NULL) {
        timer->interval = 0;
        timer->ticks = 0;
        timer->execute = execute;
        timer->opt.type = type;
        timer->opt.suspended = true;
        timer->opt.assigned = true;
    }
    return timer;
}

void timer_destruct(struct timer_module * timer)
{
    ASSERT_NOT_NULL(timer);

    timer->opt.assigned = false;
}

void timer_set_time(struct timer_module * timer, int time, int unit)
{
    ASSERT_NOT_NULL(timer);

    timer->interval = timer_compute_ticks(time, unit);
    timer->ticks = timer->interval;
}

void timer_start(struct timer_module * timer, int time, int unit)
{
    ASSERT_NOT_NULL(timer);

    timer_set_time(timer, time, unit);
    timer->opt.suspended = false;
}

void timer_stop(struct timer_module * timer)
{
    ASSERT_NOT_NULL(timer);

    timer->opt.suspended = true;
}

void timer_restart(struct timer_module * timer)
{
    ASSERT_NOT_NULL(timer);

    timer->ticks = timer->interval;
    timer->opt.suspended = false;
}

bool timer_is_running(const struct timer_module * timer)
{
    ASSERT_NOT_NULL(timer);

    return !timer->opt.suspended;
}

bool timer_is_valid(const struct timer_module * timer)
{
    ASSERT_NOT_NULL(timer);

    return timer->opt.assigned;
}