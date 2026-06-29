/*
 * M*LIB - SNAPSHOT Module
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
#ifndef MSTARLIB_SNAPSHOT_H
#define MSTARLIB_SNAPSHOT_H

#include "m-atomic.h"
#include "m-core.h"
#include "m-genint.h"

M_BEGIN_PROTECTED_CODE

/* Define a Single Producer Single Consumer snapshot and its functions
   USAGE: SNAPSHOT_SPSC_DEF(name, type[, oplist]) */
#define M_SNAPSHOT_SPSC_DEF(name, ...)                                        \
  M_SNAPSHOT_SPSC_DEF_AS(name, M_F(name,_t), __VA_ARGS__)


/* Define a Single Producer Single Consumer snapshot and its functions
   as the given name name_t
   USAGE: SNAPSHOT_SPSC_DEF_AS(name, name_t, type[, oplist]) */
#define M_SNAPSHOT_SPSC_DEF_AS(name, name_t, ...)                             \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_SNAPSH0T_SPSC_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                          \
                        ((name, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), name_t ), \
                         (name, __VA_ARGS__                                       , name_t ))) \
  M_END_PROTECTED_CODE


/* Define a Single Producer Multiple Consumer snapshot and its functions
   USAGE: SNAPSHOT_SPMC_DEF(name, type[, oplist]) */
#define M_SNAPSHOT_SPMC_DEF(name, ...)                                        \
  M_SNAPSHOT_SPMC_DEF_AS(name, M_F(name,_t), __VA_ARGS__)


/* Define a Single Producer Multiple Consumer snapshot and its functions
   as the given name name_t
   USAGE: SNAPSHOT_SPMC_DEF_AS(name, type[, oplist]) */
#define M_SNAPSHOT_SPMC_DEF_AS(name, name_t, ...)                             \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_SNAPSH0T_SPMC_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                          \
                        ((name, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), name_t ), \
                         (name, __VA_ARGS__                                       , name_t ))) \
  M_END_PROTECTED_CODE


/* Define a Multiple Producer Multiple Consumer snapshot and its functions
   USAGE: SNAPSHOT_MPMC_DEF(name, type[, oplist]) */
#define M_SNAPSHOT_MPMC_DEF(name, ...)                                        \
  M_SNAPSHOT_MPMC_DEF_AS(name, M_F(name,_t), __VA_ARGS__)


/* Define a Multiple Producer Multiple Consumer snapshot and its functions
   as the given name name_t
   USAGE: SNAPSHOT_MPMC_DEF_AS(name, name_t, type[, oplist]) */
#define M_SNAPSHOT_MPMC_DEF_AS(name, name_t, ...)                             \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_SNAPSH0T_MPMC_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                          \
                        ((name, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), name_t ), \
                         (name, __VA_ARGS__                                       , name_t ))) \
  M_END_PROTECTED_CODE


/* Define the oplist of a snapshot (SPSC, SPMC or MPMC).
   USAGE: SNAPSHOT_OPLIST(name[, oplist]) */
#define M_SNAPSHOT_OPLIST(...)                                                \
  M_SNAPSH0T_OPLIST_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                            \
                      ((__VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)() ), \
                       (__VA_ARGS__ )))


/*****************************************************************************/
/********************************** INTERNAL *********************************/
/*****************************************************************************/

// deferred evaluation of the input
#define M_SNAPSH0T_OPLIST_P1(arg) M_SNAPSH0T_OPLIST_P2 arg

/* Validation of the given oplist */
#define M_SNAPSH0T_OPLIST_P2(name, oplist)                                    \
  M_IF_OPLIST(oplist)(M_SNAPSH0T_OPLIST_P3, M_SNAPSH0T_OPLIST_FAILURE)(name, oplist)

/* Prepare a clean compilation failure */
#define M_SNAPSH0T_OPLIST_FAILURE(name, oplist)                               \
  ((M_LIB_ERROR(ARGUMENT_OF_SNAPSHOT_OPLIST_IS_NOT_AN_OPLIST, name, oplist)))

/* Define the oplist of a snapshot */
#define M_SNAPSH0T_OPLIST_P3(name, oplist)                                    \
  (INIT(M_F(name, _init))                                                     \
   ,INIT_SET(M_F(name, _init_set))                                            \
   ,SET(M_F(name, _set))                                                      \
   ,CLEAR(M_F(name, _clear))                                                  \
   ,NAME(name)                                                                \
   ,TYPE(M_F(name, _ct)) , GENTYPE(struct M_F(name,_s)*)                      \
   ,SUBTYPE(M_F(name, _subtype_ct))                                           \
   ,OPLIST(oplist)                                                            \
   ,M_IF_METHOD(INIT_MOVE, oplist)(INIT_MOVE(M_F(name, _init_move)),)         \
   ,M_IF_METHOD(MOVE, oplist)(MOVE(M_F(name, _move)),)                        \
   )


/********************************** INTERNAL *********************************/

/* Flag defining the atomic state of a snapshot:
 * - r: Index of the read buffer Range [0..2]
 * - w: Index of the write buffer  Range [0..2]
 * - f: Next index of the write buffer when a shot is taken Range [0..2]
 * - b: Boolean indicating that the read buffer shall be updated
 * all fields packed in an unsigned char type.
 */
