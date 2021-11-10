#include <layer.h>
#include <layer_config.h>
#include <kernel_task.h>
#include <tlc5940.h>
#include <spi.h>
#include <dma.h>
#include <timer.h>
#include <sys.h>
#include <register.h>
#include <toolbox.h>
#include <stddef.h>
#include <xc.h>
#include <string.h>

#define LAYER_NUM_OF_ROWS           16
#define LAYER_NUM_OF_COLS           16
#define LAYER_NUM_OF_LEDS           (LAYER_NUM_OF_ROWS * LAYER_NUM_OF_COLS)
#define LAYER_RED_OFFSET            (LAYER_NUM_OF_LEDS * 0)
#define LAYER_GREEN_OFFSET          (LAYER_NUM_OF_LEDS * 1)
#define LAYER_BLUE_OFFSET           (LAYER_NUM_OF_LEDS * 2)
#define LAYER_FRAME_DEPTH           3 // RGB
#define LAYER_FRAME_BUFFER_SIZE     (LAYER_NUM_OF_LEDS * LAYER_FRAME_DEPTH)
#define LAYER_OFFSET(index)         (index * LAYER_NUM_OF_COLS)
#define LAYER_IO(pin, bank) \
    { \
        .ansel = NULL, \
        .tris = atomic_reg_ptr_cast(&TRIS##bank), \
        .lat = atomic_reg_ptr_cast(&LAT##bank), \
        .port = atomic_reg_ptr_cast(&PORT##bank), \
        .mask = BIT(pin) \
    }
#define LAYER_ANSEL_IO(pin, bank) \
    { \
        .ansel = atomic_reg_ptr_cast(&ANSEL##bank), \
        .tris = atomic_reg_ptr_cast(&TRIS##bank), \
        .lat = atomic_reg_ptr_cast(&LAT##bank), \
        .port = atomic_reg_ptr_cast(&PORT##bank), \
        .mask = BIT(pin) \
    }

#define LAYER_SPI_CHANNEL               SPI_CHANNEL1
#define LAYER_SDI_PPS                   SDI1R
#define LAYER_SS_PPS                    SS1R

#define LAYER_SDI_TRIS                  TRISF
#define LAYER_SCK_TRIS                  TRISF
#define LAYER_SS_TRIS                   TRISB

#define LAYER_SS_ANSEL                  ANSELB 

#define LAYER_SDI_PPS_WORD              0xe
#define LAYER_SS_PPS_WORD               0x3
#define LAYER_SDI_PIN_MASK              BIT(2)
#define LAYER_SCK_PIN_MASK              BIT(6)
#define LAYER_SS_PIN_MASK               BIT(15)

struct layer_io
{
    atomic_reg_ptr(ansel);
    atomic_reg_ptr(tris);
    atomic_reg_ptr(lat);
    atomic_reg_ptr(port);
    
    unsigned int mask;
};

enum layer_state
{
    LAYER_IDLE = 0,
};

enum layer_dma_state
{
    LAYER_DMA_RECV_FRAME = 0,
    LAYER_DMA_RECV_FRAME_WAIT,
};

static int layer_rtask_init(void);
static void layer_rtask_execute(void);
static void layer_dma_rtask_execute(void);
KERN_SIMPLE_RTASK(layer, layer_rtask_init, layer_rtask_execute);
KERN_SIMPLE_RTASK(layer_dma, NULL, layer_dma_rtask_execute); // Init is done in layer_rtask_init, no need to make a separate init

#ifdef LAYER_INTERLACED
static const unsigned int layer_offset[LAYER_NUM_OF_ROWS] =
{
    LAYER_OFFSET(1), // Row 1
    LAYER_OFFSET(3), // Row 3
    LAYER_OFFSET(5), // ...
    LAYER_OFFSET(7),
    LAYER_OFFSET(9),
    LAYER_OFFSET(11),
    LAYER_OFFSET(13),
    LAYER_OFFSET(15),
    LAYER_OFFSET(0), // Row 0
    LAYER_OFFSET(2), // Row 2
    LAYER_OFFSET(4), // ...
    LAYER_OFFSET(6),
    LAYER_OFFSET(8),
    LAYER_OFFSET(10),
    LAYER_OFFSET(12),
    LAYER_OFFSET(14),
};

static const struct layer_io layer_io[LAYER_NUM_OF_ROWS] =
{
    LAYER_IO(6, D), // Row 1
    LAYER_IO(4, D), // Row 3
    LAYER_ANSEL_IO(2, D), // ...
    LAYER_IO(0, D),
    LAYER_ANSEL_IO(2, E), 
    LAYER_IO(0, E),
    LAYER_IO(10, D),
    LAYER_IO(8, D),
    LAYER_IO(7, D), // Row 0
    LAYER_IO(5, D), // Row 2
    LAYER_ANSEL_IO(3, D), // ...
    LAYER_ANSEL_IO(1, D),
    LAYER_IO(3, E),
    LAYER_IO(1, E),
    LAYER_IO(11, D),
    LAYER_IO(9, D),
};
#else
static const unsigned int layer_offset[LAYER_NUM_OF_ROWS] =
{
    LAYER_OFFSET(0), // Row 0
    LAYER_OFFSET(1), // Row 1
    LAYER_OFFSET(2), // ...
    LAYER_OFFSET(3),
    LAYER_OFFSET(4),
    LAYER_OFFSET(5),
    LAYER_OFFSET(6),
    LAYER_OFFSET(7),
    LAYER_OFFSET(8),
    LAYER_OFFSET(9),
    LAYER_OFFSET(10),
    LAYER_OFFSET(11),
    LAYER_OFFSET(12),
    LAYER_OFFSET(13),
    LAYER_OFFSET(14),
    LAYER_OFFSET(15),
};

static const struct layer_io layer_io[LAYER_NUM_OF_ROWS] =
{
    LAYER_IO(7, D), // Row 0
    LAYER_IO(6, D), // Row 1
    LAYER_IO(5, D), // ...
    LAYER_IO(4, D),
    LAYER_ANSEL_IO(3, D),
    LAYER_ANSEL_IO(2, D),
    LAYER_ANSEL_IO(1, D),
    LAYER_IO(0, D),
    LAYER_IO(3, E),
    LAYER_ANSEL_IO(2, E),
    LAYER_IO(1, E),
    LAYER_IO(0, E),
    LAYER_IO(11, D),
    LAYER_IO(10, D),
    LAYER_IO(9, D),
    LAYER_IO(8, D),
};
#endif

static const struct dma_config layer_dma_config; // No special config needed
static const struct spi_config layer_spi_config =
{
    .spi_con_flags = SPI_SRXISEL_NOT_EMPTY | SPI_DISSDO | SPI_MODE8 | SPI_SSEN,
};

static unsigned char layer_front_buffer[LAYER_FRAME_BUFFER_SIZE];
static unsigned char layer_back_buffer[LAYER_FRAME_BUFFER_SIZE];
static unsigned char* layer_dma_ptr = layer_back_buffer; // Buffer used for storing new data received on DMA channel
static unsigned char* layer_update_ptr = layer_front_buffer; // Buffer used for updating TLC5940's
static const struct layer_io* layer_row_io = layer_io;
static const struct layer_io* layer_row_previous_io = &layer_io[LAYER_NUM_OF_ROWS - 1];
static struct dma_channel* layer_dma_channel = NULL;
static struct spi_module* layer_spi_module = NULL;
static struct timer_module* layer_countdown_timer = NULL;
static enum layer_state layer_state = LAYER_IDLE;
static enum layer_dma_state layer_dma_state = LAYER_DMA_RECV_FRAME;
static unsigned int layer_row_index = 0; // Active row, corresponding row IO is layer_io[layer_row_index]

static void layer_row_io_reset(void)
{
    atomic_reg_ptr_clr(layer_row_io->lat, layer_row_io->mask);
    atomic_reg_ptr_clr(layer_row_previous_io->lat, layer_row_previous_io->mask);
    
    layer_row_io = layer_io;
    layer_row_previous_io = &layer_io[LAYER_NUM_OF_ROWS - 1];
    layer_row_index = 0;
}

inline static unsigned int __attribute__((always_inline)) layer_next_row_index(void)
{   
    unsigned int port = atomic_reg_ptr_value(layer_io[layer_row_index].port);
    if(port & layer_io[layer_row_index].mask)
        return (layer_row_index + 1) % LAYER_NUM_OF_ROWS;
    
    // This means that layer_row_io_reset() was just called (or a CPU reset)
    // and we have yet to make the first row active in tlc5940_latch_callback())
    return 0; 
}

inline static void  __attribute__((always_inline)) layer_swap_buffers(void)
{
    unsigned char* tmp = layer_dma_ptr;
    layer_dma_ptr = layer_update_ptr;
    layer_update_ptr = tmp;
}

void tlc5940_update_handler(void)
{
    unsigned int offset = layer_offset[layer_next_row_index()];
    unsigned int r, g, b;
    
    for(unsigned int i = 0; i < LAYER_NUM_OF_COLS; i++) {
        r = i + offset + LAYER_RED_OFFSET;
        g = i + offset + LAYER_GREEN_OFFSET;
        b = i + offset + LAYER_BLUE_OFFSET;

        // Convert 8 bit to 12 bit equivalent
        tlc5940_write(0, i, layer_update_ptr[r] << 4 | layer_update_ptr[r] >> 4);
        tlc5940_write(1, i, layer_update_ptr[g] << 4 | layer_update_ptr[g] >> 4);
        tlc5940_write(2, i, layer_update_ptr[b] << 4 | layer_update_ptr[b] >> 4);
    }
}

void tlc5940_latch_handler(void)
{        
    atomic_reg_ptr_clr(layer_row_previous_io->lat, layer_row_previous_io->mask);
    atomic_reg_ptr_set(layer_row_io->lat, layer_row_io->mask);
    
    layer_row_index = (unsigned int)(layer_row_io - layer_io);
    layer_row_previous_io = layer_row_io;
    if(layer_row_index >= (LAYER_NUM_OF_ROWS - 1))
        layer_row_io = layer_io;
    else
        layer_row_io++;
}

static int layer_rtask_init(void)
{
    // Configure PPS
    sys_unlock();
    LAYER_SDI_PPS = LAYER_SDI_PPS_WORD;
    LAYER_SS_PPS = LAYER_SS_PPS_WORD;
    sys_lock();
    
    // Configure IO
    REG_CLR(LAYER_SS_ANSEL, LAYER_SS_PIN_MASK);
    REG_SET(LAYER_SDI_TRIS, LAYER_SDI_PIN_MASK);
    REG_SET(LAYER_SCK_TRIS, LAYER_SCK_PIN_MASK);
    REG_SET(LAYER_SS_TRIS, LAYER_SS_PIN_MASK);
    
    const struct layer_io* io = NULL;
    for(unsigned int i = 0; i < LAYER_NUM_OF_ROWS; ++i) {
        io = &layer_io[i];
        if(io->ansel != NULL)
            atomic_reg_ptr_clr(io->ansel, io->mask);
        atomic_reg_ptr_clr(io->tris, io->mask);
        atomic_reg_ptr_clr(io->lat, io->mask);
    }

    // Initialize timer
    layer_countdown_timer = timer_construct(TIMER_TYPE_COUNTDOWN, NULL);
    if(layer_countdown_timer == NULL)
        goto fail_timer;
    
    // Initialize DMA
    layer_dma_channel = dma_construct(layer_dma_config);
    if(layer_dma_channel == NULL)
        goto fail_dma;
    
    // Initialize SPI
    layer_spi_module = spi_construct(LAYER_SPI_CHANNEL, layer_spi_config);
    if(layer_spi_module == NULL)
        goto fail_spi;
    spi_configure_dma_src(layer_spi_module, layer_dma_channel); // SPI module is the source of the dma module
    spi_enable(layer_spi_module);
    
    // Enables the TLC5940's by which the update & latch handlers will be getting called
    tlc5940_enable();
    
    // Detect open LEDs on boot
    layer_exec_lod();
   
    return KERN_INIT_SUCCCES;

fail_spi:
    dma_destruct(layer_dma_channel);
fail_dma:
    timer_destruct(layer_countdown_timer);
fail_timer:

    return KERN_INIT_FAILED;
}

static void layer_rtask_execute(void)
{
    switch(layer_state) {
        default:
        case LAYER_IDLE:
            break;
    }
}

static void layer_dma_rtask_execute(void)
{
    switch(layer_dma_state) {
        default:
        case LAYER_DMA_RECV_FRAME:
            // @Todo: add timeout timer
            // @Todo: maybe add something to reset the DMA in case we are out of sync with the master
            // especially when we add the timeout timer, because if a timeout happens we know that
            // we are out of sync!
            if(dma_ready(layer_dma_channel)) {
                dma_configure_dst(layer_dma_channel, layer_dma_ptr, LAYER_FRAME_BUFFER_SIZE);
                dma_enable_transfer(layer_dma_channel);
                layer_dma_state = LAYER_DMA_RECV_FRAME_WAIT;
            }
            break;
        case LAYER_DMA_RECV_FRAME_WAIT:
            if(dma_ready(layer_dma_channel)) {
                // @Todo: buffer swap should happen by external command
                // @Todo: eventually sync this in latch callback to avoid swapping buffers mid-frame
                layer_swap_buffers();
                layer_dma_state = LAYER_DMA_RECV_FRAME;
            }
            break;
    }
}

bool layer_busy(void)
{
    return layer_state != LAYER_IDLE;
}

bool layer_ready(void)
{
    return !layer_busy();
}

bool layer_exec_lod(void)
{
    if(layer_busy())
        return false;
    
    // @Todo: eventually start LOD execution
    return true;
}
