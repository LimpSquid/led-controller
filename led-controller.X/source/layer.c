#include <layer.h>
#include <layer_config.h>
#include <kernel_task.h>
#include <tlc5940.h>
#include <spi.h>
#include <dma.h>
#include <timer.h>
#include <sys.h>
#include <util.h>
#include <stddef.h>
#include <string.h>
#include <xc.h>
#include <assert.h>

#define LAYER_NUM_OF_ROWS           16
#define LAYER_NUM_OF_COLS           16
#define LAYER_NUM_OF_LEDS           (LAYER_NUM_OF_ROWS * LAYER_NUM_OF_COLS)
#define LAYER_RED_OFFSET            (LAYER_NUM_OF_LEDS * 0)
#define LAYER_GREEN_OFFSET          (LAYER_NUM_OF_LEDS * 1)
#define LAYER_BLUE_OFFSET           (LAYER_NUM_OF_LEDS * 2)
#define LAYER_FRAME_DEPTH           3 // RGB
#define LAYER_FRAME_BUFFER_SIZE     (LAYER_NUM_OF_LEDS * LAYER_FRAME_DEPTH)
#define LAYER_LOD_SETTLE_DELAY      10 // In microseconds, datasheet specs atleast 15 * td (20ns) + tpd2 (1us typ.)
#define LAYER_LOD_ERROR_DELAY       1000 // In milliseconds
#define LAYER_OFFSET(index)         (index * LAYER_NUM_OF_COLS)

#define LAYER_SPI_CHANNEL           SPI_CHANNEL1
#define LAYER_SDI_PPS_REG           SDI1R
#define LAYER_SS_PPS_REG            SS1R

#define LAYER_SDI_PPS_WORD          0xe
#define LAYER_SS_PPS_WORD           0x3

struct layer_flags
{
    // Don't use a bit-field here as the flags are used from an ISR,
    // thus we require atomic access. A bit-field will use a LBU (load
    // byte) instruction which is not read-modify-write safe.

    volatile bool do_buffer_swap; // Set flag to schedule a buffer swap on the next vertical sync
    volatile bool requires_buffer_swap; // Flag is set if a buffer swap is needed
};

enum layer_state
{
    LAYER_SWITCH_ENABLED_MODE = 0, // Jump to this state to switch to enabled mode before going idle
    LAYER_SWITCH_ENABLED_MODE_WAIT,
    LAYER_IDLE,

    LAYER_EXEC_LOD,
    LAYER_EXEC_LOD_SWITCH_MODE,
    LAYER_EXEC_LOD_SWITCH_MODE_WAIT,
    LAYER_EXEC_LOD_ADVANCE,
    LAYER_EXEC_LOD_SETTLE_WAIT,
    LAYER_EXEC_LOD_ERROR_WAIT,
};

enum layer_dma_state
{
    LAYER_DMA_RECV_FRAME_INIT = 0,
    LAYER_DMA_RECV_FRAME
};

static int layer_rtask_init(void);
static void layer_rtask_execute(void);
static void layer_dma_rtask_execute(void);
KERN_SIMPLE_RTASK(layer, layer_rtask_init, layer_rtask_execute)
KERN_SIMPLE_RTASK(layer_dma, NULL, layer_dma_rtask_execute) // Init is done in layer_rtask_init, no need to make a separate init

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

