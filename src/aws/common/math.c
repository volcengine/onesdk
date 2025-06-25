/**
* Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */

#include "math.h"
#include <stdarg.h>

AWS_EXTERN_C_BEGIN
/**
 * Multiplies a * b. If the result overflows, returns 2^64 - 1.
 */
AWS_STATIC_IMPL uint64_t aws_mul_u64_saturating(uint64_t a, uint64_t b) {
    if (a > 0 && b > 0 && a > (UINT64_MAX / b))
        return UINT64_MAX;
    return a * b;
}

/**
 * If a * b overflows, returns AWS_OP_ERR; otherwise multiplies
 * a * b, returns the result in *r, and returns AWS_OP_SUCCESS.
 */
AWS_STATIC_IMPL int aws_mul_u64_checked(uint64_t a, uint64_t b, uint64_t *r) {
    if (a > 0 && b > 0 && a > (UINT64_MAX / b))
        return aws_raise_error(AWS_ERROR_OVERFLOW_DETECTED);
    *r = a * b;
    return AWS_OP_SUCCESS;
}

/**
 * Multiplies a * b. If the result overflows, returns 2^32 - 1.
 */
AWS_STATIC_IMPL uint32_t aws_mul_u32_saturating(uint32_t a, uint32_t b) {
    if (a > 0 && b > 0 && a > (UINT32_MAX / b))
        return UINT32_MAX;
    return a * b;
}

/**
 * If a * b overflows, returns AWS_OP_ERR; otherwise multiplies
 * a * b, returns the result in *r, and returns AWS_OP_SUCCESS.
 */
AWS_STATIC_IMPL int aws_mul_u32_checked(uint32_t a, uint32_t b, uint32_t *r) {
    if (a > 0 && b > 0 && a > (UINT32_MAX / b))
        return aws_raise_error(AWS_ERROR_OVERFLOW_DETECTED);
    *r = a * b;
    return AWS_OP_SUCCESS;
}

/**
 * Adds a + b.  If the result overflows returns 2^64 - 1.
 */
AWS_STATIC_IMPL uint64_t aws_add_u64_saturating(uint64_t a, uint64_t b) {
    if ((b > 0) && (a > (UINT64_MAX - b)))
        return UINT64_MAX;
    return a + b;
}

/**
 * If a + b overflows, returns AWS_OP_ERR; otherwise adds
 * a + b, returns the result in *r, and returns AWS_OP_SUCCESS.
 */
AWS_STATIC_IMPL int aws_add_u64_checked(uint64_t a, uint64_t b, uint64_t *r) {
    if ((b > 0) && (a > (UINT64_MAX - b)))
        return aws_raise_error(AWS_ERROR_OVERFLOW_DETECTED);
    *r = a + b;
    return AWS_OP_SUCCESS;
}

/**
 * Adds a + b. If the result overflows returns 2^32 - 1.
 */
AWS_STATIC_IMPL uint32_t aws_add_u32_saturating(uint32_t a, uint32_t b) {
    if ((b > 0) && (a > (UINT32_MAX - b)))
        return UINT32_MAX;
    return a + b;
}

/**
 * If a + b overflows, returns AWS_OP_ERR; otherwise adds
 * a + b, returns the result in *r, and returns AWS_OP_SUCCESS.
 */
AWS_STATIC_IMPL int aws_add_u32_checked(uint32_t a, uint32_t b, uint32_t *r) {
    if ((b > 0) && (a > (UINT32_MAX - b)))
        return aws_raise_error(AWS_ERROR_OVERFLOW_DETECTED);
    *r = a + b;
    return AWS_OP_SUCCESS;
}

/*
 * These are pure C implementations of the count leading/trailing zeros calls
 * They should not be necessary unless using a really esoteric compiler with
 * no intrinsics for these functions whatsoever.
 */
#if !defined(__clang__) && !defined(__GNUC__)
/**
 * Search from the MSB to LSB, looking for a 1
 */
AWS_STATIC_IMPL size_t aws_clz_u32(uint32_t n) {
    return aws_clz_i32((int32_t)n);
}

AWS_STATIC_IMPL size_t aws_clz_i32(int32_t n) {
    size_t idx = 0;
    if (n == 0) {
        return sizeof(n) * 8;
    }
    /* sign bit is the first bit */
    if (n < 0) {
        return 0;
    }
    while (n >= 0) {
        ++idx;
        n <<= 1;
    }
    return idx;
}

AWS_STATIC_IMPL size_t aws_clz_u64(uint64_t n) {
    return aws_clz_i64((int64_t)n);
}

AWS_STATIC_IMPL size_t aws_clz_i64(int64_t n) {
    size_t idx = 0;
    if (n == 0) {
        return sizeof(n) * 8;
    }
    /* sign bit is the first bit */
    if (n < 0) {
        return 0;
    }
    while (n >= 0) {
        ++idx;
        n <<= 1;
    }
    return idx;
}

