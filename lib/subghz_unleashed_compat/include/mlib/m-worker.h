/*
 * M*LIB / WORKER - Extra worker interface
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
#ifndef MSTARLIB_WORKER_H
#define MSTARLIB_WORKER_H

/* The User Code can define M_USE_WORKER to 0 to disable the use of workers.
   The macros / functions are then defined to only use one core.
   By default, the behavior is to use workers.
*/
#ifndef M_USE_WORKER
# define M_USE_WORKER 1
#endif


#if M_USE_WORKER

#include "m-atomic.h"
#include "m-buffer.h"
#include "m-thread.h"

/* Include needed system header for detection of how many cores are available in the system */
#if defined(_WIN32)
# include <sysinfoapi.h>
#elif (defined(__APPLE__) && defined(__MACH__))                               \
  || defined(__DragonFly__) || defined(__FreeBSD__)                           \
  || defined(__NetBSD__) || defined(__OpenBSD__)
# include <sys/param.h>
# include <sys/sysctl.h>
# define M_USE_WORKER_SYSCTL 1
#else
# include <unistd.h>
#endif

/* Support for CLANG block since CLANG doesn't support nested function.
   M-WORKER uses its 'blocks' extension instead, but it is not compatible
   with function.
   So you need to compile with "-fblocks" and link with "-lBlocksRuntime"
   if you use clang & want to use the MACRO version.

   if C++, it will use Lambda function (and std::function) instead
   (It doesn't support pre-C++11 compiler).

   Otherwise go with nested function (GCC) for the MACRO version.

   This behavior can be overriden by User Code by defining to 1 or 0 the
   following macros:
   * M_USE_WORKER_CPP_FUNCTION
   * M_USE_WORKER_CLANG_BLOCK
*/

#if defined(__cplusplus) && !defined(M_USE_WORKER_CPP_FUNCTION)
# define M_USE_WORKER_CPP_FUNCTION 1
# include <functional>
#elif defined(__has_extension) && !defined(M_USE_WORKER_CLANG_BLOCK)
# if __has_extension(blocks)
#  define M_USE_WORKER_CLANG_BLOCK 1
# endif
#endif

#ifndef M_USE_WORKER_CLANG_BLOCK
# define M_USE_WORKER_CLANG_BLOCK 0
#endif
#ifndef M_USE_WORKER_CPP_FUNCTION
# define M_USE_WORKER_CPP_FUNCTION 0
#endif

/* Control that not both options are selected at the same time.
   Note: there are not really incompatible, but if we use C++ we shall go to
   lambda directly (there is no need to support blocks). */
#if M_USE_WORKER_CLANG_BLOCK && M_USE_WORKER_CPP_FUNCTION
# error M_USE_WORKER_CPP_FUNCTION and M_USE_WORKER_CLANG_BLOCK are both defined. This is not supported.
#endif

M_BEGIN_PROTECTED_CODE

/* Definition of a work order */
typedef struct m_work3r_order_s {
  struct m_worker_sync_s *block;  // Reference to the shared Synchronization block
  void * data;                    // The work order data
  void (*func) (void *data);      // The work order function (for GCC)
#if M_USE_WORKER_CLANG_BLOCK
  void (^blockFunc)(void *data);  // The work order function (block for clang)
#endif
#if M_USE_WORKER_CPP_FUNCTION
  std::function<void(void*)> function; // The work order function (for C++)
#endif
} m_work3r_order_ct;

/* Define the macros needed to initialize an order.
 * * MACRO to be used to send an empty order to stop the thread
 * * MACRO to complete the not-used fields
 */
#if M_USE_WORKER_CLANG_BLOCK || M_USE_WORKER_CPP_FUNCTION
# define M_WORK3R_EMPTY_ORDER { NULL, NULL, NULL, NULL }
# define M_WORK3R_EXTRA_ORDER , NULL
#else
# define M_WORK3R_EMPTY_ORDER { NULL, NULL, NULL }
# define M_WORK3R_EXTRA_ORDER
#endif

/* As it is C++, it uses std::function, M_POD_OPLIST
   is not sufficient for initialization of the structure.
   So let's use C++ constructor, destructor and copy constructor */
