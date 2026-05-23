/*
 * M*LIB - dynamic ARRAY module
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
#ifndef MSTARLIB_ARRAY_H
#define MSTARLIB_ARRAY_H

#include "m-core.h"

/* Define a dynamic array of the given type and its associated functions.
   USAGE: ARRAY_DEF(name, type [, oplist_of_the_type]) */
#define M_ARRAY_DEF(name, ...)                                                \
  M_ARRAY_DEF_AS(name, M_F(name,_t), M_F(name,_it_t), __VA_ARGS__)


/* Define a dynamic array of the given type and its associated functions
  as the provided type name_t with the iterator named it_t
   USAGE: ARRAY_DEF_AS(name, name_t, it_t, type [, oplist_of_the_type]) */
#define M_ARRAY_DEF_AS(name, name_t, it_t, ...)                               \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_ARRA4_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                                  \
             ((name, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), name_t, it_t ), \
              (name, __VA_ARGS__,                                        name_t, it_t ))) \
  M_END_PROTECTED_CODE


/* Define the oplist of a dynamic array given its name and its oplist.
   If no oplist is given it is assumed to be M_BASIC_OPLIST
   USAGE: ARRAY_OPLIST(name[, oplist of the type]) */
#define M_ARRAY_OPLIST(...)                                                   \
  M_ARRA4_OPLIST_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                               \
                   ((__VA_ARGS__, M_BASIC_OPLIST),                            \
                    (__VA_ARGS__ )))


/* Define an init value to init global variables of type array.
  USAGE:
    array_t global_variable = ARRAY_INIT_VALUE();
 */
#define M_ARRAY_INIT_VALUE()                                                  \
  { { 0, 0, NULL } }


/*****************************************************************************/
/********************************** INTERNAL *********************************/
/*****************************************************************************/

/* Deferred evaluation for the oplist definition,
   so that all arguments are evaluated before further expansion */
#define M_ARRA4_OPLIST_P1(arg) M_ARRA4_OPLIST_P2 arg

/* Validation of the given oplist */
#define M_ARRA4_OPLIST_P2(name, oplist)                                       \
  M_IF_OPLIST(oplist)(M_ARRA4_OPLIST_P3, M_ARRA4_OPLIST_FAILURE)(name, oplist)

/* Prepare a clean compilation failure */
#define M_ARRA4_OPLIST_FAILURE(name, oplist)                                  \
  ((M_LIB_ERROR(ARGUMENT_OF_ARRAY_OPLIST_IS_NOT_AN_OPLIST, name, oplist)))

/* OPLIST definition of a dynamic array */
/* FIXME: Do we want to export some methods as they are slow and 
   are not fit to be used for building other methods (like _it_remove)? */
#ifndef M_USE_CONTEXT
#define M_ARRA4_OPLIST_P3(name, oplist)                                       \
  (INIT(M_F(name, _init))                                                     \
   ,M_IF_METHOD2(INIT_SET,SET, oplist)(INIT_SET(M_F(name, _init_set)),)       \
   ,M_IF_METHOD(INIT_SET, oplist)(INIT_WITH(API_1(M_INIT_WITH_VAI)),)         \
   ,M_IF_METHOD2(INIT_SET,SET, oplist)(SET(M_F(name, _set)), )                \
   ,CLEAR(M_F(name, _clear))                                                  \
   ,INIT_MOVE(M_F(name, _init_move))                                          \
   ,MOVE(M_F(name, _move))                                                    \
   ,SWAP(M_F(name, _swap))                                                    \
   ,TYPE(M_F(name,_ct)) , GENTYPE(struct M_F(name,_s)*)                       \
   ,NAME(name)                                                                \
   ,SUBTYPE(M_F(name, _subtype_ct))                                           \
   ,EMPTY_P(M_F(name,_empty_p))                                               \
   ,IT_TYPE(M_F(name,_it_ct)),                                                \
   ,IT_FIRST(M_F(name,_it))                                                   \
   ,IT_LAST(M_F(name,_it_last))                                               \
   ,IT_END(M_F(name,_it_end))                                                 \
   ,IT_SET(M_F(name,_it_set))                                                 \
   ,IT_END_P(M_F(name,_end_p))                                                \
   ,IT_LAST_P(M_F(name,_last_p))                                              \
   ,IT_EQUAL_P(M_F(name,_it_equal_p))                                         \
   ,IT_NEXT(M_F(name,_next))                                                  \
   ,IT_PREVIOUS(M_F(name,_previous))                                          \
   ,IT_REF(M_F(name,_ref))                                                    \
   ,IT_CREF(M_F(name,_cref))                                                  \
   ,M_IF_METHOD(INIT_SET, oplist)(IT_INSERT(M_F(name,_insert)) ,)             \
   ,IT_REMOVE(M_F(name,_remove))                                              \
   ,RESET(M_F(name,_reset))                                                   \
   ,KEY_TYPE(size_t)                                                          \
   ,VALUE_TYPE(M_F(name, _subtype_ct))                                        \
   ,KEY_OPLIST(M_BASIC_OPLIST)                                                \
   ,VALUE_OPLIST(oplist)                                                      \
   ,M_IF_METHOD(SET, oplist)(SET_KEY(M_F(name, _set_at)) ,)                   \
   ,GET_KEY(M_F(name, _get))                                                  \
   ,M_IF_METHOD(INIT, oplist)(SAFE_GET_KEY(M_F(name, _safe_get)) ,)           \
   ,ERASE_KEY(M_F(name, _erase))                                              \
   ,GET_SIZE(M_F(name, _size))                                                \
   ,M_IF_METHOD(INIT_SET, oplist)(PUSH(M_F(name,_push_back)) ,)               \
   ,POP(M_F(name,_pop_back))                                                  \
   ,PUSH_MOVE(M_F(name,_push_move))                                           \
   ,POP_MOVE(M_F(name,_pop_move))                                             \
   ,OPLIST(oplist)                                                            \
   ,M_IF_METHOD(CMP, oplist)(SORT(M_F(name, _special_sort)),)                 \
   ,M_IF_METHOD(GET_STR, oplist)(GET_STR(M_F(name, _get_str)),)               \
   ,M_IF_METHOD(PARSE_STR, oplist)(PARSE_STR(M_F(name, _parse_str)),)         \
   ,M_IF_METHOD(OUT_STR, oplist)(OUT_STR(M_F(name, _out_str)),)               \
   ,M_IF_METHOD(IN_STR, oplist)(IN_STR(M_F(name, _in_str)),)                  \
   ,M_IF_METHOD(OUT_SERIAL, oplist)(OUT_SERIAL(M_F(name, _out_serial)),)      \
   ,M_IF_METHOD(IN_SERIAL, oplist)(IN_SERIAL(M_F(name, _in_serial)),)         \
   ,M_IF_METHOD(EQUAL, oplist)(EQUAL(M_F(name, _equal_p)),)                   \
   ,M_IF_METHOD(HASH, oplist)(HASH(M_F(name, _hash)),)                        \
   )
