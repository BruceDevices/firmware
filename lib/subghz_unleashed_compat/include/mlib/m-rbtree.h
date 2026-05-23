/*
 * M*LIB - RED BLACK TREE module
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
#ifndef MSTARLIB_RBTREE_H
#define MSTARLIB_RBTREE_H

#include "m-core.h"

/* Define a Red/Black binary tree of a given type.
   USAGE: RBTREE_DEF(name, type [, oplist_of_the_type]) */
#define M_RBTREE_DEF(name, ...)                                               \
  M_RBTREE_DEF_AS(name, M_F(name,_t), M_F(name,_it_t), __VA_ARGS__)


/* Define a Red/Black binary tree of a given type
   as the name name_t and the iterator it_t.
   USAGE: RBTREE_DEF_AS(name, name_t, it_t, type [, oplist_of_the_type]) */
#define M_RBTREE_DEF_AS(name, name_t, it_t, ...)                              \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_RBTR33_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                                 \
              ((name, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), name_t, M_F(name, _node_ct), it_t ), \
               (name, __VA_ARGS__,                                        name_t, M_F(name, _node_ct), it_t ))) \
  M_END_PROTECTED_CODE


/* Define the oplist of a rbtree of type.
   USAGE: RBTREE_OPLIST(name [, oplist_of_the_type]) */
#define M_RBTREE_OPLIST(...)                                                  \
  M_RBTR33_OPLIST_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                              \
                 ((__VA_ARGS__, M_BASIC_OPLIST),                              \
                  (__VA_ARGS__ )))


/*****************************************************************************/
/********************************** INTERNAL *********************************/
/*****************************************************************************/

/* Deferred evaluation for the oplist definition,
   so that all arguments are evaluated before further expansion */
#define M_RBTR33_OPLIST_P1(arg) M_RBTR33_OPLIST_P2 arg

/* Validation of the given oplist */
#define M_RBTR33_OPLIST_P2(name, oplist)                                      \
  M_IF_OPLIST(oplist)(M_RBTR33_OPLIST_P3, M_RBTR33_OPLIST_FAILURE)(name, oplist)

/* Prepare a clean compilation failure */
#define M_RBTR33_OPLIST_FAILURE(name, oplist)                                 \
  ((M_LIB_ERROR(ARGUMENT_OF_RBTREE_OPLIST_IS_NOT_AN_OPLIST, name, oplist)))

/* OPLIST definition of a rbtree
   NOTE: IT_REF is not exported so that the container appears as not modifiable
   by algorithm.*/
#ifndef M_USE_CONTEXT
#define M_RBTR33_OPLIST_P3(name, oplist)                                      \
  (INIT(M_F(name, _init)),                                                    \
   INIT_SET(M_F(name, _init_set)),                                            \
   INIT_WITH(API_1(M_INIT_EMPLACE_VAI)),                                      \
   SET(M_F(name, _set)),                                                      \
   CLEAR(M_F(name, _clear)),                                                  \
   INIT_MOVE(M_F(name, _init_move)),                                          \
   MOVE(M_F(name, _move)),                                                    \
   SWAP(M_F(name, _swap)),                                                    \
   NAME(name),                                                                \
   TYPE(M_F(name,_ct)), GENTYPE(struct M_F(name,_s)*),                        \
   SUBTYPE(M_F(name, _subtype_ct)),                                           \
   EMPTY_P(M_F(name,_empty_p)),                                               \
   GET_SIZE(M_F(name, _size)),                                                \
   IT_TYPE(M_F(name, _it_ct)),                                                \
   IT_FIRST(M_F(name,_it)),                                                   \
   IT_SET(M_F(name,_it_set)),                                                 \
   IT_LAST(M_F(name,_it_last)),                                               \
   IT_END(M_F(name,_it_end)),                                                 \
   IT_END_P(M_F(name,_end_p)),                                                \
   IT_LAST_P(M_F(name,_last_p)),                                              \
   IT_EQUAL_P(M_F(name,_it_equal_p)),                                         \
   IT_NEXT(M_F(name,_next)),                                                  \
   IT_PREVIOUS(M_F(name,_previous)),                                          \
   IT_CREF(M_F(name,_cref)),                                                  \
   IT_REMOVE(M_F(name,_remove)),                                              \
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
#define M_RBTR33_OPLIST_P3(name, oplist)                                      \
  (INIT(M_F(name, _init)),                                                    \
   INIT_SET(API_0P(M_F(name, _init_set))),                                    \
   INIT_WITH(API_1(M_INIT_EMPLACE_VAI)),                                      \
   SET(API_0P(M_F(name, _set))),                                              \
   CLEAR(API_0P(M_F(name, _clear))),                                          \
   INIT_MOVE(M_F(name, _init_move)),                                          \
   MOVE(API_0P(M_F(name, _move))),                                            \
   SWAP(M_F(name, _swap)),                                                    \
   NAME(name), TYPE(M_F(name,_ct)), GENTYPE(struct M_F(name,_s)*),            \
   SUBTYPE(M_F(name, _subtype_ct)),                                           \
   EMPTY_P(M_F(name,_empty_p)),                                               \
   GET_SIZE(M_F(name, _size)),                                                \
   IT_TYPE(M_F(name, _it_ct)),                                                \
   IT_FIRST(M_F(name,_it)),                                                   \
   IT_SET(M_F(name,_it_set)),                                                 \
   IT_LAST(M_F(name,_it_last)),                                               \
   IT_END(M_F(name,_it_end)),                                                 \
   IT_END_P(M_F(name,_end_p)),                                                \
   IT_LAST_P(M_F(name,_last_p)),                                              \
   IT_EQUAL_P(M_F(name,_it_equal_p)),                                         \
   IT_NEXT(M_F(name,_next)),                                                  \
   IT_PREVIOUS(M_F(name,_previous)),                                          \
   IT_CREF(M_F(name,_cref)),                                                  \
   IT_REMOVE(M_F(name,_remove)),                                              \
   RESET(API_0P(M_F(name,_reset))),                                           \
   PUSH(API_0P(M_F(name,_push))),                                             \
   GET_MIN(M_F(name,_min)),                                                   \
   GET_MAX(M_F(name,_max)),                                                   \
   M_IF_METHOD(GET_STR, oplist)(GET_STR(API_0P(M_F(name, _get_str))),),       \
   M_IF_METHOD(PARSE_STR, oplist)(PARSE_STR(API_0P(M_F(name, _parse_str))),), \
   M_IF_METHOD(OUT_STR, oplist)(OUT_STR(M_F(name, _out_str)),),               \
   M_IF_METHOD(IN_STR, oplist)(IN_STR(API_0P(M_F(name, _in_str))),),          \
   M_IF_METHOD(OUT_SERIAL, oplist)(OUT_SERIAL(API_0P(M_F(name, _out_serial))),), \
   M_IF_METHOD(IN_SERIAL, oplist)(IN_SERIAL(API_0P(M_F(name, _in_serial))),), \
   M_IF_METHOD(EQUAL, oplist)(EQUAL(M_F(name, _equal_p)),),                   \
   M_IF_METHOD(HASH, oplist)(HASH(M_F(name, _hash)),)                         \
   )
#endif

/********************************** INTERNAL *********************************/

/* Max depth of the binary tree
   It is at worst twice the depth of a perfectly even tree with maximum elements.
   The maximum number of elements is the max of size_t.
   A perfectly even tree is of depth log2(max(size_t))=CHAR_BIT*sizeof(size_t)
 */
#define M_RBTR33_MAX_STACK (2*CHAR_BIT*sizeof (size_t))

/* Encapsulation of the color of the nodes. */
#define M_RBTR33_SET_RED(x)   ((x)->color =  M_RBTR33_RED)
#define M_RBTR33_SET_BLACK(x) ((x)->color =  M_RBTR33_BLACK)
#define M_RBTR33_IS_RED(x)    ((x)->color == M_RBTR33_RED)
#define M_RBTR33_IS_BLACK(x)  ((x)->color == M_RBTR33_BLACK)
#define M_RBTR33_COPY_COLOR(x,y) ((x)->color = (y)->color)
#define M_RBTR33_GET_COLOR(x) (true ? (x)->color : (x)->color)
#define M_RBTR33_SET_COLOR(x, c) ((x)->color = (c))
#define M_RBTR33_GET_CHILD(x, n) ((x)->child[n])
#define M_RBTR33_SET_CHILD(x, n, y) ((x)->child[n] = (y))

// Color of a node of a Red/Black tree
typedef enum {
  M_RBTR33_BLACK = 0, M_RBTR33_RED
} m_rbtr33_color_e;

// General contact of a Read/Black tree
#define M_RBTR33_CONTRACT(tree) do {                                          \
    M_ASSERT ((tree) != NULL);                                                \
    M_ASSERT ((tree)->node == NULL || M_RBTR33_IS_BLACK((tree)->node));       \
    M_ASSERT ((tree)->size != 0 || (tree)->node == NULL);                     \
  } while (0)

