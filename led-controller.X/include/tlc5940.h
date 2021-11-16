#ifndef TLC5940_H
#define TLC5940_H

#include <stdbool.h>

enum tlc5940_mode
{
    TLC5940_MODE_ENABLED = 0,
    TLC5940_MODE_DISABLED,
    TLC5940_MODE_LOD,
};

enum tlc5940_mode tlc5940_get_mode(void);
void tlc5940_switch_mode(enum tlc5940_mode mode);
bool tlc5940_get_lod_error(void);
void tlc5940_write(unsigned int device, unsigned int channel, unsigned short pwm_value);
void tlc5940_write_all_channels(unsigned int device, unsigned short pwm_value);

#endif /* TLC5940_H */