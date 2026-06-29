/*
 * M*LIB - B+TREE module
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
#ifndef MSTARLIB_BPTREE_H
#define MSTARLIB_BPTREE_H

#include "m-core.h"

/* Define a B+tree of size 'N' that maps a 'key' to a 'value'
   with its associated functions.
   USAGE: 
   BPTREE_DEF2(name, N, key_t, key_oplist, value_t, value_oplist)
   OR
   BPTREE_DEF2(name, N, key_t, value_t)
*/
#define M_BPTREE_DEF2(name, N, key_type, ...)                                 \
  M_BPTREE_DEF2_AS(name, M_F(name,_t), M_F(name,_it_t), M_F(name, _itref_t), N, key_type, __VA_ARGS__)


/* Define a B+tree of size 'N' that maps a 'key' to a 'value'
   as the given name name_t with its associated functions.
   USAGE: 
   BPTREE_DEF2_AS(name, name_t, it_t, itref_t, N, key_t, key_oplist, value_t, value_oplist)
   OR
   BPTREE_DEF2_AS(name, name_t, it_t, itref_t, N, key_t, value_t)
*/
#define M_BPTREE_DEF2_AS(name, name_t, it_t, itref_t, N, key_type, ...)       \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_BPTR33_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                                 \
                 ((name, N, key_type, M_GLOBAL_OPLIST_OR_DEF(key_type)(), __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), 1, 0, name_t, M_F(name, _node_ct), M_F(name, _pit_ct), it_t, itref_t ), \
                  (name, N, key_type,                                     __VA_ARGS__,                                        1, 0, name_t, M_F(name, _node_ct), M_F(name, _pit_ct), it_t, itref_t ))) \
  M_END_PROTECTED_CODE


/* Define a B+tree of a given type, of size N.
   with its associated functions
   USAGE: BPTREE_DEF(name, N, type, [, oplist_of_the_type]) */
#define M_BPTREE_DEF(name, N, ...)                                            \
  M_BPTREE_DEF_AS(name, M_F(name,_t), M_F(name,_it_t), N, __VA_ARGS__)


/* Define a B+tree of a given type, of size N.
   as the given name name_t with its associated functions
   USAGE: BPTREE_DEF_AS(name, name_t, it_t, N, type, [, oplist_of_the_type]) */
#define M_BPTREE_DEF_AS(name, name_t, it_t, N, ...)                           \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_BPTR33_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                                 \
                 ((name, N, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), 0, 0, name_t, M_F(name, _node_ct), M_F(name, _pit_ct), it_t, M_F(name, _itref_ct)  ), \
                  (name, N, __VA_ARGS__,                                        __VA_ARGS__,                                        0, 0, name_t, M_F(name, _node_ct), M_F(name, _pit_ct), it_t, M_F(name, _itref_ct)  ))) \
  M_END_PROTECTED_CODE


/* Define a B+tree of size 'N' that maps a 'key' to a 'value',
   allowing multiple equal keys to exist,
   with its associated functions.
   USAGE: 
   BPTREE_MULTI_DEF2(name, N, key_t, key_oplist, value_t, value_oplist)
   OR
   BPTREE_MULTI_DEF2(name, N, key_t, value_t)
*/
#define M_BPTREE_MULTI_DEF2(name, N, key_type, ...)                           \
  M_BPTREE_MULTI_DEF2_AS(name, M_F(name,_t), M_F(name,_it_t), M_F(name,_itref_t), N, key_type, __VA_ARGS__)


/* Define a B+tree of size 'N' that maps a 'key' to a 'value',
   allowing multiple equal keys to exist,
   as the given name name_t with its associated functions.
   USAGE: 
   BPTREE_MULTI_DEF2_AS(name, name_t, it_t, itref_t, N, key_t, key_oplist, value_t, value_oplist)
   OR
   BPTREE_MULTI_DEF2_AS(name, name_t, it_t, itref_t, N, key_t, value_t)
*/
#define M_BPTREE_MULTI_DEF2_AS(name, name_t, it_t, itref_t, N, key_type, ...) \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_BPTR33_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                                 \
                 ((name, N, key_type, M_GLOBAL_OPLIST_OR_DEF(key_type)(), __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), 1, 1, name_t, M_F(name, _node_ct), M_F(name, _pit_ct), it_t, itref_t ), \
                  (name, N, key_type, __VA_ARGS__,                                                                            1, 1, name_t, M_F(name, _node_ct), M_F(name, _pit_ct), it_t, itref_t ))) \
  M_END_PROTECTED_CODE


/* Define a B+tree of a given type, of size N.
   allowing multiple equal keys to exist,
   with its associated functions
   USAGE: BPTREE_MULTI_DEF(name, N, type, [, oplist_of_the_type]) */
#define M_BPTREE_MULTI_DEF(name, N, ...)                                      \
  M_BPTREE_MULTI_DEF_AS(name, M_F(name,_t), M_F(name,_it_t), N, __VA_ARGS__)


/* Define a B+tree of a given type, of size N.
   allowing multiple equal keys to exist,
   as the given name name_t with its associated functions
   USAGE: BPTREE_MULTI_DEF_AS(name, name_t, it_t, N, type, [, oplist_of_the_type]) */
#define M_BPTREE_MULTI_DEF_AS(name, name_t, it_t, N, ...)                     \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_BPTR33_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                                 \
                 ((name, N, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), 0, 1, name_t, M_F(name, _node_ct), M_F(name, _pit_ct), it_t, M_F(name, _itref_ct)  ), \
                  (name, N, __VA_ARGS__,                                        __VA_ARGS__,                                        0, 1, name_t, M_F(name, _node_ct), M_F(name, _pit_ct), it_t, M_F(name, _itref_ct)  ))) \
  M_BEGIN_PROTECTED_CODE


/* Define the oplist of a B+TREE used as a set of type (from BPTREE_DEF).
   USAGE: BPTREE_OPLIST(name [, oplist_of_the_type])
   NOTE: IT_REF is not exported so that the container appears as not modifiable
   by algorithm. */
#define M_BPTREE_OPLIST(...)                                                  \
  M_BPTR33_KEY_OPLIST_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                          \
                        ((__VA_ARGS__, M_BASIC_OPLIST ),                      \
                         (__VA_ARGS__ )))


/* Define the oplist of a B+TREE used as a map of a key type to a value type (from BPTREE_DEF2).
   USAGE: BPTREE_OPLIST2(name[, key_oplist, value_oplist]) 
   NOTE: IT_REF is not exported so that the container appears as not modifiable
   by algorithm. */
#define M_BPTREE_OPLIST2(...)                                                 \
  M_BPTR33_OPLIST2_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                             \
                     ((__VA_ARGS__, M_BASIC_OPLIST, M_BASIC_OPLIST ),         \
                      (__VA_ARGS__ )))
  

/*****************************************************************************/
/******************************** INTERNAL ***********************************/
/*****************************************************************************/

/* Deferred evaluation for the oplist definition,
   so that all arguments are evaluated before further expansion */
#define M_BPTR33_KEY_OPLIST_P1(arg) M_BPTR33_KEY_OPLIST_P2 arg

/* Validation of the given oplist */
#define M_BPTR33_KEY_OPLIST_P2(name, oplist)                                  \
  M_IF_OPLIST(oplist)(M_BPTR33_KEY_OPLIST_P3, M_BPTR33_KEY_OPLIST_FAILURE)(name, oplist)

/* Prepare a clean compilation failure */
#define M_BPTR33_KEY_OPLIST_FAILURE(name, oplist)                             \
  ((M_LIB_ERROR(ARGUMENT_OF_BPTREE_OPLIST_IS_NOT_AN_OPLIST, name, oplist)))

/* OPLIST definition of a b+tree (global tree) */
#ifndef M_USE_CONTEXT
#define M_BPTR33_KEY_OPLIST_P3(name, oplist)                                  \
  (INIT(M_F(name, _init)),                                                    \
   INIT_SET(M_F(name, _init_set)),                                            \
   INIT_WITH(API_1(M_INIT_VAI)),                                              \
   SET(M_F(name, _set)),                                                      \
   CLEAR(M_F(name, _clear)),                                                  \
   INIT_MOVE(M_F(name, _init_move)),                                          \
   MOVE(M_F(name, _move)),                                                    \
   SWAP(M_F(name, _swap)),                                                    \
   NAME(name),                                                                \
   TYPE(M_F(name,_ct)), GENTYPE(struct M_F(name,_s)*),                        \
   SUBTYPE(M_F(name, _subtype_ct)),                                           \
   IT_TYPE(M_F(name, _it_ct)),                                                \
   IT_FIRST(M_F(name,_it)),                                                   \
   IT_SET(M_F(name,_it_set)),                                                 \
   IT_END(M_F(name,_it_end)),                                                 \
   IT_END_P(M_F(name,_end_p)),                                                \
   IT_LAST(M_F(name,_it_last)),                                               \
   IT_EQUAL_P(M_F(name,_it_equal_p)),                                         \
   IT_NEXT(M_F(name,_next)),                                                  \
   IT_PREVIOUS(M_F(name,_previous)),                                          \
   IT_CREF(M_F(name,_cref)),                                                  \
   RESET(M_F(name,_reset)),                                                   \
   PUSH(M_F(name,_push)),                                                     \
   GET_MIN(M_F(name,_min)),                                                   \
   GET_MAX(M_F(name,_max)),                                                   \
   M_IF_METHOD(GET_STR, oplist)(GET_STR(M_F(name, _get_str)),),               \
   M_IF_METHOD(PARSE_STR, oplist)(PARSE_STR(M_F(name, _parse_str)),),         \
   M_IF_METHOD(OUT_STR, oplist)(OUT_STR(M_F(name, _out_str)),),               \
   M_IF_METHOD(IN_STR, oplist)(IN_STR(M_F(name, _in_str)),),                  \
   M_IF_METHOD(OUT_SERIAL, oplist)(OUT_SERIAL(M_F(name, _out_serial)),),      \
   M_IF_METHOD(IN_SERIAL, oplist)(IN_SERIAL(M_F(name, _in_serial)),),         \
   M_IF_METHOD(EQUAL, oplist)(EQUAL(M_F(name, _equal_p)),),                   \
   M_IF_METHOD(HASH, oplist)(HASH(M_F(name, _hash)),)                         \
   )
#else
#define M_BPTR33_KEY_OPLIST_P3(name, oplist)                                  \
  (INIT(API_0P(M_F(name, _init))),                                            \
   INIT_SET(API_0P(M_F(name, _init_set))),                                    \
   INIT_WITH(API_1(M_INIT_VAI)),                                              \
   SET(API_0P(M_F(name, _set))),                                              \
   CLEAR(API_0P(M_F(name, _clear))),                                          \
   INIT_MOVE(M_F(name, _init_move)),                                          \
   MOVE(API_0P(M_F(name, _move))),                                            \
   SWAP(M_F(name, _swap)),                                                    \
   NAME(name), TYPE(M_F(name,_ct)), GENTYPE(struct M_F(name,_s)*),            \
   SUBTYPE(M_F(name, _subtype_ct)),                                           \
   IT_TYPE(M_F(name, _it_ct)),                                                \
   IT_FIRST(M_F(name,_it)),                                                   \
   IT_SET(M_F(name,_it_set)),                                                 \
   IT_END(M_F(name,_it_end)),                                                 \
   IT_END_P(M_F(name,_end_p)),                                                \
   IT_LAST(M_F(name,_it_last)),                                               \
   IT_EQUAL_P(M_F(name,_it_equal_p)),                                         \
   IT_NEXT(M_F(name,_next)),                                                  \
   IT_PREVIOUS(M_F(name,_previous)),                                          \
   IT_CREF(M_F(name,_cref)),                                                  \
   RESET(API_0P(M_F(name,_reset))),                                           \
   PUSH(API_0P(M_F(name,_push))),                                             \
   GET_MIN(M_F(name,_min)),                                                   \
   GET_MAX(M_F(name,_max)),                                                   \
   M_IF_METHOD(GET_STR, oplist)(API_0P(GET_STR(M_F(name, _get_str))),),       \
   M_IF_METHOD(PARSE_STR, oplist)(PARSE_STR(API_0P(M_F(name, _parse_str))),), \
   M_IF_METHOD(OUT_STR, oplist)(OUT_STR(M_F(name, _out_str)),),               \
   M_IF_METHOD(IN_STR, oplist)(IN_STR(API_0P(M_F(name, _in_str))),),          \
   M_IF_METHOD(OUT_SERIAL, oplist)(OUT_SERIAL(API_0P(M_F(name, _out_serial))),), \
   M_IF_METHOD(IN_SERIAL, oplist)(IN_SERIAL(API_0P(M_F(name, _in_serial))),), \
   M_IF_METHOD(EQUAL, oplist)(EQUAL(M_F(name, _equal_p)),),                   \
   M_IF_METHOD(HASH, oplist)(HASH(M_F(name, _hash)),)                         \
   )
#endif

/* Deferred evaluation */
#define M_BPTR33_OPLIST2_P1(arg) M_BPTR33_OPLIST2_P2 arg

/* Validation of the given oplists (first the key oplist, then the value oplist) */
#define M_BPTR33_OPLIST2_P2(name, key_oplist, value_oplist)                   \
  M_IF_OPLIST(key_oplist)(M_BPTR33_OPLIST2_P3, M_BPTR33_OPLIST2_FAILURE)(name, key_oplist, value_oplist)
