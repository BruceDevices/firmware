/*
 * M*LIB - VARIANT module
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
#ifndef MSTARLIB_VARIANT_H
#define MSTARLIB_VARIANT_H

#include "m-core.h"

/* Define the variant type and functions.
   USAGE:
     VARIANT_DEF2(name, [(field1, type1, oplist1), (field2, type2, oplist2), ...] ) */
#define M_VARIANT_DEF2(name, ...)                                             \
  M_VARIANT_DEF2_AS(name, M_F(name,_t), __VA_ARGS__)


/* Define the variant type and functions
  as the given name_t
   USAGE:
     VARIANT_DEF2_AS(name, name_t, [(field1, type1, oplist1), (field2, type2, oplist2), ...] ) */
#define M_VARIANT_DEF2_AS(name, name_t, ...)                                  \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_VAR1ANT_DEF2_P1( (name, name_t M_VAR1ANT_INJECT_GLOBAL(__VA_ARGS__)) )    \
  M_END_PROTECTED_CODE


/* Define the oplist of a variant.
   USAGE: VARIANT_OPLIST(name[, oplist of the first type, ...]) */
#define M_VARIANT_OPLIST(...)                                                 \
  M_IF_NARGS_EQ1(__VA_ARGS__)                                                 \
  (M_VAR1ANT_OPLIST_P1((__VA_ARGS__, M_BASIC_OPLIST)),                        \
   M_VAR1ANT_OPLIST_P1((__VA_ARGS__ )))


/*****************************************************************************/
/********************************** INTERNAL *********************************/
/*****************************************************************************/

/* Contract of a variant. */
#define M_VAR1ANT_CONTRACT(name, my) do {                                     \
  M_ASSERT(my != NULL);                                                       \
  M_ASSERT(my->type >= M_F(name, _EMPTY));                                    \
  M_ASSERT(my->type <= (enum M_F(name, _enum)) M_F(name, _MAX_TYPE));         \
} while (0)

/* Inject the oplist within the list of arguments */
#define M_VAR1ANT_INJECT_GLOBAL(...)                                          \
  M_MAP(M_VAR1ANT_INJECT_OPLIST_A, __VA_ARGS__)

/* Transform (x, type) into (x, type, oplist) if there is global registered oplist 
   or (x, type, M_BASIC_OPLIST) if there is no global one,
   or keep (x, type, oplist) if oplist was already present */
#define M_VAR1ANT_INJECT_OPLIST_A( duo_or_trio )                              \
  M_VAR1ANT_INJECT_OPLIST_B duo_or_trio

#define M_VAR1ANT_INJECT_OPLIST_B( f, ... )                                   \
  M_DEFERRED_COMMA                                                            \
  M_IF_NARGS_EQ1(__VA_ARGS__)( (f, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)()), (f, __VA_ARGS__) )

// Deferred evaluation
#define M_VAR1ANT_DEF2_P1(...)            M_ID( M_VAR1ANT_DEF2_P2 __VA_ARGS__ )

// Test if all third argument of all arguments is an oplist
#define M_VAR1ANT_IF_ALL_OPLIST(...)                                          \
  M_IF(M_REDUCE(M_VAR1ANT_IS_OPLIST_P, M_AND, __VA_ARGS__))

// Test if the third argument is an oplist
#define M_VAR1ANT_IS_OPLIST_P(a)                                              \
  M_OPLIST_P(M_RET_ARG3 a)

/* Validate the oplist before going further */
#define M_VAR1ANT_DEF2_P2(name, name_t, ...)                                  \
  M_VAR1ANT_IF_ALL_OPLIST(__VA_ARGS__)(M_VAR1ANT_DEF2_P3, M_VAR1ANT_DEF2_FAILURE)(name, name_t, __VA_ARGS__)

/* Stop processing with a compilation failure */
#define M_VAR1ANT_DEF2_FAILURE(name, name_t, ...)                             \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST, "(VARIANT_DEF2): at least one of the given argument is not a valid oplist: " #__VA_ARGS__)

/* Define the variant */
#define M_VAR1ANT_DEF2_P3(name, name_t, ...)                                  \
  M_VAR1ANT_DEFINE_TYPE(name, name_t, __VA_ARGS__)                            \
  M_VAR1ANT_CONTROL_ALL_OPLIST(name, __VA_ARGS__)                             \
  M_VAR1ANT_DEFINE_INIT(name, __VA_ARGS__)                                    \
  M_VAR1ANT_DEFINE_CLEAR(name, __VA_ARGS__)                                   \
  M_VAR1ANT_DEFINE_INIT_SET(name, __VA_ARGS__)                                \
  M_VAR1ANT_DEFINE_SET(name, __VA_ARGS__)                                     \
  M_VAR1ANT_DEFINE_EMPLACE(name, __VA_ARGS__)                                 \
  M_VAR1ANT_DEFINE_TEST_P(name, __VA_ARGS__)                                  \
  M_VAR1ANT_IF_ALL(INIT, __VA_ARGS__)(M_VAR1ANT_DEFINE_INIT_FIELD, M_EAT)(name, __VA_ARGS__) \
  M_VAR1ANT_DEFINE_INIT_SETTER_FIELD(name, __VA_ARGS__)                       \
  M_VAR1ANT_DEFINE_SETTER_FIELD(name, __VA_ARGS__)                            \
  M_VAR1ANT_DEFINE_GETTER_FIELD(name, __VA_ARGS__)                            \
  M_VAR1ANT_DEFINE_RESET_FUNC(name, __VA_ARGS__)                              \
  M_VAR1ANT_IF_ALL(HASH, __VA_ARGS__)(M_VAR1ANT_DEFINE_HASH, M_EAT)(name, __VA_ARGS__) \
  M_VAR1ANT_IF_ALL(EQUAL, __VA_ARGS__)(M_VAR1ANT_DEFINE_EQUAL, M_EAT)(name, __VA_ARGS__) \
  M_VAR1ANT_IF_ALL(GET_STR, __VA_ARGS__)(M_VAR1ANT_DEFINE_GET_STR, M_EAT)(name, __VA_ARGS__) \
  M_VAR1ANT_IF_ALL2(PARSE_STR, INIT, __VA_ARGS__)(M_VAR1ANT_DEFINE_PARSE_STR, M_EAT)(name, __VA_ARGS__) \
  M_VAR1ANT_IF_ALL(OUT_STR, __VA_ARGS__)(M_VAR1ANT_DEFINE_OUT_STR, M_EAT)(name, __VA_ARGS__) \
  M_VAR1ANT_IF_ALL2(IN_STR, INIT, __VA_ARGS__)(M_VAR1ANT_DEFINE_IN_STR, M_EAT)(name, __VA_ARGS__) \
  M_VAR1ANT_IF_ALL(OUT_SERIAL, __VA_ARGS__)(M_VAR1ANT_DEFINE_OUT_SERIAL, M_EAT)(name, __VA_ARGS__) \
  M_VAR1ANT_IF_ALL2(IN_SERIAL, INIT, __VA_ARGS__)(M_VAR1ANT_DEFINE_IN_SERIAL, M_EAT)(name, __VA_ARGS__) \
  M_VAR1ANT_IF_ALL(INIT_MOVE, __VA_ARGS__)(M_VAR1ANT_DEFINE_INIT_MOVE, M_EAT)(name, __VA_ARGS__) \
  M_VAR1ANT_IF_ALL(INIT_MOVE, __VA_ARGS__)(M_VAR1ANT_DEFINE_MOVE, M_EAT)(name, __VA_ARGS__) \
  M_VAR1ANT_IF_ALL(INIT_MOVE, __VA_ARGS__)(M_VAR1ANT_DEFINE_MOVER, M_EAT)(name, __VA_ARGS__) \
  M_VAR1ANT_IF_ALL(SWAP, __VA_ARGS__)(M_VAR1ANT_DEFINE_SWAP, M_EAT)(name, __VA_ARGS__)


