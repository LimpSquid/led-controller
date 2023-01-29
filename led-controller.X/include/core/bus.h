#ifndef BUS_H
#define BUS_H

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
    // Generic types
    uint8_t     by_uint8;
    uint16_t    by_uint16;
    uint32_t    by_uint32;
    int8_t      by_int8;
    int16_t     by_int16;
    int32_t     by_int32;
    bool        by_bool;

    // Custom types
    struct
    {
        bool layer_ready            :1;
        bool layer_dma_error        :1;
        bool layer_auto_buffer_swap :1;
        unsigned int                :29;
    } by_app_status;
    
    struct
    {
        bool bootloader_ready               :1;
        bool bootloader_error               :1;
        bool bootloader_waiting_for_magic   :1;
        unsigned int                        :29;
    } by_bootloader_status;

    struct
    {
        unsigned char major;
        unsigned char minor;
        unsigned char patch;
        unsigned char               :8;
    } by_version;
};

typedef enum bus_response_code (*bus_func_t)(
    bool broadcast, 
    union bus_data const * request_data,
    union bus_data * response_data);

bool bus_idle(void);

#endif /* BUS_H */

