/*
 * M*LIB - Fixed size (Bounded) QUEUE & STACK interface (Not Thread safe)
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
#ifndef MSTARLIB_QUEUE_H
#define MSTARLIB_QUEUE_H

#include "m-core.h"

/* Define a fixed size queue.
   If size is 0, then the size will only be defined at run-time when initializing the queue,
   otherwise the size will be a compile time constant embedded in the structure.
   USAGE: QUEUE_DEF(name, type, size_of_QUEUE_or_0[, oplist]) */
#define M_QUEUE_DEF(name, ... ) M_QUEUE_DEF_AS(name, M_F(name, _t), __VA_ARGS__)


/* Define a fixed size queue
   as the provided type name_t.
   USAGE: QUEUE_DEF_AS(name, name_t, type, size_of_QUEUE_or_0[, oplist of type]) */
#define M_QUEUE_DEF_AS(name, name_t, type, ... )                              \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_QUEU3_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                                  \
              ((name, type, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(type)(), name_t, 0), \
               (name, type, __VA_ARGS__,                                 name_t, 0))) \
  M_END_PROTECTED_CODE

/* Define a fixed size stack.
   If size is 0, then the size will only be defined at run-time when initializing the stack,
   otherwise the size will be a compile time constant embedded in the structure.
   USAGE: STACK_DEF(name, type, size_of_stack_or_0[, oplist]) */
#define M_STACK_DEF(name, ... ) M_STACK_DEF_AS(name, M_F(name, _t), __VA_ARGS__)


/* Define a fixed size stack
   as the provided type name_t.
   USAGE: QUEUE_DEF_AS(name, name_t, type, size_of_stack_or_0[, oplist of type]) */
#define M_STACK_DEF_AS(name, name_t, type, ... )                              \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_QUEU3_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                                  \
              ((name, type, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(type)(), name_t, 1), \
               (name, type, __VA_ARGS__,                                 name_t, 1))) \
  M_END_PROTECTED_CODE

/* Define the oplist of a fixed size queue given its name and its oplist.
   USAGE: QUEUE_OPLIST(name[, oplist of the type]) */
#define M_QUEUE_OPLIST(...)                                                   \
  M_QUEU3_OPLIST_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                               \
                 ((__VA_ARGS__, M_BASIC_OPLIST),                              \
                  (__VA_ARGS__ )))


/*****************************************************************************/
/********************************** INTERNAL *********************************/
/*****************************************************************************/

/* Deferred evaluation for the definition,
   so that all arguments are evaluated before further expansion */
#define M_QUEU3_OPLIST_P1(arg) M_QUEU3_OPLIST_P2 arg

/* Validation of the given oplist */
#define M_QUEU3_OPLIST_P2(name, oplist)                                       \
  M_IF_OPLIST(oplist)(M_QUEU3_OPLIST_P3, M_QUEU3_OPLIST_FAILURE)(name, oplist)

/* Prepare a clean compilation failure */
#define M_QUEU3_OPLIST_FAILURE(name, oplist)                                  \
  ((M_LIB_ERROR(ARGUMENT_OF_BUFFER_OPLIST_IS_NOT_AN_OPLIST, name, oplist)))

/* OPLIST definition of a queue / stack */
#ifndef M_USE_CONTEXT
#define M_QUEU3_OPLIST_P3(name, oplist)                                       \
  (INIT(API_0(M_F(name, _i_init)))                                            \
  ,INIT_SET(API_0(M_F(name, _init_set)))                                      \
  ,SET(API_0(M_F(name, _set)))                                                \
  ,CLEAR(API_0(M_F(name, _clear)))                                            \
  ,NAME(name)                                                                 \
  ,TYPE(M_F(name,_ct)), GENTYPE(struct M_F(name,_s)*)                         \
  ,SUBTYPE(M_F(name, _subtype_ct))                                            \
  ,RESET(API_0(M_F(name,_reset)))                                             \
  ,PUSH(API_0(M_F(name,_push)))                                               \
  ,POP(API_0(M_F(name,_pop)))                                                 \
  ,OPLIST(oplist)                                                             \
  ,EMPTY_P(M_F(name, _empty_p)),                                              \
  ,FULL_P(M_F(name, _full_p)),                                                \
  ,GET_SIZE(M_F(name, _size))                                                 \
  )
