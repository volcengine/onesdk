/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */
#include "byte_buf.h"
#include "string.h"
#include "common.h"

#include <ctype.h>
#include <stddef.h>

AWS_EXTERN_C_BEGIN
/**
 * Equivalent to str->bytes.
 */
AWS_STATIC_IMPL
const uint8_t *aws_string_bytes(const struct aws_string *str) {
    AWS_PRECONDITION(aws_string_is_valid(str));
    return str->bytes;
}

/**
 * Equivalent to `(const char *)str->bytes`.
 */
AWS_STATIC_IMPL
const char *aws_string_c_str(const struct aws_string *str) {
    AWS_PRECONDITION(aws_string_is_valid(str));
    return (const char *)str->bytes;
}

/**
 * Evaluates the set of properties that define the shape of all valid aws_string structures.
 * It is also a cheap check, in the sense it run in constant time (i.e., no loops or recursion).
 */
AWS_STATIC_IMPL
bool aws_string_is_valid(const struct aws_string *str) {
    return str && AWS_MEM_IS_READABLE(&str->bytes[0], str->len + 1) && str->bytes[str->len] == 0;
}

/**
 * Best-effort checks aws_string invariants, when the str->len is unknown
 */
AWS_STATIC_IMPL
bool aws_c_string_is_valid(const char *str) {
    /* Knowing the actual length to check would require strlen(), which is
     * a) linear time in the length of the string
     * b) could already cause a memory violation for a non-zero-terminated string.
     * But we know that a c-string must have at least one character, to store the null terminator
     */
    return str && AWS_MEM_IS_READABLE(str, 1);
}

/**
 * Evaluates if a char is a white character.
 */
AWS_STATIC_IMPL
bool aws_char_is_space(uint8_t c) {
    return aws_isspace(c);
}

struct aws_string *aws_string_new_from_c_str(struct aws_allocator *allocator, const char *c_str) {
    AWS_PRECONDITION(allocator && c_str);
    return aws_string_new_from_array(allocator, (const uint8_t *)c_str, strlen(c_str));
}

struct aws_string *aws_string_new_from_array(struct aws_allocator *allocator, const uint8_t *bytes, size_t len) {
    AWS_PRECONDITION(allocator);
    AWS_PRECONDITION(AWS_MEM_IS_READABLE(bytes, len));

    // struct aws_string *str = (struct aws_string *)aws_mem_acquire(allocator, offsetof(struct aws_string, bytes[len + 1]));
    struct aws_string *str = aws_mem_acquire(allocator, sizeof(struct aws_string)+1+len);
    if (!str) {
        return NULL;
    }

    /* Fields are declared const, so we need to copy them in like this */
    *(struct aws_allocator **)(&str->allocator) = allocator;
    *(size_t *)(&str->len) = len;
    if (len > 0) {
        memcpy((void *)str->bytes, bytes, len);
    }
    *(uint8_t *)&str->bytes[len] = 0;
    AWS_RETURN_WITH_POSTCONDITION(str, aws_string_is_valid(str));
}

struct aws_string *aws_string_new_from_string(struct aws_allocator *allocator, const struct aws_string *str) {
    AWS_PRECONDITION(allocator && aws_string_is_valid(str));
    return aws_string_new_from_array(allocator, str->bytes, str->len);
}

struct aws_string *aws_string_new_from_cursor(struct aws_allocator *allocator, const struct aws_byte_cursor *cursor) {
    AWS_PRECONDITION(allocator && aws_byte_cursor_is_valid(cursor));
    return aws_string_new_from_array(allocator, cursor->ptr, cursor->len);
}

struct aws_string *aws_string_new_from_buf(struct aws_allocator *allocator, const struct aws_byte_buf *buf) {
    AWS_PRECONDITION(allocator && aws_byte_buf_is_valid(buf));
    return aws_string_new_from_array(allocator, buf->buffer, buf->len);
}

void aws_string_destroy(struct aws_string *str) {
    AWS_PRECONDITION(!str || aws_string_is_valid(str));
    if (str && str->allocator) {
        aws_mem_release(str->allocator, str);
    }
}

