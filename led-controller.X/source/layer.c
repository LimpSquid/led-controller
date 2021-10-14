#include <layer.h>
#include <layer_config.h>
#include <kernel_task.h>
#include <tlc5940.h>
#include <spi.h>
#include <dma.h>
#include <sys.h>
#include <register.h>
#include <toolbox.h>
#include <stddef.h>
#include <xc.h>
#include <string.h>

#if !defined(LAYER_REFRESH_INTERVAL)
    #error "Layer refresh interval is not specified, please define 'LAYER_REFRESH_INTERVAL'"
#endif

#define LAYER_NUM_OF_ROWS           16
#define LAYER_NUM_OF_COLS           16
#define LAYER_NUM_OF_LEDS           (LAYER_NUM_OF_ROWS * LAYER_NUM_OF_COLS)
#define LAYER_RED_OFFSET            (LAYER_NUM_OF_LEDS * 0)
#define LAYER_GREEN_OFFSET          (LAYER_NUM_OF_LEDS * 1)
#define LAYER_BLUE_OFFSET           (LAYER_NUM_OF_LEDS * 2)
#define LAYER_FRAME_DEPTH           3 // RGB
#define LAYER_FRAME_BUFFER_SIZE     (LAYER_NUM_OF_LEDS * LAYER_FRAME_DEPTH)
#define LAYER_LOD_GRAYSCALE_VALUE   4095
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
    LAYER_EXEC_LOD,
    LAYER_EXEC_LOD_WRITE,
    LAYER_EXEC_LOD_WRITE_WAIT,
    LAYER_EXEC_LOD_READ_ERROR,
    LAYER_EXEC_LOD_SHOW,
    LAYER_EXEC_LOD_SHOW_WRITE,
    LAYER_EXEC_LOD_SHOW_WRITE_WAIT,
    LAYER_EXEC_LOD_SHOW_DONE,
    LAYER_EXEC_LOD_DONE,
};

enum layer_dma_state
{
    LAYER_DMA_RECEIVE_FRAME = 0,
    LAYER_DMA_RECEIVE_FRAME_WAIT,
};

static void layer_row_io_reset(void);
static unsigned int layer_next_row_index(void);
static int layer_ttask_init(void);
static void layer_ttask_execute(void);
static void layer_ttask_configure(struct kernel_ttask_param* const param);
static int layer_rtask_init(void);
static void layer_rtask_execute(void);
static void layer_dma_rtask_execute(void);
KERN_TTASK(layer, layer_ttask_init, layer_ttask_execute, layer_ttask_configure, KERN_INIT_LATE);
KERN_SIMPLE_RTASK(layer, layer_rtask_init, layer_rtask_execute);
KERN_SIMPLE_RTASK(layer_dma, NULL, layer_dma_rtask_execute);

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

static const struct dma_config layer_dma_config; // No special config needed
static const struct spi_config layer_spi_config =
{
    .spi_con_flags = SPI_SRXISEL_NOT_EMPTY | SPI_DISSDO | SPI_MODE8 | SPI_SSEN,
};

static bool layer_lod_errors[LAYER_FRAME_DEPTH][LAYER_NUM_OF_ROWS]; // channel error is indexed by layer_lod_errors[tlc5940_index][row_index]
static unsigned char layer_front_buffer[LAYER_FRAME_BUFFER_SIZE];
static unsigned char layer_back_buffer[LAYER_FRAME_BUFFER_SIZE];
static unsigned char* layer_dma_ptr = layer_back_buffer;
static unsigned char* layer_draw_ptr = layer_front_buffer;
static const struct layer_io* layer_row_io = layer_io;
static const struct layer_io* layer_row_previous_io = &layer_io[LAYER_NUM_OF_ROWS - 1];
static struct dma_channel* layer_dma_channel = NULL;
static struct spi_module* layer_spi_module = NULL;
static enum layer_state layer_state = LAYER_IDLE;
static enum layer_dma_state layer_dma_state = LAYER_DMA_RECEIVE_FRAME;
static unsigned int layer_row_index = 0;
static unsigned int layer_red_index = 0;
static unsigned int layer_green_index = 0;
static unsigned int layer_blue_index = 0;
static unsigned int layer_offset = 0;
static unsigned int layer_tlc5940_index = 0; // index of tlc5940 
static bool layer_suppress_ttask = false;

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
    
    layer_state = LAYER_EXEC_LOD;
    return true;
}

