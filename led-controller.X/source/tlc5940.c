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
#elif !defined(TLC5490_GSCLK_PERIOD)
    #error "GSCLK period is not specified, please define 'TLC5490_GSCLK_PERIOD'"
#endif

STATIC_ASSERT(TLC5940_NUM_OF_DEVICES > 0)
STATIC_ASSERT(TLC5490_GSCLK_PERIOD >= 750 && TLC5490_GSCLK_PERIOD <= 31250) // Limit values to something reasonable

#define TLC5940_CHANNELS_PER_DEVICE     16
#define TLC5940_BUFFER_SIZE             (24 * TLC5940_NUM_OF_DEVICES)
#define TLC5940_BUFFER_SIZE_DOT_CORR    (12 * TLC5940_NUM_OF_DEVICES)
#define TLC5940_CHANNEL_SIZE            (TLC5940_CHANNELS_PER_DEVICE * TLC5940_NUM_OF_DEVICES)
#define TLC5940_GSCLK_PERIOD_PULSES     4096 // number of PWM pulses for one GSCLK period to complete
#define TLC5940_PWM_FREQ                ((1000000LU * TLC5940_GSCLK_PERIOD_PULSES) / TLC5490_GSCLK_PERIOD)

#define TLC5940_SPI_CHANNEL             SPI_CHANNEL2
#define TLC5940_SDO_PPS                 RPG7R

#define TLC5940_SDO_PPS_WORD            0x6

struct tlc5940_flags
{
    bool need_update    :1;
    
    // Enabling, disabling and LOD mode needs to
    // be executed from the state machine to not interfere
    // with an ongoing update
    bool exec_enable    :1;
    bool exec_disable   :1;
    bool exec_lod_mode  :1;
};

enum tlc5940_state
{
    TLC5940_INIT = 0,
    TLC5940_INIT_WRITE_DOT_CORRECTION,

    TLC5940_IDLE,
    TLC5940_UPDATE,
    TLC5940_UPDATE_DMA_TRANSFER,
    TLC5940_UPDATE_DMA_TRANSFER_WAIT,
    TLC5940_EXEC_ENABLE,
    TLC5940_EXEC_DISABLE,
    TLC5940_EXEC_LOD_MODE,
    TLC5940_EXEC_LOD_MODE_DONE,
};

static int tlc5940_rtask_init(void);
static void tlc5940_rtask_execute(void);
KERN_SIMPLE_RTASK(tlc5940, tlc5940_rtask_init, tlc5940_rtask_execute);

static const struct dma_config tlc5940_dma_config; // No special config needed
static const struct spi_config tlc5940_spi_config =
{
    .spi_con_flags = SPI_MSTEN | SPI_STXISEL_COMPLETE | SPI_DISSDI | SPI_MODE8 | SPI_CKP,
    .baudrate = 20000000,
};

static const struct pwm_config tlc5940_pwm_config =
{
    .duty = 0.5,
    .frequency = TLC5940_PWM_FREQ,
    .period_callback_div = TLC5940_GSCLK_PERIOD_PULSES
};

static const struct io_pin tlc5940_sdo_pin = IO_ANSEL_PIN(7, G);
static const struct io_pin tlc5940_sck_pin = IO_ANSEL_PIN(6, G);
static const struct io_pin tlc5940_blank_pin = IO_ANSEL_PIN(7, E);
static const struct io_pin tlc5940_xlat_pin = IO_ANSEL_PIN(6, E);
static const struct io_pin tlc5940_vprg_pin = IO_ANSEL_PIN(9, G);
static const struct io_pin tlc5940_dcprg_pin = IO_ANSEL_PIN(5, B);
static const struct io_pin tlc5940_xerr_pin = IO_ANSEL_PIN(4, E);

// Volatile because flags are accessed from ISR and we want to avoid weird optimizations
static volatile struct tlc5940_flags tlc5940_flags =
{
    .need_update = false,
    .exec_enable = false,
    .exec_disable = false,
    .exec_lod_mode = false
};

static unsigned char tlc5940_buffer[TLC5940_BUFFER_SIZE];
static unsigned char tlc5940_dot_corr_buffer[TLC5940_BUFFER_SIZE_DOT_CORR];

static struct dma_channel* tlc5940_dma_channel = NULL;
static struct spi_module* tlc5940_spi_module = NULL;
static enum tlc5940_state tlc5940_state = TLC5940_INIT;

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
    IO_HI_PULSE(tlc5940_xlat_pin);

    // Shift diagnostic data
    spi_disable(tlc5940_spi_module);
    IO_PULSE(tlc5940_sck_pin);
    spi_enable(tlc5940_spi_module);

    tlc5940_latch_handler();
    
    IO_CLR(tlc5940_blank_pin);
    
    // Means that the update routine in the robin task did not
    // complete before the GSCLK period finished, consider lowering
    // the value of TLC5490_GSCLK_PERIOD
    ASSERT(!tlc5940_flags.need_update); 

    tlc5940_flags.need_update = true;
}

