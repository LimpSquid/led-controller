#include <util.h>
#include <assert_util.h>

void io_configure(enum io_direction direction, const struct io_pin* pins, unsigned int size)
{
    ASSERT_NOT_NULL(pins);
    
    const struct io_pin* pin = pins; // Just for the name
    for(unsigned int i = 0; i < size; ++i) {
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
        ++pin;
    }
}