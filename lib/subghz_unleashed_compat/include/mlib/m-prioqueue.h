/*
 * M*LIB - dynamic priority queue module
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
#ifndef MSTARLIB_PRIOQUEUE_H
#define MSTARLIB_PRIOQUEUE_H

#include "m-array.h"            /* Priority queue are built upon array */

/* Priority queue based on binary heap implementation */

/* Define a priority queue (prioqueue) of a given type and its associated functions.
   USAGE: PRIOQUEUE_DEF(name, type [, oplist_of_the_type]) */
#define M_PRIOQUEUE_DEF(name, ...)                                            \
  M_PRIOQUEUE_DEF_AS(name, M_F(name,_t), M_F(name,_it_t), __VA_ARGS__)


/* Define a priority queue (prioqueue) of a given type and its associated functions.
  as the name name_t with an iterator named it_t
   USAGE: PRIOQUEUE_DEF_AS(name, name_t, it_t, type [, oplist_of_the_type]) */
#define M_PRIOQUEUE_DEF_AS(name, name_t, it_t, ...)                           \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_PR1OQUEUE_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                              \
                    ((name, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), name_t, it_t ), \
                     (name, __VA_ARGS__,                                        name_t, it_t ))) \
  M_END_PROTECTED_CODE


/* Define the oplist of a prioqueue of type.
   USAGE: PRIOQUEUE_OPLIST(name[, oplist of the type]) */
#define M_PRIOQUEUE_OPLIST(...)                                               \
  M_PR1OQUEUE_OPLIST_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                           \
                       ((__VA_ARGS__, M_BASIC_OPLIST),                        \
                        (__VA_ARGS__ )))


/*****************************************************************************/
/********************************** INTERNAL *********************************/
/*****************************************************************************/

/* Deferred evaluation for the definition,
   so that all arguments are evaluated before further expansion */
#define M_PR1OQUEUE_OPLIST_P1(arg) M_PR1OQUEUE_OPLIST_P2 arg

/* Validation of the given oplist */
#define M_PR1OQUEUE_OPLIST_P2(name, oplist)                                   \
  M_IF_OPLIST(oplist)(M_PR1OQUEUE_OPLIST_P3, M_PR1OQUEUE_OPLIST_FAILURE)(name, oplist)

/* Prepare a clean compilation failure */
#define M_PR1OQUEUE_OPLIST_FAILURE(name, oplist)                              \
  ((M_LIB_ERROR(ARGUMENT_OF_PRIOQUEUE_OPLIST_IS_NOT_AN_OPLIST, name, oplist)))

/* Define oplist of a priority queue */
#ifndef M_USE_CONTEXT
#define M_PR1OQUEUE_OPLIST_P3(name, oplist)                                   \
  (INIT(M_F(name, _init))                                                     \
   ,INIT_SET(M_F(name, _init_set))                                            \
   ,INIT_WITH(API_1(M_INIT_VAI))                                              \
   ,SET(M_F(name, _set))                                                      \
   ,CLEAR(M_F(name, _clear))                                                  \
   ,INIT_MOVE(M_F(name, _init_move))                                          \
   ,MOVE(M_F(name, _move))                                                    \
   ,SWAP(M_F(name, _swap))                                                    \
   ,NAME(name)                                                                \
   ,TYPE(M_F(name,_ct)), GENTYPE(struct M_F(name,_s)*)                        \
   ,SUBTYPE(M_F(name, _subtype_ct))                                           \
   ,RESET(M_F(name,_reset))                                                   \
   ,PUSH(M_F(name,_push))                                                     \
   ,POP(M_F(name,_pop))                                                       \
   ,OPLIST(oplist)                                                            \
   ,EMPTY_P(M_F(name, _empty_p))                                              \
   ,GET_SIZE(M_F(name, _size))                                                \
   ,IT_TYPE(M_F(name, _it_ct))                                                \
   ,IT_FIRST(M_F(name,_it))                                                   \
   ,IT_END(M_F(name,_it_end))                                                 \
   ,IT_SET(M_F(name,_it_set))                                                 \
   ,IT_END_P(M_F(name,_end_p))                                                \
   ,IT_EQUAL_P(M_F(name,_it_equal_p))                                         \
   ,IT_LAST_P(M_F(name,_last_p))                                              \
   ,IT_NEXT(M_F(name,_next))                                                  \
   ,IT_CREF(M_F(name,_cref))                                                  \
   ,M_IF_METHOD(GET_STR, oplist)(GET_STR(M_F(name, _get_str)),)               \
   ,M_IF_METHOD(PARSE_STR, oplist)(PARSE_STR(M_F(name, _parse_str)),)         \
   ,M_IF_METHOD(OUT_STR, oplist)(OUT_STR(M_F(name, _out_str)),)               \
   ,M_IF_METHOD(IN_STR, oplist)(IN_STR(M_F(name, _in_str)),)                  \
   ,M_IF_METHOD(OUT_SERIAL, oplist)(OUT_SERIAL(M_F(name, _out_serial)),)      \
   ,M_IF_METHOD(IN_SERIAL, oplist)(IN_SERIAL(M_F(name, _in_serial)),)         \
   )