static int tlc5940_rtask_init(void)
{
    // Init variables
    memset(tlc5940_dot_corr_buffer, 0xff, TLC5940_BUFFER_SIZE_DOT_CORR);

    // Configure PPS
    sys_unlock();
    TLC5940_SDO_PPS = TLC5940_SDO_PPS_WORD;
    sys_lock();

    // Configure IO
    io_configure(IO_DIRECTION_DOUT_LOW, &tlc5940_sdo_pin, 1);
    io_configure(IO_DIRECTION_DOUT_LOW, &tlc5940_sck_pin, 1);
    io_configure(IO_DIRECTION_DOUT_LOW, &tlc5940_blank_pin, 1);
    io_configure(IO_DIRECTION_DOUT_LOW, &tlc5940_xlat_pin, 1);
    io_configure(IO_DIRECTION_DOUT_LOW, &tlc5940_dcprg_pin, 1);
    io_configure(IO_DIRECTION_DOUT_HIGH, &tlc5940_vprg_pin, 1);
    io_configure(IO_DIRECTION_DIN, &tlc5940_xerr_pin, 1);

    // Initialize DMA
    tlc5940_dma_channel = dma_construct(tlc5940_dma_config);
    if(tlc5940_dma_channel == NULL)
        goto fail_dma;

    // Initialize SPI
    tlc5940_spi_module = spi_construct(TLC5940_SPI_CHANNEL, tlc5940_spi_config);
    if(tlc5940_spi_module == NULL)
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
    switch(tlc5940_state) {
        default:
        case TLC5940_INIT:
        case TLC5940_INIT_WRITE_DOT_CORRECTION:
            IO_SET(tlc5940_vprg_pin);
            spi_transmit_mode8(tlc5940_spi_module, tlc5940_dot_corr_buffer, TLC5940_BUFFER_SIZE_DOT_CORR);
            IO_HI_PULSE(tlc5940_xlat_pin);
            IO_CLR(tlc5940_vprg_pin);
            tlc5940_state = TLC5940_IDLE;
            break;
        
        case TLC5940_IDLE:
            if(tlc5940_flags.need_update)
                tlc5940_state = TLC5940_UPDATE;
            else if(tlc5940_flags.exec_enable)
                tlc5940_state = TLC5940_EXEC_ENABLE;
            else if(tlc5940_flags.exec_disable)
                tlc5940_state = TLC5940_EXEC_DISABLE;
            else if(tlc5940_flags.exec_lod_mode)
                tlc5940_state = TLC5940_EXEC_LOD_MODE;
            break;
        case TLC5940_UPDATE:
            tlc5940_update_handler();
            tlc5940_state = TLC5940_UPDATE_DMA_TRANSFER;
            break;
        case TLC5940_UPDATE_DMA_TRANSFER:
            if(dma_ready(tlc5940_dma_channel)) {
                dma_configure_src(tlc5940_dma_channel, tlc5940_buffer, TLC5940_BUFFER_SIZE);
                dma_enable_transfer(tlc5940_dma_channel);
                tlc5940_state = TLC5940_UPDATE_DMA_TRANSFER_WAIT;
            }
            break;
        case TLC5940_UPDATE_DMA_TRANSFER_WAIT:
            if(dma_ready(tlc5940_dma_channel)) {
                memset(tlc5940_buffer, 0x00, TLC5940_BUFFER_SIZE); // Because we are OR'ing in tlc5940_write
                tlc5940_flags.need_update = false;
                tlc5940_state = TLC5940_IDLE;
            }
            break;
            
        case TLC5940_EXEC_ENABLE:
        case TLC5940_EXEC_DISABLE:
            if(tlc5940_state == TLC5940_EXEC_ENABLE)
                pwm_enable();
            else
                pwm_disable();
            
            tlc5940_flags.exec_enable = false;
            tlc5940_flags.exec_disable = false;
            tlc5940_state = TLC5940_IDLE;
            break;
        case TLC5940_EXEC_LOD_MODE:
            pwm_disable();
            // @Todo: Write max values to all channels, latch data, clear blank pin, 
            // give PWM pin single high pulse, and then we can read out the error flags
            tlc5940_state = TLC5940_EXEC_LOD_MODE_DONE;
            break;
        case TLC5940_EXEC_LOD_MODE_DONE:
            tlc5940_flags.exec_lod_mode = false;
            tlc5940_state = TLC5940_IDLE;
            break;
    }
}

void tlc5940_enable(void)
{
    tlc5940_flags.exec_enable = true;
}

void tlc5940_disable(void)
{
    tlc5940_flags.exec_disable = true;
}

void tlc5940_lod_mode(void)
{
    tlc5940_flags.exec_lod_mode = true;
}

void tlc5940_write(unsigned int device, unsigned int channel, unsigned short value)
{
    if(device >= TLC5940_NUM_OF_DEVICES)
        return;
    if(channel >= TLC5940_CHANNELS_PER_DEVICE)
        return;

    unsigned char byte1;
    unsigned char byte2;
    unsigned int index = channel + device * TLC5940_CHANNELS_PER_DEVICE;

    index += index >> 1;
    if(channel & 1) {
        byte1 = (value >> 8) & 0x0f;
        byte2 = (value & 0xff);
    } else {
        byte1 = (value >> 4) & 0xff;
        byte2 = (value & 0x0f) << 4;
    }

    tlc5940_buffer[index    ] |= byte1;
    tlc5940_buffer[index + 1] |= byte2;
}

void tlc5940_write_all_channels(unsigned int device, unsigned short value)
{
    if(device >= TLC5940_NUM_OF_DEVICES)
        return;
    
    for(unsigned int i = 0; i < TLC5940_CHANNELS_PER_DEVICE; i++)
        tlc5940_write(device, i, value);
}