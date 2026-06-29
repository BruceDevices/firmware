/*
 * M*LIB - ALGO module
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
#ifndef MSTARLIB_ALGO_H
#define MSTARLIB_ALGO_H

#include "m-core.h"

/* Define different kind of basic algorithms named 'name' over the container
   which oplist is 'cont_oplist'.
   USAGE:
   ALGO_DEF(algogName, containerOplist|type if oplist has been registered) */
#define M_ALGO_DEF(name, cont_oplist)                                         \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_ALG0_DEF_P1(name, M_GLOBAL_OPLIST(cont_oplist))                           \
  M_END_PROTECTED_CODE


/* Map a function (or a macro) to all elements of a container.
   USAGE:
   ALGO_FOR_EACH(container, containerOplist|type_if_registered_oplist, function[, extra arguments of function]) */
#define M_ALGO_FOR_EACH(container, cont_oplist, ...)                          \
  M_IF_NARGS_EQ1(__VA_ARGS__)                                                 \
  (M_ALG0_FOR_EACH, M_ALG0_FOR_EACH_ARG)                                      \
  (container, M_GLOBAL_OPLIST(cont_oplist), __VA_ARGS__)


/* Map a function (or a macro) to all elements of a container
   and store it into another container.
   USAGE:
   ALGO_TRANSFORM(contDst, contDOplist|type_if_registered_oplist, contSrc, contSrcOplist|type_if_registered_oplist,
                  function[, extra arguments of function]) */
#define M_ALGO_TRANSFORM(contD, contDop, contS, contSop, ...)                 \
  M_IF_NARGS_EQ1(__VA_ARGS__)                                                 \
  (M_ALG0_TRANSFORM, M_ALG0_TRANSFORM_ARG)                                    \
  (contD, M_GLOBAL_OPLIST(contDop), contS, M_GLOBAL_OPLIST(contSop), __VA_ARGS__)


/* Extract a subset of a container to copy into another container.
   USAGE:
   ALGO_EXTRACT(contDst, contDstOplist|type_if_registered_oplist, contSrc, contSrcOplist|type_if_registered_oplist
               [, function [, extra arguments of function]])  */
#define M_ALGO_EXTRACT(contD, contDop, contS, ...)                            \
  M_IF_NARGS_EQ1(__VA_ARGS__)                                                 \
  (M_ALG0_EXTRACT,                                                            \
   M_IF_NARGS_EQ2(__VA_ARGS__)(M_ALG0_EXTRACT_FUNC, M_ALG0_EXTRACT_ARG))      \
  (contD, contDop, contS,  __VA_ARGS__)


/* Perform a Reduce operation over a container.
   USAGE:
   ALGO_REDUCE(dstVar, container, contOplist|type_if_registered_oplist, reduceFunc
               [, mapFunc[, extraParameters of map function]])
   or
   ALGO_REDUCE( (dstVar, dstOplist|type_if_registered_oplist), container, contOplist|type_if_registered_oplist, reduceFunc
               [, mapFunc[, extraParameters of map function]])
  if the destination variable is not of the same type than the elements of the containers.
*/
#define M_ALGO_REDUCE(dest, cont, contOp, ...)                                \
  M_IF(M_PARENTHESIS_P(dest))                                                 \
  (M_ALG0_REDUCE_DISPATCH(M_PAIR_1 dest, M_GLOBAL_OPLIST(M_PAIR_2 dest), M_GLOBAL_TYPE(M_PAIR_2 dest),cont, M_GLOBAL_OPLIST(contOp), __VA_ARGS__), \
   M_ALG0_REDUCE_DISPATCH(dest, M_GET_OPLIST M_GLOBAL_OPLIST(contOp), M_GET_SUBTYPE M_GLOBAL_OPLIST(contOp), cont, contOp, __VA_ARGS__)) \
    

/* Insert into the container 'contDst' at position 'position' all the values
   of container 'contSrc'.
   USAGE:
   ALGO_INSERT_AT(containerDst, containerDstOPLIST|type_if_registered_oplist, containerDstIterator, containerSrc, containerSrcOPLIST|type_if_registered_oplist)
 */
#define M_ALGO_INSERT_AT(contDst, contDstOp, position, contSrc, contSrcOp)    \
  M_ALG0_INSERT_AT(contDst, M_GLOBAL_OPLIST(contDstOp), position, contSrc, M_GLOBAL_OPLIST(contSrcOp) )


/*****************************************************************************/
/******************************** INTERNAL ***********************************/
/*****************************************************************************/

/* Try to expand the algorithms */
#define M_ALG0_DEF_P1(name, cont_oplist)                                      \
  M_ALG0_DEF_P2(name, M_GET_TYPE cont_oplist, cont_oplist,                    \
               M_GET_SUBTYPE cont_oplist, M_GET_OPLIST cont_oplist,           \
               M_GET_IT_TYPE cont_oplist)

/* First validate the first oplist */
#define M_ALG0_DEF_P2(name, container_t, cont_oplist, type_t, type_oplist, it_t) \
  M_IF_OPLIST(cont_oplist)(M_ALG0_DEF_P3, M_ALG0_DEF_FAILURE)(name, container_t, cont_oplist, type_t, type_oplist, it_t)

/* Then validate the second oplist */
#define M_ALG0_DEF_P3(name, container_t, cont_oplist, type_t, type_oplist, it_t) \
  M_IF_OPLIST(type_oplist)(M_ALG0_DEF_P4, M_ALG0_DEF_FAILURE)(name, container_t, cont_oplist, type_t, type_oplist, it_t)

/* Stop processing with a compilation failure if an oplist was invalid */
#define M_ALG0_DEF_FAILURE(name, container_t, cont_oplist, type_t, type_oplist, it_t) \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST, "(ALGO_DEF): one of the given argument is not a valid oplist: " M_AS_STR(cont_oplist) " / " M_AS_STR(type_oplist) )


/* Expand all algorithms */
#define M_ALG0_DEF_P4(name, container_t, cont_oplist, type_t, type_oplist, it_t) \
                                                                              \
  M_CHECK_COMPATIBLE_OPLIST(name, 10, container_t, cont_oplist)               \
  M_CHECK_COMPATIBLE_OPLIST(name, 11, type_t, type_oplist)                    \
                                                                              \
  M_ALG0_DEF_CALLBACK(name, container_t, cont_oplist, type_t, type_oplist, it_t) \
  M_IF_FUNCOBJ(M_ALG0_DEF_FUNCOBJ(name, container_t, cont_oplist, type_t, type_oplist, it_t)) \
                                                                              \
  M_IF_METHOD(EQUAL, type_oplist)(                                            \
  M_ALG0_DEF_FIND(name, container_t, cont_oplist, type_t, type_oplist, it_t)  \
  , /* NO EQUAL */)                                                           \
                                                                              \
  M_ALG0_DEF_FIND_IF(name, container_t, cont_oplist, type_t, type_oplist, it_t, \
                     if, M_F(name,_test_cb_ct), M_F(name,_eq_cb_ct), M_APPLY, M_APPLY) \
  M_IF_FUNCOBJ(M_ALG0_DEF_FIND_IF(name, container_t, cont_oplist, type_t, type_oplist, it_t, \
                                  fo, M_F(name,_test_obj_t), M_F(name,_eq_obj_t), M_F(name, _test_obj_call), M_F(name, _eq_obj_call))) \
                                                                              \
  M_ALG0_DEF_MAP(name, container_t, cont_oplist, type_t, type_oplist, it_t)   \
                                                                              \
  M_ALG0_DEF_ALL_OF(name, container_t, cont_oplist, type_t, type_oplist, it_t, \
                    _, M_F(name,_test_cb_ct), M_APPLY)                        \
  M_IF_FUNCOBJ(M_ALG0_DEF_ALL_OF(name, container_t, cont_oplist, type_t, type_oplist, it_t, \
                                 _fo_, M_F(name,_test_obj_t), M_F(name,_test_obj_call)) ) \
                                                                              \
  /* If there is a IT_REF method, we consider the container as modifiable through iterator */ \
  M_IF_METHOD(IT_REF, cont_oplist)(                                           \
  M_ALG0_DEF_FILL(name, container_t, cont_oplist, type_t, type_oplist, it_t)  \
  M_ALG0_DEF_VECTOR(name, container_t, cont_oplist, type_t, type_oplist, it_t)\
                                                                              \
  M_IF_METHOD(CMP, type_oplist)(                                              \
  M_ALG0_DEF_MINMAX(name, container_t, cont_oplist, type_t, type_oplist, it_t) \
  M_ALG0_DEF_SORT(name, container_t, cont_oplist, type_t, type_oplist, it_t, +, _sort) \
  M_ALG0_DEF_SORT(name, container_t, cont_oplist, type_t, type_oplist, it_t, -, _sort_dsc) \
  M_IF_METHOD(IT_REMOVE, cont_oplist)(                                        \
  M_ALG0_DEF_REMOVE(name, container_t, cont_oplist, type_t, type_oplist, it_t)\
  , /* No IT_REMOVE method */)                                                \
  , /* No CMP method */)                                                      \
                                                                              \
  M_IF_FUNCOBJ(M_ALG0_DEF_SORT_AUX(name, container_t, cont_oplist, type_t, type_oplist, it_t, \
                                  _sort_fo, M_ALG0_SORT_CALL_OBJ_P4, M_ALG0_SORT_PARAM_OBJ_P4, M_ALG0_SORT_ARG_OBJ_P4) ) \
  , /* No IT_REF method */)                                                   \
                                                                              \
  M_IF_METHOD(EXT_ALGO, type_oplist)(                                         \
  M_GET_EXT_ALGO type_oplist (name, cont_oplist, type_oplist)                 \
  , /* No EXT_ALGO method */ )                                                \


