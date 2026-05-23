/*
 * M*LIB - TREE module
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
#ifndef MSTARLIB_TREE_H
#define MSTARLIB_TREE_H

#include "m-core.h"

/* Define a complete tree of 'type'
    https://en.wikipedia.org/wiki/Tree_(data_structure)
   USAGE: 
   TREE_DEF(name, type_t[, type_oplist])
*/
#define M_TREE_DEF(name, ...)                                                 \
    M_TREE_DEF_AS(name, M_F(name, _t), M_F(name, _it_t), __VA_ARGS__)

/* Define a complete tree of 'type'
   as the given name name_t with its associated functions.
   USAGE: 
   TREE_DEF_AS(name, name_t, it_t, type_t[, type_oplist])
*/
#define M_TREE_DEF_AS(name, name_t, it_t, ...)                                \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_TR33_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                                   \
             ((name, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), name_t, it_t ), \
              (name, __VA_ARGS__,                                        name_t, it_t ))) \
  M_END_PROTECTED_CODE

/* Maximum number of child per node.
   Used for assertion. */
#ifndef M_USE_TREE_MAX_CHILD_PER_PARENT
#define M_USE_TREE_MAX_CHILD_PER_PARENT 10000000
#endif


/*****************************************************************************/
/********************************** INTERNAL *********************************/
/*****************************************************************************/

# define M_TR33_NODE_CONTRACT(node, tree) do {                                \
    M_ASSERT( (tree)->size > 0);                                              \
    M_ASSERT( (node)->parent != M_TR33_NO_NODE);                              \
    M_ASSERT( (node)->parent == M_TR33_ROOT_NODE || (node)->parent >= 0);     \
    M_ASSERT( (node)->parent != M_TR33_ROOT_NODE || (node) == &(tree)->tab[(tree)->root_index] ); \
} while (0)

# define M_TR33_CONTRACT(tree) do {                                           \
    M_ASSERT( (tree)->size >= 0 && (tree)->size <= (tree)->capacity);         \
    M_ASSERT( (tree)->capacity >= 0 );                                        \
    M_ASSERT( (tree)->capacity == 0 || (tree)->tab != NULL);                  \
    M_ASSERT( (tree)->allow_realloc == 0 || (tree)->allow_realloc == INT32_MAX ); \
    M_ASSERT( (tree)->free_index >= M_TR33_NO_NODE && (tree)->free_index < (tree)->capacity); \
    M_ASSERT( (tree)->root_index >= M_TR33_NO_NODE && (tree)->root_index < (tree)->capacity); \
    M_ASSERT( (tree)->allow_realloc != 0 || (tree)->free_index < 0 || (tree)->tab[(tree)->free_index].parent == M_TR33_NO_NODE); \
    M_ASSERT( (tree)->allow_realloc != 0 || (tree)->root_index < 0 || (tree)->tab[(tree)->root_index].parent == M_TR33_ROOT_NODE); \
    M_ASSERT( (tree)->root_index != M_TR33_NO_NODE || (tree)->size == 0);     \
} while (0)

# define M_TR33_IT_CONTRACT(it, valid) do {                                   \
    M_TR33_CONTRACT( (it).tree);                                              \
    M_ASSERT(valid == false || (it).index >= 0);                              \
    if ((it).index >= 0) {                                                    \
        M_ASSERT( (it).index < (it).tree->capacity);                          \
        M_TR33_NODE_CONTRACT(&(it).tree->tab[(it).index], (it).tree);         \
        /* Don't deref table if realloc is disabled as the tree can be        \
           accessed concurrently (for _ref methods) */                        \
        if ( (it).tree->allow_realloc == 0) {                                 \
        /* All children of this node have the parent field correctly set */   \
        m_tr33_index_t itj = (it).tree->tab[(it).index].child;                \
        /* They all have their siblings as the left node */                   \
        m_tr33_index_t lftj = M_TR33_NO_NODE;                                 \
        /* We don't have any infinite loop */                                 \
        unsigned cpt = 0;                                                     \
        while (itj >= 0) {                                                    \
            M_ASSERT( (it).tree->tab[itj].parent == (it).index );             \
            M_ASSERT( (it).tree->tab[itj].left == lftj );                     \
            lftj = itj;                                                       \
            itj = (it).tree->tab[itj].right;                                  \
            M_ASSERT( ++cpt < M_USE_TREE_MAX_CHILD_PER_PARENT);               \
        }                                                                     \
        (void) cpt; /* may be unused in release mode */                       \
        (void) lftj; /* may be unused in release mode */                      \
        }                                                                     \
    }                                                                         \
} while (0)

/* Deferred evaluation */
#define M_TR33_DEF_P1(arg) M_ID( M_TR33_DEF_P2 arg )

/* Validate the oplist before going further */
#define M_TR33_DEF_P2(name, type, oplist, tree_t, it_t)                       \
  M_IF_OPLIST(oplist)(M_TR33_DEF_P3, M_TR33_DEF_FAILURE)(name, type, oplist, tree_t, it_t)

/* Stop processing with a compilation failure */
#define M_TR33_DEF_FAILURE(name, type, oplist, tree_t, it_t)                  \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST, "(TREE_DEF): the given argument is not a valid oplist: " #oplist)

/* type of an index in the tree. Up to 2^31-1 nodes can be created.*/
typedef int32_t m_tr33_index_t; 

/* Special value for the parent field of a node:
    * M_TR33_ROOT_NODE is only for the root node.
    * M_TR33_NO_NODE is for nodes without parent, i.e. the pool of free nodes.
 */
#define M_TR33_ROOT_NODE (-2)
#define M_TR33_NO_NODE (-1)

/* Internal definition:
   - name: prefix to be used
   - type: type of the elements of the tree
   - oplist: oplist of the type of the elements of the tree
   - tree_t: alias for the type of the tree
   - it_t: alias for the iterator of the tree

   A free node is identified as parent as M_TR33_NO_NODE, in which case child is the next item in the list of free node.
   A root node is identified as parent as M_TR33_ROOT_NODE
*/
#define M_TR33_DEF_P3(name, type, oplist, tree_t, it_t)                       \
  M_TR33_DEF_TYPE(name, type, oplist, tree_t, it_t)                           \
  M_TR33_DEF_P4_CORE(name, type, oplist, tree_t, it_t)                        \
  M_TR33_DEF_P4_EXT(name, type, oplist, tree_t, it_t)                         \
  M_TR33_DEF_P4_IO(name, type, oplist, tree_t, it_t)                          \
  M_TR33_DEF_P4_EMPLACE(name, type, oplist, tree_t, it_t)

#define M_TR33_DEF_TYPE(name, type, oplist, tree_t, it_t)                     \
                                                                              \
    /* Define a node of the tree.                                             \
       Each node of a tree is present in the array of the tree and as such    \
       we don't store the reference of other nodes using pointers but using   \
       integers. It is shorter and avoids allocating too much data.           \
       As such, a node may move on inserting another one, and                 \
       an iterator is not a pointer on the node but an index in this array.   \
       + 'parent' is the parent node of the current node,                     \
          or M_TR33_ROOT_NODE if it is a root.                                \
       + 'child' is the first child node of the current node (the left one)   \
          or M_TR33_NO_NODE if there is none.                                 \
       + 'left' is the left sibling or M_TR33_NO_NODE if there is none        \
       + 'right' is the right sibling or M_TR33_NO_NODE if there is none      \
       + 'data' if the data field of the node containing the given type       \
    */                                                                        \
    typedef struct M_F(name, _node_s) {                                       \
        m_tr33_index_t       parent;                                          \
        m_tr33_index_t       child;                                           \
        m_tr33_index_t       left;                                            \
        m_tr33_index_t       right;                                           \
        type                 data;                                            \
    } M_F(name, _node_ct);                                                    \
                                                                              \
    /* Define a tree:                                                         \
       + size is the number of nodes in the tree,                             \
       + capacity is the allocated size of the array 'tab'                    \
       + root_index is the index of the "first" root in the tree.             \
       + free_index is the list of free nodes in the array 'tab'.             \
       + allow_realloc is a bool encoded as true=0 and false=INT32_MAX        \
       + tab is a pointer to the allocated nodes.                             \
    */                                                                        \
    typedef struct M_F(name, _s) {                                            \
        m_tr33_index_t       size;                                            \
        m_tr33_index_t       capacity;                                        \
        m_tr33_index_t       root_index;                                      \
        m_tr33_index_t       free_index;                                      \
        uint32_t             allow_realloc;                                   \
        M_F(name, _node_ct) *tab;                                             \
    } tree_t[1];                                                              \
                                                                              \
    /* Define an iterator that references a node.                             \
    A node is an internal type, whereas the iterator is not.                  \
    A node can be moved, whereas the iterator will always remain valid        \
    until the node is destroyed.                                              \
    It is composed of:                                                        \
      + tree is a pointer to the tree structure                               \
      + index is an index in the array tab, identifying the node.             \
    NOTE: we don't define then using [1] as we want to pass then by value     \
    so that we can return iterator on the inserted elements.                  \
    */                                                                        \
    typedef struct M_F(name, _it_s) {                                         \
        struct M_F(name, _s) *tree;                                           \
        m_tr33_index_t        index;                                          \
    } it_t;                                                                   \
                                                                              \
    typedef it_t M_F(name, _it_ct);

