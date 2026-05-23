/*
 * M*LIB - TUPLE module
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
#ifndef MSTARLIB_TUPLE_H
#define MSTARLIB_TUPLE_H

#include "m-core.h"

/* Define the tuple type and functions.
   USAGE:
   TUPLE_DEF2(name, [(field1, type1[, oplist1]), (field2, type2[, oplist2]), ...] ) */
#define M_TUPLE_DEF2(name, ...)                                               \
  M_TUPLE_DEF2_AS(name, M_F(name,_t), __VA_ARGS__)


/* Define the tuple type and functions
   as the given name.
   USAGE:
   TUPLE_DEF2_AS(name, name_t, [(field1, type1[, oplist1]), (field2, type2[, oplist2]), ...] ) */
#define M_TUPLE_DEF2_AS(name, name_t, ...)                                    \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_TUPL3_DEF2_P1( (name, name_t M_TUPL3_INJECT_GLOBAL(__VA_ARGS__)) )        \
  M_END_PROTECTED_CODE


/* Define the oplist of a tuple.
   USAGE: TUPLE_OPLIST(name[, oplist of the first type, ...]) */
#define M_TUPLE_OPLIST(...)                                                   \
  M_IF_NARGS_EQ1(__VA_ARGS__)                                                 \
  (M_TUPL3_OPLIST_P1((__VA_ARGS__, M_BASIC_OPLIST )),                         \
   M_TUPL3_OPLIST_P1((__VA_ARGS__ )))


/* Return an array suitable for the WIP _cmp_order function.
   As compound literals are not supported in C++,
   provide a separate definition for C++ using initializer_list
   (shall be constexpr, but only supported in C++14).
*/
#ifndef __cplusplus
#define M_TUPLE_ORDER(name, ...)                                              \
  ( (const int[]) {M_MAP2_C(M_TUPL3_ORDER_CONVERT, name, __VA_ARGS__), 0})
#else
#include <initializer_list>
namespace m_lib {
  template <unsigned int N>
  struct m_tupl3_integer_va {
    int data[N];
    /*constexpr*/ inline m_tupl3_integer_va(std::initializer_list<int> init){
      int j = 0;
      for(auto i:init) {
        data[j++] = i;
      }
    }
  };
}
#define M_TUPLE_ORDER(name, ...)                                              \
  (m_lib::m_tupl3_integer_va<M_NARGS(__VA_ARGS__,0)>({M_MAP2_C(M_TUPL3_ORDER_CONVERT, name, __VA_ARGS__), 0}).data)
#endif


/*****************************************************************************/
/********************************** INTERNAL *********************************/
/*****************************************************************************/

/* Contract of a tuple. Nothing notable */
#define M_TUPL3_CONTRACT(tup) do {                                            \
  M_ASSERT(tup != NULL);                                                      \
} while (0)

/* Inject the oplist within the list of arguments */
#define M_TUPL3_INJECT_GLOBAL(...)                                            \
  M_MAP(M_TUPL3_INJECT_OPLIST_A, __VA_ARGS__)

/* Transform (x, type) into (x, type, oplist) if there is global registered oplist 
   or (x, type, M_BASIC_OPLIST) if there is no global one,
   or keep (x, type, oplist) if oplist was already present */
#define M_TUPL3_INJECT_OPLIST_A( duo_or_trio )                                \
  M_TUPL3_INJECT_OPLIST_B duo_or_trio

#define M_TUPL3_INJECT_OPLIST_B( f, ... )                                     \
  M_DEFERRED_COMMA                                                            \
  M_IF_NARGS_EQ1(__VA_ARGS__)( (f, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)()), (f, __VA_ARGS__) )

// Deferred evaluation
#define M_TUPL3_DEF2_P1(...)             M_ID( M_TUPL3_DEF2_P2 __VA_ARGS__ )

// Test if all third argument of all arguments is an oplist
#define M_TUPL3_IF_ALL_OPLIST(...)                                            \
  M_IF(M_REDUCE(M_TUPL3_IS_OPLIST_P, M_AND, __VA_ARGS__))

// Test if the third argument of (name, type, oplist) is an oplist
#define M_TUPL3_IS_OPLIST_P(a)                                                \
  M_OPLIST_P(M_RET_ARG3 a)

/* Validate the oplist before going further */
#define M_TUPL3_DEF2_P2(name, name_t, ...)                                    \
  M_TUPL3_IF_ALL_OPLIST(__VA_ARGS__)(M_TUPL3_DEF2_P3, M_TUPL3_DEF2_FAILURE)(name, name_t, __VA_ARGS__)

/* Stop processing with a compilation failure */
#define M_TUPL3_DEF2_FAILURE(name, name_t, ...)                               \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST, "(TUPLE_DEF2): at least one of the given argument is not a valid oplist: " #__VA_ARGS__)

/* Define the tuple */
#define M_TUPL3_DEF2_P3(name, name_t, ...)                                    \
  M_TUPL3_DEFINE_TYPE(name, name_t, __VA_ARGS__)                              \
  M_TUPL3_DEFINE_ENUM(name, __VA_ARGS__)                                      \
  M_TUPL3_CONTROL_ALL_OPLIST(name, __VA_ARGS__)                               \
  M_TUPL3_IF_ALL(INIT, __VA_ARGS__)(M_TUPL3_DEFINE_INIT(name, __VA_ARGS__),)  \
  M_TUPL3_DEFINE_INIT_SET(name, __VA_ARGS__)                                  \
  M_TUPL3_DEFINE_INIT_SET2(name, __VA_ARGS__)                                 \
  M_TUPL3_DEFINE_SET(name, __VA_ARGS__)                                       \
  M_TUPL3_DEFINE_SET2(name, __VA_ARGS__)                                      \
  M_TUPL3_DEFINE_CLEAR(name, __VA_ARGS__)                                     \
  M_TUPL3_DEFINE_GETTER_FIELD(name, __VA_ARGS__)                              \
  M_TUPL3_DEFINE_SETTER_FIELD(name, __VA_ARGS__)                              \
  M_TUPL3_DEFINE_EMPLACE_FIELD(name, __VA_ARGS__)                             \
  M_TUPL3_IF_ONE(CMP, __VA_ARGS__)(M_TUPL3_DEFINE_CMP(name, __VA_ARGS__),)    \
  M_TUPL3_IF_ALL(CMP, __VA_ARGS__)(M_TUPL3_DEFINE_CMP_ORDER(name, __VA_ARGS__),) \
  M_TUPL3_DEFINE_CMP_FIELD(name, __VA_ARGS__)                                 \
  M_TUPL3_IF_ONE(HASH, __VA_ARGS__)(M_TUPL3_DEFINE_HASH(name, __VA_ARGS__),)  \
  M_TUPL3_IF_ONE(EQUAL, __VA_ARGS__)(M_TUPL3_DEFINE_EQUAL(name, __VA_ARGS__),) \
  M_TUPL3_IF_ALL(GET_STR, __VA_ARGS__)(M_TUPL3_DEFINE_GET_STR(name, __VA_ARGS__),) \
  M_TUPL3_IF_ALL(OUT_STR, __VA_ARGS__)(M_TUPL3_DEFINE_OUT_STR(name, __VA_ARGS__),) \
  M_TUPL3_IF_ALL(IN_STR, __VA_ARGS__)(M_TUPL3_DEFINE_IN_STR(name, __VA_ARGS__),) \
  M_TUPL3_IF_ALL(PARSE_STR, __VA_ARGS__)(M_TUPL3_DEFINE_PARSE_STR(name, __VA_ARGS__),) \
  M_TUPL3_IF_ALL(OUT_SERIAL, __VA_ARGS__)(M_TUPL3_DEFINE_OUT_SERIAL(name, __VA_ARGS__),) \
  M_TUPL3_IF_ALL(IN_SERIAL, __VA_ARGS__)(M_TUPL3_DEFINE_IN_SERIAL(name, __VA_ARGS__),) \
  M_TUPL3_IF_ALL(INIT_MOVE, __VA_ARGS__)(M_TUPL3_DEFINE_INIT_MOVE(name, __VA_ARGS__),) \
  M_TUPL3_IF_ALL(MOVE, __VA_ARGS__)(M_TUPL3_DEFINE_MOVE(name, __VA_ARGS__),)  \
  M_TUPL3_IF_ALL(SWAP, __VA_ARGS__)(M_TUPL3_DEFINE_SWAP(name, __VA_ARGS__),)  \
  M_TUPL3_IF_ALL(RESET, __VA_ARGS__)(M_TUPL3_DEFINE_RESET(name, __VA_ARGS__),)