#if M_USE_WORKER_CPP_FUNCTION
# define M_WORK3R_CPP_INIT(x)        (new (&(x)) m_work3r_order_ct())
# define M_WORK3R_CPP_INIT_SET(x, y) (new (&(x)) m_work3r_order_ct(y))
# define M_WORK3R_CPP_SET(x, y)      ((x) = (y))
# define M_WORK3R_CPP_CLEAR(x)       ((&(x))->~m_work3r_order_ct())
# define M_WORK3R_CPP_INIT_MOVE(x,y) (new (&(x)) m_work3r_order_ct(y), ((&(y))->~m_work3r_order_ct()))
# define M_WORK3R_OPLIST                                                      \
      (INIT(M_WORK3R_CPP_INIT), INIT_SET(M_WORK3R_CPP_INIT_SET),              \
      SET(M_WORK3R_CPP_SET), CLEAR(M_WORK3R_CPP_CLEAR), INIT_MOVE(M_WORK3R_CPP_INIT_MOVE) )
#else
# define M_WORK3R_OPLIST M_POD_OPLIST
#endif

/* Definition of the identity of a worker thread */
typedef struct m_work3r_thread_s {
  m_thread_t id;
} m_work3r_thread_ct;

/* Definition of the queue that will record the work orders */
M_BUFFER_DEF(m_work3r_queue, m_work3r_order_ct, 0, M_BUFFER_QUEUE|M_BUFFER_DEFERRED_POP, M_WORK3R_OPLIST)

/* Definition the global pool of workers */
typedef struct m_worker_s {
  /* The work order queue */
  m_work3r_queue_t queue_g;

  /* The table of available workers */
  m_work3r_thread_ct *worker;

  /* Number of workers in the table */
  unsigned int numWorker_g;

  /* The global reset function */
  void (*resetFunc_g)(void);

  /* The global clear function */
  void (*clearFunc_g)(void);

  m_mutex_t lock;
  m_cond_t  a_thread_ends;        // EVENT: A worker has ended

} m_worker_t[1];

/* Definition of the synchronization point for workers */
typedef struct m_worker_sync_s {
  atomic_int num_spawn;                 // Number of spawned workers accord this synchronization point
  atomic_int num_terminated_spawn;      // Number of terminated spawned workers
  struct m_worker_s *worker;            // Reference to the pool of workers
} m_worker_sync_t[1];


/* Extend m_worker_spawn by defining a specialization function
   with the given arguments.
   Generate the needed encapsulation for the user.
   USAGE: name, oplists of arguments */
#define M_WORKER_SPAWN_DEF2(name, ...)                                        \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_WORK3R_SPAWN_EXTEND_P1( (name, M_MAP_C(M_WORK3R_SPAWN_EXTEND_P0, __VA_ARGS__) ) ) \
  M_END_PROTECTED_CODE

/* Output a valid oplist with the given type.
   input is (field-name, type) or (field-name, type, oplist)
   Output shall be : M_OPEXTEND(M_GLOBAL_OPLIST_OR_DEF(type_or_oplist)(), TYPE(type)) / M_OPEXTEND(oplist, TYPE(type))
 */
#define M_WORK3R_SPAWN_EXTEND_P0(...)                       M_BY_NARGS(M_WORK3R_SPAWN_EXTEND_P0, M_ID __VA_ARGS__) __VA_ARGS__
#define M_WORK3R_SPAWN_EXTEND_P0__2(field, type)            M_OPEXTEND(M_GLOBAL_OPLIST_OR_DEF(type)(), TYPE(type))
#define M_WORK3R_SPAWN_EXTEND_P0__3(field, type, oplist)    M_IF_OPLIST(oplist)(M_WORK3R_SPAWN_EXTEND_P0__3_OK, M_WORK3R_SPAWN_EXTEND_P0__3_KO)(field, type, oplist)
#define M_WORK3R_SPAWN_EXTEND_P0__3_OK(field, type, oplist) M_OPEXTEND(oplist, TYPE(type))
#define M_WORK3R_SPAWN_EXTEND_P0__3_KO(field, type, oplist)                   \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST, "(M_WORKER_SPAWN_DEF2): the argument is not a valid oplist: " M_MAP(M_AS_STR, oplist))

/* Deferred evaluation for the definition,
   so that all arguments are evaluated before further expansion */
#define M_WORK3R_SPAWN_EXTEND_P1(arg) M_ID( M_WORK3R_SPAWN_EXTEND_P2 arg )