/* Define the types of the callbacks associated to the algorithms.
 * Types remain internal */
#define M_ALG0_DEF_CALLBACK(name, container_t, cont_oplist, type_t, type_oplist, it_t) \
                                                                              \
  typedef bool (*M_F(name, _test_cb_ct))(type_t const);                       \
  typedef bool (*M_F(name, _eq_cb_ct))(type_t const, type_t const);           \
  typedef int  (*M_F(name, _cmp_cb_ct))(type_t const, type_t const);          \
  typedef bool (*M_F(name, _transform_cb_ct))(type_t *, type_t const, void*); \
  typedef void (*M_F(name, _reduce_cb_ct))(type_t *, type_t const);           \
  typedef void (*M_F(name, _apply_cb_ct))(type_t M_IF_METHOD(IT_REF, cont_oplist)( , const)  );


/* Define the function objects associated to the algorithms.
 * Created Function objects are part of the public interface */
#define M_ALG0_DEF_FUNCOBJ(name, container_t, cont_oplist, type_t, type_oplist, it_t) \
                                                                              \
  M_FUNC_OBJ_ITF_DEF(M_F(name, _test_obj), bool, type_t const)                \
  M_FUNC_OBJ_ITF_DEF(M_F(name, _eq_obj), bool, type_t const, type_t const )   \
  M_FUNC_OBJ_ITF_DEF(M_F(name, _cmp_obj), int, type_t const, type_t const )   \
  M_FUNC_OBJ_ITF_DEF(M_F(name, _transform_obj), bool, type_t *, type_t const )\
  M_FUNC_OBJ_ITF_DEF(M_F(name, _apply_obj), void, type_t * )                  \
  

/* Define the sort functions with the CMP operator using the order selected */
#define M_ALG0_DEF_SORT(name, container_t, cont_oplist, type_t, type_oplist, it_t, order, sort_name) \
                                                                              \
  /* Define the encapsulation function that perform the selected order */     \
  M_INLINE int M_C3(name,sort_name,_cmp)(type_t const*a,type_t const*b)       \
  {                                                                           \
    return order M_CALL_CMP(type_oplist, *a, *b);                             \
  }                                                                           \
                                                                              \
  M_ALG0_DEF_SORT_AUX(name, container_t, cont_oplist, type_t, type_oplist, it_t, sort_name, M_ALG0_SORT_CALL_CMP_P4, M_EAT, /*empty*/ )

// Call the comparaison function of the type oplist (Using CMP operator)
#define M_ALG0_SORT_CALL_CMP_P4(name, sort_name, ref1, ref2)                  \
  M_C3(name, sort_name,_cmp)(ref1, ref2)


// Call the created function object
#define M_ALG0_SORT_CALL_OBJ_P4(name, sort_name, ref1, ref2)                  \
  M_F(name, _cmp_obj_call)(funcobj, *ref1, *ref2)
// Adding a parameter named 'funcobj' to the algorithm functions
#define M_ALG0_SORT_PARAM_OBJ_P4(name) M_DEFERRED_COMMA M_F(name, _cmp_obj_t) funcobj
#define M_ALG0_SORT_ARG_OBJ_P4 M_DEFERRED_COMMA funcobj


/* Define the sort funtions using either the CMP operator or a function object
  - name: prefix of algorithms
  - container_t: type of the container
  - cont_oplist: oplist of the container
  - type_t: type of the data within the container
  - type_oplist: oplist of such type
  - it_t: type of the iterator of the container
  - sort_name: suffix used for creating the sort functions
  - cmp_func: macro to call to get the CMP order
  - cmp_param: macro to use to generate an extra argument for the function (needed for function object).
  It is needed to add another argument to the function.
  - cmp_arg: Name of such argument.
 */
