#ifndef ASSERT_H
#define ASSERT_H

#include <xc.h>
#include <stdarg.h>

#ifdef __DEBUG
#define ASSERT(expression)          do { if (!(expression)) { __assert_failed(expression);          }} while (0)
#define ASSERT_NO_BLOCK(expression) do { if (!(expression)) { __assert_no_block_failed(expression); }} while (0)

// Assertion formats
#define __assert_failed(expression)             __assert_print("Assertion failed: %s at line %d in %s of %s", #expression, __LINE__, __FUNCTION__, __FILE__)
#define __assert_no_block_failed(expression)    __assert_print_no_block("Assertion failed: %s at line %d in %s of %s. Continuing...", #expression, __LINE__, __FUNCTION__, __FILE__)

void __assert_print(const char* format, ...);
void __assert_print_no_block(const char* format, ...);

#else
#define ASSERT(expression)          ((void)0)
#define ASSERT_NO_BLOCK(expression) ((void)0)
#endif

#if (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) >= 40600
// GCC >= 4.6 implements _Static_assert(expr, msg)
#define STATIC_ASSERT(expression)   _Static_assert(expression,  "Assertion failed: " #expression);
#else
#define STATIC_ASSERT(expression)   typedef char __assert[(expression) ? 0 : -1];
#endif

// Assert helper macros
#define ASSERT_NULL(var)        ASSERT(var == NULL)
#define ASSERT_NOT_NULL(var)    ASSERT(var != NULL)

#endif    /* ASSERT_H */