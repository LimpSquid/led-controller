#ifndef ASSERT_UTIL_H
#define ASSERT_UTIL_H

#include <assert.h>
#include <stddef.h>

#define ASSERT_NULL(var)        ASSERT(var == NULL)
#define ASSERT_NOT_NULL(var)    ASSERT(var != NULL)

#endif /* ASSERT_H */