#define M_ALG0_DEF_SORT_AUX(name, container_t, cont_oplist, type_t, type_oplist, it_t, sort_name, cmp_func, cmp_param, cmp_arg) \
                                                                              \
  /* Test if the container is sorted */                                       \
  M_INLINE bool                                                               \
  M_C3(name,sort_name,_p)(const container_t l cmp_param(name))                \
  {                                                                           \
    it_t it1;                                                                 \
    it_t it2;                                                                 \
    /* Linear comparaison of TAB[N] & TAB[N+1] to test if the order is correct */ \
    M_CALL_IT_FIRST(cont_oplist, it1, l);                                     \
    M_CALL_IT_SET(cont_oplist, it2, it1);                                     \
    M_CALL_IT_NEXT(cont_oplist, it2);                                         \
    while (!M_CALL_IT_END_P(cont_oplist, it2)) {                              \
      type_t const *ref1 = M_CALL_IT_CREF(cont_oplist, it1);                  \
      type_t const *ref2 = M_CALL_IT_CREF(cont_oplist, it2);                  \
      if (!(cmp_func(name, sort_name, ref1, ref2) <= 0)) {                    \
        return false;                                                         \
      }                                                                       \
      M_CALL_IT_SET(cont_oplist, it1, it2);                                   \
      M_CALL_IT_NEXT(cont_oplist, it2);                                       \
    }                                                                         \
    return true;                                                              \
  }                                                                           \
                                                                              \
  /* Sort function. It can be generated from 3 algorithms: */                 \
  /*  - a specialized version defined by the container */                     \
  /*  - an unstable merge sort (need 'splice_back' method) */                 \
  /*  - an insertion sort (need 'previous' method) */                         \
  /*  - a selection sort */                                                   \
  M_IF(M_AND(M_TEST_METHOD_P(SORT, cont_oplist), M_EMPTY_P(cmp_arg)))(        \
    /******** OPTIMIZED SORT FOR CONTAINER *********/                         \
  M_INLINE void M_F(name,sort_name)(container_t l)                            \
  {                                                                           \
    M_CALL_SORT(cont_oplist, l, M_C3(name, sort_name,_cmp));                  \
  }                                                                           \
  ,                                                                           \
                                                                              \
  M_IF_METHOD2(SPLICE_BACK, SPLICE_AT, cont_oplist)(                          \
  /******** MERGE SORT (unstable) ********/                                   \
  /* NOTE: Only reasonable for lists (To move in m-list.h ?) */               \
                                                                              \
  /* Split the container 'l' into near even size l1 and l2 *                  \
     using odd/even splitting  */                                             \
  M_INLINE void                                                               \
  M_C3(name,sort_name,_split)(container_t l1, container_t l2, container_t l)  \
  {                                                                           \
    it_t it;                                                                  \
    bool b = false;                                                           \
    /* Split 'l' into 'l1' and 'l2' */                                        \
    for (M_CALL_IT_FIRST(cont_oplist,it, l);                                  \
         !M_CALL_IT_END_P(cont_oplist, it);) {                                \
      M_CALL_SPLICE_BACK(cont_oplist, (b ? l1 : l2), l, it);                  \
      b = !b;                                                                 \
    }                                                                         \
    M_ASSERT(M_CALL_EMPTY_P (cont_oplist, l));                                \
  }                                                                           \
                                                                              \
  /* Merge in empty 'l' the sorted container 'l1' and 'l2' */                 \
  M_INLINE void                                                               \
  M_C3(name,sort_name,_merge)(container_t l, container_t l1, container_t l2 cmp_param(name)) \
  {                                                                           \
    M_ASSERT(M_CALL_EMPTY_P (cont_oplist, l));                                \
    it_t it;                                                                  \
    it_t it1;                                                                 \
    it_t it2;                                                                 \
    M_CALL_IT_END(cont_oplist, it, l);                                        \
    M_CALL_IT_FIRST(cont_oplist,it1, l1);                                     \
    M_CALL_IT_FIRST(cont_oplist,it2, l2);                                     \
    while (true) {                                                            \
      /* Compare current elements of the containers l1 and l2 */              \
      int c = cmp_func(name, sort_name, M_CALL_IT_CREF(cont_oplist, it1),     \
           M_CALL_IT_CREF(cont_oplist, it2));                                 \
      if (c <= 0) {                                                           \
        /* Move the element of l1 in the new container */                     \
        M_CALL_SPLICE_AT(cont_oplist, l, it, l1, it1);                        \
        if (M_UNLIKELY (M_CALL_IT_END_P(cont_oplist, it1))) {                 \
          /* Move all remaining elements of l2 in 'l' */                      \
          while (!M_CALL_IT_END_P(cont_oplist, it2)) {                        \
            M_CALL_SPLICE_AT(cont_oplist, l, it, l2, it2);                    \
          }                                                                   \
          return;                                                             \
        }                                                                     \
      } else {                                                                \
        /* Move the element of l2 in the new container */                     \
        M_CALL_SPLICE_AT(cont_oplist, l, it, l2, it2);                        \
        if (M_UNLIKELY (M_CALL_IT_END_P(cont_oplist, it2))) {                 \
          /* Move all remaining elements of l1 in 'l' */                      \
          while (!M_CALL_IT_END_P(cont_oplist, it1)) {                        \
            M_CALL_SPLICE_AT(cont_oplist, l, it, l1, it1);                    \
          }                                                                   \
          return;                                                             \
        }                                                                     \
      }                                                                       \
    }                                                                         \
    /* Cannot occur */                                                        \
  }                                                                           \
                                                                              \
  /* Sort the container 'l' */                                                \
  M_INLINE void                                                               \
  M_F(name,sort_name)(container_t l cmp_param(name))                          \
  {                                                                           \
    container_t l1;                                                           \
    container_t l2;                                                           \
    it_t it;                                                                  \
    it_t it1;                                                                 \
    it_t it2;                                                                 \
    /* First deal with 0, 1, or 2 size container */                           \
    M_CALL_IT_FIRST(cont_oplist, it, l);                                      \
    if (M_UNLIKELY (M_CALL_IT_END_P(cont_oplist, it)))                        \
      return;                                                                 \
    M_CALL_IT_SET(cont_oplist, it1, it);                                      \
    M_CALL_IT_NEXT(cont_oplist, it);                                          \
    if (M_UNLIKELY (M_CALL_IT_END_P(cont_oplist, it)))                        \
      return;                                                                 \
    M_CALL_IT_SET(cont_oplist, it2, it);                                      \
    M_CALL_IT_NEXT(cont_oplist, it);                                          \
    if (M_UNLIKELY (M_CALL_IT_END_P(cont_oplist, it))) {                      \
      /* Two elements */                                                      \
      int c = cmp_func(name, sort_name,                                       \
           M_CALL_IT_CREF(cont_oplist, it1),                                  \
           M_CALL_IT_CREF(cont_oplist, it2));                                 \
      if (c > 0) {                                                            \
        /* SWAP */                                                            \
        M_CALL_SPLICE_BACK(cont_oplist, l, l, it2);                           \
      }                                                                       \
      return;                                                                 \
    }                                                                         \
    /* Container length is greater than 2: split, sort & merge */             \
    M_GLOBAL_CONTEXT();                                                       \
    M_CALL_INIT(cont_oplist, l1);                                             \
    M_CALL_INIT(cont_oplist, l2);                                             \
    M_C3(name,sort_name,_split)(l1, l2, l);                                   \
    M_F(name,sort_name)(l1 cmp_arg);                                          \
    M_F(name,sort_name)(l2 cmp_arg);                                          \
    M_C3(name,sort_name,_merge)(l, l1, l2 cmp_arg);                           \
    /* l1 & l2 shall be empty now */                                          \
    M_ASSERT( M_CALL_EMPTY_P(cont_oplist, l1));                               \
    M_ASSERT( M_CALL_EMPTY_P(cont_oplist, l2));                               \
    /* Normally nothing is done here */                                       \
    M_CALL_CLEAR(cont_oplist, l2);                                            \
    M_CALL_CLEAR(cont_oplist, l1);                                            \
  }                                                                           \
                                        ,                                     \
  M_IF_METHOD(IT_PREVIOUS, cont_oplist)(                                      \
    /******** GENERIC INSERTION SORT *********/                               \
  M_INLINE void M_F(name,sort_name)(container_t l cmp_param(name))            \
  {                                                                           \
    it_t it1;                                                                 \
    it_t it2;                                                                 \
    it_t it2p1;                                                               \
    /* NOTE: Do not use SET, this is a MOVE operation */                      \
    /* Start from i := 1 */                                                   \
    M_CALL_IT_FIRST(cont_oplist, it1, l);                                     \
    M_CALL_IT_NEXT(cont_oplist, it1);                                         \
    while (!M_CALL_IT_END_P(cont_oplist, it1) ) {                             \
      type_t x;                                                               \
      /* x := TAB[i] */                                                       \
      memcpy (&x, M_CALL_IT_CREF(cont_oplist, it1), sizeof (type_t));         \
      /* j := i -1 // jp1 := i (= j +1) */                                    \
      M_CALL_IT_SET(cont_oplist, it2, it1);                                   \
      M_CALL_IT_PREVIOUS(cont_oplist, it2);                                   \
      M_CALL_IT_SET(cont_oplist, it2p1, it1);                                 \
      while (!M_CALL_IT_END_P(cont_oplist, it2)                               \
             && !(cmp_func(name, sort_name,                                   \
                           M_CALL_IT_CREF(cont_oplist, it2),                  \
                           M_CONST_CAST(type_t, &x)) <= 0)) {                 \
        /* TAB[jp1=j+1] := TAB[j] */                                          \
        memcpy(M_CALL_IT_REF(cont_oplist, it2p1),                             \
               M_CALL_IT_CREF(cont_oplist, it2), sizeof (type_t) );           \
        /* jp1 := j (= jp1-1) */                                              \
        M_CALL_IT_SET(cont_oplist, it2p1, it2);                               \
        M_CALL_IT_PREVIOUS(cont_oplist, it2);                                 \
      }                                                                       \
      /* TAB[jp1] := x */                                                     \
      memcpy(M_CALL_IT_REF(cont_oplist, it2p1), &x, sizeof (type_t) );        \
      /* i := i + 1 */                                                        \
      M_CALL_IT_NEXT(cont_oplist, it1);                                       \
    }                                                                         \
  }                                                                           \
                                                                              \
  ,                                                                           \
  /********** GENERIC SELECTION SORT ************/                            \
  M_INLINE void M_F(name,sort_name)(container_t l cmp_param(name))            \
  {                                                                           \
    it_t it1;                                                                 \
    it_t it2;                                                                 \
    for(M_CALL_IT_FIRST(cont_oplist, it1, l);                                 \
        !M_CALL_IT_LAST_P(cont_oplist, it1);                                  \
        M_CALL_IT_NEXT(cont_oplist, it1))  {                                  \
      it_t it_min;                                                            \
      M_CALL_IT_SET(cont_oplist, it_min, it1);                                \
      M_CALL_IT_SET(cont_oplist, it2, it1);                                   \
      for(M_CALL_IT_NEXT(cont_oplist, it2) ;                                  \
          !M_CALL_IT_END_P(cont_oplist, it2);                                 \
          M_CALL_IT_NEXT(cont_oplist, it2)) {                                 \
        if (cmp_func(name, sort_name, M_CALL_IT_CREF(cont_oplist, it2),       \
                     M_CALL_IT_CREF(cont_oplist, it_min)) < 0) {              \
                     M_CALL_IT_SET(cont_oplist, it_min, it2);                 \
        }                                                                     \
      }                                                                       \
      if (M_CALL_IT_EQUAL_P(cont_oplist, it_min, it1) == false) {             \
        /* TODO: Use SWAP method of type_oplist if available  */              \
        type_t x; /* Do not use SET, as it is a MOVE operation */             \
        memcpy (&x, M_CALL_IT_CREF(cont_oplist, it1), sizeof (type_t));       \
        memcpy (M_CALL_IT_REF(cont_oplist, it1),                              \
                M_CALL_IT_CREF(cont_oplist, it_min), sizeof (type_t));        \
        memcpy (M_CALL_IT_REF(cont_oplist, it_min), &x, sizeof (type_t));     \
      }                                                                       \
    }                                                                         \
  }                                                                           \
                                                                        ) /* IF IT_PREVIOUS METHOD */ \
                                                                        ) /* SPLICE BACK METHOD */ \
                                                                        ) /* IF SORT METHOD */ \
  /* Compute the union of two ***sorted*** containers  */                     \
  M_IF_METHOD(IT_INSERT, cont_oplist)(                                        \
  M_INLINE void                                                               \
  M_C3(name,sort_name,_union)(M_P_EXPAND container_t dst, const container_t src cmp_param(name)) \
  {                                                                           \
    it_t itSrc;                                                               \
    it_t itDst;                                                               \
    it_t itIns;                                                               \
    M_ASSERT(M_C3(name,sort_name,_p)(dst cmp_arg));                           \
    M_ASSERT(M_C3(name,sort_name,_p)(src cmp_arg));                           \
    /* Iterate over both dst & src containers */                              \
    M_CALL_IT_FIRST(cont_oplist, itSrc, src);                                 \
    M_CALL_IT_FIRST(cont_oplist, itDst, dst);                                 \
    M_CALL_IT_END(cont_oplist, itIns, dst);                                   \
    while (!M_CALL_IT_END_P(cont_oplist, itSrc)                               \
           && !M_CALL_IT_END_P(cont_oplist, itDst)) {                         \
      type_t const *objSrc = M_CALL_IT_CREF(cont_oplist, itSrc);              \
      type_t const *objDst = M_CALL_IT_CREF(cont_oplist, itDst);              \
      /* Compare the current element of src and dst */                        \
      int cmp = cmp_func(name, sort_name, objDst, objSrc);                    \
      if (cmp <= 0) {                                                         \
        /* The element of dst is before. Go to next element of dst */         \
        M_CALL_IT_SET(cont_oplist, itIns, itDst);                             \
        M_CALL_IT_NEXT(cont_oplist, itDst);                                   \
        if (cmp == 0) {                                                       \
          /* Skip same arguments in both lists */                             \
          M_CALL_IT_NEXT(cont_oplist, itSrc);                                 \
        }                                                                     \
      } else {                                                                \
        /* The element of src is before. */                                   \
        /* insert objSrc before */                                            \
        /* NOTE: IT_INSERT insert after ==> Need of another iterator */       \
        M_CALL_IT_INSERT(cont_oplist, dst, itIns, *objSrc);                   \
        M_CALL_IT_NEXT(cont_oplist, itSrc);                                   \
      }                                                                       \
    }                                                                         \
    while (!M_CALL_IT_END_P(cont_oplist, itSrc)) {                            \
      /* Finish inserting the element of src in dst */                        \
      type_t *objSrc = M_CALL_IT_REF(cont_oplist, itSrc);                     \
      M_CALL_IT_INSERT(cont_oplist, dst, itIns, *objSrc);                     \
      M_CALL_IT_NEXT(cont_oplist, itSrc);                                     \
    }                                                                         \
  }                                                                           \
  , /* NO IT_INSERT */ )                                                      \
                                                                              \
  /* Compute the intersection of two ***sorted*** containers  */              \
  M_IF_METHOD(IT_REMOVE, cont_oplist)(                                        \
  M_INLINE void                                                               \
  M_C3(name,sort_name,_intersect)(M_P_EXPAND container_t dst, const container_t src cmp_param(name)) \
  {                                                                           \
    it_t itSrc;                                                               \
    it_t itDst;                                                               \
    M_ASSERT(M_C3(name,sort_name,_p)(dst cmp_arg));                           \
    M_ASSERT(M_C3(name,sort_name,_p)(src cmp_arg));                           \
    M_CALL_IT_FIRST(cont_oplist, itSrc, src);                                 \
    M_CALL_IT_FIRST(cont_oplist, itDst, dst);                                 \
    /* TODO: Not optimized at all for array ! O(n^2) */                       \
    while (!M_CALL_IT_END_P(cont_oplist, itSrc)                               \
           && !M_CALL_IT_END_P(cont_oplist, itDst)) {                         \
      type_t const *objSrc = M_CALL_IT_CREF(cont_oplist, itSrc);              \
      type_t const *objDst = M_CALL_IT_CREF(cont_oplist, itDst);              \
      int cmp = cmp_func(name, sort_name, objDst, objSrc);                    \
      if (cmp == 0) {                                                         \
        /* Keep it */                                                         \
        M_CALL_IT_NEXT(cont_oplist, itSrc);                                   \
        M_CALL_IT_NEXT(cont_oplist, itDst);                                   \
      } else if (cmp < 0) {                                                   \
        M_CALL_IT_REMOVE(cont_oplist, dst, itDst);                            \
      } else {                                                                \
        M_CALL_IT_NEXT(cont_oplist, itSrc);                                   \
      }                                                                       \
    }                                                                         \
    while (!M_CALL_IT_END_P(cont_oplist, itDst)) {                            \
      M_CALL_IT_REMOVE(cont_oplist, dst, itDst);                              \
    }                                                                         \
  }                                                                           \
  , /* NO IT_REMOVE */ )