/* Get the field name, the type, the oplist or the methods
   based on the variant (field, type, oplist) */
#define M_VAR1ANT_GET_FIELD(f,t,o)    f
#define M_VAR1ANT_GET_TYPE(f,t,o)     t
#define M_VAR1ANT_GET_OPLIST(f,t,o)   o
#define M_VAR1ANT_GET_INIT(f,t,o)     M_GET_INIT o
#define M_VAR1ANT_GET_INIT_SET(f,t,o) M_GET_INIT_SET o
#define M_VAR1ANT_GET_INIT_MOVE(f,t,o) M_GET_INIT_MOVE o
#define M_VAR1ANT_GET_MOVE(f,t,o)     M_GET_MOVE o
#define M_VAR1ANT_GET_SET(f,t,o)      M_GET_SET o
#define M_VAR1ANT_GET_CLEAR(f,t,o)    M_GET_CLEAR o
#define M_VAR1ANT_GET_CMP(f,t,o)      M_GET_CMP o
#define M_VAR1ANT_GET_HASH(f,t,o)     M_GET_HASH o
#define M_VAR1ANT_GET_EQUAL(f,t,o)    M_GET_EQUAL o
#define M_VAR1ANT_GET_STR(f,t,o)      M_GET_GET_STR o
#define M_VAR1ANT_GET_PARSE_STR(f,t,o) M_GET_PARSE_STR o
#define M_VAR1ANT_GET_OUT_STR(f,t,o)  M_GET_OUT_STR o
#define M_VAR1ANT_GET_IN_STR(f,t,o)   M_GET_IN_STR o
#define M_VAR1ANT_GET_OUT_SERIAL(f,t,o) M_GET_OUT_SERIAL o
#define M_VAR1ANT_GET_IN_SERIAL(f,t,o)  M_GET_IN_SERIAL o
#define M_VAR1ANT_GET_SWAP(f,t,o)     M_GET_SWAP o

/* Call the methods through API */
#define M_VAR1ANT_CALL_INIT(t, ...)       M_APPLY_API(M_VAR1ANT_GET_INIT t,  M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_INIT_SET(t, ...)   M_APPLY_API(M_VAR1ANT_GET_INIT_SET t,  M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_INIT_MOVE(t, ...)  M_APPLY_API(M_VAR1ANT_GET_INIT_MOVE t,  M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_MOVE(t, ...)       M_APPLY_API(M_VAR1ANT_GET_MOVE t,  M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_SET(t, ...)        M_APPLY_API(M_VAR1ANT_GET_SET t,   M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_CLEAR(t, ...)      M_APPLY_API(M_VAR1ANT_GET_CLEAR t, M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_CMP(t, ...)        M_APPLY_API(M_VAR1ANT_GET_CMP t,   M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_HASH(t, ...)       M_APPLY_API(M_VAR1ANT_GET_HASH t,  M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_EQUAL(t, ...)      M_APPLY_API(M_VAR1ANT_GET_EQUAL t, M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_GET_STR(t, ...)    M_APPLY_API(M_VAR1ANT_GET_STR t,   M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_PARSE_STR(t, ...)  M_APPLY_API(M_VAR1ANT_GET_PARSE_STR t, M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_OUT_STR(t, ...)    M_APPLY_API(M_VAR1ANT_GET_OUT_STR t, M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_IN_STR(t, ...)     M_APPLY_API(M_VAR1ANT_GET_IN_STR t, M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_OUT_SERIAL(t, ...) M_APPLY_API(M_VAR1ANT_GET_OUT_SERIAL t, M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_IN_SERIAL(t, ...)  M_APPLY_API(M_VAR1ANT_GET_IN_SERIAL t, M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)
#define M_VAR1ANT_CALL_SWAP(t, ...)       M_APPLY_API(M_VAR1ANT_GET_SWAP t,  M_VAR1ANT_GET_OPLIST t, __VA_ARGS__)

/* Define the type */
#define M_VAR1ANT_DEFINE_TYPE(name, name_t, ...)                              \
  /* Define enum of all types of the variant */                               \
  enum M_F(name, _enum) { M_F(name, _EMPTY)                                   \
      M_MAP2(M_VAR1ANT_DEFINE_UNION_ELE, name, __VA_ARGS__)                   \
  };                                                                          \
  /* Define enum equal to the number of types of the variant */               \
  enum M_F(name, _enum_max) {                                                 \
    M_F(name, _MAX_TYPE) = M_NARGS(__VA_ARGS__)                               \
  };                                                                          \
  /* Define the variant */                                                    \
  typedef struct M_F(name, _s) *M_F(name, _ptr);                              \
  typedef const struct M_F(name, _s) *M_F(name, _srcptr);                     \
  typedef struct M_F(name, _s) {                                              \
    enum M_F(name, _enum) type;                                               \
    union {                                                                   \
      M_MAP(M_VAR1ANT_DEFINE_TYPE_ELE , __VA_ARGS__)                          \
    } value;                                                                  \
  } name_t[1];                                                                \
                                                                              \
  /* Define internal type for oplist */                                       \
  typedef name_t M_F(name, _ct);                                              \
  /* Save alias for the types of arguments */                                 \
  M_MAP3(M_VAR1ANT_DEFINE_GENTYPE_ELE, name, __VA_ARGS__)

#define M_VAR1ANT_DEFINE_GENTYPE_ELE(name, num, a)                            \
  typedef                                                                     \
  M_IF_METHOD(GENTYPE, M_VAR1ANT_GET_OPLIST a)( M_GET_GENTYPE M_VAR1ANT_GET_OPLIST a, M_VAR1ANT_GET_TYPE a) \
  M_C4(m_var1ant_, name, _gentype_, num);                                     \
  typedef                                                                     \
  M_IF_METHOD(GENTYPE, M_VAR1ANT_GET_OPLIST a)( const M_GET_GENTYPE M_VAR1ANT_GET_OPLIST a, M_VAR1ANT_GET_TYPE a const) \
  M_C4(m_var1ant_, name, _const_gentype_, num);                               \
  typedef M_VAR1ANT_GET_TYPE a M_C4(m_var1ant_, name, _type_, M_VAR1ANT_GET_FIELD a);