#define M_SNAPSH0T_SPSC_FLAG(r, w, f, b)                                      \
  ((unsigned char)( ( (r) << 4) | ((w) << 2) | ((f)) | ((b) << 6)))
#define M_SNAPSH0T_SPSC_R(flags)                                              \
  (((unsigned int) (flags) >> 4) & 0x03u)
#define M_SNAPSH0T_SPSC_W(flags)                                              \
  (((unsigned int) (flags) >> 2) & 0x03u)
#define M_SNAPSH0T_SPSC_F(flags)                                              \
  (((unsigned int) (flags) >> 0) & 0x03u)
#define M_SNAPSH0T_SPSC_B(flags)                                              \
  (((unsigned int) (flags) >> 6) & 0x01u)

/* NOTE: Due to atomic_load only accepting non-const pointer,
   we can't have any const in the interface. */
#define M_SNAPSH0T_SPSC_FLAGS_CONTRACT(flags)                                 \
  M_ASSERT(M_SNAPSH0T_SPSC_R(flags) != M_SNAPSH0T_SPSC_W(flags)               \
         && M_SNAPSH0T_SPSC_R(flags) != M_SNAPSH0T_SPSC_F(flags)              \
         && M_SNAPSH0T_SPSC_W(flags) != M_SNAPSH0T_SPSC_F(flags))

#define M_SNAPSH0T_SPSC_CONTRACT(snap)        do {                            \
    M_ASSERT((snap) != NULL);                                                 \
    unsigned char f = atomic_load (&(snap)->flags);                           \
    (void) f;                                                                 \
    M_SNAPSH0T_SPSC_FLAGS_CONTRACT(f);                                        \
  } while (0)

// A snapshot is basically an atomic triple buffer (Lock Free)
// between a single producer thread and a single Consumer thread.
#define M_SNAPSH0T_SPSC_MAX_BUFFER             3

// Deferred evaluation of the arguments.
#define M_SNAPSH0T_SPSC_DEF_P1(arg)        M_ID( M_SNAPSH0T_SPSC_DEF_P2 arg )

/* Validate the oplist before going further */
#define M_SNAPSH0T_SPSC_DEF_P2(name, type, oplist, snapshot_t)                \
  M_IF_OPLIST(oplist)(M_SNAPSH0T_SPSC_DEF_P3, M_SNAPSH0T_SPSC_DEF_FAILURE)(name, type, oplist, snapshot_t)

/* Stop processing with a compilation failure */
#define M_SNAPSH0T_SPSC_DEF_FAILURE(name, type, oplist, snapshot_t)           \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST, "(SNAPSHOT_SPSC_DEF): the given argument is not a valid oplist: " #oplist)

/* Expand the type and the functions of a SPSC snapshot */
#define M_SNAPSH0T_SPSC_DEF_P3(name, type, oplist, snapshot_t)                \
  M_SNAPSH0T_SPSC_DEF_TYPE(name, type, oplist, snapshot_t)                    \
  M_CHECK_COMPATIBLE_OPLIST(name, 1, type, oplist)                            \
  M_SNAPSH0T_SPSC_DEF_CORE(name, type, oplist, snapshot_t)                    \

/* Define the type */
#define M_SNAPSH0T_SPSC_DEF_TYPE(name, type, oplist, snapshot_t)              \
                                                                              \
  /* Create an aligned type to avoid false sharing between threads */         \
  typedef struct M_F(name, _aligned_type_s) {                                 \
    type         x;                                                           \
    M_CACHELINE_ALIGN(align, type);                                           \
  } M_F(name, _aligned_type_ct);                                              \
                                                                              \
  typedef struct M_F(name, _s) {                                              \
    M_F(name, _aligned_type_ct)  data[M_SNAPSH0T_SPSC_MAX_BUFFER];            \
    atomic_uchar flags;                                                       \
  } snapshot_t[1];                                                            \
  typedef struct M_F(name, _s) *M_F(name, _ptr);                              \
  typedef const struct M_F(name, _s) *M_F(name, _srcptr);                     \
                                                                              \
  /* Define internal types for oplist */                                      \
  typedef snapshot_t M_F(name, _ct);                                          \
  typedef type          M_F(name, _subtype_ct);                               \