#else
#define M_ARRA4_OPLIST_P3(name, oplist)                                       \
  (INIT(M_F(name, _init))                                                     \
   ,M_IF_METHOD2(INIT_SET,SET, oplist)(INIT_SET(API_0P(M_F(name, _init_set))),) \
   ,M_IF_METHOD(INIT_SET, oplist)(INIT_WITH(API_1(M_INIT_WITH_VAI)),)         \
   ,M_IF_METHOD2(INIT_SET,SET, oplist)(SET(API_0P(M_F(name, _set))), )        \
   ,CLEAR(API_0P(M_F(name, _clear)))                                          \
   ,INIT_MOVE(M_F(name, _init_move))                                          \
   ,MOVE(API_0P(M_F(name, _move)))                                            \
   ,SWAP(M_F(name, _swap))                                                    \
   ,TYPE(M_F(name,_ct)) , GENTYPE(struct M_F(name,_s)*)                       \
   ,NAME(name)                                                                \
   ,SUBTYPE(M_F(name, _subtype_ct))                                           \
   ,EMPTY_P(M_F(name,_empty_p))                                               \
   ,IT_TYPE(M_F(name,_it_ct)),                                                \
   ,IT_FIRST(M_F(name,_it))                                                   \
   ,IT_LAST(M_F(name,_it_last))                                               \
   ,IT_END(M_F(name,_it_end))                                                 \
   ,IT_SET(M_F(name,_it_set))                                                 \
   ,IT_END_P(M_F(name,_end_p))                                                \
   ,IT_LAST_P(M_F(name,_last_p))                                              \
   ,IT_EQUAL_P(M_F(name,_it_equal_p))                                         \
   ,IT_NEXT(M_F(name,_next))                                                  \
   ,IT_PREVIOUS(M_F(name,_previous))                                          \
   ,IT_REF(M_F(name,_ref))                                                    \
   ,IT_CREF(M_F(name,_cref))                                                  \
   ,M_IF_METHOD(INIT_SET, oplist)(IT_INSERT(API_0P(M_F(name,_insert))) ,)     \
   ,IT_REMOVE(API_0P(M_F(name,_remove)))                                      \
   ,RESET(API_0P(M_F(name,_reset)))                                           \
   ,KEY_TYPE(size_t)                                                          \
   ,VALUE_TYPE(M_F(name, _subtype_ct))                                        \
   ,KEY_OPLIST(M_BASIC_OPLIST)                                                \
   ,VALUE_OPLIST(oplist)                                                      \
   ,M_IF_METHOD(SET, oplist)(SET_KEY(API_0P(M_F(name, _set_at))) ,)           \
   ,GET_KEY(M_F(name, _get))                                                  \
   ,M_IF_METHOD(INIT, oplist)(SAFE_GET_KEY(API_0P(M_F(name, _safe_get))) ,)   \
   ,ERASE_KEY(API_0P(M_F(name, _erase)))                                      \
   ,GET_SIZE(M_F(name, _size))                                                \
   ,M_IF_METHOD(INIT_SET, oplist)(PUSH(API_0P(M_F(name,_push_back))) ,)       \
   ,POP(API_0P(M_F(name,_pop_back)))                                          \
   ,PUSH_MOVE(API_0P(M_F(name,_push_move)))                                   \
   ,POP_MOVE(M_F(name,_pop_move))                                             \
   ,OPLIST(oplist)                                                            \
   ,M_IF_METHOD(CMP, oplist)(SORT(M_F(name, _special_sort)),)                 \
   ,M_IF_METHOD(GET_STR, oplist)(GET_STR(API_0P(M_F(name, _get_str))),)       \
   ,M_IF_METHOD(PARSE_STR, oplist)(PARSE_STR(API_0P(M_F(name, _parse_str))),) \
   ,M_IF_METHOD(OUT_STR, oplist)(OUT_STR(M_F(name, _out_str)),)               \
   ,M_IF_METHOD(IN_STR, oplist)(IN_STR(API_0P(M_F(name, _in_str))),)          \
   ,M_IF_METHOD(OUT_SERIAL, oplist)(OUT_SERIAL(API_0P(M_F(name, _out_serial))),) \
   ,M_IF_METHOD(IN_SERIAL, oplist)(IN_SERIAL(API_0P(M_F(name, _in_serial))),) \
   ,M_IF_METHOD(EQUAL, oplist)(EQUAL(M_F(name, _equal_p)),)                   \
   ,M_IF_METHOD(HASH, oplist)(HASH(M_F(name, _hash)),)                        \
   )
#endif

/********************************** INTERNAL *********************************/

/* Define the internal contract of an array */
#ifdef NDEBUG
#define M_ARRA4_CONTRACT(a)
#else
#define M_ARRA4_CONTRACT(a) do {					                                         \
    M_ASSERT (a != NULL);                                                     \
    M_ASSERT (a->size <= a->alloc);                                           \
    M_ASSERT (a->size == 0 || a->ptr != NULL);                                \
    M_ASSERT (a->alloc == 0 || a->ptr != NULL);                               \
  } while (0)
#endif

/* Deferred evaluation for the array definition,
   so that all arguments are fully evaluated before further expansion
   (ensuring good performance)
   and performed a final step evaluation of the returning expansion
   (ensuring delayed evaluation are still expanded)
*/
#define M_ARRA4_DEF_P1(arg) M_ID( M_ARRA4_DEF_P2 arg )

/* Validate the oplist before going further */
#define M_ARRA4_DEF_P2(name, type, oplist, array_t, it_t)                     \
  M_IF_OPLIST(oplist)(M_ARRA4_DEF_P3, M_ARRA4_DEF_FAILURE)(name, type, oplist, array_t, it_t)

/* Stop processing with a compilation failure */
#define M_ARRA4_DEF_FAILURE(name, type, oplist, array_t, it_t)                \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST, "(ARRAY_DEF): the given argument is not a valid oplist: " #oplist)

/* Internal definition:
   - name: prefix to be used
   - type: type of the elements of the array
   - oplist: oplist of the type of the elements of the array
   - array_t: alias for the type of the array
   - it_t: alias for the iterator of the array
*/
#define M_ARRA4_DEF_P3(name, type, oplist, array_t, it_t)                     \
  M_ARRA4_DEF_TYPE(name, type, oplist, array_t, it_t)                         \
  M_CHECK_COMPATIBLE_OPLIST(name, 1, type, oplist)                            \
  M_ARRA4_DEF_CORE(name, type, oplist, array_t, it_t)                         \
  M_ARRA4_DEF_IO(name, type, oplist, array_t, it_t)                           \
  M_EMPLACE_QUEUE_DEF(name, array_t, _emplace_back, oplist, M_ARRA4_EMPLACE_DEF)

