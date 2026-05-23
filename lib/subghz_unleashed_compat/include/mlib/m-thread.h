/*
 * M*LIB - Thin Mutex & Thread wrapper
 *
 * Copyright (c) 2017-2026, Patrick Pelissier
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * + Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * + Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef MSTARLIB_MUTEX_H
#define MSTARLIB_MUTEX_H

/* Auto-detect the thread backend to use if the user has not override it */
#ifndef M_USE_THREAD_BACKEND
# if defined(INC_FREERTOS_H)
#  define M_USE_THREAD_BACKEND 4
# elif defined(WIN32) || defined(_WIN32) || defined(__CYGWIN__)
#  define M_USE_THREAD_BACKEND 2
# elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L               \
  && !defined(__STDC_NO_THREADS__)
#  define M_USE_THREAD_BACKEND 1
# else
#  define M_USE_THREAD_BACKEND 3
# endif
#endif


/****************************** C11 version ********************************/
#if M_USE_THREAD_BACKEND == 1

#include <threads.h>
#include <assert.h>
#include <stdbool.h>
#include "m-core.h"

M_BEGIN_PROTECTED_CODE

/* Define a mutex type based on C11 definition */
typedef mtx_t                  m_mutex_t[1];

/* Define a condition variable type based on C11 definition */
typedef cnd_t                  m_cond_t[1];

/* Define a thread type based on C11 definition */
typedef thrd_t                 m_thread_t[1];

/* Initialize the mutex (constructor) */
M_INLINE void m_mutex_init(m_mutex_t m)
{
  int rc = mtx_init(m, mtx_plain);
  // Abort program in case of initialization failure
  // There is really nothing else to do if a mutex cannot be constructed
  M_ASSERT_INIT (rc == thrd_success, "mutex");
}

/* Clear the mutex (destructor) */
M_INLINE void m_mutex_clear(m_mutex_t m)
{
  mtx_destroy(m);
}

/* Lock the mutex */
M_INLINE void m_mutex_lock(m_mutex_t m)
{
  mtx_lock(m);
}

/* Try to lock the mutex. Return true on success */
M_INLINE bool m_mutex_trylock(m_mutex_t m)
{
  return mtx_trylock(m) == thrd_success;
}

/* Unlock the mutex */
M_INLINE void m_mutex_unlock(m_mutex_t m)
{
  mtx_unlock(m);
}

/* Initialize the condition variable (constructor) */
M_INLINE void m_cond_init(m_cond_t c)
{
  int rc = cnd_init(c);
  // Abort program in case of initialization failure
  // There is really nothing else to do if the object cannot be constructed
  M_ASSERT_INIT (rc == thrd_success, "conditional variable");
}

/* Clear the condition variable (destructor) */
M_INLINE void m_cond_clear(m_cond_t c)
{
  cnd_destroy(c);
}

/* Signal the condition variable to at least one waiting thread */
M_INLINE void m_cond_signal(m_cond_t c)
{
  cnd_signal(c);
}

/* Signal the condition variable to all waiting threads */
M_INLINE void m_cond_broadcast(m_cond_t c)
{
  cnd_broadcast(c);
}

/* Wait for signaling the condition variable by another thread */
M_INLINE void m_cond_wait(m_cond_t c, m_mutex_t m)
{
  cnd_wait(c, m);
}

/* Create the thread (constructor) and start it */
M_INLINE void m_thread_create(m_thread_t t, void (*func)(void*), void* arg)
{
  int rc = thrd_create(t, (int(*)(void*))(void(*)(void))func, arg);
  // Abort program in case of initialization failure
  M_ASSERT_INIT (rc == thrd_success, "thread");
}

/* Wait for the thread to terminate and destroy it (destructor) */
M_INLINE void m_thread_join(m_thread_t t)
{
  int rc = thrd_join(*t, NULL);
  M_ASSERT (rc == thrd_success);
  // Avoid warning about variable unused.
  (void) rc;
}

/* The thread has nothing meaningful to do.
   Inform the OS to let other threads be scheduled */
M_INLINE void m_thread_yield(void)
{
  thrd_yield();
}

/* Sleep the thread for at least usec microseconds.
   Return true if the sleep was successful (or we cannot know) */