/* Define the core functions */
#define M_SNAPSH0T_SPSC_DEF_CORE(name, type, oplist, snapshot_t)              \
                                                                              \
  M_P(void, name, _init, snapshot_t snap)                                     \
  {                                                                           \
    M_ASSERT(snap != NULL);                                                   \
    M_UNUSED_CONTEXT();                                                       \
    for(int i = 0; i < M_SNAPSH0T_SPSC_MAX_BUFFER; i++) {                     \
      M_CALL_INIT(oplist, snap->data[i].x);                                   \
    }                                                                         \
    atomic_init (&snap->flags, M_SNAPSH0T_SPSC_FLAG(0, 1, 2, 0));             \
    M_SNAPSH0T_SPSC_CONTRACT(snap);                                           \
  }                                                                           \
                                                                              \
  M_P(void, name, _clear, snapshot_t snap)                                    \
  {                                                                           \
    M_SNAPSH0T_SPSC_CONTRACT(snap);                                           \
    M_UNUSED_CONTEXT();                                                       \
    for(int i = 0; i < M_SNAPSH0T_SPSC_MAX_BUFFER; i++) {                     \
      M_CALL_CLEAR(oplist, snap->data[i].x);                                  \
    }                                                                         \
  }                                                                           \
                                                                              \
  /* const is missing for org due to use of atomic_load of org */             \
  M_P(void, name, _init_set, snapshot_t snap, snapshot_t org)                 \
  {                                                                           \
    M_SNAPSH0T_SPSC_CONTRACT(org);                                            \
    M_UNUSED_CONTEXT();                                                       \
    M_ASSERT(snap != NULL && snap != org);                                    \
    for(int i = 0; i < M_SNAPSH0T_SPSC_MAX_BUFFER; i++) {                     \
      M_CALL_INIT_SET(oplist, snap->data[i].x, org->data[i].x);               \
    }                                                                         \
    atomic_init (&snap->flags, atomic_load(&org->flags));                     \
    M_SNAPSH0T_SPSC_CONTRACT(snap);                                           \
  }                                                                           \
                                                                              \
  /* const is missing for org due to use of atomic_load of org */             \
  M_P(void, name, _set, snapshot_t snap, snapshot_t org)                      \
  {                                                                           \
    M_SNAPSH0T_SPSC_CONTRACT(snap);                                           \
    M_SNAPSH0T_SPSC_CONTRACT(org);                                            \
    M_UNUSED_CONTEXT();                                                       \
    for(int i = 0; i < M_SNAPSH0T_SPSC_MAX_BUFFER; i++) {                     \
      M_CALL_SET(oplist, snap->data[i].x, org->data[i].x);                    \
    }                                                                         \
    atomic_init (&snap->flags, atomic_load(&org->flags));                     \
    M_SNAPSH0T_SPSC_CONTRACT(snap);                                           \
  }                                                                           \
                                                                              \
  M_IF_METHOD(INIT_MOVE, oplist)(                                             \
    M_INLINE void                                                             \
    M_F(name, _init_move)(snapshot_t snap, snapshot_t org)                    \
    {                                                                         \
      M_SNAPSH0T_SPSC_CONTRACT(org);                                          \
      M_ASSERT(snap != NULL && snap != org);                                  \
      for(int i = 0; i < M_SNAPSH0T_SPSC_MAX_BUFFER; i++) {                   \
        M_CALL_INIT_MOVE(oplist, snap->data[i].x, org->data[i].x);            \
      }                                                                       \
      atomic_store (&snap->flags, atomic_load(&org->flags));                  \
      atomic_store (&org->flags, M_SNAPSH0T_SPSC_FLAG(0,0,0,0) );             \
      M_SNAPSH0T_SPSC_CONTRACT(snap);                                         \
    }                                                                         \
  ,) /* IF_METHOD (INIT_MOVE) */                                              \
                                                                              \
  M_IF_METHOD(MOVE, oplist)(                                                  \
    M_P(void, name, _move, snapshot_t snap, snapshot_t org)                   \
    {                                                                         \
      M_SNAPSH0T_SPSC_CONTRACT(snap);                                         \
      M_SNAPSH0T_SPSC_CONTRACT(org);                                          \
      M_UNUSED_CONTEXT();                                                     \
      M_ASSERT(snap != org);                                                  \
      for(int i = 0; i < M_SNAPSH0T_SPSC_MAX_BUFFER; i++) {                   \
        M_CALL_MOVE(oplist, snap->data[i].x, org->data[i].x);                 \
      }                                                                       \
      atomic_store (&snap->flags, atomic_load(&org->flags));                  \
      atomic_store (&org->flags, M_SNAPSH0T_SPSC_FLAG(0,0,0,0) );             \
      M_SNAPSH0T_SPSC_CONTRACT(snap);                                         \
    }                                                                         \
  ,) /* IF_METHOD (MOVE) */                                                   \
                                                                              \
  M_INLINE type *                                                             \
  M_F(name, _write)(snapshot_t snap)                                          \
  {                                                                           \
    M_SNAPSH0T_SPSC_CONTRACT(snap);                                           \
    unsigned char nextFlags, origFlags = atomic_load (&snap->flags);          \
    /* Atomic CAS operation */                                                \
    do {                                                                      \
      /* Swap F and W buffer, setting exchange flag */                        \
      nextFlags = M_SNAPSH0T_SPSC_FLAG(M_SNAPSH0T_SPSC_R(origFlags),          \
                                      M_SNAPSH0T_SPSC_F(origFlags),           \
                                      M_SNAPSH0T_SPSC_W(origFlags), 1);       \
      /* exponential backoff is not needed as there can't be more             \
         than 2 threads which try to update the data. */                      \
    } while (!atomic_compare_exchange_weak (&snap->flags, &origFlags,         \
                                            nextFlags));                      \
    /* Return new write buffer for new updating */                            \
    return &snap->data[M_SNAPSH0T_SPSC_W(nextFlags)].x;                       \
  }                                                                           \
                                                                              \
  M_INLINE type const *                                                       \
  M_F(name, _read)(snapshot_t snap)                                           \
  {                                                                           \
    M_SNAPSH0T_SPSC_CONTRACT(snap);                                           \
    unsigned char nextFlags, origFlags = atomic_load (&snap->flags);          \
    /* Atomic CAS operation */                                                \
    do {                                                                      \
      /* If no exchange registered, do nothing and keep the same */           \
      if (!M_SNAPSH0T_SPSC_B(origFlags)) {                                    \
        nextFlags = origFlags;                                                \
        break;                                                                \
      }                                                                       \
      /* Swap R and F buffer, clearing exchange flag */                       \
      nextFlags = M_SNAPSH0T_SPSC_FLAG(M_SNAPSH0T_SPSC_F(origFlags),          \
                                      M_SNAPSH0T_SPSC_W(origFlags),           \
                                      M_SNAPSH0T_SPSC_R(origFlags), 0);       \
      /* exponential backoff is not needed as there can't be more             \
         than 2 threads which try to update the data. */                      \
    } while (!atomic_compare_exchange_weak (&snap->flags, &origFlags,         \
                                            nextFlags));                      \
    /* Return current read buffer */                                          \
    return M_CONST_CAST(type, &snap->data[M_SNAPSH0T_SPSC_R(nextFlags)].x);   \
  }                                                                           \
                                                                              \
  /* Non const due to use of atomic_load */                                   \
  M_INLINE bool                                                               \
  M_F(name, _updated_p)(snapshot_t snap)                                      \
  {                                                                           \
    M_SNAPSH0T_SPSC_CONTRACT(snap);                                           \
    unsigned char flags = atomic_load (&snap->flags);                         \
    return M_SNAPSH0T_SPSC_B(flags);                                          \
  }                                                                           \
                                                                              \
  /* Non const due to use of atomic_load */                                   \
  M_INLINE type *                                                             \
  M_F(name, _get_write_buffer)(snapshot_t snap)                               \
  {                                                                           \
    M_SNAPSH0T_SPSC_CONTRACT(snap);                                           \
    unsigned char flags = atomic_load(&snap->flags);                          \
    return &snap->data[M_SNAPSH0T_SPSC_W(flags)].x;                           \
  }                                                                           \
                                                                              \
  /* Non const due to use of atomic_load */                                   \
  M_INLINE type const *                                                       \
  M_F(name, _get_read_buffer)(snapshot_t snap)                                \
  {                                                                           \
    M_SNAPSH0T_SPSC_CONTRACT(snap);                                           \
    unsigned char flags = atomic_load(&snap->flags);                          \
    return  M_CONST_CAST(type, &snap->data[M_SNAPSH0T_SPSC_R(flags)].x);      \
  }                                                                           \
  