/* Provide order for _cmp_order */
#define M_TUPL3_ORDER_CONVERT(name, x) M_F(name, M_C(M_TUPL3_ORDER_CONVERT_, x))
#define M_TUPL3_ORDER_CONVERT_ASC(x)   M_C3(_,x,_value)
#define M_TUPL3_ORDER_CONVERT_DSC(x)   M_C3(_,x,_value)*-1

/* Get the field name, the type, the oplist or the methods
   based on the tuple (field, type, oplist) */
#define M_TUPL3_GET_FIELD(f,t,o)     f
#define M_TUPL3_GET_TYPE(f,t,o)      t
#define M_TUPL3_GET_OPLIST(f,t,o)    o
#define M_TUPL3_GET_INIT(f,t,o)      M_GET_INIT o
#define M_TUPL3_GET_INIT_SET(f,t,o)  M_GET_INIT_SET o
#define M_TUPL3_GET_INIT_MOVE(f,t,o) M_GET_INIT_MOVE o
#define M_TUPL3_GET_MOVE(f,t,o)      M_GET_MOVE o
#define M_TUPL3_GET_SET(f,t,o)       M_GET_SET o
#define M_TUPL3_GET_CLEAR(f,t,o)     M_GET_CLEAR o
#define M_TUPL3_GET_CMP(f,t,o)       M_GET_CMP o
#define M_TUPL3_GET_HASH(f,t,o)      M_GET_HASH o
#define M_TUPL3_GET_EQUAL(f,t,o)     M_GET_EQUAL o
#define M_TUPL3_GET_STR(f,t,o)       M_GET_GET_STR o
#define M_TUPL3_GET_OUT_STR(f,t,o)   M_GET_OUT_STR o
#define M_TUPL3_GET_IN_STR(f,t,o)    M_GET_IN_STR o
#define M_TUPL3_GET_OUT_SERIAL(f,t,o) M_GET_OUT_SERIAL o
#define M_TUPL3_GET_IN_SERIAL(f,t,o) M_GET_IN_SERIAL o
#define M_TUPL3_GET_PARSE_STR(f,t,o) M_GET_PARSE_STR o
#define M_TUPL3_GET_SWAP(f,t,o)      M_GET_SWAP o
#define M_TUPL3_GET_RESET(f,t,o)     M_GET_RESET o

/* Call the method associated to the given operator for the given parameter
  of the tuple t=(name, type, oplist) */
#define M_TUPL3_CALL_INIT(t, ...)       M_APPLY_API(M_TUPL3_GET_INIT t,  M_TUPL3_GET_OPLIST t, __VA_ARGS__)
#define M_TUPL3_CALL_INIT_SET(t, ...)   M_APPLY_API(M_TUPL3_GET_INIT_SET t,  M_TUPL3_GET_OPLIST t, __VA_ARGS__)
#define M_TUPL3_CALL_INIT_MOVE(t, ...)  M_APPLY_API(M_TUPL3_GET_INIT_MOVE t,  M_TUPL3_GET_OPLIST t, __VA_ARGS__)
#define M_TUPL3_CALL_MOVE(t, ...)       M_APPLY_API(M_TUPL3_GET_MOVE t,  M_TUPL3_GET_OPLIST t, __VA_ARGS__)
#define M_TUPL3_CALL_SET(t, ...)        M_APPLY_API(M_TUPL3_GET_SET t,   M_TUPL3_GET_OPLIST t, __VA_ARGS__)
#define M_TUPL3_CALL_CLEAR(t, ...)      M_APPLY_API(M_TUPL3_GET_CLEAR t, M_TUPL3_GET_OPLIST t, __VA_ARGS__)
#define M_TUPL3_CALL_CMP(t, ...)        M_APPLY_API(M_TUPL3_GET_CMP t,   M_TUPL3_GET_OPLIST t, __VA_ARGS__)
#define M_TUPL3_CALL_HASH(t, ...)       M_APPLY_API(M_TUPL3_GET_HASH t,  M_TUPL3_GET_OPLIST t, __VA_ARGS__)
#define M_TUPL3_CALL_EQUAL(t, ...)      M_APPLY_API(M_TUPL3_GET_EQUAL t, M_TUPL3_GET_OPLIST t, __VA_ARGS__)
#define M_TUPL3_CALL_GET_STR(t, ...)    M_APPLY_API(M_TUPL3_GET_STR t,   M_TUPL3_GET_OPLIST t, __VA_ARGS__)
#define M_TUPL3_CALL_OUT_STR(t, ...)    M_APPLY_API(M_TUPL3_GET_OUT_STR t, M_TUPL3_GET_OPLIST t, __VA_ARGS__)
#define M_TUPL3_CALL_IN_STR(t, ...)     M_APPLY_API(M_TUPL3_GET_IN_STR t, M_TUPL3_GET_OPLIST t, __VA_ARGS__)
#define M_TUPL3_CALL_PARSE_STR(t, ...)  M_APPLY_API(M_TUPL3_GET_PARSE_STR t, M_TUPL3_GET_OPLIST t, __VA_ARGS__)
#define M_TUPL3_CALL_OUT_SERIAL(t, ...) M_APPLY_API(M_TUPL3_GET_OUT_SERIAL t, M_TUPL3_GET_OPLIST t, __VA_ARGS__)
#define M_TUPL3_CALL_IN_SERIAL(t, ...)  M_APPLY_API(M_TUPL3_GET_IN_SERIAL t, M_TUPL3_GET_OPLIST t, __VA_ARGS__)
#define M_TUPL3_CALL_SWAP(t, ...)       M_APPLY_API(M_TUPL3_GET_SWAP t,  M_TUPL3_GET_OPLIST t, __VA_ARGS__)
#define M_TUPL3_CALL_RESET(t, ...)      M_APPLY_API(M_TUPL3_GET_RESET t, M_TUPL3_GET_OPLIST t, __VA_ARGS__)


