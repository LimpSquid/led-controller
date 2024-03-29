#include <app/pwm.h>
#include <core/assert.h>
#include <core/sys.h>
#include <core/util.h>
#include <sys/attribs.h>
#include <xc.h>

// Below are the output compare related defines
#define PWM_OC_PR(frequency)                (SYS_PB_CLOCK / (frequency * PWM_OC_PRESCALER) - 1)
#define PWM_OC_DUTY(frequency, percentage)  ((PWM_OC_PR(frequency) + 1) * percentage)
#define PWM_OC_PRESCALER                    1

#define PWM_OC_GSCLK_PPS_REG                RPE5R
#define PWM_OC_TCON_REG                     T2CON
#define PWM_OC_PR_REG                       PR2
#define PWM_OC_TMR_REG                      TMR2
#define PWM_OC_OCCON_REG                    OC4CON
#define PWM_OC_OCRS_REG                     OC4RS

#define PWM_OC_GSCLK_PPS_WORD               MASK(0xb, 0)
#define PWM_OC_TCON_WORD                    BIT(15)
#define PWM_OC_OCCON_WORD                   MASK(0x6, 0)

#define PWM_OC_OCCON_ON_MASK                BIT(15)

// Below are the timer related defines
#define PWM_TMR_PR(frequency, div)          ((SYS_PB_CLOCK * div) / (frequency * PWM_TMR_PRESCALER) - 1)
#define PWM_TMR_PRESCALER                   256

#define PWM_TMR_VECTOR                      _TIMER_3_VECTOR

#define PWM_TMR_TCON_REG                    T3CON
#define PWM_TMR_PR_REG                      PR3
#define PWM_TMR_TMR_REG                     TMR3
#define PWM_TMR_IEC_REG                     IEC0
#define PWM_TMR_IFS_REG                     IFS0
#define PWM_TMR_IPC_REG                     IPC3

#define PWM_TMR_TCON_WORD                   MASK(0x7, 4)

#define PWM_TMR_OCCON_ON_MASK               BIT(15)
#define PWM_TMR_INT_MASK                    BIT(14)
#define PWM_TMR_INT_PRIORITY_MASK           MASK(0x7, 2) // Interrupt handler must use IPL7SRS

static struct io_pin const pwm_gsclk_pin = IO_ANLG_PIN(5, E);

void pwm_init(void)
{
    // Configure PPS
    sys_unlock();
    PWM_OC_GSCLK_PPS_REG = PWM_OC_GSCLK_PPS_WORD;
    sys_lock();

    // Configure IO
    io_configure(IO_DIRECTION_DOUT_LOW, &pwm_gsclk_pin, 1);

    // Configure timer and interrupt
    PWM_TMR_TCON_REG = PWM_TMR_TCON_WORD;
    REG_SET(PWM_TMR_IEC_REG, PWM_TMR_INT_MASK);
    REG_SET(PWM_TMR_IPC_REG, PWM_TMR_INT_PRIORITY_MASK);

    // Configure PWM module
    PWM_OC_TCON_REG = PWM_OC_TCON_WORD;
    PWM_OC_OCCON_REG = PWM_OC_OCCON_WORD;
}

void pwm_configure(struct pwm_config config)
{
    ASSERT(config.duty >= 0.0 && config.duty <= 1.0);
    ASSERT(config.period_callback_div > 0);

    // Disable timer and PWM first
    REG_CLR(PWM_TMR_TCON_REG, PWM_TMR_OCCON_ON_MASK);
    REG_CLR(PWM_OC_OCCON_REG, PWM_OC_OCCON_ON_MASK);

    // Configure PWM
    PWM_OC_PR_REG = PWM_OC_PR(config.frequency);
    PWM_OC_OCRS_REG = PWM_OC_DUTY(config.frequency, config.duty);

    // Configure timer
    PWM_TMR_PR_REG = PWM_TMR_PR(config.frequency, config.period_callback_div);
}

void pwm_enable(void)
{
    // Reset timer
    REG_CLR(PWM_TMR_IFS_REG, PWM_TMR_INT_MASK);
    PWM_TMR_TMR_REG = 0;

    // Reset PWM
    PWM_OC_TMR_REG = 0;

    // Enable timer and PWM
    REG_SET(PWM_TMR_TCON_REG, PWM_TMR_OCCON_ON_MASK);
    REG_SET(PWM_OC_OCCON_REG, PWM_OC_OCCON_ON_MASK);
}

void pwm_disable(void)
{
    // Disable timer and PWM
    REG_CLR(PWM_TMR_TCON_REG, PWM_TMR_OCCON_ON_MASK);
    REG_CLR(PWM_OC_OCCON_REG, PWM_OC_OCCON_ON_MASK);
}

void __attribute__ ((weak)) pwm_period_callback(void)
{
    // can be overridden if desired
}

void __ISR(PWM_TMR_VECTOR, IPL7SRS) pwm_timer_interrupt(void)
{
    pwm_period_callback();
    REG_CLR(PWM_TMR_IFS_REG, PWM_TMR_INT_MASK);
}
