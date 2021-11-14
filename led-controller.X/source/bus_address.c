#include <bus_address.h>
#include <assert_util.h>
#include <timer.h>

#define BUS_ADDRESS_SAMPLE_TIME		100 // In milliseconds
#define BUS_ADDRESS_SAMPLE_COUNT	2 // How many reads must yield the same result after an address change is detected before the new address is latched into actual address
#define BUS_ADDRESS_INVALID			255

static struct timer_module* bus_address_sample_timer = NULL;
static unsigned char bus_address_sample_count = 0;
static unsigned char bus_address_sampling = BUS_ADDRESS_INVALID;
static unsigned char bus_address_actual = BUS_ADDRESS_INVALID;

static unsigned char bus_address_read()
{
    return 0;
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