/* Define the core & unique methods of a tree */
#define M_TR33_DEF_P4_CORE(name, type, oplist, tree_t, it_t)                  \
    /* Initialize a generic tree (empty) */                                   \
    M_INLINE void                                                             \
    M_F(name, _init)(tree_t tree) {                                           \
        tree->size = 0;                                                       \
        tree->capacity = 0;                                                   \
        tree->root_index = M_TR33_NO_NODE;                                    \
        tree->free_index = M_TR33_NO_NODE;                                    \
        tree->allow_realloc = 0;                                              \
        tree->tab = NULL;                                                     \
        M_TR33_CONTRACT(tree);                                                \
    }                                                                         \
                                                                              \
    M_P(void, name, _reset, tree_t tree) {                                    \
        M_TR33_CONTRACT(tree);                                                \
        M_UNUSED_CONTEXT();                                                   \
        if (tree->size > 0) {                                                 \
            /* We don't scan recursively the node tree, but sequentially */   \
            m_tr33_index_t free_index = tree->free_index;                     \
            for(m_tr33_index_t i = 0 ; i < tree->capacity ; i ++) {           \
                /* If the node is not a free node */                          \
                if (tree->tab[i].parent != M_TR33_NO_NODE) {                  \
                    /* free it */                                             \
                    M_CALL_CLEAR(oplist, tree->tab[i].data);                  \
                    tree->tab[i].parent = M_TR33_NO_NODE;                     \
                    tree->tab[i].child  = free_index;                         \
                    tree->tab[i].left   = M_TR33_NO_NODE;                     \
                    tree->tab[i].right  = M_TR33_NO_NODE;                     \
                    free_index = i;                                           \
                }                                                             \
            }                                                                 \
            /* Update the free index */                                       \
            tree->free_index = free_index;                                    \
            tree->size = 0;                                                   \
            tree->root_index = M_TR33_NO_NODE;                                \
        }                                                                     \
        M_TR33_CONTRACT(tree);                                                \
    }                                                                         \
                                                                              \
    M_P(void, name, _clear, tree_t tree) {                                    \
        M_F(name, _reset)M_R(tree);                                           \
        struct M_F(name,_node_s)*ptr = tree->tab == NULL ? NULL : tree->tab-1;\
        M_CALL_FREE(oplist, struct M_F(name, _node_s), ptr, (size_t)(tree->capacity+1)); \
        /* This is so reusing the object implies an assertion failure */      \
        tree->size = 1;                                                       \
        tree->tab = NULL;                                                     \
    }                                                                         \
                                                                              \
    M_P(void, name, _reserve, tree_t tree, size_t alloc) {                    \
        M_TR33_CONTRACT(tree);                                                \
        /* Nothing to do if the request is lower than the current capacity. */ \
        if (alloc <= (size_t) tree->capacity) {                               \
            return;                                                           \
        }                                                                     \
        /* Realloc the array */                                               \
        if (M_UNLIKELY_NOMEM (alloc >= INT32_MAX)) {                          \
            M_MEMORY_FULL(struct M_F(name, _node_s), alloc);                  \
        }                                                                     \
        /* Allocate one more term in the array so that tab[-1] exists.        \
           This enables performing latter tab[M_TR33_NO_NODE].x = something;  \
           as M_TR33_NO_NODE is -1. This enables avoiding testing for         \
           M_TR33_NO_NODE in some cases, performing branchless code. */       \
        struct M_F(name,_node_s)*ptr = tree->tab == NULL ? NULL : tree->tab-1;\
        ptr = M_CALL_REALLOC(oplist, struct M_F(name, _node_s), ptr, (size_t)(tree->capacity+1), alloc+1); \
        if (M_UNLIKELY_NOMEM (ptr == NULL) ) {                                \
            M_MEMORY_FULL(struct M_F(name, _node_s), alloc);                  \
        }                                                                     \
        /* Skip the first term to keep it as empty & unused */                \
        ptr++;                                                                \
        /* Free the list */                                                   \
        m_tr33_index_t *free_index = &tree->free_index;                       \
        if (*free_index != M_TR33_NO_NODE) {                                  \
            while (ptr[*free_index].child != M_TR33_NO_NODE) {                \
                free_index = &ptr[*free_index].child;                         \
            }                                                                 \
        }                                                                     \
        *free_index = tree->capacity;                                         \
        /* Construct the list of free node in the extra allocated pool */     \
        for(size_t i = (size_t)tree->capacity; i < alloc; i++) {              \
            ptr[i].parent = M_TR33_NO_NODE;                                   \
            ptr[i].left   = M_TR33_NO_NODE;                                   \
            ptr[i].right  = M_TR33_NO_NODE;                                   \
            ptr[i].child  = (m_tr33_index_t)(i+1);                            \
        }                                                                     \
        /* The last node has no child in the free node list */                \
        ptr[alloc-1].child = M_TR33_NO_NODE;                                  \
        /* Save the free list state in the tree */                            \
        tree->tab = ptr;                                                      \
        tree->capacity = (m_tr33_index_t) alloc;                              \
        M_TR33_CONTRACT(tree);                                                \
    }                                                                         \
                                                                              \
    M_INLINE void                                                             \
    M_F(name, _lock)(tree_t tree, bool lock) {                                \
        M_TR33_CONTRACT(tree);                                                \
        tree->allow_realloc = lock ? INT32_MAX : 0;                           \
        M_TR33_CONTRACT(tree);                                                \
    }                                                                         \
                                                                              \
    M_P(m_tr33_index_t, name, _i_alloc_node, tree_t tree) {                   \
        m_tr33_index_t ret = tree->free_index;                                \
        if (M_UNLIKELY(ret < 0)) {                                            \
            /* No more enough space: realloc the array */                     \
            size_t alloc = M_CALL_INC_ALLOC(oplist, (size_t) tree->capacity); \
            /* Take into account if realloc is allowed */                     \
            alloc += tree->allow_realloc;                                     \
            if (M_UNLIKELY_NOMEM (alloc >= INT32_MAX)) {                      \
                M_MEMORY_FULL(struct M_F(name, _node_s), alloc);              \
            }                                                                 \
            /* Allocate one more term in the array so that tab[-1] exists.    \
            This enables performing latter tab[M_TR33_NO_NODE].x = something; \
            as M_TR33_NO_NODE is -1. This enables avoiding testing for        \
            M_TR33_NO_NODE in some cases, performing branchless code. */      \
            struct M_F(name,_node_s)*ptr = tree->tab == NULL ? NULL : tree->tab-1; \
            ptr = M_CALL_REALLOC(oplist, struct M_F(name, _node_s), ptr, (size_t)(tree->capacity+1), alloc+1); \
            if (M_UNLIKELY_NOMEM (ptr == NULL) ) {                            \
                M_MEMORY_FULL(struct M_F(name, _node_s), alloc);              \
            }                                                                 \
            /* Skip the first term to keep it as empty & unused */            \
            ptr++;                                                            \
            /* Construct the list of free node in the extra allocated pool */ \
            M_ASSERT(tree->capacity >= 0);                                    \
            for(size_t i = (size_t) tree->capacity; i < alloc; i++) {         \
                ptr[i].parent = M_TR33_NO_NODE;                               \
                ptr[i].left   = M_TR33_NO_NODE;                               \
                ptr[i].right  = M_TR33_NO_NODE;                               \
                ptr[i].child  = (m_tr33_index_t) i + 1;                       \
            }                                                                 \
            /* The last node has no child in the free node list */            \
            ptr[alloc-1].child = M_TR33_NO_NODE;                              \
            /* Save the free list state in the tree */                        \
            tree->tab = ptr;                                                  \
            tree->capacity = (m_tr33_index_t) alloc;                          \
            ret = tree->size;                                                 \
        }                                                                     \
        /* Pop an element in the list of free nodes */                        \
        tree->free_index = tree->tab[ret].child;                              \
        tree->size ++;                                                        \
        return ret;                                                           \
    }                                                                         \
                                                                              \
    M_INLINE void                                                             \
    M_C3(m_tr33_, name, _free_node)(tree_t tree, m_tr33_index_t i) {          \
        tree->tab[i].parent = M_TR33_NO_NODE;                                 \
        tree->tab[i].left   = M_TR33_NO_NODE;                                 \
        tree->tab[i].right  = M_TR33_NO_NODE;                                 \
        tree->tab[i].child  = tree->free_index;                               \
        tree->size --;                                                        \
        tree->free_index = i;                                                 \
    }                                                                         \
                                                                              \
    M_P(it_t, name, _set_root, tree_t tree, type const data) {                \
        M_TR33_CONTRACT(tree);                                                \
        M_F(name, _reset)M_R(tree);                                           \
        m_tr33_index_t i = M_F(name, _i_alloc_node)M_R(tree);                 \
        tree->tab[i].parent = M_TR33_ROOT_NODE;                               \
        tree->tab[i].left   = M_TR33_NO_NODE;                                 \
        tree->tab[i].right  = M_TR33_NO_NODE;                                 \
        tree->tab[i].child  = M_TR33_NO_NODE;                                 \
        tree->root_index = i;                                                 \
        M_CALL_INIT_SET(oplist, tree->tab[i].data, data);                     \
        it_t it;                                                              \
        it.tree = tree;                                                       \
        it.index = tree->root_index;                                          \
        M_TR33_CONTRACT(tree);                                                \
        return it;                                                            \
    }                                                                         \
                                                                              \
    /* The iterator references the first root node */                         \
    /* usually for pre-order walk */                                          \
    M_INLINE it_t                                                             \
    M_F(name, _it)(tree_t tree) {                                             \
        M_TR33_CONTRACT(tree);                                                \
        it_t it;                                                              \
        it.tree = tree;                                                       \
        it.index = tree->root_index;                                          \
        M_TR33_IT_CONTRACT(it, false);                                        \
        return it;                                                            \
    }                                                                         \
                                                                              \
    M_INLINE it_t                                                             \
    M_F(name, _it_end)(tree_t tree) {                                         \
        M_TR33_CONTRACT(tree);                                                \
        it_t it;                                                              \
        it.tree = tree;                                                       \
        it.index = M_TR33_NO_NODE;                                            \
        M_TR33_IT_CONTRACT(it, false);                                        \
        return it;                                                            \
    }                                                                         \
                                                                              \
    M_INLINE bool                                                             \
    M_F(name, _end_p)(it_t it) {                                              \
        M_TR33_IT_CONTRACT(it, false);                                        \
        return it.index < 0;                                                  \
    }                                                                         \
                                                                              \
    M_INLINE type *                                                           \
    M_F(name, _ref)(it_t it) {                                                \
        M_TR33_IT_CONTRACT(it, true);                                         \
        return &it.tree->tab[it.index].data;                                  \
    }                                                                         \
                                                                              \
    M_INLINE type const *                                                     \
    M_F(name, _cref)(it_t it) {                                               \
        M_TR33_IT_CONTRACT(it, true);                                         \
        return M_CONST_CAST(type, &it.tree->tab[it.index].data);              \
    }                                                                         \
                                                                              \
    M_INLINE type *                                                           \
    M_F(name, _up_ref)(it_t it) {                                             \
        M_TR33_IT_CONTRACT(it, true);                                         \
        m_tr33_index_t i = it.tree->tab[it.index].parent;                     \
        return i < 0 ? NULL : &it.tree->tab[i].data;                          \
    }                                                                         \
                                                                              \
    M_INLINE type *                                                           \
    M_F(name, _down_ref)(it_t it) {                                           \
        M_TR33_IT_CONTRACT(it, true);                                         \
        m_tr33_index_t i = it.tree->tab[it.index].child;                      \
        return i < 0 ? NULL : &it.tree->tab[i].data;                          \
    }                                                                         \
                                                                              \
    M_INLINE type *                                                           \
    M_F(name, _left_ref)(it_t it) {                                           \
        M_TR33_IT_CONTRACT(it, true);                                         \
        m_tr33_index_t i = it.tree->tab[it.index].left;                       \
        return i < 0 ? NULL : &it.tree->tab[i].data;                          \
    }                                                                         \
                                                                              \
    M_INLINE type *                                                           \
    M_F(name, _right_ref)(it_t it) {                                          \
        M_TR33_IT_CONTRACT(it, true);                                         \
        m_tr33_index_t i = it.tree->tab[it.index].right;                      \
        return i < 0 ? NULL : &it.tree->tab[i].data;                          \
    }                                                                         \
                                                                              \
    M_P(it_t, name, _insert_up_raw, it_t it) {                                \
        M_TR33_IT_CONTRACT(it, true);                                         \
        m_tr33_index_t i = M_F(name, _i_alloc_node)M_R(it.tree);              \
        m_tr33_index_t parent = it.tree->tab[it.index].parent;                \
        m_tr33_index_t left   = it.tree->tab[it.index].left;                  \
        m_tr33_index_t right  = it.tree->tab[it.index].right;                 \
        it.tree->tab[i].parent = parent;                                      \
        it.tree->tab[i].left   = left;                                        \
        it.tree->tab[i].right  = right;                                       \
        it.tree->tab[i].child  = it.index;                                    \
        it.tree->tab[it.index].parent = i;                                    \
        it.tree->tab[it.index].left = M_TR33_NO_NODE;                         \
        it.tree->tab[it.index].right = M_TR33_NO_NODE;                        \
        if (M_UNLIKELY(it.tree->root_index == it.index)) {                    \
            /* We have added a parent to the root node. Update root index */  \
            it.tree->root_index = i;                                          \
            M_ASSERT( it.tree->tab[i].parent == M_TR33_ROOT_NODE);            \
        } else { if (it.tree->tab[parent].child == it.index) {                \
            /* Update the parent to point to the new child */                 \
            it.tree->tab[parent].child = i;                                   \
        } }                                                                   \
        it.tree->tab[left].right = i;                                         \
        it.tree->tab[right].left = i;                                         \
        /* Return updated iterator on the inserted node */                    \
        it.index = i;                                                         \
        M_TR33_IT_CONTRACT(it, true);                                         \
        return it;                                                            \
    }                                                                         \
                                                                              \
    M_P(it_t, name, _insert_up, it_t pos, type const data) {                  \
        it_t it = M_F(name, _insert_up_raw)M_R(pos);                          \
        M_CALL_INIT_SET(oplist, it.tree->tab[it.index].data, data);           \
        M_TR33_IT_CONTRACT(it, true);                                         \
        return it;                                                            \
    }                                                                         \
                                                                              \
    M_P(it_t, name, _move_up, it_t pos, type *data) {                         \
        it_t it = M_F(name, _insert_up_raw)M_R(pos);                          \
        M_CALL_INIT_MOVE(oplist, it.tree->tab[it.index].data, *data);         \
        M_TR33_IT_CONTRACT(it, true);                                         \
        return it;                                                            \
    }                                                                         \
                                                                              \
    M_P(it_t, name, _insert_down_raw, it_t it) {                              \
        M_TR33_IT_CONTRACT(it, true);                                         \
        m_tr33_index_t i = M_F(name, _i_alloc_node)M_R(it.tree);              \
        m_tr33_index_t child = it.tree->tab[it.index].child;                  \
        it.tree->tab[i].parent = it.index;                                    \
        it.tree->tab[i].left  = M_TR33_NO_NODE;                               \
        it.tree->tab[i].right = M_TR33_NO_NODE;                               \
        it.tree->tab[i].child = child;                                        \
        it.tree->tab[it.index].child = i;                                     \
        /* Update the parent of all the childs if at least one exists */      \
        while (child != M_TR33_NO_NODE) {                                     \
            it.tree->tab[child].parent = i;                                   \
            child = it.tree->tab[child].right;                                \
        }                                                                     \
        /* Return updated iterator on the inserted node */                    \
        it.index = i;                                                         \
        M_TR33_IT_CONTRACT(it, true);                                         \
        return it;                                                            \
    }                                                                         \
                                                                              \
    M_P(it_t, name, _insert_down, it_t pos, type const data) {                \
        it_t it = M_F(name, _insert_down_raw)M_R(pos);                        \
        M_CALL_INIT_SET(oplist, it.tree->tab[it.index].data, data);           \
        M_TR33_IT_CONTRACT(it, true);                                         \
        return it;                                                            \
    }                                                                         \
                                                                              \
    M_P(it_t, name, _move_down, it_t pos, type *data) {                       \
        it_t it = M_F(name, _insert_down_raw)M_R(pos);                        \
        M_CALL_INIT_MOVE(oplist, it.tree->tab[it.index].data, *data);         \
        M_TR33_IT_CONTRACT(it, true);                                         \
        return it;                                                            \
    }                                                                         \
                                                                              \
    M_P(it_t, name, _insert_child_raw, it_t it) {                             \
        M_TR33_IT_CONTRACT(it, true);                                         \
        /* Insert a node as a child of another, making the current children   \
            of the nodes their siblings */                                    \
        m_tr33_index_t i = M_F(name, _i_alloc_node)M_R(it.tree);              \
        m_tr33_index_t child = it.tree->tab[it.index].child;                  \
        it.tree->tab[i].parent = it.index;                                    \
        it.tree->tab[i].left  = M_TR33_NO_NODE;                               \
        it.tree->tab[i].right = child;                                        \
        it.tree->tab[i].child = M_TR33_NO_NODE;                               \
        /* Update the parent */                                               \
        it.tree->tab[it.index].child = i;                                     \
        /* Update the sibling */                                              \
        it.tree->tab[child].left = i;                                         \
        /* Return updated iterator on the inserted node */                    \
        it.index = i;                                                         \
        M_TR33_IT_CONTRACT(it, true);                                         \
        return it;                                                            \
    }                                                                         \
                                                                              \
    M_P(it_t, name, _insert_child, it_t pos, type const data) {               \
        it_t it = M_F(name, _insert_child_raw)M_R(pos);                       \
        M_CALL_INIT_SET(oplist, it.tree->tab[it.index].data, data);           \
        M_TR33_IT_CONTRACT(it, true);                                         \
        return it;                                                            \
    }                                                                         \
                                                                              \
    M_P(it_t, name, _move_child, it_t pos, type *data) {                      \
        it_t it = M_F(name, _insert_child_raw)M_R(pos);                       \
        M_CALL_INIT_MOVE(oplist, it.tree->tab[it.index].data, *data);         \
        M_TR33_IT_CONTRACT(it, true);                                         \
        return it;                                                            \
    }                                                                         \
                                                                              \
    M_P(it_t, name, _insert_left_raw, it_t it) {                              \
        M_TR33_IT_CONTRACT(it, true);                                         \
        M_ASSERT(it.index != it.tree->root_index);                            \
        m_tr33_index_t i = M_F(name, _i_alloc_node)M_R(it.tree);              \
        m_tr33_index_t left = it.tree->tab[it.index].left;                    \
        m_tr33_index_t parent = it.tree->tab[it.index].parent;                \
        it.tree->tab[i].parent = parent;                                      \
        it.tree->tab[i].left  = left;                                         \
        it.tree->tab[i].right = it.index;                                     \
        it.tree->tab[i].child = M_TR33_NO_NODE;                               \
        it.tree->tab[it.index].left = i;                                      \
        /* If there is a left node, update its right */                       \
        it.tree->tab[left].right = i;                                         \
        if (it.tree->tab[parent].child == it.index) {                         \
            /* Update the first child of the parent */                        \
            it.tree->tab[parent].child = i;                                   \
        }                                                                     \
        /* Return updated iterator on the inserted node */                    \
        it.index = i;                                                         \
        M_TR33_IT_CONTRACT(it, true);                                         \
        return it;                                                            \
    }                                                                         \
                                                                              \
    M_P(it_t, name, _insert_left, it_t pos, type const data) {                \
        it_t it = M_F(name, _insert_left_raw)M_R(pos);                        \
        M_CALL_INIT_SET(oplist, it.tree->tab[it.index].data, data);           \
        M_TR33_IT_CONTRACT(it, true);                                         \
        return it;                                                            \
    }                                                                         \
                                                                              \
    M_P(it_t, name, _move_left, it_t pos, type *data) {                       \
        it_t it = M_F(name, _insert_left_raw)M_R(pos);                        \
        M_CALL_INIT_MOVE(oplist, it.tree->tab[it.index].data, *data);         \
        M_TR33_IT_CONTRACT(it, true);                                         \
        return it;                                                            \
    }                                                                         \
                                                                              \
    M_P(it_t, name, _insert_right_raw, it_t it) {                             \
        M_TR33_IT_CONTRACT(it, true);                                         \
        M_ASSERT(it.index != it.tree->root_index);                            \
        m_tr33_index_t i = M_F(name, _i_alloc_node)M_R(it.tree);              \
        m_tr33_index_t right = it.tree->tab[it.index].right;                  \
        it.tree->tab[i].parent = it.tree->tab[it.index].parent;               \
        it.tree->tab[i].left   = it.index;                                    \
        it.tree->tab[i].right  = right;                                       \
        it.tree->tab[i].child  = M_TR33_NO_NODE;                              \
        it.tree->tab[right].left = i;                                         \
        it.tree->tab[it.index].right = i;                                     \
        /* Return updated iterator on the inserted node */                    \
        it.index = i;                                                         \
        M_TR33_IT_CONTRACT(it, true);                                         \
        return it;                                                            \
    }                                                                         \
                                                                              \
    M_P(it_t, name, _insert_right, it_t pos, type const data) {               \
        it_t it = M_F(name, _insert_right_raw)M_R(pos);                       \
        M_CALL_INIT_SET(oplist, it.tree->tab[it.index].data, data);           \
        M_TR33_IT_CONTRACT(it, true);                                         \
        return it;                                                            \
    }                                                                         \
                                                                              \
    M_P(it_t, name, _move_right, it_t pos, type *data) {                      \
        it_t it = M_F(name, _insert_right_raw)M_R(pos);                       \
        M_CALL_INIT_MOVE(oplist, it.tree->tab[it.index].data, *data);         \
        M_TR33_IT_CONTRACT(it, true);                                         \
        return it;                                                            \
    }                                                                         \
                                                                              \
    M_INLINE bool                                                             \
    M_F(name, _it_up)(it_t *it) {                                             \
        M_ASSERT(it != NULL);                                                 \
        M_TR33_IT_CONTRACT(*it, true);                                        \
        m_tr33_index_t i = it->tree->tab[it->index].parent;                   \
        bool ret = i >= 0;                                                    \
        if (M_LIKELY(ret)) {                                                  \
            it->index = i;                                                    \
        }                                                                     \
        M_TR33_IT_CONTRACT(*it, true);                                        \
        return ret;                                                           \
    }                                                                         \
                                                                              \
    M_INLINE bool                                                             \
    M_F(name, _it_down)(it_t *it) {                                           \
        M_ASSERT(it != NULL);                                                 \
        M_TR33_IT_CONTRACT(*it, true);                                        \
        m_tr33_index_t i = it->tree->tab[it->index].child;                    \
        bool ret = i >= 0;                                                    \
        if (M_LIKELY(ret)) {                                                  \
            it->index = i;                                                    \
        }                                                                     \
        M_TR33_IT_CONTRACT(*it, true);                                        \
        return ret;                                                           \
    }                                                                         \
                                                                              \
    M_INLINE bool                                                             \
    M_F(name, _it_left)(it_t *it) {                                           \
        M_ASSERT(it != NULL);                                                 \
        M_TR33_IT_CONTRACT(*it, true);                                        \
        m_tr33_index_t i = it->tree->tab[it->index].left;                     \
        bool ret = i >= 0;                                                    \
        if (M_LIKELY(ret)) {                                                  \
            it->index = i;                                                    \
        }                                                                     \
        M_TR33_IT_CONTRACT(*it, true);                                        \
        return ret;                                                           \
    }                                                                         \
                                                                              \
    M_INLINE bool                                                             \
    M_F(name, _it_right)(it_t *it) {                                          \
        M_ASSERT(it != NULL);                                                 \
        M_TR33_IT_CONTRACT(*it, true);                                        \
        m_tr33_index_t i = it->tree->tab[it->index].right;                    \
        bool ret = i >= 0;                                                    \
        if (M_LIKELY(ret)) {                                                  \
            it->index = i;                                                    \
        }                                                                     \
        M_TR33_IT_CONTRACT(*it, true);                                        \
        return ret;                                                           \
    }                                                                         \
                                                                              \
    M_INLINE it_t                                                             \
    M_F(name, _up)(it_t it) {                                                 \
        M_TR33_IT_CONTRACT(it, true);                                         \
        it_t i = { it.tree, it.tree->tab[it.index].parent };                  \
        return i;                                                             \
    }                                                                         \
                                                                              \
    M_INLINE it_t                                                             \
    M_F(name, _down)(it_t it) {                                               \
        M_TR33_IT_CONTRACT(it, true);                                         \
        it_t i = { it.tree, it.tree->tab[it.index].child };                   \
        return i;                                                             \
    }                                                                         \
                                                                              \
    M_INLINE it_t                                                             \
    M_F(name, _left)(it_t it) {                                               \
        M_TR33_IT_CONTRACT(it, true);                                         \
        it_t i = { it.tree, it.tree->tab[it.index].left };                    \
        return i;                                                             \
    }                                                                         \
                                                                              \
    M_INLINE it_t                                                             \
    M_F(name, _right)(it_t it) {                                              \
        M_TR33_IT_CONTRACT(it, true);                                         \
        it_t i = { it.tree, it.tree->tab[it.index].right };                   \
        return i;                                                             \
    }                                                                         \
                                                                              \
    M_INLINE bool                                                             \
    M_F(name, _root_p)(const it_t it) {                                       \
        M_TR33_IT_CONTRACT(it, true);                                         \
        return it.tree->tab[it.index].parent == M_TR33_ROOT_NODE;             \
    }                                                                         \
                                                                              \
    M_INLINE bool                                                             \
    M_F(name, _node_p)(const it_t it) {                                       \
        M_TR33_IT_CONTRACT(it, true);                                         \
        return it.tree->tab[it.index].child != M_TR33_NO_NODE;                \
    }                                                                         \
                                                                              \
    M_INLINE bool                                                             \
    M_F(name, _leaf_p)(const it_t it) {                                       \
        M_TR33_IT_CONTRACT(it, true);                                         \
        return it.tree->tab[it.index].child == M_TR33_NO_NODE;                \
    }                                                                         \
                                                                              \
    /* Compute the degree of a node in linear time */                         \
    M_INLINE int32_t                                                          \
    M_F(name, _degree)(const it_t it) {                                       \
        M_TR33_IT_CONTRACT(it, true);                                         \
        int32_t ret = 0;                                                      \
        m_tr33_index_t i = it.tree->tab[it.index].child;                      \
        while (i >= 0) {                                                      \
            ret ++;                                                           \
            i = it.tree->tab[i].right;                                        \
        }                                                                     \
        return ret;                                                           \
    }                                                                         \
                                                                              \
    /* Compute the depth of a node in linear time */                          \
    M_INLINE int32_t                                                          \
    M_F(name, _depth)(it_t it) {                                              \
        M_TR33_IT_CONTRACT(it, true);                                         \
        int32_t ret = 0;                                                      \
        m_tr33_index_t i = it.tree->tab[it.index].parent;                     \
        while (i >= 0) {                                                      \
            ret ++;                                                           \
            i = it.tree->tab[i].parent;                                       \
        }                                                                     \
        return ret;                                                           \
    }                                                                         \
                                                                              \
    M_INLINE struct M_F(name, _s) *                                           \
    M_F(name, _tree)(it_t it) {                                               \
        M_TR33_IT_CONTRACT(it, false);                                        \
        return it.tree;                                                       \
    }                                                                         \
                                                                              \
    M_INLINE void                                                             \
    M_F(name, _swap_at)(it_t it1, it_t it2, bool swapChild) {                 \
        M_ASSUME(it1.tree == it2.tree);                                       \
        M_TR33_IT_CONTRACT(it1, true);                                        \
        M_TR33_IT_CONTRACT(it2, true);                                        \
        if (M_UNLIKELY(it1.index == it2.index)) { return; }                   \
        /* Read all references before modifying anything */                   \
        m_tr33_index_t tmp1_l = it1.tree->tab[it1.index].left;                \
        m_tr33_index_t tmp2_l = it2.tree->tab[it2.index].left;                \
        m_tr33_index_t tmp1_r = it1.tree->tab[it1.index].right;               \
        m_tr33_index_t tmp2_r = it2.tree->tab[it2.index].right;               \
        m_tr33_index_t tmp1_d = it1.tree->tab[it1.index].child;               \
        m_tr33_index_t tmp2_d = it2.tree->tab[it2.index].child;               \
        m_tr33_index_t tmp1_u = it1.tree->tab[it1.index].parent;              \
        m_tr33_index_t tmp2_u = it2.tree->tab[it2.index].parent;              \
        /* Special cases if both nodes are siblings of the same node */       \
        if (tmp1_r == it2.index) {                                            \
            M_ASSERT(tmp2_l == it1.index);                                    \
            tmp1_r = it1.index;                                               \
            tmp2_l = it2.index;                                               \
        }                                                                     \
        if (tmp2_r == it1.index) {                                            \
            M_ASSERT(tmp1_l == it2.index);                                    \
            tmp2_r = it2.index;                                               \
            tmp1_l = it1.index;                                               \
        }                                                                     \
        if (tmp1_u == it2.index) {                                            \
            tmp1_u = it1.index;                                               \
            if (tmp2_d == it1.index) { tmp2_d = it2.index; }                  \
        }                                                                     \
        if (tmp2_u == it1.index) {                                            \
            tmp2_u = it2.index;                                               \
            if (tmp1_d == it2.index) { tmp1_d = it1.index; }                  \
        }                                                                     \
        /* Swap left references */                                            \
        it1.tree->tab[it1.index].left = tmp2_l;                               \
        it2.tree->tab[it2.index].left = tmp1_l;                               \
        it1.tree->tab[tmp1_l].right = it2.index;                              \
        it2.tree->tab[tmp2_l].right = it1.index;                              \
        /* Swap right references */                                           \
        it1.tree->tab[it1.index].right = tmp2_r;                              \
        it2.tree->tab[it2.index].right = tmp1_r;                              \
        it1.tree->tab[tmp1_r].left = it2.index;                               \
        it2.tree->tab[tmp2_r].left = it1.index;                               \
        /* Swap down references */                                            \
        if (swapChild == false) {                                             \
            it1.tree->tab[it1.index].child = tmp2_d;                          \
            it2.tree->tab[it2.index].child = tmp1_d;                          \
            while (tmp1_d >= 0) {                                             \
                it1.tree->tab[tmp1_d].parent = it2.index;                     \
                tmp1_d = it1.tree->tab[tmp1_d].right;                         \
            }                                                                 \
            while (tmp2_d >= 0) {                                             \
                it2.tree->tab[tmp2_d].parent = it1.index;                     \
                tmp2_d = it2.tree->tab[tmp2_d].right;                         \
            }                                                                 \
        }                                                                     \
        /* Swap up references */                                              \
        bool dont_swap_back = true;                                           \
        it1.tree->tab[it1.index].parent = tmp2_u;                             \
        it2.tree->tab[it2.index].parent = tmp1_u;                             \
        if (tmp1_u >= 0 && it1.tree->tab[tmp1_u].child == it1.index) {        \
            it1.tree->tab[tmp1_u].child = it2.index;                          \
            dont_swap_back = tmp1_u != tmp2_u;                                \
        }                                                                     \
        if (tmp1_u == M_TR33_ROOT_NODE) {                                     \
            it1.tree->root_index = it2.index;                                 \
            M_ASSERT(tmp1_u != tmp2_u);                                       \
        }                                                                     \
        /* Both may have the same parent (don't swap back in this case) */    \
        if (tmp2_u >= 0 && dont_swap_back && it2.tree->tab[tmp2_u].child == it2.index) { \
            it2.tree->tab[tmp2_u].child = it1.index;                          \
        }                                                                     \
        if (tmp2_u == M_TR33_ROOT_NODE) {                                     \
            it2.tree->root_index = it1.index;                                 \
            M_ASSERT(tmp1_u != tmp2_u);                                       \
        }                                                                     \
        M_TR33_IT_CONTRACT(it1, true);                                        \
        M_TR33_IT_CONTRACT(it2, true);                                        \
    }                                                                         \
                                                                              \
    M_INLINE type *                                                           \
    M_F(name, _unlink)(it_t it) {                                             \
        M_TR33_IT_CONTRACT(it, true);                                         \
        m_tr33_index_t parent, child, left, right, child_r;                   \
        parent = it.tree->tab[it.index].parent;                               \
        child  = it.tree->tab[it.index].child;                                \
        left   = it.tree->tab[it.index].left;                                 \
        right  = it.tree->tab[it.index].right;                                \
        /* Test if No child for this node */                                  \
        if (child == M_TR33_NO_NODE) {                                        \
            remove_child_no_node:                                             \
            /* Remove node from sibling */                                    \
            it.tree->tab[left].right = right;                                 \
            it.tree->tab[right].left = left;                                  \
            /* Remove node from parent if it is the first child */            \
            if (parent >= 0 && it.tree->tab[parent].child == it.index) {      \
                M_ASSERT(left == M_TR33_NO_NODE);                             \
                it.tree->tab[parent].child = right;                           \
            } else if (parent == M_TR33_ROOT_NODE) {                          \
                it.tree->root_index = right;                                  \
            }                                                                 \
        } else {                                                              \
            if (M_UNLIKELY(it.index == it.tree->root_index)) {                \
                /* complex case. Swap root with its first child */            \
                it_t it_child = { it.tree, child };                           \
                M_F(name, _swap_at)(it, it_child, false);                     \
                parent = it.tree->tab[it.index].parent;                       \
                child  = it.tree->tab[it.index].child;                        \
                left   = it.tree->tab[it.index].left;                         \
                right  = it.tree->tab[it.index].right;                        \
                if (child == M_TR33_NO_NODE) { goto remove_child_no_node; }   \
            }                                                                 \
            /* Merge the child with the current siblings */                   \
            /* Compute the range of childs & update their parent */           \
            size_t num_child = 1;                                             \
            child_r = child;                                                  \
            it.tree->tab[child_r].parent = parent;                            \
            while (it.tree->tab[child_r].right != M_TR33_NO_NODE) {           \
                child_r = it.tree->tab[child_r].right;                        \
                it.tree->tab[child_r].parent = parent;                        \
                num_child ++;                                                 \
            }                                                                 \
            (void) num_child;                                                 \
            M_ASSERT(it.index != it.tree->root_index || num_child == 1);      \
            /* Remove node from sibling */                                    \
            it.tree->tab[left].right = child;                                 \
            it.tree->tab[child].left = left;                                  \
            it.tree->tab[right].left = child_r;                               \
            it.tree->tab[child_r].right = right;                              \
            /* Remove node from parent if it is the first child */            \
            if (parent >= 0 && it.tree->tab[parent].child == it.index) {      \
                M_ASSERT(left == M_TR33_NO_NODE);                             \
                it.tree->tab[parent].child = child;                           \
            }                                                                 \
            M_ASSERT (parent != M_TR33_ROOT_NODE);                            \
        }                                                                     \
        /* Free the node to the allocator */                                  \
        M_C3(m_tr33_, name, _free_node)(it.tree, it.index);                   \
        return &it.tree->tab[it.index].data;                                  \
    }                                                                         \
                                                                              \
    M_P(bool, name, _remove, it_t it) {                                       \
        M_TR33_IT_CONTRACT(it, false);                                        \
        M_UNUSED_CONTEXT();                                                   \
        if (M_UNLIKELY(it.index < 0)) { return false; }                       \
        type *ptr = M_F(name, _unlink)(it);                                   \
        M_CALL_CLEAR(oplist, *ptr);                                           \
        return true;                                                          \
    }                                                                         \
                                                                              \
    /* Scan all nodes, first the parent then the children (uses with _it) */  \
    /* pre-order walk */                                                      \
    M_INLINE void                                                             \
    M_F(name, _next)(it_t *it) {                                              \
        M_TR33_IT_CONTRACT(*it, true);                                        \
        /* First go down, if impossible go right */                           \
        if (M_F(name, _it_down)(it) || M_F(name, _it_right)(it)) {            \
            return;                                                           \
        }                                                                     \
        /* If impossible to go right, move up and then right until impossible */ \
        while (M_F(name, _it_up)(it)) {                                       \
            if (M_F(name, _it_right)(it)) {                                   \
                return;                                                       \
            }                                                                 \
        }                                                                     \
        /* Reach end of tree */                                               \
        it->index = M_TR33_NO_NODE;                                           \
        M_TR33_IT_CONTRACT(*it, false);                                       \
    }                                                                         \
                                                                              \
    /* Scan all nodes, first the children then the parent */                  \
    /* post-order walk */                                                     \
    M_INLINE it_t                                                             \
    M_F(name, _it_post)(tree_t tree) {                                        \
        M_TR33_CONTRACT(tree);                                                \
        it_t it;                                                              \
        it.tree = tree;                                                       \
        it.index = tree->root_index;                                          \
        /* Evaluate child first, so go down to the lowest child */            \
        while (M_F(name, _it_down)(&it)) {}                                   \
        M_TR33_IT_CONTRACT(it, false);                                        \
        return it;                                                            \
    }                                                                         \
                                                                              \
    /* Scan all nodes, first the children then the parent (uses with _it_post) */ \
    /* post-order walk */                                                     \
    M_INLINE void                                                             \
    M_F(name, _next_post)(it_t *it) {                                         \
        M_TR33_IT_CONTRACT(*it, true);                                        \
        /* First go right */                                                  \
        if (M_F(name, _it_right)(it)) {                                       \
            /* then go down */                                                \
            while (M_F(name, _it_down)(it)) {}                                \
            return;                                                           \
        }                                                                     \
        /* If impossible to go right, move up */                              \
        if (M_F(name, _it_up)(it)) {                                          \
            return;                                                           \
        }                                                                     \
        /* Reach end of tree */                                               \
        it->index = M_TR33_NO_NODE;                                           \
        M_TR33_IT_CONTRACT(*it, false);                                       \
    }                                                                         \
                                                                              \
    M_INLINE bool                                                             \
    M_F(name, _it_equal_p)(it_t it1, it_t it2) {                              \
        M_TR33_IT_CONTRACT(it1, false);                                       \
        M_TR33_IT_CONTRACT(it2, false);                                       \
        return it1.tree == it2.tree && it1.index == it2.index;                \
    }                                                                         \
                                                                              \
    /* Scan all nodes, first the parent, then the children */                 \
    /* post-order walk */                                                     \
    M_INLINE it_t                                                             \
    M_F(name, _it_subpre)(it_t it) {                                          \
        /* Nothing to do as it is already on the parent! */                   \
        return it;                                                            \
    }                                                                         \
                                                                              \
    /* Scan the nodes of it_ref, first the parent then the children */        \
    /* pre-order walk */                                                      \
    M_INLINE void                                                             \
    M_F(name, _next_subpre)(it_t *it, it_t it_ref) {                          \
        M_TR33_IT_CONTRACT(*it, true);                                        \
        M_TR33_IT_CONTRACT(it_ref, true);                                     \
        M_ASSERT(it->tree == it_ref.tree);                                    \
        /* First go down, if impossible go right */                           \
        if (M_F(name, _it_down)(it)) { return; }                              \
        if (M_F(name, _it_right)(it)) { return;}                              \
        /* If impossible to go right, move up and then right for the all section */ \
        while (M_F(name, _it_up)(it) && it->index != it_ref.index) {          \
            if (M_F(name, _it_right)(it)) {                                   \
                return;                                                       \
            }                                                                 \
        }                                                                     \
        /* Reach end of section */                                            \
        it->index = M_TR33_NO_NODE;                                           \
        M_TR33_IT_CONTRACT(*it, false);                                       \
    }                                                                         \
                                                                              \
    /* Scan all nodes, first the children then the parent */                  \
    /* post-order walk */                                                     \
    M_INLINE it_t                                                             \
    M_F(name, _it_subpost)(it_t it) {                                         \
        M_TR33_IT_CONTRACT(it, true);                                         \
        /* Evaluate child first, so go down to the lowest child */            \
        while (M_F(name, _it_down)(&it)) {}                                   \
        M_TR33_IT_CONTRACT(it, false);                                        \
        return it;                                                            \
    }                                                                         \
                                                                              \
    /* Scan all nodes, first the children then the parent (uses with _it_subpost) */ \
    /* post-order walk */                                                     \
    M_INLINE void                                                             \
    M_F(name, _next_subpost)(it_t *it, it_t ref) {                            \
        M_TR33_IT_CONTRACT(*it, true);                                        \
        M_TR33_IT_CONTRACT(ref, true);                                        \
        M_ASSERT(it->tree == ref.tree);                                       \
        if (it->index == ref.index) {                                         \
            /* Reach end of tree */                                           \
            it->index = M_TR33_NO_NODE;                                       \
            return;                                                           \
        }                                                                     \
        /* First go right */                                                  \
        if (M_F(name, _it_right)(it)) {                                       \
            /* then go down */                                                \
            while (M_F(name, _it_down)(it)) {}                                \
            return;                                                           \
        }                                                                     \
        /* If impossible to go right, move up */                              \
        bool b = M_F(name, _it_up)(it);                                       \
        (void) b; /* parameter not used */                                    \
        assert(b);                                                            \
    }                                                                         \
                                                                              \
    M_P(void, name, _prune, it_t it) {                                        \
        M_TR33_IT_CONTRACT(it, true);                                         \
        /* remove the node, including its childs */                           \
        it_t child = M_F(name, _it_subpost)(it);                              \
        while (!M_F(name, _end_p)(child)) {                                   \
            it_t next = child;                                                \
            M_F(name, _next_subpost)(&next, it);                              \
            bool b = M_F(name, _remove)M_R(child);                            \
            (void) b; /* parameter not used */                                \
            M_ASSERT(b);                                                      \
            child = next;                                                     \
        }                                                                     \
    }                                                                         \
                                                                              \
    M_INLINE it_t                                                             \
    M_F(name, _lca)(it_t it1, it_t it2) {                                     \
        M_TR33_IT_CONTRACT(it1, true);                                        \
        M_TR33_IT_CONTRACT(it2, true);                                        \
        M_ASSERT(it1.tree == it2.tree);                                       \
        /* Compute the Lowest Common Ancestor in linear time */               \
        int32_t d1 = M_F(name, _depth)(it1);                                  \
        int32_t d2 = M_F(name, _depth)(it2);                                  \
        bool b = true;                                                        \
        if (d1 > d2) {                                                        \
            M_SWAP(int32_t, d1, d2);                                          \
            M_SWAP(it_t, it1, it2);                                           \
        }                                                                     \
        /* it2 is deeper than it1 */                                          \
        while (d1 < d2) {                                                     \
            b = M_F(name, _it_up)(&it2);                                      \
            d2--;                                                             \
            assert(b);                                                        \
        }                                                                     \
        /* Move up both iterators until we found the common node */           \
        while (b && it1.index != it2.index) {                                 \
            b = M_F(name, _it_up)(&it1);                                      \
            b = M_F(name, _it_up)(&it2);                                      \
        }                                                                     \
        /* If we went back to the root node, we must have found a common node*/ \
        M_ASSERT(b);                                                          \
        return it1;                                                           \
    }                                                                         \
                                                                              \
    M_INLINE void                                                             \
    M_F(name, _graft_child)(it_t it1, const it_t it2) {                       \
        M_ASSERT(it1.tree == it2.tree);                                       \
        M_TR33_IT_CONTRACT(it1, true);                                        \
        M_TR33_IT_CONTRACT(it2, true);                                        \
        M_ASSERT(it2.index != it2.tree->root_index);                          \
        /* Move the node it2 and its child down the node *it1 */              \
        /* Both belongs to the same tree */                                   \
        const m_tr33_index_t i = it2.index;                                   \
        /* Unlink it2 except its child */                                     \
        const m_tr33_index_t parent = it1.tree->tab[i].parent;                \
        if (parent >= 0 && it1.tree->tab[parent].child == i) {                \
            it1.tree->tab[parent].child = it1.tree->tab[i].right;             \
        }                                                                     \
        const m_tr33_index_t left = it1.tree->tab[i].left;                    \
        it1.tree->tab[left].right = it1.tree->tab[i].right;                   \
        const m_tr33_index_t right = it1.tree->tab[i].right;                  \
        it1.tree->tab[right].left = it1.tree->tab[i].left;                    \
        /* Add the new node */                                                \
        const m_tr33_index_t child = it1.tree->tab[it1.index].child;          \
        it1.tree->tab[i].parent = it1.index;                                  \
        it1.tree->tab[i].left  = M_TR33_NO_NODE;                              \
        it1.tree->tab[i].right = child;                                       \
        /* Update the parent */                                               \
        it1.tree->tab[it1.index].child = i;                                   \
        /* Update the sibling */                                              \
        it1.tree->tab[child].left = i;                                        \
        M_TR33_IT_CONTRACT(it1, true);                                        \
        M_TR33_IT_CONTRACT(it2, true);                                        \
    }                                                                         \
                                                                              \
    M_IF_METHOD(CMP,oplist)(                                                  \
    M_INLINE void                                                             \
    M_F(name, _sort_child)(it_t it0) {                                        \
        M_TR33_IT_CONTRACT(it0, true);                                        \
        it_t it1 = it0;                                                       \
        /* Go to the child, if it doesn't exist, nothing to sort */           \
        if (!M_F(name, _it_down)(&it1) ) return ;                             \
        /* Selection sort */                                                  \
        do {                                                                  \
            it_t it_min = it1;                                                \
            it_t it2    = it1;                                                \
            while (M_F(name, _it_right)(&it2)) {                              \
                if (M_CALL_CMP(oplist, *M_F(name, _cref)(it2), *M_F(name, _cref)(it_min)) < 0) { \
                    it_min = it2;                                             \
                }                                                             \
            }                                                                 \
            if (M_F(name, _it_equal_p)(it_min, it1) == false) {               \
                M_F(name, _swap_at)(it1, it_min, true);                       \
                /* The iterator it1 is no longer the min */                   \
                it1 = it_min;                                                 \
            }                                                                 \
        } while (M_F(name, _it_right)(&it1));                                 \
        M_TR33_IT_CONTRACT(it0, true);                                        \
    }                                                                         \
    , /* No CMP */ )                                                          \