/********************************** INTERNAL *********************************/

#define M_SNAPSH0T_SPMC_INT_FLAG(w, n) ( ((w) << 1) | (n) )
#define M_SNAPSH0T_SPMC_INT_FLAG_W(f)  ((f) >> 1)
#define M_SNAPSH0T_SPMC_INT_FLAG_N(f)  ((f) & 1)

// 2 more buffer than the number of readers are needed
#define M_SNAPSH0T_SPMC_EXTRA_BUFFER 2

#define M_SNAPSH0T_SPMC_MAX_READER (M_GENINT_MAX_ALLOC-M_SNAPSH0T_SPMC_EXTRA_BUFFER)

/* Internal structure to handle SPMC snapshot but return an unique index in the buffer array.
   - lastNext: last published written index + next flag (format M_SNAPSH0T_SPMC_INT_FLAG)
   - currentWrite: the index being currently written.
   - n_reader : number of readers 
   - cptTab: ref counter array to keep track of how many readers use the corresponding buffer.
   - freeList: a pool of free integers.
*/
typedef struct m_snapsh0t_mrsw_s {
  atomic_uint  lastNext;
  unsigned int currentWrite;
  size_t       n_reader;
  atomic_uint *cptTab;
  m_genint_t     freeList;
} m_snapsh0t_mrsw_ct[1];

// can't check currentWrite due to potential data race on it
#define M_SNAPSH0T_SPMC_INT_CONTRACT(s) do {                                  \
    M_ASSERT (s != NULL);                                                     \
    M_ASSERT (s->n_reader > 0 && s->n_reader <= M_SNAPSH0T_SPMC_MAX_READER);  \
    M_ASSERT ((size_t)M_SNAPSH0T_SPMC_INT_FLAG_W(atomic_load(&s->lastNext))   \
            <= s->n_reader + M_SNAPSH0T_SPMC_EXTRA_BUFFER);                   \
    M_ASSERT (s->cptTab != NULL);                                             \
  } while (0)

