#include <core/util.h>
#include <core/assert.h>

void io_configure(enum io_direction direction, struct io_pin const * pins, unsigned int size)
{
    ASSERT_NOT_NULL(pins);

    struct io_pin const * pin = pins; // Just for the name
    for (unsigned int i = 0; i < size; ++i) {
        switch (direction) {
            default:
            case IO_DIRECTION_DIN:
                if (pin->ansel != NULL)
                    ATOMIC_REG_PTR_CLR(pin->ansel, pin->mask);
                ATOMIC_REG_PTR_SET(pin->tris, pin->mask);
                break;
            case IO_DIRECTION_DOUT_LOW:
            case IO_DIRECTION_DOUT_HIGH:
                if (pin->ansel != NULL)
                    ATOMIC_REG_PTR_CLR(pin->ansel, pin->mask);
                if (direction == IO_DIRECTION_DOUT_LOW)
                    ATOMIC_REG_PTR_CLR(pin->lat, pin->mask);
                else
                    ATOMIC_REG_PTR_SET(pin->lat, pin->mask);
                ATOMIC_REG_PTR_CLR(pin->tris, pin->mask);
                break;
        }
        ++pin;
    }
}