#define M_VAR1ANT_DEFINE_UNION_ELE(name, a)                                   \
  , M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value)

#define M_VAR1ANT_DEFINE_TYPE_ELE(a)                                          \
  M_VAR1ANT_GET_TYPE a M_VAR1ANT_GET_FIELD a ;


/* Control that all given oplists of all parameters are really oplists */
#define M_VAR1ANT_CONTROL_ALL_OPLIST(name, ...)                               \
  M_MAP2(M_VAR1ANT_CONTROL_OPLIST, name, __VA_ARGS__)

#define M_VAR1ANT_CONTROL_OPLIST(name, a)                                     \
  M_CHECK_COMPATIBLE_OPLIST(name, M_VAR1ANT_GET_FIELD a,                      \
                            M_VAR1ANT_GET_TYPE a, M_VAR1ANT_GET_OPLIST a)


/* Define the INIT function. Init the variant to empty */
#define M_VAR1ANT_DEFINE_INIT(name, ...)                                      \
  M_INLINE void M_F(name, _init)(M_F(name,_ct) my) {                          \
    my->type = M_F(name, _EMPTY);                                             \
  }


/* Define the INIT_SET function. */
#define M_VAR1ANT_DEFINE_INIT_SET(name, ...)                                  \
  M_P(void, name, _init_set, M_F(name,_ct) my, M_F(name,_ct) const org)       \
  {                                                                           \
    M_VAR1ANT_CONTRACT(name, org);                                            \
    M_UNUSED_CONTEXT();                                                       \
    my->type = org->type;                                                     \
    switch (org->type) {                                                      \
      M_MAP2(M_VAR1ANT_DEFINE_INIT_SET_FUNC, name, __VA_ARGS__)               \
      case M_F(name, _EMPTY): /* fallthrough */                               \
      default: M_ASSUME(org->type == M_F(name, _EMPTY)); break;               \
    }                                                                         \
    M_VAR1ANT_CONTRACT(name, my);                                             \
  }

#define M_VAR1ANT_DEFINE_INIT_SET_FUNC(name, a)                               \
  case M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value):                          \
  M_VAR1ANT_CALL_INIT_SET(a, my -> value. M_VAR1ANT_GET_FIELD a ,             \
                         org -> value.M_VAR1ANT_GET_FIELD a );                \
  break;


/* Define the SET function. */
#define M_VAR1ANT_DEFINE_SET(name, ...)                                       \
  M_P(void, name, _set, M_F(name,_ct) my, M_F(name,_ct) const org)            \
  {                                                                           \
    M_VAR1ANT_CONTRACT(name, my);                                             \
    M_VAR1ANT_CONTRACT(name, org);                                            \
    if (my->type != org->type) {                                              \
      /* Different types: clear previous one and create new */                \
      M_F(name, _clear)M_R(my);                                               \
      M_F(name, _init_set)M_R(my, org);                                       \
    } else {                                                                  \
      /* Same type: optimize the set */                                       \
      switch (org->type) {                                                    \
        M_MAP2(M_VAR1ANT_DEFINE_SET_FUNC, name, __VA_ARGS__)                  \
        case M_F(name, _EMPTY): /* fallthrough */                             \
        default: M_ASSUME(org->type == M_F(name, _EMPTY)); break;             \
      }                                                                       \
    }                                                                         \
    M_ASSERT( my->type == org->type );                                        \
    M_VAR1ANT_CONTRACT(name, my);                                             \
  }

#define M_VAR1ANT_DEFINE_SET_FUNC(name, a)                                    \
  case M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value):                          \
  M_VAR1ANT_CALL_SET(a, my -> value. M_VAR1ANT_GET_FIELD a ,                  \
                    org -> value.M_VAR1ANT_GET_FIELD a );                     \
  break;


/* Define the CLEAR function. */
#define M_VAR1ANT_DEFINE_CLEAR(name, ...)                                     \
  M_P(void, name, _clear, M_F(name,_ct) my)                                   \
  {                                                                           \
    M_VAR1ANT_CONTRACT(name, my);                                             \
    M_UNUSED_CONTEXT();                                                       \
    switch (my->type) {                                                       \
      M_MAP2(M_VAR1ANT_DEFINE_CLEAR_FUNC, name,  __VA_ARGS__)                 \
      case M_F(name, _EMPTY): /* fallthrough */                               \
      default: M_ASSUME(my->type == M_F(name, _EMPTY)); break;                \
    }                                                                         \
    my->type = M_F(name, _EMPTY);                                             \
  }

#define M_VAR1ANT_DEFINE_CLEAR_FUNC(name, a)                                  \
  case M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value):                          \
  M_VAR1ANT_CALL_CLEAR(a, my -> value. M_VAR1ANT_GET_FIELD a);                \
  break;


/* Define the TEST_P function. */
#define M_VAR1ANT_DEFINE_TEST_P(name, ...)                                    \
  M_INLINE bool M_F(name, _empty_p)(M_F(name,_ct) const my) {                 \
    M_VAR1ANT_CONTRACT(name, my);                                             \
    return my->type == M_F(name, _EMPTY);                                     \
  }                                                                           \
  M_INLINE enum M_F(name, _enum)                                              \
  M_F(name, _type)(M_F(name,_ct) my) {                                        \
    M_VAR1ANT_CONTRACT(name, my);                                             \
    return my->type;                                                          \
  }                                                                           \
  M_MAP2(M_VAR1ANT_DEFINE_TEST_FUNC, name, __VA_ARGS__)

#define M_VAR1ANT_DEFINE_TEST_FUNC(name, a)                                   \
  M_INLINE bool                                                               \
  M_C4(name, _, M_VAR1ANT_GET_FIELD a, _p)(M_F(name,_ct) const my) {          \
    M_VAR1ANT_CONTRACT(name, my);                                             \
    return my->type == M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value);          \
  }


/* Define the INIT function. */
#define M_VAR1ANT_DEFINE_INIT_FIELD(name, ...)                                \
  M_MAP2(M_VAR1ANT_DEFINE_INIT_FIELD_FUNC, name, __VA_ARGS__)

#define M_VAR1ANT_DEFINE_INIT_FIELD_FUNC(name, a)                             \
  M_INLINE void                                                               \
  M_C3(name, _init_, M_VAR1ANT_GET_FIELD a)(M_P_EXPAND M_F(name,_ct) my)      \
  {                                                                           \
    M_UNUSED_CONTEXT();                                                       \
    /* Reinit variable with the given value */                                \
    my->type = M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value);                  \
    M_VAR1ANT_CALL_INIT(a, my -> value. M_VAR1ANT_GET_FIELD a);               \
  }


/* Define the INIT_SET of a given type function. */
#define M_VAR1ANT_DEFINE_INIT_SETTER_FIELD(name, ...)                         \
  M_MAP3(M_VAR1ANT_DEFINE_INIT_SETTER_FIELD_FUNC, name, __VA_ARGS__)