/* Define the find like algorithms of a given data
  TODO: Define _find_sorted that find in a sorted random access container 
  (binary search)
 */
#define M_ALG0_DEF_FIND(name, container_t, cont_oplist, type_t, type_oplist, it_t) \
  /* It supposes that the container is not sorted */                          \
  /* Find the next occurrence from it (included) of data */                   \
  M_INLINE void                                                               \
  M_F(name, _find_again) (it_t it, type_t const data)                         \
  {                                                                           \
    for ( /*nothing*/ ; !M_CALL_IT_END_P(cont_oplist, it) ;                   \
                      M_CALL_IT_NEXT(cont_oplist, it)) {                      \
      if (M_CALL_EQUAL(type_oplist, *M_CALL_IT_CREF(cont_oplist, it), data))  \
        return ;                                                              \
    }                                                                         \
  }                                                                           \
                                                                              \
  /* Find the first occurrence of data */                                     \
  M_INLINE void                                                               \
  M_F(name, _find) (it_t it, container_t const l, type_t const data)          \
  {                                                                           \
    M_CALL_IT_FIRST(cont_oplist, it, l);                                      \
    M_F(name, _find_again)(it, data);                                         \
  }                                                                           \
                                                                              \
  /* Test if data is within the container */                                  \
  M_INLINE bool                                                               \
  M_F(name, _contain_p) (container_t const l, type_t const data)              \
  {                                                                           \
    it_t it;                                                                  \
    M_F(name,_find)(it, l, data);                                             \
    return !M_CALL_IT_END_P(cont_oplist, it);                                 \
  }                                                                           \
                                                                              \
  /* Find the last occurrence of data in the container */                     \
  /* For the definition of _find_last, if the methods                         \
     PREVIOUS & IT_LAST are defined, then search backwards */                 \
  M_IF_METHOD2(PREVIOUS, IT_LAST, cont_oplist)                                \
  (                                                                           \
   M_INLINE void                                                              \
   M_F(name, _find_last) (it_t it, container_t const l, type_t const data)    \
   {                                                                          \
     for (M_CALL_IT_LAST(cont_oplist, it, l);                                 \
          !M_CALL_IT_END_P(cont_oplist, it) ;                                 \
          M_CALL_IT_PREVIOUS(cont_oplist, it)) {                              \
       if (M_CALL_EQUAL(type_oplist, *M_CALL_IT_CREF(cont_oplist, it), data)) \
         /* We can stop as soon as a match is found */                        \
         return;                                                              \
     }                                                                        \
   }                                                                          \
   ,                                                                          \
   /* Otherwise search forward, but don't stop on the first occurrence */     \
   M_INLINE void                                                              \
   M_F(name, _find_last) (it_t it, container_t const l, type_t const data)    \
   {                                                                          \
     M_CALL_IT_END(cont_oplist, it, l);                                       \
     it_t it2;                                                                \
     for (M_CALL_IT_FIRST(cont_oplist, it2, l);                               \
          !M_CALL_IT_END_P(cont_oplist, it2) ;                                \
          M_CALL_IT_NEXT(cont_oplist, it2)) {                                 \
       if (M_CALL_EQUAL(type_oplist, *M_CALL_IT_CREF(cont_oplist, it2), data)) \
         /* We cannot stop as soon as a match is found */                     \
         M_CALL_IT_SET(cont_oplist, it, it2) ;                                \
     }                                                                        \
   }                                                                          \
   ) /* End of alternative of _find_last */                                   \
                                                                              \
  /* Count the number of occurrence of data in the container */               \
  M_INLINE size_t                                                             \
  M_F(name, _count) (container_t const l, type_t const data)                  \
  {                                                                           \
    it_t it;                                                                  \
    size_t count = 0;                                                         \
    for (M_CALL_IT_FIRST(cont_oplist, it, l);                                 \
         !M_CALL_IT_END_P(cont_oplist, it) ;                                  \
         M_CALL_IT_NEXT(cont_oplist, it)) {                                   \
      if (M_CALL_EQUAL(type_oplist, *M_CALL_IT_CREF(cont_oplist, it), data))  \
        count++ ;                                                             \
    }                                                                         \
    return count;                                                             \
  }                                                                           \
                                                                              \
                                                                              \
  /* Find the next mismatch between the containers */                         \
  M_INLINE void                                                               \
  M_F(name, _mismatch_again) (it_t it1, it_t it2)                             \
  {                                                                           \
    for (/* nothing */ ; !M_CALL_IT_END_P(cont_oplist, it1) &&                \
                         !M_CALL_IT_END_P(cont_oplist, it2);                  \
                       M_CALL_IT_NEXT(cont_oplist, it1),                      \
                         M_CALL_IT_NEXT(cont_oplist, it2)) {                  \
      if (!M_CALL_EQUAL(type_oplist, *M_CALL_IT_CREF(cont_oplist, it1),       \
                        *M_CALL_IT_CREF(cont_oplist, it2)))                   \
        break;                                                                \
    }                                                                         \
  }                                                                           \
                                                                              \
  /* Find the first mismatch between the containers */                        \
  M_INLINE void                                                               \
  M_F(name, _mismatch) (it_t it1, it_t it2, container_t const l1, container_t const l2 ) \
  {                                                                           \
    M_CALL_IT_FIRST(cont_oplist, it1, l1);                                    \
    M_CALL_IT_FIRST(cont_oplist, it2, l2);                                    \
    M_F(name, _mismatch_again)(it1, it2);                                     \
  }                                                                           \