/* Define the types */
#define M_ARRA4_DEF_TYPE(name, type, oplist, array_t, it_t)                   \
                                                                              \
  /* Define a dynamic array */                                                \
  typedef struct M_F(name, _s) {                                              \
    size_t size;            /* Number of elements in the array */             \
    size_t alloc;           /* Allocated size for the array base */           \
    type *ptr;              /* Pointer to the array base */                   \
  } array_t[1];                                                               \
                                                                              \
  /* Define an iterator over an array */                                      \
  typedef struct M_F(name, _it_s) {                                           \
    size_t index;                       /* Index of the element */            \
    const struct M_F(name, _s) *array;  /* Reference of the array */          \
  } it_t[1];                                                                  \
                                                                              \
  /* Definition of the synonyms of the type */                                \
  typedef struct M_F(name, _s) *M_F(name, _ptr);                              \
  typedef const struct M_F(name, _s) *M_F(name, _srcptr);                     \
  typedef array_t M_F(name, _ct);                                             \
  typedef it_t M_F(name, _it_ct);                                             \
  typedef type M_F(name, _subtype_ct);                                        \

/* Define the core functions */
#define M_ARRA4_DEF_CORE(name, type, oplist, array_t, it_t)                   \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _init)(array_t v)                                                 \
  {                                                                           \
    M_ASSERT (v != NULL);                                                     \
    /* Initially, the array is empty with nothing allocated */                \
    v->size  = 0;                                                             \
    v->alloc = 0;                                                             \
    v->ptr   = NULL;                                                          \
    M_ARRA4_CONTRACT(v);                                                      \
  }                                                                           \
                                                                              \
  M_P(void, name, _reset, array_t v)                                          \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    M_UNUSED_CONTEXT();                                                       \
    for(size_t i = 0; i < v->size; i++)                                       \
      M_CALL_CLEAR(oplist, v->ptr[i]);                                        \
    v->size = 0;                                                              \
    M_ARRA4_CONTRACT(v);                                                      \
  }                                                                           \
                                                                              \
  M_P(void, name, _clear, array_t v)                                          \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    M_F(name, _reset) M_R(v);                                                 \
    M_CALL_FREE(oplist, type, v->ptr, v->alloc);                              \
    /* This is so reusing the object implies an assertion failure */          \
    v->alloc = 1;                                                             \
    v->ptr = NULL;                                                            \
  }                                                                           \
                                                                              \
  M_IF_METHOD2(INIT_SET, SET, oplist)(                                        \
  M_P(void, name, _set, array_t d, const array_t s)                           \
  {                                                                           \
    M_ARRA4_CONTRACT(d);                                                      \
    M_ARRA4_CONTRACT(s);                                                      \
    if (M_UNLIKELY (d == s)) return;                                          \
    if (s->size > d->alloc) {                                                 \
      const size_t alloc = s->size;                                           \
      type *ptr = M_CALL_REALLOC(oplist, type, d->ptr, d->alloc, alloc);      \
      if (M_UNLIKELY_NOMEM (ptr == NULL)) {                                   \
        M_MEMORY_FULL(type, alloc);                                           \
      }                                                                       \
      d->ptr = ptr;                                                           \
      d->alloc = alloc;                                                       \
    }                                                                         \
    size_t i;                                                                 \
    size_t step1 = M_MIN(s->size, d->size);                                   \
    for(i = 0; i < step1; i++)                                                \
      M_CALL_SET(oplist, d->ptr[i], s->ptr[i]);                               \
    for( ; i < d->size; i++)                                                  \
      M_CALL_CLEAR(oplist, d->ptr[i]);                                        \
    for( ; i < s->size; i++) {                                                \
      M_CALL_INIT_SET(oplist, d->ptr[i], s->ptr[i]);                          \
      M_IF_EXCEPTION( d->size = i + 1 );                                      \
    }                                                                         \
    d->size = s->size;                                                        \
    M_ARRA4_CONTRACT(d);                                                      \
  }                                                                           \
                                                                              \
  M_P(void, name, _init_set, array_t d, const array_t s)                      \
  {                                                                           \
    M_ASSERT (d != s);                                                        \
    M_ON_EXCEPTION(M_F(name, _clear) M_R(d) ) {                               \
      M_F(name, _init)(d);                                                    \
      M_F(name, _set) M_R(d, s);                                              \
    }                                                                         \
  }                                                                           \
  , /* No SET & INIT_SET */)                                                  \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _init_move)(array_t d, array_t s)                                 \
  {                                                                           \
    M_ASSERT (d != s);                                                        \
    M_ARRA4_CONTRACT(s);                                                      \
    d->size  = s->size;                                                       \
    d->alloc = s->alloc;                                                      \
    d->ptr   = s->ptr;                                                        \
    /* Robustness */                                                          \
    s->alloc = 1;                                                             \
    s->ptr   = NULL;                                                          \
    M_ARRA4_CONTRACT(d);                                                      \
  }                                                                           \
                                                                              \
  M_P(void, name, _move, array_t d, array_t s)                                \
  {                                                                           \
    M_ASSERT (d != s);                                                        \
    M_F(name, _clear) M_R(d);                                                 \
    M_F(name, _init_move)(d, s);                                              \
  }                                                                           \
                                                                              \
  M_IF_METHOD(SET, oplist)(                                                   \
  M_P(void, name, _set_at, array_t v, size_t i, type const x)                 \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    M_UNUSED_CONTEXT();                                                       \
    M_ASSERT(v->size > 0 && v->ptr != NULL);                                  \
    M_ASSERT_INDEX(i, v->size);                                               \
    M_CALL_SET(oplist, v->ptr[i], x);                                         \
  }                                                                           \
  , /* No SET */)                                                             \
                                                                              \
  M_INLINE type  *                                                            \
  M_F(name, _back)(array_t v)                                                 \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    M_ASSERT(v->ptr != NULL);                                                 \
    M_ASSERT_INDEX(0, v->size);                                               \
    return &v->ptr[v->size-1];                                                \
  }                                                                           \
                                                                              \
  M_P(type *, name, _push_back_raw, array_t v)                                \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    if (M_UNLIKELY (v->size >= v->alloc)) {                                   \
      M_ASSERT(v->size == v->alloc);                                          \
      size_t alloc = M_CALL_INC_ALLOC(oplist, v->alloc);                      \
      if (M_UNLIKELY_NOMEM (alloc <= v->alloc)) {                             \
        M_MEMORY_FULL(type, -(size_t)1);                                      \
      }                                                                       \
      M_ASSERT (alloc > v->size);                                             \
      type *ptr = M_CALL_REALLOC(oplist, type, v->ptr, v->alloc, alloc);      \
      if (M_UNLIKELY_NOMEM (ptr == NULL) ) {                                  \
        M_MEMORY_FULL(type, alloc);                                           \
      }                                                                       \
      v->ptr = ptr;                                                           \
      v->alloc = alloc;                                                       \
    }                                                                         \
    M_ASSERT(v->ptr != NULL);                                                 \
    type *ret = &v->ptr[v->size];                                             \
    v->size++;                                                                \
    M_ARRA4_CONTRACT(v);                                                      \
    M_ASSUME(ret != NULL);                                                    \
    return ret;                                                               \
  }                                                                           \
                                                                              \
  M_IF_METHOD(INIT_SET, oplist)(                                              \
  M_P(void, name, _push_back, array_t v, type const x)                        \
  {                                                                           \
    type *data = M_F(name, _push_back_raw) M_R(v);                            \
    if (M_UNLIKELY (data == NULL) )                                           \
      return;                                                                 \
    M_IF_EXCEPTION( v->size --);                                              \
      M_CALL_INIT_SET(oplist, *data, x);                                      \
    M_IF_EXCEPTION( v->size ++);                                              \
  }                                                                           \
  , /* No INIT_SET */ )                                                       \
                                                                              \
  M_IF_METHOD(INIT, oplist)(                                                  \
  M_P(type *, name, _push_new, array_t v)                                     \
  {                                                                           \
    type *data = M_F(name, _push_back_raw) M_R(v);                            \
    if (M_UNLIKELY (data == NULL) )                                           \
      return NULL;                                                            \
    M_IF_EXCEPTION( v->size --);                                              \
    M_CALL_INIT(oplist, *data);                                               \
    M_IF_EXCEPTION( v->size ++);                                              \
    return data;                                                              \
  }                                                                           \
  , /* No INIT */ )                                                           \
                                                                              \
  M_P(void, name, _push_move, array_t v, type *x)                             \
  {                                                                           \
    M_ASSERT (x != NULL);                                                     \
    type *data = M_F(name, _push_back_raw) M_R(v);                            \
    if (M_UNLIKELY (data == NULL) )                                           \
      return;                                                                 \
    M_CALL_INIT_MOVE (oplist, *data, *x);                                     \
  }                                                                           \
                                                                              \
  M_IF_METHOD(INIT_SET, oplist)(                                              \
  M_P(void, name, _push_at, array_t v, size_t key, type const x)              \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    M_ASSERT_INDEX(key, v->size+1);                                           \
    if (M_UNLIKELY (v->size >= v->alloc) ) {                                  \
      M_ASSERT(v->size == v->alloc);                                          \
      size_t alloc = M_CALL_INC_ALLOC(oplist, v->alloc);                      \
      if (M_UNLIKELY_NOMEM (alloc <= v->alloc)) {                             \
        M_MEMORY_FULL(type, -(size_t)1);                                      \
      }                                                                       \
      M_ASSERT (alloc > v->size);                                             \
      type *ptr = M_CALL_REALLOC(oplist, type, v->ptr, v->alloc, alloc);      \
      if (M_UNLIKELY_NOMEM (ptr == NULL) ) {                                  \
        M_MEMORY_FULL(type, alloc);                                           \
      }                                                                       \
      v->ptr = ptr;                                                           \
      v->alloc = alloc;                                                       \
    }                                                                         \
    M_ASSERT(v->ptr != NULL);                                                 \
    memmove(&v->ptr[key+1], &v->ptr[key], (v->size-key)*sizeof(type));        \
    M_ON_EXCEPTION( memmove(&v->ptr[key], &v->ptr[v->size], sizeof(type))) {  \
      M_CALL_INIT_SET(oplist, v->ptr[key], x);                                \
    }                                                                         \
    v->size++;                                                                \
    M_ARRA4_CONTRACT(v);                                                      \
  }                                                                           \
  , /* No INIT_SET */ )                                                       \
                                                                              \
  M_IF_METHOD(INIT, oplist)(                                                  \
  M_P(void, name, _resize, array_t v, size_t size)                            \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    if (v->size > size) {                                                     \
      /* Decrease size of array */                                            \
      for(size_t i = size ; i < v->size; i++)                                 \
        M_CALL_CLEAR(oplist, v->ptr[i]);                                      \
      v->size = size;                                                         \
    } else if (v->size < size) {                                              \
      /* Increase size of array */                                            \
      if (size > v->alloc) {                                                  \
        size_t alloc = size ;                                                 \
        type *ptr = M_CALL_REALLOC(oplist, type, v->ptr, v->alloc, alloc);    \
        if (M_UNLIKELY_NOMEM (ptr == NULL) ) {                                \
          M_MEMORY_FULL(type, alloc);                                         \
        }                                                                     \
        v->ptr = ptr;                                                         \
        v->alloc = alloc;                                                     \
      }                                                                       \
      for(size_t i = v->size ; i < size; i++) {                               \
        M_CALL_INIT(oplist, v->ptr[i]);                                       \
        M_IF_EXCEPTION( v->size = i+1);                                       \
      }                                                                       \
      v->size = size;                                                         \
    }                                                                         \
    M_ARRA4_CONTRACT(v);                                                      \
  }                                                                           \
  , /* No INIT */ )                                                           \
                                                                              \
  M_P(void, name, _reserve, array_t v, size_t alloc)                          \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    /* NOTE: Reserve below needed size to perform a shrink to fit */          \
    if (v->size > alloc) {                                                    \
      alloc = v->size;                                                        \
    }                                                                         \
    if (M_UNLIKELY (alloc == 0)) {                                            \
      M_CALL_FREE(oplist, type, v->ptr, v->alloc);                            \
      v->size = v->alloc = 0;                                                 \
      v->ptr = NULL;                                                          \
    } else {                                                                  \
      type *ptr = M_CALL_REALLOC(oplist, type, v->ptr, v->alloc, alloc);      \
      if (M_UNLIKELY_NOMEM (ptr == NULL) ) {                                  \
        M_MEMORY_FULL(type, alloc);                                           \
      }                                                                       \
      v->ptr = ptr;                                                           \
      v->alloc = alloc;                                                       \
    }                                                                         \
    M_ARRA4_CONTRACT(v);                                                      \
  }                                                                           \
                                                                              \
  M_IF_METHOD(INIT, oplist)(                                                  \
  M_P(type *, name, _safe_get, array_t v, size_t idx)                         \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    const size_t size = idx + 1;                                              \
    /* resize if needed */                                                    \
    if (v->size <= size) {                                                    \
      /* Increase size of array */                                            \
      if (M_UNLIKELY (size > v->alloc) ) {                                    \
        size_t alloc = M_CALL_INC_ALLOC(oplist, size) ;                       \
        /* In case of overflow of size_t */                                   \
        if (M_UNLIKELY_NOMEM (alloc <= v->alloc)) {                           \
          M_MEMORY_FULL(type, -(size_t)1);                                    \
        }                                                                     \
        type *ptr = M_CALL_REALLOC(oplist, type, v->ptr, v->alloc, alloc);    \
        if (M_UNLIKELY_NOMEM (ptr == NULL) ) {                                \
          M_MEMORY_FULL(type, alloc);                                         \
        }                                                                     \
        v->ptr = ptr;                                                         \
        v->alloc = alloc;                                                     \
      }                                                                       \
      for(size_t i = v->size ; i < size; i++) {                               \
        M_CALL_INIT(oplist, v->ptr[i]);                                       \
        M_IF_EXCEPTION( v->size = i+1);                                       \
      }                                                                       \
      v->size = size;                                                         \
    }                                                                         \
    M_ASSERT (idx < v->size);                                                 \
    M_ARRA4_CONTRACT(v);                                                      \
    return &v->ptr[idx];                                                      \
  }                                                                           \
  , /* No INIT */)                                                            \
                                                                              \
  M_P(void, name, _pop_back, type *dest, array_t v)                           \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    M_UNUSED_CONTEXT();                                                       \
    M_ASSERT (v->ptr != NULL);                                                \
    M_ASSERT_INDEX(0, v->size);                                               \
    v->size--;                                                                \
    if (dest) {                                                               \
      M_DO_MOVE (oplist, *dest, v->ptr[v->size]);                             \
    } else {                                                                  \
      M_CALL_CLEAR(oplist, v->ptr[v->size]);                                  \
    }                                                                         \
    M_ARRA4_CONTRACT(v);                                                      \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _pop_move)(type *dest, array_t v)                                 \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    M_ASSERT (v->ptr != NULL);                                                \
    M_ASSERT_INDEX(0, v->size);                                               \
    M_ASSERT (dest != NULL);                                                  \
    v->size--;                                                                \
    M_CALL_INIT_MOVE (oplist, *dest, v->ptr[v->size]);                        \
    M_ARRA4_CONTRACT(v);                                                      \
  }                                                                           \
                                                                              \
  /* TODO: Factorize all INIT dependent methods within the same IF*/          \
  M_IF_METHOD(INIT, oplist)(                                                  \
  M_P(void, name, _pop_until, array_t v, it_t pos)                            \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    M_ASSERT (v == pos->array);                                               \
    M_ASSERT_INDEX(pos->index, v->size+1);                                    \
    M_F(name, _resize) M_R(v, pos->index);                                    \
  }                                                                           \
  , /* No INIT */ )                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _empty_p)(const array_t v)                                        \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    return v->size == 0;                                                      \
  }                                                                           \
                                                                              \
  M_INLINE size_t                                                             \
  M_F(name, _size)(const array_t v)                                           \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    return v->size;                                                           \
  }                                                                           \
                                                                              \
  M_INLINE size_t                                                             \
  M_F(name, _capacity)(const array_t v)                                       \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    return v->alloc;                                                          \
  }                                                                           \
                                                                              \
  M_P(void, name, _pop_at, type *dest, array_t v, size_t i)                   \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    M_UNUSED_CONTEXT();                                                       \
    M_ASSERT (v->size > 0 && v->ptr != NULL);                                 \
    M_ASSERT_INDEX(i, v->size);                                               \
    if (dest)                                                                 \
      M_DO_MOVE (oplist, *dest, v->ptr[i]);                                   \
    else                                                                      \
      M_CALL_CLEAR(oplist, v->ptr[i]);                                        \
    memmove(&v->ptr[i], &v->ptr[i+1], sizeof(type)*(v->size-1-i));            \
    v->size--;                                                                \
    M_ARRA4_CONTRACT(v);                                                      \
  }                                                                           \
                                                                              \
  M_P(bool, name, _erase, array_t a, size_t i)                                \
  {                                                                           \
    M_ARRA4_CONTRACT(a);                                                      \
    if (i >= a->size) return false;                                           \
    M_F(name, _pop_at) M_R(NULL, a, i);                                       \
    return true;                                                              \
  }                                                                           \
                                                                              \
  M_IF_METHOD(INIT, oplist)(                                                  \
  M_P(void, name, _insert_v, array_t v, size_t i, size_t num)                 \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    M_ASSERT_INDEX(i, v->size+1);                                             \
    size_t size = v->size + num;                                              \
    /* Test for overflow of variable size */                                  \
    if (M_UNLIKELY_NOMEM (size <= v->size)) {                                 \
      /* Unlikely case of nothing to do */                                    \
      if (num == 0) return;                                                   \
      M_MEMORY_FULL(type, v->size);                                           \
    }                                                                         \
    /* Test if alloc array is sufficient */                                   \
    if (size > v->alloc) {                                                    \
      size_t alloc = M_CALL_INC_ALLOC(oplist, size) ;                         \
      if (M_UNLIKELY_NOMEM (alloc <= v->alloc)) {                             \
        M_MEMORY_FULL(type, -(size_t)1);                                      \
      }                                                                       \
      type *ptr = M_CALL_REALLOC(oplist, type, v->ptr, v->alloc, alloc);      \
      if (M_UNLIKELY_NOMEM (ptr == NULL) ) {                                  \
        M_MEMORY_FULL(type, alloc);                                           \
      }                                                                       \
      v->ptr = ptr;                                                           \
      v->alloc = alloc;                                                       \
    }                                                                         \
    memmove(&v->ptr[i+num], &v->ptr[i], sizeof(type)*(v->size - i) );         \
    m_volatile size_t k;                                                      \
    M_ON_EXCEPTION(memmove(&v->ptr[k], &v->ptr[i+num], sizeof(type)*(v->size - i) ), v->size += (k-i) ) { \
      for(k = i ; k < i+num; k++)                                             \
        M_CALL_INIT(oplist, v->ptr[k]);                                       \
    }                                                                         \
    v->size = size;                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
  }                                                                           \
  , /* No INIT */)                                                            \
                                                                              \
  M_P(void, name, _remove_v, array_t v, size_t i, size_t j)                   \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    M_UNUSED_CONTEXT();                                                       \
    M_ASSERT(i < j && v->ptr != NULL);                                        \
    M_ASSERT_INDEX(i, v->size);                                               \
    M_ASSERT_INDEX(j, v->size+1);                                             \
    for(size_t k = i ; k < j; k++)                                            \
      M_CALL_CLEAR(oplist, v->ptr[k]);                                        \
    memmove(&v->ptr[i], &v->ptr[j], sizeof(type)*(v->size - j) );             \
    v->size -= (j-i);                                                         \
    M_ARRA4_CONTRACT(v);                                                      \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _swap)(array_t v1, array_t v2)                                    \
  {                                                                           \
    M_ARRA4_CONTRACT(v1);                                                     \
    M_ARRA4_CONTRACT(v2);                                                     \
    M_SWAP(size_t, v1->size, v2->size);                                       \
    M_SWAP(size_t, v1->alloc, v2->alloc);                                     \
    M_SWAP(type *, v1->ptr, v2->ptr);                                         \
    M_ARRA4_CONTRACT(v1);                                                     \
    M_ARRA4_CONTRACT(v2);                                                     \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _swap_at)(array_t v, size_t i, size_t j)                          \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    M_ASSERT(v->ptr != NULL);                                                 \
    M_ASSERT_INDEX(i, v->size);                                               \
    M_ASSERT_INDEX(j, v->size);                                               \
    type tmp;                                                                 \
    M_CALL_INIT_MOVE(oplist, tmp, v->ptr[i]);                                 \
    M_CALL_INIT_MOVE(oplist, v->ptr[i], v->ptr[j]);                           \
    M_CALL_INIT_MOVE(oplist, v->ptr[j], tmp);                                 \
    M_ARRA4_CONTRACT(v);                                                      \
  }                                                                           \
                                                                              \
  M_INLINE type *                                                             \
  M_F(name, _get)(const array_t v, size_t i)                                  \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    M_ASSERT (v->ptr != NULL);                                                \
    M_ASSERT_INDEX(i, v->size);                                               \
    return &v->ptr[i];                                                        \
  }                                                                           \
                                                                              \
  M_INLINE type const *                                                       \
  M_F(name, _cget)(const array_t v, size_t i)                                 \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    M_ASSERT (v->ptr != NULL);                                                \
    M_ASSERT_INDEX(i, v->size);                                               \
    return M_CONST_CAST(type, &v->ptr[i]);                                    \
  }                                                                           \
                                                                              \
  M_INLINE type *                                                             \
  M_F(name, _front)(const array_t v)                                          \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    M_ASSERT_INDEX(0, v->size);                                               \
    return M_F(name, _get)(v, 0);                                             \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it)(it_t it, const array_t v)                                    \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    M_ASSERT (it != NULL);                                                    \
    it->index = 0;                                                            \
    it->array = v;                                                            \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_last)(it_t it, const array_t v)                               \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    M_ASSERT (it != NULL);                                                    \
    /* If size is 0, index is -1 as unsigned, so it is greater than end */    \
    it->index = v->size - 1;                                                  \
    it->array = v;                                                            \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_end)(it_t it, const array_t v)                                \
  {                                                                           \
    M_ARRA4_CONTRACT(v);                                                      \
    M_ASSERT (it != NULL);                                                    \
    it->index = v->size;                                                      \
    it->array = v;                                                            \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_set)(it_t it, const it_t org)                                 \
  {                                                                           \
    M_ASSERT (it != NULL && org != NULL);                                     \
    it->index = org->index;                                                   \
    it->array = org->array;                                                   \
    M_ARRA4_CONTRACT(it->array);                                              \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _end_p)(const it_t it)                                            \
  {                                                                           \
    M_ASSERT(it != NULL && it->array != NULL);                                \
    return it->index >= it->array->size;                                      \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _last_p)(const it_t it)                                           \
  {                                                                           \
    M_ASSERT(it != NULL && it->array != NULL);                                \
    /* NOTE: Can not compute 'size-1' due to potential overflow               \
       if size is 0 */                                                        \
    return it->index + 1 >= it->array->size;                                  \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _it_equal_p)(const it_t it1,                                      \
                         const it_t it2)                                      \
  {                                                                           \
    M_ASSERT(it1 != NULL && it2 != NULL);                                     \
    return it1->array == it2->array && it1->index == it2->index;              \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _next)(it_t it)                                                   \
  {                                                                           \
    M_ASSERT(it != NULL && it->array != NULL);                                \
    it->index ++;                                                             \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _previous)(it_t it)                                               \
  {                                                                           \
    M_ASSERT(it != NULL && it->array != NULL);                                \
    /* NOTE: In the case index=0, it will be set to (unsigned) -1             \
       ==> it will be greater than size ==> end_p will return true */         \
    it->index --;                                                             \
  }                                                                           \
                                                                              \
  M_INLINE type *                                                             \
  M_F(name, _ref)(const it_t it)                                              \
  {                                                                           \
    M_ASSERT(it != NULL);                                                     \
    return M_F(name, _get)(it->array, it->index);                             \
  }                                                                           \
                                                                              \
  M_INLINE type const *                                                       \
  M_F(name, _cref)(const it_t it)                                             \
  {                                                                           \
    M_ASSERT(it != NULL);                                                     \
    return M_F(name, _cget)(it->array, it->index);                            \
  }                                                                           \
                                                                              \
  M_IF_METHOD(INIT_SET, oplist)(                                              \
  M_P(void, name, _insert, array_t a, it_t it, type const x)                  \
  {                                                                           \
    M_ASSERT (it != NULL && a == it->array);                                  \
    size_t index = M_F(name, _end_p)(it) ? 0 : it->index+1;                   \
    M_F(name, _push_at)M_R(a, index, x);                                      \
    it->index = index;                                                        \
  }                                                                           \
  , /* End of INIT_SET */ )                                                   \
                                                                              \
  M_IF_AT_LEAST_METHOD(SET, INIT_MOVE, oplist)(                               \
  M_P(void, name, _remove, array_t a, it_t it)                                \
  {                                                                           \
    M_ASSERT (it != NULL && a == it->array);                                  \
    M_F(name, _pop_at) M_R(NULL, a, it->index);                               \
    /* NOTE: it->index will naturally point to the next element */            \
  }                                                                           \
  , /* End of SET | INIT_SET */ )                                             \
                                                                              \
   M_INLINE void M_F(name, _special_sort)(array_t l,                          \
              int (*func_type) (type const *a, type const *b))                \
  {                                                                           \
    /* Using qsort is more compact but slower than a full templated           \
       version which can be twice faster */                                   \
    int (*func_void)(const void*, const void*);                               \
    /* There is no way (?) to avoid the cast */                               \
    func_void = (int (*)(const void*, const void*))func_type;                 \
    qsort (l->ptr, l->size, sizeof(type), func_void);                         \
  }                                                                           \
                                                                              \
  M_IF_METHOD3(SWAP, SET, CMP, oplist)(                                       \
  M_INLINE void                                                               \
  M_C3(m_arra4_,name,_stable_sort_noalloc)(type tab[], size_t size, type tmp[]) \
  {                                                                           \
    size_t th = 4;                                                            \
    M_IF_DEBUG(type *org_tab = tab;)                                          \
    M_ASSERT (size > 1);                                                      \
    /* Let's select the threshold of the pass 1 to be sure                    \
       the final result is in tab.*/                                          \
    if (m_core_clz64(size-1) & 1)                                             \
      th += th;                                                               \
                                                                              \
    /* Pass 1: Partial insertion sort (stable) up to 'th' size */             \
    for(size_t k = 0 ; k < size; ) {                                          \
      size_t max = size - k < 2*th ? size - k : th;                           \
      for(size_t i = 1; i < max; i++) {                                       \
        size_t j = i;                                                         \
        while (j > 0 && M_CALL_CMP(oplist, tab[k+j-1], tab[k+j]) > 0) {       \
          M_CALL_SWAP(oplist, tab[k+j-1], tab[k+j]);                          \
          j = j - 1;                                                          \
        }                                                                     \
      }                                                                       \
      k += max;                                                               \
    }                                                                         \
                                                                              \
    /* N Pass of merge sort (stable) */                                       \
    while (th < size) {                                                       \
      type *dest = tmp;                                                       \
      /* Pass n: Merge 2 sections of 'th' elements */                         \
      for(size_t k = 0 ; k < size; ) {                                        \
        type *el1 = &tab[k];                                                  \
        type *el2 = &tab[k+th];                                               \
        size_t n1 = th;                                                       \
        size_t n2 = size-k <= 3*th ? size-k-th : th;                          \
        M_ASSERT (size-k > th);                                               \
        M_ASSERT (0 < n1 && n1 <= size);                                      \
        M_ASSERT (0 < n2 && n2 <= size);                                      \
        k += n1+n2;                                                           \
        for (;;) {                                                            \
          if (M_CALL_CMP(oplist, *el1, *el2) <= 0) {                          \
            M_CALL_INIT_MOVE(oplist, *dest, *el1);                            \
            dest++;                                                           \
            el1++;                                                            \
            if (M_UNLIKELY(-- n1 == 0)) {                                     \
              M_ASSERT (n2 > 0);                                              \
              memcpy (dest, el2, n2 * sizeof (type));                         \
              dest += n2;                                                     \
              break;                                                          \
            }                                                                 \
          } else {                                                            \
            M_CALL_INIT_MOVE(oplist, *dest, *el2);                            \
            dest++;                                                           \
            el2++;                                                            \
            if (M_UNLIKELY(-- n2 == 0)) {                                     \
              M_ASSERT (n1 > 0);                                              \
              memcpy (dest, el1, n1 * sizeof (type));                         \
              dest += n1;                                                     \
              break;                                                          \
            }                                                                 \
          }                                                                   \
        }                                                                     \
      }                                                                       \
      /* Swap t & tab */                                                      \
      M_SWAP(type *, tab, tmp);                                               \
      /* Increase th for next pass */                                         \
      th += th;                                                               \
    }                                                                         \
    M_ASSERT (org_tab == tab);                                                \
  }                                                                           \
                                                                              \
  M_P(void, name, _special_stable_sort, array_t l)                            \
  {                                                                           \
    if (M_UNLIKELY (l->size < 2))                                             \
      return;                                                                 \
    /* NOTE: if size is <= 4, no need to perform an allocation */             \
    type *temp = M_CALL_REALLOC(oplist, type, NULL, 0, l->size);              \
    if (M_UNLIKELY_NOMEM (temp == NULL)) {                                    \
      M_MEMORY_FULL(type, l->size);                                           \
    }                                                                         \
    M_C3(m_arra4_,name,_stable_sort_noalloc)(l->ptr, l->size, temp);          \
    M_CALL_FREE(oplist, type, temp, l->size);                                 \
  }                                                                           \
  ,) /* IF SWAP & SET & CMP operators */                                      \
                                                                              \
  M_IF_METHOD(EQUAL, oplist)(                                                 \
  M_INLINE bool                                                               \
  M_F(name, _equal_p)(const array_t array1,                                   \
                      const array_t array2)                                   \
  {                                                                           \
    M_ARRA4_CONTRACT(array1);                                                 \
    M_ARRA4_CONTRACT(array2);                                                 \
    if (array1->size != array2->size) return false;                           \
    size_t i;                                                                 \
    for(i = 0; i < array1->size; i++) {                                       \
      type const *item1 = M_F(name, _cget)(array1, i);                        \
      type const *item2 = M_F(name, _cget)(array2, i);                        \
      bool b = M_CALL_EQUAL(oplist, *item1, *item2);                          \
      if (!b) return false;                                                   \
    }                                                                         \
    return true;                                                              \
  }                                                                           \
  , /* no EQUAL */ )                                                          \
                                                                              \
  M_IF_METHOD(HASH, oplist)(                                                  \
  M_INLINE size_t                                                             \
  M_F(name, _hash)(const array_t array)                                       \
  {                                                                           \
    M_ARRA4_CONTRACT(array);                                                  \
    M_HASH_DECL(hash);                                                        \
    for(size_t i = 0 ; i < array->size; i++) {                                \
      size_t hi = M_CALL_HASH(oplist, array->ptr[i]);                         \
      M_HASH_UP(hash, hi);                                                    \
    }                                                                         \
    return M_HASH_FINAL (hash);                                               \
  }                                                                           \
  , /* no HASH */ )                                                           \
                                                                              \
  M_P(void, name, _splice, array_t a1, array_t a2)                            \
  {                                                                           \
    M_ARRA4_CONTRACT(a1);                                                     \
    M_ARRA4_CONTRACT(a2);                                                     \
    M_ASSERT(a1 != a2);                                                       \
    if (M_LIKELY (a2->size > 0)) {                                            \
      size_t newSize = a1->size + a2->size;                                   \
      /* To overflow newSize, we need to a1 and a2 a little bit above         \
         SIZE_MAX/2, which is not possible in the classic memory model as we  \
         should have exhausted all memory before reaching such sizes. */      \
      M_ASSERT_INDEX(a1->size, newSize);                                      \
      if (newSize > a1->alloc) {                                              \
        type *ptr = M_CALL_REALLOC(oplist, type, a1->ptr, a1->alloc, newSize); \
        if (M_UNLIKELY_NOMEM (ptr == NULL) ) {                                \
          M_MEMORY_FULL(type, newSize);                                       \
        }                                                                     \
        a1->ptr = ptr;                                                        \
        a1->alloc = newSize;                                                  \
      }                                                                       \
      M_ASSERT(a1->ptr != NULL);                                              \
      M_ASSERT(a2->ptr != NULL);                                              \
      memcpy(&a1->ptr[a1->size], &a2->ptr[0], a2->size * sizeof (type));      \
      /* a2 is now empty */                                                   \
      a2->size = 0;                                                           \
      /* a1 has been expanded with the items of a2 */                         \
      a1->size = newSize;                                                     \
    }                                                                         \
  }                                                                           \