void tlc5940_latch_callback(void)
{        
    atomic_reg_ptr_clr(layer_row_previous_io->lat, layer_row_previous_io->mask);
    atomic_reg_ptr_set(layer_row_io->lat, layer_row_io->mask);
    
    layer_row_index = (unsigned int)(layer_row_io - layer_io);
    layer_row_previous_io = layer_row_io;
    if(layer_row_index >= (LAYER_NUM_OF_ROWS - 1))
        layer_row_io = layer_io;
    else
        layer_row_io++;
        
    // @Todo: We actually have a hardware issue here. The 10kOhm pull-ups on the
    // FETs are not low enough to quickly charge the gate and switch the the device off.
    // When this routine is finished the TLCs are turned back on, while the FET of
    // the previous IO is still not completely turned off. This minor cross-over area
    // makes the LEDs connected to the previous IO faintly turn on. We should really solve this
    // by lowering the value of the pull-ups and not nopping around.
    Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();
    Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();
    Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();
    Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();
    Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();
    Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();
}

static void layer_row_io_reset(void)
{
    atomic_reg_ptr_clr(layer_row_io->lat, layer_row_io->mask);
    atomic_reg_ptr_clr(layer_row_previous_io->lat, layer_row_previous_io->mask);
    
    layer_row_io = layer_io;
    layer_row_previous_io = &layer_io[LAYER_NUM_OF_ROWS - 1];
    layer_row_index = 0;
}

static unsigned int layer_next_row_index(void)
{   
    unsigned int port = atomic_reg_ptr_value(layer_io[layer_row_index].port);
    if(port & layer_io[layer_row_index].mask)
        return (layer_row_index + 1) % LAYER_NUM_OF_ROWS;
    
    // This means that layer_row_io_reset() was just called (or a CPU reset)
    // and we have yet to make the first row active in tlc5940_latch_callback())
    return 0; 
}

static int layer_ttask_init(void)
{
    const struct layer_io* io = NULL;
    
    // Initialize IO
    for(unsigned int i = 0; i < LAYER_NUM_OF_ROWS; ++i) {
        io = &layer_io[i];
        if(NULL != io->ansel)
            atomic_reg_ptr_clr(io->ansel, io->mask);
        atomic_reg_ptr_clr(io->tris, io->mask);
        atomic_reg_ptr_clr(io->lat, io->mask);
    }
    
    return KERN_INIT_SUCCCES;
}

static void layer_ttask_execute(void)
{   
    if(layer_suppress_ttask)
        return;
    
    if(tlc5940_ready()) {
        layer_offset = layer_next_row_index() * LAYER_NUM_OF_COLS;
        for(unsigned int i = 0; i < LAYER_NUM_OF_COLS; i++) {
            // Convert 8 bit to 12 bit equivalent
            layer_red_index = i + layer_offset + LAYER_RED_OFFSET;
            layer_green_index = i + layer_offset + LAYER_GREEN_OFFSET;
            layer_blue_index = i + layer_offset + LAYER_BLUE_OFFSET;
            
            tlc5940_write_grayscale(0, i, (layer_dma_ptr[layer_red_index] << 4) | (layer_dma_ptr[layer_red_index] >> 4));
            tlc5940_write_grayscale(1, i, (layer_dma_ptr[layer_green_index] << 4) | (layer_dma_ptr[layer_green_index] >> 4));
            tlc5940_write_grayscale(2, i, (layer_dma_ptr[layer_blue_index] << 4) | (layer_dma_ptr[layer_blue_index] >> 4));
        }
        tlc5940_update();
    }
}

static void layer_ttask_configure(struct kernel_ttask_param* const param)
{
    kernel_ttask_set_priority(param, KERN_TTASK_PRIORITY_HIGH);
    kernel_ttask_set_interval(param, LAYER_REFRESH_INTERVAL, KERN_TIME_UNIT_US);
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

    // Initialize DMA
    layer_dma_channel = dma_construct(layer_dma_config);
    if(NULL == layer_dma_channel)
        goto deinit_dma;
    
    // Initialize SPI
    layer_spi_module = spi_construct(LAYER_SPI_CHANNEL, layer_spi_config);
    if(NULL == layer_spi_module)
        goto deinit_spi;
    spi_configure_dma_src(layer_spi_module, layer_dma_channel); // SPI module is the source of the dma module
    spi_enable(layer_spi_module);
    
    // Detect open LEDs on boot
    layer_exec_lod();
   
    return KERN_INIT_SUCCCES;
    
deinit_spi:
    spi_destruct(layer_spi_module);
deinit_dma:
    dma_destruct(layer_dma_channel);

    return KERN_INIT_FAILED;
}