/* Define the type of a tuple */
#define M_TUPL3_DEFINE_TYPE(name, name_t, ...)                                \
  typedef struct M_F(name, _s) {                                              \
    M_MAP(M_TUPL3_DEFINE_RECUR_TYPE_ELE , __VA_ARGS__)                        \
  } name_t[1];                                                                \
                                                                              \
  typedef struct M_F(name, _s) *M_F(name, _ptr);                              \
  typedef const struct M_F(name, _s) *M_F(name, _srcptr);                     \
  /* Define internal type for oplist */                                       \
  typedef name_t M_F(name, _ct);                                              \
  /* Save constant as the number of arguments (internal) */                   \
  typedef enum {                                                              \
    M_C3(m_tupl3_, name, _num_args) = M_NARGS(__VA_ARGS__)                    \
  } M_C3(m_tupl3_, name, _num_args_ct);                                       \
  /* Save alias for the types of arguments */                                 \
  M_MAP3(M_TUPL3_DEFINE_TYPE_ELE, name, __VA_ARGS__)

#define M_TUPL3_DEFINE_TYPE_ELE(name, num, a)                                 \
  typedef M_TUPL3_GET_TYPE a M_C4(name, _type_, num, _ct);

#define M_TUPL3_DEFINE_RECUR_TYPE_ELE(a)                                      \
  M_TUPL3_GET_TYPE a M_TUPL3_GET_FIELD a ;

/* Define the basic enumerate, identifying a parameter */
#define M_TUPL3_DEFINE_ENUM(name, ...)                                        \
  typedef enum {                                                              \
    M_F(name, _first_one_val),                                                \
    M_MAP2_C(M_TUPL3_DEFINE_ENUM_ELE , name, __VA_ARGS__)                     \
  } M_F(name,_field_e);

#define M_TUPL3_DEFINE_ENUM_ELE(name, a)                                      \
  M_C4(name, _, M_TUPL3_GET_FIELD a, _value)

/* Control that all given oplists of all parameters are really oplists */
#define M_TUPL3_CONTROL_ALL_OPLIST(name, ...)                                 \
  M_MAP2(M_TUPL3_CONTROL_OPLIST, name, __VA_ARGS__)

#define M_TUPL3_CONTROL_OPLIST(name, a)                                       \
  M_CHECK_COMPATIBLE_OPLIST(name, M_TUPL3_GET_FIELD a,                        \
                            M_TUPL3_GET_TYPE a, M_TUPL3_GET_OPLIST a)

/* Define the INIT method calling the INIT method for all params */
#define M_TUPL3_DEFINE_INIT(name, ...)                                        \
  M_P(void, name, _init, M_F(name,_ct) my) {                                  \
    M_UNUSED_CONTEXT();                                                       \
    M_MAP(M_TUPL3_DEFINE_INIT_FUNC , __VA_ARGS__) {}                          \
  }

#define M_TUPL3_DEFINE_INIT_FUNC(a)                                           \
  M_CHAIN_OBJ(M_TUPL3_GET_FIELD a, M_TUPL3_GET_OPLIST a, my -> M_TUPL3_GET_FIELD a)

/* Define the INIT_SET method calling the INIT_SET method for all params */
#define M_TUPL3_DEFINE_INIT_SET(name, ...)                                    \
  M_P(void, name, _init_set, M_F(name,_ct) my , M_F(name,_ct) const org) {    \
    M_TUPL3_CONTRACT(org);                                                    \
    M_UNUSED_CONTEXT();                                                       \
    M_MAP(M_TUPL3_DEFINE_INIT_SET_FUNC , __VA_ARGS__) {}                      \
  }
#define M_TUPL3_DEFINE_INIT_SET_FUNC(a)                                       \
  M_CHAIN_OBJ(M_TUPL3_GET_FIELD a, M_TUPL3_GET_OPLIST a,                      \
              my -> M_TUPL3_GET_FIELD a , org -> M_TUPL3_GET_FIELD a )

/* Define the INIT_WITH method calling the INIT_SET method for all params. */
#define M_TUPL3_DEFINE_INIT_SET2(name, ...)                                   \
  M_P(void, name, _init_emplace, M_F(name,_ct) my M_MAP(M_TUPL3_DEFINE_SET2_PROTO, __VA_ARGS__) ) { \
    M_UNUSED_CONTEXT();                                                       \
    M_MAP(M_TUPL3_DEFINE_INIT_SET2_FUNC , __VA_ARGS__) {}                     \
  }

#define M_TUPL3_DEFINE_INIT_SET2_FUNC(a)                                      \
  M_CHAIN_OBJ(M_TUPL3_GET_FIELD a, M_TUPL3_GET_OPLIST a,                      \
              my -> M_TUPL3_GET_FIELD a , M_TUPL3_GET_FIELD a )


/* Define the SET method calling the SET method for all params. */
#define M_TUPL3_DEFINE_SET(name, ...)                                         \
  M_P(void, name, _set, M_F(name,_ct) my, M_F(name,_ct) const org) {          \
    M_TUPL3_CONTRACT(my);                                                     \
    M_TUPL3_CONTRACT(org);                                                    \
    M_UNUSED_CONTEXT();                                                       \
    M_MAP(M_TUPL3_DEFINE_SET_FUNC , __VA_ARGS__)                              \
  }

#define M_TUPL3_DEFINE_SET_FUNC(a)                                            \
  M_TUPL3_CALL_SET(a, my -> M_TUPL3_GET_FIELD a , org -> M_TUPL3_GET_FIELD a );


/* Define the SET_WITH method calling the SET method for all params. */
#define M_TUPL3_DEFINE_SET2(name, ...)                                        \
  M_P(void, name, _emplace, M_F(name,_ct) my M_MAP(M_TUPL3_DEFINE_SET2_PROTO, __VA_ARGS__) ) { \
    M_TUPL3_CONTRACT(my);                                                     \
    M_UNUSED_CONTEXT();                                                       \
    M_MAP(M_TUPL3_DEFINE_SET2_FUNC , __VA_ARGS__)                             \
  }
