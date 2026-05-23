/*
 * M*LIB - Intrusive List module
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
#ifndef MSTARLIB_I_LIST_H
#define MSTARLIB_I_LIST_H

#include "m-core.h"
#include "m-list.h" // For M_L1ST_ITBASE_DEF

/* Interface to add to a structure to enable intrusive doubly-linked support.
   name: name of the intrusive list.
   type: name of the type of the structure (aka. struct test_s) - not used currently
   USAGE:
     typedef struct tmp_str_s {
      ...
      ILIST_INTERFACE(tmpstr, struct tmp_str_s);
      ...
     } tmp_str_t;
*/
#define M_ILIST_INTERFACE(name, type)                                         \
  struct m_il1st_head_s name


/* Define a doubly-linked intrusive list of a given type.
   The type needs to have ILIST_INTERFACE().
   USAGE:
     ILIST_DEF(name, type [, oplist_of_the_type]) */
#define M_ILIST_DEF(name, ...)                                                \
  M_ILIST_DEF_AS(name, M_F(name,_t), M_F(name,_it_t), __VA_ARGS__)


/* Define a doubly-linked intrusive list of a given type
   as the provided type name_t with the iterator named it_t.
   The type needs to have ILIST_INTERFACE().
   USAGE:
     ILIST_DEF_AS(name, name_t, it_t, type [, oplist_of_the_type]) */
#define M_ILIST_DEF_AS(name, name_t, it_t, ...)                               \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_IL1ST_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                                  \
                ((name, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), name_t, it_t ), \
                 (name, __VA_ARGS__,                                        name_t, it_t ))) \
  M_END_PROTECTED_CODE


/* Define the oplist of a doubly-linked intrusive list of type.
   USAGE:
     ILIST_OPLIST(name [, oplist_of_the_type]) */
#define M_ILIST_OPLIST(...)                                                   \
  M_IL1ST_OPLIST_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                               \
                ((__VA_ARGS__, M_BASIC_OPLIST),                               \
                 (__VA_ARGS__ )))

#define M_ILIST_INIT_FIELD(name, obj) do {                                    \
    (obj).name.next = NULL;                                                   \
    (obj).name.prev = NULL;                                                   \
  } while(0)

/*****************************************************************************/
/******************************** INTERNAL ***********************************/
/*****************************************************************************/

/* Define the basic structure to be added in all objects. */
typedef struct m_il1st_head_s {
  struct m_il1st_head_s *next;
  struct m_il1st_head_s *prev;
} m_il1st_head_ct;

/* Indirection call to allow expanding all arguments */
#define M_IL1ST_OPLIST_P1(arg) M_IL1ST_OPLIST_P2 arg

/* Validation of the given oplist */
#define M_IL1ST_OPLIST_P2(name, oplist)                                       \
  M_IF_OPLIST(oplist)(M_IL1ST_OPLIST_P3, M_IL1ST_OPLIST_FAILURE)(name, oplist)

/* Prepare a clean compilation failure */
#define M_IL1ST_OPLIST_FAILURE(name, oplist)                                  \
  ((M_LIB_ERROR(ARGUMENT_OF_ILIST_OPLIST_IS_NOT_AN_OPLIST, name, oplist)))