/* Initialize m_snapsh0t_mrsw_ct for n readers (constructor) */
M_P(void, m_snapsh0t, _mrsw_init, m_snapsh0t_mrsw_ct s, size_t n)
{
  M_ASSERT (s != NULL);
  M_ASSERT (n >= 1 && n <= M_SNAPSH0T_SPMC_MAX_READER);
  s->n_reader = n;
  n += M_SNAPSH0T_SPMC_EXTRA_BUFFER;

  // Initialize the counters to zero (no reader use it)
  atomic_uint *ptr = M_MEMORY_REALLOC (m_context, atomic_uint, NULL, 0, n);
  if (M_UNLIKELY_NOMEM (ptr == NULL)) {
    M_MEMORY_FULL(atomic_uint, n);
  }
  s->cptTab = ptr;
  for(size_t i = 0; i < n; i++)
    atomic_init(&s->cptTab[i], 0U);
  m_genint_init M_R(s->freeList, (unsigned int) n);

  // Get a free buffer and set it as available for readers
  unsigned int w = m_genint_pop(s->freeList);
  M_ASSERT (w != M_GENINT_ERROR);
  atomic_store(&s->cptTab[w], 1U);
  atomic_init(&s->lastNext, M_SNAPSH0T_SPMC_INT_FLAG(w, true));

  // Get working buffer
  s->currentWrite = m_genint_pop(s->freeList);
  M_ASSERT (s->currentWrite != M_GENINT_ERROR);
  atomic_store(&s->cptTab[s->currentWrite], 1U);
  M_SNAPSH0T_SPMC_INT_CONTRACT(s);
}

/* Clear m_snapsh0t_mrsw_ct (destructor) */
M_P(void, m_snapsh0t, _mrsw_clear, m_snapsh0t_mrsw_ct s)
{
  M_SNAPSH0T_SPMC_INT_CONTRACT(s);
  M_MEMORY_FREE (m_context, atomic_uint, s->cptTab, s->n_reader+M_SNAPSH0T_SPMC_EXTRA_BUFFER);
  m_genint_clear M_R(s->freeList);
  s->cptTab = NULL;
  s->n_reader = 0;
}

/* Return the current index that is written in the buffer */
M_INLINE unsigned int
m_snapsh0t_mrsw_get_write_idx(m_snapsh0t_mrsw_ct s)
{
  M_SNAPSH0T_SPMC_INT_CONTRACT(s);
  return s->currentWrite;
}

/* Return the number of readers */
M_INLINE unsigned int
m_snapsh0t_mrsw_size(m_snapsh0t_mrsw_ct s)
{
  M_SNAPSH0T_SPMC_INT_CONTRACT(s);
  return (unsigned int) s->n_reader;
}

/* Give the current index that is written to the readers,
   and return new available index for the writer thread */
M_INLINE unsigned int
m_snapsh0t_mrsw_write_idx(m_snapsh0t_mrsw_ct s, unsigned int idx)
{
  M_SNAPSH0T_SPMC_INT_CONTRACT(s);

  // Provide the finalized written buffer to the readers.
  unsigned int newNext, previous = atomic_load(&s->lastNext);
  do {
    newNext = M_SNAPSH0T_SPMC_INT_FLAG(idx, true);
  } while (!atomic_compare_exchange_weak(&s->lastNext, &previous, newNext));

  if (M_SNAPSH0T_SPMC_INT_FLAG_N(previous)) {
    // Reuse previous buffer as it was not used by any reader
    idx = M_SNAPSH0T_SPMC_INT_FLAG_W(previous);
    // Some other read threads may already have try to reserve this index
    // So atomic_load(&s->cptTab[idx]) can be greater than 1.
    // However they will fail to ack it in lastNext,
    // so they will remove their reservation later
  } else {
    // Remove the writer thread counter from the count of the previous buffer
    idx = M_SNAPSH0T_SPMC_INT_FLAG_W(previous);
    unsigned int c = atomic_fetch_sub(&s->cptTab[idx], 1U);
    M_ASSERT (c != 0 && c <= s->n_reader + 1);
    // Get a new buffer.
    if (c != 1) {
      // If someone else keeps a ref on the buffer, we can't reuse it
      // get another free one.
      idx = m_genint_pop(s->freeList);
      M_ASSERT(idx != M_GENINT_ERROR);
    } else {
      // No other thread keep track of this buffer.
      // Reuse it.
    }
    M_ASSERT (idx < s->n_reader + M_SNAPSH0T_SPMC_EXTRA_BUFFER);
    M_ASSERT (atomic_load(&s->cptTab[idx]) == 0);
    atomic_store(&s->cptTab[idx], 1U);
  }
  M_SNAPSH0T_SPMC_INT_CONTRACT(s);
  return idx;
}

/* Perform a swap of the current write buffer and return a new one */
M_INLINE unsigned int
m_snapsh0t_mrsw_write(m_snapsh0t_mrsw_ct s)
{
  s->currentWrite = m_snapsh0t_mrsw_write_idx(s, s->currentWrite);
  M_SNAPSH0T_SPMC_INT_CONTRACT(s);
  return s->currentWrite;
}

/* Start writing to the write buffer and return its index */
M_INLINE unsigned int
m_snapsh0t_mrsw_write_start(m_snapsh0t_mrsw_ct s)
{
  M_SNAPSH0T_SPMC_INT_CONTRACT(s);
  // Get a new buffer.
  unsigned int idx = m_genint_pop(s->freeList);
  M_ASSERT (idx != M_GENINT_ERROR);
  M_ASSERT (idx < s->n_reader + M_SNAPSH0T_SPMC_EXTRA_BUFFER);
  M_ASSERT (atomic_load(&s->cptTab[idx]) == 0);
  atomic_store(&s->cptTab[idx], 1U);
  M_SNAPSH0T_SPMC_INT_CONTRACT(s);
  return idx;
}