#define M_TUPL3_DEFINE_SET2_PROTO(a)                                          \
  , M_TUPL3_GET_TYPE a const M_TUPL3_GET_FIELD a

#define M_TUPL3_DEFINE_SET2_FUNC(a)                                           \
  M_TUPL3_CALL_SET(a, my -> M_TUPL3_GET_FIELD a , M_TUPL3_GET_FIELD a );


/* Define the CLEAR method calling the CLEAR method for all params. */
#define M_TUPL3_DEFINE_CLEAR(name, ...)                                       \
  M_P(void, name, _clear, M_F(name,_ct) my) {                                 \
    M_TUPL3_CONTRACT(my);                                                     \
    M_UNUSED_CONTEXT();                                                       \
    M_MAP(M_TUPL3_DEFINE_CLEAR_FUNC , __VA_ARGS__)                            \
  }

#define M_TUPL3_DEFINE_CLEAR_FUNC(a)                                          \
  M_TUPL3_CALL_CLEAR(a, my -> M_TUPL3_GET_FIELD a );


/* Define the GET_AT_field & CGET_AT methods for all params. */
#define M_TUPL3_DEFINE_GETTER_FIELD(name, ...)                                \
  M_MAP3(M_TUPL3_DEFINE_GETTER_FIELD_PROTO, name, __VA_ARGS__)

#define M_TUPL3_DEFINE_GETTER_FIELD_PROTO(name, num, a)                       \
  M_INLINE M_TUPL3_GET_TYPE a * M_C3(name, _get_at_, M_TUPL3_GET_FIELD a)     \
       (M_F(name,_ct) my) {                                                   \
    M_TUPL3_CONTRACT(my);                                                     \
    return &(my->M_TUPL3_GET_FIELD a);                                        \
  }                                                                           \
  M_INLINE M_TUPL3_GET_TYPE a const * M_C3(name, _cget_at_, M_TUPL3_GET_FIELD a) \
    (M_F(name,_ct) const my) {                                                \
    M_TUPL3_CONTRACT(my);                                                     \
    return &(my->M_TUPL3_GET_FIELD a);                                        \
  }                                                                           \
  /* Same but uses numerical index for accessing the field (internal) */      \
  M_INLINE M_TUPL3_GET_TYPE a * M_C4(m_tupl3_, name, _get_at_, num)           \
       (M_F(name,_ct) my) {                                                   \
    return &(my->M_TUPL3_GET_FIELD a);                                        \
  }                                                                           \


/* Define the SET_field methods for all params. */
#define M_TUPL3_DEFINE_SETTER_FIELD(name, ...)                                \
  M_MAP2(M_TUPL3_DEFINE_SETTER_FIELD_PROTO, name, __VA_ARGS__)

#define M_TUPL3_DEFINE_SETTER_FIELD_PROTO(name, a)                            \
  M_INLINE void M_C3(name, _set_, M_TUPL3_GET_FIELD a)                        \
       (M_P_EXPAND M_F(name,_ct) my, M_TUPL3_GET_TYPE a const M_TUPL3_GET_FIELD a) { \
    M_TUPL3_CONTRACT(my);                                                     \
    M_UNUSED_CONTEXT();                                                       \
    M_TUPL3_CALL_SET(a, my ->M_TUPL3_GET_FIELD a, M_TUPL3_GET_FIELD a);       \
  }


/* Define the EMPLACE_field methods for all params. */
#define M_TUPL3_DEFINE_EMPLACE_FIELD(name, ...)                               \
  M_REDUCE3(M_TUPL3_DEFINE_EMPLACE_FIELD_PROTO, M_TUPL3_DEFINE_EMPLACE_G, name, __VA_ARGS__)

#define M_TUPL3_DEFINE_EMPLACE_G(a, b) a b

#define M_TUPL3_DEFINE_EMPLACE_FIELD_PROTO(name, id, a)                       \
  M_EMPLACE_QUEUE_DEF(M_TUPL3_GET_FIELD a, M_F(name, _ct), M_C3(name, _emplace_, M_TUPL3_GET_FIELD a), M_TUPL3_GET_OPLIST a, M_TUPL3_EMPLACE_DEF)

#define M_TUPL3_EMPLACE_DEF(name, name_t, function_name, oplist, init_func, exp_emplace_type) \
  M_INLINE void                                                               \
  function_name(M_P_EXPAND name_t v                                           \
                M_EMPLACE_LIST_TYPE_VAR(a, exp_emplace_type) )                \
  {                                                                           \
    M_UNUSED_CONTEXT();                                                       \
    M_CALL_CLEAR(oplist, v->name);                                            \
    M_EMPLACE_CALL_FUNC(a, init_func, oplist, v->name, exp_emplace_type);     \
  }


/* Define the CMP method by calling CMP methods for all params. */
#define M_TUPL3_DEFINE_CMP(name, ...)                                         \
  M_INLINE int M_F(name, _cmp)(M_F(name,_ct) const e1 ,                       \
                                    M_F(name,_ct) const e2) {                 \
    int i;                                                                    \
    M_TUPL3_CONTRACT(e1);                                                     \
    M_TUPL3_CONTRACT(e2);                                                     \
    M_MAP(M_TUPL3_DEFINE_CMP_FUNC_P0, __VA_ARGS__)                            \
    return 0;                                                                 \
  }

#define M_TUPL3_DEFINE_CMP_FUNC_P0(a)                                         \
  M_IF(M_TUPL3_TEST_METHOD_P(CMP, a))(M_TUPL3_DEFINE_CMP_FUNC_P1, M_EAT)(a)
#define M_TUPL3_DEFINE_CMP_FUNC_P1(a)                                         \
  i = M_TUPL3_CALL_CMP(a, e1 -> M_TUPL3_GET_FIELD a , e2 -> M_TUPL3_GET_FIELD a ); \
  if (i != 0) return i;

/* Define the CMP_ORDER method by calling CMP methods for all params
   In the right order 
   FIXME: _cmp_order is not supported by algorithm yet.
   FIXME: All oplists shall define the CMP operator or at least one?
*/
#define M_TUPL3_DEFINE_CMP_ORDER(name, ...)                                   \
  M_INLINE int M_F(name, _cmp_order)(M_F(name,_ct) const e1 ,                 \
                                          M_F(name,_ct) const e2,             \
                                          const int order[]) {                \
    int i, r;                                                                 \
    M_TUPL3_CONTRACT(e1);                                                     \
    M_TUPL3_CONTRACT(e2);                                                     \
    while (true) {                                                            \
      i=*order++;                                                             \
      switch (i) {                                                            \
        case 0: return 0;                                                     \
        M_MAP2(M_TUPL3_DEFINE_CMP_ORDER_FUNC , name, __VA_ARGS__)             \
      default: M_ASSUME(0);                                                   \
      }                                                                       \
    }                                                                         \
  }

