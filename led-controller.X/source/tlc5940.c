#include <tlc5940.h>
#include <tlc5940_config.h>
#include <spi.h>
#include <dma.h>
#include <pwm.h>
#include <sys.h>
#include <util.h>
#include <kernel_task.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>

#if !defined(TLC5940_NUM_OF_DEVICES)
    #error "Number of TLC5940 devices is not specified, please define 'TLC5940_NUM_OF_DEVICES'"
#elif !defined(TLC5940_GSCLK_PERIOD)
    #error "GSCLK period is not specified, please define 'TLC5940_GSCLK_PERIOD'"
#elif !defined(TLC5940_DOT_CORRECTION)
    #error "Dot correction is not specified, please define 'TLC5940_DOT_CORR_ADJUST'"
#endif

STATIC_ASSERT(TLC5940_NUM_OF_DEVICES > 0)
STATIC_ASSERT(TLC5940_GSCLK_PERIOD >= 450 && TLC5940_GSCLK_PERIOD <= 50000) // Limit values to something reasonable

// The hardware allows for a current of Imax = (Iref / Riref) * 31.5 = (1.24 / 2700) * 31.5 = 15mA per channel.
// Turning all channels on for a single LED (r, g, b) we have a sum of 45mA. I don't have the specs for our LEDs, but I think
// this might be a bit too much for the poor anodes. What the hell was I thinking, you can't just assume a typical of 20mA and 
// multiply it by 3 for a RGB LED. Hopefully I didn't damage LEDs too much by now... Anyway, instead of revisiting the 
// hardware, we can use the dot correction to limit the current. The formula goes: Iout = (DCn / 63) * Imax.
// So assuming 20mA for a single RGB LED we roughly need a output current of 6.6mA per channel, giving us a maximum
// dot correction value of 28. Lets stop compiling when we exceed that.
STATIC_ASSERT(TLC5940_DOT_CORRECTION <= 28)

#define TLC5940_CHANNELS_PER_DEVICE     16
#define TLC5940_BUFFER_SIZE             (24 * TLC5940_NUM_OF_DEVICES)
#define TLC5940_BUFFER_SIZE_DOT_CORR    (12 * TLC5940_NUM_OF_DEVICES)
#define TLC5940_NUM_OF_CHANNELS         (TLC5940_CHANNELS_PER_DEVICE * TLC5940_NUM_OF_DEVICES)
#define TLC5940_NUM_OF_DC_QUARTETS      (TLC5940_BUFFER_SIZE_DOT_CORR / sizeof(tlc5940_quartet_t))
#define TLC5940_MAX_PWM_VALUE           (BIT_SHIFT(12) - 1) // 12 bit
#define TLC5940_PWM_FREQ                ((1000000LU * TLC5940_MAX_PWM_VALUE) / TLC5940_GSCLK_PERIOD)
#define TLC5940_QUARTET(a, b, c, d)     {(unsigned char)(a << 2 | b >> 4), (unsigned char)(b << 4 | c >> 2), (unsigned char)(c << 6 | d)}
#define TLC5940_QUARTET_FILL(val)       TLC5940_QUARTET(val, val, val, val)

#define TLC5940_SPI_CHANNEL             SPI_CHANNEL2

#define TLC5940_SDO_PPS_REG             RPG7R

#define TLC5940_SDO_PPS_WORD            0x6

typedef unsigned char tlc5940_quartet_t[3];

struct tlc5940_flags
{
    // Don't use a bit-field here as the flag is used from an ISR,
    // thus we require atomic access. A bit-field will use a LBU (load
    // byte) instruction which is not read-modify-write safe.

    volatile bool need_update;

    // Changing the operating mode needs happen in the state machine
    // so we will not interfere with a pending update
    bool switch_mode_enable     :1;
    bool switch_mode_disable    :1;
    bool switch_mode_lod        :1;
};