/* End writing to the given write buffer */
M_INLINE void
m_snapsh0t_mrsw_write_end(m_snapsh0t_mrsw_ct s, unsigned int idx)
{
  M_SNAPSH0T_SPMC_INT_CONTRACT(s);

  // Provide this write buffer to the readers
  unsigned int newNext, previous = atomic_load(&s->lastNext);
  do {
    newNext = M_SNAPSH0T_SPMC_INT_FLAG(idx, true);
  } while (!atomic_compare_exchange_weak(&s->lastNext, &previous, newNext));

  // Free the previous write buffer
  idx = M_SNAPSH0T_SPMC_INT_FLAG_W(previous);
  unsigned int c = atomic_fetch_sub(&s->cptTab[idx], 1U);
  M_ASSERT (c != 0 && c <= s->n_reader + 1);
  if (c == 1) {
    m_genint_push(s->freeList, idx);
  }
  M_SNAPSH0T_SPMC_INT_CONTRACT(s);
}

/* Start reading the latest written buffer and return the index to it */
M_INLINE unsigned int
m_snapsh0t_mrsw_read_start(m_snapsh0t_mrsw_ct s)
{
  M_SNAPSH0T_SPMC_INT_CONTRACT(s);
  unsigned int idx, previous;
 reload:
  // Load the last published index + Next flag
  previous = atomic_load(&s->lastNext);
  while (true) {
    // Get the last published index
    idx = M_SNAPSH0T_SPMC_INT_FLAG_W(previous);
    // Load the number of threads using this index
    unsigned int c = atomic_load(&s->cptTab[idx]);
    M_ASSERT (c <= s->n_reader + 1);
    // Reserve the index if it still being reserved by someone else
    if (M_UNLIKELY (c == 0
                    || !atomic_compare_exchange_strong(&s->cptTab[idx], &c, c+1)))
      goto reload;
    // Try to ack it
    unsigned int newNext = M_SNAPSH0T_SPMC_INT_FLAG(idx, false);
  reforce:
    if (M_LIKELY (atomic_compare_exchange_strong(&s->lastNext, &previous, newNext)))
      break;
    // We have been preempted by another thread
    if (idx == M_SNAPSH0T_SPMC_INT_FLAG_W(previous)) {
      // This is still ok if the index has not changed
      // We can get previous to true again if the writer has recycled the index,
      // while we reserved it, and the reader get preempted until its CAS.
      if (M_UNLIKELY (M_SNAPSH0T_SPMC_INT_FLAG_N(previous) == true)) goto reforce;
      break;
    }
    // Free the reserved index as we failed it to ack it
    c = atomic_fetch_sub(&s->cptTab[idx], 1U);
    M_ASSERT (c != 0 && c <= s->n_reader + 1);
    if (c == 1) {
      m_genint_push(s->freeList, idx);
    }
  }
  M_SNAPSH0T_SPMC_INT_CONTRACT(s);
  return idx;
}

/* End the reading the given buffer */
M_INLINE void
m_snapsh0t_mrsw_read_end(m_snapsh0t_mrsw_ct s, unsigned int idx)
{
  M_SNAPSH0T_SPMC_INT_CONTRACT(s);
  M_ASSERT (idx < s->n_reader + M_SNAPSH0T_SPMC_EXTRA_BUFFER);
  // Decrement reference counter of the buffer
  unsigned int c = atomic_fetch_sub(&s->cptTab[idx], 1U);
  M_ASSERT (c != 0 && c <= s->n_reader + 1);
  if (c == 1) {
    // Buffer no longer used by any reader thread.
    // Push back index in free list
    m_genint_push(s->freeList, idx);
  }
  M_SNAPSH0T_SPMC_INT_CONTRACT(s);
}


/********************************** INTERNAL *********************************/

/* Contract of a SPMC snapshot.
   Nothing notable as it can be accessed concurrently */
#define M_SNAPSH0T_SPMC_CONTRACT(snap) do {                                   \
    M_ASSERT (snap != NULL);                                                  \
    M_ASSERT (snap->data != NULL);                                            \
  } while (0)


// Deferred evaluation
#define M_SNAPSH0T_SPMC_DEF_P1(arg)        M_ID( M_SNAPSH0T_SPMC_DEF_P2 arg )

/* Validate the oplist before going further */
#define M_SNAPSH0T_SPMC_DEF_P2(name, type, oplist, snapshot_t)                \
  M_IF_OPLIST(oplist)(M_SNAPSH0T_SPMC_DEF_P3, M_SNAPSH0T_SPMC_DEF_FAILURE)(name, type, oplist, snapshot_t)

/* Stop processing with a compilation failure */
#define M_SNAPSH0T_SPMC_DEF_FAILURE(name, type, oplist, snapshot_t)           \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST, "(SNAPSHOT_SPMC_DEF): the given argument is not a valid oplist: " #oplist)

/* Expand the type and the functions of a SPMC snapshot */
#define M_SNAPSH0T_SPMC_DEF_P3(name, type, oplist, snapshot_t)                \
  M_SNAPSH0T_SPMC_DEF_TYPE(name, type, oplist, snapshot_t)                    \
  M_CHECK_COMPATIBLE_OPLIST(name, 1, type, oplist)                            \
  M_SNAPSH0T_SPMC_DEF_CORE(name, type, oplist, snapshot_t)                    \

