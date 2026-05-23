/*
 * M*LIB - DICTIONARY Module
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
#ifndef MSTARLIB_DICT_H
#define MSTARLIB_DICT_H

#include "m-array.h"

/* Define a dictionary associating the key key_type to the value value_type and its associated functions.
   USAGE:
     DICT_DEF2(name, key_type, key_oplist, value_type, value_oplist)
   OR
     DICT_DEF2(name, key_type, value_type)
*/
#define M_DICT_DEF2(name, key_type, ...)                                      \
  M_DICT_DEF2_AS(name, M_F(name,_t), M_F(name,_it_t), M_F(name,_itref_t), key_type, __VA_ARGS__)


/* Define a dictionary associating the key key_type to the value value_type and its associated functions.
   as the given name name_t with its associated functions.
   USAGE:
     DICT_DEF2_AS(name, name_t, it_t, itref_t, key_type, key_oplist, value_type, value_oplist)
   OR
     DICT_DEF2_AS(name, name_t, it_t, itref_t, key_type, value_type)
*/
#define M_DICT_DEF2_AS(name, name_t, it_t, itref_t, key_type, ...)            \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_D1CT_DEF2_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                                  \
                ((name, key_type, M_GLOBAL_OPLIST_OR_DEF(key_type)(), __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), name_t, it_t, itref_t ), \
                 (name, key_type, __VA_ARGS__, name_t, it_t, itref_t ) ))     \
  M_END_PROTECTED_CODE


/* Define a dictionary associating the key key_type to the value value_type
   with an Open Addressing implementation and its associated functions.
   KEY_OPLIST needs the operators OOR_EQUAL & OOR_SET.
   USAGE:
     DICT_OA_DEF2(name, key_type, key_oplist, value_type, value_oplist)
   OR
     DICT_OA_DEF2(name, key_type, value_type)
*/
#define M_DICT_OA_DEF2(name, key_type, ...)                                   \
  M_DICT_OA_DEF2_AS(name, M_F(name,_t), M_F(name,_it_t), M_F(name,_itref_t), key_type, __VA_ARGS__)


/* Define a dictionary associating the key key_type to the value value_type
   with an Open Addressing implementation and its associated functions.
   as the given name name_t with its associated functions.
   KEY_OPLIST needs the operators OOR_EQUAL & OOR_SET.
   USAGE:
     DICT_OA_DEF2_AS(name, name_t, it_t, itref_t, key_type, key_oplist, value_type, value_oplist)
   OR
     DICT_OA_DEF2_AS(name, name_t, it_t, itref_t, key_type, value_type)
*/
#define M_DICT_OA_DEF2_AS(name, name_t, it_t, itref_t, key_type, ...)         \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_D1CT_OA_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                                \
                  ((name, key_type, M_GLOBAL_OPLIST_OR_DEF(key_type)(), __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), name_t, it_t, itref_t ), \
                   (name, key_type, __VA_ARGS__, name_t, it_t, itref_t )))    \
  M_END_PROTECTED_CODE


/* Define a set of the key key_type and its associated functions.
   The set is unordered.
   USAGE: DICT_SET_DEF(name, key_type[, key_oplist])
*/
#define M_DICT_SET_DEF(name, ...)                                             \
  M_DICT_SET_DEF_AS(name, M_F(name,_t), M_F(name,_it_t), __VA_ARGS__)


/* Define a set of the key key_type and its associated functions.
   as the given name name_t with its associated functions.
   The set is unordered.
   USAGE: DICT_SET_DEF_AS(name, name_t, it_t, key_type[, key_oplist])
*/
#define M_DICT_SET_DEF_AS(name, name_t, it_t,  ...)                           \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_D1CT_SET_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                               \
                   ((name, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), name_t, it_t, M_F(name, _itref_ct) ), \
                    (name, __VA_ARGS__, name_t, it_t, M_F(name, _itref_ct)  ))) \
  M_END_PROTECTED_CODE


/* Define a set of the key key_type 
   with an Open Addressing implementation and its associated functions.
   The set is unordered.
   USAGE: DICT_OASET_DEF(name, key_type[, key_oplist])
*/
#define M_DICT_OASET_DEF(name, ...)                                           \
  M_DICT_OASET_DEF_AS(name, M_F(name,_t), M_F(name,_it_t), __VA_ARGS__)


/* Define a set of the key key_type 
   with an Open Addressing implementation and its associated functions.
   as the given name name_t with its associated functions.
   The set is unordered.
   USAGE: DICT_OASET_DEF_AS(name, name_t, it_t, key_type[, key_oplist])
*/
#define M_DICT_OASET_DEF_AS(name, name_t, it_t, ...)                          \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_D1CT_OASET_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                             \
                     ((name, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), name_t, it_t, M_F(name, _itref_ct) ), \
                      (name, __VA_ARGS__, name_t, it_t, M_F(name, _itref_ct) ))) \
  M_END_PROTECTED_CODE


/* Define the oplist of a dictionary (DICT_DEF2 or DICT_OA_DEF2).
   USAGE:
     DICT_OPLIST(name, oplist of the key type, oplist of the value type)
   OR
     DICT_OPLIST(name)
*/
#define M_DICT_OPLIST(...)                                                    \
  M_IF_NARGS_EQ1(__VA_ARGS__)                                                 \
  (M_D1CT_OPLIST_P1((__VA_ARGS__, M_BASIC_OPLIST, M_BASIC_OPLIST )),          \
   M_D1CT_OPLIST_P1((__VA_ARGS__ )))


/* Define the oplist of a dictionary (DICT_SET_DEF).
   USAGE: DICT_SET_OPLIST(name[, oplist of the key type]) */
#define M_DICT_SET_OPLIST(...)                                                \
  M_IF_NARGS_EQ1(__VA_ARGS__)                                                 \
  (M_D1CT_SET_OPLIST_P1((__VA_ARGS__, M_BASIC_OPLIST)),                       \
   M_D1CT_SET_OPLIST_P1((__VA_ARGS__ )))


/*****************************************************************************/
/******************************** INTERNAL ***********************************/
/*****************************************************************************/

// Define the maximum index type (implying the maximum allowed size)
#ifdef M_USE_DICT_LARGE_INDEX
typedef uint64_t m_index_t;
#else
typedef uint32_t m_index_t;
#endif

// Define a pair index of the cell where the (key,value) is stored / hash of the key.
typedef struct {
  m_index_t index;
  m_index_t hash;
} m_indexhash_t;

// Define a basic array over such pair
M_ARRAY_DEF(m_array_index, m_indexhash_t, M_POD_OPLIST)

/* Performing Quadratic probing
   Replace it by '1' to perform linear probing */
#ifdef M_USE_DICT_OA_PROBING
# define M_D1CT_OA_PROBING M_USE_DICT_OA_PROBING
#else
# define M_D1CT_OA_PROBING(s) ((s)++)
#endif

/* Lower Bound of the hash table
   We shall have 2*M_D1CT_OA_LOWER_BOUND < M_D1CT_OA_UPPER_BOUND
 */
#ifndef M_D1CT_OA_LOWER_BOUND
#define M_D1CT_OA_LOWER_BOUND 0.2
#endif

/* Upper Bound of the hash table 
   The container is designed for an upper bound in the range [0.5-0.75]
   Using higher upper bound (even by a slow amount) will deteriorate a lot the performance:
   an upper-bound of 0.875 is ~3 times slower than an upper bound of 0.7 for lockup of key not present
   (without considering prefetch of cache-lines which improve performance).
   This is due to the esperance of the number of reads before reading an empty value which is:
   (1-q)*sum(q^k*(k+1),k=0 .. infinity) = 1/(1-q)
   which is the estimated number of iteration before stopping the search loop.
 */
#ifndef M_D1CT_OA_UPPER_BOUND
#define M_D1CT_OA_UPPER_BOUND 0.7
#endif

/* Define initial size of the hash table */
#ifndef M_D1CT_INITIAL_SIZE
#define M_D1CT_INITIAL_SIZE   16
#endif

/* Define a dictionary from the key key_type to the value value_type.
   It is defined as an array of singly linked list (each list
   representing a bucket of items with the same hash value modulo the
   current array size).
*/
/* Deferred evaluation for arg */
#define M_D1CT_DEF2_P1(arg) M_ID( M_D1CT_DEF2_P2 arg )

/* Validate the key oplist before going further */
#define M_D1CT_DEF2_P2(name, key_type, key_oplist, value_type, value_oplist, dict_t, dict_it_t, it_deref_t) \
  M_IF_OPLIST(key_oplist)(M_D1CT_DEF2_P3, M_D1CT_DEF2_FAILURE)(name, key_type, key_oplist, value_type, value_oplist, dict_t, dict_it_t, it_deref_t)

/* Validate the value oplist before going further */
#define M_D1CT_DEF2_P3(name, key_type, key_oplist, value_type, value_oplist, dict_t, dict_it_t, it_deref_t) \
  M_IF_OPLIST(value_oplist)(M_D1CT_DEF2_P4, M_D1CT_DEF2_FAILURE)(name, key_type, key_oplist, value_type, value_oplist, dict_t, dict_it_t, it_deref_t)

/* Stop processing with a compilation failure */
#define M_D1CT_DEF2_FAILURE(name, key_type, key_oplist, value_type, value_oplist, dict_t, dict_it_t, it_deref_t) \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST, "(DICT_DEF2): at least one of the given argument is not a valid oplist: " M_AS_STR(key_oplist) " / " M_AS_STR(value_oplist) )

#define M_D1CT_DEF2_P4(name, key_type, key_oplist, value_type, value_oplist, dict_t, dict_it_t, it_deref_t) \
                                                                              \
  M_CHECK_COMPATIBLE_OPLIST(name, 1, key_type, key_oplist)                    \
  M_CHECK_COMPATIBLE_OPLIST(name, 2, value_type, value_oplist)                \
                                                                              \
  M_D1CT_FUNC_DEF2_P5(name, key_type, key_oplist, value_type, value_oplist, 0, dict_t, dict_it_t, it_deref_t )


/* Define a set with the key key_type
   It is defined as an array of singly linked list (each list
   representing a bucket).
*/
#define M_D1CT_SET_DEF_P1(arg) M_ID( M_D1CT_SET_DEF_P2 arg )

/* Validate the key oplist before going further */
#define M_D1CT_SET_DEF_P2(name, key_type, key_oplist, dict_t, dict_it_t, it_deref_t) \
  M_IF_OPLIST(key_oplist)(M_D1CT_SET_DEF_P4, M_D1CT_SET_DEF_FAILURE)(name, key_type, key_oplist, dict_t, dict_it_t, it_deref_t)

/* Stop processing with a compilation failure */
#define M_D1CT_SET_DEF_FAILURE(name, key_type, key_oplist, dict_t, dict_it_t, it_deref_t) \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST, "(DICT_SET_DEF): the given argument is not a valid oplist: " M_AS_STR(key_oplist) )

#define M_D1CT_SET_DEF_P4(name, key_type, key_oplist, dict_t, dict_it_t, it_deref_t) \
                                                                              \
  M_CHECK_COMPATIBLE_OPLIST(name, 1, key_type, key_oplist)                    \
                                                                              \
  M_D1CT_FUNC_DEF2_P5(name, key_type, key_oplist, key_type, M_EMPTY_OPLIST, 1, dict_t, dict_it_t, it_deref_t)

/* Define the dictionary contract */
#ifdef NDEBUG
#define M_D1CT_CONTRACT(dict)
#else
#define M_D1CT_CONTRACT(dict) do {                                            \
    M_ASSERT ( (dict) != NULL);                                               \
    M_ASSERT( (dict)->lower_limit <= (dict)->count);                          \
    M_ASSERT( (dict)->count <= (dict)->upper_limit );                         \
    M_ASSERT( (dict)->index != NULL);                                         \
    M_ASSERT( (dict)->data != NULL);                                          \
    M_ASSERT( M_POWEROF2_P((dict)->mask+1));                                  \
    M_ASSERT( (dict)->mask+1 >= M_D1CT_INITIAL_SIZE);                         \
    M_ASSERT( (dict)->upper_limit <= (dict)->mask+1);                         \
    M_ASSERT( (dict)->freelist_first_data != 0 || (dict)->count+2 == (dict)->freelist_count); \
  } while (0)
#endif

