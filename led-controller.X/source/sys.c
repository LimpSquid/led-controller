#include <sys.h>
#include <util.h>
#include <assert.h>
#include <xc.h>

#define SYS_OSCCON_REG                  OSCCON
#define SYS_INTCON_REG                  INTCON
#define SYS_CFGCON_REG                  CFGCON
#define SYS_DEVCFG3_REG                 DEVCFG3

#define SYS_OSCCON_CLK_LCK_MASK         BIT(7)
#define SYS_OSCCON_CF_MASK              BIT(3)
#define SYS_INTCON_MVEC_MASK            BIT(12)
#define SYS_CFGCON_IOLOCK_MASK          BIT(12)
#define SYS_DEVCFG3_FSRSSEL_MASK        MASK(0x7, 0)

#define EXCEPTION_MEM_BASE              0x9d00d000 // see memory section `kseg0_program_exception_mem` in linker file)

#define ASSERT_EQ(lhs, rhs)             do { ASSERT(lhs == rhs); SYS_FAIL_IF_NOT(lhs == rhs); } while(0)

static void sys_control_coprocessor_init(void)
{
    // Configure exception memory base address
    _CP0_SET_EBASE(EXCEPTION_MEM_BASE);
}

void sys_lock(void)
{
    REG_SET(SYS_CFGCON_REG, SYS_CFGCON_IOLOCK_MASK);
    SYSKEY = 0x33333333;
}

void sys_unlock(void)
{
    SYSKEY = 0x33333333;
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;
    REG_CLR(SYS_CFGCON_REG, SYS_CFGCON_IOLOCK_MASK);
}

void sys_enable_global_interrupt(void)
{
    __asm("ei");
}

void sys_disable_global_interrupt(void)
{
    __asm("di");
}

void sys_cpu_early_init(void)
{
    // Wait for valid clock
    while (SYS_OSCCON_REG & SYS_OSCCON_CF_MASK);

    // Configure system control coprocessor
    sys_control_coprocessor_init();

    // Configure clock
    sys_unlock();
    REG_SET(SYS_OSCCON_REG, SYS_OSCCON_CLK_LCK_MASK); // Lock clock and PLL selections
    sys_lock();

    // Configure other stuff
    REG_SET(SYS_INTCON_REG, SYS_INTCON_MVEC_MASK);
}

void sys_cpu_reset(void)
{
    exit(EXIT_SUCCESS);
}

void sys_cpu_config_check(void)
{
    // Interrupts with priory 7 can make use of a shadow register set and are allowed
    // to use IPL7SRS instead of IPL7SOFT. All interrupts with a different priority
    // must use IPLnSOFT.
    ASSERT_EQ((SYS_DEVCFG3_REG & SYS_DEVCFG3_FSRSSEL_MASK), 7);
}