enum tlc5940_state
{
    TLC5940_INIT = 0,
    TLC5940_INIT_WRITE_DOT_CORRECTION,

    TLC5940_IDLE,
    TLC5940_UPDATE,
    TLC5940_UPDATE_DMA_TRANSFER,
    TLC5940_UPDATE_DMA_TRANSFER_WAIT,
    TLC5940_SWITCH_MODE_ENABLE,
    TLC5940_SWITCH_MODE_DISABLE,
    TLC5940_SWITCH_MODE_LOD,
};

static int tlc5940_rtask_init(void);
static void tlc5940_rtask_execute(void);
KERN_SIMPLE_RTASK(tlc5940, tlc5940_rtask_init, tlc5940_rtask_execute);

static struct dma_config const tlc5940_dma_config; // No special config needed
static struct spi_config const tlc5940_spi_config =
{
    .spi_con_flags = SPI_MSTEN | SPI_STXISEL_COMPLETE | SPI_DISSDI | SPI_MODE8 | SPI_CKP,
    .baudrate = 40000000,
};

static struct pwm_config const tlc5940_pwm_config =
{
    .duty = 0.5,
    .frequency = TLC5940_PWM_FREQ,
    .period_callback_div = TLC5940_MAX_PWM_VALUE // Number of PWM pulses for one GSCLK period to complete
};

static struct io_pin const tlc5940_sdo_pin = IO_ANLG_PIN(7, G);
static struct io_pin const tlc5940_sck_pin = IO_ANLG_PIN(6, G);
static struct io_pin const tlc5940_blank_pin = IO_ANLG_PIN(7, E);
static struct io_pin const tlc5940_xlat_pin = IO_ANLG_PIN(6, E);
static struct io_pin const tlc5940_dcprg_pin = IO_ANLG_PIN(5, B);
static struct io_pin const tlc5940_gsclk_pin = IO_ANLG_PIN(5, E); // This pin can only be controlled when PWM module is disabled, they are mutually exclusive
static struct io_pin const tlc5940_vprg_pin = IO_ANLG_PIN(9, G);
static struct io_pin const tlc5940_xerr_pin = IO_ANLG_PIN(4, E);

static struct tlc5940_flags tlc5940_flags;

static unsigned char tlc5940_buffer[TLC5940_BUFFER_SIZE];
static unsigned char tlc5940_dot_corr_buffer[TLC5940_BUFFER_SIZE_DOT_CORR];

static struct dma_channel * tlc5940_dma_channel;
static struct spi_module * tlc5940_spi_module;
static enum tlc5940_state tlc5940_state = TLC5940_INIT;
static enum tlc5940_mode tlc5940_mode = TLC5940_MODE_DISABLED;

void __attribute__ ((weak)) tlc5940_update_handler(void)
{
    // Handler to write new data to the TLC5940's
}

void __attribute__ ((weak)) tlc5940_latch_handler(void)
{
    // Handler which is called after data is latched into the TLC5940's
    // and just before the blank pin is cleared
}

void pwm_period_callback(void)
{
    // Blank and shift in data
    IO_SET(tlc5940_blank_pin);
    IO_SETCLR(tlc5940_xlat_pin);

    tlc5940_latch_handler();
    
    IO_CLR(tlc5940_blank_pin);
    
    // Means that the update routine in the robin task did not
    // complete before the GSCLK period finished, consider lowering
    // the value of TLC5940_GSCLK_PERIOD
    ASSERT(!tlc5940_flags.need_update);
    
    // Also abort in release mode, that way we atleast notice that something's wrong
    SYS_FAIL_IF(tlc5940_flags.need_update);

    tlc5940_flags.need_update = true;
}

