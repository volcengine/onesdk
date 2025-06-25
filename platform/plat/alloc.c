// Copyright (2025) Beijing Volcano Engine Technology Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string.h>
#include "alloc.h"

static void *s_non_aligned_malloc(struct aws_allocator *allocator, size_t size) {
    (void)allocator;
    void *result = malloc(size);
    return result;
}

static void s_non_aligned_free(struct aws_allocator *allocator, void *ptr) {
    (void)allocator;
    free(ptr);
}

static void *s_non_aligned_realloc(struct aws_allocator *allocator, void *ptr, size_t oldsize, size_t newsize) {
    (void)allocator;
    (void)oldsize;

    if (newsize <= oldsize) {
        return ptr;
    }

    /* newsize is > oldsize, need more memory */
    void *new_mem = s_non_aligned_malloc(allocator, newsize);

    if (ptr) {
        memcpy(new_mem, ptr, oldsize);
        s_non_aligned_free(allocator, ptr);
    }

    return new_mem;
}

static void *s_non_aligned_calloc(struct aws_allocator *allocator, size_t num, size_t size) {
    (void)allocator;
    void *mem = calloc(num, size);
    return mem;
}


static struct aws_allocator _allocator = {
    .mem_acquire = s_non_aligned_malloc,
    .mem_release = s_non_aligned_free,
    .mem_realloc = s_non_aligned_realloc,
    .mem_calloc = s_non_aligned_calloc,
};

struct aws_allocator *plat_aws_alloc(void) {
    return &_allocator;
}
