/*
 * M*LIB - Shared Pointer container providing encapsulation and protected concurrent access.
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
#ifndef MSTARLIB_SHARED_PTR2_H
#define MSTARLIB_SHARED_PTR2_H

#include "m-core.h"
#include "m-atomic.h"
#include "m-thread.h"

/* There are two kinds of shared pointer:
 * * weak shared pointer (support only one thread)
 * * shared pointer (support for multiple threads both for the counter and lock for the data)
 *
 * There are 3 flavors:
 * * DECL : for header files, declare only the functions.
 * * DEF_EXTERN: for source files, define the functions (to be used with DECL)
 * * DEF: for header/source files, define the function as static inline.
 * 
 * You can provide or not the name of the shared pointer (_AS)
 * 
 * Therefore we got 12 macros.
 * 
 * There are 2 oplists:
 * * one to handle the shared pointer itself (ie.a copy create a new pointer to the data),
 * * one to handle the data behind the shared pointer (ie. a copy create a new data)
*/

/* Declare a shared weak pointer (not atomic, single thread) for use in header file.
   oplist is mandatory but is only used to test if a function has to be declared */
#define M_SHARED_WEAK_PTR_DECL(name, oplist)                                  \
    M_SHAR3D_PTR_DECL_P1((name, M_F(name, _t), oplist))

/* Declare a shared weak pointer (not atomic, single thread) for use in header file.
  oplist is mandatory but is only used to test if a function has to be declared */
#define M_SHARED_WEAK_PTR_DECL_AS(name, shared_t, oplist)                     \
    M_SHAR3D_PTR_DECL_P1((name, shared_t, oplist))

/* Declare a shared strong pointer (atomic & lock) for use in header file.
  oplist is mandatory but is only used to test if a function has to be declared */
#define M_SHARED_PTR_DECL(name, oplist)                                       \
    M_SHAR3D_PTR_DECL_P1((name, M_F(name, _t), oplist))

/* Declare a shared strong pointer (atomic & lock) for use in header file.
  oplist is mandatory but is only used to test if a function has to be declared */
#define M_SHARED_PTR_DECL_AS(name, shared_t, oplist)                          \
    M_SHAR3D_PTR_DECL_P1((name, shared_t, oplist))


/* Define an external definition a shared weak pointer (not atomic). oplist is optional */
#define M_SHARED_WEAK_PTR_DEF_EXTERN(name, ...)                               \
  M_SHARED_WEAK_PTR_DEF_EXTERN_AS(name, M_F(name, _t), __VA_ARGS__)

/* Define an external definition a shared weak pointer (not atomic). oplist is optional */
#define M_SHARED_WEAK_PTR_DEF_EXTERN_AS(name, shared_t, ...)                  \
  M_SHAR3D_WEAK_PTR_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                        \
    ((name, shared_t, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), /*extern*/ ), \
     (name, shared_t, __VA_ARGS__,                                        /*extern*/ )))

/* Define an external definition a shared strong pointer (atomic & lock). oplist is optional */
#define M_SHARED_PTR_DEF_EXTERN(name, ...)                                    \
  M_SHARED_PTR_DEF_EXTERN_AS(name, M_F(name, _t), __VA_ARGS__)

/* Define an external definition a shared strong pointer (atomic & lock). oplist is optional */
#define M_SHARED_PTR_DEF_EXTERN_AS(name, shared_t, ...)                       \
  M_SHAR3D_PTR_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                             \
    ((name, shared_t, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), /*extern*/ ), \
     (name, shared_t, __VA_ARGS__,                                        /*extern*/ )))


/* Define a static inline a shared weak pointer (not atomic). oplist is optional */
#define M_SHARED_WEAK_PTR_DEF(name, ...)                                      \
  M_SHARED_WEAK_PTR_DEF_AS(name, M_F(name, _t), __VA_ARGS__)

/* Define a static inline a shared weak pointer (not atomic). oplist is optional */
#define M_SHARED_WEAK_PTR_DEF_AS(name, shared_t, ...)                         \
  M_SHAR3D_PTR_DECL_TYPE(name, shared_t,       )                              \
  M_SHAR3D_WEAK_PTR_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                        \
    ((name, shared_t, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), static inline ), \
     (name, shared_t, __VA_ARGS__,                                        static inline )))

/* Define a static inline a shared strong pointer (atomic & lock). oplist is optional */
#define M_SHARED_PTR_DEF(name, ...)                                           \
  M_SHARED_PTR_DEF_AS(name, M_F(name, _t), __VA_ARGS__)

/* Define a static inline a shared strong pointer (atomic & lock). oplist is optional */
#define M_SHARED_PTR_DEF_AS(name, shared_t, ...)                              \
  M_SHAR3D_PTR_DECL_TYPE(name, shared_t,       )                              \
  M_SHAR3D_PTR_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                             \
    ((name, shared_t, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(), static inline ), \
     (name, shared_t, __VA_ARGS__,                                        static inline )))


/* Define the oplist of a shared pointer given its name and its oplist.
   Oplist is mandatory.
   USAGE: SHARED_PTR_OPLIST(name, oplist of the type) */
#define M_SHARED_PTR_OPLIST(...)                                              \
  M_SHAR3D_OPLIST_P1 ((__VA_ARGS__ ))

/* Define the oplist of a shared pointer as a shared data given its name and its oplist.
   Oplist is mandatory.
   USAGE: SHARED_DATA_OPLIST(name, oplist of the type) */
#define M_SHARED_DATA_OPLIST(...)                                             \
  M_SHAR3D_DATA_OPLIST_P1 ((__VA_ARGS__ ))


/*****************************************************************************/
/********************************** INTERNAL *********************************/
/*****************************************************************************/

/* Deferred evaluation for the oplist definition,
   so that all arguments are evaluated before further expansion */
#define M_SHAR3D_OPLIST_P1(arg) M_SHAR3D_OPLIST_P2 arg

/* Validation of the given oplist */
#define M_SHAR3D_OPLIST_P2(name, oplist)                                      \
  M_IF_OPLIST(oplist)(M_SHAR3D_OPLIST_P3, M_SHAR3D_OPLIST_FAILURE)(name, oplist)

/* Prepare a clean compilation failure */
#define M_SHAR3D_OPLIST_FAILURE(name, oplist)                                 \
  ((M_LIB_ERROR(ARGUMENT_OF_SHARED_PTR_OPLIST_IS_NOT_AN_OPLIST, name, oplist)))

/* OPLIST definition of a shared pointer */
#define M_SHAR3D_OPLIST_P3(name, oplist)                                      \
  (NAME(name), TYPE(struct M_C(name,_s) *)                                    \
   ,M_IF_METHOD(INIT, oplist)(INIT(API_1(M_SHAR3D_NEW_WRAPPER)),)             \
   ,INIT_SET(API_4(M_F(name, _acquire)))                                      \
   ,SET(API_2(M_F(name, _set)))                                               \
   ,CLEAR(M_F(name, _release))                                                \
   ,INIT_MOVE(M_SET_DEFAULT)                                                  \
   ,MOVE(M_SET_DEFAULT)                                                       \
   )

/* Deferred evaluation for the oplist definition,
   so that all arguments are evaluated before further expansion */
#define M_SHAR3D_DATA_OPLIST_P1(arg) M_SHAR3D_DATA_OPLIST_P2 arg

/* Validation of the given oplist */
#define M_SHAR3D_DATA_OPLIST_P2(name, oplist)                                 \
  M_IF_OPLIST(oplist)(M_SHAR3D_DATA_OPLIST_P3, M_SHAR3D_DATA_OPLIST_FAILURE)(name, oplist)

/* Prepare a clean compilation failure */
#define M_SHAR3D_DATA_OPLIST_FAILURE(name, oplist)                            \
  ((M_LIB_ERROR(ARGUMENT_OF_SHARED_DATA_OPLIST_IS_NOT_AN_OPLIST, name, oplist)))

/* OPLIST definition of the shared data of a shared pointer */
#define M_SHAR3D_DATA_OPLIST_P3(name, oplist)                                 \
  (NAME(name), TYPE(struct M_C(name,_s) *)                                    \
   ,M_IF_METHOD(INIT, oplist)(INIT(API_1(M_SHAR3D_NEW_WRAPPER)),)             \
   ,M_IF_METHOD(INIT_SET, oplist)(INIT_SET(API_4(M_F(name, _clone))),)        \
   ,M_IF_METHOD(SET, oplist)(SET(API_4(M_F(name, _copy))),)                   \
   ,CLEAR(M_F(name, _clear))                                                  \
   ,INIT_MOVE(M_SET_DEFAULT)                                                  \
   ,MOVE(M_SET_DEFAULT)                                                       \
   )

// Wrapper for _new
// API_4 isn't fully working since there is only one output argument.
#define M_SHAR3D_NEW_WRAPPER(oplist, x) ((x) = M_F(M_GET_NAME oplist, _new)())


/* Validation of the given oplist */
#define M_SHAR3D_PTR_DECL_P1(arg) M_ID( M_SHAR3D_PTR_DECL_P2 arg )

#define M_SHAR3D_PTR_DECL_P2(name, shared_t, oplist)                          \
  M_IF_OPLIST(oplist)(M_SHAR3D_PTR_DECL_P3, M_SHAR3D_PTR_DECL_FAILURE)(name, shared_t, oplist)

/* Prepare a clean compilation failure */
#define M_SHAR3D_PTR_DECL_FAILURE(name, shared_t, oplist)                     \
  ((M_LIB_ERROR(ARGUMENT_OF_DECL_SHARED_POINTER_OPLIST_IS_NOT_AN_OPLIST, name, oplist)))

