#ifndef UTIL_H
#define	UTIL_H

#include <atomic_reg.h>
#include <stdbool.h>

// Bit manipulation utils
#define MASK(mask, shift)       MASK_SHIFT(mask, shift)
#define MASK_SHIFT(mask, shift) ((unsigned int)mask << shift)

#define BIT(shift)              BIT_SHIFT(shift)
#define BIT_SHIFT(shift)        (1U << shift)

#define REG_SET(reg, mask)      (reg |= mask)
#define REG_CLR(reg, mask)      (reg &= ~mask)
#define REG_INV(reg, mask)      (reg ^= mask)

// IO utils
#define IO_PIN(pin, bank) \
    { \
        .ansel = NULL, \
        .tris = atomic_reg_ptr_cast(&TRIS##bank), \
        .lat = atomic_reg_ptr_cast(&LAT##bank), \
        .port = atomic_reg_ptr_cast(&PORT##bank), \
        .mask = BIT(pin) \
    }
#define IO_ANSEL_PIN(pin, bank) \
    { \
        .ansel = atomic_reg_ptr_cast(&ANSEL##bank), \
        .tris = atomic_reg_ptr_cast(&TRIS##bank), \
        .lat = atomic_reg_ptr_cast(&LAT##bank), \
        .port = atomic_reg_ptr_cast(&PORT##bank), \
        .mask = BIT(pin) \
    }

struct io_pin
{
    atomic_reg_ptr(ansel);
    atomic_reg_ptr(tris);
    atomic_reg_ptr(lat);
    atomic_reg_ptr(port);
    
    unsigned int mask;
};

enum io_direction
{
    IO_DIRECTION_DIN = 0,
    IO_DIRECTION_DOUT_LOW,
    IO_DIRECTION_DOUT_HIGH,
};

void io_configure(enum io_direction direction, const struct io_pin* pins, unsigned int size);
bool io_read(const struct io_pin* pin);
void io_set(const struct io_pin* pin, bool level);

// CRC utils
typedef unsigned short crc16_t;
void crc16_reset(crc16_t* crc);
void crc16_update(crc16_t* crc, const unsigned char* data, unsigned int size);

#endif	/* UTIL_H */