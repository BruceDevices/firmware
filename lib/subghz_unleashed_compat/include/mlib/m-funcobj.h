/*
 * M*LIB - Function Object module
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
#ifndef MSTARLIB_FUNCOBJ_H
#define MSTARLIB_FUNCOBJ_H

#include "m-core.h"

/* Define a function object interface of name 'name'
 * with a function like retcode, type of param1, type of param 2, ...
 * USAGE:
 *    FUNC_OBJ_ITF_DEF(name, retcode type, type of param1, type of param 2, ...)
 */
#define M_FUNC_OBJ_ITF_DEF(name, ...)                                         \
  M_FUNC_OBJ_ITF_DEF_AS(name, M_F(name,_t), __VA_ARGS__)


/* Define a function object interface of name 'name'
 * as the given name name_t
 * USAGE:
 *    FUNC_OBJ_ITF_DEF_AS(name, name_t, retcode type, type of param1, type of param 2, ...)
 */
#define M_FUNC_OBJ_ITF_DEF_AS(name, name_t, ...)                              \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_IF_NARGS_EQ1(__VA_ARGS__)(M_FUNC0BJ_ITF_NO_PARAM_DEF, M_FUNC0BJ_ITF_PARAM_DEF)(name, name_t, __VA_ARGS__) \
  M_END_PROTECTED_CODE


/* Define a function object instance of name 'name' based on the interface 'base_name'
 * The function is defined using:
 * - the prototype of the inherited interface
 * - the parameters of the function are named as per the list param_list
 * - the core of the function given in 'callback_core'
 * - optionals member attributes of the function object can be defined after the core
 * (just like for tuple & variant: (name, type [, oplist])
 * 
 * In the core of the function, parameters are accessible just like a normal function.
 * A special variable named 'self' that refers to the function object itself
 * can be used to access member attributes using the syntax self->param1, ...
 * 
 * There shall be **exactly** the same number of parameters in 'param_list' than
 * the number of parameters of the interface 'base_name'
 *
 * USAGE/EXAMPLE:
 *   FUNC_OBJ_INS_DEF(name, base_name, (param1, ...), { return param1 * self->member1 }, (member1, int), ...)
 */
#define M_FUNC_OBJ_INS_DEF(name, base_name, param_list, ...)                  \
  M_FUNC_OBJ_INS_DEF_AS(name, M_F(name,_t), base_name, param_list, __VA_ARGS__)


/* Define a function object instance of name 'name' based on the interface 'base_name'
 * as the given name name_t.
 * See FUNC_OBJ_INS_DEF for additional details.
 * 
 * USAGE/EXAMPLE:
 *   FUNC_OBJ_INS_DEF_AS(name, name_t, base_name, (param1, ...), { return param1 * self->member1 }, (member1, int), ...)
 */
#define M_FUNC_OBJ_INS_DEF_AS(name, name_t, base_name, param_list, ...)       \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_IF_NARGS_EQ1(__VA_ARGS__)(M_FUNC0BJ_INS_NO_ATTR_DEF, M_FUNC0BJ_INS_ATTR_DEF)(name, name_t, base_name, param_list, __VA_ARGS__) \
  M_END_PROTECTED_CODE


/* OPLIST of the instanced function object
 * USAGE:
 *  FUNC_OBJ_INS_OPLIST(name, oplist of the attr1, ...)
 */
#define M_FUNC_OBJ_INS_OPLIST(...)                                            \
  M_IF_NARGS_EQ1(__VA_ARGS__)(M_FUNC0BJ_INS_NO_ATTR_OPLIST, M_FUNC0BJ_INS_ATTR_OPLIST_P1)( __VA_ARGS__)



/*****************************************************************************/
/******************************** INTERNAL ***********************************/
/*****************************************************************************/

/* To be used by M_IF_FUNCOBJ macro defined in m-core.
   NOTE: It is reversed (0 instead of 1) so that it can be used in M_IF reliably.
*/
#define M_FUNC0BJ_IS_NOT_DEFINED 0

