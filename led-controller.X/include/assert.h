#ifndef ASSERT_H
#define ASSERT_H

// @Todo: maybe rename this file someday as it can conflict with assert.h from the c standard libs

#include <xc.h>
#include <stdarg.h>

#undef ASSERT
#undef INT_ASSERT
#undef SOFT_ASSERT
#undef STATIC_ASSERT

#undef __assert_failed
#undef __assert_int_failed
#undef __assert_soft_failed

#ifdef __DEBUG
#define ASSERT(expression)          do{ if(!(expression)){ __assert_failed(expression);     }} while(0)
#define INT_ASSERT(expression)      do{ if(!(expression)){ __assert_int_failed(expression); }} while(0)
#define SOFT_ASSERT(expression)     do{ if(!(expression)){ __assert_soft_failed(expression);}} while(0)

// Assertion formats
#define __assert_failed(expression)         __assert_print("Assertion failed: %s at line %d in %s of %s", #expression, __LINE__, __FUNCTION__, __FILE__)
#define __assert_int_failed(expression)     __assert_print("Assertion failed: %s at line %d in %s of %s", #expression, __LINE__, __FUNCTION__, __FILE__)
#define __assert_soft_failed(expression)    __assert_print_no_block("Assertion failed: %s at line %d in %s of %s. Continuing...", #expression, __LINE__, __FUNCTION__, __FILE__)

void __assert_print(const char* format, ...);
void __assert_print_no_block(const char* format, ...);

#else
#define ASSERT(expression)          ((void)0)
#define INT_ASSERT(expression)      ((void)0)
#define SOFT_ASSERT(expression)     ((void)0)
#endif

#if (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) >= 40600
// GCC >= 4.6 implements _Static_assert(expr, msg)
#define STATIC_ASSERT(expression)   _Static_assert(expression,  "Assertion failed: " #expression);
#else
#define STATIC_ASSERT(expression)   typedef char __assert[(expression) ? 0 : -1];
#endif

#endif    /* ASSERT_H */