#ifndef RS485_H
#define RS485_H

#include <stdbool.h>

struct rs485_error
{
    unsigned char perr  :1;
    unsigned char ferr  :1;
    unsigned char oerr  :1;
    unsigned char       :5;
};

struct rs485_error_notifier
{
    void (*callback)(struct rs485_error error);
    struct rs485_error_notifier const * next;
};

bool rs485_idle(void);
struct rs485_error rs485_get_error(void);
void rs485_register_error_notifier(struct rs485_error_notifier * const notifier);
void rs485_reset(void);
void rs485_transmit(unsigned char data);
void rs485_transmit_buffer(unsigned char * buffer, unsigned int size);
bool rs485_bytes_available(void);
unsigned char rs485_read(void);
unsigned int rs485_read_buffer(unsigned char * buffer, unsigned int max_size);

#endif /* RS485 */