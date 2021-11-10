#include <tlc5940.h>
#include <tlc5940_config.h>
#include <spi.h>
#include <dma.h>
#include <pwm.h>
#include <sys.h>
#include <toolbox.h>
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

#define TLC5940_SDO_TRIS                TRISG
#define TLC5940_SCK_TRIS                TRISG
#define TLC5940_BLANK_TRIS              TRISE
#define TLC5940_XLAT_TRIS               TRISE
#define TLC5940_VPRG_TRIS               TRISG
#define TLC5940_DCPRG_TRIS              TRISB
#define TLC5940_XERR_TRIS               TRISE

#define TLC5940_SDO_LAT                 LATG
#define TLC5940_SCK_LAT                 LATG
#define TLC5940_BLANK_LAT               LATE
#define TLC5940_XLAT_LAT                LATE
#define TLC5940_VPRG_LAT                LATG
#define TLC5940_DCPRG_LAT               LATB

#define TLC5940_XERR_PORT               PORTE

#define TLC5940_SDO_ANSEL               ANSELG
#define TLC5940_SCK_ANSEL               ANSELG
#define TLC5940_BLANK_ANSEL             ANSELE
#define TLC5940_XLAT_ANSEL              ANSELE
#define TLC5940_VPRG_ANSEL              ANSELG
#define TLC5940_DCPRG_ANSEL             ANSELB
#define TLC5940_XERR_ANSEL              ANSELE

#define TLC5940_SDO_PPS_WORD            0x6
#define TLC5940_SDO_PIN_MASK            BIT(7)
#define TLC5940_SCK_PIN_MASK            BIT(6)
#define TLC5940_BLANK_PIN_MASK          BIT(7)
#define TLC5940_XLAT_PIN_MASK           BIT(6)
#define TLC5940_VPRG_PIN_MASK           BIT(9)
#define TLC5940_DCPRG_PIN_MASK          BIT(5)
#define TLC5940_XERR_PIN_MASK           BIT(4)

struct tlc5940_flags
{
    bool need_update    :1;
};

enum tlc5940_state
{
    TLC5940_INIT = 0,
    TLC5940_WRITE_DOT_CORRECTION,

    TLC5940_IDLE,    

    TLC5940_UPDATE,
    TLC5940_UPDATE_DMA_TRANSFER,
    TLC5940_UPDATE_DMA_TRANSFER_WAIT,
};

static int tlc5940_rtask_init(void);
static void tlc5940_rtask_execute(void);
KERN_SIMPLE_RTASK(tlc5940, tlc5940_rtask_init, tlc5940_rtask_execute);

static unsigned char tlc5940_buffer[TLC5940_BUFFER_SIZE];
static unsigned char tlc5940_dot_corr_buffer[TLC5940_BUFFER_SIZE_DOT_CORR];

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

static struct tlc5940_flags tlc5940_flags = 
{
    .need_update = false
};

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
    REG_SET(TLC5940_BLANK_LAT, TLC5940_BLANK_PIN_MASK);

    // Latch in data
    REG_SET(TLC5940_XLAT_LAT, TLC5940_XLAT_PIN_MASK);
    REG_CLR(TLC5940_XLAT_LAT, TLC5940_XLAT_PIN_MASK);

    // Shift diagnostic data
    spi_disable(tlc5940_spi_module);
    REG_INV(TLC5940_SCK_LAT, TLC5940_SCK_PIN_MASK);
    REG_INV(TLC5940_SCK_LAT, TLC5940_SCK_PIN_MASK);
    spi_enable(tlc5940_spi_module);

    tlc5940_latch_handler();

    REG_CLR(TLC5940_BLANK_LAT, TLC5940_BLANK_PIN_MASK);
    
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
    REG_CLR(TLC5940_SDO_ANSEL, TLC5940_SDO_PIN_MASK);
    REG_CLR(TLC5940_SCK_ANSEL, TLC5940_SCK_PIN_MASK);
    REG_CLR(TLC5940_BLANK_ANSEL, TLC5940_BLANK_PIN_MASK);
    REG_CLR(TLC5940_XLAT_ANSEL, TLC5940_XLAT_PIN_MASK);
    REG_CLR(TLC5940_VPRG_ANSEL, TLC5940_VPRG_PIN_MASK);
    REG_CLR(TLC5940_DCPRG_ANSEL, TLC5940_DCPRG_PIN_MASK);
    REG_CLR(TLC5940_XERR_ANSEL, TLC5940_XERR_PIN_MASK);

    REG_CLR(TLC5940_SDO_TRIS, TLC5940_SDO_PIN_MASK);
    REG_CLR(TLC5940_SCK_TRIS, TLC5940_SCK_PIN_MASK);
    REG_CLR(TLC5940_BLANK_TRIS, TLC5940_BLANK_PIN_MASK);
    REG_CLR(TLC5940_XLAT_TRIS, TLC5940_XLAT_PIN_MASK);
    REG_CLR(TLC5940_VPRG_TRIS, TLC5940_VPRG_PIN_MASK);
    REG_CLR(TLC5940_DCPRG_TRIS, TLC5940_DCPRG_PIN_MASK);
    REG_SET(TLC5940_XERR_TRIS, TLC5940_XERR_PIN_MASK);

    // Define output states
    REG_CLR(TLC5940_BLANK_LAT, TLC5940_BLANK_PIN_MASK);
    REG_CLR(TLC5940_XLAT_LAT, TLC5940_XLAT_PIN_MASK);
    REG_CLR(TLC5940_VPRG_LAT, TLC5940_VPRG_PIN_MASK);
    REG_SET(TLC5940_DCPRG_LAT, TLC5940_DCPRG_PIN_MASK);

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
        case TLC5940_INIT:
        case TLC5940_WRITE_DOT_CORRECTION:
            REG_SET(TLC5940_VPRG_LAT, TLC5940_VPRG_PIN_MASK);
            spi_transmit_mode8(tlc5940_spi_module, tlc5940_dot_corr_buffer, TLC5940_BUFFER_SIZE_DOT_CORR);
            REG_SET(TLC5940_XLAT_LAT, TLC5940_XLAT_PIN_MASK);
            REG_CLR(TLC5940_XLAT_LAT, TLC5940_XLAT_PIN_MASK);
            REG_CLR(TLC5940_VPRG_LAT, TLC5940_VPRG_PIN_MASK);
            tlc5940_state = TLC5940_IDLE;
            break;
        
        case TLC5940_IDLE:
            if(tlc5940_flags.need_update)
                tlc5940_state = TLC5940_UPDATE;
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
    }
}

bool tlc5940_enable(void)
{
    // Note that the first GSCLK is undefined because the update
    // and latch handlers are yet to be called
    pwm_enable(); 
}

bool tlc5940_disable(void)
{
    pwm_disable();
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