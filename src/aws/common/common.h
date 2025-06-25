#ifndef VOLC_COMMON_H
#define VOLC_COMMON_H

#include "assert.h"
#include "stdint.h"
#include "allocator.h"
#include "error.h"
#include "string.h"
#include "byte_buf.h"

#include <stdint.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h> /* for abort() */
#include <string.h>
#include "stdbool.h"
#include <stdbool.h>

AWS_PUSH_SANE_WARNING_LEVEL
AWS_EXTERN_C_BEGIN

/**
 * Set each byte in the struct to zero.
 */
#define AWS_ZERO_STRUCT(object)                                                                                        \
do {                                                                                                               \
memset(&(object), 0, sizeof(object));                                                                          \
} while (0)

/**
 * Set each byte in the array to zero.
 * Does not work with arrays of unknown bound.
 */
#define AWS_ZERO_ARRAY(array) memset((void *)(array), 0, sizeof(array))

/**
 * Returns whether each byte in the object is zero.
 */
#ifdef CBMC
/* clang-format off */
#    define AWS_IS_ZEROED(object)                                                                                      \
__CPROVER_forall {                                                                                             \
int i;                                                                                                     \
(i >= 0 && i < sizeof(object)) ==> ((const uint8_t *)&object)[i] == 0                                      \
}
/* clang-format on */
#else
#    define AWS_IS_ZEROED(object) aws_is_mem_zeroed(&(object), sizeof(object))
#endif

/**
 * Returns whether each byte is zero.
 */
AWS_STATIC_IMPL
bool aws_is_mem_zeroed(const void *buf, size_t bufsize);

/**
 * Securely zeroes a memory buffer. This function will attempt to ensure that
 * the compiler will not optimize away this zeroing operation.
 */
AWS_COMMON_API
void aws_secure_zero(void *pBuf, size_t bufsize);

AWS_EXTERN_C_END

AWS_POP_SANE_WARNING_LEVEL

AWS_COMMON_API
void aws_common_library_init(struct aws_allocator *allocator);

/**
 * Shuts down the internal data structures used by aws-c-common.
 */
AWS_COMMON_API
void aws_common_library_clean_up(void);

struct aws_allocator *aws_hal_allocator(void);

#ifdef AWS_OS_WINDOWS
#    define AWS_PATH_DELIM '\\'
#    define AWS_PATH_DELIM_STR "\\"
#else
#    define AWS_PATH_DELIM '/'
#    define AWS_PATH_DELIM_STR "/"
#endif

#endif  // VOLC_COMMON_H