static int tlc5940_rtask_init(void)
{
    // Init dot correction buffer
    tlc5940_quartet_t const quartet = TLC5940_QUARTET_FILL(TLC5940_DOT_CORRECTION);
    for (unsigned int i = 0; i < TLC5940_NUM_OF_DC_QUARTETS; ++i) 
        memcpy(tlc5940_dot_corr_buffer + i * sizeof(quartet), quartet, sizeof(quartet));

    // Configure PPS
    sys_unlock();
    TLC5940_SDO_PPS_REG = TLC5940_SDO_PPS_WORD;
    sys_lock();

    // Configure IO
    io_configure(IO_DIRECTION_DOUT_LOW, &tlc5940_sdo_pin, 1);
    io_configure(IO_DIRECTION_DOUT_LOW, &tlc5940_sck_pin, 1);
    io_configure(IO_DIRECTION_DOUT_LOW, &tlc5940_blank_pin, 1);
    io_configure(IO_DIRECTION_DOUT_LOW, &tlc5940_xlat_pin, 1);
    io_configure(IO_DIRECTION_DOUT_HIGH, &tlc5940_dcprg_pin, 1); // Use dot correction register
    io_configure(IO_DIRECTION_DOUT_LOW, &tlc5940_vprg_pin, 1);
    io_configure(IO_DIRECTION_DIN, &tlc5940_xerr_pin, 1);

    // Already configured in PWM module
    // io_configure(IO_DIRECTION_DOUT_LOW, &tlc5940_gsclk_pin, 1);

    // Initialize DMA
    tlc5940_dma_channel = dma_construct(tlc5940_dma_config);
    if (tlc5940_dma_channel == NULL)
        goto fail_dma;

    // Initialize SPI
    tlc5940_spi_module = spi_construct(TLC5940_SPI_CHANNEL, tlc5940_spi_config);
    if (tlc5940_spi_module == NULL)
        goto fail_spi;
    spi_configure_dma_dst(tlc5940_spi_module, tlc5940_dma_channel); // SPI module is the destination of the dma module
    spi_enable(tlc5940_spi_module);

    // Initialize PWM
    pwm_configure(tlc5940_pwm_config);

    return KERN_INIT_SUCCESS;

fail_spi:
    dma_destruct(tlc5940_dma_channel);
fail_dma:

    return KERN_INIT_FAILED;
}