/* Define the classic extended missing methods of a tree */
#define M_TR33_DEF_P4_EXT(name, type, oplist, tree_t, it_t)                   \
    M_P(void, name, _init_set, tree_t tree, const tree_t ref) {               \
        tree->size = ref->size;                                               \
        tree->capacity = ref->capacity;                                       \
        tree->root_index = ref->root_index;                                   \
        tree->free_index = ref->free_index;                                   \
        tree->allow_realloc = ref->allow_realloc;                             \
        size_t alloc = (size_t) ref->capacity;                                \
        if (ref->tab == NULL) {                                               \
            tree->tab = NULL;                                                 \
        } else {                                                              \
            struct M_F(name, _node_s) *ptr =                                  \
                M_CALL_REALLOC(oplist, struct M_F(name, _node_s), NULL, 0, alloc+1); \
            if (M_UNLIKELY_NOMEM (ptr == NULL) ) {                            \
                M_MEMORY_FULL(struct M_F(name, _node_s), alloc);              \
            }                                                                 \
            tree->tab = ++ptr;                                                \
            /* We don't scan recursively the node tree, but sequentially */   \
            for(m_tr33_index_t i = 0 ; i < ref->capacity ; i ++) {            \
                tree->tab[i].parent = ref->tab[i].parent;                     \
                tree->tab[i].child  = ref->tab[i].child;                      \
                tree->tab[i].left   = ref->tab[i].left;                       \
                tree->tab[i].right  = ref->tab[i].right;                      \
                /* If the node is not a free node, copy the data  */          \
                if (tree->tab[i].parent != M_TR33_NO_NODE) {                  \
                    M_CALL_INIT_SET(oplist, tree->tab[i].data, ref->tab[i].data); \
                }                                                             \
            }                                                                 \
        }                                                                     \
        M_TR33_CONTRACT(tree);                                                \
    }                                                                         \
                                                                              \
    M_P(void, name, _set, tree_t tree, const tree_t ref) {                    \
        /* No optimum, but good enough currently */                           \
        M_F(name, _clear)M_R(tree);                                           \
        M_F(name, _init_set)M_R(tree, ref);                                   \
    }                                                                         \
                                                                              \
    M_INLINE void                                                             \
    M_F(name, _init_move)(tree_t tree, tree_t ref) {                          \
        tree->size = ref->size;                                               \
        tree->capacity = ref->capacity;                                       \
        tree->root_index = ref->root_index;                                   \
        tree->free_index = ref->free_index;                                   \
        tree->allow_realloc = ref->allow_realloc;                             \
        tree->tab = ref->tab;                                                 \
        /* This is so reusing the object implies an assertion failure */      \
        ref->size = 1;                                                        \
        ref->tab = NULL;                                                      \
    }                                                                         \
                                                                              \
    M_P(void, name, _move, tree_t tree, tree_t ref) {                         \
        M_F(name, _clear)M_R(tree);                                           \
        M_F(name, _init_move)(tree, ref);                                     \
    }                                                                         \
                                                                              \
    M_INLINE void                                                             \
    M_F(name, _swap)(tree_t tree1, tree_t tree2) {                            \
        M_TR33_CONTRACT(tree1);                                               \
        M_TR33_CONTRACT(tree2);                                               \
        M_SWAP(m_tr33_index_t, tree1->size, tree2->size);                     \
        M_SWAP(m_tr33_index_t, tree1->capacity, tree2->capacity);             \
        M_SWAP(m_tr33_index_t, tree1->root_index, tree2->root_index);         \
        M_SWAP(m_tr33_index_t, tree1->free_index, tree2->free_index);         \
        M_SWAP(unsigned, tree1->allow_realloc, tree2->allow_realloc);         \
        M_SWAP(M_F(name, _node_ct) *, tree1->tab, tree2->tab);                \
        M_TR33_CONTRACT(tree1);                                               \
        M_TR33_CONTRACT(tree2);                                               \
    }                                                                         \
                                                                              \
    M_INLINE size_t                                                           \
    M_F(name, _size)(const tree_t tree) {                                     \
        M_TR33_CONTRACT(tree);                                                \
        return (size_t) tree->size;                                           \
    }                                                                         \
                                                                              \
    M_INLINE bool                                                             \
    M_F(name, _empty_p)(const tree_t tree) {                                  \
        M_TR33_CONTRACT(tree);                                                \
        return tree->size == 0;                                               \
    }                                                                         \
                                                                              \
    M_INLINE size_t                                                           \
    M_F(name, _capacity)(const tree_t tree) {                                 \
        M_TR33_CONTRACT(tree);                                                \
        return (size_t) tree->capacity;                                       \
    }                                                                         \
                                                                              \
    /* Service not really useful as the affectation operator works with it */ \
    M_INLINE void                                                             \
    M_F(name, _it_set)(it_t *dst, it_t src ){                                 \
        *dst = src;                                                           \
    }                                                                         \
                                                                              \
    M_IF_METHOD(EQUAL, oplist)(                                               \
    M_INLINE bool                                                             \
    M_F(name, _equal_p)(/*const*/ tree_t t1, /*const*/ tree_t t2) {           \
        M_TR33_CONTRACT(t1);                                                  \
        M_TR33_CONTRACT(t2);                                                  \
        /* Fast case if the sizes don't match */                              \
        if (M_LIKELY(t1->size != t2->size)) {                                 \
            return false;                                                     \
        }                                                                     \
        /* Slow case. We need to scan both tree                               \
           and check if we move in the same way                               \
           while checking also the data                                       \
           */                                                                 \
        it_t it1 = M_F(name, _it)(t1);                                        \
        it_t it2 = M_F(name, _it)(t2);                                        \
        while (!M_F(name, _end_p)(it1)) {                                     \
            /* Since both trees have the same size,                           \
               both iterators shall end at the same time. */                  \
            M_ASSERT(!M_F(name, _end_p)(it2));                                \
            bool b = M_CALL_EQUAL(oplist, *M_F(name, _cref)(it1), *M_F(name, _cref)(it2)); \
            if (!b) return false;                                             \
            /* First go down, if impossible go right */                       \
            if (M_F(name, _it_down)(&it1) ) {                                 \
                b = M_F(name, _it_down)(&it2);                                \
                if (!b) return false;                                         \
                continue;                                                     \
            }                                                                 \
            if (M_F(name, _it_right)(&it1)) {                                 \
                b = M_F(name, _it_right)(&it2);                               \
                if (!b) return false;                                         \
                continue;                                                     \
            }                                                                 \
            /* If impossible, move up and then right until impossible */      \
            while (M_F(name, _it_up)(&it1)) {                                 \
                /* Both iterators have move down the same tree by the same    \
                   amount. Therefore if we can move up one iterator, we can   \
                   move the other one up too*/                                \
                b = M_F(name, _it_up)(&it2);                                  \
                M_ASSERT (b);                                                 \
                if (M_F(name, _it_right)(&it1)) {                             \
                    /* it2 right child may not exist */                       \
                    b = M_F(name, _it_right)(&it2);                           \
                    if (!b) return false;                                     \
                    goto do_continue;                                         \
                }                                                             \
            }                                                                 \
            /* Both tree have the same size and the same "local" depth        \
               Since there is no longer any node up iterator it1,             \
               there shall not be any node for iterator it2 (same size        \
               same depth)                                                    \
             */                                                               \
            M_ASSERT( M_F(name, _it_up)(&it2) == false);                      \
            return true;                                                      \
            /* Reach end of tree */                                           \
            do_continue:                                                      \
            continue;                                                         \
        }                                                                     \
        M_ASSERT(M_F(name, _end_p)(it2));                                     \
        return true;                                                          \
    }                                                                         \
    , /* No EQUAL */ )                                                        \
                                                                              \
    M_IF_METHOD(HASH, oplist)(                                                \
    M_INLINE size_t                                                           \
    M_F(name, _hash)(/* const */ tree_t t1) {                                 \
        M_HASH_DECL(hash);                                                    \
        for(it_t it = M_F(name, _it)(t1);                                     \
            !M_F(name, _end_p)(it) ;                                          \
            M_F(name, _next)(&it)) {                                          \
            size_t h = M_CALL_HASH(oplist, *M_F(name, _cref)(it));            \
            M_HASH_UP(hash, h);                                               \
        }                                                                     \
        return M_HASH_FINAL(hash);                                            \
    }                                                                         \
    , /* No HASH */ )                                                         \
    

