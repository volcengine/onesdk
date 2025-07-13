#include "atomics.h"
#include "common.h"
#include "../../../include/platform_compat.h"

AWS_EXTERN_C_BEGIN

typedef size_t aws_atomic_impl_int_t;

/**
 * Initializes an atomic variable with an integer value. This operation should be done before any
 * other operations on this atomic variable, and must be done before attempting any parallel operations.
 */
AWS_STATIC_IMPL
void aws_atomic_init_int(volatile struct aws_atomic_var *var, size_t n) {
    AWS_ATOMIC_VAR_INTVAL(var) = n;
}

/**
 * Initializes an atomic variable with a pointer value. This operation should be done before any
 * other operations on this atomic variable, and must be done before attempting any parallel operations.
 */
AWS_STATIC_IMPL
void aws_atomic_init_ptr(volatile struct aws_atomic_var *var, void *p) {
    AWS_ATOMIC_VAR_PTRVAL(var) = p;
}

/**
 * Reads an atomic var as an integer, using the specified ordering, and returns the result.
 */
AWS_STATIC_IMPL
size_t aws_atomic_load_int_explicit(volatile const struct aws_atomic_var *var, enum aws_memory_order memory_order) {
    (void)memory_order;
    return (size_t)atomic_load_long((long*)&AWS_ATOMIC_VAR_INTVAL(var));
}

/**
 * Reads an atomic var as a pointer, using the specified ordering, and returns the result.
 */
AWS_STATIC_IMPL
void *aws_atomic_load_ptr_explicit(volatile const struct aws_atomic_var *var, enum aws_memory_order memory_order) {
    (void)memory_order;
    return atomic_load_ptr((void**)&AWS_ATOMIC_VAR_PTRVAL(var));
}

/**
 * Stores an integer into an atomic var, using the specified ordering.
 */
AWS_STATIC_IMPL
void aws_atomic_store_int_explicit(volatile struct aws_atomic_var *var, size_t n, enum aws_memory_order memory_order) {
    (void)memory_order;
    atomic_store_long((long*)&AWS_ATOMIC_VAR_INTVAL(var), (long)n);
}

/**
 * Stores an pointer into an atomic var, using the specified ordering.
 */
AWS_STATIC_IMPL
void aws_atomic_store_ptr_explicit(volatile struct aws_atomic_var *var, void *p, enum aws_memory_order memory_order) {
    (void)memory_order;
    atomic_store_ptr((void**)&AWS_ATOMIC_VAR_PTRVAL(var), p);
}

/**
 * Exchanges an integer with the value in an atomic_var, using the specified ordering.
 * Returns the value that was previously in the atomic_var.
 */
AWS_STATIC_IMPL
size_t aws_atomic_exchange_int_explicit(
    volatile struct aws_atomic_var *var,
    size_t n,
    enum aws_memory_order memory_order) {
    (void)memory_order;
    return (size_t)atomic_exchange_long((long*)&AWS_ATOMIC_VAR_INTVAL(var), (long)n);
}

/**
 * Exchanges a pointer with the value in an atomic_var, using the specified ordering.
 * Returns the value that was previously in the atomic_var.
 */
AWS_STATIC_IMPL
void *aws_atomic_exchange_ptr_explicit(
    volatile struct aws_atomic_var *var,
    void *p,
    enum aws_memory_order memory_order) {
    (void)memory_order;
    return atomic_exchange_ptr((void**)&AWS_ATOMIC_VAR_PTRVAL(var), p);
}

/**
 * Atomically compares *var to *expected; if they are equal, atomically sets *var = desired. Otherwise, *expected is set
 * to the value in *var. On success, the memory ordering used was order_success; otherwise, it was order_failure.
 * order_failure must be no stronger than order_success, and must not be release or acq_rel.
 */
AWS_STATIC_IMPL
bool aws_atomic_compare_exchange_int_explicit(
    volatile struct aws_atomic_var *var,
    size_t *expected,
    size_t desired,
    enum aws_memory_order order_success,
    enum aws_memory_order order_failure) {
    (void)order_success;
    (void)order_failure;
    return atomic_compare_exchange_long((long*)&AWS_ATOMIC_VAR_INTVAL(var), (long*)expected, (long)desired) != 0;
}

/**
 * Atomically compares *var to *expected; if they are equal, atomically sets *var = desired. Otherwise, *expected is set
 * to the value in *var. On success, the memory ordering used was order_success; otherwise, it was order_failure.
 * order_failure must be no stronger than order_success, and must not be release or acq_rel.
 */
AWS_STATIC_IMPL
bool aws_atomic_compare_exchange_ptr_explicit(
    volatile struct aws_atomic_var *var,
    void **expected,
    void *desired,
    enum aws_memory_order order_success,
    enum aws_memory_order order_failure) {
    (void)order_success;
    (void)order_failure;
    return atomic_compare_exchange_ptr((void**)&AWS_ATOMIC_VAR_PTRVAL(var), expected, desired) != 0;
}

/**
 * Atomically adds n to *var, and returns the previous value of *var.
 */