/* Design Constraints:
 *  callback SHALL be the first member of the structures in all the definitions.
 *
 * Structure definitions are specialized in function of the presence or not
 * of parameters and/or attributes 
 * FIXME: How to factorize reasonably well between the definitions?
 */

/* Specialization of the OPLIST in function if there is at least one member or not */
#define M_FUNC0BJ_INS_NO_ATTR_OPLIST(name) (                                  \
  NAME(name),                                                                 \
  TYPE(M_F(name, _ct)), GENTYPE(struct M_F(name,_s)*),                        \
  CLEAR(M_F(name, _clear)),                                                   \
  INIT(M_F(name,_init))                                                       \
  )

/* Validate the oplist before going further */
#define M_FUNC0BJ_INS_ATTR_OPLIST_P1(name, ...)                               \
  M_IF(M_REDUCE(M_OPLIST_P, M_AND, __VA_ARGS__))(M_FUNC0BJ_INS_ATTR_OPLIST_P3, M_FUNC0BJ_INS_ATTR_OPLIST_FAILURE)(name, __VA_ARGS__)

/* Prepare a clean compilation failure */
#define M_FUNC0BJ_INS_ATTR_OPLIST_FAILURE(name, ...)                          \
  ((M_LIB_ERROR(ONE_ARGUMENT_OF_FUNC_OBJ_INS_OPLIST_IS_NOT_AN_OPLIST, name, __VA_ARGS__)))

/* Define the oplist of the instance */
#define M_FUNC0BJ_INS_ATTR_OPLIST_P3(name, ...) (                             \
  NAME(name),                                                                 \
  TYPE(M_F(name, _ct)), GENTYPE(struct M_F(name,_s)*),                        \
  INIT_WITH(M_F(name, _init_with)),                                           \
  CLEAR(M_F(name, _clear)),                                                   \
  M_IF_METHOD_ALL(INIT, __VA_ARGS__)(INIT(M_F(name,_init)),),                 \
  PROPERTIES(( LET_AS_INIT_WITH(1) ))                                         \
  )


/******************************** INTERNAL ***********************************/

/* Specialization of the definition a function object interface of name 'name'
 * with a function like "retcode (void)" that doesn't have any input parameters.
 * Define the following types to be used by instance:
 * - M_F(name, _retcode_ct): internal type of the return code
 * - M_F(name, _callback_ct): internal type of the callback.
 * - M_F(name, _ct): synonym of main type used by oplist.
 */
#define M_FUNC0BJ_ITF_NO_PARAM_DEF(name, interface_t, retcode)                \
                                                                              \
  /* Forward declaration */                                                   \
  struct M_F(name, _s);                                                       \
                                                                              \
  /* Internal type for instance */                                            \
  typedef retcode M_F(name, _retcode_ct);                                     \
  /* No parameters to the callback */                                         \
  typedef retcode(*M_F(name, _callback_ct))(struct M_F(name, _s) *);          \
                                                                              \
  typedef struct M_F(name, _s) {                                              \
     M_F(name, _callback_ct) callback;                                        \
  } *interface_t;                                                             \
                                                                              \
  /* Internal type for oplist & instance */                                   \
  typedef interface_t M_F(name, _ct);                                         \
                                                                              \
  M_INLINE retcode                                                            \
  M_F(name, _call)(interface_t funcobj)                                       \
  {                                                                           \
    M_IF(M_KEYWORD_P(void, retcode)) ( /* nothing */,return)                  \
      funcobj->callback(funcobj);                                             \
  }


/* Specialization of the definition a function object interface of name 'name'
 * with a function like retcode, type of param1, type of param 2, ...
 * with mandatory input parameters.
 * Define the following types to be used by instance:
 * - M_F(name, _retcode_ct): internal type of the return code
 * - M_F(name, _callback_ct): internal type of the callback.
 * - M_C4(name, _param_, num, _ct) for each parameter defined
 * - M_F(name, _ct): synonym of main type used by oplist.
 */