/* Declare all functions associated to the shared pointer */
#define M_SHAR3D_PTR_DECL_P3(name, shared_t, oplist)                          \
    M_BEGIN_PROTECTED_CODE                                                    \
    M_SHAR3D_PTR_DECL_TYPE(name, shared_t, oplist)                            \
    M_SHAR3D_PTR_DECL_BASIC(name, shared_t, oplist)                           \
    M_SHAR3D_PTR_DECL_EXTRA(name, shared_t, oplist)                           \
    M_SHAR3D_PTR_DECL_ARITH(name, shared_t, oplist)                           \
    M_SHAR3D_PTR_DECL_KEY(name, shared_t, oplist, M_GET_KEY_TYPE oplist, M_GET_VALUE_TYPE oplist) \
    M_SHAR3D_PTR_DECL_PUSH(name, shared_t, oplist, M_GET_SUBTYPE oplist)      \
    M_SHAR3D_PTR_DECL_POP(name, shared_t, oplist, M_GET_SUBTYPE oplist)       \
    M_SHAR3D_PTR_DECL_IT(name, shared_t, oplist, M_GET_SUBTYPE oplist)        \
    M_SHAR3D_PTR_DECL_IO(name, shared_t, oplist)                              \
    M_END_PROTECTED_CODE


/* Deferred evaluation for the oplist definition,
   so that all arguments are evaluated before further expansion */
#define M_SHAR3D_WEAK_PTR_DEF_P1(arg) M_ID( M_SHAR3D_WEAK_PTR_DEF_P2 arg )

/* Validation of the given oplist */
#define M_SHAR3D_WEAK_PTR_DEF_P2(name, shared_t, type, oplist, fattr)         \
  M_IF_OPLIST(oplist)(M_SHAR3D_WEAK_PTR_DEF_P3, M_SHAR3D_WEAK_PTR_DEF_FAILURE)(name, shared_t, type, oplist, fattr)

/* Prepare a clean compilation failure */
#define M_SHAR3D_WEAK_PTR_DEF_FAILURE(name, shared_t, type, oplist, fattr)    \
  ((M_LIB_ERROR(ARGUMENT_OF_WEAK_DEF_SHARED_POINTER_OPLIST_IS_NOT_AN_OPLIST, name, oplist)))

/* Define all functions associated to the weak shared pointer as 'fattr' (can be static inline or extern) */
#define M_SHAR3D_WEAK_PTR_DEF_P3(name, shared_t, type, oplist, fattr)         \
    M_BEGIN_PROTECTED_CODE                                                    \
    M_SHAR3D_PTR_NO_THRD_DEF_TYPE(name, shared_t, type, oplist)               \
    M_SHAR3D_PTR_NO_THRD_DEF_CORE(name, shared_t, type, oplist, fattr)        \
    M_SHAR3D_PTR_DEF_BASIC(name, shared_t, type, oplist, fattr)               \
    M_SHAR3D_PTR_DEF_EXTRA(name, shared_t, type, oplist, fattr)               \
    M_SHAR3D_PTR_DEF_ARITH(name, shared_t, type, oplist, fattr)               \
    M_SHAR3D_PTR_DEF_KEY(name, shared_t, type, oplist, fattr, M_GET_KEY_TYPE oplist, M_GET_KEY_OPLIST oplist, M_GET_VALUE_TYPE oplist, M_GET_VALUE_OPLIST oplist) \
    M_SHAR3D_PTR_DEF_PUSH(name, shared_t, type, oplist, fattr, M_GET_SUBTYPE oplist, M_GET_OPLIST oplist) \
    M_SHAR3D_PTR_DEF_POP(name, shared_t, type, oplist, fattr, M_GET_SUBTYPE oplist, M_GET_OPLIST oplist) \
    M_SHAR3D_PTR_DEF_IT(name, shared_t, type, oplist, fattr, M_GET_SUBTYPE oplist, M_GET_OPLIST oplist) \
    M_SHAR3D_PTR_DEF_IO(name, shared_t, type, oplist, fattr)                  \
    M_END_PROTECTED_CODE


/* Deferred evaluation for the oplist definition,
   so that all arguments are evaluated before further expansion */
#define M_SHAR3D_PTR_DEF_P1(arg) M_ID( M_SHAR3D_PTR_DEF_P2 arg )

/* Validation of the given oplist */
#define M_SHAR3D_PTR_DEF_P2(name, shared_t, type, oplist, fattr)              \
  M_IF_OPLIST(oplist)(M_SHAR3D_PTR_DEF_P3, M_SHAR3D_PTR_DEF_FAILURE)(name, shared_t, type, oplist, fattr)

/* Prepare a clean compilation failure */
#define M_SHAR3D_PTR_DEF_FAILURE(name, shared_t, type, oplist, fattr)         \
  ((M_LIB_ERROR(ARGUMENT_OF_DEF_SHARED_POINTER_OPLIST_IS_NOT_AN_OPLIST, name, oplist)))

/* Define all functions associated to the shared pointer as 'fattr' (can be static inline or extern) */
#define M_SHAR3D_PTR_DEF_P3(name, shared_t, type, oplist, fattr)              \
    M_BEGIN_PROTECTED_CODE                                                    \
    M_SHAR3D_PTR_DEF_TYPE(name, shared_t, type, oplist)                       \
    M_SHAR3D_PTR_DEF_CORE(name, shared_t, type, oplist, fattr)                \
    M_SHAR3D_PTR_DEF_BASIC(name, shared_t, type, oplist, fattr)               \
    M_SHAR3D_PTR_DEF_EXTRA(name, shared_t, type, oplist, fattr)               \
    M_SHAR3D_PTR_DEF_ARITH(name, shared_t, type, oplist, fattr)               \
    M_SHAR3D_PTR_DEF_KEY(name, shared_t, type, oplist, fattr, M_GET_KEY_TYPE oplist, M_GET_KEY_OPLIST oplist, M_GET_VALUE_TYPE oplist, M_GET_VALUE_OPLIST oplist) \
    M_SHAR3D_PTR_DEF_PUSH(name, shared_t, type, oplist, fattr, M_GET_SUBTYPE oplist, M_GET_OPLIST oplist) \
    M_SHAR3D_PTR_DEF_POP(name, shared_t, type, oplist, fattr, M_GET_SUBTYPE oplist, M_GET_OPLIST oplist) \
    M_SHAR3D_PTR_DEF_IT(name, shared_t, type, oplist, fattr, M_GET_SUBTYPE oplist, M_GET_OPLIST oplist) \
    M_SHAR3D_PTR_DEF_IO(name, shared_t, type, oplist, fattr)                  \
    M_END_PROTECTED_CODE


/* Declare the type of shared pointer. Opaque structure for encapsulation */
#define M_SHAR3D_PTR_DECL_TYPE(name, shared_t, oplist)                        \
    typedef struct M_C(name, _s) shared_t;

/* Definition of the type with no thread safety (single thread) */
#define M_SHAR3D_PTR_NO_THRD_DEF_TYPE(name, shared_t, type, oplist)           \
    struct M_C(name, _s) {                                                    \
        type        data; /* Allow safe casting from shared_t* to type* */    \
        unsigned cpt;     /* Owner counter that acquire the data */           \
    };

/* Define the core functions without any thread safety */
#define M_SHAR3D_PTR_NO_THRD_DEF_CORE(name, shared_t, type, oplist, fattr)    \
/* Increment the number of owner (acquire the resource) */                    \
static inline void M_C3(m_shar3d_, name, _inc_owner)(shared_t *out)           \
{                                                                             \
    out->cpt ++;                                                              \
}                                                                             \
/* Decrement the number of owner (release the resource). Return true if it was the last owner */ \
static inline bool M_C3(m_shar3d_, name, _dec_owner)(shared_t *out)           \
{                                                                             \
    out->cpt --;                                                              \
    return 0 == out->cpt;                                                     \
}                                                                             \
/* Init the lock */                                                           \
static inline  void M_F(name, _init_lock)(shared_t *out)                      \
{                                                                             \
    out->cpt = 1;                                                             \
}                                                                             \
/* Clear the lock */                                                          \
static inline  void M_F(name, _clear_lock)(shared_t *out)                     \
{                                                                             \
    M_ASSERT(out->cpt == 0);                                                  \
    (void) out;                                                               \
}                                                                             \
/* Enter the lock for reading the data */                                     \
static inline  void M_F(name, _read_lock)(const shared_t *out)                \
{                                                                             \
    (void) out;                                                               \
}                                                                             \
/* wait for some new data to be available (within read_lock) */               \
static inline  void M_F(name, _read_wait)(const shared_t *out)                \
{                                                                             \
    (void) out;                                                               \
}                                                                             \
/* Leave the lock for reading the data (within read_lock) */                  \
static inline  void M_F(name, _read_unlock)(const shared_t *out)              \
{                                                                             \
    (void) out;                                                               \
}                                                                             \
/* Enter the lock for writing/updating the data */                            \
static inline  void M_F(name, _write_lock)(shared_t *out)                     \
{                                                                             \
    (void) out;                                                               \
}                                                                             \
/* wait to be able to write new data (within write_lock) */                   \
static inline  void M_F(name, _write_wait)(shared_t *out)                     \
{                                                                             \
    (void) out;                                                               \
}                                                                             \
/* signal some new data are available (within write_lock) */                  \
static inline  void M_F(name, _write_signal)(shared_t *out)                   \
{                                                                             \
    (void) out;                                                               \
}                                                                             \
/* signal some free slot are available (within write_lock) */                 \
static inline  void M_F(name, _free_signal)(shared_t *out)                    \
{                                                                             \
    (void) out;                                                               \
}                                                                             \
/* Leaver the lock for writing/updating the data (within write_lock) */       \
static inline  void M_F(name, _write_unlock)(shared_t *out)                   \
{                                                                             \
    (void) out;                                                               \
}                                                                             \
/* Return a pointer to the data */                                            \
static inline  type *M_F(name, _ref)(shared_t *out)                           \
{                                                                             \
    return &out->data;                                                        \
}                                                                             \
/* Return a const pointer to the data */                                      \
static inline  type const *M_F(name, _cref)(const shared_t *out)              \
{                                                                             \
    return &out->data;                                                        \
}                                                                             \
/* Perform a write lock on out and a read lock on src */                      \
static inline  void M_F(name, _write_read_lock)(shared_t *out, const shared_t *src) \
{                                                                             \
    (void) out;                                                               \
    (void) src;                                                               \
}                                                                             \
/* Leave the write lock on out and the read lock on src */                    \
static inline  void M_F(name, _write_read_unlock)(shared_t *out, const shared_t *src) \
{                                                                             \
    (void) out;                                                               \
    (void) src;                                                               \
}                                                                             \
static inline void M_F(name, _write_read2_lock)(shared_t *out, const shared_t *src1, const shared_t *src2) \
{                                                                             \
    (void) out;                                                               \
    (void) src1;                                                              \
    (void) src2;                                                              \
}                                                                             \
                                                                              \