/* Define the IO methods of a tree */
#define M_TR33_DEF_P4_IO(name, type, oplist, tree_t, it_t)                    \
M_IF_METHOD(GET_STR, oplist)(                                                 \
M_P(void, name, _get_str, m_string_t str, /*const*/ tree_t tree, bool append) { \
    (append ? m_string_cat_cstr : m_string_set_cstr) M_R(str, "[");           \
    it_t it = M_F(name, _it)(tree);                                           \
    while (!M_F(name, _end_p)(it)) {                                          \
        m_string_push_back M_R(str, '{');                                     \
        type const *item = M_F(name, _cref)(it);                              \
        M_CALL_GET_STR(oplist, str, *item, true);                             \
        /* Go down the tree */                                                \
        if (M_F(name, _it_down)(&it)) {                                       \
            m_string_push_back M_R(str, M_GET_SEPARATOR oplist);              \
            m_string_push_back M_R(str, '[');                                 \
            continue;                                                         \
        }                                                                     \
            m_string_push_back M_R(str, '}');                                 \
        if (M_F(name, _it_right)(&it)) {                                      \
            m_string_push_back M_R(str, M_GET_SEPARATOR oplist);              \
            continue;                                                         \
        }                                                                     \
        while (M_F(name, _it_up)(&it)) {                                      \
            m_string_push_back M_R(str, ']');                                 \
            m_string_push_back M_R(str, '}');                                 \
            if (M_F(name, _it_right)(&it)) {                                  \
                m_string_push_back M_R(str, M_GET_SEPARATOR oplist);          \
                goto continue_tree;                                           \
            }                                                                 \
        }                                                                     \
        it = M_F(name, _it_end)(tree);                                        \
        continue_tree:                                                        \
        (void) 0;                                                             \
    }                                                                         \
    m_string_push_back M_R(str, ']');                                         \
}                                                                             \
, /* No GET_STR */ )                                                          \
                                                                              \