/* Define the oplist of an ilist of type */
#define M_IL1ST_OPLIST_P3(name, oplist)                                       \
  (INIT(M_F(name, _init)),                                                    \
   CLEAR(M_F(name, _clear)),                                                  \
   INIT_MOVE(M_F(name, _init_move)),                                          \
   MOVE(M_F(name, _move)),                                                    \
   NAME(name),                                                                \
   TYPE(M_F(name,_ct)), GENTYPE(struct M_F(name,_s)*),                        \
   RESET(M_F(name,_reset)),                                                   \
   SUBTYPE(M_F(name,_subtype_ct)),                                            \
   EMPTY_P(M_F(name,_empty_p)),                                               \
   IT_TYPE(M_F(name,_it_ct)),                                                 \
   IT_FIRST(M_F(name,_it)),                                                   \
   IT_SET(M_F(name,_it_set)),                                                 \
   IT_LAST(M_F(name,_it_last)),                                               \
   IT_END(M_F(name,_it_end)),                                                 \
   IT_END_P(M_F(name,_end_p)),                                                \
   IT_LAST_P(M_F(name,_last_p)),                                              \
   IT_EQUAL_P(M_F(name,_it_equal_p)),                                         \
   IT_NEXT(M_F(name,_next)),                                                  \
   IT_PREVIOUS(M_F(name,_previous)),                                          \
   IT_REF(M_F(name,_ref)),                                                    \
   IT_CREF(M_F(name,_cref)),                                                  \
   IT_REMOVE(M_F(name,_remove)),                                              \
   OPLIST(oplist),                                                            \
   SPLICE_BACK(M_F(name,_splice_back))                                        \
   )


/******************************** INTERNAL ***********************************/

/* Contract respected by all intrusive lists */
#define M_IL1ST_CONTRACT(name, list) do {                                     \
    M_ASSERT(list != NULL);                                                   \
    M_ASSERT(list->name.prev != NULL);                                        \
    M_ASSERT(list->name.next != NULL);                                        \
    M_ASSERT(list->name.next->prev == &list->name);                           \
    M_ASSERT(list->name.prev->next == &list->name);                           \
    M_ASSERT(!(list->name.prev == &list->name) || list->name.prev == list->name.next); \
  } while (0)

#define M_IL1ST_NODE_CONTRACT(node) do {                                      \
    M_ASSERT((node) != NULL);                                                 \
    M_ASSERT((node)->prev != NULL);                                           \
    M_ASSERT((node)->next != NULL);                                           \
    M_ASSERT((node)->next->prev == node);                                     \
    M_ASSERT((node)->prev->next == node);                                     \
  } while (0)

/* Indirection call to allow expanding all arguments */
#define M_IL1ST_DEF_P1(arg) M_ID( M_IL1ST_DEF_P2 arg )

/* Validate the oplist before going further */
#define M_IL1ST_DEF_P2(name, type, oplist, list_t, it_t)                      \
  M_IF_OPLIST(oplist)(M_IL1ST_DEF_P3, M_IL1ST_DEF_FAILURE)(name, type, oplist, list_t, it_t)

/* Stop processing with a compilation failure */
#define M_IL1ST_DEF_FAILURE(name, type, oplist, list_t, it_t)                 \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST, "(ILIST_DEF): the given argument is not a valid oplist: " #oplist)

/* Definition of the type and function for an intrusive doubly-linked list.
   USAGE:
    name: name of the intrusive list
    type: type of the object
    oplist: oplist of the type
    list_t: type of the intrusive list (name##_t)
    it_t: iterator of the intrusive list (name##_it_t)
*/
#define M_IL1ST_DEF_P3(name, type, oplist, list_t, it_t)                      \
  M_IL1ST_DEF_TYPE(name, type, oplist, list_t, it_t)                          \
  M_CHECK_COMPATIBLE_OPLIST(name, 1, type, oplist)                            \
  M_IL1ST_DEF_CORE(name, type, oplist, list_t, it_t)                          \
  /* Used of internal macro from m-list */                                    \
  M_L1ST_ITBASE_DEF(name, type, oplist, list_t, it_t)

/* Define the type of an intrusive list */
#define M_IL1ST_DEF_TYPE(name, type, oplist, list_t, it_t)                    \
                                                                              \
  /* Define the list as a structure containing pointers                       \
   * to the front & back nodes */                                             \
  typedef struct M_F(name, _s) {                                              \
    struct m_il1st_head_s name;                                               \
  } list_t[1];                                                                \
                                                                              \
  /* Define internal types pointers to such a list */                         \
  typedef struct M_F(name, _s) *M_F(name, _ptr);                              \
  typedef const struct M_F(name, _s) *M_F(name, _srcptr);                     \
                                                                              \
  /* Define iterator of such a list */                                        \
  typedef struct M_F(name, _it_s) {                                           \
    struct m_il1st_head_s *head;                                              \
    struct m_il1st_head_s *previous;                                          \
    struct m_il1st_head_s *current;                                           \
    struct m_il1st_head_s *next;                                              \
  } it_t[1];                                                                  \
                                                                              \
  /* Define types used by oplist */                                           \
  typedef type   M_F(name, _subtype_ct);                                      \
  typedef list_t M_F(name, _ct);                                              \
  typedef it_t   M_F(name, _it_ct);                                           \