#define M_VAR1ANT_DEFINE_INIT_SETTER_FIELD_FUNC(name, num, a)                 \
  M_INLINE void                                                               \
  M_C3(name, _init_set_, M_VAR1ANT_GET_FIELD a)(M_P_EXPAND M_F(name,_ct) my, M_VAR1ANT_GET_TYPE a const M_VAR1ANT_GET_FIELD a) \
  {                                                                           \
    M_UNUSED_CONTEXT();                                                       \
    my->type = M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value);                  \
    M_VAR1ANT_CALL_INIT_SET(a, my -> value. M_VAR1ANT_GET_FIELD a,            \
                           M_VAR1ANT_GET_FIELD a);                            \
  }                                                                           \
  M_INLINE void                                                               \
  M_C4(m_var1ant_, name, _init_set_, num)(M_P_EXPAND M_F(name,_ct) my, M_VAR1ANT_GET_TYPE a const M_VAR1ANT_GET_FIELD a) \
  {                                                                           \
    M_UNUSED_CONTEXT();                                                       \
    my->type = M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value);                  \
    M_VAR1ANT_CALL_INIT_SET(a, my -> value. M_VAR1ANT_GET_FIELD a,            \
                           M_VAR1ANT_GET_FIELD a);                            \
  }


/* Define the SET of a given type function. */
#define M_VAR1ANT_DEFINE_SETTER_FIELD(name, ...)                              \
  M_MAP2(M_VAR1ANT_DEFINE_SETTER_FIELD_FUNC, name, __VA_ARGS__)

#define M_VAR1ANT_DEFINE_SETTER_FIELD_FUNC(name, a)                           \
  M_INLINE void                                                               \
  M_C3(name, _set_, M_VAR1ANT_GET_FIELD a)(M_P_EXPAND M_F(name,_ct) my,       \
                                          M_VAR1ANT_GET_TYPE a const M_VAR1ANT_GET_FIELD a  ) { \
    M_VAR1ANT_CONTRACT(name, my);                                             \
    if (my->type == M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value) ) {          \
      M_VAR1ANT_CALL_SET(a, my -> value. M_VAR1ANT_GET_FIELD a,               \
                        M_VAR1ANT_GET_FIELD a);                               \
    } else {                                                                  \
      M_F(name, _clear)M_R(my);                                               \
      /* Reinit variable with the given value */                              \
      my->type = M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value);                \
      M_VAR1ANT_CALL_INIT_SET(a, my -> value. M_VAR1ANT_GET_FIELD a,          \
                             M_VAR1ANT_GET_FIELD a);                          \
    }                                                                         \
  }


/* Define the GET_field of a given type function. */
#define M_VAR1ANT_DEFINE_GETTER_FIELD(name, ...)                              \
  M_MAP2(M_VAR1ANT_DEFINE_GETTER_FIELD_FUNC, name, __VA_ARGS__)

#define M_VAR1ANT_DEFINE_GETTER_FIELD_FUNC(name, a)                           \
  M_INLINE M_VAR1ANT_GET_TYPE a *                                             \
  M_C3(name, _get_, M_VAR1ANT_GET_FIELD a)(M_F(name,_ct) my) {                \
    M_VAR1ANT_CONTRACT(name, my);                                             \
    if (my->type != M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value) ) {          \
      return NULL;                                                            \
    }                                                                         \
    return &my -> value . M_VAR1ANT_GET_FIELD a;                              \
  }                                                                           \
                                                                              \
  M_INLINE M_VAR1ANT_GET_TYPE a const *                                       \
  M_C3(name, _cget_, M_VAR1ANT_GET_FIELD a)(M_F(name,_ct) const my) {         \
    M_VAR1ANT_CONTRACT(name, my);                                             \
    if (my->type != M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value) ) {          \
      return NULL;                                                            \
    }                                                                         \
    return &my -> value . M_VAR1ANT_GET_FIELD a;                              \
  }


/* Define the EMPLACE of a given type function.
   NOTE: Use of a variant of MAP3 because of recursive use of MAP2/MAP3/REDUCE2 !
 */
#define M_VAR1ANT_DEFINE_EMPLACE(name, ...)                                   \
  M_VAR1ANT_MAP3_ALT(M_VAR1ANT_DEFINE_EMPLACE_FUNC, name, __VA_ARGS__)
// Variant of M_MAP3 using M_REDUCE3
#define M_VAR1ANT_MAP3_ALT(f, d, ...)   M_REDUCE3(f, M_VAR1ANT_MAP3_ALT_ID, d, __VA_ARGS__)
#define M_VAR1ANT_MAP3_ALT_ID(a, b)     a b

#define M_VAR1ANT_DEFINE_EMPLACE_FUNC(name, num, a)                           \
  M_EMPLACE_QUEUE_DEF( (name, M_VAR1ANT_GET_FIELD a), M_F(name,_ct), M_C3(name, _init_emplace_, M_VAR1ANT_GET_FIELD a), M_VAR1ANT_GET_OPLIST a, M_VAR1ANT_DEFINE_INIT_EMPLACE_DEF) \
  M_EMPLACE_QUEUE_DEF( (name, M_VAR1ANT_GET_FIELD a), M_F(name,_ct), M_C3(name, _emplace_, M_VAR1ANT_GET_FIELD a), M_VAR1ANT_GET_OPLIST a, M_VAR1ANT_DEFINE_EMPLACE_DEF)

#define M_VAR1ANT_DEFINE_INIT_EMPLACE_DEF(name, name_t, function_name, oplist, init_func, exp_emplace_type) \
M_INLINE void                                                                 \
  function_name(M_P_EXPAND name_t my M_EMPLACE_LIST_TYPE_VAR(ab, exp_emplace_type) ) \
  {                                                                           \
    my->type = M_C4(M_PAIR_1 name, _, M_PAIR_2 name, _value);                 \
    M_EMPLACE_CALL_FUNC(ab, init_func, oplist, my -> value. M_PAIR_2 name, exp_emplace_type); \
  }                                                                           \

#define M_VAR1ANT_DEFINE_EMPLACE_DEF(name, name_t, function_name, oplist, init_func, exp_emplace_type) \
  M_INLINE void                                                               \
  function_name(M_P_EXPAND name_t my M_EMPLACE_LIST_TYPE_VAR(ab, exp_emplace_type) ) \
  {                                                                           \
    /* No optimization done */                                                \
    M_C(M_PAIR_1 name, _clear)M_R(my);                                        \
    my->type = M_F(M_PAIR_1 name, _EMPTY);                                    \
    M_EMPLACE_CALL_FUNC(ab, init_func, oplist, my -> value. M_PAIR_2 name, exp_emplace_type); \
    my->type = M_C4(M_PAIR_1 name, _, M_PAIR_2 name, _value);                 \
  }                                                                           \

