#ifndef LAYER_H
#define	LAYER_H

#include <stdbool.h>

bool layer_busy(void);
bool layer_ready(void);
bool layer_exec_lod(void);
void layer_dma_reset(void);
bool layer_dma_swap_buffers(void);

#endif	/* LAYER_H */