M_INLINE bool m_thread_sleep(unsigned long long usec)
{
  struct timespec tv;
  tv.tv_sec = (long) (usec / 1000000ULL);
  tv.tv_nsec = (long) ((usec % 1000000ULL) * 1000UL);
  int retval = thrd_sleep(&tv, NULL);
  return retval == 0;
}

// a helper structure for m_once_call
typedef once_flag                     m_once_t[1];

// Initial value for m_once_t
#define M_ONCE_INIT_VALUE            { ONCE_FLAG_INIT }

// Call the function exactly once
M_INLINE void m_once_call(m_once_t o, void (*func)(void))
{
  call_once(o,func);
}

// Attribute to use to allocate a global variable to a thread.
#define M_THREAD_ATTR _Thread_local

M_END_PROTECTED_CODE


/****************************** WIN32 version ******************************/
#elif M_USE_THREAD_BACKEND == 2

/* CLANG provides some useless and wrong warnings:
 * - _WIN32_WINNT starts with '_' which is reserved by the standard
 * as per the MSVC compiler, it is needed to be defined by the user
 * to define which version of windows it want to be compatible with.
 * - windows.h may be different than the case used by the file sytem
 * there is however no normalized case.
 *
 * So, theses warnings have to be ignored and are disabled.
 *
 * We cannot add theses warnings in M_BEGIN_PROTECTED_CODE
 * as they need to be disabled **BEFORE** including any system header
 * and m-core includes some system headers.
 * So we need to disable them explicitly here.
 */
#if defined(__clang__) && __clang_major__ >= 4
  _Pragma("clang diagnostic push")
  _Pragma("clang diagnostic ignored \"-Wreserved-id-macro\"")
  _Pragma("clang diagnostic ignored \"-Wnonportable-system-include-path\"")
#endif

/* CriticalSection & ConditionVariable are available from Windows Vista */
#ifndef WINVER
#define WINVER        _WIN32_WINNT_VISTA
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT  _WIN32_WINNT_VISTA
#endif

/* Include system headers */
#include <windows.h>
#include <assert.h>
#include <stdbool.h>
#include "m-core.h"

#if defined(__clang__) && __clang_major__ >= 4
  _Pragma("clang diagnostic pop")
#endif

M_BEGIN_PROTECTED_CODE

/* Define a thread type based on WINDOWS definition */
typedef HANDLE                 m_thread_t[1];

/* Define a mutex type based on WINDOWS definition */
typedef CRITICAL_SECTION       m_mutex_t[1];

/* Define a condition variable type based on WINDOWS definition */
typedef CONDITION_VARIABLE     m_cond_t[1];

/* Initialize a mutex (Constructor)*/
M_INLINE void m_mutex_init(m_mutex_t m)
{
  InitializeCriticalSection(m);
}

/* Clear a mutex (destructor) */
M_INLINE void m_mutex_clear(m_mutex_t m)
{
  DeleteCriticalSection(m);
}

/* Lock a mutex */
M_INLINE void m_mutex_lock(m_mutex_t m)
{
  EnterCriticalSection(m);
}

/* Try to Lock a mutex. Return true on success */
M_INLINE bool m_mutex_trylock(m_mutex_t m)
{
  return TryEnterCriticalSection(m) != 0;
}

/* Unlock a mutex */
M_INLINE void m_mutex_unlock(m_mutex_t m)
{
  LeaveCriticalSection(m);
}

/* Initialize a condition variable (constructor) */
M_INLINE void m_cond_init(m_cond_t c)
{
  InitializeConditionVariable(c);
}

/* Clear a condition variable (destructor) */
M_INLINE void m_cond_clear(m_cond_t c)
{
  (void) c; // There is no destructor for this object.
}

/* Signal a condition variable to at least one waiting thread */
M_INLINE void m_cond_signal(m_cond_t c)
{
  WakeConditionVariable(c);
}

/* Signal a condition variable to all waiting threads */
M_INLINE void m_cond_broadcast(m_cond_t c)
{
  WakeAllConditionVariable(c);
}

/* Wait for a condition variable */
M_INLINE void m_cond_wait(m_cond_t c, m_mutex_t m)
{
  SleepConditionVariableCS(c, m, INFINITE);
}

