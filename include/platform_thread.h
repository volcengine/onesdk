#ifndef PLATFORM_THREAD_H
#define PLATFORM_THREAD_H

#ifdef _WIN32
#include <windows.h>
#include <process.h>

typedef CRITICAL_SECTION platform_mutex_t;

#define platform_mutex_init(mutex) InitializeCriticalSection(&(mutex))
#define platform_mutex_destroy(mutex) DeleteCriticalSection(&(mutex))
#define platform_mutex_lock(mutex) EnterCriticalSection(&(mutex))
#define platform_mutex_unlock(mutex) LeaveCriticalSection(&(mutex))

#else
#include <pthread.h>

typedef pthread_mutex_t platform_mutex_t;

#define platform_mutex_init(mutex) pthread_mutex_init(&(mutex), NULL)
#define platform_mutex_destroy(mutex) pthread_mutex_destroy(&(mutex))
#define platform_mutex_lock(mutex) pthread_mutex_lock(&(mutex))
#define platform_mutex_unlock(mutex) pthread_mutex_unlock(&(mutex))

#endif

#endif // PLATFORM_THREAD_H 