/* Define the EQUAL_P function. */
#define M_VAR1ANT_DEFINE_EQUAL(name, ...)                                     \
  M_INLINE bool M_F(name, _equal_p)(M_F(name,_ct) const e1 ,                  \
                                         M_F(name,_ct) const e2) {            \
    bool b;                                                                   \
    M_VAR1ANT_CONTRACT(name, e1);                                             \
    M_VAR1ANT_CONTRACT(name, e2);                                             \
    if (e1->type != e2->type) return false;                                   \
    switch (e1->type) {                                                       \
    case M_F(name, _EMPTY): break;                                            \
      M_MAP2(M_VAR1ANT_DEFINE_EQUAL_FUNC , name, __VA_ARGS__)                 \
    default: M_ASSUME(false); break;                                          \
    }                                                                         \
    return true;                                                              \
  }

#define M_VAR1ANT_DEFINE_EQUAL_FUNC(name, a)                                  \
  case M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value):                          \
  b = M_VAR1ANT_CALL_EQUAL(a, e1 -> value . M_VAR1ANT_GET_FIELD a ,           \
                          e2 -> value . M_VAR1ANT_GET_FIELD a );              \
  return b;                                                                   \
  break;


/* Define the HASH function. */
#define M_VAR1ANT_DEFINE_HASH(name, ...)                                      \
  M_INLINE size_t M_F(name, _hash)(M_F(name,_ct) const e1) {                  \
    M_VAR1ANT_CONTRACT(name, e1);                                             \
    M_HASH_DECL(hash);                                                        \
    M_HASH_UP (hash, (unsigned int) (e1 -> type));                            \
    switch (e1->type) {                                                       \
    case M_F(name, _EMPTY): break;                                            \
      M_MAP2(M_VAR1ANT_DEFINE_HASH_FUNC , name, __VA_ARGS__)                  \
    default: M_ASSUME(false); break;                                          \
    }                                                                         \
    return M_HASH_FINAL (hash);                                               \
  }

#define M_VAR1ANT_DEFINE_HASH_FUNC(name, a)                                   \
  case M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value):                          \
  M_HASH_UP(hash, M_VAR1ANT_CALL_HASH(a, e1 -> value . M_VAR1ANT_GET_FIELD a) ); \
  break;


/* Define the INIT_MOVE function. */
#define M_VAR1ANT_DEFINE_INIT_MOVE(name, ...)                                 \
  M_INLINE void                                                               \
  M_F(name, _init_move)(M_F(name,_ct) el, M_F(name,_ct) org) {                \
    M_VAR1ANT_CONTRACT(name, org);                                            \
    el -> type = org -> type;                                                 \
    switch (el->type) {                                                       \
    case M_F(name, _EMPTY): break;                                            \
    M_MAP2(M_VAR1ANT_DEFINE_INIT_MOVE_FUNC , name, __VA_ARGS__)               \
    default: M_ASSUME(false); break;                                          \
    }                                                                         \
    org -> type = M_F(name, _EMPTY);                                          \
  }

#define M_VAR1ANT_DEFINE_INIT_MOVE_FUNC(name, a)                              \
  case M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value):                          \
  M_VAR1ANT_CALL_INIT_MOVE(a, el -> value . M_VAR1ANT_GET_FIELD a,            \
                          org -> value . M_VAR1ANT_GET_FIELD a);              \
  break;


/* Define the MOVE function.
   This is not optimized version.
   It can be optimized if both types are the same.
*/
#define M_VAR1ANT_DEFINE_MOVE(name, ...)                                      \
  M_P(void, name, _move, M_F(name,_ct) el, M_F(name,_ct) org)                 \
  {                                                                           \
    M_F(name, _clear)M_R(el);                                                 \
    M_F(name, _init_move)(el , org);                                          \
  }


/* Define the MOVE function of a given type */
#define M_VAR1ANT_DEFINE_MOVER(name, ...)                                     \
  M_MAP2(M_VAR1ANT_DEFINE_MOVER_FUNC, name, __VA_ARGS__)

#define M_VAR1ANT_DEFINE_MOVER_FUNC(name, a)                                  \
  M_INLINE void                                                               \
  M_C3(name, _move_, M_VAR1ANT_GET_FIELD a)(M_P_EXPAND M_F(name,_ct) my,      \
                                           M_VAR1ANT_GET_TYPE a  M_VAR1ANT_GET_FIELD a  ) { \
    M_VAR1ANT_CONTRACT(name, my);                                             \
    M_F(name, _clear)M_R(my);                                                 \
    /* Reinit variable with the given value */                                \
    my->type = M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value);                  \
    M_VAR1ANT_CALL_INIT_MOVE(a, my -> value. M_VAR1ANT_GET_FIELD a,           \
                            M_VAR1ANT_GET_FIELD a);                           \
  }


/* Define the SWAP function  */
#define M_VAR1ANT_DEFINE_SWAP(name, ...)                                      \
  M_INLINE void                                                               \
  M_F(name, _swap)(M_F(name,_ct) el1, M_F(name,_ct) el2) {                    \
    M_VAR1ANT_CONTRACT(name, el1);                                            \
    M_VAR1ANT_CONTRACT(name, el2);                                            \
    if (el1->type == el2->type) {                                             \
      switch (el1->type) {                                                    \
      case M_F(name, _EMPTY): break;                                          \
        M_MAP2(M_VAR1ANT_DEFINE_INIT_SWAP_FUNC , name, __VA_ARGS__)           \
      default: M_ASSUME(false); break;                                        \
      }                                                                       \
    } else {                                                                  \
      M_F(name,_ct) tmp;                                                      \
      M_VAR1ANT_IF_ALL(INIT_MOVE, __VA_ARGS__)                                \
        (      /* NOTE: Slow implementation */                                \
         M_F(name, _init_move)(tmp, el1);                                     \
         M_F(name, _init_move)(el1, el2);                                     \
         M_F(name, _init_move)(el2, tmp);                                     \
         ,                                                                    \
         /* NOTE: Very slow implementation */                                 \
         M_F(name, _init_set)(tmp, el1);                                      \
         M_F(name, _set)(el1, el2);                                           \
         M_F(name, _set)(el2, tmp);                                           \
         M_F(name, _clear)(tmp);                                              \
               )                                                              \
    }                                                                         \
  }

#define M_VAR1ANT_DEFINE_INIT_SWAP_FUNC(name, a)                              \
  case M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value):                          \
  M_VAR1ANT_CALL_SWAP(a, el1 -> value . M_VAR1ANT_GET_FIELD a,                \
                     el2 -> value . M_VAR1ANT_GET_FIELD a);                   \
  break;