/* Create a thread (constructor) and start it */
M_INLINE void m_thread_create(m_thread_t t, void (*func)(void*), void *arg)
{
  *t = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) (uintptr_t) func, arg, 0, NULL);
  M_ASSERT_INIT (*t != NULL, "thread");
}

/* Wait for the thread to terminate and destroy it (destructor) */
M_INLINE void m_thread_join(m_thread_t t)
{
  DWORD dwWaitResult = WaitForSingleObject(*t, INFINITE);
  (void) dwWaitResult;
  M_ASSERT (dwWaitResult == WAIT_OBJECT_0);
  CloseHandle(*t);
}

/* The thread has nothing meaningful to do.
   Inform the OS to let other threads be scheduled */
M_INLINE void m_thread_yield(void)
{
  Sleep(0);
}

/* Sleep the thread for at least usec microseconds
   Return true if the sleep was successful */
M_INLINE bool m_thread_sleep(unsigned long long usec)
{
  LARGE_INTEGER ft;
  M_ASSERT (usec <= LLONG_MAX);
  ft.QuadPart = -(10LL*(long long) usec);
  HANDLE hd = CreateWaitableTimer(NULL, TRUE, NULL);
  M_ASSERT_INIT (hd != NULL, "timer");
  SetWaitableTimer(hd, &ft, 0, NULL, NULL, 0);
  DWORD dwWaitResult = WaitForSingleObject(hd, INFINITE);
  CloseHandle(hd);
  return dwWaitResult == WAIT_OBJECT_0;
}


typedef INIT_ONCE                     m_once_t[1];
#define M_ONCE_INIT_VALUE            { INIT_ONCE_STATIC_INIT }
M_INLINE BOOL CALLBACK m_once_callback( PINIT_ONCE InitOnce, PVOID Parameter, PVOID *lpContext)
{
    void (*func)(void);
    (void) InitOnce;
    (void) lpContext;
    func = (void (*)(void))(uintptr_t) Parameter;
    (*func)();
    return TRUE;
}
M_INLINE void m_once_call(m_once_t o, void (*func)(void))
{
  InitOnceExecuteOnce(o, m_once_callback, (void*)(intptr_t)func, NULL);
}

#if defined(_MSC_VER)
// Attribute to use to allocate a global variable to a thread (MSVC def).
# define M_THREAD_ATTR __declspec( thread )
#else
// Attribute to use to allocate a global variable to a thread (GCC def).
# define M_THREAD_ATTR __thread
#endif

M_END_PROTECTED_CODE


/**************************** PTHREAD version ******************************/
#elif M_USE_THREAD_BACKEND == 3

#include <pthread.h>
#ifdef _POSIX_PRIORITY_SCHEDULING
#include <sched.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include "m-core.h"

M_BEGIN_PROTECTED_CODE

/* Define a mutex type based on PTHREAD definition */
typedef pthread_mutex_t        m_mutex_t[1];

/* Define a condition variable type based on PTHREAD definition */
typedef pthread_cond_t         m_cond_t[1];

/* Define a thread type based on PTHREAD definition */
typedef pthread_t              m_thread_t[1];

/* Initialize the mutex (constructor) */
M_INLINE void m_mutex_init(m_mutex_t m)
{
  int _rc = pthread_mutex_init(m, NULL);
  // Abort program in case of initialization failure
  // There is really nothing else to do if a mutex cannot be constructed
  M_ASSERT_INIT (_rc == 0, "mutex");
}

/* Clear the mutex (destructor) */
M_INLINE void m_mutex_clear(m_mutex_t m)
{
  pthread_mutex_destroy(m);
}

/* Lock the mutex */
M_INLINE void m_mutex_lock(m_mutex_t m)
{
  pthread_mutex_lock(m);
}

/* Try to Lock a mutex. Return true on success */
M_INLINE bool m_mutex_trylock(m_mutex_t m)
{
  return pthread_mutex_trylock(m) == 0;
}

/* Unlock the mutex */
M_INLINE void m_mutex_unlock(m_mutex_t m)
{
  pthread_mutex_unlock(m);
}

/* Lazy lock initialization */
#define M_MUTEXI_INIT_VALUE    { PTHREAD_MUTEX_INITIALIZER }