AWS_STATIC_IMPL size_t aws_clz_size(size_t n) {
#    if SIZE_BITS == 64
    return aws_clz_u64(n);
#    else
    return aws_clz_u32(n);
#    endif
}

/**
 * Search from the LSB to MSB, looking for a 1
 */
AWS_STATIC_IMPL size_t aws_ctz_u32(uint32_t n) {
    return aws_ctz_i32((int32_t)n);
}

AWS_STATIC_IMPL size_t aws_ctz_i32(int32_t n) {
    int32_t idx = 0;
    const int32_t max_bits = (int32_t)(SIZE_BITS / sizeof(uint8_t));
    if (n == 0) {
        return sizeof(n) * 8;
    }
    while (idx < max_bits) {
        if (n & (1 << idx)) {
            break;
        }
        ++idx;
    }
    return (size_t)idx;
}

AWS_STATIC_IMPL size_t aws_ctz_u64(uint64_t n) {
    return aws_ctz_i64((int64_t)n);
}

AWS_STATIC_IMPL size_t aws_ctz_i64(int64_t n) {
    int64_t idx = 0;
    const int64_t max_bits = (int64_t)(SIZE_BITS / sizeof(uint8_t));
    if (n == 0) {
        return sizeof(n) * 8;
    }
    while (idx < max_bits) {
        if (n & (1ULL << idx)) {
            break;
        }
        ++idx;
    }
    return (size_t)idx;
}

AWS_STATIC_IMPL size_t aws_ctz_size(size_t n) {
#    if SIZE_BITS == 64
    return aws_ctz_u64(n);
#    else
    return aws_ctz_u32(n);
#    endif
}

#endif

AWS_EXTERN_C_END


AWS_COMMON_API int aws_add_size_checked_varargs(size_t num, size_t *r, ...) {
    va_list argp;
    va_start(argp, r);

    size_t accum = 0;
    for (size_t i = 0; i < num; ++i) {
        size_t next = va_arg(argp, size_t);
        if (aws_add_size_checked(accum, next, &accum) == AWS_OP_ERR) {
            va_end(argp);
            return AWS_OP_ERR;
        }
    }
    *r = accum;
    va_end(argp);
    return AWS_OP_SUCCESS;
}

AWS_STATIC_IMPL uint64_t aws_sub_u64_saturating(uint64_t a, uint64_t b) {
    return a <= b ? 0 : a - b;
}

AWS_STATIC_IMPL int aws_sub_u64_checked(uint64_t a, uint64_t b, uint64_t *r) {
    if (a < b) {
        return aws_raise_error(AWS_ERROR_OVERFLOW_DETECTED);
    }

    *r = a - b;
    return AWS_OP_SUCCESS;
}

AWS_STATIC_IMPL uint32_t aws_sub_u32_saturating(uint32_t a, uint32_t b) {
    return a <= b ? 0 : a - b;
}

AWS_STATIC_IMPL int aws_sub_u32_checked(uint32_t a, uint32_t b, uint32_t *r) {
    if (a < b) {
        return aws_raise_error(AWS_ERROR_OVERFLOW_DETECTED);
    }

    *r = a - b;
    return AWS_OP_SUCCESS;
}

/**
 * Multiplies a * b. If the result overflows, returns SIZE_MAX.
 */
AWS_STATIC_IMPL size_t aws_mul_size_saturating(size_t a, size_t b) {
#if SIZE_BITS == 32
    return (size_t)aws_mul_u32_saturating(a, b);
#elif SIZE_BITS == 64
    return (size_t)aws_mul_u64_saturating(a, b);
#else
#    error "Target not supported"
#endif
}

/**
 * Multiplies a * b and returns the result in *r. If the result
 * overflows, returns AWS_OP_ERR; otherwise returns AWS_OP_SUCCESS.
 */
AWS_STATIC_IMPL int aws_mul_size_checked(size_t a, size_t b, size_t *r) {
#if SIZE_BITS == 32
    return aws_mul_u32_checked(a, b, (uint32_t *)r);
#elif SIZE_BITS == 64
    return aws_mul_u64_checked(a, b, (uint64_t *)r);
#else
#    error "Target not supported"
#endif
}

/**
 * Adds a + b.  If the result overflows returns SIZE_MAX.
 */
AWS_STATIC_IMPL size_t aws_add_size_saturating(size_t a, size_t b) {
#if SIZE_BITS == 32
    return (size_t)aws_add_u32_saturating(a, b);
#elif SIZE_BITS == 64
    return (size_t)aws_add_u64_saturating(a, b);
#else
#    error "Target not supported"
#endif
}

/**
 * Adds a + b and returns the result in *r. If the result
 * overflows, returns AWS_OP_ERR; otherwise returns AWS_OP_SUCCESS.
 */