#define M_FUNC0BJ_ITF_PARAM_DEF(name, interface_t, retcode, ...)              \
                                                                              \
  /* Forward declaration */                                                   \
  struct M_F(name, _s);                                                       \
                                                                              \
  /* Internal types for instance */                                           \
  typedef retcode M_F(name, _retcode_ct);                                     \
  /* Define types for all parameters */                                       \
  M_MAP3(M_FUNC0BJ_BASE_TYPE, name, __VA_ARGS__)                              \
  /* Define callback type with all parameters */                              \
  typedef retcode(*M_F(name, _callback_ct))(struct M_F(name, _s) *, __VA_ARGS__); \
                                                                              \
  typedef struct M_F(name, _s) {                                              \
     M_F(name, _callback_ct) callback;                                        \
  } *interface_t;                                                             \
                                                                              \
  /* Internal type for oplist & instance */                                   \
  typedef interface_t M_F(name, _ct);                                         \
                                                                              \
  M_INLINE retcode                                                            \
  M_F(name, _call)(interface_t funcobj                                        \
                   M_MAP3(M_FUNC0BJ_BASE_ARGLIST, name, __VA_ARGS__) )        \
  {                                                                           \
    /* If the retcode is 'void', don't return the value of the callback */    \
    M_IF(M_KEYWORD_P(void, retcode)) ( /* nothing */,return)                  \
      funcobj->callback(funcobj M_MAP3(M_FUNC0BJ_BASE_ARGCALL, name, __VA_ARGS__) ); \
  }


/******************************** INTERNAL ***********************************/

/* Specialization of the definition a function object instance of name 'name'
 * with no member attribute.
 */
#define M_FUNC0BJ_INS_NO_ATTR_DEF(name, instance_t, base_name, param_list, callback_core) \
  typedef struct M_F(name, _s) {                                              \
    M_C(base_name, _callback_ct) callback;                                    \
  } instance_t[1];                                                            \
                                                                              \
  /* Internal type for oplist */                                              \
  typedef instance_t M_F(name, _ct);                                          \
                                                                              \
  M_INLINE M_C(base_name, _retcode_ct)                                        \
  M_F(name, _callback)(M_C(base_name, _ct) _self                              \
                      M_IF_EMPTY(M_OPFLAT param_list)(                        \
                          /* No param */,                                     \
                          M_MAP3(M_FUNC0BJ_INS_ARGLIST, base_name, M_OPFLAT param_list) \
                                                                    )         \
                      )                                                       \
  {                                                                           \
    struct M_F(name, _s) *self = (struct M_F(name, _s) *)_self;               \
    (void) self; /* maybe unused */                                           \
    callback_core;                                                            \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _init_with)(instance_t obj)                                       \
  {                                                                           \
    obj->callback = M_F(name, _callback);                                     \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _clear)(instance_t obj)                                           \
  {                                                                           \
    (void) obj; /* nothing to do */                                           \
  }                                                                           \
                                                                              \
  M_INLINE struct M_C(base_name, _s) *                                        \
  M_F(name, _as_interface)(instance_t obj)                                    \
  {                                                                           \
    return (struct M_C(base_name, _s) *) obj;                                 \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _init)(instance_t obj)                                            \
  {                                                                           \
    obj->callback = M_F(name, _callback);                                     \
  }                                                                           \
   

/* Specialization of the definition a function object instance of name 'name'
 * with mandatory member attribute.
 * First inject oplist in member attributes.
 */
#define M_FUNC0BJ_INS_ATTR_DEF(name, instance_t, base_name, param_list, callback_core, ...) \
  M_FUNC0BJ_INS_ATTR_DEF_P2(name, instance_t, base_name, param_list, callback_core, M_FUNC0BJ_INJECT_GLOBAL(__VA_ARGS__) )

/* Inject the oplist within the list of arguments */
#define M_FUNC0BJ_INJECT_GLOBAL(...)                                          \
  M_MAP_C(M_FUNC0BJ_INJECT_OPLIST_A, __VA_ARGS__)

/* Transform (x, type) into (x, type, oplist) if there is global registered oplist 
   or (x, type, M_BASIC_OPLIST) if there is no global one,
   or keep (x, type, oplist) if oplist was already present */