/* Define the GET_STR function */
#define M_VAR1ANT_DEFINE_GET_STR(name, ...)                                   \
  M_P(void, name, _get_str, m_string_t str, M_F(name,_ct) const el, bool append) \
  {                                                                           \
    M_VAR1ANT_CONTRACT(name, el);                                             \
    M_ASSERT (str != NULL);                                                   \
    void (*func)(M_P_EXPAND m_string_t, const char *);                        \
    func = append ? m_string_cat_cstr : m_string_set_cstr;                    \
    switch (el->type) {                                                       \
    case M_F(name, _EMPTY): func M_R(str, "@EMPTY{"); break;                  \
      M_MAP2(M_VAR1ANT_DEFINE_GET_STR_FUNC , name, __VA_ARGS__)               \
    default: M_ASSUME(false); break;                                          \
    }                                                                         \
    m_string_push_back M_R(str, '}');                                         \
  }

#define M_VAR1ANT_DEFINE_GET_STR_FUNC(name, a)                                \
  case M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value):                          \
  func M_R(str, "@" M_AS_STR(M_VAR1ANT_GET_FIELD a) "{");                     \
  M_VAR1ANT_CALL_GET_STR(a, str, el -> value . M_VAR1ANT_GET_FIELD a, true);  \
  break;


/* Define the PARSE_STR function */
#define M_VAR1ANT_DEFINE_PARSE_STR(name, ...)                                 \
  M_P(bool, name, _parse_str, M_F(name,_ct) el, const char str[], const char **endp) \
  {                                                                           \
    M_VAR1ANT_CONTRACT(name, el);                                             \
    M_ASSERT (str != NULL);                                                   \
    bool success = false;                                                     \
    char variantTypeBuf[M_USE_IDENTIFIER_ALLOC+1];                            \
    int  c = *str++;                                                          \
    unsigned int i = 0;                                                       \
    M_F(name, _reset)M_R(el);                                                 \
    if (c != '@') goto exit;                                                  \
    /* First read the name of the type */                                     \
    c = *str++;                                                               \
    while (c != '{' && c != 0 && i < sizeof(variantTypeBuf) - 1) {            \
      variantTypeBuf[i++] = (char) c;                                         \
      c = *str++;                                                             \
    }                                                                         \
    if (c != '{') goto exit;                                                  \
    variantTypeBuf[i++] = 0;                                                  \
    M_ASSERT(i < sizeof(variantTypeBuf));                                     \
    /* In function of the type */                                             \
    if (strcmp(variantTypeBuf, "EMPTY") == 0) {                               \
      el->type = M_F(name, _EMPTY);                                           \
    }                                                                         \
    M_MAP2(M_VAR1ANT_DEFINE_PARSE_STR_FUNC , name, __VA_ARGS__)               \
    else goto exit;                                                           \
    success = (*str++ == '}');                                                \
  exit:                                                                       \
    if (endp) *endp = str;                                                    \
    return success;                                                           \
  }

#define M_VAR1ANT_DEFINE_PARSE_STR_FUNC(name, a)                              \
  else if (strcmp (variantTypeBuf, M_AS_STR(M_VAR1ANT_GET_FIELD a)) == 0) {   \
    el->type = M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value);                  \
    M_VAR1ANT_CALL_INIT(a, el ->value . M_VAR1ANT_GET_FIELD a );              \
    bool b = M_VAR1ANT_CALL_PARSE_STR(a, el -> value . M_VAR1ANT_GET_FIELD a, str, &str); \
    if (!b) goto exit;                                                        \
  }


/* Define the OUT_STR function */
#define M_VAR1ANT_DEFINE_OUT_STR(name, ...)                                   \
  M_INLINE void M_F(name, _out_str)(FILE *f, M_F(name,_ct) const el)          \
  {                                                                           \
    M_VAR1ANT_CONTRACT(name, el);                                             \
    M_ASSERT (f != NULL);                                                     \
    switch (el->type) {                                                       \
    case M_F(name, _EMPTY): fprintf(f, "@EMPTY{"); break;                     \
      M_MAP2(M_VAR1ANT_DEFINE_OUT_STR_FUNC , name, __VA_ARGS__)               \
    default: M_ASSUME(false); break;                                          \
    }                                                                         \
    fputc ('}', f);                                                           \
  }

#define M_VAR1ANT_DEFINE_OUT_STR_FUNC(name, a)                                \
  case M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value):                          \
  fprintf(f, "@" M_AS_STR(M_VAR1ANT_GET_FIELD a) "{");                        \
  M_VAR1ANT_CALL_OUT_STR(a, f, el -> value . M_VAR1ANT_GET_FIELD a);          \
  break;


/* Define the IN_STR function */
#define M_VAR1ANT_DEFINE_IN_STR(name, ...)                                    \
  M_P(bool, name, _in_str, M_F(name,_ct) el, FILE *f)                         \
  {                                                                           \
    M_VAR1ANT_CONTRACT(name, el);                                             \
    M_ASSERT (f != NULL);                                                     \
    char variantTypeBuf[M_USE_IDENTIFIER_ALLOC+1];                            \
    M_F(name, _reset)M_R(el);                                                 \
    if (fgetc(f) != '@') return false;                                        \
    /* First read the name of the type */                                     \
    bool b = true;                                                            \
    int c = fgetc(f);                                                         \
    unsigned int i = 0;                                                       \
    while (c != '{' && c != EOF && i < sizeof(variantTypeBuf) - 1) {          \
      variantTypeBuf[i++] = (char) c;                                         \
      c = fgetc(f);                                                           \
    }                                                                         \
    if (c != '{') return false;                                               \
    variantTypeBuf[i++] = 0;                                                  \
    M_ASSERT(i < sizeof(variantTypeBuf));                                     \
    /* In function of the type */                                             \
    if (strcmp(variantTypeBuf, "EMPTY") == 0) {                               \
      el->type = M_F(name, _EMPTY);                                           \
    }                                                                         \
    M_MAP2(M_VAR1ANT_DEFINE_IN_STR_FUNC , name, __VA_ARGS__)                  \
    else { b = false; }                                                       \
    return b && (fgetc(f) == '}');                                            \
  }

#define M_VAR1ANT_DEFINE_IN_STR_FUNC(name, a)                                 \
  else if (strcmp (variantTypeBuf, M_AS_STR(M_VAR1ANT_GET_FIELD a)) == 0) {   \
    el->type = M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value);                  \
    M_VAR1ANT_CALL_INIT(a, el ->value . M_VAR1ANT_GET_FIELD a );              \
    b = M_VAR1ANT_CALL_IN_STR(a, el -> value . M_VAR1ANT_GET_FIELD a, f);     \
  }


/* Return the STRING version of a parameter name */
#define M_VAR1ANT_STRINGIFY_NAME(a)                                           \
  M_AS_STR(M_VAR1ANT_GET_FIELD a)