static inline void M_F(name, _write_read2_unlock)(shared_t *out, const shared_t *src1, const shared_t *src2) \
{                                                                             \
    (void) out;                                                               \
    (void) src1;                                                              \
    (void) src2;                                                              \
}                                                                             \


/* Definition of the type with thread safety. */
//TODO: If PROPERTIES.THREADSAFE, disable the global lock as the container handles it itself.
//FIXME: Such a property may need to be more fine tuned than globally.
#define M_SHAR3D_PTR_DEF_TYPE(name, shared_t, type, oplist)                   \
    struct M_C(name, _s) {                                                    \
        type        data; /* Allow safe casting from shared_t* to type* */    \
        atomic_uint cpt;  /* Owner counter that acquire the data */           \
        m_mutex_t   lock; /* Lock to modify the data */                       \
        m_cond_t  there_is_data; /* cond var sent if there a new data added */ \
        M_IF_METHOD(FULL_P, oplist)( /* cond var has meaning if it can be full */ \
        m_cond_t  there_is_slot; /* cond var sent if there is a free slot */  \
        , )                                                                   \
    };

/* Define the core functions with thread safety 
   NOTE: lock are exception free. */
#define M_SHAR3D_PTR_DEF_CORE(name, shared_t, type, oplist, fattr)            \
                                                                              \
static inline void M_C3(m_shar3d_, name, _inc_owner)(shared_t *out)           \
{                                                                             \
    atomic_fetch_add(&out->cpt, 1);                                           \
}                                                                             \
                                                                              \
static inline bool M_C3(m_shar3d_, name, _dec_owner)(shared_t *out)           \
{                                                                             \
    return 1 == atomic_fetch_sub(&out->cpt, 1);                               \
}                                                                             \
                                                                              \
static inline void M_F(name, _init_lock)(shared_t *out)                       \
{                                                                             \
    atomic_init(&out->cpt, 1);                                                \
    m_mutex_init(out->lock);                                                  \
    m_cond_init(out->there_is_data);                                          \
    M_IF_METHOD(FULL_P, oplist)(m_cond_init(out->there_is_slot), (void)0);    \
}                                                                             \
                                                                              \
static inline void M_F(name, _clear_lock)(shared_t *out)                      \
{                                                                             \
    m_mutex_clear(out->lock);                                                 \
    m_cond_clear(out->there_is_data);                                         \
    M_IF_METHOD(FULL_P, oplist)(m_cond_clear(out->there_is_slot), (void)0);   \
}                                                                             \
                                                                              \
static inline void M_F(name, _read_lock)(const shared_t *out)                 \
{                                                                             \
    shared_t *self = (shared_t *)(uintptr_t)out;                              \
    m_mutex_lock (self->lock);                                                \
}                                                                             \
                                                                              \
static inline void M_F(name, _read_wait)(const shared_t *out)                 \
{                                                                             \
    shared_t *self = (shared_t *)(uintptr_t)out;                              \
    m_cond_wait(self->there_is_data, self->lock);                             \
}                                                                             \
                                                                              \
static inline void M_F(name, _read_unlock)(const shared_t *out)               \
{                                                                             \
    shared_t *self = (shared_t *)(uintptr_t)out;                              \
    m_mutex_unlock (self->lock);                                              \
}                                                                             \
                                                                              \
static inline void M_F(name, _write_lock)(shared_t *out)                      \
{                                                                             \
    m_mutex_lock (out->lock);                                                 \
}                                                                             \
                                                                              \
static inline void M_F(name, _write_wait)(shared_t *out)                      \
{                                                                             \
    M_IF_METHOD(FULL_P, oplist)(                                              \
        m_cond_wait(out->there_is_slot, out->lock);                           \
    ,(void) out;)                                                             \
}                                                                             \
                                                                              \
static inline void M_F(name, _write_signal)(shared_t *out)                    \
{                                                                             \
    m_cond_broadcast(out->there_is_data);                                     \
}                                                                             \
                                                                              \
static inline void M_F(name, _free_signal)(shared_t *out)                     \
{                                                                             \
    M_IF_METHOD(FULL_P, oplist)(                                              \
        m_cond_broadcast(out->there_is_slot);                                 \
    , (void) out;)                                                            \
}                                                                             \
                                                                              \
static inline void M_F(name, _write_unlock)(shared_t *out)                    \
{                                                                             \
    m_mutex_unlock (out->lock);                                               \
}                                                                             \
                                                                              \
static inline type *M_F(name, _ref)(shared_t *out)                            \
{                                                                             \
    return &out->data;                                                        \
}                                                                             \
                                                                              \
static inline type const *M_F(name, _cref)(const shared_t *out)               \
{                                                                             \
    return &out->data;                                                        \
}                                                                             \
                                                                              \
static inline void M_F(name, _write_read_lock)(shared_t *out, const shared_t *src) \
{                                                                             \
    /* We cannot perform a write_lock and a read_lock on the same variable at the same time */ \
    M_ASSERT(out != src);                                                     \
    /* Need to order the locks in a total way to avoid lock deadlock.         \
       Indeed, two call to _set can be done in two threads with :             \
       T1: A := B                                                             \
       T2: B := A                                                             \
       If we lock first the mutex of out, then the src, it could be possible  \
       in the previous scenario that both mutexes are locked: T1 has locked A \
       and T2 has locked B, and T1 is waiting for locking B, and T2 is waiting \
       for locking A, resulting in a deadlock.                                \
       To avoid this problem, we **always** lock the mutex which address is   \
       the lowest.                                                            \
       Note that is technically an undefined behavior of the C standard */    \
    if (out < src) {                                                          \
      M_F(name, _write_lock)(out);                                            \
      M_F(name, _read_lock)(src);                                             \
    } else {                                                                  \
      M_F(name, _read_lock)(src);                                             \
      M_F(name, _write_lock)(out);                                            \
    }                                                                         \
}                                                                             \
                                                                              \
static inline void M_F(name, _write_read_unlock)(shared_t *out, const shared_t *src) \
{                                                                             \
    M_ASSERT(out != src);                                                     \
    /* NOTE: No need to order the unlock */                                   \
    M_F(name, _read_unlock)(src);                                             \
    M_F(name, _write_unlock)(out);                                            \
}                                                                             \
                                                                              \
fattr void M_F(name, _write_read2_lock)(shared_t *out, const shared_t *src1, const shared_t *src2); \
fattr void M_F(name, _write_read2_lock)(shared_t *out, const shared_t *src1, const shared_t *src2) \
{                                                                             \
    /* We can have out == src1 or out == src2 or src1 == src2 */              \
    if (out == src1) {                                                        \
        if (out == src2) {                                                    \
            M_F(name, _write_lock)(out);                                      \
        } else if (out < src2) {                                              \
            M_F(name, _write_lock)(out);                                      \
            M_F(name, _read_lock)(src2);                                      \
        } else {                                                              \
            M_F(name, _read_lock)(src2);                                      \
            M_F(name, _write_lock)(out);                                      \
        }                                                                     \
        return;                                                               \
    } else if (out == src2) {                                                 \
        M_ASSERT(out != src1);                                                \
        if (out < src1) {                                                     \
            M_F(name, _write_lock)(out);                                      \
            M_F(name, _read_lock)(src1);                                      \
        } else {                                                              \
            M_F(name, _read_lock)(src1);                                      \
            M_F(name, _write_lock)(out);                                      \
        }                                                                     \
        return;                                                               \
    } else if (src1 == src2) {                                                \
        if (out < src1) {                                                     \
            /* out < src1 = src2 */                                           \
            M_F(name, _write_lock)(out);                                      \
            M_F(name, _read_lock)(src1);                                      \
        } else {                                                              \
            /* src1 = src2 < out */                                           \
            M_F(name, _read_lock)(src1);                                      \
            M_F(name, _write_lock)(out);                                      \
        }                                                                     \
        return;                                                               \
    }                                                                         \
    /* See above  */                                                          \
    if (src1 > src2) {                                                        \
        M_SWAP(const shared_t *, src1, src2);                                 \
    }                                                                         \
    /* src1 < src2 */                                                         \
    if (out < src1) {                                                         \
        /* out < src1 < src2 */                                               \
        M_F(name, _write_lock)(out);                                          \
        M_F(name, _read_lock)(src1);                                          \
        M_F(name, _read_lock)(src2);                                          \
    } else if (out < src2) {                                                  \
        /* src1 < out < src2 */                                               \
        M_F(name, _read_lock)(src1);                                          \
        M_F(name, _write_lock)(out);                                          \
        M_F(name, _read_lock)(src2);                                          \
    } else {                                                                  \
        /* src1 < src2 < out */                                               \
        M_F(name, _read_lock)(src1);                                          \
        M_F(name, _read_lock)(src2);                                          \
        M_F(name, _write_lock)(out);                                          \
    }                                                                         \
}                                                                             \
                                                                              \
