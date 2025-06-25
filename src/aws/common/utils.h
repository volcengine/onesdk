#ifndef UTILS_H
#define UTILS_H
#include "error_code.h"

#define CHECK_NULL_POINTER(ptr)                            \
    do {                                                   \
        if (NULL == (ptr)) {                               \
            printf("Invalid argument, %s = %p", #ptr, ptr); \
            return (VOLC_ERR_NULL_POINTER);         \
        }                                                  \
    } while (0)

#define PANIC_OOM(mem, msg)                                                                                        \
    do {                                                                                                           \
        if (!(mem)) {                                                                                              \
            fprintf(stderr, "%s", msg);                                                                            \
            exit(-1);                                                                                              \
        }                                                                                                          \
    } while (0)

#define ZERO_STRUCT(object)                                                                                        \
    do {                                                                                                               \
        memset(&(object), 0, sizeof(object));                                                                          \
    } while (0)
#endif //UTILS_H