static const struct io_pin layer_pins[LAYER_NUM_OF_ROWS] =
{
    IO_PIN(6, D), // Row 1
    IO_PIN(4, D), // Row 3
    IO_ANLG_PIN(2, D), // ...
    IO_PIN(0, D),
    IO_ANLG_PIN(2, E),
    IO_PIN(0, E),
    IO_PIN(10, D),
    IO_PIN(8, D),
    IO_PIN(7, D), // Row 0
    IO_PIN(5, D), // Row 2
    IO_ANLG_PIN(3, D), // ...
    IO_ANLG_PIN(1, D),
    IO_PIN(3, E),
    IO_PIN(1, E),
    IO_PIN(11, D),
    IO_PIN(9, D),
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

static const struct io_pin layer_pins[LAYER_NUM_OF_ROWS] =
{
    IO_PIN(7, D), // Row 0
    IO_PIN(6, D), // Row 1
    IO_PIN(5, D), // ...
    IO_PIN(4, D),
    IO_ANLG_PIN(3, D),
    IO_ANLG_PIN(2, D),
    IO_ANLG_PIN(1, D),
    IO_PIN(0, D),
    IO_PIN(3, E),
    IO_ANLG_PIN(2, E),
    IO_PIN(1, E),
    IO_PIN(0, E),
    IO_PIN(11, D),
    IO_PIN(10, D),
    IO_PIN(9, D),
    IO_PIN(8, D),
};
#endif

static struct dma_config const layer_dma_config; // No special config needed
static struct spi_config const layer_spi_config =
{
    .spi_con_flags = SPI_SRXISEL_NOT_EMPTY | SPI_DISSDO | SPI_MODE8 | SPI_SSEN,
};

static struct io_pin const layer_sdi_pin = IO_PIN(2, F);
static struct io_pin const layer_sck_pin = IO_PIN(6, F);
static struct io_pin const layer_ss_pin = IO_ANLG_PIN(15, B);

static struct layer_flags layer_flags;

static unsigned char layer_buffer_pool[3][LAYER_FRAME_BUFFER_SIZE]; // Tripple buffering to already receive new pixel data while waiting on the vertical sync swap
static unsigned char * layer_draw_buffer = layer_buffer_pool[0]; // Buffer to is used to write data to the TLC5940s
static unsigned char * layer_sync_buffer = layer_buffer_pool[1]; // Buffer that contains a new frame but is still waiting to be swapped with draw_buffer
static unsigned char * layer_recv_buffer = layer_buffer_pool[2]; // Buffer for receiving pixel data to form a new frame
static struct io_pin const * layer_row_pin = layer_pins;
static struct io_pin const * layer_row_previous_pin = &layer_pins[LAYER_NUM_OF_ROWS - 1];
static struct dma_channel * layer_dma_channel;
static struct spi_module * layer_spi_module;
static struct timer_module * layer_countdown_timer;
static enum layer_state layer_state = LAYER_SWITCH_ENABLED_MODE;
static enum layer_dma_state layer_dma_state = LAYER_DMA_RECV_FRAME;
static unsigned int layer_row_index; // Active row, corresponding row IO is layer_pins[layer_row_index]

static void layer_row_reset(void)
{
    IO_PTR_CLR(layer_row_previous_pin);
    IO_PTR_CLR(layer_row_pin);

    layer_row_pin = layer_pins;
    layer_row_previous_pin = &layer_pins[LAYER_NUM_OF_ROWS - 1];
    layer_row_index = 0;
}

inline static bool __attribute__((always_inline)) layer_row_at_end(void)
{
    return layer_row_index == (LAYER_NUM_OF_ROWS - 1);
}

inline static unsigned int __attribute__((always_inline)) layer_next_row_index(void)
{
    if (IO_READ(layer_pins[layer_row_index]))
        return (layer_row_index + 1) % LAYER_NUM_OF_ROWS;

    // This means that layer_row_reset() was just called (or a CPU reset)
    // and we have yet to make the first row active in tlc5940_latch_callback
    return 0;
}

inline static void  __attribute__((always_inline)) layer_possibly_swap_buffers(void)
{
    if (layer_flags.do_buffer_swap) {
        ASSERT(layer_flags.requires_buffer_swap);
        unsigned char * tmp = layer_draw_buffer;
        layer_draw_buffer = layer_sync_buffer;
        layer_sync_buffer = tmp;
        layer_flags.do_buffer_swap = false;
        layer_flags.requires_buffer_swap = false;
    }
}

inline static void  __attribute__((always_inline)) layer_advance_row(void)
{
    IO_PTR_CLR(layer_row_previous_pin);
    IO_PTR_SET(layer_row_pin);

    layer_row_index = (unsigned int)(layer_row_pin - layer_pins);
    layer_row_previous_pin = layer_row_pin;
    if (layer_row_at_end()) {
        layer_possibly_swap_buffers();
        layer_row_pin = layer_pins;
    } else
        layer_row_pin++;
}

void tlc5940_update_handler(void)
{
    unsigned int offset = layer_offset[layer_next_row_index()];

    tlc5940_write_channels_mode8(0, &layer_draw_buffer[offset + LAYER_BLUE_OFFSET]);
    tlc5940_write_channels_mode8(1, &layer_draw_buffer[offset + LAYER_GREEN_OFFSET]);
    tlc5940_write_channels_mode8(2, &layer_draw_buffer[offset + LAYER_RED_OFFSET]);
}

void tlc5940_latch_handler(void)
{
    layer_advance_row();
}

static int layer_rtask_init(void)
{
    // Configure PPS
    sys_unlock();
    LAYER_SDI_PPS_REG = LAYER_SDI_PPS_WORD;
    LAYER_SS_PPS_REG = LAYER_SS_PPS_WORD;
    sys_lock();

    // Configure IO
    io_configure(IO_DIRECTION_DIN, &layer_sdi_pin, 1);
    io_configure(IO_DIRECTION_DIN, &layer_sck_pin, 1);
    io_configure(IO_DIRECTION_DIN, &layer_ss_pin, 1);
    io_configure(IO_DIRECTION_DOUT_LOW, layer_pins, LAYER_NUM_OF_ROWS);

    // Initialize timer
    layer_countdown_timer = timer_construct(TIMER_TYPE_COUNTDOWN, NULL);
    if (layer_countdown_timer == NULL)
        goto fail_timer;

    // Initialize DMA
    layer_dma_channel = dma_construct(layer_dma_config);
    if (layer_dma_channel == NULL)
        goto fail_dma;

    // Initialize SPI
    layer_spi_module = spi_construct(LAYER_SPI_CHANNEL, layer_spi_config);
    if (layer_spi_module == NULL)
        goto fail_spi;
    spi_configure_dma_src(layer_spi_module, layer_dma_channel); // SPI module is the source of the dma module
    spi_enable(layer_spi_module);

    return KERN_INIT_SUCCESS;

fail_spi:
    dma_destruct(layer_dma_channel);
fail_dma:
    timer_destruct(layer_countdown_timer);
fail_timer:

    return KERN_INIT_FAILED;
}

static void layer_rtask_execute(void)
{
    switch (layer_state) {
        default:
        case LAYER_SWITCH_ENABLED_MODE:
            tlc5940_switch_mode(TLC5940_MODE_ENABLED);
            layer_state = LAYER_SWITCH_ENABLED_MODE_WAIT;
            break;
        case LAYER_SWITCH_ENABLED_MODE_WAIT:
            if (tlc5940_get_mode() == TLC5940_MODE_ENABLED) {
                layer_row_reset(); // Reset once we are enabled
                layer_state = LAYER_IDLE;
            }
            break;
        case LAYER_IDLE:
            break;

        case LAYER_EXEC_LOD:
        case LAYER_EXEC_LOD_SWITCH_MODE:
            tlc5940_switch_mode(TLC5940_MODE_LOD);
            layer_state = LAYER_EXEC_LOD_SWITCH_MODE_WAIT;
            break;
        case LAYER_EXEC_LOD_SWITCH_MODE_WAIT:
            if (tlc5940_get_mode() == TLC5940_MODE_LOD) {
                layer_row_reset();
                layer_state = LAYER_EXEC_LOD_ADVANCE;
            }
            break;
        case LAYER_EXEC_LOD_ADVANCE:
            layer_advance_row();

            timer_start(layer_countdown_timer, LAYER_LOD_SETTLE_DELAY, TIMER_TIME_UNIT_US);
            layer_state = LAYER_EXEC_LOD_SETTLE_WAIT;
            break;
        case LAYER_EXEC_LOD_SETTLE_WAIT:
            if (!timer_is_running(layer_countdown_timer)) {
                bool error = tlc5940_get_lod_error();
                if (error) {
                    timer_start(layer_countdown_timer, LAYER_LOD_ERROR_DELAY, TIMER_TIME_UNIT_MS);
                    layer_state = LAYER_EXEC_LOD_ERROR_WAIT;
                } else
                    layer_state = layer_row_at_end()
                        ? LAYER_SWITCH_ENABLED_MODE // Done
                        : LAYER_EXEC_LOD_ADVANCE;
            }
            break;
        case LAYER_EXEC_LOD_ERROR_WAIT:
            if (!timer_is_running(layer_countdown_timer)) {
                layer_state = layer_row_at_end()
                    ? LAYER_SWITCH_ENABLED_MODE // Done
                    : LAYER_EXEC_LOD_ADVANCE;
            }
            break;
    }
}

static void layer_dma_rtask_execute(void)
{
    if (dma_busy(layer_dma_channel))
        return;

    switch (layer_dma_state) {
        default:
        case LAYER_DMA_RECV_FRAME_INIT:
            dma_configure_dst(layer_dma_channel, layer_recv_buffer, LAYER_FRAME_BUFFER_SIZE);
            dma_enable_transfer(layer_dma_channel);
            layer_dma_state = LAYER_DMA_RECV_FRAME;
            break;

        case LAYER_DMA_RECV_FRAME:
            if (!layer_flags.requires_buffer_swap) {
                // Swap buffers and start next transfer
                unsigned char * tmp = layer_sync_buffer;
                layer_sync_buffer = layer_recv_buffer;
                layer_recv_buffer = tmp;
                dma_configure_dst(layer_dma_channel, layer_recv_buffer, LAYER_FRAME_BUFFER_SIZE);
                dma_enable_transfer(layer_dma_channel);

                layer_flags.requires_buffer_swap = true;
#ifdef LAYER_AUTO_BUFFER_SWAP
#warning "AUTO_BUFFER_SWAP defined"
                layer_flags.do_buffer_swap = true; // Keep last
#endif
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
    if (layer_busy())
        return false;

    layer_state = LAYER_EXEC_LOD;
    return true;
}

void layer_dma_reset(void)
{
    dma_disable_transfer(layer_dma_channel);

    layer_flags.do_buffer_swap = false;
    layer_flags.requires_buffer_swap = false;
    layer_dma_state = LAYER_DMA_RECV_FRAME_INIT;
}

bool layer_dma_swap_buffers(void)
{
    if (!layer_flags.requires_buffer_swap)
        return false;

    layer_flags.do_buffer_swap = true;
    return true;
}

void layer_draw_pixel(unsigned char x, unsigned char y, struct layer_color color)
{
    if (x >= LAYER_NUM_OF_COLS)
        x = LAYER_NUM_OF_COLS - 1;
    if (y >= LAYER_NUM_OF_ROWS)
        y = LAYER_NUM_OF_ROWS - 1;

    unsigned int pos = x + y * LAYER_NUM_OF_ROWS;
    layer_draw_buffer[pos + LAYER_RED_OFFSET] = color.r;
    layer_draw_buffer[pos + LAYER_GREEN_OFFSET] = color.g;
    layer_draw_buffer[pos + LAYER_BLUE_OFFSET] = color.b;
}
void layer_draw_all_pixels(struct layer_color color)
{
    // Yep we can improve with a memset, but I'm lazy
    for (unsigned char x = 0; x < LAYER_NUM_OF_COLS; ++x)
        for (unsigned char y = 0; y < LAYER_NUM_OF_ROWS; ++y)
            layer_draw_pixel(x, y, color);
}

void layer_clear_all_pixels()
{
   struct layer_color color = {0};
   layer_draw_all_pixels(color);
}