fattr void M_F(name, _write_read2_unlock)(shared_t *out, const shared_t *src1, const shared_t *src2); \
fattr void M_F(name, _write_read2_unlock)(shared_t *out, const shared_t *src1, const shared_t *src2) \
{                                                                             \
    /* We can have out == src1 or out == src2 or src1 == src2 */              \
    if (out == src1) {                                                        \
        if (out == src2) {                                                    \
            M_F(name, _write_unlock)(out);                                    \
        } else {                                                              \
            M_F(name, _read_unlock)(src2);                                    \
            M_F(name, _write_unlock)(out);                                    \
        }                                                                     \
        return;                                                               \
    } else if (out == src2) {                                                 \
        M_ASSERT(out != src1);                                                \
        M_F(name, _read_unlock)(src1);                                        \
        M_F(name, _write_unlock)(out);                                        \
        return;                                                               \
    } else if (src1 == src2) {                                                \
        M_F(name, _read_unlock)(src1);                                        \
        M_F(name, _write_unlock)(out);                                        \
        return ;                                                              \
    }                                                                         \
    /* NOTE: No need to order the unlock */                                   \
    M_F(name, _read_unlock)(src1);                                            \
    M_F(name, _read_unlock)(src2);                                            \
    M_F(name, _write_unlock)(out);                                            \
}                                                                             \


/* Define the basic function of a shared pointer */
#define M_SHAR3D_PTR_DECL_BASIC(name, shared_t, oplist)                       \
M_IF_METHOD(INIT, oplist)( extern shared_t *M_F(name, _new)(void); , )        \
M_IF_METHOD(INIT_SET, oplist)( extern shared_t *M_F(name, _clone)(const shared_t *); , ) \
M_IF_METHOD(SET, oplist)( extern void M_F(name, _copy)(shared_t *, const shared_t *); , ) \
M_IF_METHOD2(INIT_SET, TYPE, oplist)( extern shared_t *M_F(name, _new_from)(M_GET_TYPE oplist const); , ) \
extern shared_t *M_F(name, _acquire)(shared_t *);                             \
extern void      M_F(name, _release)(shared_t *);                             \
extern void      M_F(name, _set)(shared_t **, shared_t *);                    \
extern void      M_F(name, _clear)(shared_t *);                               \
M_EMPLACE_QUEUE_DEF(name, shared_t, M_F(name, _make), oplist, M_SHAR3D_PTR_DECL_BASIC_MAKE)

#define M_SHAR3D_PTR_DECL_BASIC_MAKE(name, shared_t, function_name, oplist, init_func, exp_emplace_type) \
extern shared_t *function_name(M_EMPLACE_LIST_TYPE_VAR_ALTER(a, exp_emplace_type));

#define M_SHAR3D_PTR_DEF_BASIC(name, shared_t, type, oplist, fattr)           \
M_IF_METHOD(INIT, oplist)(                                                    \
    fattr shared_t *M_F(name, _new)(void)                                     \
    {                                                                         \
        M_GLOBAL_CONTEXT();                                                   \
        shared_t *out = M_CALL_NEW(oplist, shared_t);                         \
        if (M_UNLIKELY_NOMEM( out == NULL)) {                                 \
            M_MEMORY_FULL(shared_t, 1);                                       \
        }                                                                     \
        M_ON_EXCEPTION( M_CALL_DEL(oplist, out) )                             \
            M_CALL_INIT(oplist, out->data);                                   \
        M_F(name, _init_lock)(out);                                           \
        return out;                                                           \
    }                                                                         \
, )                                                                           \
                                                                              \
M_IF_METHOD(INIT_SET, oplist)(                                                \
    fattr shared_t *M_F(name, _clone)(const shared_t *src)                    \
    {                                                                         \
        M_ASSERT(src != NULL);                                                \
        M_GLOBAL_CONTEXT();                                                   \
        shared_t *out = M_CALL_NEW(oplist, shared_t);                         \
        if (M_UNLIKELY_NOMEM( out == NULL)) {                                 \
            M_MEMORY_FULL(shared_t, 1);                                       \
        }                                                                     \
        M_F(name, _read_lock)(src);                                           \
        M_ON_EXCEPTION( M_F(name, _read_unlock)(src), M_CALL_DEL(oplist, out) ) \
            M_CALL_INIT_SET(oplist, out->data, src->data);                    \
        M_F(name, _read_unlock)(src);                                         \
        M_F(name, _init_lock)(out);                                           \
        return out;                                                           \
    }                                                                         \
                                                                              \
    M_IF_METHOD(TYPE,oplist)( ,                                               \
    fattr shared_t *M_F(name, _new_from)(type const src); )                   \
    fattr shared_t *M_F(name, _new_from)(type const src)                      \
    {                                                                         \
        M_GLOBAL_CONTEXT();                                                   \
        shared_t *out = M_CALL_NEW(oplist, shared_t);                         \
        if (M_UNLIKELY_NOMEM( out == NULL)) {                                 \
            M_MEMORY_FULL(shared_t, 1);                                       \
        }                                                                     \
        M_ON_EXCEPTION( M_CALL_DEL(oplist, out) )                             \
            M_CALL_INIT_SET(oplist, out->data, src);                          \
        M_F(name, _init_lock)(out);                                           \
        return out;                                                           \
    }                                                                         \
, )                                                                           \
                                                                              \
M_IF_METHOD(SET, oplist)(                                                     \
    fattr void M_F(name, _copy)(shared_t *out, const shared_t *src)           \
    {                                                                         \
        M_ASSERT(out != NULL);                                                \
        M_ASSERT(src != NULL);                                                \
        M_GLOBAL_CONTEXT();                                                   \
        if (M_UNLIKELY (out == src)) return;                                  \
        M_F(name, _write_read_lock)(out, src);                                \
        M_ON_EXCEPTION( M_F(name, _write_read_unlock)(out, src) )             \
            M_CALL_SET(oplist, out->data, src->data);                         \
        M_F(name, _write_signal)(out);                                        \
        M_F(name, _write_read_unlock)(out, src);                              \
    }                                                                         \
 , )                                                                          \
                                                                              \
fattr shared_t *M_F(name, _acquire)(shared_t *out)                            \
{                                                                             \
    M_ASSERT(out != NULL);                                                    \
    M_C3(m_shar3d_, name, _inc_owner)(out);                                   \
    return out;                                                               \
}                                                                             \
                                                                              \
fattr void M_F(name, _release)(shared_t *out)                                 \
{                                                                             \
    M_GLOBAL_CONTEXT();                                                       \
    if (out != NULL && M_C3(m_shar3d_, name, _dec_owner)(out)) {              \
        M_CALL_CLEAR(oplist, out->data);                                      \
        M_F(name, _clear_lock)(out);                                          \
        M_CALL_DEL(oplist, out);                                              \
    }                                                                         \
}                                                                             \
                                                                              \
fattr void M_F(name, _set)(shared_t **dst, shared_t *out)                     \
{                                                                             \
    M_ASSERT(dst != NULL && out != NULL);                                     \
    M_F(name, _release)(*dst);                                                \
    *dst = M_F(name, _acquire)(out);                                          \
}                                                                             \
                                                                              \
fattr void M_F(name, _clear)(shared_t *out)                                   \
{                                                                             \
    M_F(name, _release)(out);                                                 \
}                                                                             \
M_EMPLACE_QUEUE_DEF( (name, fattr), shared_t, M_F(name, _make), oplist, M_SHAR3D_PTR_DEF_BASIC_MAKE)

#define M_SHAR3D_PTR_DEF_BASIC_MAKE(name_attr, shared_t, function_name, oplist, init_func, exp_emplace_type) \
M_PAIR_2 name_attr shared_t *function_name(M_EMPLACE_LIST_TYPE_VAR_ALTER(a, exp_emplace_type)) \
{                                                                             \
        M_GLOBAL_CONTEXT();                                                   \
        shared_t *out = M_CALL_NEW(oplist, shared_t);                         \
        if (M_UNLIKELY_NOMEM( out == NULL)) {                                 \
            M_MEMORY_FULL(shared_t, 1);                                       \
        }                                                                     \
        M_ON_EXCEPTION( M_CALL_DEL(oplist, out) )                             \
            M_EMPLACE_CALL_FUNC(a, init_func, oplist, out->data, exp_emplace_type); \
        M_F(M_PAIR_1 name_attr, _init_lock)(out);                             \
        return out;                                                           \
}                                                                             \


/* Define the extra function of a shared pointer */
#define M_SHAR3D_PTR_DECL_EXTRA(name, shared_t, oplist)                       \
M_IF_METHOD(SWAP, oplist)( extern void M_F(name, _swap)(shared_t *, shared_t *); , ) \
M_IF_METHOD(RESET, oplist)( extern void M_F(name, _reset)(shared_t *); , )    \
M_IF_METHOD(EMPTY_P, oplist)( extern bool M_F(name, _empty_p)(const shared_t *); , ) \
M_IF_METHOD(FULL_P, oplist)( extern bool M_F(name, _full_p)(const shared_t *); , ) \
M_IF_METHOD(GET_SIZE, oplist)( extern size_t M_F(name, _size)(const shared_t *); , ) \
M_IF_METHOD(EQUAL, oplist)( extern bool M_F(name, _equal_p)(const shared_t *, const shared_t *); , ) \
M_IF_METHOD(CMP, oplist)( extern int M_F(name, _cmp)(const shared_t *, const shared_t *); , ) \
M_IF_METHOD(HASH, oplist)( extern size_t M_F(name, _hash)(const shared_t *); , ) \