#define M_TUPL3_DEFINE_CMP_ORDER_FUNC(name, a)                                \
  case M_C4(name, _, M_TUPL3_GET_FIELD a, _value):                            \
  case -M_C4(name, _, M_TUPL3_GET_FIELD a, _value):                           \
       r = M_TUPL3_CALL_CMP(a, e1 -> M_TUPL3_GET_FIELD a , e2 -> M_TUPL3_GET_FIELD a ); \
       if (r != 0) return i < 0 ? -r : r;                                     \
       break;


/* Define a CMP_field method for all given params that export a CMP method */
#define M_TUPL3_DEFINE_CMP_FIELD(name, ...)                                   \
  M_MAP2(M_TUPL3_MAP_CMP_FIELD, name, __VA_ARGS__)

#define M_TUPL3_MAP_CMP_FIELD(name, a)                                        \
  M_IF_METHOD(CMP, M_TUPL3_GET_OPLIST a)(                                     \
    M_TUPL3_DEFINE_CMP_FIELD_FUNC(name, M_TUPL3_GET_FIELD a, M_TUPL3_GET_CMP a, M_TUPL3_GET_OPLIST a), \
  )

#define M_TUPL3_DEFINE_CMP_FIELD_FUNC(name, field, func_cmp, oplist)          \
  M_INLINE int M_C3(name, _cmp_, field)(M_F(name,_ct) const e1 ,              \
                                             M_F(name,_ct) const e2) {        \
    M_TUPL3_CONTRACT(e1);                                                     \
    M_TUPL3_CONTRACT(e2);                                                     \
    return M_APPLY_API(func_cmp, oplist, e1 -> field , e2 -> field );         \
  }


/* Define a EQUAL method by calling the EQUAL methods  for all params */
#define M_TUPL3_DEFINE_EQUAL(name, ...)                                       \
  M_INLINE bool M_F(name, _equal_p)(M_F(name,_ct) const e1 ,                  \
                                         M_F(name,_ct) const e2) {            \
    bool b;                                                                   \
    M_TUPL3_CONTRACT(e1);                                                     \
    M_TUPL3_CONTRACT(e2);                                                     \
    M_MAP(M_TUPL3_DEFINE_EQUAL_FUNC_P0, __VA_ARGS__)                          \
    return true;                                                              \
  }

#define M_TUPL3_DEFINE_EQUAL_FUNC_P0(a)                                       \
  M_IF(M_TUPL3_TEST_METHOD_P(EQUAL, a))(M_TUPL3_DEFINE_EQUAL_FUNC_P1, M_EAT)(a)
#define M_TUPL3_DEFINE_EQUAL_FUNC_P1(a)                                       \
  b = M_TUPL3_CALL_EQUAL(a,  e1 -> M_TUPL3_GET_FIELD a , e2 -> M_TUPL3_GET_FIELD a ); \
  if (!b) return false;


/* Define a HASH method by calling the HASH methods  for all params */
#define M_TUPL3_DEFINE_HASH(name, ...)                                        \
  M_INLINE size_t M_F(name, _hash)(M_F(name,_ct) const e1) {                  \
    M_TUPL3_CONTRACT(e1);                                                     \
    M_HASH_DECL(hash);                                                        \
    M_MAP(M_TUPL3_DEFINE_HASH_FUNC_P0, __VA_ARGS__)                           \
    return M_HASH_FINAL (hash);                                               \
  }

#define M_TUPL3_DEFINE_HASH_FUNC_P0(a)                                        \
  M_IF(M_TUPL3_TEST_METHOD_P(HASH, a))(M_TUPL3_DEFINE_HASH_FUNC_P1, M_EAT)(a)
#define M_TUPL3_DEFINE_HASH_FUNC_P1(a)                                        \
  M_HASH_UP(hash, M_TUPL3_CALL_HASH(a, e1 -> M_TUPL3_GET_FIELD a) );


/* Define a GET_STR method by calling the GET_STR methods for all params */
#define M_TUPL3_DEFINE_GET_STR(name, ...)                                     \
  M_P(void, name, _get_str, m_string_t str, M_F(name,_ct) const el, bool append) \
  {                                                                           \
    bool comma = false;                                                       \
    M_TUPL3_CONTRACT(el);                                                     \
    M_ASSERT (str != NULL);                                                   \
    (append ? m_string_cat_cstr : m_string_set_cstr) M_R(str, "(");           \
    M_MAP(M_TUPL3_DEFINE_GET_STR_FUNC , __VA_ARGS__)                          \
    m_string_push_back M_R(str, ')');                                         \
  }

#define M_TUPL3_DEFINE_GET_STR_FUNC(a)                                        \
  if (comma) m_string_push_back M_R(str, ',');                                \
  comma = true;                                                               \
  M_TUPL3_CALL_GET_STR(a, str, el -> M_TUPL3_GET_FIELD a, true);              \


/* Define a OUT_STR method by calling the OUT_STR methods for all params */
#define M_TUPL3_DEFINE_OUT_STR(name, ...)                                     \
  M_INLINE void M_F(name, _out_str)(FILE *f, M_F(name,_ct) const el)          \
  {                                                                           \
    bool comma = false;                                                       \
    M_TUPL3_CONTRACT(el);                                                     \
    M_ASSERT (f != NULL);                                                     \
    fputc('(', f);                                                            \
    M_MAP(M_TUPL3_DEFINE_OUT_STR_FUNC , __VA_ARGS__)                          \
    fputc (')', f);                                                           \
  }

#define M_TUPL3_DEFINE_OUT_STR_FUNC(a)                                        \
  if (comma) fputc (',', f);                                                  \
  comma = true;                                                               \
  M_TUPL3_CALL_OUT_STR(a, f, el -> M_TUPL3_GET_FIELD a);                      \


/* Define a IN_STR method by calling the IN_STR methods for all params */
#define M_TUPL3_DEFINE_IN_STR(name, ...)                                      \
  M_P(bool, name, _in_str, M_F(name,_ct) el, FILE *f)                         \
  {                                                                           \
    bool comma = false;                                                       \
    M_TUPL3_CONTRACT(el);                                                     \
    M_UNUSED_CONTEXT();                                                       \
    M_ASSERT (f != NULL);                                                     \
    int c = fgetc(f);                                                         \
    if (c != '(') return false;                                               \
    M_MAP(M_TUPL3_DEFINE_IN_STR_FUNC , __VA_ARGS__)                           \
    c = fgetc(f);                                                             \
    return (c == ')');                                                        \
  }