#else
#define M_PR1OQUEUE_OPLIST_P3(name, oplist)                                   \
  (INIT(M_F(name, _init))                                                     \
   ,INIT_SET(API_0P(M_F(name, _init_set)))                                    \
   ,INIT_WITH(API_1(M_INIT_VAI))                                              \
   ,SET(API_0P(M_F(name, _set)))                                              \
   ,CLEAR(API_0P(M_F(name, _clear)))                                          \
   ,INIT_MOVE(M_F(name, _init_move))                                          \
   ,MOVE(API_0P(M_F(name, _move)))                                            \
   ,SWAP(M_F(name, _swap))                                                    \
   ,NAME(name)                                                                \
   ,TYPE(M_F(name,_ct)), GENTYPE(struct M_F(name,_s)*)                        \
   ,SUBTYPE(M_F(name, _subtype_ct))                                           \
   ,RESET(API_0P(M_F(name,_reset)))                                           \
   ,PUSH(API_0P(M_F(name,_push)))                                             \
   ,POP(API_0P(M_F(name,_pop)))                                               \
   ,OPLIST(oplist)                                                            \
   ,EMPTY_P(M_F(name, _empty_p))                                              \
   ,GET_SIZE(M_F(name, _size))                                                \
   ,IT_TYPE(M_F(name, _it_ct))                                                \
   ,IT_FIRST(M_F(name,_it))                                                   \
   ,IT_END(M_F(name,_it_end))                                                 \
   ,IT_SET(M_F(name,_it_set))                                                 \
   ,IT_END_P(M_F(name,_end_p))                                                \
   ,IT_EQUAL_P(M_F(name,_it_equal_p))                                         \
   ,IT_LAST_P(M_F(name,_last_p))                                              \
   ,IT_NEXT(M_F(name,_next))                                                  \
   ,IT_CREF(M_F(name,_cref))                                                  \
   ,M_IF_METHOD(GET_STR, oplist)(GET_STR(API_0P(M_F(name, _get_str))),)       \
   ,M_IF_METHOD(PARSE_STR, oplist)(PARSE_STR(API_0P(M_F(name, _parse_str))),) \
   ,M_IF_METHOD(OUT_STR, oplist)(OUT_STR(M_F(name, _out_str)),)               \
   ,M_IF_METHOD(IN_STR, oplist)(IN_STR(API_0P(M_F(name, _in_str))),)          \
   ,M_IF_METHOD(OUT_SERIAL, oplist)(OUT_SERIAL(API_0P(M_F(name, _out_serial))),) \
   ,M_IF_METHOD(IN_SERIAL, oplist)(IN_SERIAL(API_0P(M_F(name, _in_serial))),) \
   )
#endif

/********************************** INTERNAL *********************************/

/* Deferred evaluation for the definition,
   so that all arguments are evaluated before further expansion */
#define M_PR1OQUEUE_DEF_P1(arg)    M_ID( M_PR1OQUEUE_DEF_P2 arg )

/* Validate the oplist before going further */
#define M_PR1OQUEUE_DEF_P2(name, type, oplist, prioqueue_t, it_t)             \
  M_IF_OPLIST(oplist)(M_PR1OQUEUE_DEF_P3, M_PR1OQUEUE_DEF_FAILURE)(name, type, oplist, prioqueue_t, it_t)