M_IF_METHOD(PARSE_STR, oplist)(                                               \
M_P(bool, name, _parse_str, tree_t tree, const char str[], const char **endp) { \
    M_TR33_CONTRACT(tree);                                                    \
    int cmd = 0;                                                              \
    it_t it;                                                                  \
    M_F(name, _reset)M_R(tree);                                               \
    bool success = false;                                                     \
    int c = *str;                                                             \
    if (M_UNLIKELY (c != '[')) goto exit;                                     \
    c = *++str;                                                               \
    if (M_UNLIKELY (c == ']')) { success = true; str++; goto exit; }          \
    if (M_UNLIKELY (c == 0)) goto exit;                                       \
    M_QLET(1, item, type, oplist) {                                           \
        it = M_F(name, _it_end)(tree);                                        \
        while (true) {                                                        \
            c = *str++;                                                       \
            if (c != '{') break;                                              \
            bool b = M_CALL_PARSE_STR(oplist, item, str, &str);               \
            c = *str++;                                                       \
            if (b == false || c == 0) break;                                  \
            /* Insert the item as the root or as a right sibling or as a child */ \
            switch (cmd) {                                                    \
                case 0: it = M_F(name, _set_root)M_R(tree, item); break;      \
                case 1: it = M_F(name, _insert_right)M_R(it, item); break;    \
                case 2: it = M_F(name, _insert_child)M_R(it, item); break;    \
                default: M_ASSERT(0);                                         \
            }                                                                 \
            if (c == ',') {                                                   \
                /* The current item has some children */                      \
                c = *str++;                                                   \
                if (c != '[') break;                                          \
                /* next is a child: push_down */                              \
                cmd = 2;                                                      \
                continue;                                                     \
            }                                                                 \
            /* The current item has no children. */                           \
            if (c != '}') break;                                              \
            /* Scan the next character to decide where to move (right or up) */ \
            c = *str++;                                                       \
            if (c == ']') {                                                   \
                do {                                                          \
                    /* move up. It we cannot, we have reached the end */      \
                    if (!M_F(name, _it_up)(&it)) { success = true; break; }   \
                    c = *str++;                                               \
                    if (c != '}') goto do_break;                              \
                    c = *str++;                                               \
                } while (c == ']');                                           \
            }                                                                 \
            if (c != ',') { do_break: break; }                                \
            /* next is a sibling: push_right */                               \
            cmd = 1;                                                          \
        }                                                                     \
    }                                                                         \
exit:                                                                         \
    if (endp) *endp = str;                                                    \
    M_TR33_CONTRACT(tree);                                                    \
    return success;                                                           \
}                                                                             \
, /* No PARSE_STR */ )                                                        \
                                                                              \