#define M_FUNC0BJ_INJECT_OPLIST_A( duo_or_trio )                              \
  M_FUNC0BJ_INJECT_OPLIST_B duo_or_trio
#define M_FUNC0BJ_INJECT_OPLIST_B( f, ... )                                   \
  M_IF_NARGS_EQ1(__VA_ARGS__)( (f, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)()), (f, __VA_ARGS__) )

// Test if all third argument of all arguments is an oplist
#define M_FUNC0BJ_IF_ALL_OPLIST(...)                                          \
  M_IF(M_REDUCE(M_FUNC0BJ_IS_OPLIST_P, M_AND, __VA_ARGS__))
// Test if the third argument is an oplist. a is a trio (name, type, oplist)
#define M_FUNC0BJ_IS_OPLIST_P(a)                                              \
  M_OPLIST_P(M_RET_ARG3 a)

/* Validate the oplist before going further */
#define M_FUNC0BJ_INS_ATTR_DEF_P2(name, instance_t, base_name, param_list, callback_core, ...) \
  M_FUNC0BJ_IF_ALL_OPLIST(__VA_ARGS__)(M_FUNC0BJ_INS_ATTR_DEF_P3, M_FUNC0BJ_INS_ATTR_DEF_FAILURE)(name, instance_t, base_name, param_list, callback_core, __VA_ARGS__)

/* Stop processing with a compilation failure */
#define M_FUNC0BJ_INS_ATTR_DEF_FAILURE(name, instance_t, base_name, param_list, callback_core, ...) \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST, "(FUNC_OBJ_INS_DEF): at least one of the given argument is not a valid oplist: " #__VA_ARGS__)

/* Expand the Function Object with members */
#define M_FUNC0BJ_INS_ATTR_DEF_P3(name, instance_t, base_name, param_list, callback_core, ...) \
  typedef struct M_F(name, _s) {                                              \
    /* Callback is the mandatory first argument */                            \
    M_C(base_name, _callback_ct) callback;                                    \
    /* All the member attribute of the Function Object */                     \
    M_MAP(M_FUNC0BJ_INS_ATTR_STRUCT, __VA_ARGS__)                             \
   } instance_t[1];                                                           \
                                                                              \
  /* Internal type for oplist */                                              \
  typedef instance_t M_F(name, _ct);                                          \
                                                                              \
  M_FUNC0BJ_CONTROL_ALL_OPLIST(name, __VA_ARGS__)                             \
                                                                              \
  M_INLINE M_C(base_name, _retcode_ct)                                        \
  M_F(name, _callback)(M_C(base_name, _ct) _self                              \
                      M_IF_EMPTY(M_OPFLAT param_list)(                        \
                        /* No param */,                                       \
                        M_MAP3(M_FUNC0BJ_INS_ARGLIST, base_name, M_OPFLAT param_list) \
                                                                    )         \
                      )                                                       \
  {                                                                           \
    /* Let's go through an uintptr_t to avoid [broken] aliasing detection by compiler */ \
    uintptr_t __self = (uintptr_t) _self;                                     \
    struct M_F(name, _s) *self = (struct M_F(name, _s) *)(void*)__self;       \
    (void) self; /* maybe unused */                                           \
    callback_core;                                                            \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _init_with)(instance_t obj M_MAP(M_FUNC0BJ_INS_ATTR_LIST, __VA_ARGS__)) \
  {                                                                           \
    obj->callback = M_F(name, _callback);                                     \
    M_MAP(M_FUNC0BJ_INS_ATTR_INIT_SET, __VA_ARGS__);                          \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _clear)(instance_t obj)                                           \
  {                                                                           \
    M_MAP(M_FUNC0BJ_INS_ATTR_CLEAR, __VA_ARGS__);                             \
  }                                                                           \
                                                                              \
  M_INLINE struct M_C(base_name, _s) *                                        \
  M_F(name, _as_interface)(instance_t obj)                                    \
  {                                                                           \
    return (struct M_C(base_name, _s) *) obj;                                 \
  }                                                                           \
                                                                              \
  M_IF(M_FUNC0BJ_TEST_METHOD_P(INIT, __VA_ARGS))                              \
  (                                                                           \
  M_INLINE void                                                               \
  M_F(name, _init)(instance_t obj)                                            \
  {                                                                           \
    obj->callback = M_F(name, _callback);                                     \
    M_MAP(M_FUNC0BJ_INS_ATTR_INIT, __VA_ARGS__);                              \
  }                                                                           \
  , /* END OF INIT METHOD */ )                                                \



