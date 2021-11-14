#include <util.h>
#include <assert_util.h>

void io_configure(enum io_direction direction, const struct io_pin* pins, unsigned int size)
{
    ASSERT_NOT_NULL(pins);
    
    const struct io_pin* pin;
    for(unsigned int i = 0; i < size; ++i) {
        pin = &pins[i];
        switch(direction) {
            default:
            case IO_DIRECTION_DIN:
                if(pin->ansel != NULL)
                    atomic_reg_ptr_clr(pin->ansel, pin->mask);
                atomic_reg_ptr_set(pin->tris, pin->mask);
                break;
            case IO_DIRECTION_DOUT_LOW:
            case IO_DIRECTION_DOUT_HIGH:
                if(pin->ansel != NULL)
                    atomic_reg_ptr_clr(pin->ansel, pin->mask);
                atomic_reg_ptr_clr(pin->tris, pin->mask);
                if(direction == IO_DIRECTION_DOUT_LOW)
                    atomic_reg_ptr_clr(pin->lat, pin->mask);
                else
                    atomic_reg_ptr_set(pin->lat, pin->mask);
                break;
        }
    }
}

bool io_read(const struct io_pin* pin)
{
    ASSERT_NOT_NULL(pin);
    
    unsigned int port = atomic_reg_ptr_value(pin->port);
    return !!(port & pin->mask);
}

void io_set(const struct io_pin* pin, bool level)
{
    ASSERT_NOT_NULL(pin);
    
    if(level)
        atomic_reg_ptr_set(pin->lat, pin->mask);
    else
        atomic_reg_ptr_clr(pin->lat, pin->mask);
}

