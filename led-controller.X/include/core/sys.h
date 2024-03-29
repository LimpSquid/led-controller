#ifndef SYS_H
#define SYS_H

#include <stdbool.h>
#include <xc.h>

#if defined(_SYS_CLK) && defined(_PB_DIV)
    #define SYS_PB_CLOCK            ((unsigned long long)(_SYS_CLK / _PB_DIV))
#else
    #error "System peripheral bus clock could not be calculated, please define the _SYS_CLK and _PB_DIV." 
#endif

#define SYS_FAIL_IF(expression)     if(expression) { exit(EXIT_FAILURE); }
#define SYS_FAIL_IF_NOT(expression) SYS_FAIL_IF(!(expression))

// Macros for performance reasons
#define SYS_TUCK_IN_BONZO()         WDTCONbits.ON = 0
#define SYS_WAKEUP_BONZO()          WDTCONbits.ON = 1
#define SYS_FEED_BONZO()            WDTCONbits.WDTCLR = 1

void sys_lock(void);
void sys_unlock(void);
void sys_enable_global_interrupt(void);
void sys_disable_global_interrupt(void);
void sys_cpu_early_init(void);
void sys_cpu_reset(void);
void sys_cpu_config_check(void);

#endif /* SYS_H */