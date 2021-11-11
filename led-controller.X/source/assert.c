#include "../include/assert.h" // make sure we do not conflict with <assert.h>
#include <print.h>
#include <string.h>

#define ASSERT_BUFFER_SIZE  512

static char assert_print_buffer[ASSERT_BUFFER_SIZE];

static unsigned char assert_halt()
{
    for(;;) {
        Nop();
        Nop();
        Nop();
    }
}

void __attribute__((weak)) assert_printer(const char* format, va_list arg)
{
    // can be overridden if desired, by default dumps assertion in buffer
    memset(assert_print_buffer, 0, ASSERT_BUFFER_SIZE);
    print_vfs(assert_print_buffer, format, arg);
}

void __assert_print(const char* format, ...)
{
    va_list arg;
    va_start(arg, format);
    assert_printer(format, arg);
    va_end(arg);
    assert_halt();
}

void __assert_print_no_block(const char* format, ...)
{
    va_list arg;
    va_start(arg, format);
    assert_printer(format, arg);
    va_end(arg);
}