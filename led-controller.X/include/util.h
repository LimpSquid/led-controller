#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <xc.h>

// Atomic register utils
#define ATOMIC_REG(name)                atomic_reg_group_t name
#define ATOMIC_REG_VALUE(reg)           ATOMIC_REG_PTR_VALUE(&reg)
#define ATOMIC_REG_CLR(reg, mask)       ATOMIC_REG_PTR_CLR(&reg, mask)
#define ATOMIC_REG_SET(reg, mask)       ATOMIC_REG_PTR_SET(&reg, mask)
#define ATOMIC_REG_INV(reg, mask)       ATOMIC_REG_PTR_INV(&reg, mask)

#define ATOMIC_REG_PTR(name)            atomic_reg_group_t* name
#define ATOMIC_REG_PTR_CAST(addr)       ((atomic_reg_group_t*)addr)
#define ATOMIC_REG_PTR_VALUE(reg)       *(((atomic_reg_ptr_t)reg) + 0)
#define ATOMIC_REG_PTR_CLR(reg, mask)   *(((atomic_reg_ptr_t)reg) + 1) = mask
#define ATOMIC_REG_PTR_SET(reg, mask)   *(((atomic_reg_ptr_t)reg) + 2) = mask
#define ATOMIC_REG_PTR_INV(reg, mask)   *(((atomic_reg_ptr_t)reg) + 3) = mask

typedef volatile unsigned int atomic_reg_t;
typedef atomic_reg_t* atomic_reg_ptr_t;
typedef struct                  
{  
    atomic_reg_t reg;          
    atomic_reg_t clr;           
    atomic_reg_t set;             
    atomic_reg_t inv;       
} atomic_reg_group_t;

// Bit manipulation utils
#define MASK(mask, shift)       MASK_SHIFT(mask, shift)
#define MASK_SHIFT(mask, shift) ((unsigned int)mask << shift)

#define BIT(shift)              BIT_SHIFT(shift)
#define BIT_SHIFT(shift)        (1U << shift)

#define REG_SET(reg, mask)      (reg |= mask)
#define REG_CLR(reg, mask)      (reg &= ~mask)
#define REG_INV(reg, mask)      (reg ^= mask)

// IO utils
#define IO_READ(pin)            !!(ATOMIC_REG_PTR_VALUE((pin).port) & (pin).mask)
#define IO_SET(pin)             ATOMIC_REG_PTR_SET((pin).lat, (pin).mask)
#define IO_CLR(pin)             ATOMIC_REG_PTR_CLR((pin).lat, (pin).mask)
#define IO_INV(pin)             ATOMIC_REG_PTR_INV((pin).lat, (pin).mask)
#define IO_PULSE(pin)           do { IO_INV(pin); IO_INV(pin); } while(0)
#define IO_SETCLR(pin)          do { IO_SET(pin); IO_CLR(pin); } while(0)
#define IO_CLRSET(pin)          do { IO_CLR(pin); IO_SET(pin); } while(0)

// Variant for struct io_pin pointers
#define IO_PTR_READ(pin)        !!(ATOMIC_REG_PTR_VALUE(pin->port) & pin->mask)
#define IO_PTR_SET(pin)         ATOMIC_REG_PTR_SET(pin->lat, pin->mask)
#define IO_PTR_CLR(pin)         ATOMIC_REG_PTR_CLR(pin->lat, pin->mask)
#define IO_PTR_INV(pin)         ATOMIC_REG_PTR_INV(pin->lat, pin->mask)
#define IO_PTR_PULSE(pin)       do { IO_PTR_INV(pin); IO_PTR_INV(pin); } while(0)
#define IO_PTR_SETCLR(pin)      do { IO_PTR_SET(pin); IO_PTR_CLR(pin); } while(0)
#define IO_PTR_CLRSET(pin)      do { IO_PTR_CLR(pin); IO_PTR_SET(pin); } while(0)

#define IO_PIN(pin, bank) \
    { \
        .ansel = NULL, \
        .tris = ATOMIC_REG_PTR_CAST(&TRIS##bank), \
        .lat = ATOMIC_REG_PTR_CAST(&LAT##bank), \
        .port = ATOMIC_REG_PTR_CAST(&PORT##bank), \
        .mask = BIT(pin) \
    }
#define IO_ANLG_PIN(pin, bank) \
    { \
        .ansel = ATOMIC_REG_PTR_CAST(&ANSEL##bank), \
        .tris = ATOMIC_REG_PTR_CAST(&TRIS##bank), \
        .lat = ATOMIC_REG_PTR_CAST(&LAT##bank), \
        .port = ATOMIC_REG_PTR_CAST(&PORT##bank), \
        .mask = BIT(pin) \
    }

struct io_pin
{
    ATOMIC_REG_PTR(ansel);
    ATOMIC_REG_PTR(tris);
    ATOMIC_REG_PTR(lat);
    ATOMIC_REG_PTR(port);
    
    unsigned int mask;
};

enum io_direction
{
    IO_DIRECTION_DIN = 0,
    IO_DIRECTION_DOUT_LOW,
    IO_DIRECTION_DOUT_HIGH,
};

void io_configure(enum io_direction direction, const struct io_pin* pins, unsigned int size);

// CRC utils
typedef unsigned short crc16_t;
void crc16_reset(crc16_t* crc);
void crc16_update(crc16_t* crc, const unsigned char* data, unsigned int size);

#endif /* UTIL_H */