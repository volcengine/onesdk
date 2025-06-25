//
// Created by bgd on 2025/2/10.
//

#include <string.h>

#include "util.h"
#include "plat/alloc.h"
#include "plat/util.h"
#include <stdint.h>
#include <inttypes.h>

uint64_t unix_timestamp_ms() {
    return plat_unix_timestamp_ms();
}

uint64_t unix_timestamp() {
    return plat_unix_timestamp();
}

int32_t random_num() {
    return plat_random_num();
}

struct aws_allocator *aws_alloc(void) {
    return plat_aws_alloc();
}

char *get_random_string_with_time_suffix(struct aws_allocator *allocator) {
    uint32_t randomVal = 0;
    randomVal = (uint32_t)random_num();
    struct aws_date_time current_time;
    aws_date_time_init_now(&current_time);
    char *outStr = aws_mem_acquire(allocator, 50);
    sprintf(outStr, "%" PRIu32 "%" PRIu64, randomVal, aws_date_time_as_millis(&current_time));
    return outStr;
}
