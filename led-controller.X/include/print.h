#ifndef PRINT_H
#define PRINT_H

#include <stdarg.h>

int print_fs(char * str, char const * format, ...);
int print_vfs(char * str, char const * format, va_list arg);

#endif /* PRINT_H */