#define M_BPTR33_OPLIST2_P3(name, key_oplist, value_oplist)                   \
  M_IF_OPLIST(value_oplist)(M_BPTR33_OPLIST2_P4, M_BPTR33_OPLIST2_FAILURE)(name, key_oplist, value_oplist)

/* Prepare a clean compilation failure */
#define M_BPTR33_OPLIST2_FAILURE(name, key_oplist, value_oplist)              \
  ((M_LIB_ERROR(ARGUMENT_OF_BPTREE_OPLIST_IS_NOT_AN_OPLIST, name, key_oplist, value_oplist)))

/* Final definition of the oplist (associative array) */
#ifndef M_USE_CONTEXT
#define M_BPTR33_OPLIST2_P4(name, key_oplist, value_oplist)                   \
  (INIT(M_F(name, _init)),                                                    \
   INIT_SET(M_F(name, _init_set)),                                            \
   INIT_WITH(API_1(M_INIT_KEY_VAI)),                                          \
   SET(M_F(name, _set)),                                                      \
   CLEAR(M_F(name, _clear)),                                                  \
   INIT_MOVE(M_F(name, _init_move)),                                          \
   MOVE(M_F(name, _move)),                                                    \
   SWAP(M_F(name, _swap)),                                                    \
   NAME(name),                                                                \
   TYPE(M_F(name,_ct)), GENTYPE(struct M_F(name,_s)*),                        \
   SUBTYPE(M_F(name, _subtype_ct)),                                           \
   EMPTY_P(M_F(name,_empty_p)),                                               \
   IT_TYPE(M_F(name, _it_ct)),                                                \
   IT_FIRST(M_F(name,_it)),                                                   \
   IT_SET(M_F(name,_it_set)),                                                 \
   IT_END(M_F(name,_it_end)),                                                 \
   IT_END_P(M_F(name,_end_p)),                                                \
   IT_LAST(M_F(name,_it_last)),                                               \
   IT_EQUAL_P(M_F(name,_it_equal_p)),                                         \
   IT_NEXT(M_F(name,_next)),                                                  \
   IT_PREVIOUS(M_F(name,_previous)),                                          \
   IT_CREF(M_F(name,_cref)),                                                  \
   RESET(M_F(name,_reset)),                                                   \
   GET_MIN(M_F(name,_min)),                                                   \
   GET_MAX(M_F(name,_max)),                                                   \
   KEY_TYPE(M_F(name, _key_ct)),                                              \
   VALUE_TYPE(M_F(name, _value_ct)),                                          \
   SET_KEY(M_F(name, _set_at)),                                               \
   GET_KEY(M_F(name, _get)),                                                  \
   SAFE_GET_KEY(M_F(name, _safe_get)),                                        \
   ERASE_KEY(M_F(name, _erase)),                                              \
   KEY_OPLIST(key_oplist),                                                    \
   VALUE_OPLIST(value_oplist),                                                \
   M_IF_METHOD_BOTH(PARSE_STR, key_oplist, value_oplist)(PARSE_STR(M_F(name, _parse_str)),), \
   M_IF_METHOD_BOTH(OUT_STR, key_oplist, value_oplist)(OUT_STR(M_F(name, _out_str)),), \
   M_IF_METHOD_BOTH(IN_STR, key_oplist, value_oplist)(IN_STR(M_F(name, _in_str)),), \
   M_IF_METHOD_BOTH(OUT_SERIAL, key_oplist, value_oplist)(OUT_SERIAL(M_F(name, _out_serial)),), \
   M_IF_METHOD_BOTH(IN_SERIAL, key_oplist, value_oplist)(IN_SERIAL(M_F(name, _in_serial)),), \
   M_IF_METHOD_BOTH(EQUAL, key_oplist, value_oplist)(EQUAL(M_F(name, _equal_p)),), \
   M_IF_METHOD_BOTH(HASH, key_oplist, value_oplist)(HASH(M_F(name, _hash)),)  \
   )
#else
#define M_BPTR33_OPLIST2_P4(name, key_oplist, value_oplist)                   \
  (INIT(API_0P(M_F(name, _init))),                                            \
   INIT_SET(API_0P(M_F(name, _init_set))),                                    \
   INIT_WITH(API_1(M_INIT_KEY_VAI)),                                          \
   SET(API_0P(M_F(name, _set))),                                              \
   CLEAR(API_0P(M_F(name, _clear))),                                          \
   INIT_MOVE(M_F(name, _init_move)),                                          \
   MOVE(API_0P(M_F(name, _move))),                                            \
   SWAP(M_F(name, _swap)),                                                    \
   NAME(name), TYPE(M_F(name,_ct)), GENTYPE(struct M_F(name,_s)*),            \
   SUBTYPE(M_F(name, _subtype_ct)),                                           \
   EMPTY_P(M_F(name,_empty_p)),                                               \
   IT_TYPE(M_F(name, _it_ct)),                                                \
   IT_FIRST(M_F(name,_it)),                                                   \
   IT_SET(M_F(name,_it_set)),                                                 \
   IT_END(M_F(name,_it_end)),                                                 \
   IT_END_P(M_F(name,_end_p)),                                                \
   IT_LAST(M_F(name,_it_last)),                                               \
   IT_EQUAL_P(M_F(name,_it_equal_p)),                                         \
   IT_NEXT(M_F(name,_next)),                                                  \
   IT_PREVIOUS(M_F(name,_previous)),                                          \
   IT_CREF(M_F(name,_cref)),                                                  \
   RESET(API_0P(M_F(name,_reset))),                                           \
   GET_MIN(M_F(name,_min)),                                                   \
   GET_MAX(M_F(name,_max)),                                                   \
   KEY_TYPE(M_F(name, _key_ct)),                                              \
   VALUE_TYPE(M_F(name, _value_ct)),                                          \
   SET_KEY(API_0P(M_F(name, _set_at))),                                       \
   GET_KEY(M_F(name, _get)),                                                  \
   SAFE_GET_KEY(API_0P(M_F(name, _safe_get))),                                \
   ERASE_KEY(API_0P(M_F(name, _erase))),                                      \
   KEY_OPLIST(key_oplist),                                                    \
   VALUE_OPLIST(value_oplist),                                                \
   M_IF_METHOD_BOTH(PARSE_STR, key_oplist, value_oplist)(PARSE_STR(API_0P(M_F(name, _parse_str))),), \
   M_IF_METHOD_BOTH(OUT_STR, key_oplist, value_oplist)(OUT_STR(M_F(name, _out_str)),), \
   M_IF_METHOD_BOTH(IN_STR, key_oplist, value_oplist)(IN_STR(API_0P(M_F(name, _in_str))),), \
   M_IF_METHOD_BOTH(OUT_SERIAL, key_oplist, value_oplist)(OUT_SERIAL(API_0P(M_F(name, _out_serial))),), \
   M_IF_METHOD_BOTH(IN_SERIAL, key_oplist, value_oplist)(IN_SERIAL(API_0P(M_F(name, _in_serial))),), \
   M_IF_METHOD_BOTH(EQUAL, key_oplist, value_oplist)(EQUAL(M_F(name, _equal_p)),), \
   M_IF_METHOD_BOTH(HASH, key_oplist, value_oplist)(HASH(M_F(name, _hash)),)  \
   )
#endif

/******************************** INTERNAL ***********************************/

/* Internal contract of a B+TREE of size 'N' for a node 'node' of root 'root' */
#ifdef NDEBUG
# define M_BPTR33_NODE_CONTRACT(N, isMulti, key_oplist, node, root)
#else
# define M_BPTR33_NODE_CONTRACT(N, isMulti, key_oplist, node, root) do {      \
    M_ASSERT ((node) != NULL);                                                \
    M_ASSERT ((root) != NULL);                                                \
    int  num2     = (node)->num;                                              \
    bool is_leaf2 = num2 <= 0;                                                \
    num2 = num2 < 0 ? -num2 : num2;                                           \
    if ((node) == (root)) {                                                   \
      /* Contract of the root node. num can be 0 */                           \
      M_ASSERT( 0 <= num2 && num2 <= N);                                      \
      if (num2 == 0) M_ASSERT (is_leaf2);                                     \
    } else {                                                                  \
      /* Contract of a non-root node. num cannot be 0 */                      \
      int c2 = N / 2;                                                         \
      M_ASSERT (c2 > 0);                                                      \
      M_ASSERT (c2 <= num2 && num2 <= N);                                     \
    }                                                                         \
    /* The node is sorted */                                                  \
    for(int i2 = 1; i2 < num2 ; i2++) {                                       \
      M_ASSERT (M_CALL_CMP(key_oplist, (node)->key[i2-1], (node)->key[i2]) M_IF(isMulti)(<=, <) 0); \
    }                                                                         \
    /* The chain node is also sorted */                                       \
    if ((node)->next != NULL) {                                               \
      M_ASSERT (num2 >= 1);                                                   \
      M_ASSERT (M_CALL_CMP(key_oplist, (node)->key[num2-1], (node)->next->key[0]) M_IF(isMulti)(<=, <) 0); \
    }                                                                         \
    if ((node)->prev != NULL) {                                               \
      M_ASSERT (num2 >= 1);                                                   \
      int num_prev = (node)->prev->num;                                       \
      num_prev = num_prev < 0 ? -num_prev : num_prev;                         \
      M_ASSERT (num_prev >= 1);                                               \
      M_ASSERT (M_CALL_CMP(key_oplist, (node)->prev->key[num_prev-1], (node)->key[0]) M_IF(isMulti)(<=, <) 0); \
    }                                                                         \
  } while (0)
#endif

/* Contract for a B+TREE of size N named 'b' */
#ifdef NDEBUG
#define M_BPTR33_CONTRACT(N, isMulti, key_oplist, b)
#else
#define M_BPTR33_CONTRACT(N, isMulti, key_oplist, b) do {                     \
    M_STATIC_ASSERT (N >= 3, M_LIB_DIMENSION, "B+TREE supports only N >= 3"); \
    M_BPTR33_NODE_CONTRACT(N, isMulti, key_oplist, (b)->root+0, (b)->root);   \
    M_ASSERT ((b)->root->next == NULL);                                       \
    M_ASSERT ((b)->root->prev == NULL);                                       \
    if ((b)->root->num <= 0) M_ASSERT (-(b)->root->num == (int) (b)->size);   \
  } while (0)
#endif

/* Max depth of any B+tree
    Worst case is when all nodes are only half full.
    Worst case is with the minimum size of a node (2)
    Maximum number of elements: SIZE_MAX = 2 ^ (CHAR_BIT*sizeof (size_t)) - 1
    Height of such a tree if inferior to:
    Ceil(Log2(2 ^ (CHAR_BIT*sizeof (size_t)))) + 1
    "+ 1" due to final line composed of nodes.
 */
#define M_BPTR33_MAX_STACK ((int)(1 + CHAR_BIT*sizeof (size_t)))

/* Transaction types for B+tree operations, needed for exceptions to rewind the operations */
typedef enum {
  M_BPTR33_NONE = 0, /* No transaction */
  M_BPTR33_NEW_NODE,      /* Transaction for a new node */
  M_BPTR33_INIT_KEY,      /* Transaction for init a key */
  M_BPTR33_MOVE_KEY,      /* Transaction for moving a key */
  M_BPTR33_INIT_VALUE,    /* Transaction for init a value */
  M_BPTR33_SPLIT_LEAF,    /* Transaction for a split of a leaf */
  M_BPTR33_SPLIT_NODE,    /* Transaction for a split of a node */
  M_BPTR33_SHIFT_LEAF,    /* Transaction for a shift of a leaf */
  M_BPTR33_SHIFT_NODE,    /* Transaction for a shift of a node */
  M_BPTR33_INC_SIZE,      /* Transaction for an increase of the size of the tree */
} m_bptr33_transaction_kind_t;

typedef struct {
  m_bptr33_transaction_kind_t type; /* Type of the transaction */
  void *data_dst;              /* Data destination of the transaction */
  void *data_src;              /* Data source of the transaction */
} m_bptr33_transaction_record_t;

/* Maximum number of transactions for adding a new element in a B+Tree
   5 records for a leaf + 4 records for a node */
#define M_BPTR33_TRANSACTION_MAX (5+4*M_BPTR33_MAX_STACK)

typedef struct {
  int size;                    /* Size of the record stack */
  m_bptr33_transaction_record_t tab[M_BPTR33_TRANSACTION_MAX];
} m_bptr33_transaction_t[1];

/* Internal function to add a transaction to the transaction stack */
M_INLINE void 
m_bptr33_add_transaction(m_bptr33_transaction_t records, 
                         m_bptr33_transaction_kind_t type, void *data_dst, void *data_src)
{
  M_ASSERT( records != NULL );
  M_ASSERT( records->size < M_BPTR33_TRANSACTION_MAX );
  M_ASSERT( type != M_BPTR33_NONE );
  records->tab[records->size].type = type;
  records->tab[records->size].data_dst = data_dst;                             
  records->tab[records->size].data_src = data_src;                              
  records->size ++;
}

