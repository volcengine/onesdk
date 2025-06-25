#ifndef VEI_UTIL_H
#define VEI_UTIL_H

#include <stdint.h>
#include <time.h>
#include "aws/common/allocator.h"
#include "aws/common/date_time.h"

uint64_t unix_timestamp_ms();

// TODO: HAL here?
uint64_t unix_timestamp();

int32_t random_num();

struct aws_allocator *aws_alloc();
char *get_random_string_with_time_suffix(struct aws_allocator *allocator);

#endif // VEI_UTIL_H