/* Internal function compatible with lazy lock */
M_INLINE void m_mutexi_lazy_lock(m_mutex_t m)
{
  pthread_mutex_lock(m);
}

/* Initialize the condition variable (constructor) */
M_INLINE void m_cond_init(m_cond_t c)
{
  int _rc = pthread_cond_init(c, NULL);
  // Abort program in case of initialization failure
  // There is really nothing else to do if a mutex cannot be constructed
  M_ASSERT_INIT (_rc == 0, "conditional variable");
}

/* Clear the condition variable (destructor) */
M_INLINE void m_cond_clear(m_cond_t c)
{
  pthread_cond_destroy(c);
}

/* Signal a condition variable to at least a waiting thread */
M_INLINE void m_cond_signal(m_cond_t c)
{
  pthread_cond_signal(c);
}

/* Signal a condition variable to all waiting threads */
M_INLINE void m_cond_broadcast(m_cond_t c)
{
  pthread_cond_broadcast(c);
}

/* Waiting for a condition variable */
M_INLINE void m_cond_wait(m_cond_t c, m_mutex_t m)
{
  pthread_cond_wait(c, m);
}

/* Create a thread (constructor) and start it */
M_INLINE void m_thread_create(m_thread_t t, void (*func)(void*), void *arg)
{
  int _rc = pthread_create(t, NULL, (void*(*)(void*))(void(*)(void))func, arg);
  M_ASSERT_INIT (_rc == 0, "thread");
}

/* Wait for the thread to terminate and destroy it (destructor) */
M_INLINE void m_thread_join(m_thread_t t)
{
  int _rc = pthread_join(*t, NULL);
  (void)_rc; // Avoid warning about variable unused.
  M_ASSERT (_rc == 0);
}

/* The thread has nothing meaningful to do.
   Inform the OS to let other threads be scheduled */
M_INLINE void m_thread_yield(void)
{
#ifdef _POSIX_PRIORITY_SCHEDULING
  sched_yield();
#endif
}

/* Sleep for at least usec microseconds
   Return true if the sleep was successful */
M_INLINE bool m_thread_sleep(unsigned long long usec)
{
  struct timeval tv;
  /* We don't want to use usleep or nanosleep so that
     we remain compatible with strict C99 build */
  tv.tv_sec = (time_t) (usec / 1000000ULL);
  tv.tv_usec = (suseconds_t) (usec % 1000000ULL);
  int retval = select(1, NULL, NULL, NULL, &tv);
  return retval == 0;
}

typedef pthread_once_t                m_once_t[1];
#define M_ONCE_INIT_VALUE            { PTHREAD_ONCE_INIT }
M_INLINE void m_once_call(m_once_t o, void (*func)(void))
{
  pthread_once(o,func);
}

#if defined(__GNUC__)
# define M_THREAD_ATTR __thread
#else
# define M_THREAD_ATTR  /* Not supported */
#endif

M_END_PROTECTED_CODE

/****************************** FreeRTOS version ********************************/
#elif M_USE_THREAD_BACKEND == 4

#include <stdatomic.h>
#include <semphr.h>
#include <task.h>
#include "m-core.h"

M_BEGIN_PROTECTED_CODE

/* Default value for the stack */
#ifndef M_USE_TASK_STACK_SIZE
#define M_USE_TASK_STACK_SIZE    configMINIMAL_STACK_SIZE
#endif

/* Default value for the priority tasks */
#ifndef M_USE_TASK_PRIORITY
#define M_USE_TASK_PRIORITY ( tskIDLE_PRIORITY )
#endif

/* Define a mutex type based on FreeRTOS definition */
typedef struct m_mutex_s {
    SemaphoreHandle_t   handle;
    StaticSemaphore_t   MutexBuffer;
} m_mutex_t[1];

/* Define a thread type based on FreeRTOS definition */
typedef struct m_cond_s {
    SemaphoreHandle_t   handle;
    StaticSemaphore_t   SemBuffer;
    unsigned int        NumThreadWaiting;
} m_cond_t[1];