// Add transaction only in exception mode (See M_IF_EXCEPTION)
// data real type is determined by the type of the transaction.
// So everything is stored as void* and casted back when used.
#define M_BPTR33_ADD_TRANSACTION(records, type, dst, src)                     \
  M_IF_EXCEPTION(m_bptr33_add_transaction(records, type, (void*) dst, (void*)(intptr_t) src))

/* Deferred evaluation for the b+tree definition,
   so that all arguments are evaluated before further expansion */
#define M_BPTR33_DEF_P1(arg) M_ID( M_BPTR33_DEF_P2 arg )

/* Validate the key oplist before going further */
#define M_BPTR33_DEF_P2(name, N, key_t, key_oplist, value_t, value_oplist, isMap, isMulti, tree_t, node_t, pit_t, it_t, subtype_t) \
  M_IF_OPLIST(key_oplist)(M_BPTR33_DEF_P3, M_BPTR33_DEF_FAILURE)              \
  (name, N, key_t, key_oplist, value_t, value_oplist, isMap, isMulti, tree_t, node_t, pit_t, it_t, subtype_t)

/* Validate the value oplist before going further */
#define M_BPTR33_DEF_P3(name, N, key_t, key_oplist, value_t, value_oplist, isMap, isMulti, tree_t, node_t, pit_t, it_t, subtype_t) \
  M_IF_OPLIST(value_oplist)(M_BPTR33_DEF_P4, M_BPTR33_DEF_FAILURE)            \
  (name, N, key_t, key_oplist, value_t, value_oplist, isMap, isMulti, tree_t, node_t, pit_t, it_t, subtype_t)

/* Stop processing with a compilation failure */
#define M_BPTR33_DEF_FAILURE(name, N, key_t, key_oplist, value_t, value_oplist, isMap, isMulti, tree_t, node_t, pit_t, it_t, subtype_t) \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST,                                       \
                   "(BPTREE*_DEF): one of the given argument is not a valid oplist: " \
                   M_AS_STR(key_oplist) " / " M_AS_STR(value_oplist))

/* Internal b+tree definition
   - name: prefix to be used
   - N: size of the node
   - key_t: key type of the elements of the container
   - key_oplist: oplist of the key type of the elements of the container
   - value_t: value type of the elements of the container
   - value_oplist: oplist of the value type of the elements of the container
   - isMap: true if map, false if set
   - isMulti: true if multimap/multiset, false otherwise
   - tree_t: alias for the type of the container
   - it_t: alias for the iterator of the container
   - node_t: alias for internal node
   - pit_t: alias for internal parent iterator
   - subtype_t: alias for the type referenced by the iterator
 */
#define M_BPTR33_DEF_P4(name, N, key_t, key_oplist, value_t, value_oplist, isMap, isMulti, tree_t, node_t, pit_t, it_t, subtype_t) \
  M_BPTR33_DEF_TYPE(name, N, key_t, key_oplist, value_t, value_oplist, isMap, isMulti, tree_t, node_t, pit_t, it_t, subtype_t) \
  M_CHECK_COMPATIBLE_OPLIST(name, 1, key_t, key_oplist)                       \
  M_CHECK_COMPATIBLE_OPLIST(name, 2, value_t, value_oplist)                   \
  M_BPTR33_DEF_CORE(name, N, key_t, key_oplist, value_t, value_oplist, isMap, isMulti, tree_t, node_t, pit_t, it_t, subtype_t) \
  M_BPTR33_DEF_IT(name, N, key_t, key_oplist, value_t, value_oplist, isMap, isMulti, tree_t, node_t, pit_t, it_t, subtype_t) \
  M_BPTR33_DEF_EXT(name, N, key_t, key_oplist, value_t, value_oplist, isMap, isMulti, tree_t, node_t, pit_t, it_t, subtype_t) \
  M_EMPLACE_ASS_ARRAY_OR_QUEUE_DEF(M_INV(isMap), name, tree_t, key_oplist, value_oplist)
  /* TODO: Check if key type has not disabled INIT_MOVE */

/* Define the types of a B+Tree */
#define M_BPTR33_DEF_TYPE(name, N, key_t, key_oplist, value_t, value_oplist, isMap, isMulti, tree_t, node_t, pit_t, it_t, subtype_t) \
  M_IF(isMap)(                                                                \
    /* Type returned by the iterator. Due to having key and value             \
       separated in their own array in the node, it is pointers to            \
       the objects, not a global pointer to both objects. */                  \
    typedef struct M_F(name, _pair_s) {                                       \
      key_t   *key_ptr;                                                       \
      value_t *value_ptr;                                                     \
    } subtype_t;                                                              \
    ,                                                                         \
    typedef key_t subtype_t;                                                  \
  )                                                                           \
                                                                              \
  /* Define a Node of a B+TREE                                                \
   * For a reason of simplicity, it allocates one more element than           \
   * needed so that the code can push one more element in the node and        \
   * then split the nodes (simplify the code)                                 \
   */                                                                         \
  typedef struct M_F(name, _node_s) {                                         \
    int    num;           /* Abs=Number of keys. Sign <0 is leaf */           \
    key_t  key[N+1];      /* We can temporary push one more key */            \
    struct M_F(name, _node_s) *next;  /* next node reference */               \
    struct M_F(name, _node_s) *prev;  /* previous node reference */           \
    union  M_F(name, _kind_s) {       /* either value or pointer to other nodes */ \
      M_IF(isMap)(value_t        value[N+1];,)                                \
      struct M_F(name, _node_s) *node[N+2];                                   \
    } kind;                                                                   \
  } *node_t;                                                                  \
                                                                              \
  /* A B+TREE is just a pointer to the root node */                           \
  typedef struct M_F(name, _s) {                                              \
    node_t root;                                                              \
    size_t size;                                                              \
  } tree_t[1];                                                                \
  typedef struct M_F(name, _s) *M_F(name, _ptr);                              \
  typedef const struct M_F(name, _s) *M_F(name, _srcptr);                     \
                                                                              \
  /* Definition of the alias used by the oplists */                           \
  typedef subtype_t M_F(name, _subtype_ct);                                   \
  typedef key_t     M_F(name, _key_ct);                                       \
  typedef value_t   M_F(name, _value_ct);                                     \
  typedef tree_t    M_F(name, _ct);                                           \
                                                                              \
  /* Define the Parent Tree Iterator */                                       \
  typedef struct M_F(name, _parent_it_s) {                                    \
    int num;                                                                  \
    node_t parent[M_BPTR33_MAX_STACK];                                        \
  } pit_t[1];                                                                 \
                                                                              \
  /* Define the Iterator */                                                   \
  typedef struct M_F(name, _it_s) {                                           \
    M_IF(isMap)(struct M_F(name, _pair_s) pair;,)                             \
    node_t node;                                                              \
    int    idx;                                                               \
  } it_t[1];                                                                  \
  typedef it_t M_F(name, _it_ct);                                             \