/* Stop processing with a compilation failure */
#define M_PR1OQUEUE_DEF_FAILURE(name, type, oplist, prioqueue_t, it_t)        \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST, "(PRIOQUEUE_DEF): the given argument is not a valid oplist: " #oplist)

/* Define the priority queue:
   - name: prefix to use,
   - type: type of the contained objects,
   - oplist: oplist of the contained objects,
   - prioqueue_t: type of the container,
   - it_t: iterator of the container
*/
#define M_PR1OQUEUE_DEF_P3(name, type, oplist, prioqueue_t, it_t)             \
  /* Definition of the internal array used to construct the priority queue */ \
  M_ARRAY_DEF(M_F(name, _array), type, oplist)                                \
  M_PR1OQUEUE_DEF_TYPE(name, type, oplist, prioqueue_t, it_t)                 \
  M_CHECK_COMPATIBLE_OPLIST(name, 1, type, oplist)                            \
  M_PR1OQUEUE_DEF_CORE(name, type, oplist, prioqueue_t, it_t)                 \
  M_PR1OQUEUE_DEF_IT(name, type, oplist, prioqueue_t, it_t)                   \
  M_PR1OQUEUE_DEF_IO(name, type, oplist, prioqueue_t, it_t)                   \
  M_EMPLACE_QUEUE_DEF(name, prioqueue_t, _emplace, oplist, M_EMPLACE_QUEUE_GENE)

/* Define the types */
#define M_PR1OQUEUE_DEF_TYPE(name, type, oplist, prioqueue_t, it_t)           \
                                                                              \
  /* Define the priority queue over the defined array */                      \
  typedef struct M_F(name, _s) {                                              \
    M_F(name, _array_t) array;                                                \
  } prioqueue_t[1];                                                           \
  /* Define the pointer references to the priority queue */                   \
  typedef struct M_F(name, _s) *M_F(name, _ptr);                              \
  typedef const struct M_F(name, _s) *M_F(name, _srcptr);                     \
                                                                              \
  /* The iterator is the same one as the one of the internal array */         \
  typedef M_F(name, _array_it_t) it_t;                                        \
                                                                              \
  /* Definition of the internal types used by the oplist */                   \
  typedef prioqueue_t M_F(name, _ct);                                         \
  typedef type        M_F(name, _subtype_ct);                                 \
  typedef it_t        M_F(name, _it_ct);                                      \

