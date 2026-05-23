/*
 * M*LIB - DEQUE module
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
#ifndef MSTARLIB_DEQUE_H
#define MSTARLIB_DEQUE_H

#include "m-i-list.h"

/* Define a deque of a given type and its associated functions.
   USAGE: DEQUE_DEF(name, type [, oplist_of_the_type]) */
#define M_DEQUE_DEF(name, ...)                                                \
  M_DEQUE_DEF_AS(name, M_F(name,_t), M_F(name,_it_t), __VA_ARGS__)


/* Define a deque of a given type and its associated functions.
   as the provided type name_t with the iterator named it_t.
   USAGE: DEQUE_DEF(name, name_t, it_t, type [, oplist_of_the_type]) */
#define M_DEQUE_DEF_AS(name, name_t, it_t, ...)                               \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_D3QU3_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                                  \
                ((name, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), name_t, it_t, M_F(name, _node_ct) ), \
                 (name, __VA_ARGS__,                                        name_t, it_t, M_F(name, _node_ct)))) \
  M_END_PROTECTED_CODE


/* Define the oplist of a deque of a type.
   USAGE: DEQUE_OPLIST(name[, oplist of the type]) */
#define M_DEQUE_OPLIST(...)                                                   \
  M_D3QU3_OPLIST_P1 (M_IF_NARGS_EQ1(__VA_ARGS__)                              \
                    ((__VA_ARGS__, M_BASIC_OPLIST),                           \
                     (__VA_ARGS__ )))

/* Default initial size of a bucket of items */
#ifndef M_USE_DEQUE_DEFAULT_SIZE
#define M_USE_DEQUE_DEFAULT_SIZE  8
#endif


/*****************************************************************************/
/********************************* INTERNAL **********************************/
/*****************************************************************************/

/* Define the internal contract of a deque */
#define M_D3QU3_CONTRACT(d) do {                                              \
    M_ASSERT ((d) != NULL);                                                   \
    M_ASSERT ((d)->default_size >= M_USE_DEQUE_DEFAULT_SIZE);                 \
    M_ASSERT ((d)->front->node != NULL);                                      \
    M_ASSERT ((d)->front->index <= (d)->front->node->size);                   \
    M_ASSERT ((d)->back->node != NULL);                                       \
    M_ASSERT ((d)->back->index <= (d)->back->node->size);                     \
    M_ASSERT ((d)->front->node != (d)->back->node ||                          \
            (d)->front->index <= (d)->back->index);                           \
    M_ASSERT ((d)->front->node != (d)->back->node ||                          \
            (d)->back->index - (d)->front->index == (d)->count);              \
  } while (0)

/* Deferred evaluation for the deque definition,
   so that all arguments are evaluated before further expansion */
#define M_D3QU3_DEF_P1(arg) M_ID( M_D3QU3_DEF_P2 arg )

/* Validate the oplist before going further */
#define M_D3QU3_DEF_P2(name, type, oplist, deque_t, it_t, node_t)             \
  M_IF_OPLIST(oplist)(M_D3QU3_DEF_P3, M_D3QU3_DEF_FAILURE)(name, type, oplist, deque_t, it_t, node_t)

/* Stop processing with a compilation failure */
#define M_D3QU3_DEF_FAILURE(name, type, oplist, deque_t, it_t, node_t)        \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST, "(DEQUE_DEF): the given argument is not a valid oplist: " #oplist)

/* Internal deque definition
   - name: prefix to be used
   - type: type of the elements of the deque
   - oplist: oplist of the type of the elements of the container
   - deque_t: alias for M_F(name, _t) [ type of the container ]
   - it_t: alias for M_F(name, _it_t) [ iterator of the container ]
   - node_t: alias for node_t [ node ]
 */
#define M_D3QU3_DEF_P3(name, type, oplist, deque_t, it_t, node_t)             \
  M_D3QU3_DEF_TYPE(name, type, oplist, deque_t, it_t, node_t)                 \
  M_CHECK_COMPATIBLE_OPLIST(name, 1, type, oplist)                            \
  M_D3QU3_DEF_CORE(name, type, oplist, deque_t, it_t, node_t)                 \
  M_EMPLACE_QUEUE_DEF(name, deque_t, _emplace_back, oplist, M_D3QUE_EMPLACE_BACK_DEF) \
  M_EMPLACE_QUEUE_DEF(name, deque_t, _emplace_front, oplist, M_D3QUE_EMPLACE_FRONT_DEF)

// OPLIST for the node deletion
#ifdef M_USE_CONTEXT
#define M_D3QU3_NODE_DEL_OPLIST(name) (DEL(M_F(name, _node_list_i_del)))
#else
#define M_D3QU3_NODE_DEL_OPLIST(name) (DEL(API(M_F(name, _node_list_i_del), NONE, ARG2)))
#endif

/* Define the types.
   It is a linked list of buckets of the types,
   each new created bucket size grows compared to the previous one
   resulting in:
   strict O(1) for push/pop
   O(ln(n)) for random access.
   No insert / delete operations are planned.
   [Could be done in O(n) complexity if needed]
   Define the bucket (aka node) structure.
*/
#define M_D3QU3_DEF_TYPE(name, type, oplist, deque_t, it_t, node_t)           \
                                                                              \
  typedef struct M_F(name, _node_s) {                                         \
    M_ILIST_INTERFACE(M_F(name, _node_list), struct M_F(name, _node_s));      \
    size_t size, capacity;                                                    \
    type  data[M_MIN_FLEX_ARRAY_SIZE];                                        \
  } node_t;                                                                   \
                                                                              \
  /* Each node is allocated with a variable size bucket (so we use            \
     M_GET_REALLOC for the allocation). But we want to delete the nodes       \
     automatically with the intrusive list used for storing the nodes:        \
     so we register as a DEL operator the FREE operator of the oplist.        \
  */                                                                          \
  /* FIXME: How can I separate public types and private implementation? */    \
  M_P(void, name, _node_list_i_del, node_t *ptr)                              \
  {                                                                           \
    M_CALL_FREE(oplist, char, ptr, sizeof(node_t) + ptr->capacity * sizeof(type)); \
  }                                                                           \
  M_ILIST_DEF(M_F(name, _node_list), node_t, M_D3QU3_NODE_DEL_OPLIST(name) )  \
                                                                              \
  /* Define an internal iterator */                                           \
  typedef struct M_F(name, _it2_s) {                                          \
    node_t *node;                                                             \
    size_t        index;                                                      \
  } M_F(name, _it2_ct)[1];                                                    \
                                                                              \
  /* Define the deque type:                                                   \
     - 'list' if the list of buckets containing the objects.                  \
     - 'front' is a pseudo-iterator to the first                              \
     - 'back' is a pseudo-iterator to the one after last element              \
     - 'default_size' is the size used for the creation of a new bucket       \
     - 'count' is the number of elements in the container.                    \
  */                                                                          \
  typedef struct M_F(name, _s) {                                              \
    M_F(name, _node_list_t) list;                                             \
    M_F(name, _it2_ct)      front;                                            \
    M_F(name, _it2_ct)      back;                                             \
    size_t                  default_size;                                     \
    size_t                  count;                                            \
  } deque_t[1];                                                               \
                                                                              \
  /* Define pointer alias */                                                  \
  typedef struct M_F(name, _s) *M_F(name, _ptr);                              \
  typedef const struct M_F(name, _s) *M_F(name, _srcptr);                     \
                                                                              \
  /* Define the iterator object */                                            \
  typedef struct M_F(name, _it_s) {                                           \
    node_t *node;                                                             \
    size_t        index;                                                      \
    const struct M_F(name, _s) *deque;                                        \
  } it_t[1];                                                                  \
                                                                              \
  /* Define internal types for oplist */                                      \
  typedef deque_t M_F(name, _ct);                                             \
  typedef type    M_F(name, _subtype_ct);                                     \
  typedef it_t    M_F(name, _it_ct);                                          \