#define M_TUPL3_DEFINE_IN_STR_FUNC(a)                                         \
  if (comma) {                                                                \
    c = fgetc (f);                                                            \
    if (c != ',' || c == EOF) return false;                                   \
  }                                                                           \
  comma = true;                                                               \
  if (M_TUPL3_CALL_IN_STR(a, el -> M_TUPL3_GET_FIELD a, f) == false)          \
    return false ;                                                            \


/* Define a PARSE_STR method by calling the PARSE_STR methods for all params */
#define M_TUPL3_DEFINE_PARSE_STR(name, ...)                                   \
  M_P(bool, name, _parse_str, M_F(name,_ct) el, const char str[], const char **endptr) \
  {                                                                           \
    M_TUPL3_CONTRACT(el);                                                     \
    M_ASSERT (str != NULL);                                                   \
    M_UNUSED_CONTEXT();                                                       \
    bool success = false;                                                     \
    bool comma = false;                                                       \
    int c = *str++;                                                           \
    if (c != '(') goto exit;                                                  \
    M_MAP(M_TUPL3_DEFINE_PARSE_STR_FUNC , __VA_ARGS__)                        \
    c = *str++;                                                               \
    success = (c == ')');                                                     \
  exit:                                                                       \
    if (endptr) *endptr = str;                                                \
    return success;                                                           \
  }

#define M_TUPL3_DEFINE_PARSE_STR_FUNC(a)                                      \
  if (comma) {                                                                \
    c = *str++;                                                               \
    if (c != ',' || c == 0) goto exit;                                        \
  }                                                                           \
  comma = true;                                                               \
  if (M_TUPL3_CALL_PARSE_STR(a, el -> M_TUPL3_GET_FIELD a, str, &str) == false) \
    goto exit ;                                                               \


/* Return the parameter name as a C string */
#define M_TUPL3_STRINGIFY_NAME(a)                                             \
  M_AS_STR(M_TUPL3_GET_FIELD a)


/* Define a OUT_SERIAL method by calling the OUT_SERIAL methods for all params */
#define M_TUPL3_DEFINE_OUT_SERIAL(name, ...)                                  \
  M_P(m_serial_return_code_t, name, _out_serial, m_serial_write_t f, M_F(name,_ct) const el) \
  {                                                                           \
    M_TUPL3_CONTRACT(el);                                                     \
    M_ASSERT (f != NULL && f->m_interface != NULL);                           \
    const int field_max = M_NARGS(__VA_ARGS__);                               \
    /* Define a constant static table of all fields names */                  \
    static const char *const field_name[] =                                   \
      { M_REDUCE(M_TUPL3_STRINGIFY_NAME, M_ID, __VA_ARGS__) };                \
    int index = 0;                                                            \
    m_serial_local_t local;                                                   \
    m_serial_return_code_t ret;                                               \
    ret = f->m_interface->write_tuple_start M_R(local, f);                    \
    M_MAP(M_TUPL3_DEFINE_OUT_SERIAL_FUNC , __VA_ARGS__)                       \
    M_ASSERT( index == field_max);                                            \
    ret |= f->m_interface->write_tuple_end M_R(local, f);                     \
    return ret & M_SERIAL_FAIL;                                               \
  }

#define M_TUPL3_DEFINE_OUT_SERIAL_FUNC(a)                                     \
  f->m_interface->write_tuple_id M_R(local, f, field_name, field_max, index); \
  M_TUPL3_CALL_OUT_SERIAL(a, f, el -> M_TUPL3_GET_FIELD a);                   \
  index++;                                                                    \


/* Define a IN_SERIAL method by calling the IN_SERIAL methods for all params */
#define M_TUPL3_DEFINE_IN_SERIAL(name, ...)                                   \
  M_P(m_serial_return_code_t, name, _in_serial, M_F(name,_ct) el, m_serial_read_t f) \
  {                                                                           \
    M_TUPL3_CONTRACT(el);                                                     \
    M_UNUSED_CONTEXT();                                                       \
    M_ASSERT (f != NULL && f->m_interface != NULL);                           \
    int index = -1;                                                           \
    const int field_max = M_NARGS(__VA_ARGS__);                               \
    static const char *const field_name[] =                                   \
      { M_REDUCE(M_TUPL3_STRINGIFY_NAME, M_ID, __VA_ARGS__) };                \
    m_serial_local_t local;                                                   \
    m_serial_return_code_t ret;                                               \
    ret = f->m_interface->read_tuple_start(local, f);                         \
    while (ret == M_SERIAL_OK_CONTINUE) {                                     \
      ret = f->m_interface->read_tuple_id(local, f, field_name, field_max, &index); \
      if (ret == M_SERIAL_OK_CONTINUE) {                                      \
        M_ASSERT (index >= 0 && index < field_max);                           \
        switch (1+index) {                                                    \
          M_MAP2(M_TUPL3_DEFINE_IN_SERIAL_FUNC , name, __VA_ARGS__)           \
        default: M_ASSUME(0);                                                 \
        }                                                                     \
        ret = (ret == M_SERIAL_OK_DONE) ? M_SERIAL_OK_CONTINUE : M_SERIAL_FAIL; \
      }                                                                       \
    }                                                                         \
    return ret;                                                               \
  }

#define M_TUPL3_DEFINE_IN_SERIAL_FUNC(name, a)                                \
  case M_C4(name, _, M_TUPL3_GET_FIELD a, _value):                            \
  ret = M_TUPL3_CALL_IN_SERIAL(a, el -> M_TUPL3_GET_FIELD a, f);              \
  break;                                                                      \


/* Define a INIT_MOVE method by calling the INIT_MOVE methods for all params
   INIT_MOVE cannot fail and cannot throw any exception */
#define M_TUPL3_DEFINE_INIT_MOVE(name, ...)                                   \
  M_INLINE void M_F(name, _init_move)(M_F(name,_ct) el, M_F(name,_ct) org) {  \
    M_TUPL3_CONTRACT(el);                                                     \
    M_MAP(M_TUPL3_DEFINE_INIT_MOVE_FUNC , __VA_ARGS__)                        \
  }

#define M_TUPL3_DEFINE_INIT_MOVE_FUNC(a)                                      \
  M_TUPL3_CALL_INIT_MOVE(a, el -> M_TUPL3_GET_FIELD a, org -> M_TUPL3_GET_FIELD a);


/* Define a MOVE method by calling the MOVE methods for all params */
#define M_TUPL3_DEFINE_MOVE(name, ...)                                        \
 M_P(void, name, _move, M_F(name,_ct) el, M_F(name,_ct) org) {                \
    M_TUPL3_CONTRACT(el);                                                     \
    M_UNUSED_CONTEXT();                                                       \
    M_MAP(M_TUPL3_DEFINE_MOVE_FUNC , __VA_ARGS__)                             \
 }

#define M_TUPL3_DEFINE_MOVE_FUNC(a)                                           \
  M_TUPL3_CALL_MOVE(a, el -> M_TUPL3_GET_FIELD a, org -> M_TUPL3_GET_FIELD a);


