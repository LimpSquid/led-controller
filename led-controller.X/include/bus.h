#ifndef BUS_H
#define	BUS_H

#include <stdint.h>

enum bus_response_code
{
    BUS_OK = 0,                         // Request OK and handled, must be 0
    
    BUS_ERROR_MIN = 100,
    BUS_ERROR_UNKNOWN = BUS_ERROR_MIN,  // Unknown error
    BUS_ERROR_INVALID_PAYLOAD,          // Request payload invalid
    BUS_ERROR_INVALID_COMMAND,          // Request command does not exist
    BUS_ERROR_MAX,
};

union __attribute__((packed)) bus_data
{
    struct
    {
        unsigned char b1;
        unsigned char b2;
        unsigned char b3;
        unsigned char b4;
    } by_bytes;
    uint32_t by_uint32;
    int32_t by_int32;
};

typedef enum bus_response_code (*bus_func_t)(union bus_data*);

#endif	/* BUS_H */

