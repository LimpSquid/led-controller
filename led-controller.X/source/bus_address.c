#include <bus_address.h>
#include <assert_util.h>
#include <timer.h>
#include <util.h>

#define BUS_ADDRESS_BITS            5
#define BUS_ADDRESS_SAMPLE_TIME     250 // In milliseconds
#define BUS_ADDRESS_SAMPLE_COUNT    2 // How many reads must yield the same result after an address change is detected before the new address is latched into actual address
#define BUS_ADDRESS_INVALID         255

static const struct io_pin bus_address_pins[BUS_ADDRESS_BITS] =
{
    IO_ANSEL_PIN(2, B), // Bit 0
    IO_PIN(5, F), // Bit 1
    IO_ANSEL_PIN(12, B), // ...
    IO_ANSEL_PIN(13, B),
    IO_PIN(4, F)
};

static struct timer_module* bus_address_sample_timer = NULL;
static unsigned char bus_address_sample_count = 0;
static unsigned char bus_address_sampling = BUS_ADDRESS_INVALID;
static unsigned char bus_address_actual = BUS_ADDRESS_INVALID;

static unsigned char bus_address_read(void)
{
    unsigned char address = 0;
    for(unsigned int i = 0; i < BUS_ADDRESS_BITS; ++i)
        address |= MASK_SHIFT(io_read(&bus_address_pins[i]), i);
    return address;
}

static void bus_address_sample(struct timer_module* module)
{
    unsigned char address = bus_address_read();
    if(address != bus_address_sampling) {
        bus_address_sample_count = 0;
        bus_address_sampling = address;
    } 
    
    if(bus_address_sample_count < BUS_ADDRESS_SAMPLE_COUNT)
        bus_address_sample_count++;
    else
        bus_address_actual = address; 
}

void bus_address_init(void)
{    
    // Configure IO
    io_configure(IO_DIRECTION_DIN, bus_address_pins, BUS_ADDRESS_BITS);
    
    // Initialize timer
    bus_address_sample_timer = timer_construct(TIMER_TYPE_SOFT, bus_address_sample);
    ASSERT_NOT_NULL(bus_address_sample_timer);
    if(bus_address_sample_timer == NULL)
        return;
    
    timer_start(bus_address_sample_timer, BUS_ADDRESS_SAMPLE_TIME, TIMER_TIME_UNIT_MS);
}

bool bus_address_valid(void)
{
    return bus_address_actual >= 0 && bus_address_actual < 32;
}

unsigned char bus_address_get(void)
{
    return bus_address_actual;
}