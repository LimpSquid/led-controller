#include <sys.h>
#include <kernel.h>
#include <kernel_task.h>
#include <dma.h>
#include <pwm.h>
#include <bus_address.h>

int main()
{
    // Bonzo is sleeping for the early init
    sys_goodnight_bonzo();
    sys_disable_global_interrupt();
    sys_cpu_early_init();
    sys_wakeup_bonzo();

    // Initialize hardware
    dma_init();
    pwm_init();
    
    // Then do the kernel init
    kernel_init();
	
	// Misc init
	bus_address_init();
    
    // And finally enable interrupts again
    sys_enable_global_interrupt();

    // @todo: eventually remove this when UART works
    ANSELBbits.ANSB7 = 0;
    TRISBbits.TRISB7 = 0;
    LATBbits.LATB7 = 0;
    
    // Bonzo is always hungry...
    while(SYS_BONZO_IS_HUNGRY) {
        sys_feed_bonzo();
        kernel_execute();
    }
}