AWS_STATIC_IMPL int aws_add_size_checked(size_t a, size_t b, size_t *r) {
#if SIZE_BITS == 32
    return aws_add_u32_checked(a, b, (uint32_t *)r);
#elif SIZE_BITS == 64
    return aws_add_u64_checked(a, b, (uint64_t *)r);
#else
#    error "Target not supported"
#endif
}

AWS_STATIC_IMPL size_t aws_sub_size_saturating(size_t a, size_t b) {
#if SIZE_BITS == 32
    return (size_t)aws_sub_u32_saturating(a, b);
#elif SIZE_BITS == 64
    return (size_t)aws_sub_u64_saturating(a, b);
#else
#    error "Target not supported"
#endif
}

AWS_STATIC_IMPL int aws_sub_size_checked(size_t a, size_t b, size_t *r) {
#if SIZE_BITS == 32
    return aws_sub_u32_checked(a, b, (uint32_t *)r);
#elif SIZE_BITS == 64
    return aws_sub_u64_checked(a, b, (uint64_t *)r);
#else
#    error "Target not supported"
#endif
}

/**
 * Function to check if x is power of 2
 */
AWS_STATIC_IMPL bool aws_is_power_of_two(const size_t x) {
    /* First x in the below expression is for the case when x is 0 */
    return x && (!(x & (x - 1)));
}

/**
 * Function to find the smallest result that is power of 2 >= n. Returns AWS_OP_ERR if this cannot
 * be done without overflow
 */
AWS_STATIC_IMPL int aws_round_up_to_power_of_two(size_t n, size_t *result) {
    if (n == 0) {
        *result = 1;
        return AWS_OP_SUCCESS;
    }
    if (n > SIZE_MAX_POWER_OF_TWO) {
        return aws_raise_error(AWS_ERROR_OVERFLOW_DETECTED);
    }

    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
#if SIZE_BITS == 64
    n |= n >> 32;
#endif
    n++;
    *result = n;
    return AWS_OP_SUCCESS;
}


AWS_STATIC_IMPL uint8_t aws_min_u8(uint8_t a, uint8_t b) {
    return a < b ? a : b;
}

AWS_STATIC_IMPL uint8_t aws_max_u8(uint8_t a, uint8_t b) {
    return a > b ? a : b;
}

AWS_STATIC_IMPL int8_t aws_min_i8(int8_t a, int8_t b) {
    return a < b ? a : b;
}

AWS_STATIC_IMPL int8_t aws_max_i8(int8_t a, int8_t b) {
    return a > b ? a : b;
}

AWS_STATIC_IMPL uint16_t aws_min_u16(uint16_t a, uint16_t b) {
    return a < b ? a : b;
}

AWS_STATIC_IMPL uint16_t aws_max_u16(uint16_t a, uint16_t b) {
    return a > b ? a : b;
}

AWS_STATIC_IMPL int16_t aws_min_i16(int16_t a, int16_t b) {
    return a < b ? a : b;
}

AWS_STATIC_IMPL int16_t aws_max_i16(int16_t a, int16_t b) {
    return a > b ? a : b;
}

AWS_STATIC_IMPL uint32_t aws_min_u32(uint32_t a, uint32_t b) {
    return a < b ? a : b;
}

AWS_STATIC_IMPL uint32_t aws_max_u32(uint32_t a, uint32_t b) {
    return a > b ? a : b;
}

AWS_STATIC_IMPL int32_t aws_min_i32(int32_t a, int32_t b) {
    return a < b ? a : b;
}

AWS_STATIC_IMPL int32_t aws_max_i32(int32_t a, int32_t b) {
    return a > b ? a : b;
}

AWS_STATIC_IMPL uint64_t aws_min_u64(uint64_t a, uint64_t b) {
    return a < b ? a : b;
}

AWS_STATIC_IMPL uint64_t aws_max_u64(uint64_t a, uint64_t b) {
    return a > b ? a : b;
}

AWS_STATIC_IMPL int64_t aws_min_i64(int64_t a, int64_t b) {
    return a < b ? a : b;
}

AWS_STATIC_IMPL int64_t aws_max_i64(int64_t a, int64_t b) {
    return a > b ? a : b;
}

AWS_STATIC_IMPL size_t aws_min_size(size_t a, size_t b) {
    return a < b ? a : b;
}

AWS_STATIC_IMPL size_t aws_max_size(size_t a, size_t b) {
    return a > b ? a : b;
}

AWS_STATIC_IMPL int aws_min_int(int a, int b) {
    return a < b ? a : b;
}

AWS_STATIC_IMPL int aws_max_int(int a, int b) {
    return a > b ? a : b;
}

AWS_STATIC_IMPL float aws_min_float(float a, float b) {
    return a < b ? a : b;
}

AWS_STATIC_IMPL float aws_max_float(float a, float b) {
    return a > b ? a : b;
}

AWS_STATIC_IMPL double aws_min_double(double a, double b) {
    return a < b ? a : b;
}

AWS_STATIC_IMPL double aws_max_double(double a, double b) {
    return a > b ? a : b;
}