/* Validate the oplist before going further */
#define M_WORK3R_SPAWN_EXTEND_P2(name, ...)                                   \
  M_IF(M_REDUCE(M_OPLIST_P, M_AND, __VA_ARGS__))                              \
  (M_WORK3R_SPAWN_EXTEND_P3, M_WORK3R_SPAWN_EXTEND_FAILURE)(name, __VA_ARGS__)

/* Stop processing with a compilation failure */
#define M_WORK3R_SPAWN_EXTEND_FAILURE(name, ...)                              \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST,                                       \
                   "(M_WORKER_SPAWN_DEF2): at least one of the given argument is not a valid oplist: " \
                   M_MAP(M_AS_STR, __VA_ARGS__))

/* Define the extension of spawn */
#define M_WORK3R_SPAWN_EXTEND_P3(name, ...)                                   \
  M_WORK3R_SPAWN_EXTEND_DEF_TYPE(name, __VA_ARGS__)                           \
  M_WORK3R_SPAWN_EXTEND_DEF_CALLBACK(name, __VA_ARGS__)                       \
  M_WORK3R_SPAWN_EXTEND_DEF_EMPLACE(name, __VA_ARGS__)                        \

/* Define the type */
#define M_WORK3R_SPAWN_EXTEND_DEF_TYPE(name, ...)                             \
  typedef void (*M_C3(m_worker_,name, _callback_ct))(M_MAP_C(M_WORK3R_SPAWN_EXTEND_DEF_TYPE_TYPE, __VA_ARGS__)); \
  struct M_C3(m_worker_, name, _s){                                           \
    M_C3(m_worker_, name, _callback_ct) callback;                             \
    M_MAP3(M_WORK3R_SPAWN_EXTEND_DEF_TYPE_FIELD, data, __VA_ARGS__)           \
      };

#define M_WORK3R_SPAWN_EXTEND_DEF_TYPE_FIELD(data, num, oplist)               \
  M_GET_TYPE oplist M_C(field, num);

#define M_WORK3R_SPAWN_EXTEND_DEF_TYPE_TYPE(oplist)                           \
  M_GET_TYPE oplist

