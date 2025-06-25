#include "atomics.h"
#include "common.h"

AWS_EXTERN_C_BEGIN

/**
 * Reads an atomic var as an integer, using sequentially consistent ordering, and returns the result.
 */
AWS_STATIC_IMPL
size_t aws_atomic_load_int(volatile const struct aws_atomic_var *var) {
    return aws_atomic_load_int_explicit(var, aws_memory_order_seq_cst);
}

/**
 * Reads an atomic var as a pointer, using sequentially consistent ordering, and returns the result.
 */
AWS_STATIC_IMPL
void *aws_atomic_load_ptr(volatile const struct aws_atomic_var *var) {
    return aws_atomic_load_ptr_explicit(var, aws_memory_order_seq_cst);
}

/**
 * Stores an integer into an atomic var, using sequentially consistent ordering.
 */
AWS_STATIC_IMPL
void aws_atomic_store_int(volatile struct aws_atomic_var *var, size_t n) {
    aws_atomic_store_int_explicit(var, n, aws_memory_order_seq_cst);
}

/**
 * Stores a pointer into an atomic var, using sequentially consistent ordering.
 */
AWS_STATIC_IMPL
void aws_atomic_store_ptr(volatile struct aws_atomic_var *var, void *p) {
    aws_atomic_store_ptr_explicit(var, p, aws_memory_order_seq_cst);
}

/**
 * Exchanges an integer with the value in an atomic_var, using sequentially consistent ordering.
 * Returns the value that was previously in the atomic_var.
 */
AWS_STATIC_IMPL
size_t aws_atomic_exchange_int(volatile struct aws_atomic_var *var, size_t n) {
    return aws_atomic_exchange_int_explicit(var, n, aws_memory_order_seq_cst);
}

/**
 * Exchanges an integer with the value in an atomic_var, using sequentially consistent ordering.
 * Returns the value that was previously in the atomic_var.
 */
AWS_STATIC_IMPL
void *aws_atomic_exchange_ptr(volatile struct aws_atomic_var *var, void *p) {
    return aws_atomic_exchange_ptr_explicit(var, p, aws_memory_order_seq_cst);
}

/**
 * Atomically compares *var to *expected; if they are equal, atomically sets *var = desired. Otherwise, *expected is set
 * to the value in *var. Uses sequentially consistent memory ordering, regardless of success or failure.
 * Returns true if the compare was successful and the variable updated to desired.
 */
AWS_STATIC_IMPL
bool aws_atomic_compare_exchange_int(volatile struct aws_atomic_var *var, size_t *expected, size_t desired) {
    return aws_atomic_compare_exchange_int_explicit(
        var, expected, desired, aws_memory_order_seq_cst, aws_memory_order_seq_cst);
}

/**
 * Atomically compares *var to *expected; if they are equal, atomically sets *var = desired. Otherwise, *expected is set
 * to the value in *var. Uses sequentially consistent memory ordering, regardless of success or failure.
 * Returns true if the compare was successful and the variable updated to desired.
 */
AWS_STATIC_IMPL
bool aws_atomic_compare_exchange_ptr(volatile struct aws_atomic_var *var, void **expected, void *desired) {
    return aws_atomic_compare_exchange_ptr_explicit(
        var, expected, desired, aws_memory_order_seq_cst, aws_memory_order_seq_cst);
}

/**
 * Atomically adds n to *var, and returns the previous value of *var.
 * Uses sequentially consistent ordering.
 */
AWS_STATIC_IMPL
size_t aws_atomic_fetch_add(volatile struct aws_atomic_var *var, size_t n) {
    return aws_atomic_fetch_add_explicit(var, n, aws_memory_order_seq_cst);
}

/**
 * Atomically subtracts n from *var, and returns the previous value of *var.
 * Uses sequentially consistent ordering.
 */
AWS_STATIC_IMPL
size_t aws_atomic_fetch_sub(volatile struct aws_atomic_var *var, size_t n) {
    return aws_atomic_fetch_sub_explicit(var, n, aws_memory_order_seq_cst);
}

/**
 * Atomically ands n into *var, and returns the previous value of *var.
 * Uses sequentially consistent ordering.
 */
AWS_STATIC_IMPL
size_t aws_atomic_fetch_and(volatile struct aws_atomic_var *var, size_t n) {
    return aws_atomic_fetch_and_explicit(var, n, aws_memory_order_seq_cst);
}

/**
 * Atomically ors n into *var, and returns the previous value of *var.
 * Uses sequentially consistent ordering.
 */
AWS_STATIC_IMPL
size_t aws_atomic_fetch_or(volatile struct aws_atomic_var *var, size_t n) {
    return aws_atomic_fetch_or_explicit(var, n, aws_memory_order_seq_cst);
}

/**
 * Atomically xors n into *var, and returns the previous value of *var.
 * Uses sequentially consistent ordering.
 */
AWS_STATIC_IMPL
size_t aws_atomic_fetch_xor(volatile struct aws_atomic_var *var, size_t n) {
    return aws_atomic_fetch_xor_explicit(var, n, aws_memory_order_seq_cst);
}
AWS_EXTERN_C_END

#include <stdint.h>
#include <stdlib.h>

AWS_EXTERN_C_BEGIN

#ifdef __clang__
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wc11-extensions"
#else
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#endif

typedef size_t aws_atomic_impl_int_t;