/* Define the find like algorithms with a given callback of function object
  TODO: Define _find_sorted that find in a sorted random access container 
  (binary search)
 */
#define M_ALG0_DEF_FIND_IF(name, container_t, cont_oplist, type_t, type_oplist, it_t, suffix, test_t, eq_t, call_test, call_eq) \
                                                                              \
  /* Find the next occurrence that matches the condition */                   \
  M_INLINE void                                                               \
  M_C3(name, _find_again_, suffix) (it_t it, test_t func)                     \
  {                                                                           \
    for (/*nothing */ ; !M_CALL_IT_END_P(cont_oplist, it) ;                   \
                      M_CALL_IT_NEXT(cont_oplist, it)) {                      \
      if (call_test(func, *M_CALL_IT_CREF(cont_oplist, it)))                  \
        return ;                                                              \
    }                                                                         \
  }                                                                           \
                                                                              \
  /* Find the first occurrence that matches the condition */                  \
  M_INLINE void                                                               \
  M_C3(name, _find_, suffix) (it_t it, container_t l, test_t func)            \
  {                                                                           \
    M_CALL_IT_FIRST(cont_oplist, it, l);                                      \
    M_C3(name, _find_again_, suffix)(it, func);                               \
  }                                                                           \
                                                                              \
  /* Count the number of occurrence that matches the condition */             \
  M_INLINE size_t                                                             \
  M_C3(name, _count_, suffix) (container_t const l, test_t func)              \
  {                                                                           \
    it_t it;                                                                  \
    size_t count = 0;                                                         \
    for (M_CALL_IT_FIRST(cont_oplist, it, l);                                 \
         !M_CALL_IT_END_P(cont_oplist, it) ;                                  \
         M_CALL_IT_NEXT(cont_oplist, it)) {                                   \
      if (call_test(func, *M_CALL_IT_CREF(cont_oplist, it))) {                \
        count++ ;                                                             \
      }                                                                       \
    }                                                                         \
    return count;                                                             \
  }                                                                           \
                                                                              \
  /* Find the next mismatch between the containers according to the condition */ \
  M_INLINE void                                                               \
  M_C3(name, _mismatch_again_, suffix) (it_t it1, it_t it2, eq_t func)        \
  {                                                                           \
    for (/*nothing */ ; !M_CALL_IT_END_P(cont_oplist, it1) &&                 \
                        !M_CALL_IT_END_P(cont_oplist, it2);                   \
                      M_CALL_IT_NEXT(cont_oplist, it1),                       \
                        M_CALL_IT_NEXT(cont_oplist, it2)) {                   \
      if (!call_eq(func, *M_CALL_IT_CREF(cont_oplist, it1),                   \
        *M_CALL_IT_CREF(cont_oplist, it2)))                                   \
        break;                                                                \
    }                                                                         \
  }                                                                           \
                                                                              \
  /* Find the first mismatch between the containers according to the condition */ \
  M_INLINE void                                                               \
  M_C3(name, _mismatch_, suffix) (it_t it1, it_t it2, container_t const l1,   \
                           container_t l2, eq_t func)                         \
  {                                                                           \
    M_CALL_IT_FIRST(cont_oplist, it1, l1);                                    \
    M_CALL_IT_FIRST(cont_oplist, it2, l2);                                    \
    M_C3(name, _mismatch_again_, suffix)(it1, it2, func);                     \
  }                                                                           \


/* Define the FILL algorithms */
#define M_ALG0_DEF_FILL(name, container_t, cont_oplist, type_t, type_oplist, it_t) \
                                                                              \
  /* Fill all the container with value (overwritten) */                       \
  M_P(void, name, _fill, container_t l, type_t const value)                   \
  {                                                                           \
    M_UNUSED_CONTEXT();                                                       \
    for M_EACH(item, l, cont_oplist) {                                        \
        M_CALL_SET(type_oplist, *item, value);                                \
    }                                                                         \
  }                                                                           \
                                                                              \
  /* Fill the container with exactly 'n' occurrence of 'value' */             \
  M_IF_METHOD(PUSH, cont_oplist)(                                             \
  M_P(void, name, _fill_n, container_t l, size_t n, type_t const value)       \
  {                                                                           \
    M_CALL_RESET(cont_oplist, l);                                             \
    for(size_t i = 0; i < n; i++) {                                           \
      M_CALL_PUSH(cont_oplist, l, value);                                     \
    }                                                                         \
  }                                                                           \
  , /* PUSH method */ )                                                       \
                                                                              \
  /* Fill the container with FOR('value'; 'value'+'inc') */                   \
  M_IF_METHOD(ADD, type_oplist)(                                              \
  M_P(void, name, _fill_a, container_t l, type_t const value, type_t const inc) \
  {                                                                           \
    M_UNUSED_CONTEXT();                                                       \
    type_t tmp;                                                               \
    M_CALL_INIT_SET(type_oplist, tmp, value);                                 \
    for M_EACH(item, l, cont_oplist) {                                        \
        M_CALL_SET(type_oplist, *item, tmp);                                  \
        M_CALL_ADD(type_oplist, tmp, tmp, inc);                               \
    }                                                                         \
    M_CALL_CLEAR(type_oplist, tmp);                                           \
  }                                                                           \
                                                                              \
  /* Fill the container with n occurences of FOR('value'; 'value'+'inc') */   \
  M_IF_METHOD(PUSH, cont_oplist)(                                             \
  M_P(void, name, _fill_an, container_t l, size_t n, type_t const value, type_t const inc) \
  {                                                                           \
    M_UNUSED_CONTEXT();                                                       \
    type_t tmp;                                                               \
    M_CALL_INIT_SET(type_oplist, tmp, value);                                 \
    M_CALL_RESET(cont_oplist, l);                                             \
    for(size_t i = 0; i < n; i++) {                                           \
      M_CALL_PUSH(cont_oplist, l, tmp);                                       \
      M_CALL_ADD(type_oplist, tmp, tmp, inc);                                 \
    }                                                                         \
    M_CALL_CLEAR(type_oplist, tmp);                                           \
  }                                                                           \
  , /* PUSH method */ )                                                       \
  , /* ADD method */ )                                                        \