void aws_string_destroy_secure(struct aws_string *str) {
    AWS_PRECONDITION(!str || aws_string_is_valid(str));
    if (str) {
        aws_secure_zero((void *)aws_string_bytes(str), str->len);
        if (str->allocator) {
            aws_mem_release(str->allocator, str);
        }
    }
}

int aws_string_compare(const struct aws_string *a, const struct aws_string *b) {
    AWS_PRECONDITION(!a || aws_string_is_valid(a));
    AWS_PRECONDITION(!b || aws_string_is_valid(b));
    if (a == b) {
        return 0; /* strings identical */
    }
    if (a == NULL) {
        return -1;
    }
    if (b == NULL) {
        return 1;
    }

    size_t len_a = a->len;
    size_t len_b = b->len;
    size_t min_len = len_a < len_b ? len_a : len_b;

    int ret = memcmp(aws_string_bytes(a), aws_string_bytes(b), min_len);
    AWS_POSTCONDITION(aws_string_is_valid(a));
    AWS_POSTCONDITION(aws_string_is_valid(b));
    if (ret) {
        return ret; /* overlapping characters differ */
    }
    if (len_a == len_b) {
        return 0; /* strings identical */
    }
    if (len_a > len_b) {
        return 1; /* string b is first n characters of string a */
    }
    return -1; /* string a is first n characters of string b */
}

int aws_array_list_comparator_string(const void *a, const void *b) {
    if (a == b) {
        return 0; /* strings identical */
    }
    if (a == NULL) {
        return -1;
    }
    if (b == NULL) {
        return 1;
    }
    const struct aws_string *str_a = *(const struct aws_string **)a;
    const struct aws_string *str_b = *(const struct aws_string **)b;
    return aws_string_compare(str_a, str_b);
}

/**
 * Returns true if bytes of string are the same, false otherwise.
 */
bool aws_string_eq(const struct aws_string *a, const struct aws_string *b) {
    AWS_PRECONDITION(!a || aws_string_is_valid(a));
    AWS_PRECONDITION(!b || aws_string_is_valid(b));
    if (a == b) {
        return true;
    }
    if (a == NULL || b == NULL) {
        return false;
    }
    return aws_array_eq(a->bytes, a->len, b->bytes, b->len);
}

/**
 * Returns true if bytes of string are equivalent, using a case-insensitive comparison.
 */
bool aws_string_eq_ignore_case(const struct aws_string *a, const struct aws_string *b) {
    AWS_PRECONDITION(!a || aws_string_is_valid(a));
    AWS_PRECONDITION(!b || aws_string_is_valid(b));
    if (a == b) {
        return true;
    }
    if (a == NULL || b == NULL) {
        return false;
    }
    return aws_array_eq_ignore_case(a->bytes, a->len, b->bytes, b->len);
}

/**
 * Returns true if bytes of string and cursor are the same, false otherwise.
 */
bool aws_string_eq_byte_cursor(const struct aws_string *str, const struct aws_byte_cursor *cur) {
    AWS_PRECONDITION(!str || aws_string_is_valid(str));
    AWS_PRECONDITION(!cur || aws_byte_cursor_is_valid(cur));
    if (str == NULL && cur == NULL) {
        return true;
    }
    if (str == NULL || cur == NULL) {
        return false;
    }
    return aws_array_eq(str->bytes, str->len, cur->ptr, cur->len);
}

/**
 * Returns true if bytes of string and cursor are equivalent, using a case-insensitive comparison.
 */

bool aws_string_eq_byte_cursor_ignore_case(const struct aws_string *str, const struct aws_byte_cursor *cur) {
    AWS_PRECONDITION(!str || aws_string_is_valid(str));
    AWS_PRECONDITION(!cur || aws_byte_cursor_is_valid(cur));
    if (str == NULL && cur == NULL) {
        return true;
    }
    if (str == NULL || cur == NULL) {
        return false;
    }
    return aws_array_eq_ignore_case(str->bytes, str->len, cur->ptr, cur->len);
}

/**
 * Returns true if bytes of string and buffer are the same, false otherwise.
 */
