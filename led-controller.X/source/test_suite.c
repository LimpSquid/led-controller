#ifdef TEST_SUITE_ENABLE
#warning "TEST_SUITE_ENABLE defined"
#include <kernel_task.h>
#include <layer.h>
#include <timer.h>
#include <stddef.h>
#include <stdlib.h>

#define TEST_SUITE_X_PIXELS             16
#define TEST_SUITE_Y_PIXELS             16
#define TEST_SUITE_NUM_OF_CYCLE_COLORS  (sizeof(test_suite_cycle_colors) / sizeof(test_suite_cycle_colors[0]))

#define TEST_SUITE_CYCLE_COLORS_DELAY   2000 // In milliseconds
#define TEST_SUITE_FINISHED_DELAY       250 // In milliseconds

enum test_suite_state
{
    TEST_SUITE_INIT = 0,
    
    // Test for open LEDs
    TEST_SUITE_EXEC_LOD,
    TEST_SUITE_EXEC_LOD_WAIT,
    
    // Cycle through different colors
    TEST_SUITE_CYCLE_COLORS_INIT,
    TEST_SUITE_CYCLE_COLORS_DRAW,
    
    // Generic stuff down below
    TEST_SUITE_FINISHED
};

static int test_suite_init(void);
static void test_suite_execute(void);
KERN_SIMPLE_RTASK(test_suite, test_suite_init, test_suite_execute);

static const struct layer_color test_suite_cycle_colors[] = 
{
    { .r = 255, .g = 000, .b = 000 },
    { .r = 000, .g = 255, .b = 000 },
    { .r = 000, .g = 000, .b = 255 },
    { .r = 255, .g = 255, .b = 000 },
    { .r = 255, .g = 000, .b = 255 },
    { .r = 000, .g = 255, .b = 255 },
    { .r = 000, .g = 000, .b = 000 }, // Give eyes some time to adjust before looking at the dim white color
    { .r = 010, .g = 010, .b = 010 },
    { .r = 063, .g = 063, .b = 063 },
    { .r = 127, .g = 127, .b = 127 },
    { .r = 255, .g = 255, .b = 255 },
};

static struct timer_module* test_suite_countdown_timer = NULL;
static enum test_suite_state test_suite_state = TEST_SUITE_INIT;
static unsigned int test_suite_generic_uint = 0;

static int test_suite_init(void)
{
    // Initialize timer
    test_suite_countdown_timer = timer_construct(TIMER_TYPE_COUNTDOWN, NULL);
    if(test_suite_countdown_timer == NULL)
        goto fail_timer;
 
    return KERN_INIT_SUCCESS;

fail_timer:

    return KERN_INIT_FAILED;
}

static void test_suite_execute(void)
{
    if(timer_is_running(test_suite_countdown_timer))
        return;
            
    switch(test_suite_state) {
        default:
        case TEST_SUITE_INIT:
            if(layer_ready())
                test_suite_state = TEST_SUITE_EXEC_LOD;
            break;
            
        case TEST_SUITE_EXEC_LOD:
            if(layer_exec_lod())
                test_suite_state = TEST_SUITE_EXEC_LOD_WAIT;
            break;
        case TEST_SUITE_EXEC_LOD_WAIT:
            if(layer_ready())
                test_suite_state = TEST_SUITE_CYCLE_COLORS_INIT;
            break;
            
        case TEST_SUITE_CYCLE_COLORS_INIT:
            test_suite_generic_uint = 0;
            test_suite_state = TEST_SUITE_CYCLE_COLORS_DRAW;
            break;
        case TEST_SUITE_CYCLE_COLORS_DRAW:
            if (test_suite_generic_uint < TEST_SUITE_NUM_OF_CYCLE_COLORS) {
                struct layer_color color = test_suite_cycle_colors[test_suite_generic_uint++];
                layer_draw_all_pixels(color);
                layer_swap_buffers();
                timer_start(test_suite_countdown_timer, TEST_SUITE_CYCLE_COLORS_DELAY, TIMER_TIME_UNIT_MS);
            } else
                test_suite_state = TEST_SUITE_FINISHED;
            break;
           
        case TEST_SUITE_FINISHED: {
            struct layer_color color = 
            { 
                .r = rand() % 256,
                .g = rand() % 256,
                .b = rand() % 256
            };
            layer_draw_all_pixels(color);
            layer_swap_buffers();
            timer_start(test_suite_countdown_timer, TEST_SUITE_FINISHED_DELAY, TIMER_TIME_UNIT_MS);
            break;
        }
    }
}
#endif