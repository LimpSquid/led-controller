#ifndef LAYER_H
#define LAYER_H

#include <stdbool.h>

enum layer_buffer_swap_mode
{
    LAYER_BUFFER_SWAP_MANUAL = 0,
    LAYER_BUFFER_SWAP_AUTO,
};

bool layer_busy(void);
bool layer_ready(void);
bool layer_exec_lod(void);
bool layer_dma_error(void);
void layer_dma_reset(void);
bool layer_dma_swap_buffers(void);
enum layer_buffer_swap_mode layer_get_buffer_swap_mode(void);
void layer_set_buffer_swap_mode(enum layer_buffer_swap_mode mode);

// Used for test suite
struct layer_color
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

void layer_draw_pixel(unsigned char x, unsigned char y, struct layer_color color);
void layer_draw_all_pixels(struct layer_color color);
void layer_clear_all_pixels(void);

#endif /* LAYER_H */