/* Define the core functions */
#define M_D3QU3_DEF_CORE(name, type, oplist, deque_t, it_t, node_t)           \
                                                                              \
  /* Allocate a new node for a deque */                                       \
  M_P(node_t*, name, _i_new_node, deque_t d)                                  \
  {                                                                           \
    size_t def = d->default_size;                                             \
    /* Test for overflow of the size computation */                           \
    if (M_UNLIKELY_NOMEM (def > SIZE_MAX / sizeof (type) - sizeof(node_t))) { \
      M_MEMORY_FULL(char, sizeof(node_t)+def * sizeof(type));                 \
    }                                                                         \
    /* Alloc a new node with dynamic size */                                  \
    node_t*n = (node_t*) (void*)                                              \
      M_CALL_REALLOC(oplist, char, NULL, 0, sizeof(node_t) + def * sizeof(type) ); \
    if (M_UNLIKELY_NOMEM (n==NULL)) {                                         \
      M_MEMORY_FULL(char, sizeof(node_t)+def * sizeof(type));                 \
    }                                                                         \
    /* Initialize the node */                                                 \
    n->size = def;                                                            \
    n->capacity = def;                                                        \
    M_F(name, _node_list_init_field)(n);                                      \
    /* Increase the next bucket allocation */                                 \
    /* Do not increase it too much if there are few items */                  \
    def = M_MIN(def, d->count);                                               \
    d->default_size = M_CALL_INC_ALLOC(oplist, def);                          \
    /* FIXME: Check for overflow? */                                          \
    return n;                                                                 \
  }                                                                           \
                                                                              \
  M_P(void, name, _init, deque_t d)                                           \
  {                                                                           \
    M_F(name, _node_list_init)(d->list);                                      \
    d->default_size = M_USE_DEQUE_DEFAULT_SIZE;                               \
    d->count        = 0;                                                      \
    node_t *n = M_F(name,_i_new_node)M_R(d);                                  \
    M_F(name, _node_list_push_back)(d->list, n);                              \
    d->front->node  = n;                                                      \
    d->front->index = M_USE_DEQUE_DEFAULT_SIZE/2;                             \
    d->back->node   = n;                                                      \
    d->back->index  = M_USE_DEQUE_DEFAULT_SIZE/2;                             \
    M_D3QU3_CONTRACT(d);                                                      \
  }                                                                           \
                                                                              \
  M_P(void, name, _reset, deque_t d)                                          \
  {                                                                           \
    M_D3QU3_CONTRACT(d);                                                      \
    M_UNUSED_CONTEXT();                                                       \
    node_t *min_node = NULL;                                                  \
    for(node_t *n = d->front->node;                                           \
        n != NULL ;                                                           \
        n = (n == d->back->node) ? NULL :                                     \
          M_F(name, _node_list_next_obj)(d->list, n) ){                       \
      size_t min = n == d->front->node ? d->front->index : 0;                 \
      size_t max = n == d->back->node ? d->back->index : n->size;             \
      for(size_t i = min; i < max; i++) {                                     \
        M_CALL_CLEAR(oplist, n->data[i]);                                     \
      }                                                                       \
      min_node = (min_node == NULL || min_node->size > n->size) ? n : min_node; \
    }                                                                         \
    M_ASSERT (min_node != NULL);                                              \
    d->front->node = min_node;                                                \
    d->front->index = min_node->size / 2;                                     \
    d->back->node = min_node;                                                 \
    d->back->index = min_node->size / 2;                                      \
    d->count = 0;                                                             \
    M_D3QU3_CONTRACT(d);                                                      \
  }                                                                           \
                                                                              \
  M_P(void, name, _clear, deque_t d)                                          \
  {                                                                           \
    M_D3QU3_CONTRACT(d);                                                      \
    M_F(name, _reset)M_R(d);                                                  \
    /* We have registered the delete operator to clear all node objects */    \
    M_F(name, _node_list_clear)M_R(d->list);                                  \
    /* It is safer to clean some variables to mark d as invalid */            \
    d->front->node  = NULL;                                                   \
    d->back->node   = NULL;                                                   \
    d->count = 0;                                                             \
  }                                                                           \
                                                                              \
  M_P(type *, name, _push_back_raw, deque_t d)                                \
  {                                                                           \
    M_D3QU3_CONTRACT(d);                                                      \
    node_t *n = d->back->node;                                                \
    size_t index = d->back->index;                                            \
    if (M_UNLIKELY (n->size <= index)) {                                      \
      /* Cannot fit in the current node, try to get another allocated node */ \
      n = M_F(name, _node_list_next_obj)(d->list, n);                         \
      if (n == NULL) {                                                        \
        /* No another node exists: allocate a new one */                      \
        n = M_F(name,_i_new_node)M_R(d);                                      \
        M_F(name, _node_list_push_back)(d->list, n);                          \
      }                                                                       \
      d->back->node = n;                                                      \
      index = 0;                                                              \
    }                                                                         \
    type *ret = &n->data[index];                                              \
    index++;                                                                  \
    d->count ++;                                                              \
    d->back->index = index;                                                   \
    M_D3QU3_CONTRACT(d);                                                      \
    return ret;                                                               \
  }                                                                           \
                                                                              \
  M_IF_METHOD(INIT_SET, oplist)(                                              \
  M_P(void, name, _push_back, deque_t d, type const x)                        \
  {                                                                           \
    type *p = M_F(name, _push_back_raw)M_R(d);                                \
    M_ON_EXCEPTION(d->back->index --, d->count--) {                           \
      M_CALL_INIT_SET(oplist, *p, x);                                         \
    }                                                                         \
  }                                                                           \
  , /* No INIT_SET */ )                                                       \
                                                                              \
  M_IF_METHOD(INIT, oplist)(                                                  \
  M_P(type *, name, _push_back_new, deque_t d)                                \
  {                                                                           \
    type *p = M_F(name, _push_back_raw)M_R(d);                                \
    M_ON_EXCEPTION(d->back->index --, d->count--) {                           \
      M_CALL_INIT(oplist, *p);                                                \
    }                                                                         \
    return p;                                                                 \
  }                                                                           \
  , /* No INIT */)                                                            \
                                                                              \
  M_P(void, name, _push_back_move, deque_t d, type *x)                        \
  {                                                                           \
    M_ASSERT (x != NULL);                                                     \
    type *p = M_F(name, _push_back_raw)M_R(d);                                \
    M_CALL_INIT_MOVE (oplist, *p, *x);                                        \
  }                                                                           \
                                                                              \
  M_P(type *, name, _push_front_raw, deque_t d)                               \
  {                                                                           \
    M_D3QU3_CONTRACT(d);                                                      \
    node_t *n = d->front->node;                                               \
    size_t index = d->front->index;                                           \
    index --;                                                                 \
    /* If overflow */                                                         \
    if (M_UNLIKELY (n->size <= index)) {                                      \
      n = M_F(name, _node_list_previous_obj)(d->list, n);                     \
      if (n == NULL) {                                                        \
        n = M_F(name,_i_new_node)M_R(d);                                      \
        M_F(name, _node_list_push_front)(d->list, n);                         \
      }                                                                       \
      d->front->node = n;                                                     \
      index = n->size -1;                                                     \
    }                                                                         \
    type *ret = &n->data[index];                                              \
    d->count ++;                                                              \
    d->front->index = index;                                                  \
    M_D3QU3_CONTRACT(d);                                                      \
    return ret;                                                               \
  }                                                                           \
                                                                              \
  M_IF_METHOD(INIT_SET, oplist)(                                              \
  M_P(void, name, _push_front, deque_t d, type const x)                       \
  {                                                                           \
    type *p = M_F(name, _push_front_raw)M_R(d);                               \
    M_ON_EXCEPTION(d->front->index ++, d->count--) {                          \
      M_CALL_INIT_SET(oplist, *p, x);                                         \
    }                                                                         \
  }                                                                           \
  , /* No INIT_SET */ )                                                       \
                                                                              \
  M_IF_METHOD(INIT, oplist)(                                                  \
  M_P(type *, name, _push_front_new, deque_t d)                               \
  {                                                                           \
    type *p = M_F(name, _push_front_raw)M_R(d);                               \
    M_ON_EXCEPTION(d->front->index ++, d->count--) {                          \
      M_CALL_INIT(oplist, *p);                                                \
    }                                                                         \
    return p;                                                                 \
  }                                                                           \
  ,)                                                                          \
                                                                              \
  M_P(void, name, _push_front_move, deque_t d, type *x)                       \
  {                                                                           \
    M_ASSERT (x != NULL);                                                     \
    type *p = M_F(name, _push_front_raw)M_R(d);                               \
    M_CALL_INIT_MOVE (oplist, *p, *x);                                        \
  }                                                                           \
                                                                              \
  M_P(void, name, _pop_back, type *ptr, deque_t d)                            \
  {                                                                           \
    M_D3QU3_CONTRACT(d);                                                      \
    M_ASSERT(d->count > 0);                                                   \
    M_UNUSED_CONTEXT();                                                       \
    node_t *n = d->back->node;                                                \
    size_t index = d->back->index;                                            \
    index --;                                                                 \
    if (M_UNLIKELY (n->size <= index)) {                                      \
      /* If there is a next node,                                             \
         pop the back node and push it back to the front. This                \
         reduce the used memory if the deque is used as a FIFO queue.*/       \
      node_t *next = M_F(name, _node_list_next_obj)(d->list, n);              \
      if (next != NULL) {                                                     \
        next = M_F(name, _node_list_pop_back)(d->list);                       \
        M_ASSERT (next != n);                                                 \
        M_F(name, _node_list_push_front)(d->list, next);                      \
      }                                                                       \
      n = M_F(name, _node_list_previous_obj)(d->list, n);                     \
      M_ASSERT (n != NULL && n->size > 1);                                    \
      d->back->node = n;                                                      \
      index = n->size-1;                                                      \
    }                                                                         \
    if (ptr != NULL)                                                          \
      M_DO_MOVE(oplist, *ptr, n->data[index]);                                \
    else                                                                      \
      M_CALL_CLEAR(oplist,  n->data[index]);                                  \
    d->count --;                                                              \
    d->back->index = index;                                                   \
    M_D3QU3_CONTRACT(d);                                                      \
  }                                                                           \
                                                                              \
  M_IF_METHOD(INIT, oplist)(                                                  \
  M_P(void, name, _pop_back_move, type *ptr, deque_t d)                       \
  {                                                                           \
    M_ASSERT(ptr != NULL);                                                    \
    /* Note: Lazy implementation. Can be improved if needed */                \
    M_CALL_INIT(oplist, *ptr);                                                \
    M_F(name, _pop_back)M_R(ptr, d);                                          \
  }                                                                           \
  , )                                                                         \
                                                                              \
  M_P(void, name, _pop_front, type *ptr, deque_t d)                           \
  {                                                                           \
    M_D3QU3_CONTRACT(d);                                                      \
    M_ASSERT(d->count > 0);                                                   \
    M_UNUSED_CONTEXT();                                                       \
    node_t *n = d->front->node;                                               \
    size_t index = d->front->index;                                           \
    if (ptr != NULL)                                                          \
      M_DO_MOVE(oplist, *ptr, n->data[index]);                                \
    else                                                                      \
      M_CALL_CLEAR(oplist, n->data[index]);                                   \
    index++;                                                                  \
    if (M_UNLIKELY (n->size <= index)) {                                      \
      /* If there is a previous node,                                         \
         pop the front node and push it back to the back. This                \
         recycles the used memory if the deque is used as a FIFO queue.*/     \
      node_t *prev = M_F(name, _node_list_previous_obj)(d->list, n);          \
      if (prev != NULL) {                                                     \
        prev = M_F(name, _node_list_pop_front)(d->list);                      \
        M_ASSERT (prev != n);                                                 \
        M_F(name, _node_list_push_back)(d->list, prev);                       \
      }                                                                       \
      /* Update front iterator to point to the next object */                 \
      n = M_F(name, _node_list_next_obj)(d->list, n);                         \
      if (M_UNLIKELY(n == NULL)) {                                            \
        /* No next obj.                                                       \
           It is only possible if there was only 1 element */                 \
        M_ASSERT(d->count == 1);                                              \
        /* Reset the deque to the middle of the current node */               \
        d->back->node = d->front->node;                                       \
        index = d->front->node->size/2;                                       \
        d->back->index = d->front->node->size/2;                              \
      } else {                                                                \
        d->front->node = n;                                                   \
        index = 0;                                                            \
      }                                                                       \
    }                                                                         \
    d->count --;                                                              \
    d->front->index = index;                                                  \
    M_D3QU3_CONTRACT(d);                                                      \
  }                                                                           \
                                                                              \
  M_IF_METHOD(INIT, oplist)(                                                  \
  M_P(void, name, _pop_front_move, type *ptr, deque_t d)                      \
  {                                                                           \
    M_ASSERT(ptr != NULL);                                                    \
    /* Note: Lazy implementation */                                           \
    M_CALL_INIT(oplist, *ptr);                                                \
    M_F(name, _pop_front)M_R(ptr, d);                                         \
  }                                                                           \
  ,)                                                                          \
                                                                              \
  M_INLINE type *                                                             \
  M_F(name, _back)(const deque_t d)                                           \
  {                                                                           \
    M_D3QU3_CONTRACT(d);                                                      \
    M_ASSERT (d->count > 0);                                                  \
    size_t i = d->back->index;                                                \
    node_t *n = d->back->node;                                                \
    if (M_UNLIKELY (i == 0)) {                                                \
      n = M_F(name, _node_list_previous_obj)(d->list, n);                     \
      M_ASSERT (n != NULL);                                                   \
      i = n->size;                                                            \
    }                                                                         \
    return &n->data[i-1];                                                     \
  }                                                                           \
                                                                              \
  M_INLINE type *                                                             \
  M_F(name, _front)(const deque_t d)                                          \
  {                                                                           \
    M_D3QU3_CONTRACT(d);                                                      \
    M_ASSERT (d->count > 0);                                                  \
    size_t i = d->front->index;                                               \
    node_t *n = d->front->node;                                               \
    return &n->data[i];                                                       \
  }                                                                           \
                                                                              \
  M_INLINE size_t                                                             \
  M_F(name, _size)(const deque_t d)                                           \
  {                                                                           \
    M_D3QU3_CONTRACT(d);                                                      \
    return d->count;                                                          \
  }                                                                           \
                                                                              \
  M_INLINE size_t                                                             \
  M_F(name, _capacity_back)(const deque_t v)                                  \
  {                                                                           \
    M_D3QU3_CONTRACT(v);                                                      \
    size_t s = 0;                                                             \
    for(node_t *n = M_F(name, _node_list_back)(v->list);                      \
        n != NULL ;                                                           \
        n = (n == v->back->node) ? NULL :                                     \
          M_F(name, _node_list_previous_obj)(v->list, n) ){                   \
      s += (n == v->back->node ? v->back->index : n->size);                   \
    }                                                                         \
    return s;                                                                 \
  }                                                                           \
                                                                              \
  M_INLINE size_t                                                             \
  M_F(name, _capacity_front)(const deque_t v)                                 \
  {                                                                           \
    M_D3QU3_CONTRACT(v);                                                      \
    size_t s = 0;                                                             \
    for(node_t *n = M_F(name, _node_list_front)(v->list);                     \
        n != NULL ;                                                           \
        n = (n == v->front->node) ? NULL :                                    \
          M_F(name, _node_list_next_obj)(v->list, n) ){                       \
      s += n->size - (n == v->front->node ? v->front->index : 0);             \
    }                                                                         \
    return s;                                                                 \
  }                                                                           \
                                                                              \
  M_INLINE size_t                                                             \
  M_F(name, _capacity)(const deque_t v)                                       \
  {                                                                           \
    return M_F(name, _capacity_back)(v)+M_F(name, _capacity_front)(v);        \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _empty_p)(const deque_t d)                                        \
  {                                                                           \
    M_D3QU3_CONTRACT(d);                                                      \
    return d->count == 0;                                                     \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it)(it_t it, const deque_t d)                                    \
  {                                                                           \
    M_D3QU3_CONTRACT(d);                                                      \
    M_ASSERT (it != NULL);                                                    \
    it->node  = d->front->node;                                               \
    it->index = d->front->index;                                              \
    it->deque = d;                                                            \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_last)(it_t it, const deque_t d)                               \
  {                                                                           \
    M_D3QU3_CONTRACT(d);                                                      \
    M_ASSERT (it != NULL);                                                    \
    it->node  = d->back->node;                                                \
    it->index = d->back->index - 1;                                           \
    it->deque = d;                                                            \
    if (M_UNLIKELY (it->index >= it->node->size)) {                           \
      it->node = M_F(name, _node_list_previous_obj)(d->list, it->node);       \
      M_ASSERT (it->node != NULL && it->node->size > 1);                      \
      it->index = it->node->size-1;                                           \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_end)(it_t it, const deque_t d)                                \
  {                                                                           \
    M_D3QU3_CONTRACT(d);                                                      \
    M_ASSERT (it != NULL);                                                    \
    it->node  = d->back->node;                                                \
    it->index = d->back->index;                                               \
    it->deque = d;                                                            \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_set)(it_t it1, const it_t it2)                                \
  {                                                                           \
    M_ASSERT (it1 != NULL);                                                   \
    M_ASSERT (it2 != NULL);                                                   \
    it1->node  = it2->node;                                                   \
    it1->index = it2->index;                                                  \
    it1->deque = it2->deque;                                                  \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _end_p)(const it_t it)                                            \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    return (it->node == it->deque->back->node                                 \
            && it->index >= it->deque->back->index)                           \
      || (it->node == it->deque->front->node                                  \
          && it->index < it->deque->front->index);                            \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _next)(it_t it)                                                   \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    node_t *n = it->node;                                                     \
    it->index ++;                                                             \
    if (M_UNLIKELY (it->index >= n->size)) {                                  \
      n = M_F(name, _node_list_next_obj)(it->deque->list, n);                 \
      if (M_UNLIKELY (n == NULL || it->node == it->deque->back->node)) {      \
        /* Point to 'end' (can't undo it) */                                  \
        it->node  = it->deque->back->node;                                    \
        it->index = it->deque->back->node->size;                              \
        return;                                                               \
      }                                                                       \
      it->node = n;                                                           \
      it->index = 0;                                                          \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _previous)(it_t it)                                               \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    node_t *n = it->node;                                                     \
    it->index --;                                                             \
    if (M_UNLIKELY (it->index >= n->size)) {                                  \
      n = M_F(name, _node_list_previous_obj)(it->deque->list, n);             \
      if (M_UNLIKELY (n == NULL || it->node == it->deque->front->node)) {     \
        /* Point to 'end' (can't undo it) */                                  \
        it->node  = it->deque->back->node;                                    \
        it->index = it->deque->back->node->size;                              \
        return;                                                               \
      }                                                                       \
      it->node = n;                                                           \
      it->index = n->size - 1;                                                \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _last_p)(const it_t it)                                           \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    it_t it2;                                                                 \
    M_F(name, _it_set)(it2, it);                                              \
    M_F(name, _next)(it2);                                                    \
    return M_F(name, _end_p)(it2);                                            \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _it_equal_p)(const it_t it1, const it_t it2)                      \
  {                                                                           \
    M_ASSERT (it1 != NULL);                                                   \
    M_ASSERT (it2 != NULL);                                                   \
    return it1->deque == it2->deque                                           \
      && it1->node == it2->node                                               \
      && it1->index == it2->index;                                            \
  }                                                                           \
                                                                              \
  M_INLINE type *                                                             \
  M_F(name, _ref)(const it_t it)                                              \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    M_ASSERT (it->index < it->node->size);                                    \
    return &it->node->data[it->index];                                        \
  }                                                                           \
                                                                              \
  M_INLINE type const *                                                       \
  M_F(name, _cref)(const it_t it)                                             \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    M_ASSERT (it->index < it->node->size);                                    \
    return M_CONST_CAST(type, &it->node->data[it->index]);                    \
  }                                                                           \
                                                                              \
  M_P(void, name, _remove, deque_t d, it_t it)                                \
  {                                                                           \
    M_D3QU3_CONTRACT(d);                                                      \
    M_ASSERT (it != NULL && it->node != NULL);                                \
    M_ASSERT (d->count >= 1);                                                 \
    M_ASSERT_INDEX(it->index, it->node->size);                                \
                                                                              \
    node_t *n = it->node;                                                     \
    M_CALL_CLEAR(oplist, n->data[it->index]);                                 \
    if (n == d->back->node) {                                                 \
      /* back index points to the element after the end */                    \
      M_ASSERT (d->back->index != 0);                                         \
      M_ASSERT (it->index < d->back->index);                                  \
      /* We cannot have a node deletion in this case */                       \
      memmove(&n->data[it->index], &n->data[it->index+1],                     \
              sizeof(type) * (d->back->index - it->index - 1));               \
      d->back->index --;                                                      \
      /* The iterator points to the next element */                           \
    } else if (n == d->front->node) {                                         \
      /* front index points to the first element */                           \
      if (M_UNLIKELY (d->front->index == n->size -1)) {                       \
        /* Node 'smart' deletion (single element) */                          \
        /* Update front iterator to point to the next object */               \
        n = M_F(name, _node_list_next_obj)(d->list, n);                       \
        /* We must have a next element, as we have a different back node      \
           than the front node. */                                            \
        M_ASSERT (n != NULL);                                                 \
        d->front->node = n;                                                   \
        d->front->index = 0;                                                  \
        /* The iterator references this element */                            \
        it->node = n;                                                         \
        it->index = 0;                                                        \
      } else {                                                                \
        memmove(&n->data[d->front->index+1], &n->data[d->front->index],       \
                sizeof(type) * (it->index - d->front->index));                \
        d->front->index ++;                                                   \
        /* The iterator shall reference the next element */                   \
        M_F(name, _next)(it);                                                 \
      }                                                                       \
    } else {                                                                  \
      /* Nether front or end node */                                          \
      if (M_UNLIKELY(n->size == 1)) {                                         \
        /* The iterator shall reference the next element */                   \
        M_F(name, _next)(it);                                                 \
        /* Node deletion */                                                   \
        M_ASSERT(d->count > 1);                                               \
        M_F(name, _node_list_unlink)(n);                                      \
        M_CALL_FREE(oplist, char, n, sizeof(node_t) + n->capacity * sizeof(type) ); \
      } else {                                                                \
        memmove(&n->data[it->index], &n->data[it->index+1],                   \
                sizeof(type) * (it->node->size - it->index - 1));             \
        /* We lose capacity of the node... */                                 \
        n->size --;                                                           \
        /* The iterator points to the next element                            \
           except if it was the last one*/                                    \
        if (M_UNLIKELY(it->index >= n->size)) M_F(name, _next)(it);           \
      }                                                                       \
    }                                                                         \
    d->count--;                                                               \
    M_D3QU3_CONTRACT(d);                                                      \
  }                                                                           \
                                                                              \
  M_IF_METHOD(INIT_SET, oplist)(                                              \
  M_P(void, name, _init_set, deque_t d, const deque_t src)                    \
  {                                                                           \
    M_D3QU3_CONTRACT(src);                                                    \
    M_ASSERT (d != NULL);                                                     \
    M_F(name, _node_list_init)(d->list);                                      \
    d->default_size = M_USE_DEQUE_DEFAULT_SIZE + src->count;                  \
    node_t *n = M_F(name,_i_new_node)M_R(d);                                  \
    d->count        = src->count;                                             \
    d->default_size /= 2;                                                     \
    M_F(name, _node_list_push_back)(d->list, n);                              \
    d->front->node  = n;                                                      \
    d->front->index = M_USE_DEQUE_DEFAULT_SIZE/2;                             \
    d->back->node   = n;                                                      \
    d->back->index  = M_USE_DEQUE_DEFAULT_SIZE/2 + src->count;                \
    it_t it;                                                                  \
    size_t m_volatile i = M_USE_DEQUE_DEFAULT_SIZE/2;                         \
    M_ON_EXCEPTION(d->count = i - M_USE_DEQUE_DEFAULT_SIZE/2, d->back->index = i, M_F(name, _clear)(d) ) \
    for(M_F(name, _it)(it, src); !M_F(name, _end_p)(it) ; M_F(name, _next)(it)) { \
      type const *obj = M_F(name, _cref)(it);                                 \
      M_CALL_INIT_SET(oplist, n->data[i], *obj);                              \
      i++;                                                                    \
      M_ASSERT (i <= d->back->index);                                         \
    }                                                                         \
    M_D3QU3_CONTRACT(d);                                                      \
  }                                                                           \
                                                                              \
  M_P(void, name, _set, deque_t d, deque_t const src)                         \
  {                                                                           \
    if (M_UNLIKELY (src == d))                                                \
      return;                                                                 \
    /* TODO: Reuse memory of d! */                                            \
    M_ON_EXCEPTION( M_F(name, _init)(d) ) {                                   \
      M_F(name, _clear)M_R(d);                                                \
      M_F(name, _init_set)M_R(d, src);                                        \
    }                                                                         \
  }                                                                           \
  , /* No INIT_SET */ )                                                       \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _init_move)(deque_t d, deque_t src)                               \
  {                                                                           \
    M_D3QU3_CONTRACT(src);                                                    \
    M_ASSERT (d!= NULL);                                                      \
    M_F(name,_node_list_init_move)(d->list, src->list);                       \
    d->front->node  = src->front->node;                                       \
    d->front->index = src->front->index;                                      \
    d->back->node   = src->back->node;                                        \
    d->back->index  = src->back->index;                                       \
    d->default_size = src->default_size;                                      \
    d->count        = src->count;                                             \
    memset(src, 0, sizeof(deque_t));                                          \
    M_D3QU3_CONTRACT(d);                                                      \
  }                                                                           \
                                                                              \
  M_P(void, name, _move, deque_t d, deque_t src)                              \
  {                                                                           \
    M_D3QU3_CONTRACT(d);                                                      \
    M_D3QU3_CONTRACT(src);                                                    \
    M_F(name, _clear)M_R(d);                                                  \
    M_F(name, _init_move)(d, src);                                            \
    M_D3QU3_CONTRACT(d);                                                      \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _swap)(deque_t d, deque_t e)                                      \
  {                                                                           \
    M_D3QU3_CONTRACT(d);                                                      \
    M_D3QU3_CONTRACT(e);                                                      \
    M_F(name, _node_list_swap) (d->list, e->list);                            \
    M_SWAP(node_t *, d->front->node, e->front->node);                         \
    M_SWAP(node_t *, d->back->node, e->back->node);                           \
    M_SWAP(size_t, d->front->index, e->front->index);                         \
    M_SWAP(size_t, d->back->index, e->back->index);                           \
    M_SWAP(size_t, d->default_size, e->default_size);                         \
    M_SWAP(size_t, d->count, e->count);                                       \
    M_D3QU3_CONTRACT(d);                                                      \
    M_D3QU3_CONTRACT(e);                                                      \
  }                                                                           \
                                                                              \
  M_INLINE type*                                                              \
  M_F(name, _get)(const deque_t d, size_t key)                                \
  {                                                                           \
    M_D3QU3_CONTRACT(d);                                                      \
    M_ASSERT_INDEX (key, d->count);                                           \
    const size_t index0 = d->front->index;                                    \
    size_t count = 0;                                                         \
    /* This loop is in log(N) since the size increase exponentially.*/        \
    for(node_t *n = d->front->node; true ;                                    \
        n = (n == d->back->node) ? NULL :                                     \
          M_F(name, _node_list_next_obj)(d->list, n) ){                       \
      M_ASSERT(n != NULL);                                                    \
      if (index0 + key < count + n->size) {                                   \
        return &n->data[index0 + key - count];                                \
      }                                                                       \
      count += n->size;                                                       \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_INLINE type const *                                                       \
  M_F(name, _cget)(const deque_t d, size_t key)                               \
  {                                                                           \
    return M_CONST_CAST(type, M_F(name, _get)(d, key));                       \
  }                                                                           \
                                                                              \
  M_IF_METHOD(SET, oplist)(                                                   \
  M_P(void, name, _set_at, deque_t d, size_t key, type const x)               \
  {                                                                           \
    M_D3QU3_CONTRACT(d);                                                      \
    M_UNUSED_CONTEXT();                                                       \
    M_ASSERT_INDEX (key, d->count);                                           \
    type *p = M_F(name, _get)(d, key);                                        \
    M_CALL_SET(oplist, *p, x);                                                \
    M_D3QU3_CONTRACT(d);                                                      \
  }                                                                           \
  , /* No SET */ )                                                            \
                                                                              \
  M_IF_METHOD(EQUAL, oplist)(                                                 \
  M_INLINE bool                                                               \
  M_F(name, _equal_p)(const deque_t d1, const deque_t d2)                     \
  {                                                                           \
    M_D3QU3_CONTRACT(d1);                                                     \
    M_D3QU3_CONTRACT(d2);                                                     \
    if (d1->count != d2->count)                                               \
      return false;                                                           \
    it_t it1;                                                                 \
    it_t it2;                                                                 \
    for(M_F(name, _it)(it1, d1), M_F(name,_it)(it2, d2);                      \
        !M_F(name, _end_p)(it1) ;                                             \
        M_F(name, _next)(it1), M_F(name, _next)(it2)) {                       \
      type const *obj1 = M_F(name, _cref)(it1);                               \
      type const *obj2 = M_F(name, _cref)(it2);                               \
      if (M_CALL_EQUAL(oplist, *obj1, *obj2) == false)                        \
        return false;                                                         \
    }                                                                         \
    M_ASSERT (M_F(name, _end_p)(it2));                                        \
    return true;                                                              \
  }                                                                           \
  , /* NO EQUAL */)                                                           \
                                                                              \
  M_IF_METHOD(HASH, oplist)(                                                  \
  M_INLINE size_t                                                             \
  M_F(name, _hash)(const deque_t d)                                           \
  {                                                                           \
    M_D3QU3_CONTRACT(d);                                                      \
    M_HASH_DECL(hash);                                                        \
    it_t it;                                                                  \
    for(M_F(name, _it)(it, d); !M_F(name, _end_p)(it); M_F(name, _next)(it)) { \
      type const *obj = M_F(name, _cref)(it);                                 \
      M_HASH_UP (hash, M_CALL_HASH(oplist, *obj));                            \
    }                                                                         \
    return M_HASH_FINAL(hash);                                                \
  }                                                                           \
  , /* NO HASH */)                                                            \
                                                                              \
  M_IF_METHOD(SWAP, oplist)(                                                  \
  M_INLINE void                                                               \
  M_F(name, _swap_at)(deque_t d, size_t i, size_t j)                          \
  {                                                                           \
    M_D3QU3_CONTRACT(d);                                                      \
    M_ASSERT_INDEX (i, d->count);                                             \
    M_ASSERT_INDEX (j, d->count);                                             \
    type *obj1 = M_F(name, _get)(d, i);                                       \
    type *obj2 = M_F(name, _get)(d, j);                                       \
    M_CALL_SWAP(oplist, *obj1, *obj2);                                        \
    M_D3QU3_CONTRACT(d);                                                      \
  }                                                                           \
  , /* NO SWAP */)                                                            \
                                                                              \
  M_IF_METHOD(GET_STR, oplist)(                                               \
  M_P(void, name, _get_str, m_string_t str, deque_t const deque, bool append) \
  {                                                                           \
    M_D3QU3_CONTRACT(deque);                                                  \
    (append ? m_string_cat_cstr : m_string_set_cstr) M_R(str, "[");           \
    it_t it;                                                                  \
    for (M_F(name, _it)(it, deque) ;                                          \
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
  M_F(name, _out_str)(FILE *file, const deque_t deque)                        \
  {                                                                           \
    M_D3QU3_CONTRACT(deque);                                                  \
    M_ASSERT (file != NULL);                                                  \
    fputc ('[', file);                                                        \
    it_t it;                                                                  \
    for (M_F(name, _it)(it, deque) ;                                          \
         !M_F(name, _end_p)(it);                                              \
         M_F(name, _next)(it)) {                                              \
      type const *item = M_F(name, _cref)(it);                                \
      M_CALL_OUT_STR(oplist, file, *item);                                    \
      if (!M_F(name, _last_p)(it))                                            \
        fputc (M_GET_SEPARATOR oplist, file);                                 \
    }                                                                         \
    fputc (']', file);                                                        \
  }                                                                           \
  , /* no OUT_STR */ )                                                        \
                                                                              \
  M_IF_METHOD3(PARSE_STR, INIT, INIT_SET, oplist)(                            \
  M_P(bool, name, _parse_str, deque_t deque, const char str[], const char **endp) \
  {                                                                           \
    M_D3QU3_CONTRACT(deque);                                                  \
    M_ASSERT (str != NULL);                                                   \
    M_F(name,_reset)M_R(deque);                                               \
    int c = *str++;                                                           \
    if (M_UNLIKELY (c != '[')) { c = 0; goto exit; }                          \
    c = *str++;                                                               \
    if (M_UNLIKELY (c == ']' || c == 0)) goto exit;                           \
    str--;                                                                    \
    M_QLET(1, item, type, oplist) {                                           \
      do {                                                                    \
        bool b = M_CALL_PARSE_STR(oplist, item, str, &str);                   \
        c = m_core_str_nospace(&str);                                         \
        if (b == false || c == 0) { c = 0; break; }                           \
        M_F(name, _push_back)M_R(deque, item);                                \
      } while (c == M_GET_SEPARATOR oplist);                                  \
    }                                                                         \
  exit:                                                                       \
    if (endp) *endp = str;                                                    \
    M_D3QU3_CONTRACT(deque);                                                  \
    return c == ']';                                                          \
  }                                                                           \
  , /* no PARSE_STR */ )                                                      \
                                                                              \
  M_IF_METHOD3(IN_STR, INIT, INIT_SET, oplist)(                               \
  M_P(bool, name, _in_str, deque_t deque, FILE *file)                         \
  {                                                                           \
    M_D3QU3_CONTRACT(deque);                                                  \
    M_ASSERT (file != NULL);                                                  \
    M_F(name,_reset)M_R(deque);                                               \
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
        M_F(name, _push_back)M_R(deque, item);                                \
      } while (c == M_GET_SEPARATOR oplist);                                  \
    }                                                                         \
    M_D3QU3_CONTRACT(deque);                                                  \
    return c == ']';                                                          \
  }                                                                           \
  , /* no IN_STR */ )                                                         \
                                                                              \
  M_IF_METHOD(OUT_SERIAL, oplist)(                                            \
  M_P(m_serial_return_code_t, name, _out_serial, m_serial_write_t f, const deque_t deque) \
  {                                                                           \
    M_D3QU3_CONTRACT(deque);                                                  \
    M_ASSERT (f != NULL && f->m_interface != NULL);                           \
    m_serial_local_t local;                                                   \
    m_serial_return_code_t ret;                                               \
    bool first_done = false;                                                  \
    ret = f->m_interface->write_array_start M_R(local, f, deque->count);      \
    M_F(name, _it_ct) it;                                                     \
    for (M_F(name, _it)(it, deque) ;                                          \
         !M_F(name, _end_p)(it);                                              \
         M_F(name, _next)(it)){                                               \
      type const *item = M_F(name, _cref)(it);                                \
      if (first_done)                                                         \
        ret |= f->m_interface->write_array_next M_R(local, f);                \
      ret |= M_CALL_OUT_SERIAL(oplist, f, *item);                             \
      first_done = true;                                                      \
    }                                                                         \
    ret |= f->m_interface->write_array_end M_R(local, f);                     \
    return ret & M_SERIAL_FAIL;                                               \
  }                                                                           \
  , /* no OUT_SERIAL */ )                                                     \
                                                                              \
  M_IF_METHOD3(IN_SERIAL, INIT, INIT_SET, oplist)(                            \
  M_P(m_serial_return_code_t, name, _in_serial, deque_t deque, m_serial_read_t f) \
  {                                                                           \
    M_D3QU3_CONTRACT(deque);                                                  \
    M_ASSERT (f != NULL && f->m_interface != NULL);                           \
    m_serial_local_t local;                                                   \
    m_serial_return_code_t ret;                                               \
    size_t estimated_size = 0;                                                \
    M_F(name,_reset)M_R(deque);                                               \
    ret = f->m_interface->read_array_start(local, f, &estimated_size);        \
    if (M_UNLIKELY (ret != M_SERIAL_OK_CONTINUE)) return ret;                 \
    M_QLET(1, item, type, oplist) {                                           \
      do {                                                                    \
        ret = M_CALL_IN_SERIAL(oplist, item, f);                              \
        if (ret != M_SERIAL_OK_DONE) { break; }                               \
        M_F(name, _push_back)M_R(deque, item);                                \
        ret = f->m_interface->read_array_next(local, f);                      \
      } while (ret == M_SERIAL_OK_CONTINUE);                                  \
    }                                                                         \
    return ret;                                                               \
  }                                                                           \
  , /* no IN_SERIAL & INIT */ )                                               \

/* Definition of the emplace_back function for deque */
#define M_D3QUE_EMPLACE_BACK_DEF(name, name_t, function_name, oplist, init_func, exp_emplace_type) \
  M_P(void, name, function_name, name_t v M_EMPLACE_LIST_TYPE_VAR(a, exp_emplace_type) ) \
  {                                                                           \
    M_F(name, _subtype_ct) *data = M_F(name, _push_back_raw)M_R(v);           \
    M_ON_EXCEPTION(v->back->index --, v->count--) {                           \
      M_EMPLACE_CALL_FUNC(a, init_func, oplist, *data, exp_emplace_type);     \
    }                                                                         \
  }


/* Definition of the emplace_front function for deque */
#define M_D3QUE_EMPLACE_FRONT_DEF(name, name_t, function_name, oplist, init_func, exp_emplace_type) \
  M_P(void, name, function_name, name_t v M_EMPLACE_LIST_TYPE_VAR(a, exp_emplace_type) ) \
  {                                                                           \
    M_F(name, _subtype_ct) *data = M_F(name, _push_front_raw)M_R(v);          \
    M_ON_EXCEPTION(v->front->index ++, v->count--) {                          \
      M_EMPLACE_CALL_FUNC(a, init_func, oplist, *data, exp_emplace_type);     \
    }                                                                         \
  }


/********************************* INTERNAL **********************************/

/* Deferred evaluation for the oplist definition,
   so that all arguments are evaluated before further expansion */
#define M_D3QU3_OPLIST_P1(arg) M_D3QU3_OPLIST_P2 arg

/* Validation of the given oplist */
#define M_D3QU3_OPLIST_P2(name, oplist)                                       \
  M_IF_OPLIST(oplist)(M_D3QU3_OPLIST_P3, M_D3QU3_OPLIST_FAILURE)(name, oplist)

/* Prepare a clean compilation failure */
#define M_D3QU3_OPLIST_FAILURE(name, oplist)                                  \
  ((M_LIB_ERROR(ARGUMENT_OF_DEQUE_OPLIST_IS_NOT_AN_OPLIST, name, oplist)))

/* OPLIST definition of a deque */
#ifndef M_USE_CONTEXT
#define M_D3QU3_OPLIST_P3(name, oplist)                                       \
  (INIT(M_F(name, _init))                                                     \
   ,INIT_SET(M_F(name, _init_set))                                            \
   ,INIT_WITH(API_1(M_INIT_WITH_VAI))                                         \
   ,SET(M_F(name, _set))                                                      \
   ,CLEAR(M_F(name, _clear))                                                  \
   ,INIT_MOVE(M_F(name, _init_move))                                          \
   ,MOVE(M_F(name, _move))                                                    \
   ,SWAP(M_F(name, _swap))                                                    \
   ,NAME(name)                                                                \
   ,TYPE(M_F(name,_ct)), GENTYPE(struct M_F(name,_s)*)                        \
   ,SUBTYPE(M_F(name, _subtype_ct))                                           \
   ,EMPTY_P(M_F(name,_empty_p))                                               \
   ,IT_TYPE(M_F(name,_it_ct))                                                 \
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
   ,IT_REMOVE(M_F(name,_remove))                                              \
   ,RESET(M_F(name,_reset))                                                   \
   ,GET_SIZE(M_F(name, _size))                                                \
   ,PUSH(M_F(name,_push_back))                                                \
   ,POP(M_F(name,_pop_front))                                                 \
   ,OPLIST(oplist)                                                            \
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
#define M_D3QU3_OPLIST_P3(name, oplist)                                       \
  (INIT(API_0P(M_F(name, _init)))                                             \
   ,INIT_SET(API_0P(M_F(name, _init_set)))                                    \
   ,INIT_WITH(API_1(M_INIT_WITH_VAI))                                         \
   ,SET(API_0P(M_F(name, _set)))                                              \
   ,CLEAR(API_0P(M_F(name, _clear)))                                          \
   ,INIT_MOVE(M_F(name, _init_move))                                          \
   ,MOVE(API_0P(M_F(name, _move)))                                            \
   ,SWAP(M_F(name, _swap))                                                    \
   ,NAME(name)                                                                \
   ,TYPE(M_F(name,_ct)), GENTYPE(struct M_F(name,_s)*)                        \
   ,SUBTYPE(M_F(name, _subtype_ct))                                           \
   ,EMPTY_P(M_F(name,_empty_p))                                               \
   ,IT_TYPE(M_F(name,_it_ct))                                                 \
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
   ,IT_REMOVE(API_0P(M_F(name,_remove)))                                      \
   ,RESET(API_0P(M_F(name,_reset)))                                           \
   ,GET_SIZE(M_F(name, _size))                                                \
   ,PUSH(API_0P(M_F(name,_push_back)))                                        \
   ,POP(API_0P(M_F(name,_pop_front)))                                         \
   ,OPLIST(oplist)                                                            \
   ,M_IF_METHOD(GET_STR, oplist)(API_0P(GET_STR(M_F(name, _get_str))),)       \
   ,M_IF_METHOD(PARSE_STR, oplist)(API_0P(PARSE_STR(M_F(name, _parse_str))),) \
   ,M_IF_METHOD(OUT_STR, oplist)(OUT_STR(M_F(name, _out_str)),)               \
   ,M_IF_METHOD(IN_STR, oplist)(IN_STR(API_0P(M_F(name, _in_str))),)          \
   ,M_IF_METHOD(OUT_SERIAL, oplist)(OUT_SERIAL(API_0P(M_F(name, _out_serial))),) \
   ,M_IF_METHOD(IN_SERIAL, oplist)(IN_SERIAL(API_0P(M_F(name, _in_serial))),) \
   ,M_IF_METHOD(EQUAL, oplist)(EQUAL(M_F(name, _equal_p)),)                   \
   ,M_IF_METHOD(HASH, oplist)(HASH(M_F(name, _hash)),)                        \
   )
#endif

/********************************* INTERNAL **********************************/

#if M_USE_SMALL_NAME
#define DEQUE_DEF M_DEQUE_DEF
#define DEQUE_DEF_AS M_DEQUE_DEF_AS
#define DEQUE_OPLIST M_DEQUE_OPLIST
#endif

#endif
