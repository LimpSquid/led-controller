#include <sys.h>
#include <kernel.h>
#include <kernel_task.h>
#include <dma.h>
#include <pwm.h>
#include <bus_address.h>
#include <xc.h>

int main_app(void)
{
    // Bonzo is sleeping for the early init
    SYS_TUCK_IN_BONZO();
    sys_disable_global_interrupt();
    sys_cpu_early_init();
    SYS_WAKEUP_BONZO();

    sys_cpu_config_check();

    // Initialize hardware
    dma_init();
    pwm_init();

    // Then do the kernel init
    kernel_init();

    // Misc init
    bus_address_init();

    // And finally enable interrupts again
    sys_enable_global_interrupt();

    // Run our kernel and keep bonzo happy
    for (;;) {
        SYS_FEED_BONZO();
        kernel_execute();
    }

    // Avoid warnings
    return 0;
}

// Define main as weak so it can be overridden by the bootloader when that is linked
// into the final executable as well.
int __attribute__((weak)) main(void)
{
    return main_app();
}