M_IF_METHOD(OUT_STR, oplist)(                                                 \
M_INLINE void                                                                 \
M_F(name, _out_str)(FILE *f, /*const*/ tree_t tree) {                         \
    fputc('[', f);                                                            \
    it_t it = M_F(name, _it)(tree);                                           \
    while (!M_F(name, _end_p)(it)) {                                          \
        fputc('{', f);                                                        \
        type const *item = M_F(name, _cref)(it);                              \
        M_CALL_OUT_STR(oplist, f, *item);                                     \
        /* Go down the tree */                                                \
        if (M_F(name, _it_down)(&it)) {                                       \
            fputc (M_GET_SEPARATOR oplist, f);                                \
            fputc ('[', f);                                                   \
            continue;                                                         \
        }                                                                     \
            fputc ('}', f);                                                   \
        if (M_F(name, _it_right)(&it)) {                                      \
            fputc (M_GET_SEPARATOR oplist, f);                                \
            continue;                                                         \
        }                                                                     \
        while (M_F(name, _it_up)(&it)) {                                      \
            fputc (']', f);                                                   \
            fputc ('}', f);                                                   \
            if (M_F(name, _it_right)(&it)) {                                  \
                fputc (M_GET_SEPARATOR oplist, f);                            \
                goto continue_tree;                                           \
            }                                                                 \
        }                                                                     \
        it = M_F(name, _it_end)(tree);                                        \
        continue_tree:                                                        \
        (void) 0;                                                             \
    }                                                                         \
    fputc (']', f);                                                           \
}                                                                             \
, /* No OUT_STR */ )                                                          \
                                                                              \