#else
#define M_QUEU3_OPLIST_P3(name, oplist)                                       \
  (INIT(API_0P(M_F(name, _i_init)))                                           \
  ,INIT_SET(API_0P(M_F(name, _init_set)))                                     \
  ,SET(API_0P(M_F(name, _set)))                                               \
  ,CLEAR(API_0P(M_F(name, _clear)))                                           \
  ,NAME(name)                                                                 \
  ,TYPE(M_F(name,_ct)), GENTYPE(struct M_F(name,_s)*)                         \
  ,SUBTYPE(M_F(name, _subtype_ct))                                            \
  ,RESET(API_0P(M_F(name,_reset)))                                            \
  ,PUSH(API_0P(M_F(name,_push)))                                              \
  ,POP(API_0P(M_F(name,_pop)))                                                \
  ,OPLIST(oplist)                                                             \
  ,EMPTY_P(M_F(name, _empty_p)),                                              \
  ,FULL_P(M_F(name, _full_p)),                                                \
  ,GET_SIZE(M_F(name, _size))                                                 \
  )
#endif
   
/* Test if the size is only run-time or build time */
#define M_QUEU3_IF_CTE_SIZE(m_size)  M_IF(M_BOOL(m_size))

/* Return the size (run time or build time).
   NOTE: It assumed that the queue variable name is 'v' */
#define M_QUEU3_SIZE(v, m_size)                                               \
  ((size_t)M_QUEU3_IF_CTE_SIZE(m_size) (m_size, v->capacity))

/* Contract of a queue. */
#define M_QUEU3_CONTRACT(queue, size)        do {                             \
    M_ASSERT (queue != NULL);                                                 \
    M_ASSERT (queue->data != NULL);                                           \
  }while (0)

/* Deferred evaluation for the definition,
   so that all arguments are evaluated before further expansion */
#define M_QUEU3_DEF_P1(arg) M_ID( M_QUEU3_DEF_P2 arg )

/* Validate the value oplist before going further */
#define M_QUEU3_DEF_P2(name, type, m_size, oplist, queue_t, stack_mode)       \
  M_IF_OPLIST(oplist)(M_QUEU3_DEF_P3, M_QUEU3_DEF_FAILURE)(name, type, m_size, oplist, queue_t, stack_mode)

/* Stop processing with a compilation failure */
#define M_QUEU3_DEF_FAILURE(name, type, m_size, oplist, queue_t, stack_mode)  \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST, "(M_QUEUE_DEF/M_STACK_DEF): the given argument is not a valid oplist: " M_AS_STR(oplist))

/* Define the queue type using mutex lock and its functions.
  - name: main prefix of the container
  - type: type of an element of the queue
  - m_size: constant to 0 if variable runtime size, or else the fixed size of the queue
  - oplist: the oplist of the type of an element of the queue
  - queue_t: name of the queue
  */
#define M_QUEU3_DEF_P3(name, type, m_size, oplist, queue_t, stack_mode)       \
  M_QUEU3_DEF_TYPE(name, type, m_size, oplist, queue_t)                       \
  M_CHECK_COMPATIBLE_OPLIST(name, 1, type, oplist)                            \
  M_QUEU3_DEF_CORE(name, type, m_size, oplist, queue_t, stack_mode)           \
  M_EMPLACE_QUEUE_DEF(name, queue_t, _emplace, oplist, M_EMPLACE_QUEUE_GENE)