/* Define the core functions */
#define M_PR1OQUEUE_DEF_CORE(name, type, oplist, prioqueue_t, it_t)           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _init)(prioqueue_t p)                                             \
  {                                                                           \
    M_F(name, _array_init)(p->array);                                         \
  }                                                                           \
                                                                              \
  M_P(void, name, _init_set, prioqueue_t p, prioqueue_t const o)              \
  {                                                                           \
    M_F(name, _array_init_set) M_R(p->array, o->array);                       \
  }                                                                           \
                                                                              \
  M_P(void, name, _set, prioqueue_t p, prioqueue_t const o)                   \
  {                                                                           \
    M_F(name, _array_set) M_R(p->array, o->array);                            \
  }                                                                           \
                                                                              \
  M_P(void, name, _clear, prioqueue_t p)                                      \
  {                                                                           \
    M_F(name, _array_clear) M_R(p->array);                                    \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _init_move)(prioqueue_t p, prioqueue_t o)                         \
  {                                                                           \
    M_F(name, _array_init_move)(p->array, o->array);                          \
  }                                                                           \
                                                                              \
  M_P(void, name, _move, prioqueue_t p, prioqueue_t o)                        \
  {                                                                           \
    M_F(name, _array_move) M_R(p->array, o->array);                           \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _swap)(prioqueue_t p, prioqueue_t o)                              \
  {                                                                           \
    M_F(name, _array_swap)(p->array, o->array);                               \
  }                                                                           \
                                                                              \
  M_P(void, name, _reset, prioqueue_t p)                                      \
  {                                                                           \
    M_F(name, _array_reset)M_R(p->array);                                     \
  }                                                                           \
                                                                              \
  M_INLINE size_t                                                             \
  M_F(name, _i_parent)(size_t i)                                              \
  {                                                                           \
    M_ASSERT (i > 0);                                                         \
    return (i - 1) / 2;                                                       \
  }                                                                           \
                                                                              \
  M_INLINE size_t                                                             \
  M_F(name, _i_lchild)(size_t i)                                              \
  {                                                                           \
    M_ASSERT(i <= ((SIZE_MAX)-2)/2);                                          \
    return 2*i + 1;                                                           \
  }                                                                           \
                                                                              \
  M_INLINE size_t                                                             \
  M_F(name, _i_rchild)(size_t i)                                              \
  {                                                                           \
    M_ASSERT(i <= ((SIZE_MAX)-2)/2);                                          \
    return 2*i + 2;                                                           \
  }                                                                           \
                                                                              \
  M_INLINE int                                                                \
  M_F(name, _i_cmp)(const prioqueue_t p, size_t i, size_t j)                  \
  {                                                                           \
    return M_CALL_CMP(oplist, *M_F(name, _array_cget)(p->array, i),           \
                      *M_F(name, _array_cget)(p->array, j));                  \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _empty_p)(prioqueue_t const p)                                    \
  {                                                                           \
    return M_F(name, _array_empty_p)(p->array);                               \
  }                                                                           \
                                                                              \
  M_INLINE size_t                                                             \
  M_F(name, _size)(prioqueue_t const p)                                       \
  {                                                                           \
    return M_F(name, _array_size)(p->array);                                  \
  }                                                                           \
                                                                              \
  M_P(void, name, _push, prioqueue_t p, type const x)                         \
  {                                                                           \
    /* Push back the new element at the end of the array */                   \
    M_F(name, _array_push_back) M_R(p->array, x);                             \
                                                                              \
    /* Reorder the array by swapping with its parent                          \
     * until it reaches the right position */                                 \
    size_t i = M_F(name, _array_size)(p->array)-1;                            \
    while (i > 0) {                                                           \
      size_t j = M_F(name, _i_parent)(i);                                     \
      if (M_F(name, _i_cmp)(p, j, i) <= 0)                                    \
        break;                                                                \
      M_F(name, _array_swap_at) (p->array, i, j);                             \
      i = j;                                                                  \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_INLINE type const *                                                       \
  M_F(name, _front)(prioqueue_t const p)                                      \
  {                                                                           \
    return M_F(name, _array_cget)(p->array, 0);                               \
  }                                                                           \
                                                                              \
  M_P(void, name, _pop, type *x, prioqueue_t p)                               \
  {                                                                           \
    /* Swap the front element with the last element */                        \
    size_t size = M_F(name, _array_size)(p->array)-1;                         \
    M_F(name, _array_swap_at) (p->array, 0, size);                            \
    /* Swap the new last element  */                                          \
    M_F(name, _array_pop_back) M_R(x, p->array);                              \
                                                                              \
    /* Reorder the heap */                                                    \
    size_t i = 0;                                                             \
    while (true) {                                                            \
      size_t child = M_F(name, _i_lchild)(i);                                 \
      if (child >= size)                                                      \
        break;                                                                \
      size_t otherChild = M_F(name, _i_rchild)(i);                            \
      if (otherChild < size                                                   \
          && M_F(name, _i_cmp)(p, otherChild, child) < 0 ) {                  \
        child = otherChild;                                                   \
      }                                                                       \
      if (M_F(name, _i_cmp)(p, i, child) <= 0)                                \
        break;                                                                \
      M_F(name, _array_swap_at) (p->array, i, child);                         \
      i = child;                                                              \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_IF_METHOD(EQUAL, oplist)                                                  \
  (                                                                           \
    /* EQUAL & CMP may be uncorrelated */                                     \
    M_INLINE bool                                                             \
    M_F(name, _equal_p)(prioqueue_t const p, prioqueue_t const q)             \
    {                                                                         \
      return M_F(name, _array_equal_p)(p->array, q->array);                   \
    }                                                                         \
                                                                              \
   M_INLINE size_t                                                            \
   M_F(name, _i_find)(prioqueue_t p, type const x)                            \
   {                                                                          \
     size_t size = M_F(name, _array_size)(p->array);                          \
     size_t i = 0;                                                            \
     for(i = 0; i < size; i++) {                                              \
       /* We cannot use CMP and the partial order to go faster                \
        EQUAL & CMP may be uncorrelated */                                    \
       if (M_CALL_EQUAL(oplist, *M_F(name, _array_cget)(p->array, i), x))     \
         break;                                                               \
     }                                                                        \
     return i;                                                                \
   }                                                                          \
                                                                              \
   M_P(bool, name, _erase, prioqueue_t p, type const x)                       \
   {                                                                          \
     /* First pass: search for an item EQUAL to x */                          \
     size_t size = M_F(name, _array_size)(p->array);                          \
     size_t i = M_F(name, _i_find)(p, x);                                     \
     /* If x is not found, then stop */                                       \
     if (i >= size)                                                           \
       return false;                                                          \
     /* Swap the found item and the last element */                           \
     size--;                                                                  \
     M_F(name, _array_swap_at) (p->array, i, size);                           \
     M_F(name, _array_pop_back) M_R(NULL, p->array);                          \
     /* Move back the last swapped element to its right position in the heap */ \
     while (true) {                                                           \
       size_t child = M_F(name, _i_lchild)(i);                                \
       if (child >= size) break;                                              \
       size_t otherChild = M_F(name, _i_rchild)(i);                           \
       if (otherChild < size                                                  \
           && M_F(name, _i_cmp)(p, otherChild, child) < 0 ) {                 \
         child = otherChild;                                                  \
       }                                                                      \
       if (M_F(name, _i_cmp)(p, i, child) <= 0) break;                        \
       M_F(name, _array_swap_at) (p->array, i, child);                        \
       i = child;                                                             \
     }                                                                        \
     return true;                                                             \
   }                                                                          \
                                                                              \
   M_P(void, name, _update, prioqueue_t p, type const xold, type const xnew)  \
   {                                                                          \
     /* NOTE: xold can be the same pointer than xnew */                       \
     /* First pass: search for an item EQUAL to x */                          \
     size_t size = M_F(name, _array_size)(p->array);                          \
     size_t i = M_F(name, _i_find)(p, xold);                                  \
     /* We shall have found the item */                                       \
     M_ASSERT (i < size);                                                     \
     /* Test if the position of the old data is further or nearer than the new */ \
     int cmp = M_CALL_CMP(oplist, *M_F(name, _array_cget)(p->array, i), xnew); \
     /* Set the found item to the new element */                              \
     M_F(name, _array_set_at) M_R(p->array, i, xnew);                         \
     if (cmp < 0) {                                                           \
      /* Move back the updated element to its new position, further in the heap */ \
      while (true) {                                                          \
        size_t child = M_F(name, _i_lchild)(i);                               \
        if (child >= size) break;                                             \
        size_t otherChild = M_F(name, _i_rchild)(i);                          \
        if (otherChild < size                                                 \
            && M_F(name, _i_cmp)(p, otherChild, child) < 0 ) {                \
          child = otherChild;                                                 \
        }                                                                     \
        if (M_F(name, _i_cmp)(p, i, child) <= 0) break;                       \
        M_F(name, _array_swap_at) (p->array, i, child);                       \
        i = child;                                                            \
      }                                                                       \
     } else {                                                                 \
      /* Move back the updated element to its new position, nearest in the heap */ \
      while (i > 0) {                                                         \
        size_t parent = M_F(name, _i_parent)(i);                              \
        if (M_F(name, _i_cmp)(p, parent, i) <= 0) break;                      \
        M_F(name, _array_swap_at) (p->array, i, parent);                      \
        i = parent;                                                           \
      }                                                                       \
     }                                                                        \
   }                                                                          \
   , /* No EQUAL */ )                                                         \

/* Define the IT based functions */
#define M_PR1OQUEUE_DEF_IT(name, type, oplist, prioqueue_t, it_t)             \
                                                                              \
  /* Define iterators over the array iterator */                              \
  M_INLINE void                                                               \
  M_F(name, _it)(it_t it, prioqueue_t const v)                                \
  {                                                                           \
    M_F(name, _array_it)(it, v->array);                                       \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_last)(it_t it, prioqueue_t const v)                           \
  {                                                                           \
    M_F(name, _array_it_last)(it, v->array);                                  \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_end)(it_t it, prioqueue_t const v)                            \
  {                                                                           \
    M_F(name, _array_it_end)(it, v->array);                                   \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_set)(it_t it, const it_t org)                                 \
  {                                                                           \
    M_F(name, _array_it_set)(it, org);                                        \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _end_p)(const it_t it)                                            \
  {                                                                           \
    return M_F(name, _array_end_p)(it);                                       \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _last_p)(const it_t it)                                           \
  {                                                                           \
    return M_F(name, _array_last_p)(it);                                      \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _it_equal_p)(const it_t it1,                                      \
                         const it_t it2)                                      \
  {                                                                           \
    return M_F(name, _array_it_equal_p)(it1, it2);                            \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _next)(it_t it)                                                   \
  {                                                                           \
    M_F(name, _array_next)(it);                                               \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _previous)(it_t it)                                               \
  {                                                                           \
    M_F(name, _array_previous)(it);                                           \
  }                                                                           \
                                                                              \
  M_INLINE type const *                                                       \
  M_F(name, _cref)(const it_t it)                                             \
  {                                                                           \
    return M_F(name, _array_cref)(it);                                        \
  }                                                                           \

/* Define the IO functions */
#define M_PR1OQUEUE_DEF_IO(name, type, oplist, prioqueue_t, it_t)             \
  M_IF_METHOD(OUT_STR, oplist)(                                               \
  M_INLINE void                                                               \
  M_F(name, _out_str)(FILE *file, const prioqueue_t p)                        \
  {                                                                           \
    M_F(name, _array_out_str)(file, p->array);                                \
  }                                                                           \
  ,/* No OUT_STR */)                                                          \
                                                                              \
  M_IF_METHOD(IN_STR, oplist)(                                                \
  M_P(bool, name, _in_str, prioqueue_t p, FILE *file)                         \
  {                                                                           \
    return M_F(name, _array_in_str)M_R(p->array, file);                       \
  }                                                                           \
  ,/* No IN_STR */)                                                           \
                                                                              \
  M_IF_METHOD(GET_STR, oplist)(                                               \
  M_P(void, name, _get_str, m_string_t str, const prioqueue_t p, bool append) \
  {                                                                           \
    M_F(name, _array_get_str)M_R(str, p->array, append);                      \
  }                                                                           \
  ,/* No GET_STR */)                                                          \
                                                                              \
  M_IF_METHOD(PARSE_STR, oplist)(                                             \
  M_P(bool, name, _parse_str, prioqueue_t p, const char str[], const char **endp) \
  {                                                                           \
    return M_F(name, _array_parse_str)M_R(p->array, str, endp);               \
  }                                                                           \
  ,/* No PARSE_STR */)                                                        \
                                                                              \
  M_IF_METHOD(OUT_SERIAL, oplist)(                                            \
  M_P(m_serial_return_code_t, name, _out_serial, m_serial_write_t f, const prioqueue_t p) \
  {                                                                           \
    return M_F(name, _array_out_serial) M_R(f, p->array);                     \
  }                                                                           \
  ,/* No OUT_SERIAL */)                                                       \
                                                                              \
  M_IF_METHOD2(IN_SERIAL, INIT, oplist)(                                      \
  M_P(m_serial_return_code_t, name, _in_serial, prioqueue_t p, m_serial_read_t f) \
  {                                                                           \
    return M_F(name, _array_in_serial)M_R(p->array, f);                       \
  }                                                                           \
  ,/* No in_SERIAL */)                                                        \


// TODO: set all & remove all function

/********************************** INTERNAL *********************************/

#if M_USE_SMALL_NAME
#define PRIOQUEUE_DEF M_PRIOQUEUE_DEF
#define PRIOQUEUE_DEF_AS M_PRIOQUEUE_DEF_AS
#define PRIOQUEUE_OPLIST M_PRIOQUEUE_OPLIST
#endif

#endif