#define M_SHAR3D_PTR_DEF_EXTRA(name, shared_t, type, oplist, fattr)           \
M_IF_METHOD(SWAP, oplist)(                                                    \
    fattr void M_F(name, _swap)(shared_t *o1, shared_t *o2)                   \
    {                                                                         \
        M_ASSERT(o1 != NULL && o2 != NULL);                                   \
        if (M_UNLIKELY (o1 == o2)) return;                                    \
        /* See comment above on _write_read_lock */                           \
        if (o1 > o2) {                                                        \
            M_SWAP(shared_t *, o1, o2);                                       \
        }                                                                     \
        M_F(name, _write_lock)(o1);                                           \
        M_F(name, _write_lock)(o2);                                           \
        /* SWAP cannot throw exception */                                     \
        M_CALL_SWAP(oplist, o1->data, o2->data);                              \
        /* It doesn't make sense to signal new data or new slot, Right? */    \
        M_F(name, _write_unlock)(o1);                                         \
        M_F(name, _write_unlock)(o2);                                         \
    }                                                                         \
, )                                                                           \
                                                                              \
M_IF_METHOD(RESET, oplist)(                                                   \
    fattr void M_F(name, _reset)(shared_t *out)                               \
    {                                                                         \
        M_ASSERT(out != NULL);                                                \
        M_GLOBAL_CONTEXT();                                                   \
        M_F(name, _write_lock)(out);                                          \
        /* RESET cannot throw exception */                                    \
        M_CALL_RESET(oplist, out->data);                                      \
        M_F(name, _free_signal)(out);                                         \
        M_F(name, _write_unlock)(out);                                        \
    }                                                                         \
, )                                                                           \
                                                                              \
M_IF_METHOD(EMPTY_P, oplist)(                                                 \
    fattr bool M_F(name, _empty_p)(const shared_t *out)                       \
    {                                                                         \
        M_ASSERT(out != NULL);                                                \
        M_F(name, _read_lock)(out);                                           \
        /* EMPTY_P cannot throw exception */                                  \
        bool r = M_CALL_EMPTY_P(oplist, out->data);                           \
        M_F(name, _read_unlock)(out);                                         \
        return r;                                                             \
    }                                                                         \
, )                                                                           \
                                                                              \
M_IF_METHOD(FULL_P, oplist)(                                                  \
    fattr bool M_F(name, _full_p)(const shared_t *out)                        \
    {                                                                         \
        M_ASSERT(out != NULL);                                                \
        M_F(name, _read_lock)(out);                                           \
        /* FULL_P cannot throw exception */                                   \
        bool r = M_CALL_FULL_P(oplist, out->data);                            \
        M_F(name, _read_unlock)(out);                                         \
        return r;                                                             \
    }                                                                         \
, )                                                                           \
                                                                              \
M_IF_METHOD(GET_SIZE, oplist)(                                                \
    fattr size_t M_F(name, _size)(const shared_t *out)                        \
    {                                                                         \
        M_ASSERT(out != NULL);                                                \
        M_F(name, _read_lock)(out);                                           \
        /* GET_SIZE cannot throw exception */                                 \
        size_t r = M_CALL_GET_SIZE(oplist, out->data);                        \
        M_F(name, _read_unlock)(out);                                         \
        return r;                                                             \
    }                                                                         \
, )                                                                           \
                                                                              \
M_IF_METHOD(EQUAL, oplist)(                                                   \
    fattr bool M_F(name, _equal_p)(const shared_t *o1, const shared_t *o2)    \
    {                                                                         \
        M_ASSERT(o1 != NULL && o2 != NULL);                                   \
        if (o1 == o2) return true;                                            \
        /* See comment above on _write_read_lock */                           \
        if (o1 > o2) {                                                        \
            M_SWAP(const shared_t *, o1, o2);                                 \
        }                                                                     \
        /* FIXME: OOR compare and constraint on OOR argument? */              \
        M_F(name, _read_lock)(o1);                                            \
        M_F(name, _read_lock)(o2);                                            \
        /* EQUAL cannot throw exception */                                    \
        bool r = M_CALL_EQUAL(oplist, o1->data, o2->data);                    \
        M_F(name, _read_unlock)(o1);                                          \
        M_F(name, _read_unlock)(o2);                                          \
        return r;                                                             \
    }                                                                         \
, )                                                                           \
                                                                              \
M_IF_METHOD(CMP, oplist)(                                                     \
    fattr int M_F(name, _cmp)(const shared_t *o1, const shared_t *o2)         \
    {                                                                         \
        M_ASSERT(o1 != NULL && o2 != NULL);                                   \
        int neg = 1;                                                          \
        if (o1 == o2) return 0;                                               \
        /* See comment above on _write_read_lock */                           \
        if (o1 > o2) {                                                        \
            M_SWAP(const shared_t *, o1, o2);                                 \
            /* inverse input, so inverse result too */                        \
            neg = -1;                                                         \
        }                                                                     \
        M_F(name, _read_lock)(o1);                                            \
        M_F(name, _read_lock)(o2);                                            \
        /* CMP cannot throw exception */                                      \
        int r = M_CALL_CMP(oplist, o1->data, o2->data);                       \
        M_F(name, _read_unlock)(o1);                                          \
        M_F(name, _read_unlock)(o2);                                          \
        return r * neg;                                                       \
    }                                                                         \
, )                                                                           \
                                                                              \
M_IF_METHOD(HASH, oplist)(                                                    \
    fattr size_t M_F(name, _hash)(const shared_t *out)                        \
    {                                                                         \
        M_ASSERT(out != NULL);                                                \
        M_F(name, _read_lock)(out);                                           \
        /* HASH cannot throw exception */                                     \
        size_t r = M_CALL_HASH(oplist, out->data);                            \
        M_F(name, _read_unlock)(out);                                         \
        return r;                                                             \
    }                                                                         \
, )                                                                           \


/* Define the arithmetic operators */
#define M_SHAR3D_PTR_DECL_ARITH(name, shared_t, oplist)                       \
M_IF_METHOD(ADD, oplist)( extern void M_F(name, _add)(shared_t *, const shared_t *, const shared_t *); , ) \
M_IF_METHOD(SUB, oplist)( extern void M_F(name, _sub)(shared_t *, const shared_t *, const shared_t *); , ) \
M_IF_METHOD(MUL, oplist)( extern void M_F(name, _mul)(shared_t *, const shared_t *, const shared_t *); , ) \
M_IF_METHOD(DIV, oplist)( extern void M_F(name, _div)(shared_t *, const shared_t *, const shared_t *); , ) \
M_IF_METHOD(SPLICE, oplist)( extern void M_F(name, _splice)(shared_t *, shared_t *); , ) \

#define M_SHAR3D_PTR_DEF_ARITH(name, shared_t, type, oplist, fattr)           \
M_IF_METHOD(ADD, oplist)(                                                     \
    fattr void M_F(name, _add)(shared_t *out, const shared_t *src1, const shared_t *src2) \
    {                                                                         \
        M_ASSERT(out != NULL && src1 != NULL && src2 != NULL);                \
        M_GLOBAL_CONTEXT();                                                   \
        M_F(name, _write_read2_lock)(out, src1, src2);                        \
        M_ON_EXCEPTION( M_F(name, _write_read2_unlock)(out, src1, src2) )     \
            M_CALL_ADD(oplist, out->data, src1->data, src2->data);            \
        M_F(name, _write_read2_unlock)(out, src1, src2);                      \
    }                                                                         \
, )                                                                           \
                                                                              \
M_IF_METHOD(SUB, oplist)(                                                     \
    fattr void M_F(name, _sub)(shared_t *out, const shared_t *src1, const shared_t *src2) \
    {                                                                         \
        M_ASSERT(out != NULL && src1 != NULL && src2 != NULL);                \
        M_GLOBAL_CONTEXT();                                                   \
        M_F(name, _write_read2_lock)(out, src1, src2);                        \
        M_ON_EXCEPTION( M_F(name, _write_read2_unlock)(out, src1, src2) )     \
            M_CALL_SUB(oplist, out->data, src1->data, src2->data);            \
        M_F(name, _write_read2_unlock)(out, src1, src2);                      \
    }                                                                         \
, )                                                                           \
                                                                              \
M_IF_METHOD(MUL, oplist)(                                                     \
    fattr void M_F(name, _mul)(shared_t *out, const shared_t *src1, const shared_t *src2) \
    {                                                                         \
        M_ASSERT(out != NULL && src1 != NULL && src2 != NULL);                \
        M_GLOBAL_CONTEXT();                                                   \
        M_F(name, _write_read2_lock)(out, src1, src2);                        \
        M_ON_EXCEPTION( M_F(name, _write_read2_unlock)(out, src1, src2) )     \
            M_CALL_MUL(oplist, out->data, src1->data, src2->data);            \
        M_F(name, _write_read2_unlock)(out, src1, src2);                      \
    }                                                                         \
, )                                                                           \
                                                                              \
M_IF_METHOD(DIV, oplist)(                                                     \
    fattr void M_F(name, _div)(shared_t *out, const shared_t *src1, const shared_t *src2) \
    {                                                                         \
        M_ASSERT(out != NULL && src1 != NULL && src2 != NULL);                \
        M_GLOBAL_CONTEXT();                                                   \
        M_F(name, _write_read2_lock)(out, src1, src2);                        \
        M_ON_EXCEPTION( M_F(name, _write_read2_unlock)(out, src1, src2) )     \
            M_CALL_DIV(oplist, out->data, src1->data, src2->data);            \
        M_F(name, _write_read2_unlock)(out, src1, src2);                      \
    }                                                                         \
, )                                                                           \
                                                                              \
M_IF_METHOD(SPLICE, oplist)(                                                  \
    fattr void M_F(name, _splice)(shared_t *out, shared_t *src)               \
    {                                                                         \
        M_ASSERT(out != NULL && src != NULL);                                 \
        M_GLOBAL_CONTEXT();                                                   \
        if (out < src) {                                                      \
            M_F(name, _write_lock)(out);                                      \
            M_F(name, _write_lock)(src);                                      \
        } else {                                                              \
            M_F(name, _write_lock)(src);                                      \
            M_F(name, _write_lock)(out);                                      \
        }                                                                     \
        M_ON_EXCEPTION( M_F(name, _write_unlock)(out), M_F(name, _write_unlock)(src) ) \
            M_CALL_SPLICE(oplist, out->data, src->data);                      \
        M_F(name, _write_signal)(out);                                        \
        M_F(name, _write_unlock)(out);                                        \
        M_F(name, _write_unlock)(src);                                        \
    }                                                                         \
, )                                                                           \