/* Define the I/O functions */
#define M_ARRA4_DEF_IO(name, type, oplist, array_t, it_t)                     \
                                                                              \
  M_IF_METHOD(GET_STR, oplist)(                                               \
  M_P(void, name, _get_str, m_string_t str, array_t const array, bool append) \
  {                                                                           \
    M_ARRA4_CONTRACT(array);                                                  \
    (append ? m_string_cat_cstr : m_string_set_cstr) M_R(str, "[");           \
    it_t it;                                                                  \
    for (M_F(name, _it)(it, array) ;                                          \
         !M_F(name, _end_p)(it);                                              \
         M_F(name, _next)(it)){                                               \
      type const *item = M_F(name, _cref)(it);                                \
      M_CALL_GET_STR(oplist, str, *item, true);                               \
      if (!M_F(name, _last_p)(it))                                            \
        m_string_push_back M_R(str, M_GET_SEPARATOR oplist);                  \
    }                                                                         \
    m_string_push_back M_R(str, ']');                                         \
  }                                                                           \
  , /* no GET_STR */ )                                                        \
                                                                              \
  M_IF_METHOD(OUT_STR, oplist)(                                               \
  M_INLINE void                                                               \
  M_F(name, _out_str)(FILE *file, const array_t array)                        \
  {                                                                           \
    M_ARRA4_CONTRACT(array);                                                  \
    M_ASSERT (file != NULL);                                                  \
    fputc ('[', file);                                                        \
    for (size_t i = 0; i < array->size; i++) {                                \
      type const *item = M_F(name, _cget)(array, i);                          \
      M_CALL_OUT_STR(oplist, file, *item);                                    \
      if (i != array->size-1)                                                 \
        fputc (M_GET_SEPARATOR oplist, file);                                 \
    }                                                                         \
    fputc (']', file);                                                        \
  }                                                                           \
  , /* no OUT_STR */ )                                                        \
                                                                              \
  M_IF_METHOD2(PARSE_STR, INIT, oplist)(                                      \
  M_P(bool, name, _parse_str, array_t array, const char str[], const char**endp) \
  {                                                                           \
    M_ARRA4_CONTRACT(array);                                                  \
    M_ASSERT (str != NULL);                                                   \
    M_F(name,_reset)M_R(array);                                               \
    int c = *str++;                                                           \
    if (M_UNLIKELY (c != '[')) { c = 0; goto exit; }                          \
    c = m_core_str_nospace(&str);                                             \
    if (M_UNLIKELY (c == ']')) { goto exit; }                                 \
    if (M_UNLIKELY (c == 0)) { goto exit; }                                   \
    str--;                                                                    \
    M_QLET(1, item, type, oplist) {                                           \
      do {                                                                    \
        bool b = M_CALL_PARSE_STR(oplist, item, str, &str);                   \
        c = m_core_str_nospace(&str);                                         \
        if (b == false || c == 0) { c = 0; break; }                           \
        M_F(name, _push_back) M_R(array, item);                               \
      } while (c == M_GET_SEPARATOR oplist);                                  \
    }                                                                         \
  exit:                                                                       \
    M_ARRA4_CONTRACT(array);                                                  \
    if (endp) *endp = str;                                                    \
    return c == ']';                                                          \
  }                                                                           \
  , /* no PARSE_STR & INIT */ )                                               \
                                                                              \
  M_IF_METHOD2(IN_STR, INIT, oplist)(                                         \
  M_P(bool, name, _in_str, array_t array, FILE *file)                         \
  {                                                                           \
    M_ARRA4_CONTRACT(array);                                                  \
    M_ASSERT (file != NULL);                                                  \
    M_F(name,_reset)M_R(array);                                               \
    int c = fgetc(file);                                                      \
    if (M_UNLIKELY (c != '[')) return false;                                  \
    c = fgetc(file);                                                          \
    if (M_UNLIKELY (c == ']')) return true;                                   \
    if (M_UNLIKELY (c == EOF)) return false;                                  \
    ungetc(c, file);                                                          \
    M_QLET(1, item, type, oplist) {                                           \
      do {                                                                    \
        bool b = M_CALL_IN_STR(oplist, item, file);                           \
        c = m_core_fgetc_nospace(file);                                       \
        if (b == false || c == EOF) { c = 0; break; }                         \
        M_F(name, _push_back) M_R(array, item);                               \
      } while (c == M_GET_SEPARATOR oplist);                                  \
    }                                                                         \
    M_ARRA4_CONTRACT(array);                                                  \
    return c == ']';                                                          \
  }                                                                           \
  , /* no IN_STR & INIT */ )                                                  \
                                                                              \
  M_IF_METHOD(OUT_SERIAL, oplist)(                                            \
  M_P(m_serial_return_code_t, name, _out_serial, m_serial_write_t f, const array_t array) \
  {                                                                           \
    M_ARRA4_CONTRACT(array);                                                  \
    M_ASSERT (f != NULL && f->m_interface != NULL);                           \
    m_serial_return_code_t ret;                                               \
    m_serial_local_t local;                                                   \
    ret = f->m_interface->write_array_start M_R(local, f, array->size);       \
    for (size_t i = 0; i < array->size; i++) {                                \
      type const *item = M_F(name, _cget)(array, i);                          \
      if (i != 0) {                                                           \
        ret |= f->m_interface->write_array_next M_R(local, f);                \
      }                                                                       \
      ret |= M_CALL_OUT_SERIAL(oplist, f, *item);                             \
    }                                                                         \
    ret |= f->m_interface->write_array_end M_R(local, f);                     \
    return ret & M_SERIAL_FAIL;                                               \
  }                                                                           \
  , /* no OUT_SERIAL */ )                                                     \
                                                                              \
  M_IF_METHOD2(IN_SERIAL, INIT, oplist)(                                      \
  M_P(m_serial_return_code_t, name, _in_serial, array_t array, m_serial_read_t f) \
  {                                                                           \
    M_ARRA4_CONTRACT(array);                                                  \
    M_ASSERT (f != NULL && f->m_interface != NULL);                           \
    m_serial_return_code_t ret;                                               \
    m_serial_local_t local;                                                   \
    size_t estimated_size = 0;                                                \
    M_F(name,_reset)M_R(array);                                               \
    ret = f->m_interface->read_array_start(local, f, &estimated_size);        \
    if (M_UNLIKELY (ret != M_SERIAL_OK_CONTINUE)) {                           \
       return ret;                                                            \
    }                                                                         \
    M_F(name, _reserve) M_R(array, estimated_size);                           \
    M_QLET(1, item, type, oplist) {                                           \
      do {                                                                    \
        ret = M_CALL_IN_SERIAL(oplist, item, f);                              \
        if (ret != M_SERIAL_OK_DONE) { break; }                               \
        M_F(name, _push_back)M_R(array, item);                                \
        ret = f->m_interface->read_array_next(local, f);                      \
      } while (ret == M_SERIAL_OK_CONTINUE);                                  \
    }                                                                         \
    M_ARRA4_CONTRACT(array);                                                  \
    return ret;                                                               \
  }                                                                           \
  , /* no IN_SERIAL & INIT */ )                                               \

/* Definition of the emplace_back function for arrays */
#define M_ARRA4_EMPLACE_DEF(name, name_t, function_name, oplist, init_func, exp_emplace_type) \
  M_P(void, name, function_name, name_t v M_EMPLACE_LIST_TYPE_VAR(a, exp_emplace_type) ) \
  {                                                                           \
    M_F(name, _subtype_ct) *data = M_F(name, _push_back_raw) M_R(v);          \
    if (M_UNLIKELY (data == NULL) )                                           \
      return;                                                                 \
    M_IF_EXCEPTION( v->size --);                                              \
    M_EMPLACE_CALL_FUNC(a, init_func, oplist, *data, exp_emplace_type);       \
    M_IF_EXCEPTION( v->size ++);                                              \
  }                                                                           \

/********************************** INTERNAL *********************************/

#if M_USE_SMALL_NAME
#define ARRAY_DEF M_ARRAY_DEF
#define ARRAY_DEF_AS M_ARRAY_DEF_AS
#define ARRAY_OPLIST M_ARRAY_OPLIST
#define ARRAY_INIT_VALUE M_ARRAY_INIT_VALUE
#endif

#endif