/* Define the type of a queue */
#define M_QUEU3_DEF_TYPE(name, type, m_size, oplist, queue_t)                 \
                                                                              \
  typedef struct M_F(name, _s) {                                              \
    unsigned    idx_prod;     /* Index of the production threads  */          \
    M_QUEU3_IF_CTE_SIZE(m_size)( ,unsigned capacity;) /* Capacity of the queue */ \
    unsigned  idx_cons;     /* Index of the consumption threads */            \
    unsigned  number;    /* Number of elements in the queue */                \
    /* If fixed size, array of elements, otherwise pointer to element */      \
    type  M_QUEU3_IF_CTE_SIZE(m_size)(data[m_size], *data);                   \
  } queue_t[1];                                                               \
                                                                              \
  typedef struct M_F(name, _s) *M_F(name, _ptr);                              \
  typedef const struct M_F(name, _s) *M_F(name, _srcptr);                     \
  /* Internal types used by the oplist */                                     \
  typedef type M_F(name, _subtype_ct);                                        \
  typedef queue_t M_F(name, _ct);                                             \

/* Define the core functionalities of a queue */
#define M_QUEU3_DEF_CORE(name, type, m_size, oplist, queue_t, stack_mode)     \
                                                                              \
M_P(void, name, _init, queue_t v, size_t size)                                \
{                                                                             \
  M_ASSERT(size <= UINT_MAX);                                                 \
  M_UNUSED_CONTEXT();                                                         \
  M_QUEU3_IF_CTE_SIZE(m_size)(M_ASSERT(size == m_size), v->capacity = (unsigned) size); \
  v->idx_prod = v->idx_cons = v->number = 0;                                  \
                                                                              \
  M_QUEU3_IF_CTE_SIZE(m_size)( /* Statically allocated */ ,                   \
    v->data = M_CALL_REALLOC(oplist, type, NULL, 0, M_QUEU3_SIZE(v, m_size)); \
    if (M_UNLIKELY_NOMEM (v->data == NULL)) {                                 \
      M_MEMORY_FULL (type, M_QUEU3_SIZE(v, m_size));                          \
    }                                                                         \
  )                                                                           \
  for(size_t i = 0; i < size; i++) {                                          \
    M_CALL_INIT(oplist, v->data[i]);                                          \
  }                                                                           \
  M_QUEU3_CONTRACT(v,m_size);                                                 \
}                                                                             \
                                                                              \
