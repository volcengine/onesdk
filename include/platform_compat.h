#ifndef PLATFORM_COMPAT_H
#define PLATFORM_COMPAT_H

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <io.h>
#include <time.h>

// POSIX compatibility
#define F_OK 0
#define R_OK 4
#define W_OK 2
#define X_OK 1

// POSIX function compatibility
#define access _access
#define strdup _strdup
#define pclose _pclose

// Sleep functions
#define sleep(seconds) Sleep((seconds) * 1000)
#define usleep(microseconds) Sleep((microseconds) / 1000)

// Clock constants (Windows doesn't have these)
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

// Windows implementation of clock_gettime
static inline int clock_gettime(int clk_id, struct timespec *tp) {
    FILETIME ft;
    ULARGE_INTEGER ui;
    GetSystemTimeAsFileTime(&ft);
    ui.LowPart = ft.dwLowDateTime;
    ui.HighPart = ft.dwHighDateTime;
    
    // Convert to seconds since Unix epoch
    ui.QuadPart -= 116444736000000000ULL;
    
    tp->tv_sec = (long)(ui.QuadPart / 10000000ULL);
    tp->tv_nsec = (long)((ui.QuadPart % 10000000ULL) * 100);
    
    return 0;
}

// Define timezone structure if not already defined
#ifndef _TIMEZONE_DEFINED
#define _TIMEZONE_DEFINED
struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};
#endif

// Windows implementation of gettimeofday
static inline int gettimeofday(struct timeval *tv, struct timezone *tz) {
    FILETIME ft;
    ULARGE_INTEGER ui;
    GetSystemTimeAsFileTime(&ft);
    ui.LowPart = ft.dwLowDateTime;
    ui.HighPart = ft.dwHighDateTime;
    
    // Convert to seconds since Unix epoch
    ui.QuadPart -= 116444736000000000ULL;
    
    tv->tv_sec = (long)(ui.QuadPart / 10000000ULL);
    tv->tv_usec = (long)((ui.QuadPart % 10000000ULL) / 10);
    
    if (tz) {
        TIME_ZONE_INFORMATION tzi;
        GetTimeZoneInformation(&tzi);
        tz->tz_minuteswest = tzi.Bias;
        tz->tz_dsttime = 0;
    }
    
    return 0;
}

// Windows POSIX compatibility functions
static inline char* strtok_r(char* str, const char* delim, char** saveptr) {
    return strtok_s(str, delim, saveptr);
}

static inline struct tm* localtime_r(const time_t* timep, struct tm* result) {
    localtime_s(result, timep);
    return result;
}

static inline struct tm* gmtime_r(const time_t* timep, struct tm* result) {
    gmtime_s(result, timep);
    return result;
}

// Windows atomic operations
static inline void* atomic_load_ptr(void** ptr) {
    return InterlockedCompareExchangePointer(ptr, NULL, NULL);
}

static inline void atomic_store_ptr(void** ptr, void* value) {
    InterlockedExchangePointer(ptr, value);
}

static inline void* atomic_exchange_ptr(void** ptr, void* value) {
    return InterlockedExchangePointer(ptr, value);
}

static inline int atomic_compare_exchange_ptr(void** ptr, void** expected, void* desired) {
    void* old = InterlockedCompareExchangePointer(ptr, desired, *expected);
    if (old == *expected) {
        return 1; // success
    } else {
        *expected = old;
        return 0; // failure
    }
}

static inline long atomic_load_long(long* ptr) {
    return InterlockedCompareExchange(ptr, 0, 0);
}

static inline void atomic_store_long(long* ptr, long value) {
    InterlockedExchange(ptr, value);
}

static inline long atomic_exchange_long(long* ptr, long value) {
    return InterlockedExchange(ptr, value);
}

static inline long atomic_add_long(long* ptr, long value) {
    return InterlockedExchangeAdd(ptr, value);
}

static inline long atomic_sub_long(long* ptr, long value) {
    return InterlockedExchangeAdd(ptr, -value);
}

static inline long atomic_and_long(long* ptr, long value) {
    return InterlockedAnd(ptr, value);
}

static inline long atomic_or_long(long* ptr, long value) {
    return InterlockedOr(ptr, value);
}

static inline long atomic_xor_long(long* ptr, long value) {
    return InterlockedXor(ptr, value);
}

static inline int atomic_compare_exchange_long(long* ptr, long* expected, long desired) {
    long old = InterlockedCompareExchange(ptr, desired, *expected);
    if (old == *expected) {
        return 1; // success
    } else {
        *expected = old;
        return 0; // failure
    }
}

#else
// Non-Windows platforms
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

// Use standard POSIX functions
#define atomic_load_ptr(ptr) __atomic_load_n(ptr, __ATOMIC_SEQ_CST)
#define atomic_store_ptr(ptr, value) __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST)
#define atomic_exchange_ptr(ptr, value) __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST)
#define atomic_compare_exchange_ptr(ptr, expected, desired) __atomic_compare_exchange_n(ptr, expected, desired, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

#define atomic_load_long(ptr) __atomic_load_n(ptr, __ATOMIC_SEQ_CST)
#define atomic_store_long(ptr, value) __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST)
#define atomic_exchange_long(ptr, value) __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST)
#define atomic_add_long(ptr, value) __atomic_add_fetch(ptr, value, __ATOMIC_SEQ_CST)
#define atomic_sub_long(ptr, value) __atomic_sub_fetch(ptr, value, __ATOMIC_SEQ_CST)
#define atomic_and_long(ptr, value) __atomic_and_fetch(ptr, value, __ATOMIC_SEQ_CST)
#define atomic_or_long(ptr, value) __atomic_or_fetch(ptr, value, __ATOMIC_SEQ_CST)
#define atomic_xor_long(ptr, value) __atomic_xor_fetch(ptr, value, __ATOMIC_SEQ_CST)
#define atomic_compare_exchange_long(ptr, expected, desired) __atomic_compare_exchange_n(ptr, expected, desired, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

#endif

#endif // PLATFORM_COMPAT_H 