/* Define the key based operators */
#define M_SHAR3D_PTR_DECL_KEY(name, shared_t, oplist, key_type, value_type)   \
M_IF_METHOD(GET_KEY, oplist)( extern bool M_F(name, _get)(value_type *, const shared_t *, key_type const); , ) \
M_IF_METHOD(SAFE_GET_KEY, oplist)( extern void M_F(name, _safe_get)(value_type *, shared_t *, key_type const); , ) \
M_IF_METHOD(SET_KEY, oplist)( extern void M_F(name, _set_at)(shared_t *, key_type const, value_type const); , ) \
M_IF_METHOD(ERASE_KEY, oplist)( extern bool M_F(name, _erase)(shared_t *, key_type const); , )

#define M_SHAR3D_PTR_DEF_KEY(name, shared_t, type, oplist, fattr, key_type, key_oplist, value_type, value_oplist) \
M_IF_METHOD(GET_KEY, oplist)(                                                 \
    fattr bool M_F(name, _get)(value_type *value, const shared_t *out, key_type const key) \
    {                                                                         \
        M_ASSERT (out != NULL);                                               \
        M_ASSERT (value != NULL);                                             \
        M_GLOBAL_CONTEXT();                                                   \
        M_F(name, _read_lock)(out);                                           \
        /* No exception */                                                    \
        value_type *p = M_CALL_GET_KEY(oplist, out->data, key);               \
        if (p != NULL) {                                                      \
            M_CALL_SET(value_oplist, *value, *p);                             \
        }                                                                     \
        M_F(name, _read_unlock)(out);                                         \
        return p != NULL;                                                     \
    }                                                                         \
, )                                                                           \
/* FIXME: _try_get and _get is blocking? */                                   \
M_IF_METHOD(SAFE_GET_KEY, oplist)(                                            \
    fattr void M_F(name, _safe_get)(value_type *value, shared_t *out, key_type const key) \
    {                                                                         \
        M_ASSERT (out != NULL);                                               \
        M_ASSERT (value != NULL);                                             \
        M_GLOBAL_CONTEXT();                                                   \
        M_F(name, _write_lock)(out);                                          \
        M_ON_EXCEPTION( M_F(name, _write_unlock)(out) ) {                     \
            value_type *p = M_CALL_SAFE_GET_KEY(oplist, out->data, key);      \
            M_ASSERT (p != NULL);                                             \
            M_CALL_SET(value_oplist, *value, *p);                             \
        }                                                                     \
        /* FIXME: Signal?*/                                                   \
        M_F(name, _write_unlock)(out);                                        \
    }                                                                         \
, )                                                                           \
                                                                              \
M_IF_METHOD(SET_KEY, oplist)(                                                 \
    fattr void M_F(name, _set_at)(shared_t *out, key_type const key, value_type const value) \
    {                                                                         \
        M_ASSERT (out != NULL);                                               \
        M_GLOBAL_CONTEXT();                                                   \
        M_F(name, _write_lock)(out);                                          \
        M_ON_EXCEPTION( M_F(name, _write_unlock)(out) )                       \
            M_CALL_SET_KEY(oplist, out->data, key, value);                    \
        /* FIXME: Signal?*/                                                   \
        M_F(name, _write_unlock)(out);                                        \
    }                                                                         \
, )                                                                           \
                                                                              \
M_IF_METHOD(ERASE_KEY, oplist)(                                               \
    fattr bool M_F(name, _erase)(shared_t *out, key_type const key)           \
    {                                                                         \
        M_ASSERT (out != NULL);                                               \
        M_GLOBAL_CONTEXT();                                                   \
        M_F(name, _write_lock)(out);                                          \
        /* No exception */                                                    \
        bool r = M_CALL_ERASE_KEY(oplist, out->data, key);                    \
        /* FIXME: Signal?*/                                                   \
        M_F(name, _write_unlock)(out);                                        \
        return r;                                                             \
    }                                                                         \
, )                                                                           \


// TODO: Define default for FULL_P & EMPTY_P if not defined in the oplist of the type
// always return false for FULL_P and EMPTY_P only for PUSH and POP

/* Define the push based operators */
#define M_SHAR3D_PTR_DECL_PUSH(name, shared_t, oplist, sub_type)              \
M_IF_METHOD(PUSH, oplist)( extern void M_F(name, _push)(shared_t *, sub_type const); , ) \
M_IF_METHOD(PUSH_MOVE, oplist)( extern void M_F(name, _push_move)(shared_t *, sub_type *); , ) \
M_IF_METHOD(PUSH, oplist)( extern bool M_F(name, _try_push)(shared_t *, sub_type const); , ) \
M_IF_METHOD(PUSH_MOVE, oplist)( extern bool M_F(name, _try_push_move)(shared_t *, sub_type *); , ) \
M_EMPLACE_QUEUE_DEF(name, shared_t, M_F(name, _emplace), M_GET_OPLIST oplist, M_SHAR3D_PTR_DECL_EMPLACE) \
M_EMPLACE_QUEUE_DEF(name, shared_t, M_F(name, _try_emplace), M_GET_OPLIST oplist, M_SHAR3D_PTR_DECL_TRY_EMPLACE)

#define M_SHAR3D_PTR_DECL_EMPLACE(name, shared_t, function_name, oplist, init_func, exp_emplace_type) \
extern void function_name(shared_t* M_EMPLACE_LIST_TYPE_VAR(a, exp_emplace_type));

#define M_SHAR3D_PTR_DECL_TRY_EMPLACE(name, shared_t, function_name, oplist, init_func, exp_emplace_type) \
extern bool function_name(shared_t* M_EMPLACE_LIST_TYPE_VAR(a, exp_emplace_type));

#define M_SHAR3D_PTR_DEF_PUSH(name, shared_t, type, oplist, fattr, sub_type, sub_oplist) \
M_IF_METHOD(PUSH, oplist)(                                                    \
    fattr void M_F(name, _push)(shared_t *out, sub_type const value)          \
    {                                                                         \
        M_ASSERT(out != NULL);                                                \
        M_GLOBAL_CONTEXT();                                                   \
        M_F(name, _write_lock)(out);                                          \
        while (true) {                                                        \
            if (M_CALL_FULL_P(oplist, out->data) == false) {                  \
                M_ON_EXCEPTION( M_F(name, _write_unlock)(out) )               \
                    M_CALL_PUSH(oplist, out->data, value);                    \
                M_F(name, _write_signal)(out);                                \
                break;                                                        \
            }                                                                 \
            M_F(name, _write_wait)(out);                                      \
        }                                                                     \
        M_F(name, _write_unlock)(out);                                        \
    }                                                                         \
                                                                              \
    fattr bool M_F(name, _try_push)(shared_t *out, sub_type const value)      \
    {                                                                         \
        M_ASSERT(out != NULL);                                                \
        M_GLOBAL_CONTEXT();                                                   \
        bool ret = false;                                                     \
        M_F(name, _write_lock)(out);                                          \
        if (M_CALL_FULL_P(oplist, out->data) == false) {                      \
            M_ON_EXCEPTION( M_F(name, _write_unlock)(out) )                   \
                M_CALL_PUSH(oplist, out->data, value);                        \
            M_F(name, _write_signal)(out);                                    \
            ret = true;                                                       \
        }                                                                     \
        M_F(name, _write_unlock)(out);                                        \
        return ret;                                                           \
    }                                                                         \
, )                                                                           \
M_IF_METHOD(PUSH_MOVE, oplist)(                                               \
    fattr void M_F(name, _push_move)(shared_t *out, sub_type *value)          \
    {                                                                         \
        M_ASSERT(out != NULL);                                                \
        M_GLOBAL_CONTEXT();                                                   \
        M_F(name, _write_lock)(out);                                          \
        while (true) {                                                        \
            if (M_CALL_FULL_P(oplist, out->data) == false) {                  \
                M_ON_EXCEPTION( M_F(name, _write_unlock)(out) )               \
                    M_CALL_PUSH_MOVE(oplist, out->data, value);               \
                M_F(name, _write_signal)(out);                                \
                break;                                                        \
            }                                                                 \
            M_F(name, _write_wait)(out);                                      \
        }                                                                     \
        M_F(name, _write_unlock)(out);                                        \
    }                                                                         \
                                                                              \
    fattr bool M_F(name, _try_push_move)(shared_t *out, sub_type *value)      \
    {                                                                         \
        M_ASSERT(out != NULL);                                                \
        M_GLOBAL_CONTEXT();                                                   \
        bool ret = false;                                                     \
        M_F(name, _write_lock)(out);                                          \
        if (M_CALL_FULL_P(oplist, out->data) == false) {                      \
            M_ON_EXCEPTION( M_F(name, _write_unlock)(out) )                   \
                M_CALL_PUSH_MOVE(oplist, out->data, value);                   \
            M_F(name, _write_signal)(out);                                    \
            ret = true;                                                       \
        }                                                                     \
        M_F(name, _write_unlock)(out);                                        \
        return ret;                                                           \
    }                                                                         \
    M_EMPLACE_QUEUE_DEF( (name, fattr, type, oplist), shared_t, M_F(name, _emplace), sub_oplist, M_SHAR3D_PTR_DEF_EMPLACE) \
    M_EMPLACE_QUEUE_DEF( (name, fattr, type, oplist), shared_t, M_F(name, _try_emplace), sub_oplist, M_SHAR3D_PTR_DEF_TRY_EMPLACE) \
, )                                                                           \

