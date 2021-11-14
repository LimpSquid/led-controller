#ifndef SYS_H
#define SYS_H

#include <stdbool.h>
#include <xc.h>

#if defined(_SYS_CLK) && defined(_PB_DIV)
    #define SYS_PB_CLOCK            ((unsigned long long)(_SYS_CLK / _PB_DIV))
#else
    #error "System peripheral bus clock could not be calculated, please define the _SYS_CLK and _PB_DIV." 
#endif

// Version ranges between 0 - 255
#define SYS_VERSION_MAJOR           0
#define SYS_VERSION_MINOR           0
#define SYS_VERSION_PATCH           0
#define SYS_BONZO_IS_HUNGRY         true

#define sys_goodnight_bonzo()       WDTCONbits.ON = 0
#define sys_wakeup_bonzo()          WDTCONbits.ON = 1
#define sys_feed_bonzo()            WDTCONbits.WDTCLR = 1

void sys_lock(void);
void sys_unlock(void);
void sys_enable_global_interrupt(void);
void sys_disable_global_interrupt(void);
void sys_cpu_early_init(void);
void sys_cpu_reset(void);

#endif /* SYS_H */