// Contract of a node (doesn't check for equal depth in black)
#define M_RBTR33_CONTRACT_NODE(node) do {                                     \
    M_ASSERT((node) != NULL);                                                 \
    M_ASSERT(M_RBTR33_IS_BLACK(node) || M_RBTR33_IS_RED(node));               \
    M_ASSERT(M_RBTR33_IS_BLACK(node)                                          \
           || (((node)->child[0] == NULL || M_RBTR33_IS_BLACK(node->child[0])) \
               && ((node)->child[1] == NULL || M_RBTR33_IS_BLACK(node->child[1])))); \
  } while (0)


/* Deferred evaluation for the rbtree definition,
   so that all arguments are evaluated before further expansion */
#define M_RBTR33_DEF_P1(arg) M_ID( M_RBTR33_DEF_P2 arg )

/* Validate the oplist before going further */
#define M_RBTR33_DEF_P2(name, type, oplist, tree_t, node_t, it_t)             \
  M_IF_OPLIST(oplist)(M_RBTR33_DEF_P3, M_RBTR33_DEF_FAILURE)(name, type, oplist, tree_t, node_t, it_t)

/* Stop processing with a compilation failure */
#define M_RBTR33_DEF_FAILURE(name, type, oplist, tree_t, note_t, it_t)        \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST, "(RBTREE_DEF): the given argument is not a valid oplist: " #oplist)

/* Internal rbtree definition
   - name: prefix to be used
   - type: type of the elements of the rbtree
   - oplist: oplist of the type of the elements of the container
   - tree_t: alias for the type of the container
   - it_t: alias for the iterator of the container
   - node_t: alias for the node of an element of the container
 */
#define M_RBTR33_DEF_P3(name, type, oplist, tree_t, node_t, it_t)             \
  M_RBTR33_DEF_TYPE(name, type, oplist, tree_t, node_t, it_t)                 \
  M_CHECK_COMPATIBLE_OPLIST(name, 1, type, oplist)                            \
  M_RBTR33_DEF_CORE(name, type, oplist, tree_t, node_t, it_t)                 \
  M_RBTR33_DEF_IO(name, type, oplist, tree_t, node_t, it_t)                   \
  M_EMPLACE_QUEUE_DEF(name, tree_t, _emplace, oplist, M_EMPLACE_QUEUE_GENE)

/* Define the types associated to a R/B Tree */
#define M_RBTR33_DEF_TYPE(name, type, oplist, tree_t, node_t, it_t)           \
                                                                              \
  /* Node of Red/Black tree.                                                  \
     Each node has up to two child, a color (Red or black)                    \
     and the data stored in it */                                             \
  typedef struct M_F(name, _node_s) {                                         \
    struct M_F(name, _node_s) *child[2];                                      \
    type data;                                                                \
    m_rbtr33_color_e color;                                                   \
  } node_t;                                                                   \
                                                                              \
  /* Define the Red/Black tree */                                             \
  typedef struct M_F(name, _s) {                                              \
    size_t size;    /* Number of elements in the tree */                      \
    node_t *node;   /* Root node of the tree */                               \
  } tree_t[1];                                                                \
  typedef struct M_F(name, _s) *M_F(name, _ptr);                              \
  typedef const struct M_F(name, _s) *M_F(name, _srcptr);                     \
                                                                              \
  /* Iterator on a tree. The iterator stores the full path to the             \
  current node through all its parents and its depth */                       \
  typedef struct M_F(name, _it_s) {                                           \
    node_t *stack[M_RBTR33_MAX_STACK];                                        \
    int8_t  which[M_RBTR33_MAX_STACK];                                        \
    unsigned int cpt;                                                         \
  } it_t[1];                                                                  \
                                                                              \
  /* Definition of the alias used by the oplists */                           \
  typedef type   M_F(name, _subtype_ct);                                      \
  typedef tree_t M_F(name, _ct);                                              \
  typedef it_t   M_F(name, _it_ct);                                           \

