#ifndef BUS_ADDRESS_H
#define BUS_ADDRESS_H

#include <stdbool.h>

void bus_address_init(void);
bool bus_address_valid(void);
unsigned char bus_address_get(void);

#endif /* BUS_ADDRESS_H */