#define M_SHAR3D_PTR_DEF_EMPLACE(name_attr, shared_t, function_name, sub_oplist, init_func, exp_emplace_type) \
    M_SHAR3D_PTR_DEF_EMPLACE_P2(M_QUAD_1 name_attr, M_QUAD_2 name_attr, M_QUAD_3 name_attr, M_QUAD_4 name_attr, shared_t, function_name, sub_oplist, init_func, exp_emplace_type)
#define M_SHAR3D_PTR_DEF_EMPLACE_P2(name, fattr, type, oplist, shared_t, function_name, sub_oplist, init_func, exp_emplace_type) \
fattr void function_name(shared_t *out M_EMPLACE_LIST_TYPE_VAR(a, exp_emplace_type)) \
{                                                                             \
        M_ASSERT(out != NULL);                                                \
        M_GLOBAL_CONTEXT();                                                   \
        M_F(name, _write_lock)(out);                                          \
        while (true) {                                                        \
            if (M_CALL_FULL_P(oplist, out->data) == false) {                  \
                M_ON_EXCEPTION( M_F(name, _write_unlock)(out) ) {             \
                    M_GET_TYPE sub_oplist tmp;                                \
                    M_EMPLACE_CALL_FUNC(a, init_func, sub_oplist, tmp, exp_emplace_type); \
                    M_CALL_PUSH_MOVE(oplist, out->data, &tmp);                \
                    M_F(name, _write_signal)(out);                            \
                }                                                             \
                break;                                                        \
            }                                                                 \
            M_F(name, _write_wait)(out);                                      \
        }                                                                     \
        M_F(name, _write_unlock)(out);                                        \
}                                                                             \

#define M_SHAR3D_PTR_DEF_TRY_EMPLACE(name_attr, shared_t, function_name, sub_oplist, init_func, exp_emplace_type) \
    M_SHAR3D_PTR_DEF_TRY_EMPLACE_P2(M_QUAD_1 name_attr, M_QUAD_2 name_attr, M_QUAD_3 name_attr, M_QUAD_4 name_attr, shared_t, function_name, sub_oplist, init_func, exp_emplace_type)
#define M_SHAR3D_PTR_DEF_TRY_EMPLACE_P2(name, fattr, type, oplist, shared_t, function_name, sub_oplist, init_func, exp_emplace_type) \
fattr bool function_name(shared_t *out M_EMPLACE_LIST_TYPE_VAR(a, exp_emplace_type)) \
{                                                                             \
        M_ASSERT(out != NULL);                                                \
        M_GLOBAL_CONTEXT();                                                   \
        bool ret = false;                                                     \
        M_F(name, _write_lock)(out);                                          \
        if (M_CALL_FULL_P(oplist, out->data) == false) {                      \
            M_ON_EXCEPTION( M_F(name, _write_unlock)(out) ) {                 \
                M_GET_TYPE sub_oplist tmp;                                    \
                M_EMPLACE_CALL_FUNC(a, init_func, sub_oplist, tmp, exp_emplace_type); \
                M_CALL_PUSH_MOVE(oplist, out->data, &tmp);                    \
                M_F(name, _write_signal)(out);                                \
                ret = true;                                                   \
            }                                                                 \
        }                                                                     \
        M_F(name, _write_unlock)(out);                                        \
        return ret;                                                           \
}                                                                             \


/* Define the pop based operators */
#define M_SHAR3D_PTR_DECL_POP(name, shared_t, oplist, sub_type)               \
M_IF_METHOD(POP, oplist)( extern void M_F(name, _pop)(sub_type *const, shared_t *); , ) \
M_IF_METHOD(POP_MOVE, oplist)( extern void M_F(name, _pop_move)(sub_type *, shared_t *); , ) \
M_IF_METHOD(POP, oplist)( extern bool M_F(name, _try_pop)(sub_type*, shared_t *); , ) \
M_IF_METHOD(POP_MOVE, oplist)( extern bool M_F(name, _try_pop_move)(sub_type *, shared_t *); , )

#define M_SHAR3D_PTR_DEF_POP(name, shared_t, type, oplist, fattr, sub_type, sub_oplist) \
M_IF_METHOD(POP, oplist)(                                                     \
    fattr void M_F(name, _pop)(sub_type *value, shared_t *out)                \
    {                                                                         \
        M_ASSERT(out != NULL);                                                \
        M_GLOBAL_CONTEXT();                                                   \
        M_F(name, _write_lock)(out);                                          \
        while (true) {                                                        \
            if (M_CALL_EMPTY_P(oplist, out->data) == false) {                 \
                M_ON_EXCEPTION( M_F(name, _write_unlock)(out) )               \
                    M_CALL_POP(oplist, value, out->data);                     \
                M_F(name, _free_signal)(out);                                 \
                break;                                                        \
            }                                                                 \
            M_F(name, _read_wait)(out);                                       \
        }                                                                     \
        M_F(name, _write_unlock)(out);                                        \
    }                                                                         \
                                                                              \
    fattr bool M_F(name, _try_pop)(sub_type *value, shared_t *out)            \
    {                                                                         \
        M_ASSERT(out != NULL);                                                \
        M_GLOBAL_CONTEXT();                                                   \
        bool ret = false;                                                     \
        M_F(name, _write_lock)(out);                                          \
        if (M_CALL_EMPTY_P(oplist, out->data) == false) {                     \
            M_ON_EXCEPTION( M_F(name, _write_unlock)(out) )                   \
                M_CALL_POP(oplist, value, out->data);                         \
            M_F(name, _free_signal)(out);                                     \
            ret = true;                                                       \
        }                                                                     \
        M_F(name, _write_unlock)(out);                                        \
        return ret;                                                           \
    }                                                                         \
, )                                                                           \
M_IF_METHOD(POP_MOVE, oplist)(                                                \
    fattr void M_F(name, _pop_move)(sub_type *value, shared_t *out)           \
    {                                                                         \
        M_ASSERT(out != NULL);                                                \
        M_GLOBAL_CONTEXT();                                                   \
        M_F(name, _write_lock)(out);                                          \
        while (true) {                                                        \
            if (M_CALL_EMPTY_P(oplist, out->data) == false) {                 \
                M_ON_EXCEPTION( M_F(name, _write_unlock)(out) )               \
                    M_CALL_POP_MOVE(oplist, value, out->data);                \
                M_F(name, _free_signal)(out);                                 \
                break;                                                        \
            }                                                                 \
            M_F(name, _read_wait)(out);                                       \
        }                                                                     \
        M_F(name, _write_unlock)(out);                                        \
    }                                                                         \
                                                                              \
    fattr bool M_F(name, _try_pop_move)(sub_type *value, shared_t *out)       \
    {                                                                         \
        M_ASSERT(out != NULL);                                                \
        M_GLOBAL_CONTEXT();                                                   \
        bool ret = false;                                                     \
        M_F(name, _write_lock)(out);                                          \
        if (M_CALL_EMPTY_P(oplist, out->data) == false) {                     \
            M_ON_EXCEPTION( M_F(name, _write_unlock)(out) )                   \
                M_CALL_PUSH_MOVE(oplist, out->data, value);                   \
            M_F(name, _free_signal)(out);                                     \
            ret = true;                                                       \
        }                                                                     \
        M_F(name, _write_unlock)(out);                                        \
        return ret;                                                           \
    }                                                                         \
, )                                                                           \


/* Define the iterator based operators
   We cannot provide directly the iterator due to concurency API,
   so we produce an encapsulation of the major usage of them */
#define M_SHAR3D_PTR_DECL_IT(name, shared_t, oplist, sub_type)                \
M_IF_METHOD2(IT_FIRST, IT_REF, oplist)( extern int M_F(name, _apply)(shared_t *, int (*callback)(void *, sub_type*), void*); , ) \
M_IF_METHOD2(IT_FIRST, IT_CREF, oplist)( extern int M_F(name, _for_each)(const shared_t *, int (*callback)(void *, const sub_type*), void*); , ) \
M_IF_METHOD2(IT_LAST, IT_REF, oplist)( extern int M_F(name, _r_apply)(shared_t *, int (*callback)(void *, sub_type*), void*); , ) \
M_IF_METHOD2(IT_LAST, IT_CREF, oplist)( extern int M_F(name, _r_for_each)(const shared_t *, int (*callback)(void *, const sub_type*), void*); , ) \