/* Define a numbered type of a parameter of the callback*/
#define M_FUNC0BJ_BASE_TYPE(name, num, type)                                  \
  typedef type M_C4(name, _param_, num, _ct);

/* Define a list of the type of arguments for a function definition */
#define M_FUNC0BJ_BASE_ARGLIST(name, num, type)                               \
  M_DEFERRED_COMMA type M_C(param_, num)

/* Define a list of arguments for a function call */
#define M_FUNC0BJ_BASE_ARGCALL(name, num, type)                               \
  M_DEFERRED_COMMA M_C(param_, num)


/* Helper macros */
/* arg = (name, type [, oplist]) */
#define M_FUNC0BJ_INS_ATTR_STRUCT(arg)                                        \
  M_RET_ARG2 arg M_RET_ARG1 arg;

#define M_FUNC0BJ_INS_ATTR_LIST(arg)                                          \
  M_DEFERRED_COMMA M_RET_ARG2 arg const M_RET_ARG1 arg

#define M_FUNC0BJ_INS_ATTR_INIT(arg)                                          \
  M_CALL_INIT(M_RET_ARG3 arg, obj -> M_RET_ARG1 arg);

#define M_FUNC0BJ_INS_ATTR_INIT_SET(arg)                                      \
  M_CALL_INIT_SET(M_RET_ARG3 arg, obj -> M_RET_ARG1 arg, M_RET_ARG1 arg);

#define M_FUNC0BJ_INS_ATTR_CLEAR(arg)                                         \
  M_CALL_CLEAR(M_RET_ARG3 arg, obj -> M_RET_ARG1 arg);

/* Define the list of arguments of the instance of the callback */
#define M_FUNC0BJ_INS_ARGLIST(name, num, param)                               \
  M_DEFERRED_COMMA M_C4(name, _param_, num, _ct) param


/* Macros for testing for a method presence in all the attributes */
#define M_FUNC0BJ_TEST_METHOD2_P(method, op)                                  \
  M_TEST_METHOD_P(method, op)
#define M_FUNC0BJ_TEST_METHOD1_P(method, arg)                                 \
  M_APPLY(M_FUNC0BJ_TEST_METHOD2_P, method, M_RET_ARG3 arg)
#define M_FUNC0BJ_TEST_METHOD_P(method, ...)                                  \
  M_IF(M_REDUCE2(M_FUNC0BJ_TEST_METHOD1_P, M_AND, method, __VA_ARGS__))

/* Macro for checking compatible type and oplist for all the attributes */
#define M_FUNC0BJ_CONTROL_ALL_OPLIST(name, ...)                               \
  M_MAP2(M_FUNC0BJ_CONTROL_OPLIST, name, __VA_ARGS__)
#define M_FUNC0BJ_CONTROL_OPLIST(name, a)                                     \
  M_CHECK_COMPATIBLE_OPLIST(name, M_RET_ARG1 a, M_RET_ARG2 a, M_RET_ARG3 a)


/******************************** INTERNAL ***********************************/

#if M_USE_SMALL_NAME
#define FUNC_OBJ_ITF_DEF M_FUNC_OBJ_ITF_DEF
#define FUNC_OBJ_ITF_DEF_AS M_FUNC_OBJ_ITF_DEF_AS
#define FUNC_OBJ_INS_DEF M_FUNC_OBJ_INS_DEF
#define FUNC_OBJ_INS_DEF_AS M_FUNC_OBJ_INS_DEF_AS
#define FUNC_OBJ_INS_OPLIST M_FUNC_OBJ_INS_OPLIST
#endif

#endif