/* Define the type */
#define M_SNAPSH0T_SPMC_DEF_TYPE(name, type, oplist, snapshot_t)              \
                                                                              \
  /* Create an aligned type to avoid false sharing between threads */         \
  typedef struct M_F(name, _aligned_type_s) {                                 \
    type         x;                                                           \
    M_CACHELINE_ALIGN(align, type);                                           \
  } M_F(name, _aligned_type_ct);                                              \
                                                                              \
  typedef struct M_F(name, _s) {                                              \
    M_F(name, _aligned_type_ct)  *data;                                       \
    m_snapsh0t_mrsw_ct          core;                                         \
  } snapshot_t[1];                                                            \
                                                                              \
  /* Define internal types for oplist */                                      \
  typedef snapshot_t M_F(name, _ct);                                          \
  typedef type       M_F(name, _subtype_ct);                                  \

/* Define the core functions */
#define M_SNAPSH0T_SPMC_DEF_CORE(name, type, oplist, snapshot_t)              \
                                                                              \
  M_P(void, name, _init, snapshot_t snap, size_t nReader)                     \
  {                                                                           \
    M_ASSERT (snap != NULL);                                                  \
    M_ASSERT (nReader > 0 && nReader <= M_SNAPSH0T_SPMC_MAX_READER);          \
    snap->data = M_CALL_REALLOC(oplist, M_F(name, _aligned_type_ct),          \
                                NULL, 0, nReader+M_SNAPSH0T_SPMC_EXTRA_BUFFER); \
    if (M_UNLIKELY_NOMEM (snap->data == NULL)) {                              \
      M_MEMORY_FULL(M_F(name, _aligned_type_ct),                              \
                    (nReader+M_SNAPSH0T_SPMC_EXTRA_BUFFER));                  \
    }                                                                         \
    for(size_t i = 0; i < nReader + M_SNAPSH0T_SPMC_EXTRA_BUFFER; i++) {      \
      M_CALL_INIT(oplist, snap->data[i].x);                                   \
    }                                                                         \
    m_snapsh0t_mrsw_init M_R(snap->core, nReader);                            \
    M_SNAPSH0T_SPMC_CONTRACT(snap);                                           \
  }                                                                           \
                                                                              \
  M_P(void, name, _clear, snapshot_t snap)                                    \
  {                                                                           \
    M_SNAPSH0T_SPMC_CONTRACT(snap);                                           \
    size_t nReader = m_snapsh0t_mrsw_size(snap->core);                        \
    for(size_t i = 0; i < nReader + M_SNAPSH0T_SPMC_EXTRA_BUFFER; i++) {      \
      M_CALL_CLEAR(oplist, snap->data[i].x);                                  \
    }                                                                         \
    M_CALL_FREE(oplist, M_F(name, _aligned_type_ct), snap->data, nReader + M_SNAPSH0T_SPMC_EXTRA_BUFFER); \
    m_snapsh0t_mrsw_clear M_R(snap->core);                                    \
  }                                                                           \
                                                                              \
  M_INLINE type *                                                             \
  M_F(name, _write)(snapshot_t snap)                                          \
  {                                                                           \
    M_SNAPSH0T_SPMC_CONTRACT(snap);                                           \
    const unsigned int idx = m_snapsh0t_mrsw_write(snap->core);               \
    return &snap->data[idx].x;                                                \
  }                                                                           \
                                                                              \
  M_INLINE type const *                                                       \
  M_F(name, _read_start)(snapshot_t snap)                                     \
  {                                                                           \
    M_SNAPSH0T_SPMC_CONTRACT(snap);                                           \
    const unsigned int idx = m_snapsh0t_mrsw_read_start(snap->core);          \
    return M_CONST_CAST(type, &snap->data[idx].x);                            \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _read_end)(snapshot_t snap, type const *old)                      \
  {                                                                           \
    M_SNAPSH0T_SPMC_CONTRACT(snap);                                           \
    M_ASSERT (old != NULL);                                                   \
    const M_F(name, _aligned_type_ct) *oldx;                                  \
    oldx = M_CTYPE_FROM_FIELD(M_F(name, _aligned_type_ct), old, type, x);     \
    M_ASSERT (oldx >= snap->data);                                            \
    M_ASSERT (oldx < snap->data + snap->core->n_reader + M_SNAPSH0T_SPMC_EXTRA_BUFFER); \
    M_ASSERT(snap->core->n_reader +M_SNAPSH0T_SPMC_EXTRA_BUFFER < UINT_MAX);  \
    const unsigned int idx = (unsigned int) (oldx - snap->data);              \
    m_snapsh0t_mrsw_read_end(snap->core, idx);                                \
  }                                                                           \
                                                                              \
  M_INLINE type *                                                             \
  M_F(name, _get_write_buffer)(snapshot_t snap)                               \
  {                                                                           \
    M_SNAPSH0T_SPMC_CONTRACT(snap);                                           \
    const unsigned int idx = m_snapsh0t_mrsw_get_write_idx(snap->core);       \
    return &snap->data[idx].x;                                                \
  }                                                                           \
                                                                              \


/********************************** INTERNAL *********************************/

// MPMC is built upon SPMC

// Deferred evaluation
#define M_SNAPSH0T_MPMC_DEF_P1(arg)        M_ID( M_SNAPSH0T_MPMC_DEF_P2 arg )