/* Define core functions for intrusive lists */
#define M_IL1ST_DEF_CORE(name, type, oplist, list_t, it_t)                    \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _init)(list_t list)                                               \
  {                                                                           \
    M_ASSERT (list != NULL);                                                  \
    list->name.next = &list->name;                                            \
    list->name.prev = &list->name;                                            \
    M_IL1ST_CONTRACT(name, list);                                             \
  }                                                                           \
                                                                              \
  M_P(void, name, _reset, list_t list)                                        \
  {                                                                           \
    M_IL1ST_CONTRACT(name, list);                                             \
    for(struct m_il1st_head_s *it = list->name.next, *next ;                  \
        it != &list->name; it = next) {                                       \
      /* Cannot check node contract as previous node may be deleted */        \
      type *obj = M_TYPE_FROM_FIELD(type, it,                                 \
                                    struct m_il1st_head_s, name);             \
      /* Read next now before the object is destroyed */                      \
      next = it->next;                                                        \
      M_ASSERT (next != NULL);                                                \
      M_CALL_CLEAR(oplist, *obj);                                             \
      /* Delete also the object if a DELETE operand is registered */          \
      M_IF_METHOD(DEL, oplist)(M_CALL_DEL(oplist, obj), (void) 0);            \
    }                                                                         \
    /* Nothing remains in the list anymore */                                 \
    list->name.next = &list->name;                                            \
    list->name.prev = &list->name;                                            \
    M_IL1ST_CONTRACT(name, list);                                             \
  }                                                                           \
                                                                              \
  M_P(void, name, _clear, list_t list)                                        \
  {                                                                           \
    /* Nothing to do more than clean the list itself */                       \
    M_F(name, _reset)M_R(list);                                               \
    /* For safety purpose (create invalid representation of object) */        \
    list->name.next = NULL;                                                   \
    list->name.prev = NULL;                                                   \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _empty_p)(const list_t list)                                      \
  {                                                                           \
    M_IL1ST_CONTRACT(name, list);                                             \
    return list->name.next == &list->name;                                    \
  }                                                                           \
                                                                              \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _init_move)(list_t list, list_t ref)                              \
  {                                                                           \
    M_IL1ST_CONTRACT(name, ref);                                              \
    M_ASSERT (list != ref);                                                   \
    M_F(name,_init)(list);                                                    \
    if (!M_F(name,_empty_p)(ref)) {                                           \
      list->name.next = ref->name.next;                                       \
      list->name.prev = ref->name.prev;                                       \
      list->name.next->prev = &list->name;                                    \
      list->name.prev->next = &list->name;                                    \
    }                                                                         \
    ref->name.next = NULL;                                                    \
    ref->name.prev = NULL;                                                    \
    M_IL1ST_CONTRACT(name, list);                                             \
  }                                                                           \
                                                                              \
  M_P(void, name, _move, list_t list, list_t ref)                             \
  {                                                                           \
    M_F(name, _clear)M_R(list);                                               \
    M_F(name, _init_move)(list, ref);                                         \
  }                                                                           \
                                                                              \
  M_INLINE size_t                                                             \
  M_F(name, _size)(const list_t list)                                         \
  {                                                                           \
    M_IL1ST_CONTRACT(name, list);                                             \
    size_t s = 0;                                                             \
    /* Scan the full list to count the number of elements */                  \
    for(const struct m_il1st_head_s *it = list->name.next ;                   \
        it != &list->name; it = it->next) {                                   \
      M_IL1ST_NODE_CONTRACT(it);                                              \
      s++;                                                                    \
    }                                                                         \
    return s;                                                                 \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _push_back)(list_t list, type *obj)                               \
  {                                                                           \
    M_IL1ST_CONTRACT(name, list);                                             \
    M_ASSERT (obj != NULL);                                                   \
    struct m_il1st_head_s *prev = list->name.prev;                            \
    list->name.prev = &obj->name;                                             \
    obj->name.prev = prev;                                                    \
    obj->name.next = &list->name;                                             \
    prev->next = &obj->name;                                                  \
    M_IL1ST_CONTRACT(name, list);                                             \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _push_front)(list_t list, type *obj)                              \
  {                                                                           \
    M_IL1ST_CONTRACT(name, list);                                             \
    M_ASSERT (obj != NULL);                                                   \
    struct m_il1st_head_s *next = list->name.next;                            \
    list->name.next = &obj->name;                                             \
    obj->name.next = next;                                                    \
    obj->name.prev = &list->name;                                             \
    next->prev = &obj->name;                                                  \
    M_IL1ST_CONTRACT(name, list);                                             \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _push_after)(type *obj_pos, type *obj)                            \
  {                                                                           \
    M_ASSERT (obj_pos != NULL && obj != NULL);                                \
    /* We don't have the list, so we have no contract at list level */        \
    M_IL1ST_NODE_CONTRACT(&obj_pos->name);                                    \
    struct m_il1st_head_s *next = obj_pos->name.next;                         \
    obj_pos->name.next = &obj->name;                                          \
    obj->name.next = next;                                                    \
    obj->name.prev = &obj_pos->name;                                          \
    next->prev = &obj->name;                                                  \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _init_field)(type *obj)                                           \
  {                                                                           \
    M_ASSERT (obj != NULL);                                                   \
    /* Init the fields of the node. To be used in object constructor */       \
    obj->name.next = NULL;                                                    \
    obj->name.prev = NULL;                                                    \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _unlink)(type *obj)                                               \
  {                                                                           \
    M_ASSERT (obj != NULL);                                                   \
    /* We don't have the list, so we have no contract at list level */        \
    M_IL1ST_NODE_CONTRACT(&obj->name);                                        \
    struct m_il1st_head_s *next = obj->name.next;                             \
    struct m_il1st_head_s *prev = obj->name.prev;                             \
    next->prev = prev;                                                        \
    prev->next = next;                                                        \
    /* Note: not really needed, but safer */                                  \
    obj->name.next = NULL;                                                    \
    obj->name.prev = NULL;                                                    \
  }                                                                           \
                                                                              \
  M_INLINE type *                                                             \
  M_F(name, _back)(const list_t list)                                         \
  {                                                                           \
    M_IL1ST_CONTRACT(name, list);                                             \
    M_ASSERT(!M_F(name, _empty_p)(list));                                     \
    return M_TYPE_FROM_FIELD(type, list->name.prev,                           \
                             struct m_il1st_head_s, name);                    \
  }                                                                           \
                                                                              \
  M_INLINE type *                                                             \
  M_F(name, _front)(const list_t list)                                        \
  {                                                                           \
    M_IL1ST_CONTRACT(name, list);                                             \
    M_ASSERT(!M_F(name, _empty_p)(list));                                     \
    return M_TYPE_FROM_FIELD(type, list->name.next,                           \
                             struct m_il1st_head_s, name);                    \
  }                                                                           \
                                                                              \
  M_INLINE type *                                                             \
  M_F(name, _next_obj)(const list_t list, type const *obj)                    \
  {                                                                           \
    M_IL1ST_CONTRACT(name, list);                                             \
    M_ASSERT (obj != NULL);                                                   \
    M_IL1ST_NODE_CONTRACT(&obj->name);                                        \
    return obj->name.next == &list->name ? NULL :                             \
      M_TYPE_FROM_FIELD(type, obj->name.next,                                 \
                        struct m_il1st_head_s, name);                         \
  }                                                                           \
                                                                              \
  M_INLINE type *                                                             \
  M_F(name, _previous_obj)(const list_t list, type const *obj)                \
  {                                                                           \
    M_IL1ST_CONTRACT(name, list);                                             \
    M_ASSERT (obj != NULL);                                                   \
    M_IL1ST_NODE_CONTRACT(&obj->name);                                        \
    return obj->name.prev == &list->name ? NULL :                             \
      M_TYPE_FROM_FIELD(type, obj->name.prev,                                 \
                        struct m_il1st_head_s, name);                         \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it)(it_t it, const list_t list)                                  \
  {                                                                           \
    M_IL1ST_CONTRACT(name, list);                                             \
    M_ASSERT (it != NULL);                                                    \
    it->head = list->name.next->prev;                                         \
    it->current = list->name.next;                                            \
    it->next = list->name.next->next;                                         \
    it->previous = it->head;                                                  \
    M_IL1ST_NODE_CONTRACT(it->current);                                       \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_set)(it_t it, const it_t cit)                                 \
  {                                                                           \
    M_ASSERT (it != NULL && cit != NULL);                                     \
    it->head = cit->head;                                                     \
    it->current = cit->current;                                               \
    it->next = cit->next;                                                     \
    it->previous = cit->previous;                                             \
    M_IL1ST_NODE_CONTRACT(it->current);                                       \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_last)(it_t it, list_t const list)                             \
  {                                                                           \
    M_IL1ST_CONTRACT(name, list);                                             \
    M_ASSERT (it != NULL);                                                    \
    it->head = list->name.next->prev;                                         \
    it->current = list->name.prev;                                            \
    it->next = it->head;                                                      \
    it->previous = list->name.prev->prev;                                     \
    M_IL1ST_NODE_CONTRACT(it->current);                                       \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_end)(it_t it, list_t const list)                              \
  {                                                                           \
    M_ASSERT (it != NULL && list != NULL);                                    \
    it->head = list->name.next->prev;                                         \
    it->current = it->head;                                                   \
    it->next = list->name.next;                                               \
    it->previous = list->name.prev;                                           \
    M_IL1ST_NODE_CONTRACT(it->current);                                       \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _end_p)(const it_t it)                                            \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    M_IL1ST_NODE_CONTRACT(it->current);                                       \
    return it->current == it->head;                                           \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _last_p)(const it_t it)                                           \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    M_IL1ST_NODE_CONTRACT(it->current);                                       \
    return it->next == it->head || it->current == it->head;                   \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _next)(it_t it)                                                   \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    /* Cannot check node for it->current: it may have been deleted! */        \
    /* Note: Can't set it->previous to it->current.                           \
       it->current may have been unlinked from the list */                    \
    it->current  = it->next;                                                  \
    M_ASSERT (it->current != NULL);                                           \
    it->next     = it->current->next;                                         \
    it->previous = it->current->prev;                                         \
    M_ASSERT (it->next != NULL && it->previous != NULL);                      \
    M_IL1ST_NODE_CONTRACT(it->current);                                       \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _previous)(it_t it)                                               \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    /* Cannot check node for it->current: it may have been deleted! */        \
    /* Note: Can't set it->next to it->current.                               \
       it->current may have been unlinked from the list */                    \
    it->current  = it->previous;                                              \
    M_ASSERT (it->current != NULL);                                           \
    it->next     = it->current->next;                                         \
    it->previous = it->current->prev;                                         \
    M_ASSERT (it->next != NULL && it->previous != NULL);                      \
    M_IL1ST_NODE_CONTRACT(it->current);                                       \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _it_equal_p)(const it_t it1, const it_t it2 )                     \
  {                                                                           \
    M_ASSERT (it1 != NULL && it2 != NULL);                                    \
    /* No need to check for next & previous */                                \
    return it1->head == it2->head && it1->current == it2->current;            \
  }                                                                           \
                                                                              \
  M_INLINE type *                                                             \
  M_F(name, _ref)(const it_t it)                                              \
  {                                                                           \
    M_ASSERT (it != NULL && it->current != NULL);                             \
    M_IL1ST_NODE_CONTRACT(it->current);                                       \
    /* check if 'it' was not deleted */                                       \
    M_ASSERT (it->current->next == it->next);                                 \
    M_ASSERT (it->current->prev == it->previous);                             \
    M_ASSERT (!M_F(name, _end_p)(it));                                        \
    return M_TYPE_FROM_FIELD(type, it->current,                               \
                             struct m_il1st_head_s, name);                    \
  }                                                                           \
                                                                              \
  M_INLINE type const *                                                       \
  M_F(name, _cref)(const it_t it)                                             \
  {                                                                           \
    type *ptr = M_F(name, _ref)(it);                                          \
    return M_CONST_CAST(type, ptr);                                           \
  }                                                                           \
                                                                              \
  M_P(void, name, _remove, list_t list, it_t it)                              \
  {                                                                           \
    M_IL1ST_CONTRACT(name, list);                                             \
    M_IL1ST_NODE_CONTRACT(it->current);                                       \
    (void)list;        /* list param is not used */                           \
    type *obj = M_TYPE_FROM_FIELD(type, it->current,                          \
                                  struct m_il1st_head_s, name);               \
    M_F(name, _unlink)(obj);                                                  \
    M_CALL_CLEAR(oplist, *obj);                                               \
    M_IF_METHOD(DEL, oplist)(M_CALL_DEL(oplist, obj), (void) 0);              \
    M_F(name, _next)(it);                                                     \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _insert)(list_t list, it_t it, type *p)                           \
  {                                                                           \
    M_IL1ST_CONTRACT(name, list);                                             \
    M_IL1ST_NODE_CONTRACT(it->current);                                       \
    type *obj = M_F(name, _ref)(it);                                          \
    M_F(name, _push_after)(obj, p);                                           \
    it->current = &p->name;                                                   \
    (void) list;                                                              \
    M_IL1ST_CONTRACT(name, list);                                             \
  }                                                                           \
                                                                              \
  M_INLINE type *                                                             \
  M_F(name, _pop_back)(list_t list)                                           \
  {                                                                           \
    M_IL1ST_CONTRACT(name, list);                                             \
    M_ASSERT (!M_F(name, _empty_p)(list));                                    \
    type *obj = M_F(name, _back)(list);                                       \
    list->name.prev = list->name.prev->prev;                                  \
    list->name.prev->next = &list->name;                                      \
    return obj;                                                               \
  }                                                                           \
                                                                              \
  M_INLINE type *                                                             \
  M_F(name, _pop_front)(list_t list)                                          \
  {                                                                           \
    M_IL1ST_CONTRACT(name, list);                                             \
    M_ASSERT (!M_F(name, _empty_p)(list));                                    \
    type *obj = M_F(name, _front)(list);                                      \
    list->name.next = list->name.next->next;                                  \
    list->name.next->prev = &list->name;                                      \
    return obj;                                                               \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _splice)(list_t list1, list_t list2)                              \
  {                                                                           \
    M_IL1ST_CONTRACT(name, list1);                                            \
    M_IL1ST_CONTRACT(name, list2);                                            \
    struct m_il1st_head_s *midle1 = list1->name.prev;                         \
    struct m_il1st_head_s *midle2 = list2->name.next;                         \
    midle1->next = midle2;                                                    \
    midle2->prev = midle1;                                                    \
    list1->name.prev = list2->name.prev;                                      \
    list2->name.prev->next = &list1->name;                                    \
    list2->name.next = &list2->name;                                          \
    list2->name.prev = &list2->name;                                          \
    M_IL1ST_CONTRACT(name, list1);                                            \
    M_IL1ST_CONTRACT(name, list2);                                            \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _splice_back)(list_t nv, list_t ov, it_t it)                      \
  {                                                                           \
    M_IL1ST_CONTRACT(name, nv);                                               \
    M_IL1ST_CONTRACT(name, ov);                                               \
    M_IL1ST_NODE_CONTRACT(it->current);                                       \
    M_ASSERT (it != NULL);                                                    \
    (void) ov;                                                                \
    type *obj = M_F(name, _ref)(it);                                          \
    M_F(name, _unlink)(obj);                                                  \
    M_F(name, _push_back)(nv, obj);                                           \
    M_F(name, _next)(it);                                                     \
    M_IL1ST_CONTRACT(name, nv);                                               \
    M_IL1ST_CONTRACT(name, ov);                                               \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _splice_at)(list_t nlist, it_t npos,                              \
                        list_t olist, it_t opos)                              \
  {                                                                           \
    M_IL1ST_CONTRACT(name, nlist);                                            \
    M_IL1ST_CONTRACT(name, olist);                                            \
    M_ASSERT (npos != NULL && opos != NULL);                                  \
    M_ASSERT (!M_F(name, _end_p)(opos));                                      \
    /* npos may be end */                                                     \
    (void) olist, (void) nlist;                                               \
    type *obj = M_F(name, _ref)(opos);                                        \
    struct m_il1st_head_s *ref = npos->current;                               \
    /* Remove object */                                                       \
    M_F(name, _unlink)(obj);                                                  \
    /* Push 'obj' after 'ref' */                                              \
    struct m_il1st_head_s *next = ref->next;                                  \
    ref->next = &obj->name;                                                   \
    obj->name.next = next;                                                    \
    obj->name.prev = ref;                                                     \
    next->prev = &obj->name;                                                  \
    /* Move iterator in old list */                                           \
    M_F(name, _next)(opos);                                                   \
    /* Set npos iterator to new position of object */                         \
    npos->previous = ref;                                                     \
    npos->current = &obj->name;                                               \
    npos->next = next;                                                        \
    M_IL1ST_CONTRACT(name, nlist);                                            \
    M_IL1ST_CONTRACT(name, olist);                                            \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _swap)(list_t d, list_t e)                                        \
  {                                                                           \
    M_IL1ST_CONTRACT(name, d);                                                \
    M_IL1ST_CONTRACT(name, e);                                                \
    struct m_il1st_head_s *d_item = d->name.next;                             \
    struct m_il1st_head_s *e_item = e->name.next;                             \
    /* it is more complicated than other swap functions since                 \
       we need to detect "cyclic" loop */                                     \
    d->name.next = e_item == &e->name ? &d->name : e_item;                    \
    e->name.next = d_item == &d->name ? &e->name : d_item;                    \
    d_item = d->name.prev;                                                    \
    e_item = e->name.prev;                                                    \
    d->name.prev = e_item == &e->name ? &d->name : e_item;                    \
    e->name.prev = d_item == &d->name ? &e->name : d_item;                    \
    d->name.next->prev = &d->name;                                            \
    d->name.prev->next = &d->name;                                            \
    e->name.next->prev = &e->name;                                            \
    e->name.prev->next = &e->name;                                            \
    M_IL1ST_CONTRACT(name, d);                                                \
    M_IL1ST_CONTRACT(name, e);                                                \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _reverse)(list_t list)                                            \
  {                                                                           \
    M_IL1ST_CONTRACT(name, list);                                             \
    struct m_il1st_head_s *next, *it;                                         \
    for(it = list->name.next ; it != &list->name; it = next) {                \
      next = it->next;                                                        \
      it->next = it->prev;                                                    \
      it->prev = next;                                                        \
    }                                                                         \
    next = it->next;                                                          \
    it->next = it->prev;                                                      \
    it->prev = next;                                                          \
    M_IL1ST_CONTRACT(name, list);                                             \
  }                                                                           \


/******************************** INTERNAL ***********************************/

#if M_USE_SMALL_NAME
#define ILIST_INTERFACE M_ILIST_INTERFACE
#define ILIST_DEF M_ILIST_DEF
#define ILIST_DEF_AS M_ILIST_DEF_AS
#define ILIST_OPLIST M_ILIST_OPLIST
#endif

#endif
