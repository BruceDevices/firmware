/*
 * M*LIB - Thin stdatomic wrapper for C++ compatibility
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
#ifndef MSTARLIB_ATOMIC_H
#define MSTARLIB_ATOMIC_H

/* NOTE: Due to the C++ not having recognized stdatomic.h officially,
   it is hard to use this header directly with a C++ compiler.
   See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60932
   clang++ has no issue with this header but if someone includes 
   atomic from C++, there is incompatibility between atomic & stdatomic.
   Moreover some compilers lack a working stdatomic header.
   GCC 4.9 doesn't have a working implementation of 'atomic'.
   APPLE Clang defines __GNUC__ to be only 4 despite having full support
   for atomic.
*/
#if defined(__cplusplus) && __cplusplus >= 201103L                            \
  && !(defined(__GNUC__) && __GNUC__ < 5 && !defined(__APPLE__))

/* NOTE: This is what the stdatomic.h header shall do in C++ mode. */
#include <atomic>

using std::memory_order;

using std::atomic_bool;
using std::atomic_char;
using std::atomic_short;
using std::atomic_int;
using std::atomic_long;
using std::atomic_llong;
using std::atomic_uchar;
using std::atomic_schar;
using std::atomic_ushort;
using std::atomic_uint;
using std::atomic_ulong;
using std::atomic_ullong;
using std::atomic_intptr_t;
using std::atomic_uintptr_t;
using std::atomic_size_t;
using std::atomic_ptrdiff_t;
using std::atomic_intmax_t;
using std::atomic_uintmax_t;
using std::atomic_flag;

using std::kill_dependency;
using std::atomic_thread_fence;
using std::atomic_signal_fence;
using std::atomic_is_lock_free;
using std::atomic_store_explicit;
using std::atomic_store;
using std::atomic_load_explicit;
using std::atomic_load;
using std::atomic_exchange_explicit;
using std::atomic_exchange;
using std::atomic_compare_exchange_strong_explicit;
using std::atomic_compare_exchange_strong;
using std::atomic_compare_exchange_weak_explicit;
using std::atomic_compare_exchange_weak;
using std::atomic_fetch_add;
using std::atomic_fetch_add_explicit;
using std::atomic_fetch_sub;
using std::atomic_fetch_sub_explicit;
using std::atomic_fetch_or;
using std::atomic_fetch_or_explicit;
using std::atomic_fetch_xor;
using std::atomic_fetch_xor_explicit;
using std::atomic_fetch_and;
using std::atomic_fetch_and_explicit;
using std::atomic_flag_test_and_set;
using std::atomic_flag_test_and_set_explicit;
using std::atomic_flag_clear;
using std::atomic_flag_clear_explicit;

using std::memory_order_relaxed;
using std::memory_order_consume;
using std::memory_order_acquire;
using std::memory_order_release;
using std::memory_order_acq_rel;
using std::memory_order_seq_cst;

/* CLANG provides a warning on defining _Atomic as it sees it 
 * as a reserved system macro. It is true. However, the goal of this
 * header is to provide stdatomic semantic, so it needs to define
 * _Atomic macro.
 *
 * So, this warning has to be ignored.
 *
 * It cannot use M_BEGIN_PROTECTED_CODE as this header is normally
 * independent of m-core.h
 */
#if defined(__clang__) && __clang_major__ >= 4
  _Pragma("clang diagnostic push")
  _Pragma("clang diagnostic ignored \"-Wreserved-id-macro\"")
#endif

#define _Atomic(T) std::atomic< T >

#if defined(__clang__) && __clang_major__ >= 4
  _Pragma("clang diagnostic pop")
#endif

/* C11 with working stdatomic
   STDATOMIC doesn't work with C++ except for clang but is incompatible with atomic.
   GCC < 4.9 doesn't provide a compliant stdatomic.h
   CLANG 3.5 has issues with GCC's stdatomic.h and doesn't provide its own
   ICC < 18 doesn't provide a compliant stdatomic.h
*/
#elif (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__) ) \
  || (defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER) && !defined(__cplusplus) && (__GNUC__*100 + __GNUC_MINOR__) >= 409) \
  || (defined(__clang__) && (__clang_major__ * 100 + __clang_minor__) >= 308) \
  || (defined(__INTEL_COMPILER) && __INTEL_COMPILER >= 1800)