/* Define the OUT_SERIAL function */
#define M_VAR1ANT_DEFINE_OUT_SERIAL(name, ...)                                \
  M_P(m_serial_return_code_t, name, _out_serial, m_serial_write_t f, M_F(name,_ct) const el) \
  {                                                                           \
    M_VAR1ANT_CONTRACT(name, el);                                             \
    const int field_max = M_NARGS(__VA_ARGS__);                               \
    static const char *const field_name[] =                                   \
      { M_REDUCE(M_VAR1ANT_STRINGIFY_NAME, M_ID, __VA_ARGS__) };              \
    M_ASSERT (f != NULL && f->m_interface != NULL);                           \
    m_serial_local_t local;                                                   \
    m_serial_return_code_t ret;                                               \
    switch (el->type) {                                                       \
    case M_F(name, _EMPTY):                                                   \
      return f->m_interface->write_variant_start M_R(local, f, field_name, field_max, -1); \
      break;                                                                  \
    M_MAP2(M_VAR1ANT_DEFINE_OUT_SERIAL_FUNC , name, __VA_ARGS__)              \
    default: M_ASSUME(false); break;                                          \
    }                                                                         \
    ret |= f->m_interface->write_variant_end M_R(local, f);                   \
    return ret & M_SERIAL_FAIL;                                               \
  }

#define M_VAR1ANT_DEFINE_OUT_SERIAL_FUNC(name, a)                             \
  case M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value):                          \
  ret = f->m_interface->write_variant_start M_R(local, f, field_name, field_max, \
                       M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value) -1);      \
  M_VAR1ANT_CALL_OUT_SERIAL(a, f, el -> value . M_VAR1ANT_GET_FIELD a);       \
  break;


/* Define the IN_SERIAL function */
#define M_VAR1ANT_DEFINE_IN_SERIAL(name, ...)                                 \
  M_P(m_serial_return_code_t, name, _in_serial, M_F(name,_ct) el, m_serial_read_t f) \
  {                                                                           \
    M_VAR1ANT_CONTRACT(name, el);                                             \
    const int field_max = M_NARGS(__VA_ARGS__);                               \
    static const char *const field_name[] =                                   \
      { M_REDUCE(M_VAR1ANT_STRINGIFY_NAME, M_ID, __VA_ARGS__) };              \
    M_ASSERT (f != NULL && f->m_interface != NULL);                           \
    m_serial_local_t local;                                                   \
    m_serial_return_code_t ret;                                               \
    int id = -1;                                                              \
    M_F(name, _reset)M_R(el);                                                 \
    ret = f->m_interface->read_variant_start(local, f, field_name, field_max, &id); \
    if (ret != M_SERIAL_OK_CONTINUE) return ret;                              \
    M_ASSERT (id >= 0 && id < field_max);                                     \
    el->type = (enum M_F(name, _enum))(id+1);                                 \
    switch (id+1) {                                                           \
      M_MAP2(M_VAR1ANT_DEFINE_IN_SERIAL_FUNC , name, __VA_ARGS__)             \
    default: M_ASSUME(false); break;                                          \
    }                                                                         \
    if (ret == M_SERIAL_OK_DONE)                                              \
      ret = f->m_interface->read_variant_end(local, f);                       \
    return ret;                                                               \
  }

#define M_VAR1ANT_DEFINE_IN_SERIAL_FUNC(name, a)                              \
  case M_C4(name, _, M_VAR1ANT_GET_FIELD a, _value):                          \
    M_VAR1ANT_CALL_INIT(a, el ->value . M_VAR1ANT_GET_FIELD a );              \
    ret = M_VAR1ANT_CALL_IN_SERIAL(a, el -> value . M_VAR1ANT_GET_FIELD a, f); \
    break;                                                                    \


/* Define the RESET function */
#define M_VAR1ANT_DEFINE_RESET_FUNC(name, ...)                                \
  M_P(void, name, _reset, M_F(name,_ct) my)                                   \
  {                                                                           \
    M_VAR1ANT_CONTRACT(name, my);                                             \
    M_F(name, _clear)M_R(my);                                                 \
    M_F(name, _init)(my);                                                     \
  }                                                                           \


/********************************** INTERNAL *********************************/

/* deferred evaluation of the oplist */
#define M_VAR1ANT_OPLIST_P1(arg) M_VAR1ANT_OPLIST_P2 arg

/* Validate the oplist before going further */
#define M_VAR1ANT_OPLIST_P2(name, ...)                                        \
  M_IF(M_REDUCE(M_OPLIST_P, M_AND, __VA_ARGS__))(M_VAR1ANT_OPLIST_P3, M_VAR1ANT_OPLIST_FAILURE)(name, __VA_ARGS__)

/* Prepare a clean compilation failure */
#define M_VAR1ANT_OPLIST_FAILURE(name, ...)                                   \
  ((M_LIB_ERROR(ONE_ARGUMENT_OF_VARIANT_OPLIST_IS_NOT_AN_OPLIST, name, __VA_ARGS__)))

/* Define the oplist */
#ifndef M_USE_CONTEXT
#define M_VAR1ANT_OPLIST_P3(name, ...)                                        \
  (INIT(M_F(name,_init)),                                                     \
   INIT_SET(M_F(name, _init_set)),                                            \
   SET(M_F(name,_set)),                                                       \
   CLEAR(M_F(name, _clear)),                                                  \
   RESET(M_F(name, _reset)),                                                  \
   NAME(name),                                                                \
   TYPE(M_F(name,_ct)), GENTYPE(struct M_F(name,_s)*),                        \
   EMPTY_P(M_F(name,_empty_p)),                                               \
   SIZE( M_NARGS(__VA_ARGS__) ),                                              \
   INIT_WITH( API_1(M_VAR1ANT_INIT_WITH) ),                                   \
   M_IF_METHOD_ALL(HASH, __VA_ARGS__)(HASH(M_F(name, _hash)),),               \
   M_IF_METHOD_ALL(EQUAL, __VA_ARGS__)(EQUAL(M_F(name, _equal_p)),),          \
   M_IF_METHOD_ALL(GET_STR, __VA_ARGS__)(GET_STR(M_F(name, _get_str)),),      \
   M_IF_METHOD2_ALL(PARSE_STR, INIT, __VA_ARGS__)(PARSE_STR(M_F(name, _parse_str)),), \
   M_IF_METHOD2_ALL(IN_STR, INIT, __VA_ARGS__)(IN_STR(M_F(name, _in_str)),),  \
   M_IF_METHOD_ALL(OUT_STR, __VA_ARGS__)(OUT_STR(M_F(name, _out_str)),),      \
   M_IF_METHOD2_ALL(IN_SERIAL, INIT, __VA_ARGS__)(IN_SERIAL(M_F(name, _in_serial)),), \
   M_IF_METHOD_ALL(OUT_SERIAL, __VA_ARGS__)(OUT_SERIAL(M_F(name, _out_serial)),), \
   M_IF_METHOD_ALL(INIT_MOVE, __VA_ARGS__)(INIT_MOVE(M_F(name, _init_move)),), \
   M_IF_METHOD_ALL(INIT_MOVE, __VA_ARGS__)(MOVE(M_F(name, _move)),),          \
   M_IF_METHOD_ALL(SWAP, __VA_ARGS__)(SWAP(M_F(name, _swap)),),               \
   M_IF_METHOD_ALL(FIELD, __VA_ARGS__)(M_VAR1ANT_OPLIST_EMPLACE_TYPE,M_EAT)(name, __VA_ARGS__), \
   )