M_IF_METHOD(IN_STR, oplist)(                                                  \
M_P(bool, name, _in_str, tree_t tree, FILE *f) {                              \
    M_TR33_CONTRACT(tree);                                                    \
    int cmd = 0;                                                              \
    it_t it;                                                                  \
    M_F(name, _reset)M_R(tree);                                               \
    bool success = false;                                                     \
    int c = fgetc(f);                                                         \
    if (M_UNLIKELY (c != '[')) goto exit;                                     \
    c = fgetc(f);                                                             \
    if (M_UNLIKELY (c == ']')) { success = true; goto exit; }                 \
    ungetc(c, f);                                                             \
    if (M_UNLIKELY (c == 0)) goto exit;                                       \
    M_QLET(1, item, type, oplist) {                                           \
        it = M_F(name, _it_end)(tree);                                        \
        while (true) {                                                        \
            c = fgetc(f);                                                     \
            if (c != '{') break;                                              \
            bool b = M_CALL_IN_STR(oplist, item, f);                          \
            c = fgetc(f);                                                     \
            if (b == false || c == 0) break;                                  \
            /* Insert the item as the root or as a right sibling or as a child */ \
            switch (cmd) {                                                    \
                case 0: it = M_F(name, _set_root)M_R(tree, item); break;      \
                case 1: it = M_F(name, _insert_right)M_R(it, item); break;    \
                case 2: it = M_F(name, _insert_child)M_R(it, item); break;    \
                default: M_ASSERT(0);                                         \
            }                                                                 \
            if (c == ',') {                                                   \
                /* The current item has some children */                      \
                c = fgetc(f);                                                 \
                if (c != '[') break;                                          \
                /* next is a child: push_down */                              \
                cmd = 2;                                                      \
                continue;                                                     \
            }                                                                 \
            /* The current item has no children. */                           \
            if (c != '}') break;                                              \
            /* Scan the next character to decide where to move (right or up) */ \
            c = fgetc(f);                                                     \
            if (c == ']') {                                                   \
                do {                                                          \
                    /* move up. It we cannot, we have reached the end */      \
                    if (!M_F(name, _it_up)(&it)) { success = true ; break; }  \
                    c = fgetc(f);                                             \
                    if (c != '}') goto do_break;                              \
                    c = fgetc(f);                                             \
                } while (c == ']');                                           \
            }                                                                 \
            if (c != ',') { do_break: break; }                                \
            /* next is a sibling: push_right */                               \
            cmd = 1;                                                          \
        }                                                                     \
    }                                                                         \
exit:                                                                         \
    M_TR33_CONTRACT(tree);                                                    \
    return success;                                                           \
}                                                                             \
, /* No IN_STR */ )                                                           \