#include <stdatomic.h>

/* MSYS2 has a conflict between cdefs.h which defines a _Atomic macro (if not C11)
   not compatible with the used stdatomic.h (from GCC).
   Provide a configurable mechanism to undef it with auto-detection of msys2 / gcc */
#ifndef M_USE_UNDEF_ATOMIC
# if defined(__MSYS__) && defined(__GNUC__) && (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L)
#  define M_USE_UNDEF_ATOMIC 1
# endif
#endif
#if defined(M_USE_UNDEF_ATOMIC) && M_USE_UNDEF_ATOMIC == 1
# undef _Atomic
#endif

/* Non working C++ atomic header, nor working stdatomic.h found.
   Write a compatible layer using mutex as slim as possible.
   Supports only up to 64-bits atomic (sizeof long long to be more precise).
   The locks are never properly cleared and remain active until
   the end of the program.
   We also assume that the call to the atomic_* interface is "macro clean".
*/
#else

#include "m-thread.h"
#include "m-core.h"

M_BEGIN_PROTECTED_CODE

/* _Atomic qualifier for a type (emulation).
  The structure is quite large:
   _val     : value of the atomic type,
   _zero    : zero value of the atomic type (constant),
   _previous: temporary value used within the mutex lock,
   _lock    : the mutex lock.
  Support up to sizeof (long long) type.
 */
#define        _Atomic(T)                                                     \
  struct {                                                                    \
    T volatile _val;                                                          \
    T          _zero;                                                         \
    T          _previous;                                                     \
    m_mutex_t  _lock;                                                         \
  }

/* Define the supported memory order.
   Even if memory order is defined, only the strongest constraint is used */
typedef enum {
  memory_order_relaxed,
  memory_order_consume,
  memory_order_acquire,
  memory_order_release,
  memory_order_acq_rel,
  memory_order_seq_cst
} memory_order;

typedef _Atomic(bool)               atomic_bool;
typedef _Atomic(char)               atomic_char;
typedef _Atomic(short)              atomic_short;
typedef _Atomic(int)                atomic_int;
typedef _Atomic(long)               atomic_long;
typedef _Atomic(long long)          atomic_llong;
typedef _Atomic(unsigned char)      atomic_uchar;
typedef _Atomic(signed char)        atomic_schar;
typedef _Atomic(unsigned short)     atomic_ushort;
typedef _Atomic(unsigned int)       atomic_uint;
typedef _Atomic(unsigned long)      atomic_ulong;
typedef _Atomic(unsigned long long) atomic_ullong;
typedef _Atomic(intptr_t)           atomic_intptr_t;
typedef _Atomic(uintptr_t)          atomic_uintptr_t;
typedef _Atomic(size_t)             atomic_size_t;
typedef _Atomic(ptrdiff_t)          atomic_ptrdiff_t;

/* Detect if stdint.h was included */
#if (defined (INTMAX_C) && defined (UINTMAX_C) && !defined(__cplusplus)) ||   \
  defined (_STDINT_H) || defined (_STDINT_H_) || defined (_STDINT) ||         \
  defined (_SYS_STDINT_H_)
/* Define additional atomic types */
typedef _Atomic(intmax_t)           atomic_intmax_t;
typedef _Atomic(uintmax_t)          atomic_uintmax_t;
#endif

/* (INTERNAL) Unlock the mutex and return the given value */
M_INLINE long long atomic_fetch_unlock (m_mutex_t *lock, long long val)
{
  m_mutex_unlock (*lock);
  return val;
}

/* (INTERNAL) This is the heart of the wrapper:
   lock the atomic value, read it and returns the value.
   In order to avoid any compiler extension, we need to transform the
   atomic type into 'long long' then convert it back to its value.
   This is because _previous can't be read after the lock, and we can't
   generate temporary variable within a macro.
   The trick is computing _val - _zero within the lock, then
   returns retvalue + _zero after the release of the lock.
*/
#define atomic_fetch_op(ptr, val, op)                                         \
  (m_mutex_lock((ptr)->_lock),                                                \
   (ptr)->_previous = (ptr)->_val,                                            \
   (ptr)->_val op (val),                                                      \
   atomic_fetch_unlock(&(ptr)->_lock, (long long)((ptr)->_previous-(ptr)->_zero))+(ptr)->_zero)