/* Define MAP algorithms */
#define M_ALG0_DEF_MAP(name, container_t, cont_oplist, type_t, type_oplist, it_t) \
                                                                              \
  /* Apply func for all elements of the container */                          \
  M_INLINE void                                                               \
  M_F(name, _for_each) (container_t l, M_F(name, _apply_cb_ct) func)          \
  {                                                                           \
    for M_EACH(item, l, cont_oplist) {                                        \
        func(*item);                                                          \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_IF_METHOD(INIT, type_oplist)(                                             \
  M_IF_METHOD(PUSH, cont_oplist)(                                             \
                                                                              \
  /* Apply func for all elements of the container src and push the result in dst */ \
  M_P(void, name, _transform, container_t dst, container_t src, M_F(name, _transform_cb_ct) func, void *data) \
  {                                                                           \
    M_ASSERT(dst != src);                                                     \
    M_CALL_RESET(cont_oplist, dst);                                           \
    type_t tmp;                                                               \
    M_CALL_INIT(type_oplist, tmp);                                            \
    for M_EACH(item, src, cont_oplist) {                                      \
        if (func(&tmp, *item, data)) {                                        \
          M_CALL_PUSH(cont_oplist, dst, tmp);                                 \
        }                                                                     \
    }                                                                         \
    M_CALL_CLEAR(type_oplist, tmp);                                           \
    M_IF_METHOD(REVERSE, cont_oplist)(M_CALL_REVERSE(cont_oplist, dst),);     \
  }                                                                           \
  , /* END PUSH_MOVE */), /* END INIT */ )                                    \
                                                                              \
  /* Reduce all elements of the container in dst in function of func */       \
  M_IF_METHOD(SET, type_oplist)(                                              \
  M_P(bool, name, _reduce, type_t *dest, container_t const l, M_F(name, _reduce_cb_ct) func) \
  {                                                                           \
    M_UNUSED_CONTEXT();                                                       \
    bool initDone = false;                                                    \
    for M_EACH(item, l, cont_oplist) {                                        \
        if (initDone) {                                                       \
          func(dest, *item);                                                  \
        } else {                                                              \
          M_CALL_SET(type_oplist, *dest, *item);                              \
          initDone = true;                                                    \
        }                                                                     \
      }                                                                       \
    return initDone;                                                          \
  }                                                                           \
  , /* END SET */)                                                            \
                                                                              \
  /* Reduce all transformed elements of the container in dst in function of func */ \
  M_IF_METHOD(INIT, type_oplist)(                                             \
  M_P(bool, name, _transform_reduce, type_t *dest, const container_t l,       \
                               M_F(name, _reduce_cb_ct) redFunc,              \
                               M_F(name, _transform_cb_ct) mapFunc, void *data) \
  {                                                                           \
    M_UNUSED_CONTEXT();                                                       \
    bool initDone = false;                                                    \
    type_t tmp;                                                               \
    M_CALL_INIT(type_oplist, tmp);                                            \
    for M_EACH(item, l, cont_oplist) {                                        \
        if (initDone) {                                                       \
          if (mapFunc(&tmp, *item, data) ) {                                  \
            redFunc(dest, tmp);                                               \
          }                                                                   \
        } else {                                                              \
          initDone = mapFunc(dest, *item, data);                              \
        }                                                                     \
      }                                                                       \
    M_CALL_CLEAR(type_oplist, tmp);                                           \
    return initDone;                                                          \
  }                                                                           \
  , )                                                                         \


/* Define ALL_OF algorithms */
#define M_ALG0_DEF_ALL_OF(name, container_t, cont_oplist, type_t, type_oplist, it_t, suffix, func_t, call) \
                                                                              \
  M_INLINE bool                                                               \
  M_C4(name, _any_of, suffix, p) (container_t const l,                        \
                                  func_t func )                               \
  {                                                                           \
    for M_EACH(item, l, cont_oplist) {                                        \
      if (call(func, *item))                                                  \
          return true;                                                        \
    }                                                                         \
    return false;                                                             \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_C4(name, _all_of, suffix, p) (container_t const l,                        \
                                  func_t func )                               \
  {                                                                           \
    for M_EACH(item, l, cont_oplist) {                                        \
        if (!call(func, *item))                                               \
          return false;                                                       \
    }                                                                         \
    return true;                                                              \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_C4(name, _none_of, suffix, p) (container_t l,                             \
                                    func_t func )                             \
  {                                                                           \
    for M_EACH(item, l, cont_oplist) {                                        \
        if (call(func, *item))                                                \
          return false;                                                       \
      }                                                                       \
    return true;                                                              \
  }                                                                           \


/* Define MIN / MAX algorithms */
#define M_ALG0_DEF_MINMAX(name, container_t, cont_oplist, type_t, type_oplist, it_t) \
                                                                              \
  M_INLINE type_t *                                                           \
  M_F(name, _min) (const container_t l)                                       \
  {                                                                           \
    type_t *min = NULL;                                                       \
    for M_EACH(cref, l, cont_oplist) {                                        \
        if (min == NULL ||                                                    \
            M_CALL_CMP(type_oplist, *min, *cref) > 0) {                       \
          min = cref;                                                         \
        }                                                                     \
      }                                                                       \
    return min;                                                               \
  }                                                                           \
                                                                              \
  M_INLINE type_t *                                                           \
  M_F(name, _max) (const container_t l)                                       \
  {                                                                           \
    type_t *max = NULL;                                                       \
    for M_EACH(cref, l, cont_oplist) {                                        \
        if (max == NULL ||                                                    \
            M_CALL_CMP(type_oplist, *max, *cref) < 0) {                       \
            max = cref;                                                       \
        }                                                                     \
      }                                                                       \
    return max;                                                               \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _minmax) (type_t **min_p, type_t **max_p,                         \
                      const container_t l)                                    \
  {                                                                           \
    type_t *min = NULL;                                                       \
    type_t *max = NULL;                                                       \
    for M_EACH(cref, l, cont_oplist) {                                        \
        if (min == NULL ||                                                    \
            M_CALL_CMP(type_oplist, *min, *cref) > 0) {                       \
            min = cref;                                                       \
        }                                                                     \
        if (max == NULL ||                                                    \
            M_CALL_CMP(type_oplist, *max, *cref) < 0) {                       \
          max = cref;                                                         \
        }                                                                     \
      }                                                                       \
    *min_p = min;                                                             \
    *max_p = max;                                                             \
  }                                                                           \

/* Define functions based on remove method */
#define M_ALG0_DEF_REMOVE(name, container_t, cont_oplist, type_t, type_oplist, it_t) \
                                                                              \
  M_P(void, name, _uniq, container_t l)                                       \
  {                                                                           \
    it_t it1;                                                                 \
    it_t it2;                                                                 \
    M_ASSERT(M_F(name, _sort_p)(l));                                          \
    M_CALL_IT_FIRST(cont_oplist, it1, l);                                     \
    M_CALL_IT_SET(cont_oplist, it2, it1);                                     \
    M_CALL_IT_NEXT(cont_oplist, it2);                                         \
    /* Not efficient for array! */                                            \
    while (!M_CALL_IT_END_P(cont_oplist, it2)) {                              \
      type_t const *ref1 = M_CALL_IT_CREF(cont_oplist, it1);                  \
      type_t const *ref2 = M_CALL_IT_CREF(cont_oplist, it2);                  \
      if (M_CALL_CMP(type_oplist, *ref1, *ref2) == 0) {                       \
        M_CALL_IT_REMOVE(cont_oplist, l, it2);                                \
      } else {                                                                \
        M_CALL_IT_SET(cont_oplist, it1, it2);                                 \
        M_CALL_IT_NEXT(cont_oplist, it2);                                     \
      }                                                                       \
      }                                                                       \
  }                                                                           \
                                                                              \
  M_P(void, name, _remove_val, container_t l, type_t const val)               \
  {                                                                           \
    it_t it1;                                                                 \
    M_CALL_IT_FIRST(cont_oplist, it1, l);                                     \
    while (!M_CALL_IT_END_P(cont_oplist, it1)) {                              \
      type_t const *ref1 = M_CALL_IT_CREF(cont_oplist, it1);                  \
      if (M_CALL_EQUAL(type_oplist, *ref1, val)) {                            \
        M_CALL_IT_REMOVE(cont_oplist, l, it1);                                \
      } else {                                                                \
        M_CALL_IT_NEXT(cont_oplist, it1);                                     \
      }                                                                       \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_P(void, name, _remove_if, container_t l, M_F(name, _test_cb_ct) func)     \
  {                                                                           \
    it_t it1;                                                                 \
    M_CALL_IT_FIRST(cont_oplist, it1, l);                                     \
    while (!M_CALL_IT_END_P(cont_oplist, it1)) {                              \
      type_t const *ref1 = M_CALL_IT_CREF(cont_oplist, it1);                  \
      if (func(*ref1)) {                                                      \
        M_CALL_IT_REMOVE(cont_oplist, l, it1);                                \
      } else {                                                                \
        M_CALL_IT_NEXT(cont_oplist, it1);                                     \
      }                                                                       \
    }                                                                         \
  }                                                                           \

/* Define vector algorithms */
#define M_ALG0_DEF_VECTOR(name, container_t, cont_oplist, type_t, type_oplist, it_t) \
                                                                              \
  M_IF_METHOD(ADD, type_oplist)(                                              \
  M_INLINE void M_F(name, _add) (container_t dst, const container_t src)      \
  {                                                                           \
    it_t itSrc;                                                               \
    it_t itDst;                                                               \
    for (M_CALL_IT_FIRST(cont_oplist, itSrc, src) ,                           \
           M_CALL_IT_FIRST(cont_oplist, itDst, dst) ;                         \
         !M_CALL_IT_END_P(cont_oplist, itSrc)                                 \
           && !M_CALL_IT_END_P(cont_oplist, itDst) ;                          \
         M_CALL_IT_NEXT(cont_oplist, itSrc),                                  \
           M_CALL_IT_NEXT(cont_oplist, itDst) ) {                             \
      type_t *dstItem = M_CALL_IT_REF(cont_oplist, itDst);                    \
      type_t const *srcItem = M_CALL_IT_CREF(cont_oplist, itSrc);             \
      M_CALL_ADD(type_oplist, *dstItem, *dstItem, *srcItem);                  \
    }                                                                         \
  }                                                                           \
  , /* NO_ADD METHOD */ )                                                     \
                                                                              \
  M_IF_METHOD(SUB, type_oplist)(                                              \
  M_INLINE void M_F(name, _sub) (container_t dst, const container_t src)      \
  {                                                                           \
    it_t itSrc;                                                               \
    it_t itDst;                                                               \
    for (M_CALL_IT_FIRST(cont_oplist, itSrc, src) ,                           \
           M_CALL_IT_FIRST(cont_oplist, itDst, dst) ;                         \
         !M_CALL_IT_END_P(cont_oplist, itSrc)                                 \
           && !M_CALL_IT_END_P(cont_oplist, itDst) ;                          \
         M_CALL_IT_NEXT(cont_oplist, itSrc),                                  \
           M_CALL_IT_NEXT(cont_oplist, itDst) ) {                             \
      type_t *dstItem = M_CALL_IT_REF(cont_oplist, itDst);                    \
      type_t const *srcItem = M_CALL_IT_CREF(cont_oplist, itSrc);             \
      M_CALL_SUB(type_oplist, *dstItem, *dstItem, *srcItem);                  \
    }                                                                         \
  }                                                                           \
  , /* NO_SUB METHOD */ )                                                     \
                                                                              \
  M_IF_METHOD(MUL, type_oplist)(                                              \
  M_INLINE void M_F(name, _mul) (container_t dst, const container_t src)      \
  {                                                                           \
    it_t itSrc;                                                               \
    it_t itDst;                                                               \
    for (M_CALL_IT_FIRST(cont_oplist, itSrc, src) ,                           \
           M_CALL_IT_FIRST(cont_oplist, itDst, dst) ;                         \
         !M_CALL_IT_END_P(cont_oplist, itSrc)                                 \
           && !M_CALL_IT_END_P(cont_oplist, itDst) ;                          \
         M_CALL_IT_NEXT(cont_oplist, itSrc),                                  \
           M_CALL_IT_NEXT(cont_oplist, itDst) ) {                             \
      type_t *dstItem = M_CALL_IT_REF(cont_oplist, itDst);                    \
      type_t const *srcItem = M_CALL_IT_CREF(cont_oplist, itSrc);             \
      M_CALL_MUL(type_oplist, *dstItem, *dstItem, *srcItem);                  \
    }                                                                         \
  }                                                                           \
  , /* NO_MUL METHOD */ )                                                     \
                                                                              \
  M_IF_METHOD(DIV, type_oplist)(                                              \
  M_INLINE void M_F(name, _div) (container_t dst, const container_t src)      \
  {                                                                           \
    it_t itSrc;                                                               \
    it_t itDst;                                                               \
    for (M_CALL_IT_FIRST(cont_oplist, itSrc, src) ,                           \
           M_CALL_IT_FIRST(cont_oplist, itDst, dst) ;                         \
         !M_CALL_IT_END_P(cont_oplist, itSrc)                                 \
           && !M_CALL_IT_END_P(cont_oplist, itDst) ;                          \
         M_CALL_IT_NEXT(cont_oplist, itSrc),                                  \
           M_CALL_IT_NEXT(cont_oplist, itDst) ) {                             \
      type_t *dstItem = M_CALL_IT_REF(cont_oplist, itDst);                    \
      type_t const *srcItem = M_CALL_IT_CREF(cont_oplist, itSrc);             \
      M_CALL_DIV(type_oplist, *dstItem, *dstItem, *srcItem);                  \
    }                                                                         \
  }                                                                           \
  , /* NO_DIV METHOD */ )                                                     \
                                                                              \


//TODO: Algorithm missing
// _nth_element ( http://softwareengineering.stackexchange.com/questions/284767/kth-selection-routine-floyd-algorithm-489 )
// _average
// _sort_uniq (_sort + _uniq)
// fast _uniq for array
// _flatten (takes a set of containers and returns a new container containing all flatten objects)

/******************************** INTERNAL ***********************************/

#define M_ALG0_FOR_EACH(container, cont_oplist, func) do {                    \
    for M_EACH(_item, container, cont_oplist) {                               \
        func(*_item);                                                         \
      }                                                                       \
  } while (0)

#define M_ALG0_FOR_EACH_ARG(container, cont_oplist, func, ...) do {           \
    for M_EACH(_item, container, cont_oplist) {                               \
        func(__VA_ARGS__, *_item);                                            \
      }                                                                       \
  } while (0)


/******************************** INTERNAL ***********************************/

#define M_ALG0_TRANSFORM(contD, contDop, contS, contSop, func) do {           \
    M_CALL_RESET(contDop, contD);                                             \
    for M_EACH(_item, contS, contSop) {                                       \
        M_GET_SUBTYPE contDop _tmp;                                           \
        M_CALL_INIT(M_GET_OPLIST contDop, _tmp);                              \
        func(_tmp, *_item);                                                   \
        M_CALL_PUSH_MOVE(contDop, contD, &_tmp);                              \
      }                                                                       \
    M_IF_METHOD(REVERSE, contDop) (M_CALL_REVERSE(contDop, contD);, )         \
  } while (0)

#define M_ALG0_TRANSFORM_ARG(contD, contDop, contS, contSop, func, ...) do {  \
    M_CALL_RESET(contDop, contD);                                             \
    for M_EACH(_item, contS, contSop) {                                       \
        M_GET_SUBTYPE contDop _tmp;                                           \
        M_CALL_INIT(M_GET_OPLIST contDop, _tmp);                              \
        func(_tmp, *_item, __VA_ARGS__);                                      \
        M_CALL_PUSH_MOVE(contDop, contD, &_tmp);                              \
      }                                                                       \
    M_IF_METHOD(REVERSE, contDop) (M_CALL_REVERSE(contDop, contD);, )         \
  } while (0)


/******************************** INTERNAL ***********************************/

#define M_ALG0_EXTRACT(contDst, contDstOplist, contSrc, contSrcOplist)        \
  M_ALG0_EXTRACT_P1(contDst, M_GLOBAL_OPLIST(contDstOplist), contSrc, M_GLOBAL_OPLIST(contSrcOplist))
#define M_ALG0_EXTRACT_P1(contDst, contDstOplist, contSrc, contSrcOplist) do{ \
    M_CALL_RESET(contDstOplist, contDst);                                     \
    for M_EACH(_item, contSrc, contSrcOplist) {                               \
        M_IF_METHOD(PUSH, contDstOplist)(                                     \
          M_CALL_PUSH(contDstOplist, contDst, *_item);                        \
        ,                                                                     \
          M_CALL_SET_KEY(contDstOplist, contDst, (*_item).key, (*_item).value); \
                                                                        )     \
    }                                                                         \
    M_IF_METHOD(REVERSE, contDstOplist) (M_CALL_REVERSE(contDstOplist, contDst);, ) \
  } while (0)

#define M_ALG0_EXTRACT_FUNC(contDst, contDstOplist, contSrc, contSrcOplist, condFunc) \
  M_ALG0_EXTRACT_FUNC_P1(contDst, M_GLOBAL_OPLIST(contDstOplist), contSrc, M_GLOBAL_OPLIST(contSrcOplist), condFunc)
#define M_ALG0_EXTRACT_FUNC_P1(contDst, contDstOplist, contSrc, contSrcOplist, \
                      condFunc) do {                                          \
    M_CALL_RESET(contDstOplist, contDst);                                     \
    for M_EACH(_item, contSrc, contSrcOplist) {                               \
        if (condFunc (*_item)) {                                              \
          M_IF_METHOD(PUSH, contDstOplist)(                                   \
            M_CALL_PUSH(contDstOplist, contDst, *_item);                      \
          ,                                                                   \
            M_CALL_SET_KEY(contDstOplist, contDst, (*_item).key, (*_item).value); \
                                                                        )     \
        }                                                                     \
    }                                                                         \
    M_IF_METHOD(REVERSE, contDstOplist) (M_CALL_REVERSE(contDstOplist, contDst);, ) \
  } while (0)