#define M_SHAR3D_PTR_DEF_IT(name, shared_t, type, oplist, fattr, sub_type, sub_oplist) \
M_IF_METHOD2(IT_FIRST, IT_REF, oplist)(                                       \
    fattr int M_F(name, _apply)(shared_t *out, int (*callback)(void *, sub_type*), void*data) \
    {                                                                         \
        M_ASSERT(out != NULL);                                                \
        M_ASSERT(callback != NULL);                                           \
        int ret = 0;                                                          \
        M_F(name, _write_lock)(out);                                          \
        M_ON_EXCEPTION( M_F(name, _write_unlock)(out) ) {                     \
            M_GET_IT_TYPE oplist it;                                          \
            for(M_CALL_IT_FIRST(oplist, it, out->data); !M_CALL_IT_END_P(oplist, it); M_CALL_IT_NEXT(oplist, it)) { \
                ret = callback(data, M_CALL_IT_REF(oplist, it));              \
                if (ret)                                                      \
                    break;                                                    \
            }                                                                 \
        }                                                                     \
        /* There is no NEW data in the container: no signal to do */          \
        M_F(name, _write_unlock)(out);                                        \
        return ret;                                                           \
    }                                                                         \
, )                                                                           \
M_IF_METHOD2(IT_FIRST, IT_CREF, oplist)(                                      \
    fattr int M_F(name, _for_each)(const shared_t *out, int (*callback)(void *, const sub_type*), void*data) \
    {                                                                         \
        M_ASSERT(out != NULL);                                                \
        M_ASSERT(callback != NULL);                                           \
        int ret = 0;                                                          \
        M_F(name, _read_lock)(out);                                           \
        M_ON_EXCEPTION( M_F(name, _read_unlock)(out) ) {                      \
            M_GET_IT_TYPE oplist it;                                          \
            for(M_CALL_IT_FIRST(oplist, it, out->data); !M_CALL_IT_END_P(oplist, it); M_CALL_IT_NEXT(oplist, it)) { \
                ret = callback(data, M_CALL_IT_CREF(oplist, it));             \
                if (ret)                                                      \
                    break;                                                    \
            }                                                                 \
        }                                                                     \
        M_F(name, _read_unlock)(out);                                         \
        return ret;                                                           \
    }                                                                         \
, )                                                                           \
M_IF_METHOD2(IT_LAST, IT_REF, oplist)(                                        \
    fattr int M_F(name, _r_apply)(shared_t *out, int (*callback)(void *, sub_type*), void*data) \
    {                                                                         \
        M_ASSERT(out != NULL);                                                \
        M_ASSERT(callback != NULL);                                           \
        int ret = 0;                                                          \
        M_F(name, _write_lock)(out);                                          \
        M_ON_EXCEPTION( M_F(name, _write_unlock)(out) ) {                     \
            M_GET_IT_TYPE oplist it;                                          \
            for(M_CALL_IT_LAST(oplist, it, out->data); !M_CALL_IT_END_P(oplist, it); M_CALL_IT_PREVIOUS(oplist, it)) { \
                ret = callback(data, M_CALL_IT_REF(oplist, it));              \
                if (ret)                                                      \
                    break;                                                    \
            }                                                                 \
        }                                                                     \
        /* There is no NEW data in the container: no signal to do */          \
        M_F(name, _write_unlock)(out);                                        \
        return ret;                                                           \
    }                                                                         \
, )                                                                           \
M_IF_METHOD2(IT_LAST, IT_CREF, oplist)(                                       \
    fattr int M_F(name, _r_for_each)(const shared_t *out, int (*callback)(void *, const sub_type*), void*data) \
    {                                                                         \
        M_ASSERT(out != NULL);                                                \
        M_ASSERT(callback != NULL);                                           \
        int ret = 0;                                                          \
        M_F(name, _read_lock)(out);                                           \
        M_ON_EXCEPTION( M_F(name, _read_unlock)(out) ) {                      \
            M_GET_IT_TYPE oplist it;                                          \
            for(M_CALL_IT_LAST(oplist, it, out->data); !M_CALL_IT_END_P(oplist, it); M_CALL_IT_PREVIOUS(oplist, it)) { \
                ret = callback(data, M_CALL_IT_CREF(oplist, it));             \
                if (ret)                                                      \
                    break;                                                    \
            }                                                                 \
        }                                                                     \
        M_F(name, _read_unlock)(out);                                         \
        return ret;                                                           \
    }                                                                         \
, )                                                                           \


/* Define the I/O based operators */
#define M_SHAR3D_PTR_DECL_IO(name, shared_t, oplist)                          \
M_IF_METHOD(OUT_STR, oplist)( extern void M_F(name, _out_str)(FILE *, const shared_t *); , ) \
M_IF_METHOD(IN_STR, oplist)( extern bool M_F(name, _in_str)(shared_t *, FILE *); , ) \
M_IF_METHOD(GET_STR, oplist)( extern void M_F(name, _get_str)(m_string_t, const shared_t *, bool); , ) \
M_IF_METHOD(PARSE_STR, oplist)( extern bool M_F(name, _parse_str)(shared_t *, const char *, const char **); , ) \
M_IF_METHOD(OUT_SERIAL, oplist)( extern m_serial_return_code_t M_F(M_P_EXPAND name, _out_serial)(m_serial_write_t, const shared_t *); , ) \
M_IF_METHOD(IN_SERIAL, oplist)( extern m_serial_return_code_t M_F(M_P_EXPAND name, _in_serial)(shared_t *, m_serial_read_t); , ) \

#define M_SHAR3D_PTR_DEF_IO(name, shared_t, type, oplist, fattr)              \
M_IF_METHOD(OUT_STR, oplist)(                                                 \
    fattr void M_F(name, _out_str)(FILE *file, const shared_t *out)           \
    {                                                                         \
        M_ASSERT (out != NULL && file != NULL);                               \
        M_F(name, _read_lock)(out);                                           \
        /* No exception */                                                    \
        M_CALL_OUT_STR(oplist, file, out->data);                              \
        M_F(name, _read_unlock)(out);                                         \
    }                                                                         \
, )                                                                           \
M_IF_METHOD(IN_STR, oplist)(                                                  \
    fattr bool M_F(name, _in_str)(shared_t *out, FILE *file)                  \
    {                                                                         \
        M_ASSERT (out != NULL && file != NULL);                               \
        M_GLOBAL_CONTEXT();                                                   \
        M_F(name, _write_lock)(out);                                          \
        m_volatile bool r = false;                                            \
        M_ON_EXCEPTION( M_F(name, _write_unlock)(out) )                       \
            r = M_CALL_IN_STR(oplist, out->data, file);                       \
        /* even if r is false, we signal the data (no functional impact)*/    \
        M_F(name, _write_signal)(out);                                        \
        M_F(name, _write_unlock)(out);                                        \
        return r;                                                             \
    }                                                                         \
, )                                                                           \
M_IF_METHOD(GET_STR, oplist)(                                                 \
    fattr void M_F(name, _get_str)(m_string_t str, const shared_t *out, bool append) \
    {                                                                         \
        M_ASSERT (out != NULL);                                               \
        M_GLOBAL_CONTEXT();                                                   \
        M_F(name, _read_lock)(out);                                           \
        M_ON_EXCEPTION( M_F(name, _read_unlock)(out) )                        \
            M_CALL_GET_STR(oplist, str, out->data, append);                   \
        M_F(name, _read_unlock)(out);                                         \
    }                                                                         \
, )                                                                           \
M_IF_METHOD(PARSE_STR, oplist)(                                               \
    fattr bool M_F(name, _parse_str)(shared_t *out, const char str[], const char **endp) \
    {                                                                         \
        M_ASSERT (out != NULL);                                               \
        M_GLOBAL_CONTEXT();                                                   \
        M_F(name, _write_lock)(out);                                          \
        m_volatile bool r = false;                                            \
        M_ON_EXCEPTION( M_F(name, _write_unlock)(out) )                       \
            r = M_CALL_PARSE_STR(oplist, out->data, str, endp);               \
        /* even if r is false, we signal the data (no functional impact)*/    \
        M_F(name, _write_signal)(out);                                        \
        M_F(name, _write_unlock)(out);                                        \
        return r;                                                             \
    }                                                                         \
, )                                                                           \
M_IF_METHOD(OUT_SERIAL, oplist)(                                              \
    fattr m_serial_return_code_t M_F(name, _out_serial)(M_P_EXPAND m_serial_write_t serial, const shared_t *out) \
    {                                                                         \
        M_ASSERT (out != NULL);                                               \
        M_F(name, _read_lock)(out);                                           \
        m_volatile m_serial_return_code_t r = M_SERIAL_FAIL;                  \
        M_ON_EXCEPTION( M_F(name, _read_unlock)(out) )                        \
            r = M_CALL_OUT_SERIAL(oplist, serial, out->data);                 \
        M_F(name, _read_unlock)(out);                                         \
        return r;                                                             \
    }                                                                         \
, )                                                                           \
M_IF_METHOD(IN_SERIAL, oplist)(                                               \
    fattr m_serial_return_code_t M_F(name, _in_serial)(shared_t *out, m_serial_read_t serial) \
    {                                                                         \
        M_ASSERT (out != NULL);                                               \
        M_GLOBAL_CONTEXT();                                                   \
        M_F(name, _write_lock)(out);                                          \
        m_volatile m_serial_return_code_t r = M_SERIAL_FAIL;                  \
        M_ON_EXCEPTION( M_F(name, _write_unlock)(out) )                       \
            r = M_CALL_IN_SERIAL(oplist, out->data, serial);                  \
        /* even if r is false, we signal the data (no functional impact)*/    \
        M_F(name, _write_signal)(out);                                        \
        M_F(name, _write_unlock)(out);                                        \
        return r;                                                             \
    }                                                                         \
, )                                                                           \


#if M_USE_SMALL_NAME

#define SHARED_WEAK_PTR_DECL M_SHARED_WEAK_PTR_DECL
#define SHARED_WEAK_PTR_DECL_AS M_SHARED_WEAK_PTR_DECL_AS
#define SHARED_PTR_DECL M_SHARED_PTR_DECL
#define SHARED_PTR_DECL_AS M_SHARED_PTR_DECL_AS
#define SHARED_WEAK_PTR_DEF_EXTERN M_SHARED_WEAK_PTR_DEF_EXTERN
#define SHARED_WEAK_PTR_DEF_EXTERN_AS M_SHARED_WEAK_PTR_DEF_EXTERN_AS
#define SHARED_PTR_DEF_EXTERN M_SHARED_PTR_DEF_EXTERN
#define SHARED_PTR_DEF_EXTERN_AS M_SHARED_PTR_DEF_EXTERN_AS
#define SHARED_WEAK_PTR_DEF M_SHARED_WEAK_PTR_DEF
#define SHARED_WEAK_PTR_DEF_AS M_SHARED_WEAK_PTR_DEF_AS
#define SHARED_PTR_DEF M_SHARED_PTR_DEF
#define SHARED_PTR_DEF_AS M_SHARED_PTR_DEF_AS
#define SHARED_PTR_OPLIST M_SHARED_PTR_OPLIST
#define SHARED_DATA_OPLIST M_SHARED_DATA_OPLIST
#endif

#endif