/* Define the core functions of a B+ Tree */
#define M_BPTR33_DEF_CORE(name, N, key_t, key_oplist, value_t, value_oplist, isMap, isMulti, tree_t, node_t, pit_t, it_t, subtype_t) \
                                                                              \
  /* Allocate a new node */                                                   \
  /* TODO: Can be specialized to alloc for leaf or for non leaf */            \
  M_INLINE node_t M_F(name, _new_node)(M_P_EXPAND_void)                       \
  {                                                                           \
    node_t n = M_CALL_NEW(key_oplist, struct M_F(name, _node_s));             \
    if (M_UNLIKELY_NOMEM (n == NULL)) {                                       \
      M_MEMORY_FULL(node_t, 1);                                               \
    }                                                                         \
    n->next = NULL;                                                           \
    n->prev = NULL;                                                           \
    n->num = 0;                                                               \
    return n;                                                                 \
  }                                                                           \
                                                                              \
  M_P(void, name, _init, tree_t b)                                            \
  {                                                                           \
    b->root = M_F(name, _new_node)(M_R_EXPAND_void);                          \
    b->size = 0;                                                              \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                             \
  }                                                                           \
                                                                              \
  M_INLINE bool M_F(name, _is_leaf)(const node_t n)                           \
  {                                                                           \
    /* We consider the empty node as a leaf */                                \
    /* Only the root node can be empty */                                     \
    return n->num <= 0;                                                       \
  }                                                                           \
                                                                              \
  /* Return the number of keys of the node */                                 \
  M_INLINE int M_F(name, _get_num)(const node_t n)                            \
  {                                                                           \
    int num = n->num;                                                         \
    num = num < 0 ? -num : num;                                               \
    M_ASSERT (num <= N);                                                      \
    return num;                                                               \
  }                                                                           \
                                                                              \
  M_P(void, name, _reset, tree_t b)                                           \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                             \
    node_t next, n = b->root;                                                 \
    pit_t pit;                                                                \
    /* np is the heigh of the tree */                                         \
    int np = 0;                                                               \
    /* Scan down the nodes to the left down node */                           \
    while (!M_F(name, _is_leaf)(n)) {                                         \
      pit->parent[np++] = n;                                                  \
      M_ASSERT (np <= M_BPTR33_MAX_STACK);                                    \
      n = n->kind.node[0];                                                    \
    }                                                                         \
    pit->parent[np++] = n;                                                    \
    M_ASSERT (np <= M_BPTR33_MAX_STACK);                                      \
    /* Clean & free non root */                                               \
    for(int i = 0; i < np; i++) {                                             \
      n = pit->parent[i];                                                     \
      while (n != NULL) {                                                     \
        /* Clear key (& value for leaf) */                                    \
        int num = M_F(name, _get_num)(n);                                     \
        M_IF(isMap)(bool is_leaf = M_F(name, _is_leaf)(n);,)                  \
        for(int j = 0; j < num; j++) {                                        \
          M_CALL_CLEAR(key_oplist, n->key[j]);                                \
          M_IF(isMap)(if (is_leaf) {                                          \
              M_CALL_CLEAR(value_oplist, n->kind.value[j]);                   \
            },)                                                               \
        }                                                                     \
        /* Next node of the same height */                                    \
        next = n->next;                                                       \
        if (i != 0) {                                                         \
          /* Free the node if non root */                                     \
          M_CALL_DEL(key_oplist, n);                                          \
        }                                                                     \
        n = next;                                                             \
      }                                                                       \
    }                                                                         \
    /* Clean root */                                                          \
    b->root->num = 0;                                                         \
    b->size = 0;                                                              \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                             \
  }                                                                           \
                                                                              \
  M_P(void, name, _clear, tree_t b)                                           \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                             \
    M_F(name, _reset)M_R(b);                                                  \
    /* Once the tree is clean, only the root remains */                       \
    M_CALL_DEL(key_oplist, b->root);                                          \
    b->root = NULL;                                                           \
  }                                                                           \
                                                                              \
  /* Copy recursively the node 'o' of root node 'root' */                     \
  M_P(void, name, _copy_node, node_t *dst, const node_t o, const node_t root M_IF_EXCEPTION(M_DEFERRED_COMMA key_t *volatile* key_to_rewind)) \
  {                                                                           \
    node_t n = M_F(name, _new_node)(M_R_EXPAND_void);                         \
    *dst = n;                                                                 \
    /* In exception mode, keep track of the number of elements in the node */ \
    M_IF_EXCEPTION(n->num = 0);                                               \
    /* By default it is not linked to its brother(s).                         \
       Only its parent can do it: it has to fix it */                         \
    /* Get number of keys in the node and copy them */                        \
    const int num = M_F(name, _get_num)(o);                                   \
    if (M_F(name, _is_leaf)(o)) {                                             \
      /* Copy of a leaf */                                                    \
      for(int i = 0; i < num; i++) {                                          \
        M_CALL_INIT_SET(key_oplist, n->key[i], o->key[i]);                    \
        /* In exception, keep track of the key to rewind */                   \
        /* Only **one** key can be tracked at a time */                       \
        M_IF_EXCEPTION(*key_to_rewind = &n->key[i]);                          \
        /* Copy the associated values if it is a leaf and a MAP */            \
        M_IF(isMap)(                                                          \
          M_CALL_INIT_SET(value_oplist, n->kind.value[i], o->kind.value[i]);  \
        , /* End of isMap */)                                                 \
        M_IF_EXCEPTION(*key_to_rewind = NULL);                                \
        M_IF_EXCEPTION(n->num += -1);                                         \
      }                                                                       \
   } else {                                                                   \
      /* Copy recursively the associated nodes if it is not a leaf */         \
      M_ASSERT(num != 0);                                                     \
      /* In exception mode, clear the child pointers to know if allocation fails */ \
      M_IF_EXCEPTION( for(int i = 0; i <= num; i++) {                         \
        n->kind.node[i] = NULL;                                               \
      })                                                                      \
      for(int i = 0; i < num; i++) {                                          \
        M_CALL_INIT_SET(key_oplist, n->key[i], o->key[i]);                    \
        M_IF_EXCEPTION(n->num += 1; );                                        \
      }                                                                       \
      for(int i = 0; i <= num; i++) {                                         \
        M_ASSERT(o->kind.node[i] != root);                                    \
        M_F(name, _copy_node)M_R(&n->kind.node[i], o->kind.node[i], root M_IF_EXCEPTION(M_DEFERRED_COMMA key_to_rewind)); \
      }                                                                       \
      /* The copied nodes don't have their next/prev field correct */         \
      /* Fix the next/prev field for the copied nodes */                      \
      for(int i = 0; i < num; i++) {                                          \
        node_t current = n->kind.node[i];                                     \
        node_t next = n->kind.node[i+1];                                      \
        current->next = next;                                                 \
        next->prev = current;                                                 \
        /* Go down the tree up to the leaf                                    \
           and fix the final 'next' link with the copied node */              \
        while (!M_F(name, _is_leaf)(current)) {                               \
          M_ASSERT(!M_F(name, _is_leaf)(next));                               \
          current = current->kind.node[current->num];                         \
          next    = next->kind.node[0];                                       \
          current->next = next;                                               \
          next->prev = current;                                               \
        }                                                                     \
      }                                                                       \
    }                                                                         \
    /* Set default number of keys and type to copy */                         \
    M_IF_EXCEPTION(M_ASSUME(n->num == o->num));                               \
    n->num = o->num;                                                          \
    M_BPTR33_NODE_CONTRACT(N, isMulti, key_oplist, n, (o==root) ? n : root);  \
  }                                                                           \
                                                                              \
  M_IF_EXCEPTION(                                                             \
  /* Rewind a node created using _copy_node */                                \
  M_P(void, name, _reset_rewind_node, node_t n)                               \
  {                                                                           \
    M_ASSERT (n != NULL);                                                     \
    /* WARNING : node is partially initialized and contracts aren't true ! */ \
    const int num = M_F(name, _get_num)(n);                                   \
    if (M_F(name, _is_leaf)(n)) {                                             \
      for(int i = 0; i < num; i++) {                                          \
        M_CALL_CLEAR(key_oplist, n->key[i]);                                  \
        M_IF(isMap)(M_CALL_CLEAR(value_oplist, n->kind.value[i]); ,)          \
      }                                                                       \
    } else {                                                                  \
      for(int i = 0; i < num; i++) {                                          \
        M_CALL_CLEAR(key_oplist, n->key[i]);                                  \
      }                                                                       \
      /* Note: cannot use next pointes which may be broken. */                \
      for(int i = 0; i <= num; i++) {                                         \
        /* n->kind.node[i] is different of NULL, then it at least in a        \
        semi-initialized state we can parse and undo */                       \
        if (n->kind.node[i] != NULL) {                                        \
          M_F(name, _reset_rewind_node)M_R(n->kind.node[i]);                  \
        }                                                                     \
      }                                                                       \
    }                                                                         \
    M_CALL_DEL(key_oplist, n);                                                \
  }                                                                           \
                                                                              \
  M_P(void, name, _reset_rewind, tree_t b, key_t *volatile key_to_rewind)     \
  {                                                                           \
    M_ASSERT (b != NULL);                                                     \
    /* B+Tree is not fully constructed. Clear it with care */                 \
    /* In particular, not contract of a B+Tree holds. */                      \
    /* Clear the unique key that belongs to a leaf for which its associated   \
       value was not initialized */                                           \
    if (key_to_rewind != NULL) {                                              \
      M_CALL_CLEAR(key_oplist, *key_to_rewind);                               \
    }                                                                         \
    /* Clear the tree */                                                      \
    if (b->root != NULL) {                                                    \
      M_F(name, _reset_rewind_node)M_R(b->root);                              \
    }                                                                         \
    b->root = NULL;                                                           \
    b->size = 1; /* make an invalid representation */                         \
  }                                                                           \
  /* End M_IF_EXCEPTION */ )                                                  \
                                                                              \
  M_P(void, name, _init_set, tree_t b, const tree_t o)                        \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, o);                             \
    M_ASSERT (b != NULL);                                                     \
    /* Just copy recursively the root node */                                 \
    M_IF_EXCEPTION(key_t *volatile key_to_rewind = NULL);                     \
    M_ON_EXCEPTION( M_F(name, _reset_rewind)M_R(b, key_to_rewind) ) {         \
      M_IF_EXCEPTION(b->root = NULL);                                         \
      M_F(name, _copy_node)M_R(&b->root, o->root, o->root M_IF_EXCEPTION( M_DEFERRED_COMMA &key_to_rewind)); \
      b->size = o->size;                                                      \
    }                                                                         \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                             \
  }                                                                           \
                                                                              \
  M_INLINE bool M_F(name, _empty_p)(const tree_t b)                           \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                             \
    /* root shall be an empty leaf */                                         \
    return b->size == 0;                                                      \
  }                                                                           \
                                                                              \
  M_INLINE size_t M_F(name, _size)(const tree_t b)                            \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                             \
    return b->size;                                                           \
  }                                                                           \
                                                                              \
  M_INLINE node_t M_F(name, _i_search_for_leaf)(pit_t pit, const tree_t b, key_t const key) \
  {                                                                           \
    node_t n = b->root;                                                       \
    int np = 0;                                                               \
    /* Go down the tree while searching for key */                            \
    while (!M_F(name, _is_leaf)(n)) {                                         \
      M_ASSERT (np <= M_BPTR33_MAX_STACK);                                    \
      int i, hi = n->num;                                                     \
      M_ASSERT (hi > 0);                                                      \
      /* Linear search is usually faster than binary search for               \
         B+TREE (due to cache effect). If a binary tree is faster for         \
         the choosen type and size , it probably means that the               \
         size of B+TREE is too big and should be reduced. */                  \
      for(i = 0; i < hi; i++) {                                               \
        if (M_CALL_CMP(key_oplist, key, n->key[i]) <= 0)                      \
          break;                                                              \
      }                                                                       \
      /* Update the Parent iterator */                                        \
      pit->parent[np++] = n;                                                  \
      /* Select the new node to go down to */                                 \
      n = n->kind.node[i];                                                    \
      M_ASSERT (n != NULL);                                                   \
      M_BPTR33_NODE_CONTRACT(N, isMulti, key_oplist, n, b->root);             \
    }                                                                         \
    pit->num = np;                                                            \
    return n;                                                                 \
  }                                                                           \
                                                                              \
  M_INLINE value_t *M_F(name, _get)(const tree_t b, key_t const key)          \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                             \
    pit_t pit;                                                                \
    /* Get the leaf node where the key can be */                              \
    node_t n = M_F(name, _i_search_for_leaf)(pit, b, key);                    \
    int cmp = 0;                                                              \
    M_BPTR33_NODE_CONTRACT(N, isMulti, key_oplist, n, b->root);               \
    /* Search in the leaf for key */                                          \
    for(int i = 0; cmp >= 0 && i < -n->num; i++) {                            \
      cmp = M_CALL_CMP(key_oplist, key, n->key[i]);                           \
      if (cmp == 0) {                                                         \
        /* Return the value if MAP mode or the key if SET mode */             \
        return M_IF(isMap)(&n->kind.value[i],&n->key[i]);                     \
      }                                                                       \
    }                                                                         \
    /* Key not found */                                                       \
    return NULL;                                                              \
  }                                                                           \
                                                                              \
  M_INLINE value_t const *M_F(name, _cget)(const tree_t b, key_t const key)   \
  {                                                                           \
    return M_CONST_CAST(value_t, M_F(name, _get)(b, key));                    \
  }                                                                           \
                                                                              \
  M_P(void, name, _rewind_transaction, m_bptr33_transaction_t records)        \
  {                                                                           \
    M_ASSERT (records != NULL);                                               \
    /* Rewind all the recorded transactions in reverse order */               \
    for(int j = records->size - 1; j >= 0; j--) {                             \
      m_bptr33_transaction_record_t *record = &records->tab[j];               \
      switch (record->type) {                                                 \
        case M_BPTR33_NEW_NODE:                                               \
          M_CALL_DEL(key_oplist, (node_t) record->data_dst);                  \
          break;                                                              \
        case M_BPTR33_INIT_KEY:                                               \
          M_CALL_CLEAR(key_oplist, *(key_t*) record->data_dst);               \
          break;                                                              \
        case M_BPTR33_MOVE_KEY:                                               \
          M_CALL_INIT_MOVE(key_oplist, *(key_t*) record->data_src, *(key_t*) record->data_dst); \
          break;                                                              \
        case M_BPTR33_INIT_VALUE:                                             \
          M_CALL_CLEAR(value_oplist, *(value_t*) record->data_dst);           \
          break;                                                              \
        case M_BPTR33_INC_SIZE:                                               \
          {                                                                   \
          struct M_F(name, _s) *t = (struct M_F(name, _s)*) record->data_dst; \
          M_ASSERT(t != NULL);                                                \
          M_ASSERT(t->size > 0);                                              \
          t->size --;                                                         \
          }                                                                   \
          break;                                                              \
        case M_BPTR33_SPLIT_LEAF:                                             \
          { /* We need to link back the splitted node to the original node */ \
            node_t a = (node_t) record->data_dst;                             \
            node_t b = (node_t) record->data_src;                             \
            M_ASSERT(a != NULL && a->next == b);                              \
            M_ASSERT(M_F(name, _is_leaf)(a) && M_F(name, _is_leaf)(b));       \
            /* Unlink b */                                                    \
            a->next = b->next;                                                \
            if (b->next != NULL) {                                            \
              b->next->prev = a;                                              \
            }                                                                 \
            /* Move items from b back to a*/                                  \
            int na = M_F(name, _get_num)(a);                                  \
            int nb = M_F(name, _get_num)(b);                                  \
            M_ASSERT(na + nb == N + 1);                                       \
            memmove(&a->key[na], &b->key[0], sizeof(key_t)*(unsigned int)nb); \
            M_IF(isMap)(memmove(&a->kind.value[na], &b->kind.value[0], sizeof(value_t)*(unsigned int)nb);,) \
            a->num -= nb; /* Increase num for leaf (negative representation) */ \
            /* b will be deleted later due to M_BPTR33_NEW_NODE record */     \
          }                                                                   \
          break;                                                              \
        case M_BPTR33_SPLIT_NODE:                                             \
          { /* We need to link back the splitted node to the original node */ \
            node_t a = (node_t) record->data_dst;                             \
            node_t b = (node_t) record->data_src;                             \
            M_ASSERT(a != NULL && a->next == b);                              \
            M_ASSERT(!M_F(name, _is_leaf)(a) && !M_F(name, _is_leaf)(b));     \
            /* Unlink b */                                                    \
            a->next = b->next;                                                \
            if (b->next != NULL) {                                            \
              b->next->prev = a;                                              \
            }                                                                 \
            /* Move items from b back to a*/                                  \
            int na = M_F(name, _get_num)(a);                                  \
            int nb = M_F(name, _get_num)(b);                                  \
            M_ASSERT(na + nb + 1 == N + 1);                                   \
            memmove(&a->key[na+1], &b->key[0], sizeof(key_t)*(unsigned int)nb); \
            memmove(&a->kind.node[na+1], &b->kind.node[0], sizeof(node_t)*(unsigned int)(nb+1)); \
            /* We need to keep an empty slot in the middle at [na] */         \
            /* it will be filled later by a M_BPTR33_MOVE_KEY record */       \
            a->num += nb + 1; /* Increase num for node */                     \
            /* b will be deleted later due to M_BPTR33_NEW_NODE record */     \
          }                                                                   \
          break;                                                              \
        case M_BPTR33_SHIFT_LEAF:                                             \
          { /* We need to shift back the shifted node */                      \
            node_t n = (node_t) record->data_dst;                             \
            M_ASSERT(M_F(name, _is_leaf)(n));                                 \
            int i = (int)(intptr_t) record->data_src;                         \
            int num = -(++n->num);                                            \
            memmove(&n->key[i], &n->key[i+1], sizeof(key_t)*(unsigned int)(num-i)); \
            M_IF(isMap)(memmove(&n->kind.value[i], &n->kind.value[i+1], sizeof(value_t)*(unsigned int)(num-i));,) \
          }                                                                   \
          break;                                                              \
        case M_BPTR33_SHIFT_NODE:                                             \
          { /* We need to shift back the shifted node */                      \
            node_t n = (node_t) record->data_dst;                             \
            M_ASSERT(!M_F(name, _is_leaf)(n));                                \
            int i = (int)(intptr_t) record->data_src;                         \
            int num = --n->num;                                               \
            memmove(&n->key[i], &n->key[i+1], sizeof(key_t)*(unsigned int)(num-i)); \
            memmove(&n->kind.node[i], &n->kind.node[i+1], sizeof(node_t)*(unsigned int)(num-i+1)); \
          }                                                                   \
          break;                                                              \
        case M_BPTR33_NONE:                                                   \
        default:                                                              \
          M_ASSERT (false); /* Invalid transaction type */                    \
      }                                                                       \
    }                                                                         \
    records->size = 0;                                                        \
    return;                                                                   \
  }                                                                           \
                                                                              \
  M_P(int, name, _i_search_and_insert_in_leaf, node_t n, key_t const key      \
                          M_IF(isMap)( M_DEFERRED_COMMA value_t const value,) \
                          M_IF_EXCEPTION( M_DEFERRED_COMMA m_bptr33_transaction_t records) ) \
  {                                                                           \
    M_ASSERT (M_F(name, _is_leaf)(n));                                        \
    M_UNUSED_CONTEXT();                                                       \
    int i, num = M_F(name, _get_num)(n);                                      \
    M_ASSERT (num <= N);                                                      \
    /* Search for the key in the node n (a leaf) for insertion */             \
    for(i = 0; i < num; i++) {                                                \
      int cmp = M_CALL_CMP(key_oplist, key, n->key[i]);                       \
      if (cmp <= 0) {                                                         \
        M_IF(isMulti)( /* Nothing to do : fallthrough */,                     \
          /* Update value if keys are equal */                                \
          if (M_UNLIKELY (cmp == 0)) {                                        \
            M_IF(isMap)(M_CALL_SET(value_oplist, n->kind.value[i], value);,)  \
            return -1;                                                        \
          }                                                                   \
        )                                                                     \
        /* Move tables to make space for insertion */                         \
        memmove(&n->key[i+1], &n->key[i], sizeof(key_t)*(unsigned int)(num-i)); \
        M_IF(isMap)(memmove(&n->kind.value[i+1], &n->kind.value[i], sizeof(value_t)*(unsigned int)(num-i));,) \
        break;                                                                \
      }                                                                       \
    }                                                                         \
    M_BPTR33_ADD_TRANSACTION(records, M_BPTR33_SHIFT_LEAF, n, i);             \
    /* Increase the number of key in the node */                              \
    n->num  += -1; /* Increase num as num<0 for leaf */                       \
    /* Insert key & value if MAP mode */                                      \
    M_CALL_INIT_SET(key_oplist, n->key[i], key);                              \
    M_BPTR33_ADD_TRANSACTION(records, M_BPTR33_INIT_KEY, &n->key[i], 0);      \
    M_IF(isMap)(M_CALL_INIT_SET(value_oplist, n->kind.value[i], value);,)     \
    M_BPTR33_ADD_TRANSACTION(records, M_BPTR33_INIT_VALUE, &n->kind.value[i], 0); \
    return i;                                                                 \
  }                                                                           \
                                                                              \
  M_P(int, name, _i_search_and_insert_in_node, node_t n, node_t l, key_t *key \
                          M_IF_EXCEPTION( M_DEFERRED_COMMA m_bptr33_transaction_t records) ) \
  {                                                                           \
    M_ASSERT (!M_F(name, _is_leaf)(n));                                       \
    M_UNUSED_CONTEXT();                                                       \
    int i, num = M_F(name, _get_num)(n);                                      \
    M_ASSERT (num <= N);                                                      \
    /* Search for the key in the node n (not a leaf) for insertion */         \
    for(i = 0; i < num; i++) {                                                \
      if (n->kind.node[i] == l) {                                             \
        /* Move tables to make space for insertion */                         \
        memmove(&n->key[i+1], &n->key[i], sizeof(key_t)*(unsigned int)(num-i)); \
        memmove(&n->kind.node[i+1], &n->kind.node[i], sizeof(node_t)*(unsigned int)(num-i+1)); \
        break;                                                                \
      }                                                                       \
    }                                                                         \
    M_BPTR33_ADD_TRANSACTION(records, M_BPTR33_SHIFT_NODE, n, i);             \
    /* Increase the number of key in the node */                              \
    n->num  += 1;                                                             \
    /* Insert key in node */                                                  \
    /* If the leaf variable is a leaf node, we need to insert a copy of the key */ \
    /* otherwise if it is node, we need to steal the key by moving it */      \
    if (M_F(name, _is_leaf)(l)) {                                             \
      M_CALL_INIT_SET(key_oplist, n->key[i], *key);                           \
      M_BPTR33_ADD_TRANSACTION(records, M_BPTR33_INIT_KEY, &n->key[i], 0);    \
    } else {                                                                  \
      M_CALL_INIT_MOVE(key_oplist, n->key[i], *key);                          \
      M_BPTR33_ADD_TRANSACTION(records, M_BPTR33_MOVE_KEY, &n->key[i], key);  \
    }                                                                         \
    return i;                                                                 \
  }                                                                           \
                                                                              \
  M_IF(isMap)(                                                                \
    M_P(void, name, _set_at, tree_t b, key_t const key, value_t const value) , \
    M_P(void, name, _push, tree_t b, key_t const key) )                       \
  {                                                                           \
    pit_t pit;                                                                \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                             \
    M_IF_EXCEPTION(m_bptr33_transaction_t records; records->size = 0;)        \
    M_ON_EXCEPTION( M_F(name, _rewind_transaction)M_R(records); ) {           \
    node_t leaf = M_F(name, _i_search_for_leaf)(pit, b, key), nleaf;          \
    key_t *key_ptr;                                                           \
    int num, nnum;                                                            \
    /* Insert key into the leaf.*/                                            \
    /* NOTE: Even if there is N elements, we can still add one more.*/        \
    int i = M_F(name, _i_search_and_insert_in_leaf)M_R(leaf, key M_IF (isMap)(M_DEFERRED_COMMA value,) M_IF_EXCEPTION(M_DEFERRED_COMMA records)); \
    if (i < 0) {                                                              \
      /* Nothing to do anymore. key already exists in the tree.               \
         value has been updated if needed */                                  \
      M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                           \
      goto exit;                                                              \
    }                                                                         \
    b->size ++;                                                               \
    M_BPTR33_ADD_TRANSACTION(records, M_BPTR33_INC_SIZE, b, 0);               \
    /* Most likely case: leaf can accept key */                               \
    num = -leaf->num;                                                         \
    M_ASSERT (num > 0);                                                       \
    if (M_LIKELY (num <= N)) {                                                \
      /* nothing more to do */                                                \
      M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                           \
      goto exit;                                                              \
    }                                                                         \
    M_ASSERT (num == N+1);                                                    \
                                                                              \
    /* Needs to rebalance the B+TREE */                                       \
    /* leaf is full: need to slip the leaf in two */                          \
    nnum = (N + 1) / 2;                                                       \
    num = N + 1 - nnum;                                                       \
    nleaf = M_F(name, _new_node)(M_R_EXPAND_void);                            \
    M_BPTR33_ADD_TRANSACTION(records, M_BPTR33_NEW_NODE, nleaf, 0);           \
    /* Move half objects to the new node */                                   \
    memmove(&nleaf->key[0], &leaf->key[num], sizeof(key_t)*(unsigned int)nnum); \
    M_IF(isMap)(memmove(&nleaf->kind.value[0], &leaf->kind.value[num], sizeof(value_t)*(unsigned int)nnum);,) \
    leaf->num = -num;                                                         \
    nleaf->num = -nnum;                                                       \
    nleaf->next = leaf->next;                                                 \
    leaf->next = nleaf;                                                       \
    nleaf->prev = leaf;                                                       \
    if (nleaf->next != NULL) {                                                \
      nleaf->next->prev = nleaf;                                              \
    }                                                                         \
    M_BPTR33_ADD_TRANSACTION(records, M_BPTR33_SPLIT_LEAF, leaf, nleaf);      \
    M_BPTR33_NODE_CONTRACT(N, isMulti, key_oplist, leaf, b->root);            \
    M_BPTR33_NODE_CONTRACT(N, isMulti, key_oplist, nleaf, b->root);           \
    /* Update parent to inject *key_ptr that splits between (leaf, nleaf) */  \
    key_ptr = &leaf->key[num-1];                                              \
    while (true) {                                                            \
      if (pit->num == 0) {                                                    \
        /* We reach root ==> Need to increase the height of the tree.*/       \
        node_t parent = M_F(name, _new_node)(M_R_EXPAND_void);                \
        M_BPTR33_ADD_TRANSACTION(records, M_BPTR33_NEW_NODE, parent, 0);      \
        parent->num = 1;                                                      \
        /* If the 'leaf' variable is a leaf node, we need to insert a copy of the key */ \
        /* otherwise if it is node, we need to steal the key by moving it */  \
        if (M_F(name, _is_leaf)(leaf)) {                                      \
          M_CALL_INIT_SET(key_oplist, parent->key[0], *key_ptr);              \
          M_BPTR33_ADD_TRANSACTION(records, M_BPTR33_INIT_KEY, &parent->key[0], 0); \
        } else {                                                              \
          M_CALL_INIT_MOVE(key_oplist, parent->key[0], *key_ptr);             \
          M_BPTR33_ADD_TRANSACTION(records, M_BPTR33_MOVE_KEY, &parent->key[0], key_ptr); \
        }                                                                     \
        parent->kind.node[0] = leaf;                                          \
        parent->kind.node[1] = nleaf;                                         \
        b->root = parent;                                                     \
        M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                         \
        goto exit;                                                            \
      }                                                                       \
      /* Non root node. Get the parent node */                                \
      node_t parent = pit->parent[--pit->num];                                \
      /* Insert into parent (It is big enough to receive temporary one more) */ \
      i = M_F(name, _i_search_and_insert_in_node)M_R(parent, leaf, key_ptr M_IF_EXCEPTION(M_DEFERRED_COMMA records)); \
      parent->kind.node[i] = leaf;                                            \
      parent->kind.node[i+1] = nleaf;                                         \
      /* Test if parent node is full? */                                      \
      if (M_LIKELY (parent->num <= N)) {                                      \
        M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                         \
        goto exit; /* No need to split parent.*/                              \
      }                                                                       \
      M_ASSERT (parent->num == N+1);                                          \
      /* Need to split parent in {np} {med} {nnp} */                          \
      int nnp = N / 2;                                                        \
      int np = N - nnp;                                                       \
      M_ASSERT (nnp > 0 && np > 0 && nnp+np+1 == N+1);                        \
      node_t nparent = M_F(name, _new_node)(M_R_EXPAND_void);                 \
      M_BPTR33_ADD_TRANSACTION(records, M_BPTR33_NEW_NODE, nparent, 0);       \
      /* Move half items to new node (Like a classic B-TREE)                  \
         and **move** the median key to the grand-parent*/                    \
      memmove(&nparent->key[0], &parent->key[np+1], sizeof(key_t)*(unsigned int)nnp); \
      memmove(&nparent->kind.node[0], &parent->kind.node[np+1], sizeof(node_t)*(unsigned int)(nnp+1)); \
      parent->num = np;                                                       \
      nparent->num = nnp;                                                     \
      nparent->next = parent->next;                                           \
      parent->next = nparent;                                                 \
      nparent->prev = parent;                                                 \
      if (nparent->next != NULL) {                                            \
        nparent->next->prev = nparent;                                        \
      }                                                                       \
      M_BPTR33_ADD_TRANSACTION(records, M_BPTR33_SPLIT_NODE, parent, nparent); \
      M_BPTR33_NODE_CONTRACT(N, isMulti, key_oplist, parent, b->root);        \
      M_BPTR33_NODE_CONTRACT(N, isMulti, key_oplist, nparent, b->root);       \
      /* Prepare for the next step */                                         \
      key_ptr = &parent->key[np];                                             \
      /* Well, leaf is now a node */                                          \
      leaf = parent;                                                          \
      nleaf = nparent;                                                        \
    }                                                                         \
    exit:                                                                     \
    /* Cannot use return on an exception protected block */                   \
    (void)0; /* avoid warning about unused label */                           \
    } /* On exception */                                                      \
  }                                                                           \
                                                                              \
  M_P(value_t *, name, _safe_get, tree_t b, key_t const key)                  \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                             \
    /* Not optimized implementation */                                        \
    value_t *ret = M_F(name, _get)(b, key);                                   \
    if (ret == NULL) {                                                        \
      M_IF(isMap)(                                                            \
        value_t v;                                                            \
        M_CALL_INIT(value_oplist, v);                                         \
        M_DEFER( M_CALL_CLEAR(value_oplist, v)) {                             \
          M_F(name, _set_at)M_R(b, key, v);                                   \
        }                                                                     \
      ,                                                                       \
        M_F(name, _push)(b, key);                                             \
      )                                                                       \
      ret = M_F(name, _get)(b, key);                                          \
    }                                                                         \
    return ret;                                                               \
  }                                                                           \
                                                                              \
  M_P(int, name, _i_search_and_remove_in_leaf, node_t n, key_t const key)     \
  {                                                                           \
    M_ASSERT(M_F(name, _is_leaf)(n));                                         \
    M_UNUSED_CONTEXT();                                                       \
    const int num = M_F(name, _get_num)(n);                                   \
    for(int i = 0; i < num; i++) {                                            \
      const int cmp = M_CALL_CMP(key_oplist, key, n->key[i]);                 \
      if (cmp == 0) {                                                         \
        /* found key ==> delete it */                                         \
        M_CALL_CLEAR(key_oplist, n->key[i]);                                  \
        M_IF(isMap)(M_CALL_CLEAR(value_oplist, n->kind.value[i]);,)           \
        memmove(&n->key[i], &n->key[i+1], sizeof(key_t)*(unsigned int)(num-1-i)); \
        M_IF(isMap)(memmove(&n->kind.value[i], &n->kind.value[i+1], sizeof(value_t)*(unsigned int)(num-1-i));,) \
        n->num -= -1; /* decrease number as num is < 0 */                     \
        return i;                                                             \
      }                                                                       \
    }                                                                         \
    return -1; /* Not found */                                                \
  }                                                                           \
                                                                              \
  M_P(void, name, _i_left_shift, node_t parent, int k)                        \
  {                                                                           \
    M_ASSERT (parent != NULL && !M_F(name, _is_leaf)(parent));                \
    M_ASSERT (0 <= k && k < M_F(name, _get_num)(parent));                     \
    M_UNUSED_CONTEXT();                                                       \
    node_t left = parent->kind.node[k];                                       \
    node_t right = parent->kind.node[k+1];                                    \
    M_ASSERT (left != NULL && right != NULL);                                 \
    int num_left = M_F(name, _get_num)(left);                                 \
    int num_right = M_F(name, _get_num)(right);                               \
    M_ASSERT (num_left > N/2);                                                \
    M_ASSERT (num_right < N/2);                                               \
                                                                              \
    /* Move one item from the left node to the right node */                  \
    memmove(&right->key[1], &right->key[0], sizeof(key_t)*(unsigned int)num_right); \
    if (M_F(name, _is_leaf)(left)) {                                          \
      M_IF(isMap)(memmove (&right->kind.value[1], &right->kind.value[0], sizeof(value_t)*(unsigned int)num_right);,) \
      memmove (&right->key[0], &left->key[num_left-1], sizeof (key_t));       \
      M_IF(isMap)(memmove (&right->kind.value[0], &left->kind.value[num_left-1], sizeof (value_t));,) \
      right->num = -num_right - 1;                                            \
      left->num = -num_left + 1;                                              \
      M_CALL_SET(key_oplist, parent->key[k], left->key[num_left-2]);          \
    } else {                                                                  \
      memmove(&right->kind.node[1], &right->kind.node[0], sizeof(node_t)*(unsigned int)(num_right+1)); \
      /* parent[k] is moved to right[0] (clear). parent[k] is therefore clear */ \
      memmove(&right->key[0], &parent->key[k], sizeof(key_t));                \
      right->kind.node[0] = left->kind.node[num_left];                        \
      right->num = num_right + 1;                                             \
      left->num = num_left - 1;                                               \
      /* left[n-1] is move to parent[k] (clear). left[n-1] is therefore clear */ \
      memmove(&parent->key[k], &left->key[num_left-1], sizeof (key_t));       \
    }                                                                         \
    M_ASSERT (right->num != 0);                                               \
    M_ASSERT (left->num != 0);                                                \
  }                                                                           \
                                                                              \
  M_P(void, name, _i_right_shift, node_t parent, int k)                       \
  {                                                                           \
    M_ASSERT (parent != NULL && !M_F(name, _is_leaf)(parent));                \
    M_ASSERT (0 <= k && k < M_F(name, _get_num)(parent));                     \
    M_UNUSED_CONTEXT();                                                       \
    node_t left = parent->kind.node[k];                                       \
    node_t right = parent->kind.node[k+1];                                    \
    M_ASSERT (left != NULL && right != NULL);                                 \
    int num_left = M_F(name, _get_num)(left);                                 \
    int num_right = M_F(name, _get_num)(right);                               \
    M_ASSERT (num_left < N/2);                                                \
    M_ASSERT (num_right > N/2);                                               \
                                                                              \
    /* Move one item from the right node to the left node. */                 \
    if (M_F(name, _is_leaf)(right)) {                                         \
      memmove (&left->key[num_left], &right->key[0], sizeof(key_t));          \
      memmove (&right->key[0], &right->key[1], sizeof(key_t)*(unsigned int)(num_right-1)); \
      M_IF(isMap)(memmove (&left->kind.value[num_left], &right->kind.value[0], sizeof (value_t));,) \
      M_IF(isMap)(memmove (&right->kind.value[0], &right->kind.value[1], sizeof(value_t)*(unsigned int)(num_right-1));,) \
      right->num = -num_right + 1;                                            \
      left->num = -num_left - 1;                                              \
      M_CALL_SET(key_oplist, parent->key[k], left->key[num_left]);            \
    } else {                                                                  \
      memmove (&left->key[num_left], &parent->key[k], sizeof (key_t));        \
      memmove (&parent->key[k], &right->key[0], sizeof (key_t));              \
      memmove (&right->key[0], &right->key[1], sizeof(key_t)*(unsigned int)(num_right-1)); \
      left->kind.node[num_left+1] = right->kind.node[0];                      \
      memmove (&right->kind.node[0], &right->kind.node[1], sizeof(node_t)*(unsigned int)num_right); \
      right->num = num_right - 1;                                             \
      left->num = num_left + 1;                                               \
    }                                                                         \
    M_ASSERT (right->num != 0);                                               \
    M_ASSERT (left->num != 0);                                                \
  }                                                                           \
                                                                              \
  M_P(void, name, _i_merge_node, node_t parent, int k, bool leaf)             \
  {                                                                           \
    M_ASSERT (parent != NULL && !M_F(name, _is_leaf)(parent));                \
    M_ASSERT (0 <= k && k < M_F(name, _get_num(parent)));                     \
    node_t left = parent->kind.node[k];                                       \
    node_t right = parent->kind.node[k+1];                                    \
    M_ASSERT (left != NULL && right != NULL);                                 \
    int num_parent = M_F(name, _get_num)(parent);                             \
    int num_left   = M_F(name, _get_num)(left);                               \
    int num_right  = M_F(name, _get_num)(right);                              \
                                                                              \
    /* Merge node 'k' and 'k+1' into a single one */                          \
    if (leaf) {                                                               \
      M_ASSERT (num_left + num_right <= N);                                   \
      memmove(&left->key[num_left], &right->key[0], sizeof(key_t)*(unsigned int)num_right); \
      M_IF(isMap)(memmove(&left->kind.value[num_left], &right->kind.value[0], sizeof(value_t)*(unsigned int)num_right);,) \
      left->num = -num_left - num_right;                                      \
    } else {                                                                  \
      M_ASSERT (num_left + num_right <= N -1);                                \
      memmove(&left->key[num_left+1], &right->key[0], sizeof(key_t)*(unsigned int)num_right); \
      memmove(&left->kind.node[num_left+1], &right->kind.node[0], sizeof(node_t)*(unsigned int)(num_right+1)); \
      M_CALL_INIT_SET(key_oplist, left->key[num_left], parent->key[k]);       \
      left->num = num_left + 1 + num_right;                                   \
    }                                                                         \
    left->next = right->next;                                                 \
    if (left->next != NULL) {                                                 \
      left->next->prev = left;                                                \
    }                                                                         \
    M_CALL_DEL(key_oplist, right);                                            \
    /* remove k'th key from the parent */                                     \
    M_CALL_CLEAR(key_oplist, parent->key[k]);                                 \
    memmove(&parent->key[k], &parent->key[k+1], sizeof(key_t)*(unsigned int)(num_parent - k - 1)); \
    memmove(&parent->kind.node[k+1], &parent->kind.node[k+2], sizeof(node_t)*(unsigned int)(num_parent - k -1)); \
    parent->num --;                                                           \
  }                                                                           \
                                                                              \
  /* We can also cache the index when we descend the tree.                    \
     TODO: Bench if this is worth the effort.*/                               \
  M_INLINE int                                                                \
  M_F(name, _i_search_for_node)(node_t parent, node_t child)                  \
  {                                                                           \
    M_ASSERT (!M_F(name, _is_leaf)(parent));                                  \
    int num = M_F(name, _get_num)(parent);                                    \
    for (int i = 0; i <= num; i++) {                                          \
      if (parent->kind.node[i] == child)                                      \
        return i;                                                             \
    }                                                                         \
    return -1;                                                                \
  }                                                                           \
                                                                              \
  M_P(bool, name, _erase, tree_t b, key_t const key)                          \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                             \
    pit_t pit;                                                                \
    node_t leaf = M_F(name, _i_search_for_leaf)(pit, b, key);                 \
    int k = M_F(name, _i_search_and_remove_in_leaf)M_R(leaf, key);            \
    /* If key is not found ==> erase failed */                                \
    if (k < 0) return false;                                                  \
    /* Remove one item from the B+TREE */                                     \
    b->size --;                                                               \
    /* If number of keys greater than N>2 or root ==> Nothing more to do */   \
    if (M_LIKELY (M_F(name, _get_num)(leaf) >= N/2) || pit->num == 0)         \
      return true;                                                            \
    /* Leaf is too small. Needs rebalancing */                                \
    M_ASSERT (M_F(name, _get_num)(leaf) == N/2-1);                            \
    bool pass1 = true;                                                        \
    while (true) {                                                            \
      M_ASSERT (pit->num > 0);                                                \
      /* Search for node 'leaf' in parent */                                  \
      node_t parent = pit->parent[--pit->num];                                \
      M_ASSERT (parent != NULL);                                              \
      k = M_F(name, _i_search_for_node)(parent, leaf);                        \
      M_ASSUME(k >= 0);                                                       \
      /* Look for the neighbour of the removed key. */                        \
      /* if we can steal one key from them to keep our node balanced */       \
      if (k > 0 && M_F(name, _get_num)(parent->kind.node[k-1]) > N/2) {       \
        M_F(name, _i_left_shift)M_R(parent, k-1);                             \
        return true;                                                          \
      } else if (k < M_F(name, _get_num)(parent)                              \
                 && M_F(name, _get_num)(parent->kind.node[k+1]) > N/2) {      \
        M_F(name, _i_right_shift)M_R(parent, k);                              \
        return true;                                                          \
      }                                                                       \
      /* Merge both nodes, removing 'k' from parent */                        \
      if (k == M_F(name, _get_num)(parent))                                   \
        k--;                                                                  \
      M_ASSERT(k >= 0 && k < M_F(name, _get_num)(parent));                    \
      /* Merge 'k' & 'k+1' & remove 'k' from parent */                        \
      M_F(name, _i_merge_node)M_R(parent, k, pass1);                          \
      /* Check if we need to continue */                                      \
      if (M_F(name, _get_num)(parent) >= N/2)                                 \
        return true;                                                          \
      if (pit->num == 0) {                                                    \
        /* We reach the root */                                               \
        if (M_F(name, _get_num)(parent) == 0) {                               \
          /* B+ Tree reduce its heigh by deleting the root: */                \
          /* Update root to its unique child and delete it */                 \
          b->root = parent->kind.node[0];                                     \
          M_CALL_DEL(key_oplist, parent);                                     \
        }                                                                     \
        return true;                                                          \
      }                                                                       \
      /* Next iteration */                                                    \
      leaf = parent;                                                          \
      pass1 = false;                                                          \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_P(bool, name, _pop_at, value_t *ptr, tree_t b, key_t const key)           \
  {                                                                           \
    if (ptr != NULL) {                                                        \
      /* Not optimized tree parsing */                                        \
      value_t *ref = M_F(name, _get)(b, key);                                 \
      if (ref == NULL) {                                                      \
        return false;                                                         \
      }                                                                       \
      M_CALL_SET(value_oplist, *ptr, *ref);                                   \
    }                                                                         \
    return M_F(name, _erase)M_R(b, key);                                      \
  }                                                                           \
                                                                              \
  M_INLINE value_t *                                                          \
  M_F(name, _min)(const tree_t b)                                             \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                             \
    if (M_UNLIKELY (b->size == 0)) return NULL;                               \
    node_t n = b->root;                                                       \
    /* Scan down the nodes */                                                 \
    while (!M_F(name, _is_leaf)(n)) {                                         \
      n = n->kind.node[0];                                                    \
    }                                                                         \
    return &n->M_IF(isMap)(kind.value, key)[0];                               \
  }                                                                           \
                                                                              \
  M_INLINE value_t *                                                          \
  M_F(name, _max)(const tree_t b)                                             \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                             \
    if (M_UNLIKELY (b->size == 0)) return NULL;                               \
    node_t n = b->root;                                                       \
    /* Scan down the nodes */                                                 \
    while (!M_F(name, _is_leaf)(n)) {                                         \
      n = n->kind.node[n->num];                                               \
    }                                                                         \
    return &n->M_IF(isMap)(kind.value, key)[-n->num-1];                       \
  }                                                                           \
                                                                              \
  M_INLINE value_t const *                                                    \
  M_F(name, _cmin)(const tree_t tree)                                         \
  {                                                                           \
    return M_CONST_CAST(value_t, M_F(name, _min)(tree));                      \
  }                                                                           \
                                                                              \
  M_INLINE value_t const *                                                    \
  M_F(name, _cmax)(const tree_t tree)                                         \
  {                                                                           \
    return M_CONST_CAST(value_t, M_F(name, _max)(tree));                      \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _init_move)(tree_t b, tree_t ref)                                 \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, ref);                           \
    M_ASSERT (b != NULL && b != ref);                                         \
    b->size = ref->size;                                                      \
    b->root = ref->root;                                                      \
    ref->root = NULL;                                                         \
    ref->size = 1;                                                            \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                             \
  }                                                                           \
                                                                              \
  M_P(void, name, _move, tree_t b, tree_t ref)                                \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                             \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, ref);                           \
    M_ASSERT (b != ref);                                                      \
    M_F(name,_clear)M_R(b);                                                   \
    M_F(name,_init_move)(b, ref);                                             \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                             \
  }                                                                           \
                                                                              \
  M_P(void, name, _set, tree_t b, const tree_t o)                             \
  {                                                                           \
    /* NOTE: We could reuse the already allocated nodes of 'b'.               \
       Not sure if it worth the effort */                                     \
    tree_t tmp;                                                               \
    M_ON_EXCEPTION( /* nothing*/) {                                           \
      M_F(name, _init_set)M_R(tmp, o);                                        \
      M_F(name, _move)M_R(b, tmp);                                            \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _swap)(tree_t tree1, tree_t tree2)                                \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, tree1);                         \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, tree2);                         \
    M_SWAP(size_t, tree1->size, tree2->size);                                 \
    M_SWAP(node_t, tree1->root, tree2->root);                                 \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, tree1);                         \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, tree2);                         \
  }                                                                           \