#define M_ALG0_EXTRACT_ARG(contDst, contDstOplist, contSrc, contSrcOplist,    \
                          condFunc, ...)                                      \
  M_ALG0_EXTRACT_ARG_P1(contDst, M_GLOBAL_OPLIST(contDstOplist), contSrc, M_GLOBAL_OPLIST(contSrcOplist), \
                        condFunc, __VA_ARGS__)
#define M_ALG0_EXTRACT_ARG_P1(contDst, contDstOplist, contSrc, contSrcOplist, \
                          condFunc, ...) do {                                 \
    M_CALL_RESET(contDstOplist, contDst);                                     \
    for M_EACH(_item, contSrc, contSrcOplist) {                               \
        if (condFunc (__VA_ARGS__, *_item)) {                                 \
          M_IF_METHOD(PUSH, contDstOplist)(                                   \
            M_CALL_PUSH(contDstOplist, contDst, *_item);                      \
            ,                                                                 \
            M_CALL_SET_KEY(contDstOplist, contDst, (*_item).key, (*_item).value); \
                                                                        )     \
        }                                                                     \
    }                                                                         \
    M_IF_METHOD(REVERSE, contDstOplist) (M_CALL_REVERSE(contDstOplist, contDstOplist);, ) \
  } while (0)


/******************************** INTERNAL ***********************************/