M_QUEU3_IF_CTE_SIZE(m_size)(                                                  \
 M_P(void, name,_i_init, queue_t v)                                           \
 {                                                                            \
   M_F(name, _init)M_R(v, m_size);                                            \
 }                                                                            \
 , )                                                                          \
                                                                              \
 M_P(void, name, _i_clear_obj, queue_t v)                                     \
 {                                                                            \
   M_QUEU3_CONTRACT(v,m_size);                                                \
   M_UNUSED_CONTEXT();                                                        \
   for(size_t i = 0; i < M_QUEU3_SIZE(v, m_size); i++) {                      \
     M_CALL_CLEAR(oplist, v->data[i]);                                        \
   }                                                                          \
   v->idx_prod = v->idx_cons = v->number = 0;                                 \
   M_QUEU3_CONTRACT(v,m_size);                                                \
 }                                                                            \
                                                                              \
 M_P(void, name, _clear, queue_t v)                                           \
 {                                                                            \
   M_QUEU3_CONTRACT(v,m_size);                                                \
   M_UNUSED_CONTEXT();                                                        \
   M_F(name,_i_clear_obj)M_R(v);                                              \
   M_QUEU3_IF_CTE_SIZE(m_size)( ,                                             \
     M_CALL_FREE(oplist, type, v->data, M_QUEU3_SIZE(v, m_size));             \
     v->data = NULL;                                                          \
   )                                                                          \
 }                                                                            \
                                                                              \
 M_N(void, name, _reset, queue_t v)                                           \
 {                                                                            \
   M_QUEU3_CONTRACT(v,m_size);                                                \
   v->idx_prod = v->idx_cons = v->number = 0;                                 \
   M_QUEU3_CONTRACT(v,m_size);                                                \
 }                                                                            \
                                                                              \
 M_P(void, name, _init_set, queue_t dest, const queue_t src)                  \
 {                                                                            \
   M_ASSERT (dest != src);                                                    \
   M_QUEU3_CONTRACT(src,m_size);                                              \
   M_F(name, _init) M_R(dest, M_QUEU3_SIZE(src, m_size));                     \
   for(size_t i = 0; i < M_QUEU3_SIZE(src, m_size); i++) {                    \
     M_CALL_SET(oplist, dest->data[i], src->data[i]);                         \
   }                                                                          \
   dest->idx_prod = src->idx_prod;                                            \
   dest->idx_cons = src->idx_cons;                                            \
   dest->number   = src->number;                                              \
   M_QUEU3_CONTRACT(dest, m_size);                                            \
 }                                                                            \
                                                                              \
 M_P(void, name, _set, queue_t dest, const queue_t src)                       \
 {                                                                            \
   M_F(name, _clear) M_R(dest);                                               \
   M_F(name, _init_set) M_R(dest, src);                                       \
 }                                                                            \
                                                                              \
 M_INLINE bool                                                                \
 M_F(name, _empty_p)(const queue_t v)                                         \
 {                                                                            \
   M_QUEU3_CONTRACT(v,m_size);                                                \
   return v->number == 0;                                                     \
 }                                                                            \
                                                                              \
 M_INLINE bool                                                                \
 M_F(name, _full_p)(const queue_t v)                                          \
 {                                                                            \
   M_QUEU3_CONTRACT(v,m_size);                                                \
   return v->number == M_QUEU3_SIZE(v, m_size);                               \
 }                                                                            \
                                                                              \
 M_INLINE size_t                                                              \
 M_F(name, _size)(const queue_t v)                                            \
 {                                                                            \
   M_QUEU3_CONTRACT(v,m_size);                                                \
   return v->number;                                                          \
 }                                                                            \
                                                                              \
 M_P(bool, name, _push, queue_t v, type const data)                           \
 {                                                                            \
  M_QUEU3_CONTRACT(v,m_size);                                                 \
  M_UNUSED_CONTEXT();                                                         \
  if (M_F(name, _full_p)(v))                                                  \
    return false;                                                             \
                                                                              \
  unsigned idx = v->idx_prod;                                                 \
  M_CALL_SET(oplist, v->data[idx], data);                                     \
  idx++;                                                                      \
  M_IF(stack_mode)(, idx = (idx == M_QUEU3_SIZE(v, m_size)) ? 0 : idx);       \
  v->idx_prod = idx;                                                          \
                                                                              \
  v->number++;                                                                \
                                                                              \
  M_QUEU3_CONTRACT(v,m_size);                                                 \
  return true;                                                                \
 }                                                                            \
                                                                              \
 M_P(bool, name, _pop, type *data, queue_t v)                                 \
 {                                                                            \
   M_QUEU3_CONTRACT(v,m_size);                                                \
   M_ASSERT (data != NULL);                                                   \
   M_UNUSED_CONTEXT();                                                        \
   if (M_F(name, _empty_p)(v))                                                \
     return false;                                                            \
                                                                              \
   /* POP data from the queue and update INDEX */                             \
   M_IF(stack_mode)(                                                          \
    /* STACK queue */                                                         \
    v->idx_prod --;                                                           \
    M_CALL_SET(oplist, *data, v->data[v->idx_prod]);                          \
   ,                                                                          \
    /* FIFO queue */                                                          \
    M_CALL_SET(oplist, *data, v->data[v->idx_cons]);                          \
    v->idx_cons = (v->idx_cons == M_QUEU3_SIZE(v, m_size)-1) ? 0 : (v->idx_cons + 1); \
   )                                                                          \
   v->number--;                                                               \
                                                                              \
   M_QUEU3_CONTRACT(v,m_size);                                                \
   return true;                                                               \
 }                                                                            \
                                                                              \
 M_INLINE size_t                                                              \
 M_F(name, _capacity)(const queue_t v)                                        \
 {                                                                            \
   (void) v; /* may be unused */                                              \
   return M_QUEU3_SIZE(v, m_size);                                            \
 }                                                                            \


#if M_USE_SMALL_NAME
#define QUEUE_DEF M_QUEUE_DEF
#define QUEUE_DEF_AS M_QUEUE_DEF_AS
#define STACK_DEF M_STACK_DEF
#define STACK_DEF_AS M_STACK_DEF_AS
#define QUEUE_OPLIST M_QUEUE_OPLIST
#endif

#endif