static void tlc5940_rtask_execute(void)
{
    switch (tlc5940_state) {
        default:
        case TLC5940_INIT:
        case TLC5940_INIT_WRITE_DOT_CORRECTION:
            IO_SET(tlc5940_vprg_pin);
            spi_transmit_mode8(tlc5940_spi_module, tlc5940_dot_corr_buffer, TLC5940_BUFFER_SIZE_DOT_CORR);
            IO_SETCLR(tlc5940_xlat_pin);
            IO_CLR(tlc5940_vprg_pin);
            tlc5940_state = TLC5940_IDLE;
            break;
        
        case TLC5940_IDLE:
            if (tlc5940_flags.need_update)
                tlc5940_state = TLC5940_UPDATE;
            else if (tlc5940_flags.switch_mode_enable)
                tlc5940_state = TLC5940_SWITCH_MODE_ENABLE;
            else if (tlc5940_flags.switch_mode_disable)
                tlc5940_state = TLC5940_SWITCH_MODE_DISABLE;
            else if (tlc5940_flags.switch_mode_lod)
                tlc5940_state = TLC5940_SWITCH_MODE_LOD;
            break;
        case TLC5940_UPDATE:
            tlc5940_update_handler();
            tlc5940_state = TLC5940_UPDATE_DMA_TRANSFER;
            // no break
        case TLC5940_UPDATE_DMA_TRANSFER:
            if (dma_ready(tlc5940_dma_channel)) {
                dma_configure_src(tlc5940_dma_channel, tlc5940_buffer, TLC5940_BUFFER_SIZE);
                dma_enable_transfer(tlc5940_dma_channel);
                tlc5940_state = TLC5940_UPDATE_DMA_TRANSFER_WAIT;
            }
            break;
        case TLC5940_UPDATE_DMA_TRANSFER_WAIT:
            if (dma_ready(tlc5940_dma_channel)) {
                tlc5940_flags.need_update = false;
                memset(tlc5940_buffer, 0x00, TLC5940_BUFFER_SIZE); // Because we are OR'ing in tlc5940_write
                tlc5940_state = TLC5940_IDLE;
            }
            break;
            
        case TLC5940_SWITCH_MODE_ENABLE:
            IO_SETCLR(tlc5940_blank_pin);
            pwm_enable();
            
            tlc5940_flags.switch_mode_enable = false;
            tlc5940_mode = TLC5940_MODE_ENABLED;
            tlc5940_state = TLC5940_IDLE;
            break;
        case TLC5940_SWITCH_MODE_DISABLE:
            pwm_disable();
            IO_SETCLR(tlc5940_blank_pin);
            
            tlc5940_flags.switch_mode_disable = false;
            tlc5940_mode = TLC5940_MODE_DISABLED;
            tlc5940_state = TLC5940_IDLE;
            break;
        case TLC5940_SWITCH_MODE_LOD:
            // Disable PWM so normal operation is halted and we can control the GSCLK pin
            pwm_disable();
            
            // Set every channel to max PWM value
            memset(tlc5940_buffer, 0xff, TLC5940_BUFFER_SIZE);
            spi_transmit_mode8(tlc5940_spi_module, tlc5940_buffer, TLC5940_BUFFER_SIZE);
            memset(tlc5940_buffer, 0x00, TLC5940_BUFFER_SIZE);
            
            // Blank and latch in data
            IO_SET(tlc5940_blank_pin);
            IO_SETCLR(tlc5940_xlat_pin);
            
            // Enable output so we can read LOD errors
            IO_CLR(tlc5940_blank_pin);
            IO_SETCLR(tlc5940_gsclk_pin);
            
            tlc5940_flags.switch_mode_lod = false;
            tlc5940_mode = TLC5940_MODE_LOD;
            tlc5940_state = TLC5940_IDLE;
            break;
    }
}

enum tlc5940_mode tlc5940_get_mode(void)
{
    return tlc5940_mode;
};

void tlc5940_switch_mode(enum tlc5940_mode mode)
{
    switch (mode) {
        case TLC5940_MODE_ENABLED:  tlc5940_flags.switch_mode_enable = true;    break;
        case TLC5940_MODE_DISABLED: tlc5940_flags.switch_mode_disable = true;   break;
        case TLC5940_MODE_LOD:      tlc5940_flags.switch_mode_lod = true;       break;
        default:;
    }
}

bool tlc5940_get_lod_error(void)
{
    return !IO_READ(tlc5940_xerr_pin);
}

void tlc5940_write(unsigned int device, unsigned int channel, unsigned short pwm_value)
{
    if (device >= TLC5940_NUM_OF_DEVICES)
        return;
    if (channel >= TLC5940_CHANNELS_PER_DEVICE)
        return;
    if (pwm_value > TLC5940_MAX_PWM_VALUE)
        pwm_value = TLC5940_MAX_PWM_VALUE;

    unsigned char byte1;
    unsigned char byte2;
    unsigned int index = channel + device * TLC5940_CHANNELS_PER_DEVICE;

    index += index >> 1;
    if (channel & 1) {
        byte1 = (pwm_value >> 8) & 0x0f;
        byte2 = (pwm_value & 0xff);
    } else {
        byte1 = (pwm_value >> 4) & 0xff;
        byte2 = (pwm_value & 0x0f) << 4;
    }

    tlc5940_buffer[index    ] |= byte1;
    tlc5940_buffer[index + 1] |= byte2;
}

void tlc5940_write_all_channels(unsigned int device, unsigned short pwm_value)
{
    if (device >= TLC5940_NUM_OF_DEVICES)
        return;
    
    for (unsigned int i = 0; i < TLC5940_CHANNELS_PER_DEVICE; i++)
        tlc5940_write(device, i, pwm_value);
}