bool aws_string_eq_byte_buf(const struct aws_string *str, const struct aws_byte_buf *buf) {
    AWS_PRECONDITION(!str || aws_string_is_valid(str));
    AWS_PRECONDITION(!buf || aws_byte_buf_is_valid(buf));
    if (str == NULL && buf == NULL) {
        return true;
    }
    if (str == NULL || buf == NULL) {
        return false;
    }
    return aws_array_eq(str->bytes, str->len, buf->buffer, buf->len);
}

/**
 * Returns true if bytes of string and buffer are equivalent, using a case-insensitive comparison.
 */

bool aws_string_eq_byte_buf_ignore_case(const struct aws_string *str, const struct aws_byte_buf *buf) {
    AWS_PRECONDITION(!str || aws_string_is_valid(str));
    AWS_PRECONDITION(!buf || aws_byte_buf_is_valid(buf));
    if (str == NULL && buf == NULL) {
        return true;
    }
    if (str == NULL || buf == NULL) {
        return false;
    }
    return aws_array_eq_ignore_case(str->bytes, str->len, buf->buffer, buf->len);
}

bool aws_string_eq_c_str(const struct aws_string *str, const char *c_str) {
    AWS_PRECONDITION(!str || aws_string_is_valid(str));
    if (str == NULL && c_str == NULL) {
        return true;
    }
    if (str == NULL || c_str == NULL) {
        return false;
    }
    return aws_array_eq_c_str(str->bytes, str->len, c_str);
}

/**
 * Returns true if bytes of strings are equivalent, using a case-insensitive comparison.
 */
bool aws_string_eq_c_str_ignore_case(const struct aws_string *str, const char *c_str) {
    AWS_PRECONDITION(!str || aws_string_is_valid(str));
    if (str == NULL && c_str == NULL) {
        return true;
    }
    if (str == NULL || c_str == NULL) {
        return false;
    }
    return aws_array_eq_c_str_ignore_case(str->bytes, str->len, c_str);
}

bool aws_byte_buf_write_from_whole_string(
    struct aws_byte_buf *AWS_RESTRICT buf,
    const struct aws_string *AWS_RESTRICT src) {
    AWS_PRECONDITION(!buf || aws_byte_buf_is_valid(buf));
    AWS_PRECONDITION(!src || aws_string_is_valid(src));
    if (buf == NULL || src == NULL) {
        return false;
    }
    return aws_byte_buf_write(buf, aws_string_bytes(src), src->len);
}

/**
 * Creates an aws_byte_cursor from an existing string.
 */
struct aws_byte_cursor aws_byte_cursor_from_string(const struct aws_string *src) {
    AWS_PRECONDITION(aws_string_is_valid(src));
    return aws_byte_cursor_from_array(aws_string_bytes(src), src->len);
}

struct aws_string *aws_string_clone_or_reuse(struct aws_allocator *allocator, const struct aws_string *str) {
    AWS_PRECONDITION(allocator);
    AWS_PRECONDITION(aws_string_is_valid(str));

    if (str->allocator == NULL) {
        /* Since the string cannot be deallocated, we assume that it will remain valid for the lifetime of the
         * application */
        AWS_POSTCONDITION(aws_string_is_valid(str));
        return (struct aws_string *)str;
    }

    AWS_POSTCONDITION(aws_string_is_valid(str));
    return aws_string_new_from_string(allocator, str);
}

int aws_secure_strlen(const char *str, size_t max_read_len, size_t *str_len) {
    AWS_ERROR_PRECONDITION(str && str_len, AWS_ERROR_INVALID_ARGUMENT);

    /* why not strnlen? It doesn't work everywhere as it wasn't standardized til C11, and is considered
     * a GNU extension. This should be faster anyways. This should work for ascii and utf8.
     * Any other character sets in use deserve what they get. */
    char *null_char_ptr = memchr(str, '\0', max_read_len);

    if (null_char_ptr) {
        *str_len = null_char_ptr - str;
        return AWS_OP_SUCCESS;
    }

    return aws_raise_error(AWS_ERROR_C_STRING_BUFFER_NOT_NULL_TERMINATED);
}

AWS_EXTERN_C_END