/* Define iterator functions. */
#define M_BPTR33_DEF_IT(name, N, key_t, key_oplist, value_t, value_oplist, isMap, isMulti, tree_t, node_t, pit_t, it_t, subtype_t) \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it)(it_t it, const tree_t b)                                     \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                             \
    M_ASSERT (it != NULL);                                                    \
    node_t n = b->root;                                                       \
    /* Scan down the nodes */                                                 \
    while (!M_F(name, _is_leaf)(n)) {                                         \
      n = n->kind.node[0];                                                    \
    }                                                                         \
    it->node = n;                                                             \
    it->idx  = 0;                                                             \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_end)(it_t it, const tree_t b)                                 \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                             \
    M_ASSERT (it != NULL);                                                    \
    node_t n = b->root;                                                       \
    /* Scan down the nodes */                                                 \
    while (!M_F(name, _is_leaf)(n)) {                                         \
      n = n->kind.node[n->num];                                               \
    }                                                                         \
    it->node = n;                                                             \
    it->idx  = -n->num;                                                       \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_set)(it_t itd, const it_t its)                                \
  {                                                                           \
    M_ASSERT (itd != NULL && its != NULL);                                    \
    itd->node = its->node;                                                    \
    itd->idx  = its->idx;                                                     \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _end_p)(it_t it)                                                  \
  {                                                                           \
    M_ASSERT (it != NULL && it->node != NULL);                                \
    M_ASSERT (M_F(name, _is_leaf)(it->node));                                 \
    /* Test if the unsigned value is bigger:                                  \
    allow to test both previous iteration (<0) and next iteration (>num) */   \
    return (unsigned) it->idx >= (unsigned) -it->node->num;                   \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _it_equal_p)(const it_t it1, const it_t it2)                      \
  {                                                                           \
    return it1->node == it2->node && it1->idx == it2->idx;                    \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _next)(it_t it)                                                   \
  {                                                                           \
    M_ASSERT (it != NULL && it->node != NULL);                                \
    M_ASSERT (M_F(name, _is_leaf)(it->node));                                 \
    it->idx ++;                                                               \
    if (it->idx >= -it->node->num && it->node->next != NULL) {                \
      it->node = it->node->next;                                              \
      it->idx = 0;                                                            \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _previous)(it_t it)                                               \
  {                                                                           \
    M_ASSERT (it != NULL && it->node != NULL);                                \
    M_ASSERT (M_F(name, _is_leaf)(it->node));                                 \
    it->idx --;                                                               \
    if (it->idx < 0 && it->node->prev != NULL) {                              \
      it->node = it->node->prev;                                              \
      it->idx = -it->node->num - 1;                                           \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_last)(it_t it, const tree_t b)                                \
  {                                                                           \
    M_F(name, _it_end)(it, b);                                                \
    M_F(name, _previous)(it);                                                 \
  }                                                                           \
                                                                              \
  M_INLINE subtype_t *                                                        \
  M_F(name, _ref)(it_t it)                                                    \
  {                                                                           \
    M_ASSERT (it != NULL && it->node != NULL);                                \
    M_ASSERT (M_F(name, _is_leaf)(it->node));                                 \
    M_ASSERT (it->idx >= 0 && it->idx <= -it->node->num);                     \
    M_IF(isMap)(                                                              \
                it->pair.key_ptr = &it->node->key[it->idx];                   \
                it->pair.value_ptr = &it->node->kind.value[it->idx];          \
                return &it->pair                                              \
                ,                                                             \
                return &it->node->key[it->idx]                                \
                                                                        );    \
  }                                                                           \
                                                                              \
  M_INLINE subtype_t const *                                                  \
  M_F(name, _cref)(it_t it)                                                   \
  {                                                                           \
    return M_CONST_CAST(subtype_t, M_F(name, _ref)(it));                      \
  }                                                                           \
                                                                              \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_from)(it_t it, const tree_t b, key_t const key)               \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                             \
    M_ASSERT (it != NULL);                                                    \
    pit_t pit;                                                                \
    node_t n = M_F(name, _i_search_for_leaf)(pit, b, key);                    \
    it->node = n;                                                             \
    int i;                                                                    \
    M_BPTR33_NODE_CONTRACT(N, isMulti, key_oplist, n, b->root);               \
    for(i = 0; i < -n->num; i++) {                                            \
      if (M_CALL_CMP(key_oplist, key, n->key[i]) <= 0)                        \
        break;                                                                \
    }                                                                         \
    if (i == -n->num && n->next != NULL) {                                    \
      it->node = n->next;                                                     \
      i = 0;                                                                  \
    }                                                                         \
    it->idx  = i;                                                             \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _it_until_p)(it_t it, key_t const key)                            \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    node_t n = it->node;                                                      \
    if ((unsigned) it->idx >= (unsigned) -n->num) return (it->idx >= 0);      \
    int cmp = M_CALL_CMP(key_oplist, n->key[it->idx], key);                   \
    return (cmp >= 0);                                                        \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _it_while_p)(it_t it, key_t const key)                            \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    node_t n = it->node;                                                      \
    if ((unsigned) it->idx >= (unsigned) -n->num) return (it->idx < 0);       \
    int cmp = M_CALL_CMP(key_oplist, n->key[it->idx], key);                   \
    return (cmp <= 0);                                                        \
  }                                                                           \
                                                                              \
  M_P(void, name, _remove, tree_t b, it_t it)                                 \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, b);                             \
    node_t n = it->node; /* It is a leaf*/                                    \
    M_ASSERT( n->num < 0);                                                    \
    int i = it->idx;                                                          \
    int num = M_F(name, _get_num)(n);                                         \
    M_ASSERT (i >= 0 && i < num);                                             \
    /* Get an index of an existing remaining key of the node. */              \
    /* Unfortunately, if N=2 or N=3, we can no longer have any remaining keys in this node*/ \
    /* so we need to fill in the pit table before removing even if it can be useless */ \
    pit_t pit;                                                                \
    node_t leaf = M_F(name, _i_search_for_leaf)(pit, b, n->key[i]);           \
    /* Remove the exact referenced object from the object */                  \
    M_CALL_CLEAR(key_oplist, n->key[i]);                                      \
    M_IF(isMap)(M_CALL_CLEAR(value_oplist, n->kind.value[i]);,)               \
    memmove(&n->key[i], &n->key[i+1], sizeof(key_t)*(unsigned int)(num-1-i)); \
    M_IF(isMap)(memmove(&n->kind.value[i], &n->kind.value[i+1], sizeof(value_t)*(unsigned int)(num-1-i));,) \
    n->num -= -1; /* decrease number as num is < 0 */                         \
    /* Remove one item from the B+TREE */                                     \
    b->size --;                                                               \
    /* Update it to point to the next item */                                 \
    if (M_UNLIKELY(i >= num-1 && n->next != NULL)) {                          \
      /* Reset to next node */                                                \
      it->node = n->next;                                                     \
      it->idx = 0;                                                            \
    }                                                                         \
    /* If number of keys greater than N>2 or the node is root ==> Nothing more to do */ \
    if (M_LIKELY (M_F(name, _get_num)(n) >= N/2) || (b->root == n))           \
      return;                                                                 \
    /* Update the B+ Tree to keep it balanced */                              \
    /* We need to update the 'pit' table to reference the item of the node (multimap case) */ \
    while (M_UNLIKELY(n != leaf)) {                                           \
      /* multimap case where the leaf associated to the key is not ok */      \
      /* we need to go to the next one and fix pit */                         \
      M_ASSERT(leaf->next != NULL);                                           \
      leaf = leaf->next;                                                      \
      int p_num = pit->num;                                                   \
      node_t p_n = leaf;                                                      \
      do {                                                                    \
        M_ASSERT(p_num >= 1);                                                 \
        node_t parent = pit->parent[--p_num];                                 \
        int k = M_F(name, _i_search_for_node)(parent, p_n);                   \
        if (k >= 0) break;                                                    \
        /* p_n not found in parent, go to next parent */                      \
        pit->parent[p_num] = pit->parent[p_num]->next;                        \
        p_n = pit->parent[p_num];                                             \
        /* perform another loop as the great parent may need update too */    \
      } while (1);                                                            \
    }                                                                         \
      /* Leaf is too small. Needs rebalancing */                              \
    M_ASSERT (M_F(name, _get_num)(leaf) == N/2-1);                            \
    bool pass1 = true;                                                        \
    while (true) {                                                            \
      M_ASSERT (pit->num > 0);                                                \
      /* Search for node 'leaf' in parent */                                  \
      node_t parent = pit->parent[--pit->num];                                \
      M_ASSERT (parent != NULL);                                              \
      int k = M_F(name, _i_search_for_node)(parent, leaf);                    \
      M_ASSUME(k >= 0);                                                       \
      /* Look for the neighbour of the removed key. */                        \
      /* if we can steal one key from them to keep our node balanced */       \
      if (k > 0 && M_F(name, _get_num)(parent->kind.node[k-1]) > N/2) {       \
        M_F(name, _i_left_shift)M_R(parent, k-1);                             \
        /* Fix iterator 'it' so that it still references the same element */  \
        if (it->node == parent->kind.node[k]) {                               \
          it->idx ++;                                                         \
        } else if (it->node == parent->kind.node[k-1]) {                      \
          if (it->idx >= M_F(name, _get_num)(it->node)) {                     \
            it->idx = 0;                                                      \
            it->node = parent->kind.node[k-1];                                \
          }                                                                   \
        }                                                                     \
        return ;                                                              \
      } else if (k < M_F(name, _get_num)(parent)                              \
                 && M_F(name, _get_num)(parent->kind.node[k+1]) > N/2) {      \
        M_F(name, _i_right_shift)M_R(parent, k);                              \
        /* Fix iterator 'it' so that it still references the same element */  \
        if (it->node == parent->kind.node[k+1]) {                             \
          if (it->idx == 0) {                                                 \
            it->node = parent->kind.node[k];                                  \
            it->idx = M_F(name, _get_num)(parent->kind.node[k])-1;            \
          } else {                                                            \
            it->idx --;                                                       \
          }                                                                   \
        }                                                                     \
        return ;                                                              \
      }                                                                       \
      /* Merge both nodes, removing 'k' from parent */                        \
      if (k == M_F(name, _get_num)(parent))                                   \
        k--;                                                                  \
      M_ASSERT(k >= 0 && k < M_F(name, _get_num)(parent));                    \
      /* Merge 'k' & 'k+1' & remove 'k' from parent */                        \
      num = M_F(name, _get_num)(parent->kind.node[k]);                        \
      bool it_to_fix = it->node == parent->kind.node[k+1];                    \
      M_F(name, _i_merge_node)M_R(parent, k, pass1);                          \
      /* Fix iterator 'it' so that it still references the same element */    \
      if (it_to_fix) {                                                        \
        it->node = parent->kind.node[k];                                      \
        it->idx += num;                                                       \
      }                                                                       \
      /* Check if we need to continue */                                      \
      if (M_F(name, _get_num)(parent) >= N/2)                                 \
        return ;                                                              \
      if (pit->num == 0) {                                                    \
        /* We reach the root */                                               \
        if (M_F(name, _get_num)(parent) == 0) {                               \
          /* Update root (deleted) */                                         \
          b->root = parent->kind.node[0];                                     \
          M_CALL_DEL(key_oplist, parent);                                     \
        }                                                                     \
        return ;                                                              \
      }                                                                       \
      /* Next iteration */                                                    \
      leaf = parent;                                                          \
      pass1 = false;                                                          \
    }                                                                         \
  }                                                                           \