AWS_STATIC_IMPL
size_t aws_atomic_fetch_add_explicit(volatile struct aws_atomic_var *var, size_t n, enum aws_memory_order order) {
    (void)order;
    return (size_t)atomic_add_long((long*)&AWS_ATOMIC_VAR_INTVAL(var), (long)n);
}

/**
 * Atomically subtracts n from *var, and returns the previous value of *var.
 */
AWS_STATIC_IMPL
size_t aws_atomic_fetch_sub_explicit(volatile struct aws_atomic_var *var, size_t n, enum aws_memory_order order) {
    (void)order;
    return (size_t)atomic_sub_long((long*)&AWS_ATOMIC_VAR_INTVAL(var), (long)n);
}

/**
 * Atomically ORs n with *var, and returns the previous value of *var.
 */
AWS_STATIC_IMPL
size_t aws_atomic_fetch_or_explicit(volatile struct aws_atomic_var *var, size_t n, enum aws_memory_order order) {
    (void)order;
    return (size_t)atomic_or_long((long*)&AWS_ATOMIC_VAR_INTVAL(var), (long)n);
}

/**
 * Atomically ANDs n with *var, and returns the previous value of *var.
 */
AWS_STATIC_IMPL
size_t aws_atomic_fetch_and_explicit(volatile struct aws_atomic_var *var, size_t n, enum aws_memory_order order) {
    (void)order;
    return (size_t)atomic_and_long((long*)&AWS_ATOMIC_VAR_INTVAL(var), (long)n);
}

/**
 * Atomically XORs n with *var, and returns the previous value of *var.
 */
AWS_STATIC_IMPL
size_t aws_atomic_fetch_xor_explicit(volatile struct aws_atomic_var *var, size_t n, enum aws_memory_order order) {
    (void)order;
    return (size_t)atomic_xor_long((long*)&AWS_ATOMIC_VAR_INTVAL(var), (long)n);
}

/**
 * Provides the same reordering guarantees as an atomic operation with the specified memory order, without
 * needing to actually perform an atomic operation.
 */
AWS_STATIC_IMPL
void aws_atomic_thread_fence(enum aws_memory_order order) {
    (void)order;
    // On Windows, we don't need to do anything for thread fence
    // The atomic operations themselves provide the necessary ordering
}

#define AWS_ATOMICS_HAVE_THREAD_FENCE
AWS_EXTERN_C_END

// Default implementations using seq_cst ordering
AWS_STATIC_IMPL
size_t aws_atomic_load_int(volatile const struct aws_atomic_var *var) {
    return (size_t)atomic_load_long((long*)&var->value);
}

AWS_STATIC_IMPL
void *aws_atomic_load_ptr(volatile const struct aws_atomic_var *var) {
    return atomic_load_ptr((void**)&var->value);
}

AWS_STATIC_IMPL
void aws_atomic_store_int(volatile struct aws_atomic_var *var, size_t n) {
    atomic_store_long((long*)&var->value, (long)n);
}

AWS_STATIC_IMPL
void aws_atomic_store_ptr(volatile struct aws_atomic_var *var, void *p) {
    atomic_store_ptr((void**)&var->value, p);
}

AWS_STATIC_IMPL
size_t aws_atomic_exchange_int(volatile struct aws_atomic_var *var, size_t n) {
    return (size_t)atomic_exchange_long((long*)&var->value, (long)n);
}

AWS_STATIC_IMPL
void *aws_atomic_exchange_ptr(volatile struct aws_atomic_var *var, void *p) {
    return atomic_exchange_ptr((void**)&var->value, p);
}

AWS_STATIC_IMPL
bool aws_atomic_compare_exchange_int(volatile struct aws_atomic_var *var, size_t *expected, size_t desired) {
    return atomic_compare_exchange_long((long*)&var->value, (long*)expected, (long)desired) != 0;
}

AWS_STATIC_IMPL
bool aws_atomic_compare_exchange_ptr(volatile struct aws_atomic_var *var, void **expected, void *desired) {
    return atomic_compare_exchange_ptr((void**)&var->value, expected, desired) != 0;
}

AWS_STATIC_IMPL
size_t aws_atomic_fetch_add(volatile struct aws_atomic_var *var, size_t n) {
    return (size_t)atomic_add_long((long*)&var->value, (long)n);
}

AWS_STATIC_IMPL
size_t aws_atomic_fetch_sub(volatile struct aws_atomic_var *var, size_t n) {
    return (size_t)atomic_sub_long((long*)&var->value, (long)n);
}

AWS_STATIC_IMPL
size_t aws_atomic_fetch_and(volatile struct aws_atomic_var *var, size_t n) {
    return (size_t)atomic_and_long((long*)&var->value, (long)n);
}

AWS_STATIC_IMPL
size_t aws_atomic_fetch_or(volatile struct aws_atomic_var *var, size_t n) {
    return (size_t)atomic_or_long((long*)&var->value, (long)n);
}

AWS_STATIC_IMPL
size_t aws_atomic_fetch_xor(volatile struct aws_atomic_var *var, size_t n) {
    return (size_t)atomic_xor_long((long*)&var->value, (long)n);
}