/* Define a SWAP method by calling the SWAP methods for all params */
#define M_TUPL3_DEFINE_SWAP(name, ...)                                        \
  M_INLINE void M_F(name, _swap)(M_F(name,_ct) el1, M_F(name,_ct) el2) {      \
    M_TUPL3_CONTRACT(el1);                                                    \
    M_TUPL3_CONTRACT(el2);                                                    \
    M_MAP(M_TUPL3_DEFINE_SWAP_FUNC , __VA_ARGS__)                             \
  }

#define M_TUPL3_DEFINE_SWAP_FUNC(a)                                           \
  M_TUPL3_CALL_SWAP(a, el1 -> M_TUPL3_GET_FIELD a, el2 -> M_TUPL3_GET_FIELD a);


/* Define a RESET method by calling the RESET methods for all params */
#define M_TUPL3_DEFINE_RESET(name, ...)                                       \
  M_P(void, name, _reset, M_F(name,_ct) el1) {                                \
    M_TUPL3_CONTRACT(el1);                                                    \
    M_UNUSED_CONTEXT();                                                       \
    M_MAP(M_TUPL3_DEFINE_RESET_FUNC , __VA_ARGS__)                            \
  }                                                                           \

#define M_TUPL3_DEFINE_RESET_FUNC(a)                                          \
  M_TUPL3_CALL_RESET(a, el1 -> M_TUPL3_GET_FIELD a);


/********************************** INTERNAL *********************************/

/* INIT_WITH macro enabling recursive INIT_WITH initialization
    tuple = { int, m_string_t, array<m_string_t> }
  USAGE:
    M_LET( (x, 2, ("John"), ( ("Bear"), ("Rabbit") )), tuple_t)

  "If you think it's simple, you're deluding yourself."

  Several pass are done:
  1) If the number of arguments doesn't match the number of oplists of the 
  tuple oplist, it is assumed something is wrong. It uses the _init_emplace
  function to provide proper warning in such case.
  2) Otherwise, it checks that the number of arguments matches the number
  of arguments of the tuple definition.
  3) Mix all arguments with their associated oplists to have pair (arg, oplist),
  4) Map the following macro for each computed pair :
  4.a) If INIT_WITH macro is not defined for this pair, it uses INIT_SET
  4.b) If the argument is encapsulated with parenthesis, it uses INIT_WITH
  4.c) If the oplist property LET_AS_INIT_WITH is defined, it uses INIT_WITH
  4.d) Otherwise it uses INIT_SET.
*/
#define M_TUPL3_INIT_WITH(oplist, dest, ...)                                  \
  M_TUPL3_INIT_WITH_P1(M_GET_NAME oplist, M_GET_OPLIST oplist, dest, __VA_ARGS__)
#define M_TUPL3_INIT_WITH_P1(name, oplist_arglist, dest, ...)                 \
  M_IF(M_NOTEQUAL( M_NARGS oplist_arglist, M_NARGS (__VA_ARGS__)))            \
    (M_TUPL3_INIT_WITH_P1_FUNC, M_TUPL3_INIT_WITH_P1_MACRO)(name, oplist_arglist, dest, __VA_ARGS__)
#define M_TUPL3_INIT_WITH_P1_FUNC(name, oplist_arglist, dest, ...)            \
  M_F(name, _init_emplace)(dest, __VA_ARGS__)
#define M_TUPL3_INIT_WITH_P1_MACRO(name, oplist_arglist, dest, ...)           \
  ( M_STATIC_ASSERT( M_NARGS oplist_arglist == M_C3(m_tupl3_, name, _num_args), M_LIB_DIMENSION_ERROR, "The number of oplists given to TUPLE_OPLIST don't match the number of oplists used to create the tuple." ), \
    M_STATIC_ASSERT( M_NARGS(__VA_ARGS__) == M_C3(m_tupl3_, name, _num_args), M_LIB_DIMENSION_ERROR, "Missing / Too many arguments for tuple"), \
    M_MAP3(M_TUPL3_INIT_WITH_P2, (name, dest), M_OPFLAT M_MERGE_ARGLIST( oplist_arglist, (__VA_ARGS__) ) ) \
    (void) 0)
#define M_TUPL3_INIT_WITH_P2(name_dest, num, pair)                            \
    M_TUPL3_INIT_WITH_P3( M_PAIR_1 name_dest, M_PAIR_2 name_dest, num, M_PAIR_1 pair, M_PAIR_2 pair )
#define M_TUPL3_INIT_WITH_P3(name, dest, num, oplist, param)                  \
  M_IF(M_TEST_METHOD_P(INIT_WITH, oplist))(M_TUPL3_INIT_WITH_P4, M_TUPL3_INIT_WITH_SET)(name, dest, num, oplist, param)
#define M_TUPL3_INIT_WITH_SET(name, dest, num, oplist, param)                 \
  M_CALL_INIT_SET (oplist, *M_C4(m_tupl3_, name, _get_at_, num)(dest), param) ,
#define M_TUPL3_INIT_WITH_P4(name, dest, num, oplist, param)                  \
  M_IF(M_PARENTHESIS_P( param))(M_TUPL3_INIT_WITH_P5, M_TUPL3_INIT_WITH_P6)(name, dest, num, oplist, param)
#define M_TUPL3_INIT_WITH_P5(name, dest, num, oplist, param)                  \
  M_CALL_INIT_WITH(oplist, *M_C4(m_tupl3_, name, _get_at_, num)(dest), M_REMOVE_PARENTHESIS (param) ) ,
#define M_TUPL3_INIT_WITH_P6(name, dest, num, oplist, param)                  \
  M_IF(M_GET_PROPERTY(oplist, LET_AS_INIT_WITH))(M_TUPL3_INIT_WITH_P5, M_TUPL3_INIT_WITH_SET)(name, dest, num, oplist, param)

/* Macros for testing for the presence of a method in the parameter (name, type, oplist) */
#define M_TUPL3_TEST_METHOD_P(method, trio)                                   \
  M_APPLY(M_TUPL3_TEST_METHOD2_P, method, M_OPFLAT trio)

#define M_TUPL3_TEST_METHOD2_P(method, f, t, op)                              \
  M_TEST_METHOD_P(method, op)


/********************************** INTERNAL *********************************/

/* Macros for testing for the presence of a method in all the params */
#define M_TUPL3_IF_ALL(method, ...)                                           \
  M_IF(M_REDUCE2(M_TUPL3_TEST_METHOD_P, M_AND, method, __VA_ARGS__))

/* Macros for testing for the presence of a method in at least one params */
#define M_TUPL3_IF_ONE(method, ...)                                           \
  M_IF(M_REDUCE2(M_TUPL3_TEST_METHOD_P, M_OR, method, __VA_ARGS__))