/* Define the emplace methods of a tree */
#define M_TR33_DEF_P4_EMPLACE(name, type, oplist, tree_t, it_t)               \
  M_EMPLACE_QUEUE_DEF(name, tree_t, _emplace_root, oplist, M_TR33_EMPLACE_ROOT_DEF) \
  M_EMPLACE_QUEUE_DEF(name, it_t, _emplace_up, oplist, M_TR33_EMPLACE_UP_DEF) \
  M_EMPLACE_QUEUE_DEF(name, it_t, _emplace_down, oplist, M_TR33_EMPLACE_DOWN_DEF) \
  M_EMPLACE_QUEUE_DEF(name, it_t, _emplace_child, oplist, M_TR33_EMPLACE_CHILD_DEF) \
  M_EMPLACE_QUEUE_DEF(name, it_t, _emplace_left, oplist, M_TR33_EMPLACE_LEFT_DEF) \
  M_EMPLACE_QUEUE_DEF(name, it_t, _emplace_right, oplist, M_TR33_EMPLACE_RIGHT_DEF)

/* Definition of the emplace_back functions */
#define M_TR33_EMPLACE_ROOT_DEF(name, name_t, function_name, oplist, init_func, exp_emplace_type) \
  M_P(M_F(name, _it_ct), name, function_name, name_t tree M_EMPLACE_LIST_TYPE_VAR(a, exp_emplace_type) ) \
    {                                                                         \
        M_TR33_CONTRACT(tree);                                                \
        M_F(name, _reset)M_R(tree);                                           \
        m_tr33_index_t i = M_F(name, _i_alloc_node)M_R(tree);                 \
        tree->tab[i].parent = M_TR33_ROOT_NODE;                               \
        tree->tab[i].left   = M_TR33_NO_NODE;                                 \
        tree->tab[i].right  = M_TR33_NO_NODE;                                 \
        tree->tab[i].child  = M_TR33_NO_NODE;                                 \
        tree->root_index = i;                                                 \
        M_EMPLACE_CALL_FUNC(a, init_func, oplist, tree->tab[i].data,          \
                        exp_emplace_type);                                    \
        M_F(name, _it_ct) it;                                                 \
        it.tree = tree;                                                       \
        it.index = tree->root_index;                                          \
        M_TR33_CONTRACT(tree);                                                \
        return it;                                                            \
    }

#define M_TR33_EMPLACE_UP_DEF(name, name_t, function_name, oplist, init_func, exp_emplace_type) \
  M_P(name_t, name, function_name, name_t pos M_EMPLACE_LIST_TYPE_VAR(a, exp_emplace_type) ) \
  {                                                                           \
    name_t it = M_F(name, _insert_up_raw)M_R(pos);                            \
    M_EMPLACE_CALL_FUNC(a, init_func, oplist, it.tree->tab[it.index].data,    \
                        exp_emplace_type);                                    \
    return it;                                                                \
  }

#define M_TR33_EMPLACE_DOWN_DEF(name, name_t, function_name, oplist, init_func, exp_emplace_type) \
  M_P(name_t, name, function_name, name_t pos M_EMPLACE_LIST_TYPE_VAR(a, exp_emplace_type) ) \
  {                                                                           \
    name_t it = M_F(name, _insert_down_raw)M_R(pos);                          \
    M_EMPLACE_CALL_FUNC(a, init_func, oplist, it.tree->tab[it.index].data,    \
                        exp_emplace_type);                                    \
    return it;                                                                \
  }

#define M_TR33_EMPLACE_CHILD_DEF(name, name_t, function_name, oplist, init_func, exp_emplace_type) \
  M_P(name_t, name, function_name, name_t pos M_EMPLACE_LIST_TYPE_VAR(a, exp_emplace_type) ) \
  {                                                                           \
    name_t it = M_F(name, _insert_child_raw)M_R(pos);                         \
    M_EMPLACE_CALL_FUNC(a, init_func, oplist, it.tree->tab[it.index].data,    \
                        exp_emplace_type);                                    \
    return it;                                                                \
  }

#define M_TR33_EMPLACE_LEFT_DEF(name, name_t, function_name, oplist, init_func, exp_emplace_type) \
  M_P(name_t, name, function_name, name_t pos M_EMPLACE_LIST_TYPE_VAR(a, exp_emplace_type) ) \
  {                                                                           \
    name_t it = M_F(name, _insert_left_raw)M_R(pos);                          \
    M_EMPLACE_CALL_FUNC(a, init_func, oplist, it.tree->tab[it.index].data,    \
                        exp_emplace_type);                                    \
    return it;                                                                \
  }

#define M_TR33_EMPLACE_RIGHT_DEF(name, name_t, function_name, oplist, init_func, exp_emplace_type) \
  M_P(name_t, name, function_name, name_t pos M_EMPLACE_LIST_TYPE_VAR(a, exp_emplace_type) ) \
  {                                                                           \
    name_t it = M_F(name, _insert_right_raw)M_R(pos);                         \
    M_EMPLACE_CALL_FUNC(a, init_func, oplist, it.tree->tab[it.index].data,    \
                        exp_emplace_type);                                    \
    return it;                                                                \
  }

// TODO: 
// * serialization
// * OPLIST ?
// * _previous ?

/********************************** INTERNAL *********************************/

#if M_USE_SMALL_NAME
#define TREE_DEF M_TREE_DEF
#define TREE_DEF_AS M_TREE_DEF_AS
#define TREE_OPLIST M_TREE_OPLIST
#endif

#endif