/* Perform an atomic add (EMULATION) */
#define atomic_fetch_add(ptr, val) atomic_fetch_op(ptr, val, +=)
/* Perform an atomic sub (EMULATION) */
#define atomic_fetch_sub(ptr, val) atomic_fetch_op(ptr, val, -=)
/* Perform an atomic or (EMULATION) */
#define atomic_fetch_or(ptr, val)  atomic_fetch_op(ptr, val, |=)
/* Perform an atomic xor (EMULATION) */
#define atomic_fetch_xor(ptr, val) atomic_fetch_op(ptr, val, ^=)
/* Perform an atomic and (EMULATION) */
#define atomic_fetch_and(ptr, val) atomic_fetch_op(ptr, val, &=)
/* Perform an atomic exchange (EMULATION) */
#define atomic_exchange(ptr, val)  atomic_fetch_op(ptr, val, =)

/* Initialize an atomic GLOBAL variable */
#define ATOMIC_VAR_INIT(val) { val, 0, 0, M_MUTEXI_INIT_VALUE }

/* Initialize an atomic variable */
#define atomic_init(ptr, val)                                                 \
  (m_mutex_init((ptr)->_lock), (ptr)->_val = val, (ptr)->_zero = 0)

/* Atomic load of a variable (EMULATION) */
#define atomic_load(ptr)                                                      \
  (m_mutex_lock((ptr)->_lock),                                                \
   (ptr)->_previous = (ptr)->_val,                                            \
   atomic_fetch_unlock(&(ptr)->_lock, (long long) ((ptr)->_previous-(ptr)->_zero))+(ptr)->_zero)
  
/* Atomic store of a variable (EMULATION) */
#define atomic_store(ptr, val)                                                \
  (m_mutex_lock((ptr)->_lock),                                                \
   (ptr)->_val = (val),                                                       \
   m_mutex_unlock((ptr)->_lock))

/* Perform a CAS (Compare and swap) operation (EMULATION) */
#define atomic_compare_exchange_strong(ptr, exp, val)                         \
  (m_mutex_lock((ptr)->_lock),                                                \
   atomic_fetch_unlock(&(ptr)->_lock,                                         \
                       (ptr)->_val == *(exp)                                  \
                       ? ((ptr)->_val = (val), true)                          \
                       : (*(exp) = (ptr)->_val, false)))

  
#define atomic_fetch_add_explicit(ptr, val, mem) atomic_fetch_op(ptr, val, +=)
#define atomic_fetch_sub_explicit(ptr, val, mem) atomic_fetch_op(ptr, val, -=)
#define atomic_fetch_or_explicit(ptr, val, mem)  atomic_fetch_op(ptr, val, |=)
#define atomic_fetch_xor_explicit(ptr, val, mem) atomic_fetch_op(ptr, val, ^=)
#define atomic_fetch_and_explicit(ptr, val, mem) atomic_fetch_op(ptr, val, &=)
#define atomic_exchange_explicit(ptr, val, mem)  atomic_fetch_op(ptr, val, =)
#define atomic_load_explicit(ptr, mem)           atomic_load(ptr)
#define atomic_store_explicit(ptr, val, mem)     atomic_store(ptr, val)
#define kill_dependency(ptr)                     atomic_load(ptr)
#define atomic_thread_fence(mem)                 (void) 0
#define atomic_signal_fence(mem)                 (void) 0
#define atomic_is_lock_free(ptr)                 false
#define atomic_compare_exchange_strong_explicit(ptr, exp, val, mem1, mem2) atomic_compare_exchange_strong(ptr, exp, val)
#define atomic_compare_exchange_weak_explicit(ptr, exp, val, mem1, mem2)   atomic_compare_exchange_strong(ptr, exp, val)
#define atomic_compare_exchange_weak(ptr, exp, val)                        atomic_compare_exchange_strong(ptr, exp, val)

/* TODO: Missing atomic_flag. Problem: it is supposed to be lock free! */

M_END_PROTECTED_CODE

#endif

// C17 deprecated ATOMIC_VAR_INIT
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201710L
# define M_ATOMIC_VAR_INIT(x) (x)
#else
# define M_ATOMIC_VAR_INIT(x) ATOMIC_VAR_INIT(x)
#endif

#endif
