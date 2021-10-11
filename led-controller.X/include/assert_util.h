#ifndef ASSERT_UTIL_H
#define	ASSERT_UTIL_H

//#include <assert.h>
#include "assert.h" // make sure we do not conflict with anyone
#include <stddef.h>

#define ASSERT_NULL(var)        (NULL == var)
#define ASSERT_NOT_NULL(var)    (NULL != var)


#endif	/* ASSERT_H */