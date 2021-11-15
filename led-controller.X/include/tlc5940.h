#ifndef TLC5940_H
#define TLC5940_H

#include <stdbool.h>

void tlc5940_enable(void);
void tlc5940_disable(void);
void tlc5940_lod_mode(void);
void tlc5940_write(unsigned int device, unsigned int channel, unsigned short value);
void tlc5940_write_all_channels(unsigned int device, unsigned short value);

#endif /* TLC5940_H */