static inline int aws_atomic_priv_xlate_order(enum aws_memory_order order) {
    switch (order) {
        case aws_memory_order_relaxed:
            return __ATOMIC_RELAXED;
        case aws_memory_order_acquire:
            return __ATOMIC_ACQUIRE;
        case aws_memory_order_release:
            return __ATOMIC_RELEASE;
        case aws_memory_order_acq_rel:
            return __ATOMIC_ACQ_REL;
        case aws_memory_order_seq_cst:
            return __ATOMIC_SEQ_CST;
        default: /* Unknown memory order */
            abort();
    }
}

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
    return __atomic_load_n(&AWS_ATOMIC_VAR_INTVAL(var), aws_atomic_priv_xlate_order(memory_order));
}

/**
 * Reads an atomic var as a pointer, using the specified ordering, and returns the result.
 */
AWS_STATIC_IMPL
void *aws_atomic_load_ptr_explicit(volatile const struct aws_atomic_var *var, enum aws_memory_order memory_order) {
    return __atomic_load_n(&AWS_ATOMIC_VAR_PTRVAL(var), aws_atomic_priv_xlate_order(memory_order));
}

/**
 * Stores an integer into an atomic var, using the specified ordering.
 */
AWS_STATIC_IMPL
void aws_atomic_store_int_explicit(volatile struct aws_atomic_var *var, size_t n, enum aws_memory_order memory_order) {
    __atomic_store_n(&AWS_ATOMIC_VAR_INTVAL(var), n, aws_atomic_priv_xlate_order(memory_order));
}

/**
 * Stores an pointer into an atomic var, using the specified ordering.
 */
AWS_STATIC_IMPL
void aws_atomic_store_ptr_explicit(volatile struct aws_atomic_var *var, void *p, enum aws_memory_order memory_order) {
    __atomic_store_n(&AWS_ATOMIC_VAR_PTRVAL(var), p, aws_atomic_priv_xlate_order(memory_order));
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
    return __atomic_exchange_n(&AWS_ATOMIC_VAR_INTVAL(var), n, aws_atomic_priv_xlate_order(memory_order));
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
    return __atomic_exchange_n(&AWS_ATOMIC_VAR_PTRVAL(var), p, aws_atomic_priv_xlate_order(memory_order));
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
    return __atomic_compare_exchange_n(
        &AWS_ATOMIC_VAR_INTVAL(var),
        expected,
        desired,
        false,
        aws_atomic_priv_xlate_order(order_success),
        aws_atomic_priv_xlate_order(order_failure));
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
    return __atomic_compare_exchange_n(
        &AWS_ATOMIC_VAR_PTRVAL(var),
        expected,
        desired,
        false,
        aws_atomic_priv_xlate_order(order_success),
        aws_atomic_priv_xlate_order(order_failure));
}

/**
 * Atomically adds n to *var, and returns the previous value of *var.
 */
AWS_STATIC_IMPL
size_t aws_atomic_fetch_add_explicit(volatile struct aws_atomic_var *var, size_t n, enum aws_memory_order order) {
    return __atomic_fetch_add(&AWS_ATOMIC_VAR_INTVAL(var), n, aws_atomic_priv_xlate_order(order));
}

/**
 * Atomically subtracts n from *var, and returns the previous value of *var.
 */
AWS_STATIC_IMPL
size_t aws_atomic_fetch_sub_explicit(volatile struct aws_atomic_var *var, size_t n, enum aws_memory_order order) {
    return __atomic_fetch_sub(&AWS_ATOMIC_VAR_INTVAL(var), n, aws_atomic_priv_xlate_order(order));
}

/**
 * Atomically ORs n with *var, and returns the previous value of *var.
 */
AWS_STATIC_IMPL
size_t aws_atomic_fetch_or_explicit(volatile struct aws_atomic_var *var, size_t n, enum aws_memory_order order) {
    return __atomic_fetch_or(&AWS_ATOMIC_VAR_INTVAL(var), n, aws_atomic_priv_xlate_order(order));
}

/**
 * Atomically ANDs n with *var, and returns the previous value of *var.
 */
AWS_STATIC_IMPL
size_t aws_atomic_fetch_and_explicit(volatile struct aws_atomic_var *var, size_t n, enum aws_memory_order order) {
    return __atomic_fetch_and(&AWS_ATOMIC_VAR_INTVAL(var), n, aws_atomic_priv_xlate_order(order));
}

/**
 * Atomically XORs n with *var, and returns the previous value of *var.
 */
AWS_STATIC_IMPL
size_t aws_atomic_fetch_xor_explicit(volatile struct aws_atomic_var *var, size_t n, enum aws_memory_order order) {
    return __atomic_fetch_xor(&AWS_ATOMIC_VAR_INTVAL(var), n, aws_atomic_priv_xlate_order(order));
}

/**
 * Provides the same reordering guarantees as an atomic operation with the specified memory order, without
 * needing to actually perform an atomic operation.
 */
AWS_STATIC_IMPL
void aws_atomic_thread_fence(enum aws_memory_order order) {
    __atomic_thread_fence(order);
}

#ifdef __clang__
#    pragma clang diagnostic pop
#else
#    pragma GCC diagnostic pop
#endif

#define AWS_ATOMICS_HAVE_THREAD_FENCE
AWS_EXTERN_C_END

#ifndef AWS_ATOMICS_HAVE_THREAD_FENCE

void aws_atomic_thread_fence(enum aws_memory_order order) {
    struct aws_atomic_var var;
    aws_atomic_int_t expected = 0;

    aws_atomic_store_int(&var, expected, aws_memory_order_relaxed);
    aws_atomic_compare_exchange_int(&var, &expected, 1, order, aws_memory_order_relaxed);
}

#endif /* AWS_ATOMICS_HAVE_THREAD_FENCE */