/* Define a thread type based on FreeRTOS definition */
typedef struct m_thread_s {
    SemaphoreHandle_t   SemHandle;
    StaticSemaphore_t   SemBuffer;
    TaskHandle_t        TaskHandle;
    StaticTask_t        TaskBuffer;
    void                (*EntryPoint)(void *);
    void*               ArgsEntryPoint;
    StackType_t*        StackBuffer;
} m_thread_t[1];

/* Initialize the mutex (constructor) */
M_INLINE void m_mutex_init(m_mutex_t m)
{
    /* Create a mutex semaphore without using any dynamic allocation */
    m->handle = xSemaphoreCreateMutexStatic(&m->MutexBuffer);
    // It cannot fail, so we won't use M_ASSERT_INIT
    M_ASSERT(m->handle);
}

/* Clear the mutex (destructor) */
M_INLINE void m_mutex_clear(m_mutex_t m)
{
    vSemaphoreDelete(m->handle);
}

/* Lock the mutex */
M_INLINE void m_mutex_lock(m_mutex_t m)
{
    xSemaphoreTake(m->handle, portMAX_DELAY);
}

/* Try to Lock a mutex. Return true on success */
M_INLINE bool m_mutex_trylock(m_mutex_t m)
{
  return xSemaphoreTake(m->handle, 0) == pdTRUE;
}

/* Unlock the mutex */
M_INLINE void m_mutex_unlock(m_mutex_t m)
{
    xSemaphoreGive(m->handle);
}


/* Initialize the condition variable (constructor) */
M_INLINE void m_cond_init(m_cond_t c)
{
    c->NumThreadWaiting = 0;
    // Create a semaphore to implement the conditional variable
    // Initial value is 0 and valid range is <= 0
    c->handle = xSemaphoreCreateCountingStatic( INT_MAX, 0, &c->SemBuffer );
    // It cannot fail, so we won't use M_ASSERT_INIT
    M_ASSERT(c->handle);
}

/* Clear the condition variable (destructor) */
M_INLINE void m_cond_clear(m_cond_t c)
{
    vSemaphoreDelete(c->handle);
}

/* Signal the condition variable to at least one waiting thread */
M_INLINE void m_cond_signal(m_cond_t c)
{
    // This function is called within the mutex lock
    // NumThreadWaiting doesn't need to be atomic
    if (c->NumThreadWaiting > 0) {
        // Wakeup one thread by posting on the semaphore
        xSemaphoreGive(c->handle);
    } // Otherwise there is no waiting thread, so nothing to signal
}

/* Signal the condition variable to all waiting threads */
M_INLINE void m_cond_broadcast(m_cond_t c)
{
    // This function is called within the mutex lock
    // NumThreadWaiting doesn't need to be atomic
    if (c->NumThreadWaiting > 0) {
        // Wakeup all thread by posting on the semaphore
        // as many times as there are waiting threads
        for(unsigned i = 0; i < c->NumThreadWaiting; i++) {
            xSemaphoreGive(c->handle);
        }
    } // Otherwise there is no waiting thread, so nothing to signal
}

/* Wait for signaling the condition variable by another thread */
M_INLINE void m_cond_wait(m_cond_t c, m_mutex_t m)
{
    // This function is called within the mutex lock
    // Increment the number of waiting thread
    c->NumThreadWaiting ++;
    m_mutex_unlock(m);
    // Wait for post in the semaphore
    xSemaphoreTake(c->handle, portMAX_DELAY);
    m_mutex_lock(m);
    c->NumThreadWaiting --;
}

M_INLINE void m_thr3ad_wrapper( void *args)
{
    struct m_thread_s *thread_ptr = args;
    thread_ptr->EntryPoint(thread_ptr->ArgsEntryPoint);
    // Give back the semaphore.
    xSemaphoreGive(thread_ptr->SemHandle);
    // Wait for destruction
    while (true) { vTaskSuspend(NULL); }
}