/* Define the core functions */
#define M_RBTR33_DEF_CORE(name, type, oplist, tree_t, node_t, it_t)           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _init)(tree_t tree)                                               \
  {                                                                           \
    M_ASSERT (tree != NULL);                                                  \
    tree->size = 0;                                                           \
    tree->node = NULL;                                                        \
    M_RBTR33_CONTRACT(tree);                                                  \
  }                                                                           \
                                                                              \
  M_P(void, name, _reset, tree_t tree)                                        \
  {                                                                           \
    M_RBTR33_CONTRACT(tree);                                                  \
    node_t *stack[M_RBTR33_MAX_STACK];                                        \
    unsigned int cpt = 0;                                                     \
    /* If nothing (no node) nothing to clean: return */                       \
    if (tree->node == NULL) return;                                           \
    /* Parse all the tree */                                                  \
    stack[cpt++] = tree->node;                                                \
    while (cpt > 0) {                                                         \
      node_t *n = stack[cpt-1];                                               \
      /* Go down to the bottom left node that exists */                       \
      while (true) {                                                          \
        M_RBTR33_CONTRACT_NODE(n);                                            \
        /* If there is a left child, get it */                                \
        if (n->child[0] != NULL) {                                            \
          M_ASSERT (cpt < M_RBTR33_MAX_STACK);                                \
          stack[cpt++] = n->child[0];                                         \
          n = n->child[0];                                                    \
          stack[cpt-2]->child[0] = NULL;                                      \
        /* If there is a right child, get it */                               \
        } else if (n->child[1] != NULL) {                                     \
          M_ASSERT (cpt < M_RBTR33_MAX_STACK);                                \
          stack[cpt++] = n->child[1];                                         \
          n = n->child[1];                                                    \
          stack[cpt-2]->child[1] = NULL;                                      \
        /* No left nor right child, node can be deleted */                    \
        } else {                                                              \
          break;                                                              \
        }                                                                     \
      }                                                                       \
      M_ASSERT (n == stack[cpt - 1]);                                         \
      /* Clear the bottom left node */                                        \
      M_CALL_CLEAR(oplist, n->data);                                          \
      M_CALL_DEL(oplist, n);                                                  \
      M_ASSERT((stack[cpt-1] = NULL) == NULL);                                \
      /* Go up to the parent */                                               \
      cpt--;                                                                  \
    }                                                                         \
    /* Mark the root node as empty */                                         \
    tree->node = NULL;                                                        \
    tree->size = 0;                                                           \
   }                                                                          \
                                                                              \
  M_P(void, name, _clear, tree_t tree)                                        \
  {                                                                           \
    /* Nothing more than clean the tree as everything is cleared */           \
    M_F(name, _reset) M_R(tree);                                              \
  }                                                                           \
                                                                              \
  M_P(void, name, _push, tree_t tree, type const data)                        \
  {                                                                           \
    M_RBTR33_CONTRACT(tree);                                                  \
    node_t *tab[M_RBTR33_MAX_STACK];                                          \
    int8_t  which[M_RBTR33_MAX_STACK];                                        \
    unsigned int cpt = 0;                                                     \
    node_t *n = tree->node;                                                   \
    /* If there is no root node, create a new node */                         \
    if (n == NULL) {                                                          \
      n = M_CALL_NEW(oplist, node_t);                                         \
      if (M_UNLIKELY_NOMEM (n == NULL)) {                                     \
        M_MEMORY_FULL(node_t, 1);                                             \
      }                                                                       \
      /* Copy the data in the root node */                                    \
      M_CALL_INIT_SET(oplist, n->data, data);                                 \
      /* Mark the root node as black */                                       \
      n->child[0] = n->child[1] = NULL;                                       \
      M_RBTR33_SET_BLACK (n);                                                 \
      tree->node = n;                                                         \
      M_ASSERT(tree->size == 0);                                              \
      tree->size = 1;                                                         \
      M_RBTR33_CONTRACT(tree);                                                \
      return;                                                                 \
    }                                                                         \
    /* Search for insertion point in the tree */                              \
    tab[cpt] = n;                                                             \
    while (n != NULL) {                                                       \
      M_RBTR33_CONTRACT_NODE(n);                                              \
      int cmp = M_CALL_CMP(oplist, n->data, data);                            \
      if (cmp == 0) {                                                         \
        /* key found ==> stop analysis */                                     \
        break;                                                                \
      } else {                                                                \
        /* go left (if cmp > 0) or right (if cmp < 0) */                      \
        int s = (cmp < 0);                                                    \
        which[cpt++] = (int8_t) s;                                            \
        n = n->child[s];                                                      \
      }                                                                       \
      /* We cannot overflow the max depth of a tree */                        \
      M_ASSERT (cpt < M_RBTR33_MAX_STACK);                                    \
      tab[cpt] = n;                                                           \
    }                                                                         \
    /* If found, update the data (default is set) */                          \
    if (n != NULL) {                                                          \
      M_CALL_SET(oplist, n->data, data);                                      \
      M_RBTR33_CONTRACT (tree);                                               \
      return;                                                                 \
    }                                                                         \
    /* Create new node to store the data */                                   \
    n = M_CALL_NEW(oplist, node_t);                                           \
    if (M_UNLIKELY_NOMEM (n == NULL) ) {                                      \
      M_MEMORY_FULL (node_t, 1);                                              \
    }                                                                         \
    /* Copy the data and mark the node as red */                              \
    M_CALL_INIT_SET(oplist, n->data, data);                                   \
    n->child[0] = n->child[1] = NULL;                                         \
    M_RBTR33_SET_RED (n);                                                     \
    /* Add it in the iterator */                                              \
    M_ASSERT (tab[cpt] == NULL);                                              \
    tab[cpt] = n;                                                             \
    /* Add it in the tree */                                                  \
    tree->size ++;                                                            \
    M_ASSERT(tab[cpt-1]->child[0+which[cpt-1]] == NULL);                      \
    tab[cpt-1]->child[0+which[cpt-1]] = n;                                    \
    /* Fix the tree to still respect the red/back properties */               \
    while (cpt >= 2                                                           \
           && M_RBTR33_IS_RED(tab[cpt-1])                                     \
           && tab[cpt-2]->child[1-which[cpt-2]] != NULL                       \
           && M_RBTR33_IS_RED(tab[cpt-2]->child[1-which[cpt-2]])) {           \
      M_RBTR33_SET_BLACK(tab[cpt-1]);                                         \
      M_RBTR33_SET_BLACK(tab[cpt-2]->child[1-which[cpt-2]]);                  \
      M_RBTR33_SET_RED(tab[cpt-2]);                                           \
      cpt-=2;                                                                 \
    }                                                                         \
    /* root is always black */                                                \
    M_RBTR33_SET_BLACK(tab[0]);                                               \
    if (cpt <= 1 || M_RBTR33_IS_BLACK(tab[cpt-1])) {                          \
      M_RBTR33_CONTRACT (tree);                                               \
      return;                                                                 \
    }                                                                         \
    /* Read the grand-parent, the parent and the element */                   \
    node_t *pp = tab[cpt-2];                                                  \
    node_t *p  = tab[cpt-1];                                                  \
    node_t *x  = tab[cpt];                                                    \
    int i      = which[cpt-2];                                                \
    int j      = 1 - i;                                                       \
    M_ASSERT (i == 0 || i == 1);                                              \
    /* We need to do some rotations */                                        \
    if (i == which[cpt-1]) {                                                  \
      /* The child is the left child of its parent */                         \
      /* OR The child is the right child of its parent */                     \
      /* Right rotation: cpt is the new grand-parent.                         \
         x is its left child, the grand-parent is the right one */            \
      pp->child[i] = p->child[j];                                             \
      p->child[i] = x;                                                        \
      p->child[j] = pp;                                                       \
      M_RBTR33_SET_BLACK(p);                                                  \
      M_RBTR33_SET_RED(pp);                                                   \
    } else {                                                                  \
      M_ASSERT (j == which[cpt-1]);                                           \
      /* The child is the right child of its parent */                        \
      /* OR The child is the left child of its parent */                      \
      /* Left rotation */                                                     \
      pp->child[i] = x->child[j];                                             \
      p->child[j]  = x->child[i];                                             \
      x->child[i]  = p;                                                       \
      x->child[j]  = pp;                                                      \
      M_RBTR33_SET_BLACK(x);                                                  \
      M_RBTR33_SET_RED(p);                                                    \
      M_RBTR33_SET_RED(pp);                                                   \
      p = x;                                                                  \
    }                                                                         \
    /* Insert the new grand parent */                                         \
    if (cpt == 2) {                                                           \
      tree->node = p;                                                         \
    } else {                                                                  \
      M_ASSERT (cpt >= 3);                                                    \
      tab[cpt-3]->child[which[cpt-3]] = p;                                    \
    }                                                                         \
    /* Done */                                                                \
    M_RBTR33_CONTRACT (tree);                                                 \
  }                                                                           \
                                                                              \
  M_INLINE size_t                                                             \
  M_F(name, _size)(const tree_t tree)                                         \
  {                                                                           \
    M_RBTR33_CONTRACT (tree);                                                 \
    return tree->size;                                                        \
  }                                                                           \
                                                                              \
  /* Set the iterator to the first (child=0) or last (child=1) element */     \
  M_INLINE void                                                               \
  M_C3(m_rbtr33_,name,_it)(it_t it, const tree_t tree, int child)             \
  {                                                                           \
    M_RBTR33_CONTRACT (tree);                                                 \
    M_ASSERT (it != NULL);                                                    \
    M_ASSERT (child == 0 || child == 1);                                      \
    unsigned int cpt = 0;                                                     \
    if (tree->node != NULL) {                                                 \
      it->which[cpt] = (int8_t) child;                                        \
      node_t *n = it->stack[cpt++] = tree->node;                              \
      /* Go down the tree and fill in the iterator */                         \
      while (n->child[child] != NULL) {                                       \
        M_ASSERT (cpt < M_RBTR33_MAX_STACK);                                  \
        n = n->child[child];                                                  \
        it->which[cpt] = (int8_t) child;                                      \
        it->stack[cpt++] = n;                                                 \
      }                                                                       \
      M_ASSERT (n == it->stack[cpt - 1]);                                     \
    }                                                                         \
    it->cpt = cpt;                                                            \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it)(it_t it, const tree_t tree)                                  \
  {                                                                           \
    M_C3(m_rbtr33_,name,_it)(it, tree, 0);                                    \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_last)(it_t it, const tree_t tree)                             \
  {                                                                           \
    M_C3(m_rbtr33_,name,_it)(it, tree, 1);                                    \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_end)(it_t it, const tree_t tree)                              \
  {                                                                           \
    M_RBTR33_CONTRACT (tree);                                                 \
    M_ASSERT (it != NULL);                                                    \
    (void) tree; /* parameter not used */                                     \
    it->cpt = 0;                                                              \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_set)(it_t it, const it_t ref)                                 \
  {                                                                           \
    M_ASSERT (it != NULL && ref != NULL);                                     \
    *it = *ref;                                                               \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _end_p)(const it_t it)                                            \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    return it->cpt == 0;                                                      \
  }                                                                           \
                                                                              \
  /* Go to the next (child = 0)or previous element (child = 1) */             \
  M_INLINE void                                                               \
  M_C3(m_rbtr33_,name,_next)(it_t it, int child)                              \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    M_ASSERT (child == 0 || child == 1);                                      \
    if (it->cpt == 0) return;                                                 \
    unsigned int cpt = it->cpt - 1;                                           \
    node_t *n = it->stack[cpt];                                               \
    /* Get the other child */                                                 \
    const int right = 1 ^ child;                                              \
    if (n->child[right] != NULL) {                                            \
      /* Going right */                                                       \
      M_ASSERT (cpt < M_RBTR33_MAX_STACK);                                    \
      n = n->child[right];                                                    \
      it->which[cpt++] = (int8_t) right;                                      \
      it->stack[cpt] = n;                                                     \
      it->which[cpt++] = (int8_t) child;                                      \
      /* Going left */                                                        \
      while (n->child[child] != NULL) {                                       \
        M_ASSERT (cpt < M_RBTR33_MAX_STACK);                                  \
        n = n->child[child];                                                  \
        it->which[cpt] = (int8_t) child;                                      \
        it->stack[cpt++] = n;                                                 \
      }                                                                       \
      M_ASSERT (n == it->stack[cpt - 1]);                                     \
    } else {                                                                  \
      /* Going up */                                                          \
      while (cpt > 0 && it->which[cpt-1] == right) cpt--;                     \
    }                                                                         \
    it->cpt = cpt;                                                            \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _next)(it_t it)                                                   \
  {                                                                           \
    M_C3(m_rbtr33_,name,_next)(it, 0);                                        \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _previous)(it_t it)                                               \
  {                                                                           \
    M_C3(m_rbtr33_,name,_next)(it, 1);                                        \
  }                                                                           \
                                                                              \
  M_INLINE type *                                                             \
  M_F(name, _ref)(const it_t it)                                              \
  {                                                                           \
    M_ASSERT(it != NULL);                                                     \
    /* There shall be at least one element */                                 \
    M_ASSERT_INDEX(it->cpt-1, M_RBTR33_MAX_STACK);                            \
    /* NOTE: partially unsafe if the user modify the order of the el */       \
    return &(it->stack[it->cpt-1]->data);                                     \
  }                                                                           \
                                                                              \
  M_INLINE type const *                                                       \
  M_F(name, _cref)(const it_t it)                                             \
  {                                                                           \
    return M_CONST_CAST(type, M_F(name, _ref)(it));                           \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _it_equal_p)(const it_t it1, const it_t it2)                      \
  {                                                                           \
    M_ASSERT(it1 != NULL && it2 != NULL);                                     \
    /* There can be no element */                                             \
    M_ASSERT_INDEX(it1->cpt, M_RBTR33_MAX_STACK);                             \
    M_ASSERT_INDEX(it2->cpt, M_RBTR33_MAX_STACK);                             \
    return it1->cpt == it2->cpt                                               \
      && (it1->cpt == 0 || it1->stack[it1->cpt-1] == it2->stack[it2->cpt-1]); \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_from)(it_t it, const tree_t tree, type const data)            \
  {                                                                           \
    M_RBTR33_CONTRACT (tree);                                                 \
    M_ASSERT (it != NULL);                                                    \
    unsigned int cpt = 0;                                                     \
    int cmp = 1;                                                              \
    node_t *n = tree->node;                                                   \
    /* Find the lowest element greater or equal than data in the tree */      \
    while (n != NULL) {                                                       \
      M_ASSERT (cpt < M_RBTR33_MAX_STACK);                                    \
      it->which[cpt]   = 0;                                                   \
      it->stack[cpt++] = n;                                                   \
      cmp = M_CALL_CMP(oplist, n->data, data);                                \
      if (cmp == 0)                                                           \
        break;                                                                \
      int child = (cmp < 0);                                                  \
      it->which[cpt-1] = (int8_t) child;                                      \
      n = n->child[child];                                                    \
    }                                                                         \
    /* Save the iterator */                                                   \
    it->cpt = cpt;                                                            \
    /* The iterator found may be strictly lower than data.                    \
       In this case, go to the next element */                                \
    if (cmp < 0) {                                                            \
      M_F(name, _next)(it);                                                   \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _it_until_p)(it_t it, type const data)                            \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    if (M_UNLIKELY(it->cpt == 0)) return true;                                \
    M_ASSERT (it->cpt > 0 && it->cpt < M_RBTR33_MAX_STACK);                   \
    node_t *n = it->stack[it->cpt-1];                                         \
    int cmp = M_CALL_CMP(oplist, n->data, data);                              \
    return (cmp >= 0);                                                        \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _it_while_p)(it_t it, type const data)                            \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    if (M_UNLIKELY(it->cpt == 0)) return false;                               \
    M_ASSERT (it->cpt > 0 && it->cpt < M_RBTR33_MAX_STACK);                   \
    node_t *n = it->stack[it->cpt-1];                                         \
    int cmp = M_CALL_CMP(oplist, n->data, data);                              \
    return (cmp <= 0);                                                        \
  }                                                                           \
                                                                              \
  M_INLINE type *                                                             \
  M_F(name, _min)(const tree_t tree)                                          \
  {                                                                           \
    M_RBTR33_CONTRACT (tree);                                                 \
    node_t *n = tree->node;                                                   \
    if (M_UNLIKELY (n == NULL) ) return NULL;                                 \
    while (n->child[0] != NULL) {                                             \
      M_RBTR33_CONTRACT_NODE (n);                                             \
      n = n->child[0];                                                        \
    }                                                                         \
    return &n->data;                                                          \
  }                                                                           \
                                                                              \
  M_INLINE type *                                                             \
  M_F(name, _max)(const tree_t tree)                                          \
  {                                                                           \
    M_RBTR33_CONTRACT (tree);                                                 \
    node_t *n = tree->node;                                                   \
    if (M_UNLIKELY (n == NULL) ) return NULL;                                 \
    while (n->child[1] != NULL) {                                             \
      M_RBTR33_CONTRACT_NODE (n);                                             \
      n = n->child[1];                                                        \
    }                                                                         \
    return &n->data;                                                          \
  }                                                                           \
                                                                              \
  M_INLINE type const *                                                       \
  M_F(name, _cmin)(const tree_t tree)                                         \
  {                                                                           \
    return M_CONST_CAST(type, M_F(name, _min)(tree));                         \
  }                                                                           \
                                                                              \
  M_INLINE type const *                                                       \
  M_F(name, _cmax)(const tree_t tree)                                         \
  {                                                                           \
    return M_CONST_CAST(type, M_F(name, _max)(tree));                         \
  }                                                                           \
                                                                              \
  M_INLINE type *                                                             \
  M_F(name, _get)(const tree_t tree, type const data)                         \
  {                                                                           \
    M_RBTR33_CONTRACT (tree);                                                 \
    node_t *n = tree->node;                                                   \
    /* Go down the tree */                                                    \
    while (n != NULL) {                                                       \
      M_RBTR33_CONTRACT_NODE (n);                                             \
      int cmp = M_CALL_CMP(oplist, n->data, data);                            \
      if (cmp == 0) {                                                         \
        return &n->data;                                                      \
      } else {                                                                \
        /* Go left (if cmp > 0) or right (if cmp < 0) */                      \
        n = n->child[cmp < 0];                                                \
      }                                                                       \
    }                                                                         \
    return NULL;                                                              \
  }                                                                           \
                                                                              \
  M_INLINE type const *                                                       \
  M_F(name, _cget)(const tree_t tree, type const data)                        \
  {                                                                           \
    return M_CONST_CAST(type, M_F(name, _get)(tree, data));                   \
  }                                                                           \
                                                                              \
  /* Create a copy of the given node (recursively) */                         \
  M_P(node_t *, name, _i_copy_node, const node_t *o)                          \
  {                                                                           \
    if (M_UNLIKELY(o == NULL)) return NULL;                                   \
    node_t *n = M_CALL_NEW(oplist, node_t);                                   \
    if (M_UNLIKELY_NOMEM (n == NULL) ) {                                      \
      M_MEMORY_FULL (node_t, 1);                                              \
    }                                                                         \
    M_CALL_INIT_SET(oplist, n->data, o->data);                                \
    n->child[0] = M_F(name,_i_copy_node)M_R(o->child[0]);                     \
    n->child[1] = M_F(name,_i_copy_node)M_R(o->child[1]);                     \
    M_RBTR33_COPY_COLOR (n, o);                                               \
    return n;                                                                 \
  }                                                                           \
                                                                              \
  M_P(void, name, _init_set, tree_t tree, const tree_t ref)                   \
  {                                                                           \
    M_RBTR33_CONTRACT (ref);                                                  \
    M_ASSERT (tree != NULL && tree != ref);                                   \
    tree->size = ref->size;                                                   \
    /* Copy the root node recursively */                                      \
    tree->node = M_F(name,_i_copy_node)M_R(ref->node);                        \
    M_RBTR33_CONTRACT (tree);                                                 \
  }                                                                           \
                                                                              \
  M_P(void, name, _set, tree_t tree, const tree_t ref)                        \
  {                                                                           \
    M_RBTR33_CONTRACT (tree);                                                 \
    M_RBTR33_CONTRACT (ref);                                                  \
    if (tree == ref) return;                                                  \
    M_F(name,_clear)M_R(tree);                                                \
    M_F(name,_init_set)M_R(tree, ref);                                        \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _init_move)(tree_t tree, tree_t ref)                              \
  {                                                                           \
    M_RBTR33_CONTRACT (ref);                                                  \
    M_ASSERT (tree != NULL && tree != ref);                                   \
    tree->size = ref->size;                                                   \
    tree->node = ref->node;                                                   \
    /* Mark ref as an invalid representation */                               \
    ref->node = NULL;                                                         \
    ref->size = 1;                                                            \
    M_RBTR33_CONTRACT (tree);                                                 \
  }                                                                           \
                                                                              \
  M_P(void, name, _move, tree_t tree, tree_t ref)                             \
  {                                                                           \
    M_RBTR33_CONTRACT (tree);                                                 \
    M_RBTR33_CONTRACT (ref);                                                  \
    M_ASSERT (tree != ref);                                                   \
    M_F(name,_clear)M_R(tree);                                                \
    M_F(name,_init_move)(tree, ref);                                          \
    M_RBTR33_CONTRACT (tree);                                                 \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _swap)(tree_t tree1, tree_t tree2)                                \
  {                                                                           \
    M_RBTR33_CONTRACT (tree1);                                                \
    M_RBTR33_CONTRACT (tree2);                                                \
    M_SWAP(size_t, tree1->size, tree2->size);                                 \
    M_SWAP(node_t *, tree1->node, tree2->node);                               \
    M_RBTR33_CONTRACT (tree1);                                                \
    M_RBTR33_CONTRACT (tree2);                                                \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _empty_p)(const tree_t tree)                                      \
  {                                                                           \
    M_RBTR33_CONTRACT (tree);                                                 \
    return tree->size == 0;                                                   \
  }                                                                           \
                                                                              \
  /* Take care of the case n == NULL too */                                   \
  M_INLINE bool                                                               \
  M_C3(m_rbtr33_,name,_black_p)(const node_t *n)                              \
  {                                                                           \
    return (n == NULL) ? true : M_RBTR33_IS_BLACK(n);                         \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_C3(m_rbtr33_,name,_set_black)(node_t *n)                                  \
  {                                                                           \
    if (n != NULL) M_RBTR33_SET_BLACK(n);                                     \
  }                                                                           \
                                                                              \
  M_INLINE node_t *                                                           \
  M_C3(m_rbtr33_,name,_rotate)(node_t *pp, node_t *ppp, const bool right)     \
  {                                                                           \
    M_ASSERT (pp != NULL && ppp != NULL);                                     \
    bool left = !right;                                                       \
    node_t *p = pp->child[right];                                             \
    M_ASSERT (p != NULL);                                                     \
    pp->child[right] = p->child[left];                                        \
    p->child[left]  = pp;                                                     \
    /* Fix grandparent with new parent */                                     \
    M_ASSERT(ppp->child[0] == pp || ppp->child[1] == pp);                     \
    ppp->child[(ppp->child[0] != pp)] = p;                                    \
    return p;                                                                 \
  }                                                                           \
                                                                              \
  M_IF_DEBUG(                                                                 \
  /* Compute the depth of a node */                                           \
  M_INLINE size_t                                                             \
  M_C3(m_rbtr33_,name,_compute_depth)(const node_t *n)                        \
  {                                                                           \
    if (n == NULL) return 1;                                                  \
    return M_RBTR33_IS_BLACK (n)                                              \
      + M_C3(m_rbtr33_,name,_compute_depth)(n->child[0]);                     \
  }                                                                           \
  )                                                                           \
                                                                              \
  M_P(bool, name, _pop_at, type *data_ptr, tree_t tree, type const key)       \
  {                                                                           \
    M_RBTR33_CONTRACT (tree);                                                 \
    node_t *tab[M_RBTR33_MAX_STACK];                                          \
    int8_t  which[M_RBTR33_MAX_STACK];                                        \
    unsigned int cpt = 0;                                                     \
    node_t root_dummy;                                                        \
    node_t *n = tree->node;                                                   \
    which[0] = 0;                                                             \
    root_dummy.child[0] = n;                                                  \
    tab[cpt++] = &root_dummy;                                                 \
    /* Search for the deletion point */                                       \
    tab[cpt] = n;                                                             \
    while (n != NULL) {                                                       \
      M_RBTR33_CONTRACT_NODE (n);                                             \
      M_ASSERT(M_C3(m_rbtr33_,name,_compute_depth)(n->child[0])               \
             == M_C3(m_rbtr33_,name,_compute_depth)(n->child[1]));            \
      int cmp = M_CALL_CMP(oplist, n->data, key);                             \
      if (cmp == 0) {                                                         \
        break;                                                                \
      }                                                                       \
      int i = (cmp < 0);                                                      \
      which[cpt++] = (int8_t) i;                                              \
      n = n->child[i];                                                        \
      M_ASSERT (cpt < M_RBTR33_MAX_STACK);                                    \
      tab[cpt] = n;                                                           \
    }                                                                         \
    M_ASSERT (tab[cpt] == n);                                                 \
    /* If not found, fail */                                                  \
    if (n == NULL) {                                                          \
      return false;                                                           \
    }                                                                         \
    unsigned int cpt_n = cpt;                                                 \
    node_t *v = n;     /* the replacement node */                             \
    node_t *u;         /* the deleted node */                                 \
    m_rbtr33_color_e v_color = M_RBTR33_GET_COLOR(v);                         \
    /* Classical removal of a node from a binary tree */                      \
    if (v->child[0] != NULL && v->child[1] != NULL) {                         \
      /* node has 2 child. */                                                 \
      /* Get the element right next to the deleted one */                     \
      v = v->child[1];                                                        \
      which[cpt++] = 1;                                                       \
      tab[cpt] = v;                                                           \
      while (v != NULL) {                                                     \
        /* Always left node */                                                \
        M_RBTR33_CONTRACT_NODE (v);                                           \
        M_ASSERT(M_C3(m_rbtr33_,name,_compute_depth)(v->child[0])             \
               == M_C3(m_rbtr33_,name,_compute_depth)(v->child[1]));          \
        which[cpt++] = 0;                                                     \
        v = v->child[0];                                                      \
        M_ASSERT (cpt < M_RBTR33_MAX_STACK);                                  \
        tab[cpt] = v;                                                         \
      }                                                                       \
      /* Pop the last element to get the last non-null element */             \
      v = tab[--cpt];                                                         \
      M_ASSERT (v != NULL);                                                   \
      u = v->child[1];                                                        \
      /* Replace 'v' by 'u' in the tree */                                    \
      M_ASSERT(cpt >= 1 && tab[cpt-1] != NULL && tab[cpt-1]->child[which[cpt-1]] == v); \
      tab[cpt-1]->child[which[cpt-1]] = u;                                    \
      /* Replace 'n' by 'v' in the tree */                                    \
      M_ASSERT(cpt_n >= 1 && tab[cpt_n-1] != NULL);                           \
      M_ASSERT(tab[cpt_n-1]->child[which[cpt_n-1]] == n);                     \
      tab[cpt_n-1]->child[which[cpt_n-1]] = v;                                \
      v->child[0] = n->child[0];                                              \
      v->child[1] = n->child[1];                                              \
      v_color = M_RBTR33_GET_COLOR(v);                                        \
      M_RBTR33_COPY_COLOR(v, n);                                              \
      tab[cpt_n] = v;                                                         \
      /* For the algorithm, 'u' is now the deleted node */                    \
    } else {                                                                  \
      /* 1 or no child to the node. Replace the element */                    \
      v = n;                                                                  \
      u = v->child[(n->child[0] == NULL)];                                    \
      M_ASSERT (cpt_n >= 1 &&tab[cpt_n-1] != NULL && tab[cpt_n-1]->child[which[cpt_n-1]] == n); \
      M_ASSERT (n->child[(n->child[0] != NULL)] == NULL);                     \
      tab[cpt_n-1]->child[which[cpt_n-1]] = u;                                \
      /* in all cases, this node shall be set to black */                     \
    }                                                                         \
                                                                              \
    /* Rebalance from child to root */                                        \
    if (v_color == M_RBTR33_BLACK                                             \
        && M_C3(m_rbtr33_,name,_black_p)(u)) {                                \
      /* tab[0] is NULL, tab[1] is root, u is double black */                 \
      node_t *p = u, *s;                                                      \
      while (cpt >= 2) {                                                      \
        p  = tab[--cpt];                                                      \
        bool nbChild = which[cpt];                                            \
        M_ASSERT (p != NULL && u == p->child[nbChild]);                       \
        s = p->child[!nbChild];                                               \
        /* if sibling is red, perform a rotation to move sibling up */        \
        if (!M_C3(m_rbtr33_,name,_black_p)(s)) {                              \
          p = M_C3(m_rbtr33_,name,_rotate) (p, tab[cpt-1], !nbChild);         \
          M_RBTR33_SET_BLACK(p); /* was sibling */                            \
          tab[cpt] = p;                                                       \
          which[cpt++] = nbChild;                                             \
          p = p->child[nbChild];  /* was parent */                            \
          M_ASSERT (p != NULL);                                               \
          M_RBTR33_SET_RED(p);                                                \
          s = p->child[!nbChild];                                             \
          M_ASSERT (M_C3(m_rbtr33_,name,_black_p)(s));                        \
        }                                                                     \
        M_ASSERT (p != NULL && u == p->child[nbChild]);                       \
        /* if both children of s are black */                                 \
        /* perform recoloring and recur on parent if black */                 \
        if (s != NULL                                                         \
            && M_C3(m_rbtr33_,name,_black_p)(s->child[0])                     \
            && M_C3(m_rbtr33_,name,_black_p)(s->child[1])) {                  \
          M_ASSERT(M_C3(m_rbtr33_,name,_compute_depth)(s->child[0]) == M_C3(m_rbtr33_,name,_compute_depth)(s->child[1])); \
          M_RBTR33_SET_RED(s);                                                \
          if (M_RBTR33_IS_RED(p)) {                                           \
            M_RBTR33_SET_BLACK(p);                                            \
            M_RBTR33_CONTRACT_NODE(p);                                        \
            M_ASSERT(M_C3(m_rbtr33_,name,_compute_depth)(p->child[0]) == M_C3(m_rbtr33_,name,_compute_depth)(p->child[1])); \
            break;                                                            \
          }                                                                   \
          u = p;                                                              \
        } else  {                                                             \
          M_ASSERT (s != NULL);                                               \
          /* at least one child of 's' is red */                              \
          /* perform rotation(s) */                                           \
          bool childIsRight =  !M_C3(m_rbtr33_,name,_black_p)(s->child[1]);   \
          m_rbtr33_color_e p_color = M_RBTR33_GET_COLOR (p);                  \
          if (childIsRight != nbChild) {                                      \
            /* left-left or right-right case */                               \
            p = M_C3(m_rbtr33_,name,_rotate) (p, tab[cpt-1], childIsRight);   \
          } else {                                                            \
            s = M_C3(m_rbtr33_,name,_rotate) (s, p, childIsRight);            \
            p = M_C3(m_rbtr33_,name,_rotate) (p, tab[cpt-1], !nbChild);       \
          }                                                                   \
          M_RBTR33_SET_COLOR(p, p_color);                                     \
          M_ASSERT(p->child[0] != NULL && p->child[1] != NULL);               \
          M_RBTR33_SET_BLACK(p->child[0]);                                    \
          M_RBTR33_SET_BLACK(p->child[1]);                                    \
          M_RBTR33_CONTRACT_NODE(p);                                          \
          M_ASSERT(M_C3(m_rbtr33_,name,_compute_depth)(p->child[0]) == M_C3(m_rbtr33_,name,_compute_depth)(p->child[1])); \
          break;                                                              \
        }                                                                     \
      } /* while */                                                           \
      if (cpt == 1 /* root has been reached? */ ) {                           \
        M_C3(m_rbtr33_,name,_set_black)(p);                                   \
        M_ASSERT (root_dummy.child[0] == p);                                  \
      }                                                                       \
    } else {                                                                  \
      M_C3(m_rbtr33_,name,_set_black)(u);                                     \
    }                                                                         \
    tree->node = root_dummy.child[0];                                         \
    M_ASSERT (tree->node == NULL || M_RBTR33_IS_BLACK(tree->node));           \
    /* delete it */                                                           \
    if (data_ptr != NULL)                                                     \
      M_DO_MOVE(oplist, *data_ptr, n->data);                                  \
    else                                                                      \
      M_CALL_CLEAR(oplist, n->data);                                          \
    M_CALL_DEL(oplist, n);                                                    \
    tree->size --;                                                            \
    M_RBTR33_CONTRACT (tree);                                                 \
    return true;                                                              \
  }                                                                           \
                                                                              \
  M_P(void, name,_remove, tree_t t, it_t it)                                  \
  {                                                                           \
    /* Not optimum: another search in the tree is performed */                \
    type const *data = M_F(name,_cref)(it);                                   \
    M_F(name,_next)(it);                                                      \
    M_F(name, _pop_at)M_R(NULL, t, *data);                                    \
    /* We have changed the tree: the iterator is partially invalid */         \
    if (!M_F(name, _end_p)(it)) {                                             \
      M_F(name, _it_from)(it, t, *M_F(name,_cref)(it));                       \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_IF_METHOD(EQUAL, oplist)(                                                 \
  M_INLINE bool M_F(name,_equal_p)(const tree_t t1, const tree_t t2) {        \
    M_RBTR33_CONTRACT(t1);                                                    \
    M_RBTR33_CONTRACT(t2);                                                    \
    if (t1->size != t2->size) return false;                                   \
    it_t it1;                                                                 \
    it_t it2;                                                                 \
    /* NOTE: We can't compare two tree directly as they can be                \
       structurally different but functionally equal (you get this by         \
       constructing the tree in a different way). We have to                  \
       compare the ordered value within the tree. */                          \
    M_F(name, _it)(it1, t1);                                                  \
    M_F(name, _it)(it2, t2);                                                  \
    while (!M_F(name, _end_p)(it1)                                            \
           && !M_F(name, _end_p)(it2)) {                                      \
      type const *ref1 = M_F(name, _cref)(it1);                               \
      type const *ref2 = M_F(name, _cref)(it2);                               \
      if (M_CALL_EQUAL(oplist, *ref1, *ref2) == false)                        \
        return false;                                                         \
      M_F(name, _next)(it1);                                                  \
      M_F(name, _next)(it2);                                                  \
    }                                                                         \
    return M_F(name, _end_p)(it1)                                             \
      && M_F(name, _end_p)(it2);                                              \
  }                                                                           \
  , /* NO EQUAL METHOD */ )                                                   \
                                                                              \
  M_IF_METHOD(HASH, oplist)(                                                  \
  M_INLINE size_t M_F(name,_hash)(const tree_t t1) {                          \
    M_RBTR33_CONTRACT(t1);                                                    \
    M_HASH_DECL(hash);                                                        \
    /* NOTE: We can't compute the hash directly for the same reason           \
       than for EQUAL operator. */                                            \
    it_t it1;                                                                 \
    M_F(name, _it)(it1, t1);                                                  \
    while (!M_F(name, _end_p)(it1)) {                                         \
      type const *ref1 = M_F(name, _cref)(it1);                               \
      M_HASH_UP(hash, M_CALL_HASH(oplist, *ref1));                            \
      M_F(name, _next)(it1);                                                  \
    }                                                                         \
    return M_HASH_FINAL (hash);                                               \
  }                                                                           \
  , /* NO HASH METHOD */ )                                                    \

/* Define the I/O functions */
#define M_RBTR33_DEF_IO(name, type, oplist, tree_t, node_t, it_t)             \
  M_IF_METHOD(GET_STR, oplist)(                                               \
  M_P(void, name, _get_str, m_string_t str, tree_t const t1, bool append)     \
  {                                                                           \
    M_RBTR33_CONTRACT(t1);                                                    \
    M_ASSERT(str != NULL);                                                    \
    (append ? m_string_cat_cstr : m_string_set_cstr) M_R(str, "[");           \
    /* NOTE: The print is really naive, and not really efficient */           \
    bool commaToPrint = false;                                                \
    it_t it1;                                                                 \
    M_F(name, _it)(it1, t1);                                                  \
    while (!M_F(name, _end_p)(it1)) {                                         \
      if (commaToPrint)                                                       \
        m_string_push_back M_R(str, M_GET_SEPARATOR oplist);                  \
      commaToPrint = true;                                                    \
      type const *ref1 = M_F(name, _cref)(it1);                               \
      M_CALL_GET_STR(oplist, str, *ref1, true);                               \
      M_F(name, _next)(it1);                                                  \
    }                                                                         \
    m_string_push_back M_R(str, ']');                                         \
  }                                                                           \
  , /* NO GET_STR */ )                                                        \
                                                                              \
  M_IF_METHOD(OUT_STR, oplist)(                                               \
  M_INLINE void                                                               \
  M_F(name, _out_str)(FILE *file, tree_t const rbtree)                        \
  {                                                                           \
    M_RBTR33_CONTRACT(rbtree);                                                \
    M_ASSERT (file != NULL);                                                  \
    fputc ('[', file);                                                        \
    it_t it;                                                                  \
    bool commaToPrint = false;                                                \
    for (M_F(name, _it)(it, rbtree) ;                                         \
         !M_F(name, _end_p)(it);                                              \
         M_F(name, _next)(it)){                                               \
      if (commaToPrint)                                                       \
        fputc (M_GET_SEPARATOR oplist, file);                                 \
      commaToPrint = true;                                                    \
      type const *item = M_F(name, _cref)(it);                                \
      M_CALL_OUT_STR(oplist, file, *item);                                    \
    }                                                                         \
    fputc (']', file);                                                        \
  }                                                                           \
  , /* no out_str */ )                                                        \
                                                                              \
  M_IF_METHOD(PARSE_STR, oplist)(                                             \
  M_P(bool, name, _parse_str, tree_t rbtree, const char str[], const char **endp) \
  {                                                                           \
    M_RBTR33_CONTRACT(rbtree);                                                \
    M_ASSERT (str != NULL);                                                   \
    M_F(name,_reset)M_R(rbtree);                                              \
    int c = m_core_str_nospace(&str);                                         \
    if (M_UNLIKELY (c != '[')) { c = 0; goto exit; }                          \
    c = m_core_str_nospace(&str);                                             \
    if (M_UNLIKELY (c == ']' || c == 0)) goto exit;                           \
    str--;                                                                    \
    M_QLET(1, item, type, oplist) {                                           \
      do {                                                                    \
        bool b = M_CALL_PARSE_STR(oplist, item, str, &str);                   \
        c = m_core_str_nospace(&str);                                         \
        if (b == false || c == 0) { c= 0 ; break; };                          \
        M_F(name, _push)M_R(rbtree, item);                                    \
      } while (c == M_GET_SEPARATOR oplist);                                  \
    }                                                                         \
  exit:                                                                       \
    if (endp) *endp = str;                                                    \
    return c == ']';                                                          \
  }                                                                           \
  , /* no parse_str */ )                                                      \
                                                                              \
  M_IF_METHOD(IN_STR, oplist)(                                                \
  M_P(bool, name, _in_str, tree_t rbtree, FILE *file)                         \
  {                                                                           \
    M_RBTR33_CONTRACT(rbtree);                                                \
    M_ASSERT (file != NULL);                                                  \
    M_F(name,_reset)M_R(rbtree);                                              \
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
        M_F(name, _push)M_R(rbtree, item);                                    \
      } while (c == M_GET_SEPARATOR oplist);                                  \
    }                                                                         \
    return c == ']';                                                          \
  }                                                                           \
  , /* no in_str */ )                                                         \
                                                                              \
  M_IF_METHOD(OUT_SERIAL, oplist)(                                            \
  M_P(m_serial_return_code_t, name, _out_serial, m_serial_write_t f, tree_t const t1) \
  {                                                                           \
    M_RBTR33_CONTRACT(t1);                                                    \
    M_ASSERT (f != NULL && f->m_interface != NULL);                           \
    m_serial_local_t local;                                                   \
    m_serial_return_code_t ret;                                               \
    M_F(name, _subtype_ct) const *item;                                       \
    bool first_done = false;                                                  \
    it_t it;                                                                  \
    ret = f->m_interface->write_array_start M_R(local, f, t1->size);          \
    for (M_F(name, _it)(it, t1) ;                                             \
         !M_F(name, _end_p)(it);                                              \
         M_F(name, _next)(it)){                                               \
      item = M_F(name, _cref)(it);                                            \
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
  M_IF_METHOD(IN_SERIAL, oplist)(                                             \
  M_P(m_serial_return_code_t, name, _in_serial, tree_t t1, m_serial_read_t f) \
  {                                                                           \
    M_RBTR33_CONTRACT(t1);                                                    \
    M_ASSERT (f != NULL && f->m_interface != NULL);                           \
    m_serial_local_t local;                                                   \
    m_serial_return_code_t ret;                                               \
    size_t estimated_size = 0;                                                \
    M_F(name,_reset)M_R(t1);                                                  \
    ret = f->m_interface->read_array_start(local, f, &estimated_size);        \
    if (M_UNLIKELY (ret != M_SERIAL_OK_CONTINUE)) return ret;                 \
    M_QLET(1, key, type, oplist) {                                            \
      do {                                                                    \
        ret = M_CALL_IN_SERIAL(oplist, key, f);                               \
        if (ret != M_SERIAL_OK_DONE) { break; }                               \
        M_F(name, _push)M_R(t1, key);                                         \
        ret = f->m_interface->read_array_next(local, f);                      \
      } while (ret == M_SERIAL_OK_CONTINUE);                                  \
    }                                                                         \
    return ret;                                                               \
  }                                                                           \
  , /* no in_serial */ )                                                      \
                                                                              \


/********************************** INTERNAL *********************************/

// TODO: specialized _sort shall do nothing, but shall check the requested order. How ?
#if M_USE_SMALL_NAME
#define RBTREE_DEF M_RBTREE_DEF
#define RBTREE_DEF_AS M_RBTREE_DEF_AS
#define RBTREE_OPLIST M_RBTREE_OPLIST
#endif

#endif