// deferred evaluation
#define M_TUPL3_OPLIST_P1(arg) M_TUPL3_OPLIST_P2 arg

/* Validate the oplist before going further */
#define M_TUPL3_OPLIST_P2(name, ...)                                          \
  M_IF(M_REDUCE(M_OPLIST_P, M_AND, __VA_ARGS__))(M_TUPL3_OPLIST_P3, M_TUPL3_OPLIST_FAILURE)(name, __VA_ARGS__)

/* Prepare a clean compilation failure */
#define M_TUPL3_OPLIST_FAILURE(name, ...)                                     \
  ((M_LIB_ERROR(ONE_ARGUMENT_OF_M_TUPL3_OPLIST_IS_NOT_AN_OPLIST, name, __VA_ARGS__)))

/* Define the TUPLE oplist */
#ifndef M_USE_CONTEXT
#define M_TUPL3_OPLIST_P3(name, ...)                                          \
  (M_IF_METHOD_ALL(INIT, __VA_ARGS__)(INIT(M_F(name,_init)),),                \
   INIT_SET(M_F(name, _init_set)),                                            \
   INIT_WITH(API_1(M_TUPL3_INIT_WITH)),                                       \
   SET(M_F(name,_set)),                                                       \
   CLEAR(M_F(name, _clear)),                                                  \
   NAME(name),                                                                \
   TYPE(M_F(name,_ct)), GENTYPE(struct M_F(name,_s)*),                        \
   OPLIST( (__VA_ARGS__) ),                                                   \
   M_IF_METHOD_ALL(CMP, __VA_ARGS__)(CMP(M_F(name, _cmp)),),                  \
   M_IF_METHOD_ALL(HASH, __VA_ARGS__)(HASH(M_F(name, _hash)),),               \
   M_IF_METHOD_ALL(EQUAL, __VA_ARGS__)(EQUAL(M_F(name, _equal_p)),),          \
   M_IF_METHOD_ALL(GET_STR, __VA_ARGS__)(GET_STR(M_F(name, _get_str)),),      \
   M_IF_METHOD_ALL(PARSE_STR, __VA_ARGS__)(PARSE_STR(M_F(name, _parse_str)),), \
   M_IF_METHOD_ALL(IN_STR, __VA_ARGS__)(IN_STR(M_F(name, _in_str)),),         \
   M_IF_METHOD_ALL(OUT_STR, __VA_ARGS__)(OUT_STR(M_F(name, _out_str)),),      \
   M_IF_METHOD_ALL(IN_SERIAL, __VA_ARGS__)(IN_SERIAL(M_F(name, _in_serial)),), \
   M_IF_METHOD_ALL(OUT_SERIAL, __VA_ARGS__)(OUT_SERIAL(M_F(name, _out_serial)),), \
   M_IF_METHOD_ALL(INIT_MOVE, __VA_ARGS__)(INIT_MOVE(M_F(name, _init_move)),), \
   M_IF_METHOD_ALL(MOVE, __VA_ARGS__)(MOVE(M_F(name, _move)),),               \
   M_IF_METHOD_ALL(SWAP, __VA_ARGS__)(SWAP(M_F(name, _swap)),),               \
   M_IF_METHOD_ALL(RESET, __VA_ARGS__)(RESET(M_F(name, _reset)),),            \
   EMPLACE_TYPE( ( M_REDUCE2(M_TUPL3_OPLIST_SUBTYPE, M_ID, name, M_SEQ(1, M_NARGS(__VA_ARGS__))) ) ) \
   )
#else
#define M_TUPL3_OPLIST_P3(name, ...)                                          \
  (M_IF_METHOD_ALL(INIT, __VA_ARGS__)(API_0P(INIT(M_F(name,_init))),),        \
   INIT_SET(API_0P(M_F(name, _init_set))),                                    \
   INIT_WITH(API_1(M_TUPL3_INIT_WITH)),                                       \
   SET(API_0P(M_F(name,_set))),                                               \
   CLEAR(API_0P(M_F(name, _clear))),                                          \
   NAME(name), TYPE(M_F(name,_ct)), GENTYPE(struct M_F(name,_s)*),            \
   OPLIST( (__VA_ARGS__) ),                                                   \
   M_IF_METHOD_ALL(CMP, __VA_ARGS__)(CMP(M_F(name, _cmp)),),                  \
   M_IF_METHOD_ALL(HASH, __VA_ARGS__)(HASH(M_F(name, _hash)),),               \
   M_IF_METHOD_ALL(EQUAL, __VA_ARGS__)(EQUAL(M_F(name, _equal_p)),),          \
   M_IF_METHOD_ALL(GET_STR, __VA_ARGS__)(GET_STR(API_0P(M_F(name, _get_str))),), \
   M_IF_METHOD_ALL(PARSE_STR, __VA_ARGS__)(PARSE_STR(API_0P(M_F(name, _parse_str))),), \
   M_IF_METHOD_ALL(IN_STR, __VA_ARGS__)(IN_STR(API_0P(M_F(name, _in_str))),), \
   M_IF_METHOD_ALL(OUT_STR, __VA_ARGS__)(OUT_STR(M_F(name, _out_str)),),      \
   M_IF_METHOD_ALL(IN_SERIAL, __VA_ARGS__)(IN_SERIAL(API_0P(M_F(name, _in_serial))),), \
   M_IF_METHOD_ALL(OUT_SERIAL, __VA_ARGS__)(OUT_SERIAL(API_0P(M_F(name, _out_serial))),), \
   M_IF_METHOD_ALL(INIT_MOVE, __VA_ARGS__)(INIT_MOVE(M_F(name, _init_move)),), \
   M_IF_METHOD_ALL(MOVE, __VA_ARGS__)(MOVE(API_0P(M_F(name, _move))),),       \
   M_IF_METHOD_ALL(SWAP, __VA_ARGS__)(SWAP(M_F(name, _swap)),),               \
   M_IF_METHOD_ALL(RESET, __VA_ARGS__)(RESET(API_0P(M_F(name, _reset))),),    \
   EMPLACE_TYPE( ( M_REDUCE2(M_TUPL3_OPLIST_SUBTYPE, M_ID, name, M_SEQ(1, M_NARGS(__VA_ARGS__))) ) ) \
   )
#endif

/* Support for EMPLACE_TYPE in OPLIST. It refers the created internal type alias */
#define M_TUPL3_OPLIST_SUBTYPE(name, num)                                     \
    M_C4(name, _type_, num, _ct)

/********************************** INTERNAL *********************************/

#if M_USE_SMALL_NAME
#define TUPLE_DEF2 M_TUPLE_DEF2
#define TUPLE_DEF2_AS M_TUPLE_DEF2_AS
#define TUPLE_OPLIST M_TUPLE_OPLIST
#define TUPLE_ORDER M_TUPLE_ORDER
#endif

#endif