#else
#define M_VAR1ANT_OPLIST_P3(name, ...)                                        \
  (INIT(M_F(name,_init)),                                                     \
   INIT_SET(API_0P(M_F(name, _init_set))),                                    \
   SET(API_0P(M_F(name,_set))),                                               \
   CLEAR(API_0P(M_F(name, _clear))),                                          \
   RESET(API_0P(M_F(name, _reset))),                                          \
   NAME(name), TYPE(M_F(name,_ct)), GENTYPE(struct M_F(name,_s)*),            \
   EMPTY_P(M_F(name,_empty_p)),                                               \
   SIZE( M_NARGS(__VA_ARGS__) ),                                              \
   INIT_WITH( API_1(M_VAR1ANT_INIT_WITH) ),                                   \
   M_IF_METHOD_ALL(HASH, __VA_ARGS__)(HASH(M_F(name, _hash)),),               \
   M_IF_METHOD_ALL(EQUAL, __VA_ARGS__)(EQUAL(M_F(name, _equal_p)),),          \
   M_IF_METHOD_ALL(GET_STR, __VA_ARGS__)(GET_STR(API_0P(M_F(name, _get_str))),), \
   M_IF_METHOD2_ALL(PARSE_STR, INIT, __VA_ARGS__)(PARSE_STR(API_0P(M_F(name, _parse_str))),), \
   M_IF_METHOD2_ALL(IN_STR, INIT, __VA_ARGS__)(IN_STR(API_0P(M_F(name, _in_str))),), \
   M_IF_METHOD_ALL(OUT_STR, __VA_ARGS__)(OUT_STR(M_F(name, _out_str)),),      \
   M_IF_METHOD2_ALL(IN_SERIAL, INIT, __VA_ARGS__)(IN_SERIAL(API_0P(M_F(name, _in_serial))),), \
   M_IF_METHOD_ALL(OUT_SERIAL, __VA_ARGS__)(OUT_SERIAL(API_0P(M_F(name, _out_serial))),), \
   M_IF_METHOD_ALL(INIT_MOVE, __VA_ARGS__)(INIT_MOVE(M_F(name, _init_move)),), \
   M_IF_METHOD_ALL(INIT_MOVE, __VA_ARGS__)(MOVE(API_0P(M_F(name, _move))),),  \
   M_IF_METHOD_ALL(SWAP, __VA_ARGS__)(SWAP(M_F(name, _swap)),),               \
   M_IF_METHOD_ALL(FIELD, __VA_ARGS__)(M_VAR1ANT_OPLIST_EMPLACE_TYPE,M_EAT)(name, __VA_ARGS__), \
   )
#endif

/* Expand to an EMPLACE_TYPE compatible with OPLIST definition
   (Use M_REDUCE2 which is already used by M_IF_METHOD_ALL)
 */
#define M_VAR1ANT_OPLIST_EMPLACE_TYPE(name, ...) EMPLACE_TYPE( LIST(          \
 M_REDUCE2(M_VAR1ANT_OPLIST_EMPLACE_TYPE_EXPAND, M_ID, name, __VA_ARGS__)     \
                                                                   ))
#define M_VAR1ANT_OPLIST_EMPLACE_TYPE_EXPAND(name, oplist)                    \
  ( M_C(_, M_GET_FIELD oplist), M_C3(name, _init_set_, M_GET_FIELD oplist),   \
    M_C4(m_var1ant_, name, _type_, M_GET_FIELD oplist) )


/********************************** INTERNAL *********************************/

/* Macros for testing for method presence */
#define M_VAR1ANT_TEST_METHOD_P2(method, f, t, op)                            \
  M_TEST_METHOD_P(method, op)
#define M_VAR1ANT_TEST_METHOD_P(method, trio)                                 \
  M_APPLY(M_VAR1ANT_TEST_METHOD_P2, method, M_OPFLAT trio)
#define M_VAR1ANT_IF_ALL(method, ...)                                         \
  M_IF(M_REDUCE2(M_VAR1ANT_TEST_METHOD_P, M_AND, method, __VA_ARGS__))

#define M_VAR1ANT_TEST_METHOD2_P2(method1, method2, f, t, op)                 \
  M_AND(M_TEST_METHOD_P(method1, op), M_TEST_METHOD_P(method2, op))
#define M_VAR1ANT_TEST_METHOD2_P(method, trio)                                \
  M_APPLY(M_VAR1ANT_TEST_METHOD2_P2, M_PAIR_1 method, M_PAIR_2 method, M_OPFLAT trio)
#define M_VAR1ANT_IF_ALL2(method1, method2,  ...)                             \
  M_IF(M_REDUCE2(M_VAR1ANT_TEST_METHOD2_P, M_AND, (method1, method2), __VA_ARGS__))


/********************************** INTERNAL *********************************/

/* INIT_WITH macro using _Generic
   Expand as:
    _Generic ( (x),
        M_C4(m_var1ant_, name, _gentype_, num) : M_C4(m_var1ant_, name, _init_set_, num)(x, M_AS_TYPE() ),
        ...
    )
*/
#define M_VAR1ANT_INIT_WITH(oplist, dest, src)                                \
  M_VAR1ANT_INIT_WITH_B(M_GET_NAME oplist, M_GET_SIZE oplist, dest, src)
#define M_VAR1ANT_INIT_WITH_B(name, num, dest, src)                           \
  M_G3N_BEGIN_PROTECTED_CODE                                                  \
  _Generic( (src),                                                            \
  M_MAP2(M_VAR1ANT_INIT_WITH_FUNC, (name,dest,src), M_SEQ(1, num))            \
  struct m_var1ant_dummy_s *: /* cannot happen */ (void)0 )                   \
  M_G3N_END_PROTECTED_CODE
#define M_VAR1ANT_INIT_WITH_FUNC(triplet, num)                                \
  M_VAR1ANT_INIT_WITH_FUNC_EXPAND(M_TRIPLE_1 triplet, M_TRIPLE_2 triplet, M_TRIPLE_3 triplet, num)
#define M_VAR1ANT_INIT_WITH_FUNC_EXPAND(name, dest, src, num)                 \
  M_C4(m_var1ant_, name, _gentype_, num) : M_C4(m_var1ant_, name, _init_set_, num)(dest, M_AS_TYPE(M_C4(m_var1ant_, name, _gentype_, num), src) ), \
  M_C4(m_var1ant_, name, _const_gentype_, num): M_C4(m_var1ant_, name, _init_set_, num)(dest, M_AS_TYPE(M_C4(m_var1ant_, name, _const_gentype_, num), src) ),

/********************************** INTERNAL *********************************/

#if M_USE_SMALL_NAME
#define VARIANT_DEF2 M_VARIANT_DEF2
#define VARIANT_DEF2_AS M_VARIANT_DEF2_AS
#define VARIANT_OPLIST M_VARIANT_OPLIST
#endif

#endif