/* Validate the oplist before going further */
#define M_SNAPSH0T_MPMC_DEF_P2(name, type, oplist, snapshot_t)                \
  M_IF_OPLIST(oplist)(M_SNAPSH0T_MPMC_DEF_P3, M_SNAPSH0T_MPMC_DEF_FAILURE)(name, type, oplist, snapshot_t)

/* Stop processing with a compilation failure */
#define M_SNAPSH0T_MPMC_DEF_FAILURE(name, type, oplist, snapshot_t)           \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST, "(SNAPSHOT_MPMC_DEF): the given argument is not a valid oplist: " #oplist)

/* Expand the type and the functions of a MPMC snapshot */
#define M_SNAPSH0T_MPMC_DEF_P3(name, type, oplist, snapshot_t)                \
  M_SNAPSH0T_SPMC_DEF_P1((M_F(name, _mrsw), type, oplist, M_F(name, _mrsw_pct))) \
  M_SNAPSH0T_MPMC_DEF_TYPE(name, type, oplist, snapshot_t)                    \
  M_CHECK_COMPATIBLE_OPLIST(name, 1, type, oplist)                            \
  M_SNAPSH0T_MPMC_DEF_CORE(name, type, oplist, snapshot_t)                    \

/* Define the types */
#define M_SNAPSH0T_MPMC_DEF_TYPE(name, type, oplist, snapshot_t)              \
                                                                              \
  typedef struct M_F(name, _s) {                                              \
    M_F(name, _mrsw_pct)  core;                                               \
  } snapshot_t[1];                                                            \
                                                                              \
  /* Define internal types for oplist */                                      \
  typedef snapshot_t M_F(name, _ct);                                          \
  typedef type       M_F(name, _subtype_ct);                                  \

/* Define the core functions */
#define M_SNAPSH0T_MPMC_DEF_CORE(name, type, oplist, snapshot_t)              \
                                                                              \
  M_P(void, name, _init, snapshot_t snap, size_t nReader, size_t nWriter)     \
  {                                                                           \
    M_F(name, _mrsw_init)M_R(snap->core, nReader + nWriter -1 );              \
    unsigned int idx = snap->core->core->currentWrite;                        \
    snap->core->core->currentWrite = M_GENINT_ERROR;                          \
    m_snapsh0t_mrsw_write_end(snap->core->core, idx);                         \
  }                                                                           \
                                                                              \
  M_P(void, name, _clear, snapshot_t snap)                                    \
  {                                                                           \
    M_F(name, _mrsw_clear)M_R(snap->core);                                    \
  }                                                                           \
                                                                              \
  M_INLINE type *                                                             \
  M_F(name, _write_start)(snapshot_t snap)                                    \
  {                                                                           \
    M_SNAPSH0T_SPMC_CONTRACT(snap->core);                                     \
    const unsigned int idx = m_snapsh0t_mrsw_write_start(snap->core->core);   \
    return &snap->core->data[idx].x;                                          \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _write_end)(snapshot_t snap, type *old)                           \
  {                                                                           \
    M_SNAPSH0T_SPMC_CONTRACT(snap->core);                                     \
    const M_F(name, _mrsw_aligned_type_ct) *oldx;                             \
    oldx = (const M_F(name, _mrsw_aligned_type_ct) *) old;                    \
    M_ASSERT (oldx >= snap->core->data);                                      \
    M_ASSERT (oldx < snap->core->data + snap->core->core->n_reader + M_SNAPSH0T_SPMC_EXTRA_BUFFER); \
    M_ASSERT(snap->core->core->n_reader + M_SNAPSH0T_SPMC_EXTRA_BUFFER < UINT_MAX); \
    const unsigned int idx = (unsigned int) (oldx - snap->core->data);        \
    m_snapsh0t_mrsw_write_end(snap->core->core, idx);                         \
  }                                                                           \
                                                                              \
  M_INLINE type const *                                                       \
  M_F(name, _read_start)(snapshot_t snap)                                     \
  {                                                                           \
    return M_F(name, _mrsw_read_start)(snap->core);                           \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _read_end)(snapshot_t snap, type const *old)                      \
  {                                                                           \
    M_F(name, _mrsw_read_end)(snap->core, old);                               \
  }                                                                           \
                                                                              \

//FIXME: Evaluate the needs for the methods _set_, _init_set.

M_END_PROTECTED_CODE

/********************************** INTERNAL *********************************/

#if M_USE_SMALL_NAME
#define SNAPSHOT_SPSC_DEF M_SNAPSHOT_SPSC_DEF
#define SNAPSHOT_SPSC_DEF_AS M_SNAPSHOT_SPSC_DEF_AS
#define SNAPSHOT_SPMC_DEF M_SNAPSHOT_SPMC_DEF
#define SNAPSHOT_SPMC_DEF_AS M_SNAPSHOT_SPMC_DEF_AS
#define SNAPSHOT_MPMC_DEF M_SNAPSHOT_MPMC_DEF
#define SNAPSHOT_MPMC_DEF_AS M_SNAPSHOT_MPMC_DEF_AS
#define SNAPSHOT_OPLIST M_SNAPSHOT_OPLIST
#endif

#endif
