#ifndef LAYER_H
#define LAYER_H

#include <stdbool.h>

bool layer_busy(void);
bool layer_ready(void);
bool layer_exec_lod(void);
void layer_dma_reset(void);
bool layer_dma_ready_to_recv(void);
bool layer_dma_swap_buffers(void);

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
void layer_swap_buffers(void);

#endif /* LAYER_H */