/* Define the structure of a dictionary for map and set
 * name: prefix of the container,
 * key_type: type of the key
 * key_oplist: oplist of the key
 * value_type: type of the value (if a SET, it is = key_type)
 * value_oplist: oplist of the value (if a SET, all methods are NOP)
 * isSet: is the container a SET (=1) or a MAP (=0)
 * dict_t: name of the type to construct
 * dict_it_t: name of the iterator within the dictionary.
 * it_deref_t: name of the type returned by an iterator
*/
#define M_D1CT_FUNC_DEF2_P5(name, key_type, key_oplist, value_type, value_oplist, isSet, dict_t, dict_it_t, it_deref_t) \
                                                                              \
  /* Define pair of key,value */                                              \
  typedef struct M_F(name, _pair_s) {                                         \
    key_type   key;                                                           \
    M_IF(isSet)( , value_type value;)                                         \
  } M_F(name, _pair_ct);                                                      \
                                                                              \
  /* Define either a freelist or a pair of (key,value) */                     \
  typedef union {                                                             \
    m_index_t           freelist;                                             \
    M_F(name, _pair_ct)  pair;                                                \
  } M_F(name, _freelist_ct);                                                  \
                                                                              \
  /* Define the dictionary */                                                 \
  typedef struct M_F(name, _s) {                                              \
    m_index_t count, count_delete;                                            \
    m_index_t upper_limit, lower_limit;                                       \
    m_index_t mask;                                                           \
    m_index_t freelist_first_data, freelist_count, freelist_cap;              \
    m_indexhash_t *index;                                                     \
    M_F(name, _freelist_ct) *data;                                            \
  } dict_t[1];                                                                \
                                                                              \
  typedef struct M_F(name, _s) *M_F(name, _ptr);                              \
  typedef const struct M_F(name, _s) *M_F(name, _srcptr);                     \
                                                                              \
  /* Define iterator type */                                                  \
  typedef struct M_F(name, _it_s) {                                           \
    const struct M_F(name, _s) *dict;                                         \
    m_index_t index;                                                          \
  } dict_it_t[1];                                                             \
                                                                              \
  /* Define type returned by the _ref method of an iterator */                \
  typedef M_IF(isSet)(key_type, M_F(name, _pair_ct)) it_deref_t;              \
                                                                              \
  /* Define internal types for oplist usage */                                \
  typedef dict_t M_F(name, _ct);                                              \
  typedef it_deref_t M_F(name, _subtype_ct);                                  \
  typedef key_type M_F(name, _key_ct);                                        \
  typedef value_type M_F(name, _value_ct);                                    \
  typedef dict_it_t M_F(name, _it_ct);                                        \
  typedef m_index_t M_F(name, _index_ct);                                     \
                                                                              \
  M_INLINE void                                                               \
  M_C3(m_d1ct_,name,_update_limit)(dict_t map, m_index_t size)                \
  {                                                                           \
    map->upper_limit = (m_index_t) ((double) size * M_D1CT_OA_UPPER_BOUND) - 1; \
    map->lower_limit = (size <= M_D1CT_INITIAL_SIZE) ? 0 : (m_index_t) ((double) size * M_D1CT_OA_LOWER_BOUND) ; \
    if (M_UNLIKELY(map->count <= map->lower_limit)) {                         \
      map->lower_limit = 0;                                                   \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_P(void, name, _init, dict_t map)                                          \
  {                                                                           \
    M_ASSERT (map != NULL);                                                   \
    map->mask = M_D1CT_INITIAL_SIZE-1;                                        \
    map->count = 0;                                                           \
    map->count_delete = 0;                                                    \
    M_C3(m_d1ct_,name,_update_limit)(map, M_D1CT_INITIAL_SIZE);               \
    /* The first 2 buckets are reserved for (empty) and (deleted) access. Allocation could be avoided */ \
    map->data = M_CALL_REALLOC(key_oplist, M_F(name, _freelist_ct), NULL, 0, (size_t) 1+2+map->upper_limit); \
    if (M_UNLIKELY_NOMEM (map->data == NULL)) {                               \
      M_MEMORY_FULL(M_F(name, _freelist_ct), 2+M_D1CT_INITIAL_SIZE);          \
    }                                                                         \
    map->freelist_first_data = 0;                                             \
    map->freelist_count = 2;                                                  \
    map->freelist_cap = 1+2+map->upper_limit;                                 \
    map->index = M_CALL_REALLOC(key_oplist, m_indexhash_t, NULL, 0, (size_t)(0+M_D1CT_INITIAL_SIZE)); \
    if (M_UNLIKELY_NOMEM (map->index == NULL)) {                              \
      M_MEMORY_FULL(m_indexhash_t, 2+M_D1CT_INITIAL_SIZE);                    \
    }                                                                         \
    /* Populate the initial table with the 'empty' representation which is 0 */ \
    for(m_index_t i = 0; i < M_D1CT_INITIAL_SIZE; i++) {                      \
      map->index[i].index = 0;                                                \
      map->index[i].hash  = 0;                                                \
    }                                                                         \
    M_D1CT_CONTRACT(map);                                                     \
  }                                                                           \
                                                                              \
  M_P(void, name,_clear, dict_t map)                                          \
  {                                                                           \
    M_D1CT_CONTRACT(map);                                                     \
    /* Clear objects */                                                       \
    /* We want to iterate over data directly, but we cannot :                 \
       we don't know if it is initialized or not otherwise */                 \
    for(m_index_t i = 0; i <= map->mask; i++) {                               \
      m_index_t d = map->index[i].index;                                      \
      if (d >= 2) {                                                           \
        M_CALL_CLEAR(key_oplist, map->data[d].pair.key);                      \
        M_CALL_CLEAR(value_oplist, map->data[d].pair.value);                  \
      }                                                                       \
    }                                                                         \
    M_CALL_FREE(key_oplist, m_indexhash_t, map->index, map->mask+1);          \
    M_CALL_FREE(key_oplist, M_F(name, _freelist_ct), map->data, map->freelist_cap); \
    /* Mark the dictionary as cleared */                                      \
    map->data = NULL;                                                         \
    map->index = NULL;                                                        \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _init_move)(dict_t map, dict_t org)                               \
  {                                                                           \
    M_D1CT_CONTRACT(org);                                                     \
    memcpy(map, org, sizeof (dict_t));                                        \
    /* Mark the original dictionary as cleared (invalid representation) */    \
    org->data = NULL;                                                         \
    org->index = NULL;                                                        \
  }                                                                           \
                                                                              \
                                                                              \
  M_IF_DEBUG(                                                                 \
  M_INLINE void                                                               \
  M_C3(m_d1ct_,name,_control_after_resize)(const dict_t h)                    \
  {                                                                           \
    /* This function checks if the reshashing of the dict is ok */            \
    size_t empty = 0;                                                         \
    size_t del = 0;                                                           \
    /* Count the number of empty elements and the number of deleted */        \
    for(size_t i = 0 ; i <= h->mask ; i++) {                                  \
      empty += h->index[i].index == 0;                                        \
      del   += h->index[i].index == 1;                                        \
      M_ASSERT( h->index[i].index < h->freelist_count);                       \
    }                                                                         \
    M_ASSERT(del == 0);                                                       \
    M_ASSERT(empty + h->count == h->mask + 1);                                \
  }                                                                           \
  )                                                                           \
                                                                              \
  M_INLINE m_index_t                                                          \
  M_C3(m_d1ct_,name,_get_free_bucket)(dict_t map)                             \
  {                                                                           \
    m_index_t d = map->freelist_first_data;                                   \
    if (d != 0) {                                                             \
      M_ASSERT(d >= 2);                                                       \
      map->freelist_first_data = map->data[d].freelist;                       \
      return d;                                                               \
    }                                                                         \
    M_ASSERT(map->freelist_count <= 2+map->upper_limit);                      \
    return map->freelist_count++;                                             \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_C3(m_d1ct_,name,_release_bucket)(dict_t map, m_index_t d)                 \
  {                                                                           \
    M_ASSERT(d >= 2 && d < map->freelist_count);                              \
    map->data[d].freelist = map->freelist_first_data;                         \
    map->freelist_first_data = d;                                             \
  }                                                                           \
                                                                              \
  M_INLINE value_type *                                                       \
  M_F(name, _get)(const dict_t map, key_type const key)                       \
  {                                                                           \
    M_D1CT_CONTRACT(map);                                                     \
    const m_index_t mask = map->mask;                                         \
    m_index_t hash = (m_index_t) M_CALL_HASH(key_oplist, key);                \
    m_index_t p = hash & mask;                                                \
    m_index_t s = 1;                                                          \
    /* We are likely to find the correct bucket first */                      \
    while (true) {                                                            \
      if (M_LIKELY (hash == map->index[p].hash)) {                            \
        m_index_t d = map->index[p].index;                                    \
        if (M_LIKELY(d >=2 && M_CALL_EQUAL(key_oplist, map->data[d].pair.key, key))) { \
          return &map->data[d].pair.M_IF(isSet)(key, value);                  \
        }                                                                     \
      }                                                                       \
      if (M_LIKELY (map->index[p].index == 0)) {                              \
        return NULL;                                                          \
      }                                                                       \
      p = (p + M_D1CT_OA_PROBING(s)) & mask;                                  \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_INLINE value_type *                                                       \
  M_F(name, _prehashed_get)(const dict_t map, key_type const key, size_t prehash) \
  {                                                                           \
    M_D1CT_CONTRACT(map);                                                     \
    M_ASSERT( M_CALL_HASH(key_oplist, key) == prehash);                       \
    const m_index_t mask = map->mask;                                         \
    m_index_t p = (m_index_t) prehash & mask;                                 \
    m_index_t s = 1;                                                          \
    /* We are likely to find the correct bucket first */                      \
    while (true) {                                                            \
      if (M_LIKELY ((m_index_t) prehash == map->index[p].hash)) {             \
        m_index_t d = map->index[p].index;                                    \
        if (M_LIKELY(d >=2 && M_CALL_EQUAL(key_oplist, map->data[d].pair.key, key))) { \
          return &map->data[d].pair.M_IF(isSet)(key, value);                  \
        }                                                                     \
      }                                                                       \
      if (M_LIKELY (map->index[p].index == 0)) {                              \
        return NULL;                                                          \
      }                                                                       \
      p = (p + M_D1CT_OA_PROBING(s)) & mask;                                  \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_P(void, name, _i_resize_up, dict_t h, m_index_t newSize, bool updateLimit) \
  {                                                                           \
    /* NOTE: Contract may not be fulfilled here */                            \
    m_index_t oldSize = h->mask+1;                                            \
    M_ASSERT (newSize >= oldSize);                                            \
    M_ASSERT (M_POWEROF2_P(newSize));                                         \
                                                                              \
    if (updateLimit == true) {                                                \
      M_C3(m_d1ct_,name,_update_limit)(h, newSize);                           \
    }                                                                         \
    /* resize can be called just to delete the items */                       \
    if (newSize > oldSize) {                                                  \
      /* h->data is not always the same size than h->index: it may be bigger  \
         if there is some erase of items: h->index will shrink, but not h->data \
         if it is bigger than our new size, then we don't need to reallocate */ \
      /* We need to allocate '2' for the dummy first two entries in the table (not used), \
         then we can have at maximum only up to 'upper_limit+1' data */       \
      if (1+2+h->upper_limit > h->freelist_cap) {                             \
        h->data = M_CALL_REALLOC(key_oplist, M_F(name, _freelist_ct), h->data, h->freelist_cap, (size_t) 1+2+h->upper_limit); \
        if (M_UNLIKELY_NOMEM (h->data == NULL) ) {                            \
          M_MEMORY_FULL(M_F(name, _freelist_ct), 2+newSize);                  \
        }                                                                     \
        h->freelist_cap = 1+2+h->upper_limit;                                 \
      }                                                                       \
      m_indexhash_t *index = M_CALL_REALLOC(key_oplist, m_indexhash_t, h->index, oldSize, (size_t)0+newSize); \
      if (M_UNLIKELY_NOMEM (index == NULL) ) {                                \
        M_MEMORY_FULL(m_indexhash_t, newSize);                                \
      }                                                                       \
      h->index = index;                                                       \
                                                                              \
      /* First mark the extended space as empty */                            \
      for(m_index_t i = oldSize ; i < newSize; i++) {                         \
        h->index[i].index = 0;                                                \
        h->index[i].hash  = 0;                                                \
      }                                                                       \
    }                                                                         \
                                                                              \
    /* Then let's rehash all the entries in their **exact** position.         \
      If we can't, let's put them in the 'tmp' array.                         \
      It has been measured that the size of this 'tmp' array is               \
      around 6% of the size of updated dictionary.                            \
      NOTE: This should be much cache friendly than typical hash code  */     \
    m_array_index_t tmp;                                                      \
    m_array_index_init(tmp);                                                  \
    const m_index_t mask = (newSize -1);                                      \
                                                                              \
    for(m_index_t i = 0 ; i < oldSize; i++) {                                 \
      m_index_t ind  = h->index[i].index;                                     \
      m_index_t hash = h->index[i].hash;                                      \
      h->index[i].index = 0;                                                  \
      if (ind >= 2) {                                                         \
        m_index_t p = hash & mask;                                            \
        if (h->index[p].index <= 1) {                                         \
          h->index[p].index = ind;                                            \
          h->index[p].hash  = hash;                                           \
        } else {                                                              \
          m_indexhash_t t = { ind, hash };                                    \
          m_array_index_push_back M_R(tmp, t);                                \
        }                                                                     \
      }                                                                       \
    }                                                                         \
                                                                              \
    /* Let's put back the entries in the tmp array in their right place */    \
    /* NOTE: There should be very few entries in this array                   \
      which contains what we weren't be able to fit in the first pass */      \
    while (m_array_index_size(tmp) > 0) {                                     \
      m_indexhash_t ind;                                                      \
      m_array_index_pop_back M_R(&ind, tmp);                                  \
      m_index_t p = ind.hash & mask;                                          \
      /* NOTE: since the first pass, the bucket might be free now */          \
      m_index_t s = 1;                                                        \
      while (h->index[p].index != 0) {                                        \
        p = (p + M_D1CT_OA_PROBING(s)) & mask;                                \
        M_ASSERT (s <= h->mask);                                              \
      }                                                                       \
      h->index[p].index = ind.index;                                          \
      h->index[p].hash  = ind.hash;                                           \
    }                                                                         \
                                                                              \
    m_array_index_clear M_R(tmp);                                             \
    h->mask = newSize-1;                                                      \
    h->count_delete = h->count;                                               \
    M_IF_DEBUG(  M_C3(m_d1ct_,name,_control_after_resize)(h);  )              \
  }                                                                           \
                                                                              \
  M_P(void, name, _i_resize_down, dict_t h, m_index_t newSize)                \
  {                                                                           \
    /* NOTE: Contract may not be fulfilled here */                            \
    m_index_t oldSize = h->mask+1;                                            \
    M_ASSERT (newSize <= oldSize && M_POWEROF2_P(newSize));                   \
    if (M_UNLIKELY (newSize < M_D1CT_INITIAL_SIZE))                           \
      newSize = M_D1CT_INITIAL_SIZE;                                          \
    const m_index_t mask = newSize -1;                                        \
    m_array_index_t tmp;                                                      \
    m_array_index_init(tmp);                                                  \
                                                                              \
    /* Pass 1: scan lower entries, and move them if needed */                 \
    for(m_index_t i = 0; i < newSize; i++) {                                  \
      m_index_t ind  = h->index[i].index;                                     \
      m_index_t hash = h->index[i].hash;                                      \
      h->index[i].index = 0;                                                  \
      if (ind >= 2) {                                                         \
        m_index_t p = hash & mask;                                            \
        if (h->index[p].index <= 1) {                                         \
          h->index[p].index = ind;                                            \
          h->index[p].hash  = hash;                                           \
        } else {                                                              \
          m_indexhash_t t = { ind, hash };                                    \
          m_array_index_push_back M_R(tmp, t);                                \
        }                                                                     \
      }                                                                       \
    }                                                                         \
    /* Pass 2: scan upper entries and move them back */                       \
    for(m_index_t i = newSize; i < oldSize; i++) {                            \
      if (h->index[i].index >= 2) {                                           \
        m_index_t p = h->index[i].hash & mask;                                \
        m_index_t s = 1;                                                      \
        while (h->index[p].index != 0) {                                      \
          p = (p + M_D1CT_OA_PROBING(s)) & mask;                              \
          M_ASSERT (s <= h->mask);                                            \
        }                                                                     \
        h->index[p].index = h->index[i].index;                                \
        h->index[p].hash  = h->index[i].hash;                                 \
      }                                                                       \
    }                                                                         \
    /* Pass 3: scan failed moved entries and move them back */                \
    while (m_array_index_size(tmp) > 0) {                                     \
      m_indexhash_t ind;                                                      \
      m_array_index_pop_back M_R(&ind, tmp);                                  \
      m_index_t p = ind.hash & mask;                                          \
      m_index_t s = 1;                                                        \
      while (h->index[p].index != 0) {                                        \
        p = (p + M_D1CT_OA_PROBING(s)) & mask;                                \
        M_ASSERT (s <= h->mask);                                              \
      }                                                                       \
      h->index[p].index = ind.index;                                          \
      h->index[p].hash  = ind.hash;                                           \
    }                                                                         \
                                                                              \
    m_array_index_clear M_R(tmp);                                             \
    h->count_delete = h->count;                                               \
    if (newSize != oldSize) {                                                 \
      h->mask = newSize-1;                                                    \
      M_C3(m_d1ct_,name,_update_limit)(h, newSize);                           \
      h->index = M_CALL_REALLOC(key_oplist, m_indexhash_t, h->index, oldSize, (size_t)0+newSize); \
      M_ASSERT (h->index != NULL);                                            \
      /* FIXME: What to do for h->data ? */                                   \
    }                                                                         \
    M_IF_DEBUG(  M_C3(m_d1ct_,name,_control_after_resize)(h);  )              \
    M_D1CT_CONTRACT(h);                                                       \
  }                                                                           \
                                                                              \
  M_IF(isSet)(                                                                \
    M_P(void, name, _push, dict_t map, key_type const key) ,                  \
    M_P(void, name, _set_at, dict_t map, key_type const key, value_type const value)) \
  {                                                                           \
    M_D1CT_CONTRACT(map);                                                     \
    const m_index_t mask = map->mask;                                         \
    m_index_t hash = (m_index_t) M_CALL_HASH(key_oplist, key);                \
    m_index_t p = hash & mask;                                                \
                                                                              \
    /* Test if bucket is not empty ? (50 % likely) */                         \
    if (map->index[p].index != 0) {                                           \
      /* Find the insertion point as the bucket[] is not empty */             \
      m_index_t delPos = (m_index_t) -1;                                      \
      m_index_t s = 1U;                                                       \
      do {                                                                    \
        if (M_UNLIKELY(hash == (map->index[p].hash))) {                       \
          m_index_t d = map->index[p].index;                                  \
          M_ASSERT(d <= map->freelist_count);                                 \
          if (d >= 2 && M_CALL_EQUAL(key_oplist, map->data[d].pair.key, key)) { \
            M_CALL_SET(value_oplist, map->data[d].pair.value, value);         \
            return;                                                           \
          }                                                                   \
        }                                                                     \
        if (map->index[p].index == 1 && delPos == (m_index_t) -1) delPos = p; \
        p = (p + M_D1CT_OA_PROBING(s)) & mask;                                \
      } while (map->index[p].index != 0);                                     \
      if (delPos != (m_index_t) -1) {                                         \
        p = delPos;                                                           \
        map->count_delete --;                                                 \
      }                                                                       \
    }                                                                         \
                                                                              \
    m_index_t d = M_C3(m_d1ct_,name,_get_free_bucket)(map);                   \
    M_CALL_INIT_SET(key_oplist, map->data[d].pair.key, key);                  \
    M_CALL_INIT_SET(value_oplist, map->data[d].pair.value, value);            \
    map->index[p].index = d;                                                  \
    map->index[p].hash  = hash;                                               \
    map->count++;                                                             \
    map->count_delete ++;                                                     \
                                                                              \
    if (M_UNLIKELY (map->count_delete >= map->upper_limit)) {                 \
      m_index_t newSize = map->mask+1;                                        \
      if (map->count > newSize/2) {                                           \
        newSize += newSize;                                                   \
        if (M_UNLIKELY_NOMEM (newSize <= map->mask+1)) {                      \
          M_MEMORY_FULL(char, (size_t)-1);                                    \
        }                                                                     \
      }                                                                       \
      M_F(name,_i_resize_up)M_R(map, newSize, true);                          \
    }                                                                         \
    M_D1CT_CONTRACT(map);                                                     \
  }                                                                           \
                                                                              \
  M_P(value_type *, name, _safe_get, dict_t map, key_type const key)          \
  {                                                                           \
    M_D1CT_CONTRACT(map);                                                     \
    const m_index_t mask = map->mask;                                         \
    const m_index_t hash = (m_index_t) M_CALL_HASH(key_oplist, key);          \
    m_index_t p = hash & mask;                                                \
                                                                              \
    if (M_UNLIKELY (hash == map->index[p].hash)) {                            \
      m_index_t d = map->index[p].index;                                      \
      M_ASSERT(d <= map->freelist_count);                                     \
      if (d >= 2 && M_CALL_EQUAL(key_oplist, map->data[d].pair.key, key)) {   \
        return & map->data[d].pair.M_IF(isSet)(key, value);                   \
      }                                                                       \
    }                                                                         \
                                                                              \
    if (M_UNLIKELY( map->index[p].index != 0)) {                              \
      /* Find the insertion point as the bucket[] is not empty */             \
      m_index_t delPos = (m_index_t)-1;                                       \
      if (map->index[p].index == 1) delPos = p;                               \
      m_index_t s = 1U;                                                       \
      do {                                                                    \
        p = (p + M_D1CT_OA_PROBING(s)) & mask;                                \
        if (hash == (map->index[p].hash)) {                                   \
          m_index_t d = map->index[p].index;                                  \
          M_ASSERT(d <= map->freelist_count);                                 \
          if (d >= 2 && M_CALL_EQUAL(key_oplist, map->data[d].pair.key, key)) { \
            return & map->data[d].pair.M_IF(isSet)(key, value);               \
          }                                                                   \
        }                                                                     \
        if (map->index[p].index == 1 && delPos == (m_index_t) -1) delPos = p; \
      } while (map->index[p].index != 0);                                     \
      if (delPos != (m_index_t) -1) {                                         \
        p = delPos;                                                           \
        map->count_delete --;                                                 \
      }                                                                       \
    }                                                                         \
                                                                              \
    m_index_t d = M_C3(m_d1ct_,name,_get_free_bucket)(map);                   \
    M_CALL_INIT_SET(key_oplist, map->data[d].pair.key, key);                  \
    M_CALL_INIT(value_oplist, map->data[d].pair.value);                       \
    map->index[p].index = d;                                                  \
    map->index[p].hash  = hash;                                               \
    map->count++;                                                             \
    map->count_delete ++;                                                     \
                                                                              \
    if (M_UNLIKELY (map->count_delete >= map->upper_limit)) {                 \
      m_index_t newSize = map->mask+1;                                        \
      if (map->count > newSize/2) {                                           \
        newSize += newSize;                                                   \
        if (M_UNLIKELY_NOMEM (newSize <= map->mask+1)) {                      \
          M_MEMORY_FULL(char, (size_t)-1);                                    \
        }                                                                     \
      }                                                                       \
      M_F(name,_i_resize_up)M_R(map, newSize, true);                          \
    }                                                                         \
    M_D1CT_CONTRACT(map);                                                     \
    /* bucket index won't move even if resize is done */                      \
    return &map->data[d].pair.M_IF(isSet)(key, value);                        \
  }                                                                           \
                                                                              \
  M_P(bool, name, _erase, dict_t map, key_type const key)                     \
  {                                                                           \
    M_D1CT_CONTRACT(map);                                                     \
                                                                              \
    const m_index_t mask = map->mask;                                         \
    const m_index_t hash = (m_index_t) M_CALL_HASH(key_oplist, key);          \
    m_index_t p = hash & mask;                                                \
                                                                              \
    m_index_t s = 1;                                                          \
    while (true) {                                                            \
      if (M_LIKELY (hash == map->index[p].hash)) {                            \
        m_index_t d = map->index[p].index;                                    \
        M_ASSERT(d <= map->freelist_count);                                   \
        if (d >= 2 && M_CALL_EQUAL(key_oplist, map->data[d].pair.key, key)) { \
          break;                                                              \
        }                                                                     \
      }                                                                       \
      if (map->index[p].index == 0)                                           \
        return false;                                                         \
      p = (p + M_D1CT_OA_PROBING(s)) & mask;                                  \
      M_ASSERT (s <= map->mask);                                              \
    }                                                                         \
    const m_index_t d = map->index[p].index;                                  \
    M_ASSERT(d <= map->freelist_count);                                       \
    M_CALL_CLEAR(key_oplist, map->data[d].pair.key);                          \
    M_CALL_CLEAR(value_oplist, map->data[d].pair.value);                      \
    M_C3(m_d1ct_,name,_release_bucket)(map, d);                               \
    map->index[p].index = 1;                                                  \
    M_ASSERT (map->count >= 1);                                               \
    map->count--;                                                             \
    if (M_UNLIKELY (map->count < map->lower_limit)) {                         \
      M_F(name,_i_resize_down)M_R(map, (map->mask+1) >> 1);                   \
    }                                                                         \
    M_D1CT_CONTRACT(map);                                                     \
    return true;                                                              \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it)(dict_it_t it, const dict_t d)                                \
  {                                                                           \
    M_D1CT_CONTRACT(d);                                                       \
    M_ASSERT(it != NULL);                                                     \
    it->dict = d;                                                             \
    m_index_t i = 0;                                                          \
    while (i <= d->mask && d->index[i].index <= 1)                            \
      i++;                                                                    \
    it->index = i;                                                            \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_set)(dict_it_t it, const dict_it_t ref)                       \
  {                                                                           \
    M_ASSERT (it != NULL && ref != NULL);                                     \
    it->dict = ref->dict;                                                     \
    it->index = ref->index;                                                   \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_end)(dict_it_t it, const dict_t d)                            \
  {                                                                           \
    M_D1CT_CONTRACT(d);                                                       \
    M_ASSERT(it != NULL);                                                     \
    it->dict  = d;                                                            \
    it->index = d->mask+1;                                                    \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _end_p)(const dict_it_t it)                                       \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    return it->index > it->dict->mask;                                        \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _next)(dict_it_t it)                                              \
  {                                                                           \
    M_ASSERT(it != NULL);                                                     \
    const M_F(name, _srcptr) d = it->dict;                                    \
    m_index_t i = it->index;                                                  \
    do {                                                                      \
      i++;                                                                    \
    } while (M_LIKELY(i <= d->mask) && M_UNLIKELY(d->index[i].index <= 1));   \
    it->index = i;                                                            \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _last_p)(const dict_it_t it)                                      \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    const M_F(name, _srcptr) d = it->dict;                                    \
    m_index_t i = it->index+1;                                                \
    while (i <= d->mask && d->index[i].index <= 1)                            \
      i++;                                                                    \
    return i > d->mask;                                                       \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _it_equal_p)(const dict_it_t it1, const dict_it_t it2)            \
  {                                                                           \
    M_ASSERT (it1 != NULL && it2 != NULL);                                    \
    return it1->dict == it2->dict && it1->index == it2->index;                \
  }                                                                           \
                                                                              \
  M_INLINE it_deref_t *                                                       \
  M_F(name, _ref)(const dict_it_t it)                                         \
  {                                                                           \
    M_ASSERT(it != NULL);                                                     \
    M_ASSERT(!M_F(name, _end_p)(it));                                         \
    const M_F(name, _srcptr) d = it->dict;                                    \
    M_D1CT_CONTRACT(d);                                                       \
    /* NOTE: partially unsafe if the user modify the 'key'                    \
       in a non equivalent way */                                             \
    return &d->data[d->index[it->index].index].pair M_IF(isSet)(.key, );      \
  }                                                                           \
                                                                              \
  M_INLINE const it_deref_t *                                                 \
  M_F(name, _cref)(const dict_it_t it)                                        \
  {                                                                           \
    M_ASSERT(it != NULL);                                                     \
    M_ASSERT(!M_F(name, _end_p)(it));                                         \
    const M_F(name, _srcptr) d = it->dict;                                    \
    M_D1CT_CONTRACT(d);                                                       \
    return (const it_deref_t *) &d->data[d->index[it->index].index].pair M_IF(isSet)(.key, ); \
  }                                                                           \
                                                                              \
  M_D1CT_FUNC_ADDITIONAL_DEF2(name, key_type, key_oplist, value_type, value_oplist, isSet, dict_t, dict_it_t, it_deref_t)


/* Define additional functions for dictionary (Common for all kinds of dictionary).
   Do not used any specific fields of the dictionary but the public API

   It is not possible to define a method for IT_REMOVE: we could easily define it
   by performing an _erase of the key get by the _cref method. However,
   computing the next element is way harder. We could easily compute the next
   element of the iteration (using _next). However with the _erase method, the
   dict may perform a resize down operation, reducing the size of the array,
   base of the dict, by two. This operation renders the computation of the
   'next' element impossible as the order of the elements in the dict
   has fundamentally changed in this case. We could detect this and restart
   the iteration from the first element, but it wouldn't fit the contract
   of the IT_REMOVE operator.

   HASH method for dictionary itself seems hard to implement:
   we have to handle the case where two dictionaries are structurally
   different, but functionally identical (seems they have the same
   members, but put in a different order).
   We cannot iterator over the dictionary to compute a hash, as the
   order of the items in the dictionary is not specified: they more
   or less follow the hash of the keys, but if the low bits of the
   hash of the key is equal, they order may be different.
   Or if the table of the dictionary has different values (this may
   be avoided).
 */
#define M_D1CT_FUNC_ADDITIONAL_DEF2(name, key_type, key_oplist, value_type, value_oplist, isSet, dict_t, dict_it_t, it_deref_t) \
                                                                              \
                                                                              \
  M_P(void, name, _init_set, dict_t map, const dict_t org)                    \
  {                                                                           \
    dict_it_t it;                                                             \
    M_ASSERT (map != org);                                                    \
    M_F(name, _init)M_R(map);                                                 \
    /* Naive implementation. No need to optimize such access */               \
    for (M_F(name,_it)(it, org); !M_F(name,_end_p)(it); M_F(name,_next)(it)){ \
      M_IF(isSet)(                                                            \
        M_F(name,_push)M_R(map, * M_F(name,_ref)(it) )                        \
      , M_F(name,_set_at)M_R(map, M_F(name,_ref)(it)->key, M_F(name,_ref)(it)->value) \
      );                                                                      \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_P(void, name, _set, dict_t map, const dict_t org)                         \
  {                                                                           \
    if (M_LIKELY (map != org)) {                                              \
      /* Naive implementation. No need to optimize such access */             \
      M_F(name, _clear)M_R(map);                                              \
      M_F(name, _init_set)M_R(map, org);                                      \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_P(void, name, _move, dict_t map, dict_t org)                              \
  {                                                                           \
    M_ASSERT (map != org);                                                    \
    M_F(name,_clear)M_R(map);                                                 \
    M_F(name,_init_move)(map, org);                                           \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _swap)(dict_t d1, dict_t d2)                                      \
  {                                                                           \
    /* Naive swap (not worth optimizing) */                                   \
    dict_t tmp;                                                               \
    memcpy( tmp, d1, sizeof (dict_t));                                        \
    memcpy( d1, d2, sizeof (dict_t));                                         \
    memcpy( d2, tmp, sizeof (dict_t));                                        \
  }                                                                           \
                                                                              \
  M_P(void, name, _reset, dict_t d)                                           \
  {                                                                           \
    if (d->count == 0) return;                                                \
    /* Naive algorithm. Not work optimizing it */                             \
    M_F(name, _clear)M_R(d);                                                  \
    M_F(name, _init)M_R(d);                                                   \
  }                                                                           \
                                                                              \
  M_INLINE value_type const *                                                 \
  M_F(name, _cget)(const dict_t map, key_type const key)                      \
  {                                                                           \
    return M_CONST_CAST(value_type, M_F(name,_get)(map, key));                \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name,_empty_p)(const dict_t map)                                        \
  {                                                                           \
    M_ASSERT(map != NULL);                                                    \
    return map->count == 0;                                                   \
  }                                                                           \
                                                                              \
  M_INLINE size_t                                                             \
  M_F(name,_size)(const dict_t map)                                           \
  {                                                                           \
    M_ASSERT(map != NULL);                                                    \
    return map->count;                                                        \
  }                                                                           \
                                                                              \
  M_P(void, name,_reserve, dict_t dict, size_t capacity)                      \
  {                                                                           \
    if (capacity == 0) {                                                      \
      /* Perform a shrink to fit: copy everything to a new dict properly, and move it back */ \
      dict_t tmp;                                                             \
      M_F(name, _init_set)M_R(tmp, dict);                                     \
      M_F(name, _move)M_R(dict, tmp);                                         \
      return;                                                                 \
    }                                                                         \
    /* Get the size which will allow to fit this capacity                     \
       NOTE: Strictly speaking we need to perform a round up to ensure        \
       that no reallocation of the hash map occurs up to capacity */          \
    M_F(name, _index_ct) size = (M_F(name, _index_ct))                        \
      m_core_roundpow2 ((uint64_t) (1.0+(double) capacity * (1.0 / M_D1CT_OA_UPPER_BOUND))); \
    M_ASSERT (M_POWEROF2_P(size));                                            \
    /* Test for overflow of the computation */                                \
    if (M_UNLIKELY_NOMEM (size < capacity)) {                                 \
      M_MEMORY_FULL(char, (size_t)-1);                                        \
    }                                                                         \
    if (size > dict->mask+1) {                                                \
      dict->upper_limit = (M_F(name, _index_ct)) ((double) size * M_D1CT_OA_UPPER_BOUND) - 1; \
      M_F(name,_i_resize_up)M_R(dict, size, false);                           \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_IF_METHOD(EQUAL, value_oplist)(                                           \
  M_INLINE bool                                                               \
  M_F(name, _equal_p)(const dict_t dict1, const dict_t dict2)                 \
  {                                                                           \
    M_ASSERT (dict1 != NULL && dict2 != NULL);                                \
    /* NOTE: Key type has mandatory equal operator */                         \
    /* First the easy cases */                                                \
    if (M_LIKELY (dict1->count != dict2->count))                              \
      return false;                                                           \
    if (M_UNLIKELY (dict1->count == 0))                                       \
      return true;                                                            \
    /* Otherwise this is the slow path :                                      \
       both dictionary may not have arrays with the same size, but            \
       still the dictionaries shall be equal as they contain the same         \
       items. */                                                              \
    dict_it_t it;                                                             \
    for(M_F(name, _it)(it, dict1) ;                                           \
        !M_F(name, _end_p)(it);                                               \
        M_F(name, _next)(it)) {                                               \
      const it_deref_t *item = M_F(name, _cref)(it);                          \
      value_type *ptr = M_F(name, _get)(dict2, M_IF(isSet)(*item, item->key)); \
      if (ptr == NULL)                                                        \
        return false;                                                         \
      if (M_CALL_EQUAL(value_oplist, item->value, *ptr) == false)             \
        return false;                                                         \
    }                                                                         \
    return true;                                                              \
  }                                                                           \
  , /* no value equal */ )                                                    \
                                                                              \
  M_IF_METHOD_BOTH(GET_STR, key_oplist, value_oplist)(                        \
  M_P(void, name, _get_str, m_string_t str, const dict_t dict, const bool append) \
  {                                                                           \
    (append ? m_string_cat_cstr : m_string_set_cstr)M_R(str, "{");            \
    dict_it_t it;                                                             \
    bool print_comma = false;                                                 \
    for (M_F(name, _it)(it, dict) ;                                           \
         !M_F(name, _end_p)(it);                                              \
         M_F(name, _next)(it)){                                               \
      if (print_comma)                                                        \
        m_string_push_back M_R(str, ',');                                     \
      const it_deref_t *item = M_F(name, _cref)(it);                          \
      M_IF(isSet)(                                                            \
                  M_CALL_GET_STR(key_oplist, str, *item, true);               \
                  ,                                                           \
                  M_CALL_GET_STR(key_oplist, str, item->key, true);           \
                  m_string_push_back M_R(str, ':');                           \
                  M_CALL_GET_STR(value_oplist, str, item->value, true);       \
                  )                                                           \
      print_comma = true;                                                     \
    }                                                                         \
    m_string_push_back M_R(str, '}');                                         \
  }                                                                           \
  , /* no GET_STR */ )                                                        \
                                                                              \
  M_IF_METHOD_BOTH(OUT_STR, key_oplist, value_oplist)(                        \
  M_INLINE void                                                               \
  M_F(name, _out_str)(FILE *file, const dict_t dict)                          \
  {                                                                           \
    M_ASSERT (file != NULL);                                                  \
    fputc ('{', file);                                                        \
    dict_it_t it;                                                             \
    bool print_comma = false;                                                 \
    for (M_F(name, _it)(it, dict) ;                                           \
         !M_F(name, _end_p)(it);                                              \
         M_F(name, _next)(it)){                                               \
      if (print_comma)                                                        \
        fputc (',', file);                                                    \
      const it_deref_t *item = M_F(name, _cref)(it);                          \
      M_IF(isSet)(                                                            \
                  M_CALL_OUT_STR(key_oplist, file, *item);                    \
                  ,                                                           \
                  M_CALL_OUT_STR(key_oplist, file, item->key);                \
                  fputc (':', file);                                          \
                  M_CALL_OUT_STR(value_oplist, file, item->value);            \
                  )                                                           \
      print_comma = true;                                                     \
    }                                                                         \
    fputc ('}', file);                                                        \
  }                                                                           \
  , /* no OUT_STR */ )                                                        \
                                                                              \
  M_IF_METHOD_BOTH(PARSE_STR, key_oplist, value_oplist)(                      \
  M_P(bool, name, _parse_str, dict_t dict, const char str[], const char **endp) \
  {                                                                           \
    M_ASSERT (str != NULL);                                                   \
    M_F(name, _reset)M_R(dict);                                               \
    int c = m_core_str_nospace(&str);                                         \
    if (M_UNLIKELY (c != '{')) { c = 0; goto exit; }                          \
    c = m_core_str_nospace(&str);                                             \
    if (M_UNLIKELY (c == '}' || c == 0)) goto exit;                           \
    str--;                                                                    \
    M_QLET(1, key, key_type, key_oplist)                                      \
    M_IF(isSet)(,M_QLET(2, value, value_type, value_oplist)) {                \
      do {                                                                    \
        c = m_core_str_nospace(&str);                                         \
        str--;                                                                \
        bool b = M_CALL_PARSE_STR(key_oplist, key, str, &str);                \
        M_IF(isSet)(                                                          \
            if (b == false) { c = 0; break; }                                 \
            M_F(name, _push)M_R(dict, key);                                   \
          , /* is a map */                                                    \
            c = m_core_str_nospace(&str);                                     \
            if (b == false || c != ':') { c = 0; break; }                     \
            c = m_core_str_nospace(&str);                                     \
            str--;                                                            \
            b = M_CALL_PARSE_STR(value_oplist, value, str, &str);             \
            if (b == false) { c = 0; break; }                                 \
            M_F(name, _set_at)M_R(dict, key, value);                          \
          )                                                                   \
        c = m_core_str_nospace(&str);                                         \
      } while (c == ',');                                                     \
    }                                                                         \
  exit:                                                                       \
    if (endp) *endp = str;                                                    \
    return c == '}';                                                          \
  }                                                                           \
  , /* no PARSE_STR */ )                                                      \
                                                                              \
  M_IF_METHOD_BOTH(IN_STR, key_oplist, value_oplist)(                         \
  M_P(bool, name, _in_str, dict_t dict, FILE *file)                           \
  {                                                                           \
    M_ASSERT (file != NULL);                                                  \
    M_F(name, _reset)M_R(dict);                                               \
    int c = m_core_fgetc_nospace(file);                                       \
    if (M_UNLIKELY (c != '{')) return false;                                  \
    c = m_core_fgetc_nospace(file);                                           \
    if (M_UNLIKELY(c == '}')) return true;                                    \
    if (M_UNLIKELY (c == EOF)) return false;                                  \
    ungetc(c, file);                                                          \
    M_QLET(1, key, key_type, key_oplist)                                      \
    M_IF(isSet)(,M_QLET(2, value, value_type, value_oplist)) {                \
      do {                                                                    \
        c = m_core_fgetc_nospace(file);                                       \
        if (M_UNLIKELY (c == EOF)) { break; }                                 \
        ungetc(c, file);                                                      \
        bool b = M_CALL_IN_STR(key_oplist, key, file);                        \
        M_IF(isSet)(                                                          \
            if (M_UNLIKELY (b == false)) { c = 0; break; }                    \
            M_F(name, _push)M_R(dict, key);                                   \
        , /* is map */                                                        \
            c = m_core_fgetc_nospace(file);                                   \
            if (M_UNLIKELY (b == false || c != ':')) { c = 0; break; }        \
            c = m_core_fgetc_nospace(file);                                   \
            if (M_UNLIKELY (c == EOF)) { break; }                             \
            ungetc(c, file);                                                  \
            b = M_CALL_IN_STR(value_oplist, value, file);                     \
            if (M_UNLIKELY (b == false)) { c = 0; break; }                    \
            M_F(name, _set_at)M_R(dict, key, value);                          \
        )                                                                     \
      c = m_core_fgetc_nospace(file);                                         \
      } while (c == ',');                                                     \
    }                                                                         \
    return c == '}';                                                          \
  }                                                                           \
  , /* no IN_STR */ )                                                         \
                                                                              \
  M_IF_METHOD_BOTH(OUT_SERIAL, key_oplist, value_oplist)(                     \
  M_P(m_serial_return_code_t, name, _out_serial, m_serial_write_t f, dict_t const t1) \
  {                                                                           \
    M_ASSERT (f != NULL && f->m_interface != NULL);                           \
    m_serial_local_t local;                                                   \
    m_serial_return_code_t ret;                                               \
    const it_deref_t *item;                                                   \
    bool first_done = false;                                                  \
    dict_it_t it;                                                             \
    /* Format is different between associative container                      \
       & set container */                                                     \
    M_IF(isSet)(                                                              \
                ret = f->m_interface->write_array_start M_R(local, f, M_F(name, _size)(t1)); \
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
                ,                                                             \
                ret = f->m_interface->write_map_start M_R(local, f, M_F(name, _size)(t1)); \
                for (M_F(name, _it)(it, t1) ;                                 \
                     !M_F(name, _end_p)(it);                                  \
                     M_F(name, _next)(it)){                                   \
                  item = M_F(name, _cref)(it);                                \
                  if (first_done)                                             \
                    ret |= f->m_interface->write_map_next M_R(local, f);      \
                  ret |= M_CALL_OUT_SERIAL(key_oplist, f, item->key);         \
                  ret |= f->m_interface->write_map_value M_R(local, f);       \
                  ret |= M_CALL_OUT_SERIAL(value_oplist, f, item->value);     \
                  first_done = true;                                          \
                }                                                             \
                ret |= f->m_interface->write_map_end M_R(local, f);           \
                                                                        )     \
      return ret & M_SERIAL_FAIL;                                             \
  }                                                                           \
  , /* no OUT_SERIAL */ )                                                     \
                                                                              \
  M_IF_METHOD_BOTH(IN_SERIAL, key_oplist, value_oplist)(                      \
  M_P(m_serial_return_code_t, name, _in_serial, dict_t t1, m_serial_read_t f) \
  {                                                                           \
    M_ASSERT (f != NULL && f->m_interface != NULL);                           \
    m_serial_local_t local;                                                   \
    m_serial_return_code_t ret;                                               \
    size_t estimated_size = 0;                                                \
    M_F(name,_reset)M_R(t1);                                                  \
    M_QLET(1, key, key_type, key_oplist)                                      \
    M_IF(isSet)(,M_QLET(2, value, value_type, value_oplist)) {                \
        M_IF(isSet)(                                                          \
            ret = f->m_interface->read_array_start(local, f, &estimated_size); \
            if (M_UNLIKELY (ret != M_SERIAL_OK_CONTINUE)) break;              \
            do {                                                              \
              ret = M_CALL_IN_SERIAL(key_oplist, key, f);                     \
              if (ret != M_SERIAL_OK_DONE) break;                             \
              M_F(name, _push)M_R(t1, key);                                   \
            } while ((ret = f->m_interface->read_array_next(local, f)) == M_SERIAL_OK_CONTINUE); \
        , /* is map */                                                        \
            ret = f->m_interface->read_map_start(local, f, &estimated_size);  \
            if (M_UNLIKELY (ret != M_SERIAL_OK_CONTINUE)) break;              \
            do {                                                              \
              ret = M_CALL_IN_SERIAL(key_oplist, key, f);                     \
              if (ret != M_SERIAL_OK_DONE) break;                             \
              ret = f->m_interface->read_map_value(local, f);                 \
              if (ret != M_SERIAL_OK_CONTINUE) break;                         \
              ret = M_CALL_IN_SERIAL(value_oplist, value, f);                 \
              if (ret != M_SERIAL_OK_DONE) break;                             \
              M_F(name, _set_at)M_R(t1, key, value);                          \
            } while ((ret = f->m_interface->read_map_next(local, f)) == M_SERIAL_OK_CONTINUE); \
        ) /* End of IF isSet */                                               \
    }                                                                         \
    return ret;                                                               \
  }                                                                           \
  , /* no in_serial */ )                                                      \
                                                                              \
  M_IF(isSet)(                                                                \
  M_P(void, name, _splice, dict_t d1, dict_t d2)                              \
  {                                                                           \
    dict_it_t it;                                                             \
    M_ASSERT(d1 != d2);                                                       \
    /* NOTE: Despite using set_at, the accessing of the item in d1            \
       is not as random as other uses of the HASH table as d2                 \
       uses the same order than d1 */                                         \
    for (M_F(name, _it)(it, d2); !M_F(name, _end_p)(it); M_F(name, _next)(it)){ \
      const it_deref_t *item = M_F(name, _cref)(it);                          \
      M_F(name, _push)M_R(d1, *item);                                         \
    }                                                                         \
    M_F(name, _reset)M_R(d2);                                                 \
  }                                                                           \
  ,                                                                           \
  M_IF_METHOD(ADD, value_oplist)(                                             \
  M_P(void, name, _splice, dict_t d1, dict_t d2)                              \
  {                                                                           \
    dict_it_t it;                                                             \
    /* NOTE: Despite using set_at, the accessing of the item in d1            \
       is not as random as other uses of the HASH table as d2                 \
       uses the same order than d1 */                                         \
    for (M_F(name, _it)(it, d2); !M_F(name, _end_p)(it); M_F(name, _next)(it)){ \
      const struct M_F(name, _pair_s) *item = M_F(name, _cref)(it);           \
      value_type *ptr = M_F(name, _get)(d1, item->key);                       \
      if (ptr == NULL) {                                                      \
        M_F(name, _set_at)M_R(d1, item->key, item->value);                    \
      } else {                                                                \
        M_CALL_ADD(value_oplist, *ptr, *ptr, item->value);                    \
      }                                                                       \
    }                                                                         \
    M_F(name, _reset)M_R(d2);                                                 \
  }                                                                           \
  , /* NO UPDATE */) )                                                        \
                                                                              \
  M_EMPLACE_ASS_ARRAY_OR_QUEUE_DEF(isSet, name, dict_t, key_oplist, value_oplist)


/******************************** INTERNAL ***********************************/

/* Deferred evaluation for the definition,
   so that all arguments are evaluated before further expansion */
#define M_D1CT_OPLIST_P1(arg) M_D1CT_OPLIST_P2 arg

/* Validation of the given oplists */
#define M_D1CT_OPLIST_P2(name, key_oplist, value_oplist)                      \
  M_IF_OPLIST(key_oplist)(M_D1CT_OPLIST_P3, M_D1CT_OPLIST_FAILURE)(name, key_oplist, value_oplist)
#define M_D1CT_OPLIST_P3(name, key_oplist, value_oplist)                      \
  M_IF_OPLIST(value_oplist)(M_D1CT_OPLIST_P4, M_D1CT_OPLIST_FAILURE)(name, key_oplist, value_oplist)

/* Prepare a clean compilation failure */
#define M_D1CT_OPLIST_FAILURE(name, key_oplist, value_oplist)                 \
  ((M_LIB_ERROR(ARGUMENT_OF_DICT_OPLIST_IS_NOT_AN_OPLIST, name, key_oplist, value_oplist)))

/* Define the oplist of a dictionary
   NOTE: IT_REF is not exported so that the contained appears as not modifiable
   by algorithm.*/
#ifndef M_USE_CONTEXT
#define M_D1CT_OPLIST_P4(name, key_oplist, value_oplist)                      \
  (INIT(M_F(name, _init)),                                                    \
   INIT_SET(M_F(name, _init_set)),                                            \
   INIT_WITH(API_1(M_INIT_KEY_VAI)),                                          \
   SET(M_F(name, _set)),                                                      \
   CLEAR(M_F(name, _clear)),                                                  \
   INIT_MOVE(M_F(name, _init_move)),                                          \
   MOVE(M_F(name, _move)),                                                    \
   SWAP(M_F(name, _swap)),                                                    \
   RESET(M_F(name, _reset)),                                                  \
   NAME(name),                                                                \
   TYPE(M_F(name, _ct)), GENTYPE(struct M_F(name,_s)*),                       \
   SUBTYPE(M_F(name, _subtype_ct)),                                           \
   EMPTY_P(M_F(name,_empty_p)),                                               \
   IT_TYPE(M_F(name, _it_ct)),                                                \
   IT_FIRST(M_F(name,_it)),                                                   \
   IT_SET(M_F(name, _it_set)),                                                \
   IT_END(M_F(name,_it_end)),                                                 \
   IT_END_P(M_F(name,_end_p)),                                                \
   IT_LAST_P(M_F(name,_last_p)),                                              \
   IT_NEXT(M_F(name,_next)),                                                  \
   IT_CREF(M_F(name,_cref))                                                   \
   ,KEY_TYPE(M_F(name, _key_ct))                                              \
   ,VALUE_TYPE(M_F(name, _value_ct))                                          \
   ,SET_KEY(M_F(name, _set_at))                                               \
   ,GET_KEY(M_F(name, _get))                                                  \
   ,SAFE_GET_KEY(M_F(name, _safe_get))                                        \
   ,ERASE_KEY(M_F(name, _erase))                                              \
   ,KEY_OPLIST(key_oplist)                                                    \
   ,VALUE_OPLIST(value_oplist)                                                \
   ,GET_SIZE(M_F(name, _size))                                                \
   ,M_IF_METHOD_BOTH(GET_STR, key_oplist, value_oplist)(GET_STR(M_F(name, _get_str)),) \
   ,M_IF_METHOD_BOTH(PARSE_STR, key_oplist, value_oplist)(PARSE_STR(M_F(name, _parse_str)),) \
   ,M_IF_METHOD_BOTH(OUT_STR, key_oplist, value_oplist)(OUT_STR(M_F(name, _out_str)),) \
   ,M_IF_METHOD_BOTH(IN_STR, key_oplist, value_oplist)(IN_STR(M_F(name, _in_str)),) \
   ,M_IF_METHOD_BOTH(OUT_SERIAL, key_oplist, value_oplist)(OUT_SERIAL(M_F(name, _out_serial)),) \
   ,M_IF_METHOD_BOTH(IN_SERIAL, key_oplist, value_oplist)(IN_SERIAL(M_F(name, _in_serial)),) \
   ,M_IF_METHOD(EQUAL, value_oplist)(EQUAL(M_F(name, _equal_p)),)             \
   )
#else
#define M_D1CT_OPLIST_P4(name, key_oplist, value_oplist)                      \
  (INIT(API_0P(M_F(name, _init))),                                            \
   INIT_SET(API_0P(M_F(name, _init_set))),                                    \
   INIT_WITH(API_1(M_INIT_KEY_VAI)),                                          \
   SET(API_0P(M_F(name, _set))),                                              \
   CLEAR(API_0P(M_F(name, _clear))),                                          \
   INIT_MOVE(M_F(name, _init_move)),                                          \
   MOVE(API_0P(M_F(name, _move))),                                            \
   SWAP(M_F(name, _swap)),                                                    \
   RESET(API_0P(M_F(name, _reset))),                                          \
   NAME(name), TYPE(M_F(name, _ct)), GENTYPE(struct M_F(name,_s)*),           \
   SUBTYPE(M_F(name, _subtype_ct)),                                           \
   EMPTY_P(M_F(name,_empty_p)),                                               \
   IT_TYPE(M_F(name, _it_ct)),                                                \
   IT_FIRST(M_F(name,_it)),                                                   \
   IT_SET(M_F(name, _it_set)),                                                \
   IT_END(M_F(name,_it_end)),                                                 \
   IT_END_P(M_F(name,_end_p)),                                                \
   IT_LAST_P(M_F(name,_last_p)),                                              \
   IT_NEXT(M_F(name,_next)),                                                  \
   IT_CREF(M_F(name,_cref))                                                   \
   ,KEY_TYPE(M_F(name, _key_ct))                                              \
   ,VALUE_TYPE(M_F(name, _value_ct))                                          \
   ,SET_KEY(API_0P(M_F(name, _set_at)))                                       \
   ,GET_KEY(M_F(name, _get))                                                  \
   ,SAFE_GET_KEY(API_0P(M_F(name, _safe_get)))                                \
   ,ERASE_KEY(API_0P(M_F(name, _erase)))                                      \
   ,KEY_OPLIST(key_oplist)                                                    \
   ,VALUE_OPLIST(value_oplist)                                                \
   ,GET_SIZE(M_F(name, _size))                                                \
   ,M_IF_METHOD_BOTH(GET_STR, key_oplist, value_oplist)(API_0P(GET_STR(M_F(name, _get_str))),) \
   ,M_IF_METHOD_BOTH(PARSE_STR, key_oplist, value_oplist)(API_0P(PARSE_STR(M_F(name, _parse_str))),) \
   ,M_IF_METHOD_BOTH(OUT_STR, key_oplist, value_oplist)(OUT_STR(M_F(name, _out_str)),) \
   ,M_IF_METHOD_BOTH(IN_STR, key_oplist, value_oplist)(IN_STR(API_0P(M_F(name, _in_str))),) \
   ,M_IF_METHOD_BOTH(OUT_SERIAL, key_oplist, value_oplist)(OUT_SERIAL(API_0P(M_F(name, _out_serial))),) \
   ,M_IF_METHOD_BOTH(IN_SERIAL, key_oplist, value_oplist)(IN_SERIAL(API_0P(M_F(name, _in_serial))),) \
   ,M_IF_METHOD(EQUAL, value_oplist)(EQUAL(M_F(name, _equal_p)),)             \
   )
#endif

/* Deferred evaluation for the definition,
   so that all arguments are evaluated before further expansion */
#define M_D1CT_SET_OPLIST_P1(arg) M_D1CT_SET_OPLIST_P2 arg

/* Validation of the given oplist */
#define M_D1CT_SET_OPLIST_P2(name, oplist)                                    \
  M_IF_OPLIST(oplist)(M_D1CT_SET_OPLIST_P3, M_D1CT_SET_OPLIST_FAILURE)(name, oplist)

/* Prepare a clean compilation failure */
#define M_D1CT_SET_OPLIST_FAILURE(name, oplist)                               \
  ((M_LIB_ERROR(ARGUMENT_OF_DICT_SET_OPLIST_IS_NOT_AN_OPLIST, name, oplist)))

/* Define the oplist of a set
   Note: IT_REF is not exported so that the contained appears as not modifiable
*/
#ifndef M_USE_CONTEXT
#define M_D1CT_SET_OPLIST_P3(name, oplist)                                    \
  (INIT(M_F(name, _init)),                                                    \
   INIT_SET(M_F(name, _init_set)),                                            \
   INIT_WITH(API_1(M_INIT_VAI)),                                              \
   SET(M_F(name, _set)),                                                      \
   CLEAR(M_F(name, _clear)),                                                  \
   INIT_MOVE(M_F(name, _init_move)),                                          \
   MOVE(M_F(name, _move)),                                                    \
   SWAP(M_F(name, _swap)),                                                    \
   RESET(M_F(name, _reset)),                                                  \
   NAME(name),                                                                \
   TYPE(M_F(name, _ct)), GENTYPE(struct M_F(name,_s)*),                       \
   SUBTYPE(M_F(name, _subtype_ct)),                                           \
   EMPTY_P(M_F(name,_empty_p)),                                               \
   PUSH(M_F(name,_push)),                                                     \
   KEY_TYPE(M_F(name, _key_ct)),                                              \
   VALUE_TYPE(M_F(name, _key_ct)),                                            \
   GET_KEY(M_F(name, _get)),                                                  \
   SAFE_GET_KEY(M_F(name, _safe_get)),                                        \
   ERASE_KEY(M_F(name, _erase)),                                              \
   KEY_OPLIST(oplist),                                                        \
   VALUE_OPLIST(oplist),                                                      \
   GET_SIZE(M_F(name, _size)),                                                \
   IT_TYPE(M_F(name, _it_ct)),                                                \
   IT_FIRST(M_F(name,_it)),                                                   \
   IT_SET(M_F(name, _it_set)),                                                \
   IT_END(M_F(name,_it_end)),                                                 \
   IT_END_P(M_F(name,_end_p)),                                                \
   IT_LAST_P(M_F(name,_last_p)),                                              \
   IT_NEXT(M_F(name,_next)),                                                  \
   IT_CREF(M_F(name,_cref))                                                   \
   ,OPLIST(oplist)                                                            \
   ,M_IF_METHOD(GET_STR, oplist)(GET_STR(M_F(name, _get_str)),)               \
   ,M_IF_METHOD(PARSE_STR, oplist)(PARSE_STR(M_F(name, _parse_str)),)         \
   ,M_IF_METHOD(OUT_STR, oplist)(OUT_STR(M_F(name, _out_str)),)               \
   ,M_IF_METHOD(IN_STR, oplist)(IN_STR(M_F(name, _in_str)),)                  \
   ,M_IF_METHOD(OUT_SERIAL, oplist)(OUT_SERIAL(M_F(name, _out_serial)),)      \
   ,M_IF_METHOD(IN_SERIAL, oplist)(IN_SERIAL(M_F(name, _in_serial)),)         \
   ,EQUAL(M_F(name, _equal_p)),                                               \
   )
#else
#define M_D1CT_SET_OPLIST_P3(name, oplist)                                    \
  (INIT(API_0P(M_F(name, _init))),                                            \
   INIT_SET(API_0P(M_F(name, _init_set))),                                    \
   INIT_WITH(API_1(M_INIT_VAI)),                                              \
   SET(API_0P(M_F(name, _set))),                                              \
   CLEAR(API_0P(M_F(name, _clear))),                                          \
   INIT_MOVE(M_F(name, _init_move)),                                          \
   MOVE(API_0P(M_F(name, _move))),                                            \
   SWAP(M_F(name, _swap)),                                                    \
   RESET(API_0P(M_F(name, _reset))),                                          \
   NAME(name), TYPE(M_F(name, _ct)), GENTYPE(struct M_F(name,_s)*),           \
   SUBTYPE(M_F(name, _subtype_ct)),                                           \
   EMPTY_P(M_F(name,_empty_p)),                                               \
   PUSH(API_0P(M_F(name,_push))),                                             \
   KEY_TYPE(M_F(name, _key_ct)),                                              \
   VALUE_TYPE(M_F(name, _key_ct)),                                            \
   GET_KEY(M_F(name, _get)),                                                  \
   SAFE_GET_KEY(API_0P(M_F(name, _safe_get))),                                \
   ERASE_KEY(API_0P(M_F(name, _erase))),                                      \
   KEY_OPLIST(oplist),                                                        \
   VALUE_OPLIST(oplist),                                                      \
   GET_SIZE(M_F(name, _size)),                                                \
   IT_TYPE(M_F(name, _it_ct)),                                                \
   IT_FIRST(M_F(name,_it)),                                                   \
   IT_SET(M_F(name, _it_set)),                                                \
   IT_END(M_F(name,_it_end)),                                                 \
   IT_END_P(M_F(name,_end_p)),                                                \
   IT_LAST_P(M_F(name,_last_p)),                                              \
   IT_NEXT(M_F(name,_next)),                                                  \
   IT_CREF(M_F(name,_cref))                                                   \
   ,OPLIST(oplist)                                                            \
   ,M_IF_METHOD(GET_STR, oplist)(GET_STR(API_0P(M_F(name, _get_str))),)       \
   ,M_IF_METHOD(PARSE_STR, oplist)(PARSE_STR(API_0P(M_F(name, _parse_str))),) \
   ,M_IF_METHOD(OUT_STR, oplist)(OUT_STR(M_F(name, _out_str)),)               \
   ,M_IF_METHOD(IN_STR, oplist)(IN_STR(API_0P(M_F(name, _in_str))),)          \
   ,M_IF_METHOD(OUT_SERIAL, oplist)(OUT_SERIAL(API_0P(M_F(name, _out_serial))),) \
   ,M_IF_METHOD(IN_SERIAL, oplist)(IN_SERIAL(API_0P(M_F(name, _in_serial))),) \
   ,EQUAL(M_F(name, _equal_p)),                                               \
   )
#endif


/******************************** INTERNAL ***********************************/

enum m_d1ct_oa_element_e {
  M_D1CT_OA_EMPTY = 0, M_D1CT_OA_DELETED = 1
};

#ifdef NDEBUG
#define M_D1CT_OA_CONTRACT(dict)
#else
#define M_D1CT_OA_CONTRACT(dict) do {                                         \
    M_ASSERT ( (dict) != NULL);                                               \
    M_ASSERT( (dict)->lower_limit <= (dict)->count);                          \
    M_ASSERT( (dict)->count <= (dict)->upper_limit );                         \
    M_ASSERT( (dict)->data != NULL);                                          \
    M_ASSERT( M_POWEROF2_P((dict)->mask+1));                                  \
    M_ASSERT( (dict)->mask+1 >= M_D1CT_INITIAL_SIZE);                         \
    M_ASSERT( (dict)->upper_limit <= (dict)->mask+1);                         \
  } while (0)
#endif

#define M_D1CT_OA_DEF_P1(args) M_ID( M_D1CT_OA_DEF_P2 args )

/* Validate the key oplist before going further */
#define M_D1CT_OA_DEF_P2(name, key_type, key_oplist, value_type, value_oplist, dict_t, dict_it_t, it_deref_t) \
  M_IF_OPLIST(key_oplist)(M_D1CT_OA_DEF_P3, M_D1CT_OA_DEF_FAILURE)(name, key_type, key_oplist, value_type, value_oplist, dict_t, dict_it_t, it_deref_t)

/* Validate the value oplist before going further */
#define M_D1CT_OA_DEF_P3(name, key_type, key_oplist, value_type, value_oplist, dict_t, dict_it_t, it_deref_t) \
  M_IF_OPLIST(value_oplist)(M_D1CT_OA_DEF_P4, M_D1CT_OA_DEF_FAILURE)(name, key_type, key_oplist, value_type, value_oplist, dict_t, dict_it_t, it_deref_t)

/* Stop processing with a compilation failure */
#define M_D1CT_OA_DEF_FAILURE(name, key_type, key_oplist, value_type, value_oplist, dict_t, dict_it_t, it_deref_t) \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST, "(DICT_OA_DEF2): at least one of the given argument is not a valid oplist: " M_AS_STR(key_oplist) " / " M_AS_STR(value_oplist) )

#define M_D1CT_OA_DEF_P4(name, key_type, key_oplist, value_type, value_oplist, dict_t, dict_it_t, it_deref_t) \
  M_D1CT_OA_DEF_P5(name, key_type, key_oplist, value_type, value_oplist, 0,   \
                  M_D1CT_OA_LOWER_BOUND, M_D1CT_OA_UPPER_BOUND,               \
                  dict_t, dict_it_t, it_deref_t)

#define M_D1CT_OASET_DEF_P1(args) M_ID( M_D1CT_OASET_DEF_P2 args )

/* Validate the value oplist before going further */
#define M_D1CT_OASET_DEF_P2(name, key_type, key_oplist, dict_t, dict_it_t, it_deref_t) \
  M_IF_OPLIST(key_oplist)(M_D1CT_OASET_DEF_P4, M_D1CT_OASET_DEF_FAILURE)(name, key_type, key_oplist, dict_t, dict_it_t, it_deref_t)

/* Stop processing with a compilation failure */
#define M_D1CT_OASET_DEF_FAILURE(name, key_type, key_oplist, dict_t, dict_it_t, it_deref_t) \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST, "(DICT_OASET_DEF): the given argument is not a valid oplist: " M_AS_STR(key_oplist) )

#define M_D1CT_OASET_DEF_P4(name, key_type, key_oplist, dict_t, dict_it_t, it_deref_t) \
  M_D1CT_OA_DEF_P5(name, key_type, key_oplist, key_type, M_EMPTY_OPLIST, 1,   \
                  M_D1CT_OA_LOWER_BOUND, M_D1CT_OA_UPPER_BOUND, dict_t, dict_it_t, it_deref_t )

#define M_D1CT_OA_DEF_P5(name, key_type, key_oplist, value_type, value_oplist, isSet, coeff_down, coeff_up, dict_t, dict_it_t, it_deref_t) \
                                                                              \
  /* NOTE:                                                                    \
     if isSet is true, all methods of value_oplist are NOP methods */         \
                                                                              \
  typedef struct M_F(name, _pair_s) {                                         \
    key_type   key;                                                           \
    M_IF(isSet)( , value_type value;)                                         \
  } M_F(name, _pair_ct);                                                      \
                                                                              \
  /* Define type returned by the _ref method of an iterator */                \
  M_IF(isSet)(                                                                \
    typedef key_type it_deref_t;                                              \
  ,                                                                           \
    typedef struct M_F(name, _pair_s) it_deref_t;                             \
  )                                                                           \
                                                                              \
  M_CHECK_COMPATIBLE_OPLIST(name, 1, key_type, key_oplist)                    \
  M_CHECK_COMPATIBLE_OPLIST(name, 2, value_type, value_oplist)                \
                                                                              \
  /* NOTE: We don't want a real oplist for this sub type */                   \
  M_ARRAY_DEF(M_F(name, _array_pair), M_F(name, _pair_ct),                    \
            (INIT(M_NOTHING_DEFAULT), SET(M_SET_DEFAULT),                     \
             INIT_SET(M_SET_DEFAULT), CLEAR(M_NOTHING_DEFAULT)))              \
                                                                              \
  typedef struct M_F(name,_s) {                                               \
    size_t mask, count, count_delete;                                         \
    size_t upper_limit, lower_limit;                                          \
    struct M_F(name, _pair_s) *data;                                          \
  } dict_t[1];                                                                \
  typedef struct M_F(name, _s) *M_F(name, _ptr);                              \
  typedef const struct M_F(name, _s) *M_F(name, _srcptr);                     \
                                                                              \
  typedef struct M_F(name, _it_s) {                                           \
    const struct M_F(name,_s) *dict;                                          \
    size_t index;                                                             \
  } dict_it_t[1];                                                             \
                                                                              \
  /* Define internal types for oplist */                                      \
  typedef dict_t M_F(name, _ct);                                              \
  typedef it_deref_t M_F(name, _subtype_ct);                                  \
  typedef key_type M_F(name, _key_ct);                                        \
  typedef value_type M_F(name, _value_ct);                                    \
  typedef dict_it_t M_F(name, _it_ct);                                        \
  typedef size_t M_F(name, _index_ct);                                        \
                                                                              \
  M_INLINE void                                                               \
  M_C3(m_d1ct_,name,_update_limit)(dict_t dict, size_t size)                  \
  {                                                                           \
    /* FIXME: Overflow not handled. What to do in case of it? */              \
    dict->upper_limit = (size_t) ((double) size * coeff_up) - 1;              \
    dict->lower_limit = (size <= M_D1CT_INITIAL_SIZE) ? 0 : (size_t) ((double) size * coeff_down) ; \
    if (M_UNLIKELY(dict->count <= dict->lower_limit)) {                       \
      dict->lower_limit = 0;                                                  \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_P(void, name, _init, dict_t dict)                                         \
  {                                                                           \
    M_ASSERT(0 <= (coeff_down) && (coeff_down)*2 < (coeff_up) && (coeff_up) < 1); \
    dict->mask = M_D1CT_INITIAL_SIZE-1;                                       \
    dict->count = 0;                                                          \
    dict->count_delete = 0;                                                   \
    M_C3(m_d1ct_,name,_update_limit)(dict, M_D1CT_INITIAL_SIZE);              \
    dict->data = M_CALL_REALLOC(key_oplist, M_F(name, _pair_ct), NULL, 0, M_D1CT_INITIAL_SIZE); \
    if (M_UNLIKELY_NOMEM (dict->data == NULL)) {                              \
      M_MEMORY_FULL(M_F(name, _pair_ct), M_D1CT_INITIAL_SIZE);                \
    }                                                                         \
    /* Populate the initial table with the 'empty' representation */          \
    for(size_t i = 0; i < M_D1CT_INITIAL_SIZE; i++) {                         \
      M_CALL_OOR_SET(key_oplist, dict->data[i].key, M_D1CT_OA_EMPTY);         \
      M_ASSERT(M_CALL_OOR_EQUAL(key_oplist, dict->data[i].key, M_D1CT_OA_EMPTY)); \
    }                                                                         \
    M_D1CT_OA_CONTRACT(dict);                                                 \
  }                                                                           \
                                                                              \
  M_P(void, name, _clear, dict_t dict)                                        \
  {                                                                           \
    M_D1CT_OA_CONTRACT(dict);                                                 \
    for(size_t i = 0; i <= dict->mask; i++) {                                 \
      if (!M_CALL_OOR_EQUAL(key_oplist, dict->data[i].key, M_D1CT_OA_EMPTY)   \
          && !M_CALL_OOR_EQUAL(key_oplist, dict->data[i].key, M_D1CT_OA_DELETED)) { \
        M_CALL_CLEAR(key_oplist, dict->data[i].key);                          \
        M_CALL_CLEAR(value_oplist, dict->data[i].value);                      \
      }                                                                       \
    }                                                                         \
    M_CALL_FREE(key_oplist, M_F(name, _pair_ct), dict->data, dict->mask+1);   \
    /* Not really needed, but safer */                                        \
    dict->mask = 0;                                                           \
    dict->data = NULL;                                                        \
  }                                                                           \
                                                                              \
  M_INLINE value_type * M_ATTR_HOT_FUNCTION                                   \
  M_F(name, _get)(const dict_t dict, key_type const key)                      \
  {                                                                           \
    M_D1CT_OA_CONTRACT(dict);                                                 \
    /* NOTE: Key can not be the representation of empty or deleted */         \
    M_ASSERT (!M_CALL_OOR_EQUAL(key_oplist, key, M_D1CT_OA_EMPTY));           \
    M_ASSERT (!M_CALL_OOR_EQUAL(key_oplist, key, M_D1CT_OA_DELETED));         \
                                                                              \
    M_F(name, _pair_ct) *const data = dict->data;                             \
    const size_t mask = dict->mask;                                           \
    size_t p = M_CALL_HASH(key_oplist, key) & mask;                           \
    size_t s = 1;                                                             \
    while (true) {                                                            \
      /* Random access, and probably cache miss */                            \
      if (M_LIKELY (M_CALL_EQUAL(key_oplist, data[p].key, key)) )             \
        return &data[p].M_IF(isSet)(key, value);                              \
      if (M_LIKELY (M_CALL_OOR_EQUAL(key_oplist, data[p].key, M_D1CT_OA_EMPTY)) ) \
        return NULL;                                                          \
      p = (p + M_D1CT_OA_PROBING(s)) & mask;                                  \
      M_ASSERT (s <= dict->mask);                                             \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_INLINE value_type * M_ATTR_HOT_FUNCTION                                   \
  M_F(name, _prehashed_get)(const dict_t dict, key_type const key, size_t prehash) \
  {                                                                           \
    M_D1CT_OA_CONTRACT(dict);                                                 \
    M_ASSERT( prehash == M_CALL_HASH(key_oplist, key));                       \
    /* NOTE: Key can not be the representation of empty or deleted */         \
    M_ASSERT (!M_CALL_OOR_EQUAL(key_oplist, key, M_D1CT_OA_EMPTY));           \
    M_ASSERT (!M_CALL_OOR_EQUAL(key_oplist, key, M_D1CT_OA_DELETED));         \
                                                                              \
    M_F(name, _pair_ct) *const data = dict->data;                             \
    const size_t mask = dict->mask;                                           \
    size_t p = prehash & mask;                                                \
    size_t s = 1;                                                             \
    while (true) {                                                            \
      /* Random access, and probably cache miss */                            \
      if (M_LIKELY (M_CALL_EQUAL(key_oplist, data[p].key, key)) )             \
        return &data[p].M_IF(isSet)(key, value);                              \
      if (M_LIKELY (M_CALL_OOR_EQUAL(key_oplist, data[p].key, M_D1CT_OA_EMPTY)) ) \
        return NULL;                                                          \
      p = (p + M_D1CT_OA_PROBING(s)) & mask;                                  \
      M_ASSERT (s <= dict->mask);                                             \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_IF_DEBUG(                                                                 \
  M_INLINE bool                                                               \
  M_C3(m_d1ct_,name,_control_after_resize)(const dict_t h)                    \
  {                                                                           \
    /* This function checks if the reshashing of the dict is ok */            \
    M_F(name, _pair_ct) *data = h->data;                                      \
    size_t empty = 0;                                                         \
    size_t del = 0;                                                           \
    /* Count the number of empty elements and the number of deleted */        \
    for(size_t i = 0 ; i <= h->mask ; i++) {                                  \
      empty += M_CALL_OOR_EQUAL(key_oplist, data[i].key, M_D1CT_OA_EMPTY);    \
      del   += M_CALL_OOR_EQUAL(key_oplist, data[i].key, M_D1CT_OA_DELETED);  \
    }                                                                         \
    M_ASSERT(del == 0);                                                       \
    M_ASSERT(empty + h->count == h->mask + 1);                                \
    return true;                                                              \
  }                                                                           \
  )                                                                           \
                                                                              \
  M_P(void, name, _i_resize_up, dict_t h, size_t newSize, bool updateLimit)   \
  {                                                                           \
    size_t oldSize = h->mask+1;                                               \
    M_ASSERT (newSize >= oldSize);                                            \
    M_ASSERT (M_POWEROF2_P(newSize));                                         \
    M_F(name, _pair_ct) *data = h->data;                                      \
    /* resize can be called just to delete the items */                       \
    if (newSize > oldSize) {                                                  \
      data = M_CALL_REALLOC(key_oplist, M_F(name, _pair_ct), data, oldSize, newSize); \
      if (M_UNLIKELY_NOMEM (data == NULL) ) {                                 \
        M_MEMORY_FULL(M_F(name, _pair_ct), newSize);                          \
      }                                                                       \
                                                                              \
      /* First mark the extended space as empty */                            \
      M_ASSUME( oldSize + 8 < newSize);                                       \
      for(size_t i = oldSize ; i < newSize; i+=8) {                           \
        /* Unroll loop as compiler doesn't do it by itself */                 \
        M_CALL_OOR_SET(key_oplist, data[i+0].key, M_D1CT_OA_EMPTY);           \
        M_CALL_OOR_SET(key_oplist, data[i+1].key, M_D1CT_OA_EMPTY);           \
        M_CALL_OOR_SET(key_oplist, data[i+2].key, M_D1CT_OA_EMPTY);           \
        M_CALL_OOR_SET(key_oplist, data[i+3].key, M_D1CT_OA_EMPTY);           \
        M_CALL_OOR_SET(key_oplist, data[i+4].key, M_D1CT_OA_EMPTY);           \
        M_CALL_OOR_SET(key_oplist, data[i+5].key, M_D1CT_OA_EMPTY);           \
        M_CALL_OOR_SET(key_oplist, data[i+6].key, M_D1CT_OA_EMPTY);           \
        M_CALL_OOR_SET(key_oplist, data[i+7].key, M_D1CT_OA_EMPTY);           \
      }                                                                       \
    }                                                                         \
                                                                              \
    /* Then let's rehash all the entries in their **exact** position.         \
       If we can't, let's put them in the 'tmp' array.                        \
       It has been measured that the size of this 'tmp' array is              \
       around 6% of the size of updated dictionary.                           \
       NOTE: This should be much cache friendly than typical hash code        \
       Reserve a little bit of array to avoid reallocation if possible */     \
    M_F(name, _array_pair_ct) tmp;                                            \
    M_F(name, _array_pair_init)(tmp);                                         \
    M_F(name, _array_pair_reserve)M_R(tmp, oldSize >> 2);                     \
    const size_t mask = (newSize -1);                                         \
                                                                              \
    for(size_t i = 0 ; i < oldSize; i++) {                                    \
      if (!M_CALL_OOR_EQUAL(key_oplist, data[i].key, M_D1CT_OA_DELETED)       \
          && !M_CALL_OOR_EQUAL(key_oplist, data[i].key, M_D1CT_OA_EMPTY)) {   \
        size_t p = M_CALL_HASH(key_oplist, data[i].key) & mask;               \
        if (p != i) {                                                         \
          if (M_LIKELY (M_CALL_OOR_EQUAL(key_oplist, data[p].key, M_D1CT_OA_EMPTY) \
                      || M_CALL_OOR_EQUAL(key_oplist, data[p].key, M_D1CT_OA_DELETED))) { \
            M_CALL_INIT_MOVE(key_oplist, data[p].key, data[i].key);           \
            M_CALL_INIT_MOVE(value_oplist, data[p].value, data[i].value);     \
          } else {                                                            \
            M_F(name, _pair_ct) *ptr = M_F(name, _array_pair_push_back_raw)M_R(tmp); \
            M_CALL_INIT_MOVE(key_oplist, ptr->key, data[i].key);              \
            M_CALL_INIT_MOVE(value_oplist, ptr->value, data[i].value);        \
          }                                                                   \
          M_CALL_OOR_SET(key_oplist, data[i].key, M_D1CT_OA_EMPTY);           \
        }                                                                     \
      } else {                                                                \
        M_CALL_OOR_SET(key_oplist, data[i].key, M_D1CT_OA_EMPTY);             \
      }                                                                       \
    }                                                                         \
                                                                              \
    /* Let's put back the entries in the tmp array in their right place */    \
    /* NOTE: There should be very few entries in this array                   \
       which contains what we weren't be able to fit in the first pass */     \
    while (M_F(name, _array_pair_size)(tmp) > 0) {                            \
      M_F(name, _pair_ct) const *item = M_F(name, _array_pair_back)(tmp);     \
      size_t p = M_CALL_HASH(key_oplist, item->key) & mask;                   \
      size_t s = 1;                                                           \
      /* NOTE: since the first pass, the bucket might be free now */          \
      /* Likely cache miss */                                                 \
      while (!M_CALL_OOR_EQUAL(key_oplist, data[p].key, M_D1CT_OA_EMPTY)) {   \
        p = (p + M_D1CT_OA_PROBING(s)) & mask;                                \
        M_ASSERT (s <= h->mask);                                              \
      }                                                                       \
      M_F(name, _array_pair_pop_move)(&data[p], tmp);                         \
    }                                                                         \
                                                                              \
    M_F(name, _array_pair_clear)M_R(tmp);                                     \
    h->mask = newSize-1;                                                      \
    h->count_delete = h->count;                                               \
    if (updateLimit == true) {                                                \
      M_C3(m_d1ct_,name,_update_limit)(h, newSize);                           \
    }                                                                         \
    h->data = data;                                                           \
    M_IF_DEBUG (M_ASSERT (M_C3(m_d1ct_,name,_control_after_resize)(h));)      \
    M_D1CT_OA_CONTRACT(h);                                                    \
  }                                                                           \
                                                                              \
  M_IF(isSet)(                                                                \
    M_P(void, name, _push, dict_t dict, key_type const key) ,                 \
    M_P(void, name, _set_at, dict_t dict, key_type const key, value_type const value)) \
  {                                                                           \
    M_D1CT_OA_CONTRACT(dict);                                                 \
    /* NOTE: key can not be the representation of empty or deleted */         \
    M_ASSERT (!M_CALL_OOR_EQUAL(key_oplist, key, M_D1CT_OA_EMPTY));           \
    M_ASSERT (!M_CALL_OOR_EQUAL(key_oplist, key, M_D1CT_OA_DELETED));         \
                                                                              \
    M_F(name, _pair_ct) *const data = dict->data;                             \
    const size_t mask = dict->mask;                                           \
    size_t p = M_CALL_HASH(key_oplist, key) & mask;                           \
                                                                              \
    /* Test if bucket is not empty? (50% chance)  */                          \
    if (!M_CALL_OOR_EQUAL(key_oplist, data[p].key, M_D1CT_OA_EMPTY) ) {       \
      /* Find the insertion point as the bucket[] is not empty */             \
      size_t delPos = SIZE_MAX;                                               \
      size_t s = 1U;                                                          \
      do {                                                                    \
        if (M_UNLIKELY(M_CALL_EQUAL(key_oplist, data[p].key, key))) {         \
          M_CALL_SET(value_oplist, data[p].value, value);                     \
          return;                                                             \
        }                                                                     \
        M_ASSERT (s <= dict->mask);                                           \
        if (M_CALL_OOR_EQUAL(key_oplist, data[p].key, M_D1CT_OA_DELETED)      \
            && (delPos == (size_t)-1)) {                                      \
          delPos = p;                                                         \
        }                                                                     \
        p = (p + M_D1CT_OA_PROBING(s)) & mask;                                \
      } while (!M_CALL_OOR_EQUAL(key_oplist, data[p].key, M_D1CT_OA_EMPTY) ); \
      if (delPos != SIZE_MAX) {                                               \
        p = delPos;                                                           \
        dict->count_delete --;                                                \
      }                                                                       \
    }                                                                         \
                                                                              \
    M_CALL_INIT_SET(key_oplist, data[p].key, key);                            \
    M_CALL_INIT_SET(value_oplist, data[p].value, value);                      \
    dict->count++;                                                            \
    dict->count_delete ++;                                                    \
                                                                              \
    if (M_UNLIKELY (dict->count_delete >= dict->upper_limit)) {               \
      size_t newSize = dict->mask+1;                                          \
      if (dict->count > (dict->mask / 2)) {                                   \
        newSize += newSize;                                                   \
        if (M_UNLIKELY_NOMEM (newSize <= dict->mask+1)) {                     \
          M_MEMORY_FULL(char, -(size_t)1);                                    \
        }                                                                     \
      }                                                                       \
      M_F(name,_i_resize_up)M_R(dict, newSize, true);                         \
    }                                                                         \
    M_D1CT_OA_CONTRACT(dict);                                                 \
  }                                                                           \
                                                                              \
  M_P(value_type *, name,_safe_get, dict_t dict, key_type const key)          \
  {                                                                           \
    M_D1CT_OA_CONTRACT(dict);                                                 \
    /* NOTE: key can not be the representation of empty or deleted */         \
    M_ASSERT (!M_CALL_OOR_EQUAL(key_oplist, key, M_D1CT_OA_EMPTY));           \
    M_ASSERT (!M_CALL_OOR_EQUAL(key_oplist, key, M_D1CT_OA_DELETED));         \
                                                                              \
    M_F(name, _pair_ct) *const data = dict->data;                             \
    const size_t mask = dict->mask;                                           \
    size_t p = M_CALL_HASH(key_oplist, key) & mask;                           \
                                                                              \
    if (M_CALL_EQUAL(key_oplist, data[p].key, key))  {                        \
      return &data[p].M_IF(isSet)(key, value);                                \
    }                                                                         \
    if (M_UNLIKELY (!M_CALL_OOR_EQUAL(key_oplist, data[p].key, M_D1CT_OA_EMPTY) ) ) { \
      size_t delPos = SIZE_MAX;                                               \
      if (M_CALL_OOR_EQUAL(key_oplist, data[p].key, M_D1CT_OA_DELETED)) delPos = p; \
      size_t s = 1U;                                                          \
      do {                                                                    \
        p = (p + M_D1CT_OA_PROBING(s)) & mask;                                \
        if (M_CALL_EQUAL(key_oplist, data[p].key, key)) {                     \
          return &data[p].M_IF(isSet)(key, value);                            \
        }                                                                     \
        M_ASSERT (s <= dict->mask);                                           \
        if (M_CALL_OOR_EQUAL(key_oplist, data[p].key, M_D1CT_OA_DELETED)      \
            && (delPos == (size_t)-1)) {                                      \
          delPos = p;                                                         \
        }                                                                     \
      } while (!M_CALL_OOR_EQUAL(key_oplist, data[p].key, M_D1CT_OA_EMPTY) ); \
      if (delPos != SIZE_MAX) {                                               \
        p = delPos;                                                           \
        dict->count_delete --;                                                \
      }                                                                       \
    }                                                                         \
                                                                              \
    M_CALL_INIT_SET(key_oplist, data[p].key, key);                            \
    M_CALL_INIT(value_oplist, data[p].value);                                 \
    dict->count++;                                                            \
    dict->count_delete ++;                                                    \
                                                                              \
    if (M_UNLIKELY (dict->count_delete >= dict->upper_limit)) {               \
      size_t newSize = dict->mask+1;                                          \
      if (dict->count > (dict->mask / 2)) {                                   \
        newSize += newSize;                                                   \
        if (M_UNLIKELY_NOMEM (newSize <= dict->mask+1)) {                     \
          M_MEMORY_FULL(char, (size_t)-1);                                    \
        }                                                                     \
      }                                                                       \
      M_F(name,_i_resize_up)M_R(dict, newSize, true);                         \
      /* data is now invalid */                                               \
      return M_F(name, _get)(dict, key);                                      \
    }                                                                         \
    M_D1CT_OA_CONTRACT(dict);                                                 \
    return &data[p].M_IF(isSet)(key, value);                                  \
  }                                                                           \
                                                                              \
  M_P(void, name, _i_resize_down, dict_t h, size_t newSize)                   \
  {                                                                           \
    size_t oldSize = h->mask+1;                                               \
    M_ASSERT (newSize <= oldSize && M_POWEROF2_P(newSize));                   \
    if (M_UNLIKELY (newSize < M_D1CT_INITIAL_SIZE))                           \
      newSize = M_D1CT_INITIAL_SIZE;                                          \
    const size_t mask = newSize -1;                                           \
    M_F(name, _pair_ct) *data = h->data;                                      \
    M_F(name, _array_pair_ct) tmp;                                            \
    M_F(name, _array_pair_init)(tmp);                                         \
                                                                              \
    /* Pass 1: scan lower entries, and move them if needed */                 \
    for(size_t i = 0; i < newSize; i++) {                                     \
      if (M_CALL_OOR_EQUAL(key_oplist, data[i].key, M_D1CT_OA_EMPTY))         \
        continue;                                                             \
      if (M_CALL_OOR_EQUAL(key_oplist, data[i].key, M_D1CT_OA_DELETED)) {     \
        M_CALL_OOR_SET(key_oplist, data[i].key, M_D1CT_OA_EMPTY);             \
        continue;                                                             \
      }                                                                       \
      size_t p = M_CALL_HASH(key_oplist, data[i].key) & mask;                 \
      if (p != i) {                                                           \
        if (M_CALL_OOR_EQUAL(key_oplist, data[p].key, M_D1CT_OA_EMPTY)        \
            || M_CALL_OOR_EQUAL(key_oplist, data[p].key, M_D1CT_OA_DELETED)) { \
          M_CALL_INIT_MOVE(key_oplist, data[p].key, data[i].key);             \
          M_CALL_INIT_MOVE(value_oplist, data[p].value, data[i].value);       \
        } else {                                                              \
          M_F(name, _pair_ct) *ptr = M_F(name, _array_pair_push_back_raw)M_R(tmp); \
          M_CALL_INIT_MOVE(key_oplist, ptr->key, data[i].key);                \
          M_CALL_INIT_MOVE(value_oplist, ptr->value, data[i].value);          \
        }                                                                     \
        M_CALL_OOR_SET(key_oplist, data[i].key, M_D1CT_OA_EMPTY);             \
      }                                                                       \
    }                                                                         \
    /* Pass 2: scan upper entries and move them back */                       \
    for(size_t i = newSize; i < oldSize; i++) {                               \
      if (!M_CALL_OOR_EQUAL(key_oplist, data[i].key, M_D1CT_OA_DELETED)       \
          && !M_CALL_OOR_EQUAL(key_oplist, data[i].key, M_D1CT_OA_EMPTY)) {   \
        size_t p = M_CALL_HASH(key_oplist, data[i].key) & mask;               \
        M_ASSERT (p < i);                                                     \
        if (!M_CALL_OOR_EQUAL(key_oplist, data[p].key, M_D1CT_OA_EMPTY)) {    \
          size_t s = 1;                                                       \
          do {                                                                \
            p = (p + M_D1CT_OA_PROBING(s)) & mask;                            \
            M_ASSERT (s <= h->mask);                                          \
          } while (!M_CALL_OOR_EQUAL(key_oplist, data[p].key, M_D1CT_OA_EMPTY) ); \
        }                                                                     \
        M_CALL_INIT_MOVE(key_oplist, data[p].key, data[i].key);               \
        M_CALL_INIT_MOVE(value_oplist, data[p].value, data[i].value);         \
      }                                                                       \
    }                                                                         \
    /* Pass 3: scan moved entries and move them back */                       \
    while (M_F(name, _array_pair_size)(tmp) > 0) {                            \
      M_F(name, _pair_ct) const *item = M_F(name, _array_pair_back)(tmp);     \
      size_t p = M_CALL_HASH(key_oplist, item->key) & mask;                   \
      if (!M_CALL_OOR_EQUAL(key_oplist, data[p].key, M_D1CT_OA_EMPTY)) {      \
        size_t s = 1;                                                         \
        do {                                                                  \
          p = (p + M_D1CT_OA_PROBING(s)) & mask;                              \
          M_ASSERT (s <= h->mask);                                            \
        } while (!M_CALL_OOR_EQUAL(key_oplist, data[p].key, M_D1CT_OA_EMPTY) ); \
      }                                                                       \
      M_F(name, _array_pair_pop_move)(&data[p], tmp);                         \
    }                                                                         \
                                                                              \
    M_F(name, _array_pair_clear) M_R(tmp);                                    \
    h->count_delete = h->count;                                               \
    if (newSize != oldSize) {                                                 \
      h->mask = newSize-1;                                                    \
      M_C3(m_d1ct_,name,_update_limit)(h, newSize);                           \
      h->data = M_CALL_REALLOC(key_oplist, M_F(name, _pair_ct), data, oldSize, newSize); \
      M_ASSERT (h->data != NULL);                                             \
    }                                                                         \
    M_IF_DEBUG (M_ASSERT (M_C3(m_d1ct_,name,_control_after_resize)(h));)      \
    M_ASSERT (h->lower_limit < h->count && h->count < h->upper_limit);        \
    M_D1CT_OA_CONTRACT(h);                                                    \
  }                                                                           \
                                                                              \
  M_P(bool, name,_erase, dict_t dict, key_type const key)                     \
  {                                                                           \
    M_D1CT_OA_CONTRACT(dict);                                                 \
    /* NOTE: key can't be the representation of empty or deleted */           \
    M_ASSERT (!M_CALL_OOR_EQUAL(key_oplist, key, M_D1CT_OA_EMPTY));           \
    M_ASSERT (!M_CALL_OOR_EQUAL(key_oplist, key, M_D1CT_OA_DELETED));         \
                                                                              \
    M_F(name, _pair_ct) *const data = dict->data;                             \
    const size_t mask = dict->mask;                                           \
    size_t p = M_CALL_HASH(key_oplist, key) & mask;                           \
                                                                              \
    /* Random access, and probably cache miss */                              \
    if (M_UNLIKELY (!M_CALL_EQUAL(key_oplist, data[p].key, key)) ) {          \
      if (M_CALL_OOR_EQUAL(key_oplist, data[p].key, M_D1CT_OA_EMPTY))         \
        return false;                                                         \
      size_t s = 1;                                                           \
      do {                                                                    \
        p = (p + M_D1CT_OA_PROBING(s)) & mask;                                \
        if (M_CALL_OOR_EQUAL(key_oplist, data[p].key, M_D1CT_OA_EMPTY) )      \
          return false;                                                       \
        M_ASSERT (s <= dict->mask);                                           \
      } while (!M_CALL_EQUAL(key_oplist, data[p].key, key));                  \
    }                                                                         \
    M_CALL_CLEAR(key_oplist, data[p].key);                                    \
    M_CALL_CLEAR(value_oplist, data[p].value);                                \
    M_CALL_OOR_SET(key_oplist, data[p].key, M_D1CT_OA_DELETED);               \
    M_ASSERT (dict->count >= 1);                                              \
    dict->count--;                                                            \
    if (M_UNLIKELY (dict->count < dict->lower_limit)) {                       \
      M_F(name,_i_resize_down)M_R(dict, (dict->mask+1) >> 1);                 \
    }                                                                         \
    M_D1CT_OA_CONTRACT(dict);                                                 \
    return true;                                                              \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _init_move)(dict_t map, dict_t org)                               \
  {                                                                           \
    M_D1CT_OA_CONTRACT(org);                                                  \
    M_ASSERT (map != org);                                                    \
    map->mask         = org->mask;                                            \
    map->count        = org->count;                                           \
    map->count_delete = org->count_delete;                                    \
    map->upper_limit  = org->upper_limit;                                     \
    map->lower_limit  = org->lower_limit;                                     \
    map->data         = org->data;                                            \
    /* Mark org as cleared (safety) */                                        \
    org->mask         = 0;                                                    \
    org->data         = NULL;                                                 \
    M_D1CT_OA_CONTRACT(map);                                                  \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it)(dict_it_t it, const dict_t d)                                \
  {                                                                           \
    M_D1CT_OA_CONTRACT(d);                                                    \
    M_ASSERT (it != NULL);                                                    \
    it->dict = d;                                                             \
    size_t i = 0;                                                             \
    while (i <= d->mask                                                       \
        && (M_CALL_OOR_EQUAL(key_oplist, d->data[i].key, M_D1CT_OA_EMPTY)     \
          || M_CALL_OOR_EQUAL(key_oplist, d->data[i].key, M_D1CT_OA_DELETED))) { \
      i++;                                                                    \
    }                                                                         \
    it->index = i;                                                            \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_set)(dict_it_t it, const dict_it_t ref)                       \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    M_ASSERT (ref != NULL);                                                   \
    it->dict = ref->dict;                                                     \
    it->index = ref->index;                                                   \
    M_D1CT_OA_CONTRACT (it->dict);                                            \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_last)(dict_it_t it, const dict_t d)                           \
  {                                                                           \
    M_D1CT_OA_CONTRACT(d);                                                    \
    M_ASSERT (it != NULL);                                                    \
    it->dict = d;                                                             \
    size_t i = d->mask;                                                       \
    while (i <= d->mask                                                       \
        && (M_CALL_OOR_EQUAL(key_oplist, d->data[i].key, M_D1CT_OA_EMPTY)     \
         ||  M_CALL_OOR_EQUAL(key_oplist, d->data[i].key, M_D1CT_OA_DELETED))) { \
      i--;                                                                    \
    }                                                                         \
    it->index = i;                                                            \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _it_end)(dict_it_t it, const dict_t d)                            \
  {                                                                           \
    M_D1CT_OA_CONTRACT(d);                                                    \
    M_ASSERT (it != NULL);                                                    \
    it->dict = d;                                                             \
    it->index = d->mask+1;                                                    \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _end_p)(const dict_it_t it)                                       \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    M_D1CT_OA_CONTRACT (it->dict);                                            \
    return it->index > it->dict->mask;                                        \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _next)(dict_it_t it)                                              \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    M_D1CT_OA_CONTRACT (it->dict);                                            \
    size_t i = it->index;                                                     \
    do {                                                                      \
      i++;                                                                    \
    } while (M_LIKELY (i <= it->dict->mask) &&                                \
             M_UNLIKELY ((M_CALL_OOR_EQUAL(key_oplist, it->dict->data[i].key, M_D1CT_OA_EMPTY) \
                          || M_CALL_OOR_EQUAL(key_oplist, it->dict->data[i].key, M_D1CT_OA_DELETED)))) ; \
    it->index = i;                                                            \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _previous)(dict_it_t it)                                          \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    M_D1CT_OA_CONTRACT (it->dict);                                            \
    /* if index was 0, the operation will overflow, and stops the loop */     \
    size_t i = it->index - 1;                                                 \
    while (i <= it->dict->mask &&                                             \
           (M_CALL_OOR_EQUAL(key_oplist, it->dict->data[i].key, M_D1CT_OA_EMPTY) \
            || M_CALL_OOR_EQUAL(key_oplist, it->dict->data[i].key, M_D1CT_OA_DELETED))) { \
      i--;                                                                    \
    }                                                                         \
    it->index = i;                                                            \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _last_p)(const dict_it_t it)                                      \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    dict_it_t it2;                                                            \
    M_F(name,_it_set)(it2, it);                                               \
    M_F(name, _next)(it2);                                                    \
    return M_F(name, _end_p)(it2);                                            \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _it_equal_p)(const dict_it_t it1,const dict_it_t it2)             \
  {                                                                           \
    M_ASSERT (it1 != NULL && it2 != NULL);                                    \
    M_D1CT_OA_CONTRACT (it1->dict);                                           \
    M_D1CT_OA_CONTRACT (it2->dict);                                           \
    return it1->dict == it2->dict && it1->index == it2->index;                \
  }                                                                           \
                                                                              \
  M_INLINE it_deref_t *                                                       \
  M_F(name, _ref)(const dict_it_t it)                                         \
  {                                                                           \
    M_ASSERT (it != NULL);                                                    \
    M_ASSERT(!M_F(name, _end_p)(it));                                         \
    M_D1CT_OA_CONTRACT (it -> dict);                                          \
    const size_t i = it->index;                                               \
    M_ASSERT (i <= it->dict->mask);                                           \
    M_ASSERT (!M_CALL_OOR_EQUAL(key_oplist, it->dict->data[i].key, M_D1CT_OA_EMPTY)); \
    M_ASSERT (!M_CALL_OOR_EQUAL(key_oplist, it->dict->data[i].key, M_D1CT_OA_DELETED)); \
    return &it->dict->data[i] M_IF(isSet)(.key, );                            \
  }                                                                           \
                                                                              \
  M_INLINE const  it_deref_t *                                                \
  M_F(name, _cref)(const dict_it_t it)                                        \
  {                                                                           \
    return M_CONST_CAST(it_deref_t, M_F(name, _ref)(it));                     \
  }                                                                           \
                                                                              \
  M_D1CT_FUNC_ADDITIONAL_DEF2(name, key_type, key_oplist, value_type, value_oplist, isSet, dict_t, dict_it_t, it_deref_t) \
  M_IF(isSet)(M_EAT, M_D1CT_OA_DEF_BULK)(name, key_type, key_oplist, value_type, value_oplist, isSet, coeff_down, coeff_up, dict_t, dict_it_t, it_deref_t)


// Number of loads to perform in advance
#ifndef M_USE_MAX_PREFETCH
#define M_USE_MAX_PREFETCH 16
#endif

// WIP. Only if isSet is false for the time being
#define M_D1CT_OA_DEF_BULK(name, key_type, key_oplist, value_type, value_oplist, isSet, coeff_down, coeff_up, dict_t, dict_it_t, it_deref_t) \
                                                                              \
M_P(void, name, _bulk_get, unsigned n, value_type val[M_VLA(n)], dict_t dict, key_type const key[M_VLA(n)], value_type def) \
{                                                                             \
  M_D1CT_OA_CONTRACT (dict);                                                  \
  M_UNUSED_CONTEXT();                                                         \
  size_t h[M_USE_MAX_PREFETCH];                                               \
  M_F(name, _pair_ct) *data = dict->data;                                     \
  const size_t mask = dict->mask;                                             \
  const unsigned num = M_MIN(M_USE_MAX_PREFETCH, n);                          \
  for(unsigned i = 0 ; i < num; i++) {                                        \
    h[i] = M_CALL_HASH(key_oplist, key[i]) & mask;                            \
    M_PREFETCH(&data[h[i]]);                                                  \
  }                                                                           \
  for (unsigned i = 0; i < n; i++) {                                          \
    size_t s = 1, p = h[i % M_USE_MAX_PREFETCH];                              \
    if (i+M_USE_MAX_PREFETCH < n) {                                           \
      h[i % M_USE_MAX_PREFETCH] = M_CALL_HASH(key_oplist, key[i+M_USE_MAX_PREFETCH]) & mask; \
      M_PREFETCH(&data[h[i % M_USE_MAX_PREFETCH]]);                           \
    }                                                                         \
    while (true) {                                                            \
      if (M_LIKELY (M_CALL_EQUAL(key_oplist, data[p].key, key[i]))) {         \
        M_CALL_SET(value_oplist, val[i], data[p].value);                      \
        break;                                                                \
      }                                                                       \
      if (M_LIKELY (M_CALL_OOR_EQUAL (key_oplist, data[p].key, M_D1CT_OA_EMPTY)) ) { \
        M_CALL_SET(value_oplist, val[i], def);                                \
        break;                                                                \
      }                                                                       \
      p = (p + M_D1CT_OA_PROBING(s)) & mask;                                  \
      M_ASSERT(p <= mask);                                                    \
    }                                                                         \
  }                                                                           \
}                                                                             \
                                                                              \
M_P(void, name, _bulk_set, dict_t dict, unsigned n, value_type const val[M_VLA(n)], key_type const key[M_VLA(n)]) \
{                                                                             \
  M_D1CT_OA_CONTRACT (dict);                                                  \
  size_t h[M_USE_MAX_PREFETCH];                                               \
  const unsigned num = M_MIN(M_USE_MAX_PREFETCH, n);                          \
  for(unsigned i = 0 ; i < num; i++) {                                        \
    h[i] = M_CALL_HASH(key_oplist, key[i]);                                   \
    M_PREFETCH(&dict->data[h[i] & dict->mask]);                               \
  }                                                                           \
                                                                              \
  for (unsigned i = 0; i < n; i++) {                                          \
    size_t p = h[i % M_USE_MAX_PREFETCH] & dict->mask;                        \
    if (i+M_USE_MAX_PREFETCH < n) {                                           \
      h[i % M_USE_MAX_PREFETCH] = M_CALL_HASH(key_oplist, key[i+M_USE_MAX_PREFETCH]); \
      M_PREFETCH(&dict->data[h[i % M_USE_MAX_PREFETCH] & dict->mask]);        \
    }                                                                         \
    if (M_UNLIKELY (M_CALL_EQUAL(key_oplist, dict->data[p].key, key[i]))) {   \
      M_CALL_SET(value_oplist, dict->data[p].value, val[i]);                  \
      continue;                                                               \
    }                                                                         \
    /* Search loop for insertion */                                           \
    size_t s = 1;                                                             \
    size_t delPos = SIZE_MAX;                                                 \
    while (M_UNLIKELY (!M_CALL_OOR_EQUAL (key_oplist, dict->data[p].key, M_D1CT_OA_EMPTY)) ) { \
      if (M_CALL_EQUAL(key_oplist,  dict->data[p].key, key[i])) {             \
        M_CALL_SET(value_oplist, dict->data[p].value, val[i]);                \
        goto next;                                                            \
      }                                                                       \
      if (M_CALL_OOR_EQUAL (key_oplist, dict->data[p].key, M_D1CT_OA_DELETED) \
          && (delPos == SIZE_MAX))                                            \
        delPos = p;                                                           \
      p = (p + M_D1CT_OA_PROBING(s)) & dict->mask;                            \
    }                                                                         \
    /* Add new item */                                                        \
    if (delPos != SIZE_MAX) {                                                 \
      p = delPos;                                                             \
      dict->count_delete --;                                                  \
    }                                                                         \
    M_CALL_INIT_SET(key_oplist, dict->data[p].key, key[i]);                   \
    M_CALL_INIT_SET(value_oplist, dict->data[p].value, val[i]);               \
    dict->count++;                                                            \
    dict->count_delete ++;                                                    \
                                                                              \
    if (M_UNLIKELY (dict->count_delete >= dict->upper_limit)) {               \
      size_t newSize = dict->mask+1;                                          \
      if (dict->count > (dict->mask / 2)) {                                   \
        newSize += newSize;                                                   \
        if (M_UNLIKELY_NOMEM (newSize <= dict->mask+1)) {                     \
          M_MEMORY_FULL(char, (size_t)-1);                                    \
        }                                                                     \
      }                                                                       \
      M_F(name,_i_resize_up)M_R(dict, newSize, true);                         \
    }                                                                         \
  next:                                                                       \
    (void)0;                                                                  \
  }                                                                           \
  M_D1CT_OA_CONTRACT (dict);                                                  \
}                                                                             \
                                                                              \
                                                                              \
M_P(void, name, _bulk_update, dict_t dict, unsigned n, value_type val[M_VLA(n)], key_type const key[M_VLA(n)], value_type def, \
                        void (*update)(dict_t dict, key_type const key, value_type *storedval, value_type *newval)) \
{                                                                             \
  M_D1CT_OA_CONTRACT (dict);                                                  \
  size_t h[M_USE_MAX_PREFETCH];                                               \
  const unsigned num = M_MIN(M_USE_MAX_PREFETCH, n);                          \
  for(unsigned i = 0 ; i < num; i++) {                                        \
    h[i] = M_CALL_HASH(key_oplist, key[i]);                                   \
    M_PREFETCH(&dict->data[h[i] & dict->mask]);                               \
  }                                                                           \
                                                                              \
  for (unsigned i = 0; i < n; i++) {                                          \
    size_t p = h[i % M_USE_MAX_PREFETCH] & dict->mask;                        \
    if (i+M_USE_MAX_PREFETCH < n) {                                           \
      h[i % M_USE_MAX_PREFETCH] = M_CALL_HASH(key_oplist, key[i+M_USE_MAX_PREFETCH]); \
      M_PREFETCH(&dict->data[h[i % M_USE_MAX_PREFETCH] & dict->mask]);        \
    }                                                                         \
    if (M_UNLIKELY (M_CALL_EQUAL(key_oplist, dict->data[p].key, key[i]))) {   \
      update (dict, key[i], &dict->data[p].value, &val[i]);                   \
      continue;                                                               \
    }                                                                         \
    /* Search loop for insertion */                                           \
    size_t s = 1;                                                             \
    size_t delPos = SIZE_MAX;                                                 \
    while (M_UNLIKELY (!M_CALL_OOR_EQUAL (key_oplist, dict->data[p].key, M_D1CT_OA_EMPTY)) ) { \
      if (M_CALL_EQUAL(key_oplist,  dict->data[p].key, key[i])) {             \
      update (dict, key[i], &dict->data[p].value, &val[i]);                   \
        goto next;                                                            \
      }                                                                       \
      if (M_CALL_OOR_EQUAL (key_oplist, dict->data[p].key, M_D1CT_OA_DELETED) \
          && (delPos == SIZE_MAX))                                            \
        delPos = p;                                                           \
      p = (p + M_D1CT_OA_PROBING(s)) & dict->mask;                            \
    }                                                                         \
    /* Add new item */                                                        \
    if (delPos != SIZE_MAX) {                                                 \
      p = delPos;                                                             \
      dict->count_delete --;                                                  \
    }                                                                         \
    M_CALL_INIT_SET(key_oplist, dict->data[p].key, key[i]);                   \
    M_CALL_INIT_SET(value_oplist, dict->data[p].value, def);                  \
    update (dict, key[i], &dict->data[p].value, &val[i]);                     \
    dict->count++;                                                            \
    dict->count_delete ++;                                                    \
                                                                              \
    if (M_UNLIKELY (dict->count_delete >= dict->upper_limit)) {               \
      size_t newSize = dict->mask+1;                                          \
      if (dict->count > (dict->mask / 2)) {                                   \
        newSize += newSize;                                                   \
        if (M_UNLIKELY_NOMEM (newSize <= dict->mask+1)) {                     \
          M_MEMORY_FULL(char, (size_t)-1);                                    \
        }                                                                     \
      }                                                                       \
      M_F(name,_i_resize_up)M_R(dict, newSize, true);                         \
    }                                                                         \
  next:                                                                       \
    (void)0;                                                                  \
  }                                                                           \
  M_D1CT_OA_CONTRACT (dict);                                                  \
}                                                                             \

/******************************** INTERNAL ***********************************/

#if M_USE_SMALL_NAME
#define DICT_DEF2 M_DICT_DEF2
#define DICT_DEF2_AS M_DICT_DEF2_AS
#define DICT_OA_DEF2 M_DICT_OA_DEF2
#define DICT_OA_DEF2_AS M_DICT_OA_DEF2_AS
#define DICT_SET_DEF M_DICT_SET_DEF
#define DICT_SET_DEF_AS M_DICT_SET_DEF_AS
#define DICT_OASET_DEF M_DICT_OASET_DEF
#define DICT_OASET_DEF_AS M_DICT_OASET_DEF_AS
#define DICT_OPLIST M_DICT_OPLIST
#define DICT_SET_OPLIST M_DICT_SET_OPLIST
#endif

#endif