/* Define additional functions.
   Do not used any fields but the already defined methods */
#define M_BPTR33_DEF_EXT(name, N, key_t, key_oplist, value_t, value_oplist, isMap, isMulti, tree_t, node_t, pit_t, it_t, subtype_t) \
                                                                              \
  M_IF_METHOD_BOTH(EQUAL, key_oplist, value_oplist)(                          \
  M_INLINE bool M_F(name,_equal_p)(const tree_t t1, const tree_t t2) {        \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, t1);                            \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, t2);                            \
    if (t1->size != t2->size) return false;                                   \
    if (t1->size == 0) return true;                                           \
    /* Slow comparaison */                                                    \
    it_t it1;                                                                 \
    it_t it2;                                                                 \
    /* NOTE: We can't compare two trees directly as they can be               \
       structurally different but functionally equal (you get this by         \
       constructing the tree in a different way). We have to                  \
       compare the ordered value within the tree. */                          \
    M_F(name, _it)(it1, t1);                                                  \
    M_F(name, _it)(it2, t2);                                                  \
    while (!M_F(name, _end_p)(it1)                                            \
           && !M_F(name, _end_p)(it2)) {                                      \
      const subtype_t *ref1 = M_F(name, _cref)(it1);                          \
      const subtype_t *ref2 = M_F(name, _cref)(it2);                          \
      M_IF(isMap)(                                                            \
      if (!M_CALL_EQUAL(key_oplist, *ref1->key_ptr, *ref2->key_ptr))          \
        return false;                                                         \
      if (!M_CALL_EQUAL(value_oplist, *ref1->value_ptr, *ref2->value_ptr))    \
        return false;                                                         \
      ,                                                                       \
      if (!M_CALL_EQUAL(key_oplist, *ref1, *ref2))                            \
        return false;                                                         \
                                                                        )     \
      M_F(name, _next)(it1);                                                  \
      M_F(name, _next)(it2);                                                  \
    }                                                                         \
    return M_F(name, _end_p)(it1)                                             \
      && M_F(name, _end_p)(it2);                                              \
  }                                                                           \
  , /* NO EQUAL METHOD */ )                                                   \
                                                                              \
  M_IF_METHOD_BOTH(HASH, key_oplist, value_oplist)(                           \
  M_INLINE size_t M_F(name,_hash)(const tree_t t1) {                          \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, t1);                            \
    M_HASH_DECL(hash);                                                        \
    /* NOTE: We can't compute the hash directly for the same reason           \
       than for EQUAL operator. */                                            \
    it_t it1;                                                                 \
    M_F(name, _it)(it1, t1);                                                  \
    while (!M_F(name, _end_p)(it1)) {                                         \
      subtype_t const *ref1 = M_F(name, _cref)(it1);                          \
      M_IF(isMap)(                                                            \
                  M_HASH_UP(hash, M_CALL_HASH(key_oplist, *ref1->key_ptr));   \
                  M_HASH_UP(hash, M_CALL_HASH(value_oplist, *ref1->value_ptr)); \
                  ,                                                           \
                  M_HASH_UP(hash, M_CALL_HASH(key_oplist, *ref1));            \
                                                                        )     \
      M_F(name, _next)(it1);                                                  \
    }                                                                         \
    return M_HASH_FINAL (hash);                                               \
  }                                                                           \
  , /* NO HASH METHOD */ )                                                    \
                                                                              \
  M_IF_METHOD_BOTH(GET_STR, key_oplist, value_oplist)(                        \
  M_P(void, name, _get_str, m_string_t str, const tree_t t1, bool append)     \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, t1);                            \
    M_ASSERT(str != NULL);                                                    \
    (append ? m_string_cat_cstr : m_string_set_cstr) M_R(str, "[");           \
    bool commaToPrint = false;                                                \
    it_t it;                                                                  \
    for (M_F(name, _it)(it, t1) ;                                             \
         !M_F(name, _end_p)(it);                                              \
         M_F(name, _next)(it)) {                                              \
      if (commaToPrint)                                                       \
        m_string_push_back M_R(str, M_GET_SEPARATOR key_oplist);              \
      commaToPrint = true;                                                    \
      subtype_t const *ref1 = M_F(name, _cref)(it);                           \
      M_IF(isMap)(                                                            \
                  M_CALL_GET_STR(key_oplist, str, *ref1->key_ptr, true);      \
                  m_string_cat_cstr M_R(str, ":");                            \
                  M_CALL_GET_STR(value_oplist,str, *ref1->value_ptr, true)    \
                  ,                                                           \
                  M_CALL_GET_STR(key_oplist, str, *ref1, true);               \
                                                                        );    \
    }                                                                         \
    m_string_push_back M_R(str, ']');                                         \
  }                                                                           \
  , /* NO GET_STR */ )                                                        \
                                                                              \
  M_IF_METHOD_BOTH(OUT_STR, key_oplist, value_oplist)(                        \
  M_INLINE void                                                               \
  M_F(name, _out_str)(FILE *file, tree_t const t1)                            \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, t1);                            \
    M_ASSERT (file != NULL);                                                  \
    fputc ('[', file);                                                        \
    bool commaToPrint = false;                                                \
    it_t it;                                                                  \
    for (M_F(name, _it)(it, t1) ;                                             \
         !M_F(name, _end_p)(it);                                              \
         M_F(name, _next)(it)){                                               \
      if (commaToPrint)                                                       \
        fputc (M_GET_SEPARATOR key_oplist, file);                             \
      commaToPrint = true;                                                    \
      subtype_t const *ref1 = M_F(name, _cref)(it);                           \
      M_IF(isMap)(                                                            \
                  M_CALL_OUT_STR(key_oplist, file, *ref1->key_ptr);           \
                  fputc (':', file);                                          \
                  M_CALL_OUT_STR(value_oplist, file, *ref1->value_ptr)        \
                  ,                                                           \
                  M_CALL_OUT_STR(key_oplist, file, *ref1);                    \
                                                                        );    \
    }                                                                         \
    fputc (']', file);                                                        \
  }                                                                           \
  , /* no out_str */ )                                                        \
                                                                              \
  M_IF_METHOD_BOTH(PARSE_STR, key_oplist, value_oplist)(                      \
  M_P(bool, name, _parse_str, tree_t t1, const char str[], const char **endp) \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, t1);                            \
    M_ASSERT (str != NULL);                                                   \
    M_F(name,_reset)M_R(t1);                                                  \
    int c = *str++;                                                           \
    if (M_UNLIKELY (c != '[')) { c = 0; goto exit; }                          \
    c = *str++;                                                               \
    if (M_UNLIKELY (c == ']' || c == 0)) goto exit;                           \
    str--;                                                                    \
    M_QLET(1, key, key_t, key_oplist)                                         \
    M_IF(isMap)(M_QLET(2, value, value_t, value_oplist), /* No isMap */) {    \
      do {                                                                    \
        bool b = M_CALL_PARSE_STR(key_oplist, key, str, &str);                \
        c = m_core_str_nospace(&str);                                         \
        if (b == false) { c = 0; break; }                                     \
        M_IF(isMap)(if (c != ':') { c = 0; break; }                           \
                    b = M_CALL_PARSE_STR(value_oplist, value, str, &str);     \
                    c = m_core_str_nospace(&str);                             \
        if (b == false || c == 0) { c = 0; break; }                           \
        M_F(name, _set_at)M_R(t1, key, value);                                \
        ,                                                                     \
        M_F(name, _push)M_R(t1, key);                                         \
        )                                                                     \
      } while (c == M_GET_SEPARATOR key_oplist);                              \
    }                                                                         \
  exit:                                                                       \
    if (endp) *endp = str;                                                    \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, t1);                            \
    return (c == ']');                                                        \
  }                                                                           \
  , /* no parse_str */ )                                                      \
                                                                              \
  M_IF_METHOD_BOTH(IN_STR, key_oplist, value_oplist)(                         \
  M_P(bool, name, _in_str, tree_t t1, FILE *file)                             \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, t1);                            \
    M_ASSERT (file != NULL);                                                  \
    M_F(name,_reset)M_R(t1);                                                  \
    int c = fgetc(file);                                                      \
    if (M_UNLIKELY (c != '[')) return false;                                  \
    c = fgetc(file);                                                          \
    if (M_UNLIKELY (c == ']')) return true;                                   \
    if (M_UNLIKELY (c == EOF)) return false;                                  \
    ungetc(c, file);                                                          \
    M_QLET(1, key, key_t, key_oplist)                                         \
    M_IF(isMap)(M_QLET(2, value, value_t, value_oplist) , /*nothing*/) {      \
      do {                                                                    \
        bool b = M_CALL_IN_STR(key_oplist, key, file);                        \
        c = m_core_fgetc_nospace(file);                                       \
        if (b == false) { c = 0; break; }                                     \
        M_IF(isMap)(if (c!=':') { c = 0; break; }                             \
                    b = M_CALL_IN_STR(value_oplist,value, file);              \
                    c = m_core_fgetc_nospace(file);                           \
                    if (b == false || c == EOF) { c = 0; break; }             \
                    M_F(name, _set_at)M_R(t1, key, value)                     \
                    ,                                                         \
                    M_F(name, _push)M_R(t1, key)                              \
                    );                                                        \
      } while (c == M_GET_SEPARATOR key_oplist);                              \
    }                                                                         \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, t1);                            \
    return c == ']';                                                          \
  }                                                                           \
  , /* no in_str */ )                                                         \
                                                                              \
  M_IF_METHOD_BOTH(OUT_SERIAL, key_oplist, value_oplist)(                     \
  M_P(m_serial_return_code_t, name, _out_serial, m_serial_write_t f, tree_t const t1) \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, t1);                            \
    M_ASSERT (f != NULL && f->m_interface != NULL);                           \
    m_serial_return_code_t ret;                                               \
    m_serial_local_t local;                                                   \
    subtype_t const *item;                                                    \
    bool first_done = false;                                                  \
    it_t it;                                                                  \
    /* Format is different between associative container                      \
       & set container */                                                     \
    M_IF(isMap)(                                                              \
                ret = f->m_interface->write_map_start M_R(local, f, t1->size); \
                for (M_F(name, _it)(it, t1) ;                                 \
                     !M_F(name, _end_p)(it);                                  \
                     M_F(name, _next)(it)){                                   \
                  item = M_F(name, _cref)(it);                                \
                  if (first_done)                                             \
                    ret |= f->m_interface->write_map_next M_R(local, f);      \
                  ret |= M_CALL_OUT_SERIAL(key_oplist, f, *item->key_ptr);    \
                  ret |= f->m_interface->write_map_value M_R(local, f);       \
                  ret |= M_CALL_OUT_SERIAL(value_oplist, f, *item->value_ptr); \
                  first_done = true;                                          \
                }                                                             \
                ret |= f->m_interface->write_map_end M_R(local, f);           \
                ,                                                             \
                ret = f->m_interface->write_array_start M_R(local, f, t1->size); \
                for (M_F(name, _it)(it, t1) ;                                 \
                     !M_F(name, _end_p)(it);                                  \
                     M_F(name, _next)(it)){                                   \
                  item = M_F(name, _cref)(it);                                \
                  if (first_done)                                             \
                    ret |= f->m_interface->write_array_next M_R(local, f);    \
                  ret |= M_CALL_OUT_SERIAL(key_oplist, f, *item);             \
                  first_done = true;                                          \
                }                                                             \
                ret |= f->m_interface->write_array_end M_R(local, f);         \
                                                                        )     \
      return ret & M_SERIAL_FAIL;                                             \
  }                                                                           \
  , /* no OUT_SERIAL */ )                                                     \
                                                                              \
  M_IF_METHOD_BOTH(IN_SERIAL, key_oplist, value_oplist)(                      \
  M_P(m_serial_return_code_t, name, _in_serial, tree_t t1, m_serial_read_t f) \
  {                                                                           \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, t1);                            \
    M_ASSERT (f != NULL && f->m_interface != NULL);                           \
    m_serial_local_t local;                                                   \
    m_serial_return_code_t ret;                                               \
    size_t estimated_size = 0;                                                \
    M_F(name,_reset)M_R(t1);                                                  \
    M_QLET(1, key, key_t, key_oplist)                                         \
    M_IF(isMap)(M_QLET(2, value, value_t, value_oplist) , /*nothing*/) {      \
        M_IF(isMap)(                                                          \
                ret = f->m_interface->read_map_start(local, f, &estimated_size); \
                if (M_UNLIKELY (ret != M_SERIAL_OK_CONTINUE)) break;          \
                do {                                                          \
                  ret = M_CALL_IN_SERIAL(key_oplist, key, f);                 \
                  if (ret != M_SERIAL_OK_DONE) break;                         \
                  ret = f->m_interface->read_map_value(local, f);             \
                  if (ret != M_SERIAL_OK_CONTINUE) break;                     \
                  ret = M_CALL_IN_SERIAL(value_oplist, value, f);             \
                  if (ret != M_SERIAL_OK_DONE) break;                         \
                  M_F(name, _set_at)M_R(t1, key, value);                      \
                } while ((ret = f->m_interface->read_map_next(local, f)) == M_SERIAL_OK_CONTINUE); \
        , /* queue style */                                                   \
                ret = f->m_interface->read_array_start(local, f, &estimated_size); \
                if (M_UNLIKELY (ret != M_SERIAL_OK_CONTINUE)) break;          \
                do {                                                          \
                  ret = M_CALL_IN_SERIAL(key_oplist, key, f);                 \
                  if (ret != M_SERIAL_OK_DONE) break;                         \
                  M_F(name, _push)M_R(t1, key);                               \
                } while ((ret = f->m_interface->read_array_next(local, f)) == M_SERIAL_OK_CONTINUE); \
        ) /* End of IF isMap */                                               \
    }                                                                         \
    M_BPTR33_CONTRACT(N, isMulti, key_oplist, t1);                            \
    return ret;                                                               \
  }                                                                           \
  , /* no in_serial */ )                                                      \


/******************************** INTERNAL ***********************************/

#if M_USE_SMALL_NAME
#define BPTREE_DEF2 M_BPTREE_DEF2
#define BPTREE_DEF2_AS M_BPTREE_DEF2_AS
#define BPTREE_DEF M_BPTREE_DEF
#define BPTREE_DEF_AS M_BPTREE_DEF_AS
#define BPTREE_MULTI_DEF2 M_BPTREE_MULTI_DEF2
#define BPTREE_MULTI_DEF2_AS M_BPTREE_MULTI_DEF2_AS
#define BPTREE_MULTI_DEF M_BPTREE_MULTI_DEF
#define BPTREE_MULTI_DEF_AS M_BPTREE_MULTI_DEF_AS
#define BPTREE_OPLIST M_BPTREE_OPLIST
#define BPTREE_OPLIST2 M_BPTREE_OPLIST2
#endif

#endif
