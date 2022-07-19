#include <app/dma.h>
#include <app/pwm.h>
#include <core/sys.h>
#include <core/kernel.h>
#include <core/kernel_task.h>
#include <core/bus_address.h>
#include <xc.h>

void app_cpu_init()
{
    _CP0_SET_EBASE(EXCEPTION_MEM_BASE);
}

int app_main(void)
{
    // Bonzo is sleeping for the early init
    SYS_TUCK_IN_BONZO();
    sys_disable_global_interrupt();
    sys_cpu_early_init();
    app_cpu_init();
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

int main(void)
{
    return app_main();
}
