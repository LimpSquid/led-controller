#ifndef BUS_H
#define    BUS_H

#include <stdint.h>
#include <stdbool.h>

enum bus_response_code
{
    BUS_OK = 0,                         // Request OK and handled, must be 0
    
    BUS_ERR_START = 100,
    BUS_ERR_UNKNOWN = BUS_ERR_START,    // Unknown error
    BUS_ERR_AGAIN,                      // Request couldn't be completed at this time
    BUS_ERR_INVALID_PAYLOAD,            // Request payload invalid
    BUS_ERR_INVALID_COMMAND,            // Request command does not exist
};

union __attribute__((packed)) bus_data
{
    struct
    {
        unsigned char b1;
        unsigned char b2;
        unsigned char b3;
        unsigned char b4;
    }           by_bytes;
    uint8_t     by_uint8;
    uint16_t    by_uint16;
    uint32_t    by_uint32;
    int8_t      by_int8;
    int16_t     by_int16;
    int32_t     by_int32;
    bool        by_bool;
};

// @Todo: eventually have separate broadcast functions
typedef enum bus_response_code (*bus_func_t)(
    bool broadcast, 
    const union bus_data* request_data, 
    union bus_data* response_data);

#endif    /* BUS_H */