/* Create the thread (constructor) and start it */
M_INLINE void m_thread_create(m_thread_t t, void (*func)(void*), void* arg)
{
    // Create a semaphore to implement the final wait
    t->SemHandle = xSemaphoreCreateCountingStatic( 1, 0, &t->SemBuffer );
    M_ASSERT(t->SemHandle);
    // Save the argument to the thread
    t->EntryPoint = func;
    t->ArgsEntryPoint = arg;

    // Allocate the stack
    t->StackBuffer = pvPortMalloc( sizeof (StackType_t) * M_USE_TASK_STACK_SIZE);
    M_ASSERT_INIT(t->StackBuffer, "STACK");

    // Create the task without using any dynamic allocation
    t->TaskHandle = xTaskCreateStatic(m_thr3ad_wrapper, "M*LIB", M_USE_TASK_STACK_SIZE, (void*) t, M_USE_TASK_PRIORITY, t->StackBuffer, &t->TaskBuffer);
    // It cannot fail, so we won't use M_ASSERT_INIT
    M_ASSERT(t->TaskHandle);
}

/* Wait for the thread to terminate and destroy it (destructor) */
M_INLINE void m_thread_join(m_thread_t t)
{
    xSemaphoreTake(t->SemHandle, portMAX_DELAY);
    vTaskDelete(t->TaskHandle);
    vPortFree(t->StackBuffer);
    vSemaphoreDelete(t->SemHandle);
    t->TaskHandle = 0;
    t->StackBuffer = 0;
    t->SemHandle = 0;
}

/* The thread has nothing meaningful to do.
   Inform the OS to let other threads be scheduled */
M_INLINE void m_thread_yield(void)
{
    taskYIELD();
}

/* Sleep the thread for at least usec microseconds.
   Return true if the sleep was successful */
M_INLINE bool m_thread_sleep(unsigned long long usec)
{
    TickType_t  delay = (TickType_t) (usec / portTICK_PERIOD_MS / 1000ULL);
    vTaskDelay(delay);
    return true;
}

// a helper structure for m_once_call
typedef struct {
    atomic_int count;
} m_once_t[1];

// Initial value for m_once_t
#define M_ONCE_INIT_VALUE            { { M_ATOMIC_VAR_INIT(0) } }

// Call the function exactly once
M_INLINE void m_once_call(m_once_t o, void (*func)(void))
{
    if (atomic_load(&o->count) != 2) {
        int n = 0;
        if (atomic_compare_exchange_strong( &o->count, &n, 1)) {
            // First thread success
            func();
            atomic_store(&o->count,  2);
        }
        // Wait for function call (FIXME: priority inversion possible?)
        while (atomic_load(&o->count) != 2) { m_thread_yield(); }
    } // Already called. Nothing to do
}

// Attribute to use to allocate a global variable to a thread.
#define M_THREAD_ATTR __thread

M_END_PROTECTED_CODE

/******************************** INVALID VALUE **********************************/

#else
# error Value of M_USE_THREAD_BACKEND is incorrect. Please see the documentation for valid usage.
#endif

// TODO: Obsolete M_LOCK macro.

/* M_LOCK macro. Allow simple locking encapsulation.
   USAGE:
    static M_LOCK_DECL(name);
    int f(int n) {
      M_LOCK(name) {
        // Exclusive access
      }
    }
*/
/* NOTE: Either using direct support by the OS (WIN32/PTHREAD)
   or using C11's ONCE mechanism */
#ifdef M_MUTEXI_INIT_VALUE
# define M_LOCK_DECL(name) m_mutex_t name = M_MUTEXI_INIT_VALUE
# define M_LOCK(name)                                                         \
  M_LOCKI_DO(name, M_C(local_cont_, __LINE__), m_mutexi_lazy_lock, m_mutex_unlock)
#else
# define M_LOCK_DECL(name)                                                    \
  m_mutex_t name;                                                             \
  static void M_C(m_mutex_init_, name)(void) {                                \
    m_mutex_init(name);                                                       \
  }                                                                           \
  m_once_t M_C(m_once_, name) = M_ONCE_INIT_VALUE
# define M_LOCKI_BY_ONCE(name)                                                \
  (m_once_call(M_C(m_once_, name), M_C(m_mutex_init_, name)),                 \
   m_mutex_lock(name), (void) 0 )
# define M_LOCK(name)                                                         \
  M_LOCKI_DO(name, M_C(local_cont_, __LINE__), M_LOCKI_BY_ONCE, m_mutex_unlock)
#endif

#define M_LOCKI_DO(name, cont, lock_func, unlock_func)                        \
  for(bool cont = true                                                        \
        ; cont && (lock_func (name), true);                                   \
      (unlock_func (name), cont = false))

#endif