#define M_ALG0_REDUCE_DISPATCH(dest, destOp, dest_t, cont, contOp,  ...)      \
  M_IF_NARGS_EQ1(__VA_ARGS__)                                                 \
  (M_ALG0_REDUCE_BASIC(dest, dest_t, destOp, cont, contOp, __VA_ARGS__),      \
   M_IF_NARGS_EQ2(__VA_ARGS__)                                                \
   (M_ALG0_REDUCE_FOR_EACH(dest, dest_t, destOp, cont, contOp, __VA_ARGS__),  \
    M_ALG0_REDUCE_FOR_EACH_ARG(dest, dest_t, destOp, cont, contOp, __VA_ARGS__) ) )

/* The special functions handled by ALGO_REDUCE */
#define M_ALG0_REDUCE_AND(a,b) ((a) &= (b))
#define M_ALG0_REDUCE_OR(a,b)  ((a) |= (b))
#define M_ALG0_REDUCE_SUM(a,b) ((a) += (b))
#define M_ALG0_REDUCE_PRODUCT(a,b) ((a) *= (b))

/* Return the method associated to a reduce operation.
   It returns the special function handler if function is and, or, sum or add.
   Otherwise it returns the original function */
#define M_ALG0_REDUCE_FUNC(reduceFunc)                                        \
  M_IF(M_KEYWORD_P(and, reduceFunc))                                          \
  (M_ALG0_REDUCE_AND,                                                         \
   M_IF(M_KEYWORD_P(or, reduceFunc))                                          \
   (M_ALG0_REDUCE_OR,                                                         \
    M_IF(M_KEYWORD_P(sum, reduceFunc))                                        \
    (M_ALG0_REDUCE_SUM,                                                       \
     M_IF(M_KEYWORD_P(product, reduceFunc))                                   \
     (M_ALG0_REDUCE_PRODUCT,                                                  \
      reduceFunc)                                                             \
     )                                                                        \
    )                                                                         \
   )

#define M_ALG0_REDUCE_BASIC(dest, dest_t, destOp, cont, cont_oplist, reduceFunc) do { \
    bool _init_done = false;                                                  \
    for M_EACH(_item, cont, cont_oplist) {                                    \
        if (_init_done) {                                                     \
          M_ALG0_REDUCE_FUNC(reduceFunc) (dest, *_item);                      \
        } else {                                                              \
          M_CALL_SET(destOp, dest, *_item);                                   \
          _init_done = true;                                                  \
        }                                                                     \
      }                                                                       \
  } while (0)

#define M_ALG0_REDUCE_FOR_EACH(dest, dest_t, destOp, cont, cont_oplist, reduceFunc, mapFunc) do { \
    bool _init_done = false;                                                  \
    dest_t _tmp;                                                              \
    M_CALL_INIT(destOp, _tmp);                                                \
    for M_EACH(_item, cont, cont_oplist) {                                    \
        mapFunc(_tmp, *_item);                                                \
        if (_init_done) {                                                     \
          M_ALG0_REDUCE_FUNC(reduceFunc) (dest, _tmp);                        \
        } else {                                                              \
          M_CALL_SET(destOp, dest, _tmp);                                     \
          _init_done = true;                                                  \
        }                                                                     \
      }                                                                       \
    M_CALL_CLEAR(destOp, _tmp);                                               \
  } while (0)

#define M_ALG0_REDUCE_FOR_EACH_ARG(dest, dest_t, destOp, cont, cont_oplist, reduceFunc, mapFunc, ...) do { \
    bool _init_done = false;                                                  \
    dest_t _tmp;                                                              \
    M_CALL_INIT(destOp, _tmp);                                                \
    for M_EACH(_item, cont, cont_oplist) {                                    \
        mapFunc(_tmp, __VA_ARGS__, *_item);                                   \
        if (_init_done) {                                                     \
          M_ALG0_REDUCE_FUNC(reduceFunc) (dest, _tmp);                        \
        } else {                                                              \
          M_CALL_SET(destOp, dest, _tmp);                                     \
          _init_done = true;                                                  \
        }                                                                     \
      }                                                                       \
    M_CALL_CLEAR(destOp, _tmp);                                               \
  } while (0)


/******************************** INTERNAL ***********************************/

#define M_ALG0_INSERT_AT(contDst, contDstOp, position, contSrc, contSrcOp) do { \
    M_GET_IT_TYPE contSrcOp _itSrc;                                           \
    M_GET_IT_TYPE contDstOp _itDst;                                           \
    M_CALL_IT_SET(contDstOp, _itDst, position);                               \
    for (M_CALL_IT_FIRST(contSrcOp, _itSrc, contSrc) ;                        \
         !M_CALL_IT_END_P(contSrcOp, _itSrc) ;                                \
         M_CALL_IT_NEXT(contSrcOp, _itSrc) ) {                                \
      M_CALL_IT_INSERT(contDstOp, contDst, _itDst,                            \
                       *M_CALL_IT_CREF(contSrcOp, _itSrc));                   \
    }                                                                         \
  } while (0)


/******************************** INTERNAL ***********************************/

#if M_USE_SMALL_NAME
#define ALGO_DEF M_ALGO_DEF
#define ALGO_FOR_EACH M_ALGO_FOR_EACH
#define ALGO_TRANSFORM M_ALGO_TRANSFORM
#define ALGO_EXTRACT M_ALGO_EXTRACT
#define ALGO_REDUCE M_ALGO_REDUCE
#define ALGO_INSERT_AT M_ALGO_INSERT_AT
#endif

#endif