static void layer_rtask_execute(void)
{
    switch(layer_state) {
        default:
        case LAYER_IDLE:
            break;
        case LAYER_EXEC_LOD:
            layer_suppress_ttask = true;
            layer_tlc5940_index = 0;
            layer_row_io_reset();
            memset(layer_lod_errors, 0, LAYER_FRAME_DEPTH * LAYER_NUM_OF_ROWS);
            layer_state = LAYER_EXEC_LOD_WRITE;
            break;
        case LAYER_EXEC_LOD_WRITE:
            for(unsigned int i = 0; i < LAYER_NUM_OF_COLS; i++)
                tlc5940_write_grayscale(layer_tlc5940_index, i, LAYER_LOD_GRAYSCALE_VALUE);
            tlc5940_update();
            layer_state = LAYER_EXEC_LOD_WRITE_WAIT;
            break;
        case LAYER_EXEC_LOD_WRITE_WAIT:
            if (tlc5940_ready())
                layer_state = LAYER_EXEC_LOD_READ_ERROR;
            // Ideally we would wait at least 1us as specified by the TLC5940's datasheet
            break;
        case LAYER_EXEC_LOD_READ_ERROR:
            layer_lod_errors[layer_tlc5940_index][layer_row_index] |= tlc5940_read_lod_error();
            layer_state = (layer_row_index != (LAYER_NUM_OF_ROWS - 1) || ++layer_tlc5940_index < LAYER_FRAME_DEPTH)
                ? LAYER_EXEC_LOD_WRITE // more rows to check for errors
                : LAYER_EXEC_LOD_SHOW; // done with checking LOD errors for each row and color
            break;
        case LAYER_EXEC_LOD_SHOW:
            layer_tlc5940_index = 0;
            layer_state = LAYER_EXEC_LOD_SHOW_WRITE;
            break;
        case LAYER_EXEC_LOD_SHOW_WRITE: {
            unsigned int next_row = layer_next_row_index(); // need the next row index, because that is what we're about to write
            bool error = layer_lod_errors[layer_tlc5940_index][next_row];
            
            for(unsigned int i = 0; i < LAYER_NUM_OF_COLS; i++)
                tlc5940_write_grayscale(layer_tlc5940_index, i, LAYER_LOD_GRAYSCALE_VALUE * error);
            tlc5940_update();
            layer_state = LAYER_EXEC_LOD_SHOW_WRITE_WAIT;
            break;
        }
        case LAYER_EXEC_LOD_SHOW_WRITE_WAIT:
            if (tlc5940_ready()) {
                // Give user some time to see the broken LED
                bool error = layer_lod_errors[layer_tlc5940_index][layer_row_index];
                if (error) for (unsigned int i = 0; i < 1000000; i++) Nop(); // @Commit: change with timer
                layer_state = LAYER_EXEC_LOD_SHOW_DONE;
            }
            break;
        case LAYER_EXEC_LOD_SHOW_DONE:
            layer_state = (layer_row_index != (LAYER_NUM_OF_ROWS - 1) || ++layer_tlc5940_index < LAYER_FRAME_DEPTH)
                ? LAYER_EXEC_LOD_SHOW_WRITE
                : LAYER_EXEC_LOD_DONE; // shown all error'ed rows
            break;
        case LAYER_EXEC_LOD_DONE:
            layer_suppress_ttask = false;
            layer_state = LAYER_IDLE;
            break;
    }
}

static void layer_dma_rtask_execute(void)
{
    switch(layer_dma_state) {
        default:
        case LAYER_DMA_RECEIVE_FRAME:
            // @Todo: add timeout timer
            // @Todo: maybe add something to reset the DMA in case we are out of sync with the master
            // especially when we add the timeout timer, because if a timeout happens we know that
            // we are out of sync!
            if(dma_ready(layer_dma_channel)) {
                dma_configure_dst(layer_dma_channel, layer_dma_ptr, LAYER_FRAME_BUFFER_SIZE); // @todo: eventually use layer_draw_ptr
                dma_enable_transfer(layer_dma_channel);
                layer_dma_state = LAYER_DMA_RECEIVE_FRAME_WAIT;
            }
            break;
        case LAYER_DMA_RECEIVE_FRAME_WAIT:
            if(dma_ready(layer_dma_channel))
                layer_dma_state = LAYER_DMA_RECEIVE_FRAME;
            break;
    }
}