/* Define the callback */
#define M_WORK3R_SPAWN_EXTEND_DEF_CALLBACK(name, ...)                         \
  M_INLINE void                                                               \
  M_C3(m_work3r_, name, _clear)(struct M_C3(m_worker_, name, _s) *p)          \
  {                                                                           \
    M_GLOBAL_CONTEXT();                                                       \
    M_MAP3(M_WORK3R_SPAWN_EXTEND_DEF_CALLBACK_CLEAR, data, __VA_ARGS__)       \
    /* TODO: Overload */                                                      \
    M_MEMORY_DEL(m_context, p);                                               \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_C3(m_work3r_, name, _callback)(void *data)                                \
  {                                                                           \
    struct M_C3(m_worker_, name, _s) *p = (struct M_C3(m_worker_, name, _s) *) data; \
    (*p->callback)(                                                           \
    M_MAP3_C(M_WORK3R_SPAWN_EXTEND_DEF_CALLBACK_FIELD, data, __VA_ARGS__)     \
                                                                        );    \
    M_C3(m_work3r_, name, _clear)(p);                                         \
  }

#define M_WORK3R_SPAWN_EXTEND_DEF_CALLBACK_FIELD(data, num, oplist)           \
  p->M_C(field, num)

#define M_WORK3R_SPAWN_EXTEND_DEF_CALLBACK_CLEAR(data, num, oplist)           \
  M_CALL_CLEAR(oplist, p->M_C(field, num)) ;

/* Define the emplace like spawn method */
#define M_WORK3R_SPAWN_EXTEND_DEF_EMPLACE(name, ...)                          \
  M_INLINE void                                                               \
  M_C(m_worker_spawn_, name)(m_worker_sync_t block, M_C3(m_worker_, name, _callback_ct) callback, \
                             M_MAP3_C(M_WORK3R_SPAWN_EXTEND_DEF_EMPLACE_FIELD, data, __VA_ARGS__) \
                             )                                                \
  {                                                                           \
    M_GLOBAL_CONTEXT();                                                       \
    if (!m_work3r_queue_full_p(block->worker->queue_g)) {                     \
      struct M_C3(m_worker_, name, _s) *p = M_MEMORY_ALLOC (m_context, struct M_C3(m_worker_, name, _s)); \
      if (M_UNLIKELY_NOMEM(p == NULL)) {                                      \
        M_MEMORY_FULL(struct M_C3(m_worker_, name, _s), 1);                   \
      }                                                                       \
      p->callback = callback;                                                 \
      M_MAP3(M_WORK3R_SPAWN_EXTEND_DEF_EMPLACE_FIELD_INIT_SET, data, __VA_ARGS__) \
      const m_work3r_order_ct w = { block, p, M_C3(m_work3r_, name, _callback) M_WORK3R_EXTRA_ORDER }; \
      if (m_work3r_queue_push_blocking (block->worker->queue_g, w, false) == true) { \
        atomic_fetch_add (&block->num_spawn, 1);                              \
        return;                                                               \
      }                                                                       \
      /* No worker available now. Call the function ourself */                \
      /* But before clear the allocated data */                               \
      M_C3(m_work3r_, name, _clear)(p);                                       \
    }                                                                         \
    /* No worker available. Call the function ourself */                      \
    (*callback) (                                                             \
                 M_MAP3_C(M_WORK3R_SPAWN_EXTEND_DEF_EMPLACE_FIELD_ALONE, data, __VA_ARGS__) \
                                                                        );    \
  }

#define M_WORK3R_SPAWN_EXTEND_DEF_EMPLACE_FIELD(data, num, oplist)            \
  M_GET_TYPE oplist M_C(param, num)

#define M_WORK3R_SPAWN_EXTEND_DEF_EMPLACE_FIELD_INIT_SET(data, num, oplist)   \
  M_CALL_INIT_SET(oplist, p-> M_C(field, num), M_C(param, num) );

#define M_WORK3R_SPAWN_EXTEND_DEF_EMPLACE_FIELD_ALONE(data, num, oplist)      \
  M_C(param, num)



/* Return the number of CPU cores available in the system.
   Works for WINDOWS, MACOS, *BSD, LINUX.
 */
M_INLINE int
m_work3r_get_cpu_count(void)
{
#if defined(_WIN32)
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  M_ASSERT(sysinfo.dwNumberOfProcessors <= INT_MAX);
  return (int) sysinfo.dwNumberOfProcessors;
#elif defined(M_USE_WORKER_SYSCTL)
  int nm[2];
  int count = 0;
  size_t len = sizeof (count);
  nm[0] = CTL_HW;
  nm[1] = HW_NCPU;
  sysctl(nm, 2, &count, &len, NULL, 0);
  return M_MAX(1, count);
#elif defined (_SC_NPROCESSORS_ONLN)
  return (int) sysconf(_SC_NPROCESSORS_ONLN);
#elif defined (_SC_NPROCESSORS_CONF)
  return (int) sysconf(_SC_NPROCESSORS_CONF);
#else
  return 1;
#endif
}

// (INTERNAL) Debug support for workers
#if 1
#define M_WORK3R_DEBUG(...) (void) 0
#else
#define M_WORK3R_DEBUG(...) printf(__VA_ARGS__)
#endif

/* Execute the registered work order **synchronously** */
M_INLINE void
m_work3r_exec(m_work3r_order_ct *w)
{
  M_ASSERT (w!= NULL && w->block != NULL);
  M_WORK3R_DEBUG ("Starting thread with data %p\n", w->data);
#if M_USE_WORKER_CLANG_BLOCK
  M_WORK3R_DEBUG ("Running %s f=%p b=%p\n", (w->func == NULL) ? "Blocks" : "Function", w->func, w->blockFunc);
  if (w->func == NULL)
    w->blockFunc(w->data);
  else
#endif
#if M_USE_WORKER_CPP_FUNCTION
    M_WORK3R_DEBUG ("Running %s f=%p b=%p\n", (w->function == NULL) ? "Lambda" : "Function", w->func, w->blockFunc);
    if (w->function)
      w->function(w->data);
    else
#endif
      w->func(w->data);
  /* Increment the number of terminated work order for the synchronous point */
  atomic_fetch_add (&w->block->num_terminated_spawn, 1);
}


/* The worker thread main loop*/
M_INLINE void
m_work3r_thread(void *arg)
{
  // Get back the given argument
  struct m_worker_s *g = M_ASSIGN_CAST(struct m_worker_s *, arg);
  M_GLOBAL_CONTEXT();
  while (true) {
    m_work3r_order_ct w;
    // If needed, reset the global state of the worker
    if (g->resetFunc_g != NULL) {
      g->resetFunc_g();
    }
    // Waiting for data
    M_WORK3R_DEBUG ("Waiting for data (queue: %lu / %lu)\n", m_work3r_queue_size(g->queue_g), m_work3r_queue_capacity(g->queue_g));
    m_work3r_queue_pop (&w, g->queue_g);
    // We received a work order 
    // Note: that the work order is still present in the queue
    // preventing further work order to be pushed in the queue until it finishes doing the work
    // If a stop request is received, terminate the thread 
    if (w.block == NULL) break;
    // Execute the work order
    m_work3r_exec(&w);
    // Consume fully the work order in the queue
    m_work3r_queue_pop_release(g->queue_g);
    // Signal that a worker has finished.
    m_mutex_lock(g->lock);
    m_cond_broadcast(g->a_thread_ends);
    m_mutex_unlock(g->lock);
  }
  // If needed, clear global state of the thread
  if (g->clearFunc_g != NULL) {
    g->clearFunc_g();
  }
}

/* Initialization of the worker module (constructor)
   Input:
   @numWorker: number of worker to create (0=autodetect, -1=2*autodetect)
   @extraQueue: number of extra work order we can get if all workers are full
   @resetFunc: function to reset the state of a worker between work orders (or NULL if none)
   @clearFunc: function to clear the state of a worker before terminating (or NULL if none)
*/
M_INLINE void
m_worker_init(m_worker_t g, int numWorker, unsigned int extraQueue, void (*resetFunc)(void), void (*clearFunc)(void))
{
  M_ASSERT (numWorker >= -1);
  // Auto compute number of workers if the argument is 0
  if (numWorker <= 0)
    numWorker = (1 + (numWorker == -1))*m_work3r_get_cpu_count()-1;
  M_WORK3R_DEBUG ("Starting queue with: %d\n", numWorker + extraQueue);
  // Initialization
  // numWorker can still be 0 if it is a single core cpu (no worker available)
  M_ASSERT(numWorker >= 0);
  size_t numWorker_st = (size_t) numWorker;
  M_GLOBAL_CONTEXT();
  g->worker = M_MEMORY_REALLOC(m_context, m_work3r_thread_ct, NULL, 0, numWorker_st);
  if (M_UNLIKELY_NOMEM (g->worker == NULL)) {
    M_MEMORY_FULL(m_work3r_thread_ct, numWorker_st);
  }
  m_work3r_queue_init (g->queue_g, numWorker_st + extraQueue);
  g->numWorker_g = (unsigned int) numWorker_st;
  g->resetFunc_g = resetFunc;
  g->clearFunc_g = clearFunc;
  m_mutex_init(g->lock);
  m_cond_init(g->a_thread_ends);
  
  // Create & start the workers
  for(size_t i = 0; i < numWorker_st; i++) {
    m_thread_create(g->worker[i].id, m_work3r_thread, M_ASSIGN_CAST(void*, g));
  }
}
/* Initialization of the worker module (constructor)
   Provide default values for the arguments.
   Input:
   @numWorker: number of worker to create (0=autodetect, -1=2*autodetect)
   @extraQueue: number of extra work order we can get if all workers are full
   @resetFunc: function to reset the state of a worker between work orders (optional)
   @clearFunc: function to clear the state of a worker before terminating (optional)
*/
#define m_worker_init(...) m_worker_init(M_DEFAULT_ARGS(5, (0, 0, NULL, NULL), __VA_ARGS__))

/* Clear of the worker module (destructor) */
M_INLINE void
m_worker_clear(m_worker_t g)
{
  M_ASSERT (m_work3r_queue_empty_p (g->queue_g));
  M_GLOBAL_CONTEXT();
  // Push the terminate order on the queue
  for(unsigned int i = 0; i < g->numWorker_g; i++) {
    m_work3r_order_ct w = M_WORK3R_EMPTY_ORDER;
    // Normally all worker threads shall be waiting at this
    // stage, so all push won't block as the queue is empty.
    // But for robustness, let's wait.
    m_work3r_queue_push_blocking (g->queue_g, w, true);
  }
  // Wait for thread termination
  for(unsigned int i = 0; i < g->numWorker_g; i++) {
    m_thread_join(g->worker[i].id);
  }
  // Clear memory
  M_MEMORY_FREE(m_context, m_work3r_thread_ct, g->worker, g->numWorker_g);
  m_mutex_clear(g->lock);
  m_cond_clear(g->a_thread_ends);
  m_work3r_queue_clear (g->queue_g);
}

/* Start a new collaboration between workers of pool 'g'
   by defining the synchronization point 'block' */
M_INLINE void
m_worker_start(m_worker_sync_t block, m_worker_t g)
{
  atomic_init (&block->num_spawn, 0);
  atomic_init (&block->num_terminated_spawn, 0);
  block->worker = g;
}

/* Spawn the given work order to workers if possible,
   or do it ourself if no worker is available.
   The synchronization point is defined a 'block'
   The work order if composed of the function 'func' and its 'data'
*/
M_INLINE void
m_worker_spawn(m_worker_sync_t block, void (*func)(void *data), void *data)
{
  const m_work3r_order_ct w = {  block, data, func M_WORK3R_EXTRA_ORDER };
  M_GLOBAL_CONTEXT();
  if (M_UNLIKELY (!m_work3r_queue_full_p(block->worker->queue_g))
      && m_work3r_queue_push_blocking (block->worker->queue_g, w, false) == true) {
    M_WORK3R_DEBUG ("Sending data to thread: %p (block: %d / %d)\n", data, block->num_spawn, block->num_terminated_spawn);
    atomic_fetch_add (&block->num_spawn, 1);
    return;
  }
  M_WORK3R_DEBUG ("Running data ourself: %p\n", data);
  /* No worker available. Call the function ourself */
  (*func) (data);
}

#if M_USE_WORKER_CLANG_BLOCK
/* Spawn or not the given work order to workers,
   or do it ourself if no worker is available */
M_INLINE void
m_work3r_spawn_block(m_worker_sync_t block, void (^func)(void *data), void *data)
{
  const m_work3r_order_ct w = {  block, data, NULL, func };
  M_GLOBAL_CONTEXT();
  if (M_UNLIKELY (!m_work3r_queue_full_p(block->worker->queue_g))
      && m_work3r_queue_push_blocking (block->worker->queue_g, w, false) == true) {
    M_WORK3R_DEBUG ("Sending data to thread as block: %p (block: %d / %d)\n", data, block->num_spawn, block->num_terminated_spawn);
    atomic_fetch_add (&block->num_spawn, 1);
    return;
  }
  M_WORK3R_DEBUG ("Running data ourself as block: %p\n", data);
  /* No worker available. Call the function ourself */
  func (data);
}
#endif

#if M_USE_WORKER_CPP_FUNCTION
/* Spawn or not the given work order to workers,
   or do it ourself if no worker is available */
M_INLINE void
m_work3r_spawn_function(m_worker_sync_t block, std::function<void(void *data)> func, void *data)
{
  const m_work3r_order_ct w = {  block, data, NULL, func };
  M_GLOBAL_CONTEXT();
  if (M_UNLIKELY (!m_work3r_queue_full_p(block->worker->queue_g))
      && m_work3r_queue_push_blocking (block->worker->queue_g, w, false) == true) {
    M_WORK3R_DEBUG ("Sending data to thread as block: %p (block: %d / %d)\n", data, block->num_spawn, block->num_terminated_spawn);
    atomic_fetch_add (&block->num_spawn, 1);
    return;
  }
  M_WORK3R_DEBUG ("Running data ourself as block: %p\n", data);
  /* No worker available. Call the function ourself */
  func (data);
}
#endif

/* Test if all work orders of the given synchronization point are finished */
M_INLINE bool
m_worker_sync_p(m_worker_sync_t block)
{
  /* If the number of spawns is greater than the number
     of terminated spawns, some spawns are still working.
     So wait for termination */
  return (atomic_load(&block->num_spawn) == atomic_load (&block->num_terminated_spawn));
}

/* Wait for all work orders of the given synchronization point to be finished */
M_INLINE void
m_worker_sync(m_worker_sync_t block)
{
  M_WORK3R_DEBUG ("Waiting for thread termination.\n");
  // Fast case: all workers have finished
  if (m_worker_sync_p(block)) return;
  // Slow case: perform a locked wait to put this thread to waiting state
  m_mutex_lock(block->worker->lock);
  while (!m_worker_sync_p(block)) {
    m_cond_wait(block->worker->a_thread_ends, block->worker->lock);
  }
  m_mutex_unlock(block->worker->lock);
}

/* Flush any work order in the queue ourself if some remains.*/
M_INLINE void
m_worker_flush(m_worker_t g)
{
  m_work3r_order_ct w;
  M_GLOBAL_CONTEXT();
  while (m_work3r_queue_pop_blocking (&w, g->queue_g, false) == true) {
    m_work3r_exec(&w);
    m_work3r_queue_pop_release(g->queue_g);
  }
}


/* Return the number of workers */
M_INLINE size_t
m_worker_count(m_worker_t g)
{
  return g->numWorker_g + 1;
}

/* Spawn the 'core' block computation into another thread if
   a worker thread is available. Compute it in the current thread otherwise.
   'block' shall be the initialised synchronized block for all threads.
   'input' is the list of input variables of the 'core' block within "( )"
   'output' is the list of output variables of the 'core' block within "( )"
   Output variables are only available after a synchronisation block.
  TODO: Support oplist for input & outputs parameters 
*/
#if M_USE_WORKER_CLANG_BLOCK
#define M_WORKER_SPAWN(_block, _input, _core, _output)                        \
  _Pragma("clang diagnostic push")                                            \
  _Pragma("clang diagnostic ignored \"-Wshadow\"")                            \
  M_WORK3R_DEF_DATA(_input, _output)                                          \
  M_WORK3R_DEF_SUBBLOCK(_input, _output, _core)                               \
  m_work3r_spawn_block ((_block), M_WORK3R_SPAWN_SUBFUNC_NAME,  &M_WORK3R_SPAWN_DATA_NAME); \
  _Pragma("clang diagnostic pop")
#elif M_USE_WORKER_CPP_FUNCTION
// TODO: Explicit pass all arguments by reference.
#define M_WORKER_SPAWN(_block, _input, _core, _output)                        \
  m_work3r_spawn_function ((_block), [&](void *param) {(void)param ; _core } ,  NULL)
#else
#define M_WORKER_SPAWN(_block, _input, _core, _output)                        \
  _Pragma("GCC diagnostic push")                                              \
  _Pragma("GCC diagnostic ignored \"-Wshadow\"")                              \
  M_WORK3R_DEF_DATA(_input, _output)                                          \
  M_WORK3R_DEF_SUBFUNC(_input, _output, _core)                                \
  m_worker_spawn ((_block), M_WORK3R_SPAWN_SUBFUNC_NAME,  &M_WORK3R_SPAWN_DATA_NAME); \
  _Pragma("GCC diagnostic pop")
#endif

#define M_WORK3R_SPAWN_STRUCT_NAME   M_C(m_work3r_data_s_, __LINE__)
#define M_WORK3R_SPAWN_DATA_NAME     M_C(m_work3r_data_, __LINE__)
#define M_WORK3R_SPAWN_SUBFUNC_NAME  M_C(m_work3r_subfunc_, __LINE__)
#define M_WORK3R_DEF_DATA(_input, _output)                                    \
  struct M_WORK3R_SPAWN_STRUCT_NAME {                                         \
    M_WORK3R_DEF_DATA_INPUT _input                                            \
    M_IF_EMPTY _output ( , M_WORK3R_DEF_DATA_OUTPUT _output)                  \
      } M_WORK3R_SPAWN_DATA_NAME = {                                          \
    M_WORK3R_INIT_DATA_INPUT _input                                           \
    M_IF_EMPTY _output (, M_WORK3R_INIT_DATA_OUTPUT _output)                  \
  };
#define M_WORK3R_DEF_SINGLE_INPUT(var) __typeof__(var) var;
#define M_WORK3R_DEF_DATA_INPUT(...)                                          \
  M_MAP(M_WORK3R_DEF_SINGLE_INPUT, __VA_ARGS__)
#define M_WORK3R_DEF_SINGLE_OUTPUT(var)                                       \
  __typeof__(var) *M_C(var, _ptr);
#define M_WORK3R_DEF_DATA_OUTPUT(...)                                         \
  M_MAP(M_WORK3R_DEF_SINGLE_OUTPUT, __VA_ARGS__)
#define M_WORK3R_INIT_SINGLE_INPUT(var)                                       \
  .var = var,
#define M_WORK3R_INIT_DATA_INPUT(...)                                         \
  M_MAP(M_WORK3R_INIT_SINGLE_INPUT, __VA_ARGS__)
#define M_WORK3R_INIT_SINGLE_OUTPUT(var)                                      \
  .M_C(var, _ptr) = &var,
#define M_WORK3R_INIT_DATA_OUTPUT(...)                                        \
  M_MAP(M_WORK3R_INIT_SINGLE_OUTPUT, __VA_ARGS__)
#define M_WORK3R_DEF_SUBFUNC(_input, _output, _core)                          \
  __extension__ auto void M_WORK3R_SPAWN_SUBFUNC_NAME(void *) ;               \
  __extension__ void M_WORK3R_SPAWN_SUBFUNC_NAME(void *_data)                 \
  {                                                                           \
    struct M_WORK3R_SPAWN_STRUCT_NAME *_s_data = _data ;                      \
    M_WORK3R_INIT_LOCAL_INPUT _input                                          \
      M_IF_EMPTY _output ( , M_WORK3R_INIT_LOCAL_OUTPUT _output)              \
      do { _core } while (0);                                                 \
    M_IF_EMPTY _output ( , M_WORK3R_PROPAGATE_LOCAL_OUTPUT _output)           \
  };
#define M_WORK3R_DEF_SUBBLOCK(_input, _output, _core)                         \
  void (^M_WORK3R_SPAWN_SUBFUNC_NAME) (void *) = ^ void (void * _data)        \
  {                                                                           \
    struct M_WORK3R_SPAWN_STRUCT_NAME *_s_data = _data ;                      \
    M_WORK3R_INIT_LOCAL_INPUT _input                                          \
      M_IF_EMPTY _output ( , M_WORK3R_INIT_LOCAL_OUTPUT _output)              \
      do { _core } while (0);                                                 \
    M_IF_EMPTY _output ( , M_WORK3R_PROPAGATE_LOCAL_OUTPUT _output)           \
  };
#define M_WORK3R_INIT_SINGLE_LOCAL_INPUT(var)                                 \
  __typeof__(var) var = _s_data->var;
#define M_WORK3R_INIT_LOCAL_INPUT(...)                                        \
  M_MAP(M_WORK3R_INIT_SINGLE_LOCAL_INPUT, __VA_ARGS__)
#define M_WORK3R_INIT_SINGLE_LOCAL_OUTPUT(var)                                \
  __typeof__(var) var;
#define M_WORK3R_INIT_LOCAL_OUTPUT(...)                                       \
  M_MAP(M_WORK3R_INIT_SINGLE_LOCAL_OUTPUT, __VA_ARGS__)
#define M_WORK3R_PROPAGATE_SINGLE_OUTPUT(var)                                 \
  *(_s_data->M_C(var, _ptr)) = var;
#define M_WORK3R_PROPAGATE_LOCAL_OUTPUT(...)                                  \
  M_MAP(M_WORK3R_PROPAGATE_SINGLE_OUTPUT, __VA_ARGS__)

M_END_PROTECTED_CODE

#else /* M_USE_WORKER */

/*   Define empty types and empty functions to not use any worker */
#include "m-core.h"

typedef struct m_worker_block_s {
  int x;
} m_worker_sync_t[1];

typedef struct m_worker_s {
  int x;
} m_worker_t[1];

#define m_worker_init(g, numWorker, extraQueue, resetFunc) do { (void) g; } while (0)
#define m_worker_clear(g) do { (void) g; } while (0)
#define m_worker_start(b, w) do { (void) b; } while (0)
#define m_worker_spawn(b, f, d) do { f(d); } while (0)
#define m_worker_sync_p(b) true
#define m_worker_sync(b) do { (void) b; } while (0)
#define m_worker_count(w) 1
#define m_worker_flush(w) do { (void) w; } while (0)
#define M_WORKER_SPAWN(b, i, c, o) do { c } while (0)

// TODO: M_WORKER_SPAWN_DEF2

#endif /* M_USE_WORKER */


#if M_USE_SMALL_NAME
#define worker_t      m_worker_t
#define worker_sync_t m_worker_sync_t
#define worker_init   m_worker_init
#define worker_clear  m_worker_clear
#define worker_start  m_worker_start
#define worker_spawn  m_worker_spawn
#define worker_sync_p m_worker_sync_p
#define worker_sync   m_worker_sync
#define worker_count  m_worker_count
#define worker_flush  m_worker_flush
#define WORKER_SPAWN  M_WORKER_SPAWN
#endif

#endif
