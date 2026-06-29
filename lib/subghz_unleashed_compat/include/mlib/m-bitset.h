/*
 * M*LIB - BITSET module
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
#ifndef MSTARLIB_BITSET_H
#define MSTARLIB_BITSET_H

#include <stdint.h>
#include "m-core.h"

/********************************** INTERNAL ************************************/

M_BEGIN_PROTECTED_CODE

// Define the basic limb of a bitset
typedef uint64_t m_b1tset_limb_ct;
// And its size in bits
#define M_B1TSET_LIMB_BIT (sizeof(m_b1tset_limb_ct) * CHAR_BIT)

// bitset grow policy. n is limb size
#define M_B1TSET_INC_ALLOC_SIZE(n) ((n) < 4 ? 4 : (n) * 2)

// Compute the number of allocated limbs needed to handle 'n' bits.
#define M_B1TSET_TO_ALLOC(n)       (((n) + M_B1TSET_LIMB_BIT - 1) / M_B1TSET_LIMB_BIT)

// Compute the number of bits available from the allocated size in limbs
#define M_B1TSET_FROM_ALLOC(n)     ((n) * M_B1TSET_LIMB_BIT)

// Contract of a bitset
#define M_B1TSET_CONTRACT(t) do {                                             \
    M_ASSERT (t != NULL);                                                     \
    M_ASSERT (t->size <= M_B1TSET_FROM_ALLOC (t->alloc));                     \
    M_ASSERT (t->alloc <= ((size_t)-1) / M_B1TSET_LIMB_BIT);                  \
    M_ASSERT (t->size < ((size_t)-1) - M_B1TSET_LIMB_BIT);                    \
    M_ASSERT (t->size == 0 || t->ptr != NULL);                                \
    M_ASSERT (t->alloc == 0 || t->ptr != NULL);                               \
    M_ASSERT ((t->size % M_B1TSET_LIMB_BIT) == 0 || (t->ptr[ (t->size-1) / M_B1TSET_LIMB_BIT] & ~(((((m_b1tset_limb_ct)1)<<(t->size % M_B1TSET_LIMB_BIT))<<1)-1)) == 0); \
  } while (0)


/********************************** EXTERNAL ************************************/

/* Define a type of variable 'bits' or array of packed booleans */
typedef struct m_bitset_s {
  size_t size;            // Size is the number of bits
  size_t alloc;           // Alloc is the number of allocated limbs
  m_b1tset_limb_ct *ptr;  // Pointer to the allocated limbs
} m_bitset_t[1];

/* Pointer to a m_bitset_t */
typedef struct m_bitset_s *m_bitset_ptr;

/* Constant Pointer to a m_bitset_t */
typedef const struct m_bitset_s *m_bitset_srcptr;

/* Iterator on a bitset */
typedef struct m_bitset_it_s {
  size_t index;           // index to the array of bit
  bool   value;           // value used for _ref & _cref to store the value
  struct m_bitset_s *set; // the associated bitset
} m_bitset_it_t[1];

/* Initialize a bitset (CONSTRUCTOR) */
M_INLINE void
m_bitset_init(m_bitset_t t)
{
  M_ASSERT (t != NULL);
  M_STATIC_ASSERT (M_POWEROF2_P(M_B1TSET_LIMB_BIT), MLIB_INTERNAL, "M*LIB: BITSET LIMB shall be a power of 2.");
  t->size = 0;
  t->alloc = 0;
  t->ptr = NULL;
  M_B1TSET_CONTRACT(t);
}

/* Clean a bitset */
M_INLINE void
m_bitset_reset(m_bitset_t t)
{
  M_B1TSET_CONTRACT(t);
  t->size = 0;
}

/* Clear a bitset (DESTRUCTOR) */
M_P(void, m_bitset, _clear, m_bitset_t t)
{
  m_bitset_reset(t);
  M_MEMORY_FREE(m_context, m_b1tset_limb_ct, t->ptr, t->alloc);
  // This is not really needed, but is safer
  // This representation is invalid and will be detected by the contract.
  // A C compiler should be able to optimize out theses initializations.
  t->alloc = 1;
  t->ptr = NULL;
}

/* Set a bitset to another one */
M_P(void, m_bitset, _set, m_bitset_t d, const m_bitset_t s)
{
  M_B1TSET_CONTRACT(d);
  M_B1TSET_CONTRACT(s);
  if (M_UNLIKELY (d == s)) return;
  const size_t needAlloc = M_B1TSET_TO_ALLOC (s->size);
  if (M_LIKELY (s->size > 0)) {
    // Test if enough space in target
    if (s->size > M_B1TSET_FROM_ALLOC (d->alloc)) {
      m_b1tset_limb_ct *ptr = M_MEMORY_REALLOC (m_context, m_b1tset_limb_ct, d->ptr, d->alloc, needAlloc);
      if (M_UNLIKELY_NOMEM (ptr == NULL)) {
        M_MEMORY_FULL(m_b1tset_limb_ct, needAlloc);
      }
      d->ptr = ptr;
      d->alloc = needAlloc;
    }
    M_ASSERT(d->ptr != NULL);
    M_ASSERT(s->ptr != NULL);
    memcpy (d->ptr, s->ptr, needAlloc * sizeof(m_b1tset_limb_ct) );
  }
  d->size = s->size;
  M_B1TSET_CONTRACT(d);
}

/* Initialize & set a bitset to another one (CONSTRUCTOR) */
M_P(void, m_bitset, _init_set, m_bitset_t d, const m_bitset_t s)
{
  M_ASSERT (d != s);
  // In case of exception, d remains initialized,
  // but without any allocation done.
  // Therefore it is safe, not to clear d on exception.
  m_bitset_init(d);
  m_bitset_set M_R(d, s);
}

/* Initialize & move a bitset (CONSTRUCTOR) from another one (DESTRUCTOR) */
M_INLINE void
m_bitset_init_move(m_bitset_t d, m_bitset_t s)
{
  M_B1TSET_CONTRACT(s);
  d->size  = s->size;
  d->alloc = s->alloc;
  d->ptr   = s->ptr;
  // Illegal representation of a bitset, to be detectable
  s->alloc = 1;
  s->ptr = NULL;
  M_B1TSET_CONTRACT(d);  
}

/* Move a bitset from another one (DESTRUCTOR) */
M_P(void, m_bitset, _move, m_bitset_t d, m_bitset_t s)
{
  m_bitset_clear M_R(d);
  m_bitset_init_move (d, s);
}

/* Set the bit 'i' in the bitset to the value 'x' */
M_INLINE void
m_bitset_set_at(m_bitset_t v, size_t i, bool x)
{
  M_B1TSET_CONTRACT(v);
  M_ASSERT (v->ptr != NULL);
  M_ASSERT_INDEX(i, v->size);
  const size_t offset = i / M_B1TSET_LIMB_BIT;
  const size_t index  = i % M_B1TSET_LIMB_BIT;
  // This is a branchless version as x can only be 0 or 1 with only one variable shift.
  const m_b1tset_limb_ct mask = ((m_b1tset_limb_ct)1)<<index;
  v->ptr[offset] = (v->ptr[offset] & ~mask) | (mask & (0-(m_b1tset_limb_ct)x));
  M_B1TSET_CONTRACT (v);
}

/* Flip the bit 'i' in the bitset */
M_INLINE void
m_bitset_flip_at(m_bitset_t v, size_t i)
{
  M_B1TSET_CONTRACT(v);
  M_ASSERT (v->ptr != NULL);
  M_ASSERT_INDEX(i, v->size);
  size_t offset = i / M_B1TSET_LIMB_BIT;
  size_t index  = i % M_B1TSET_LIMB_BIT;
  v->ptr[offset] ^= ((m_b1tset_limb_ct)1)<<index;
  M_B1TSET_CONTRACT (v);
}

/* Push back the boolean 'x' in the bitset (increasing the bitset) */
M_P(void, m_bitset, _push_back, m_bitset_t v, bool x)
{
  M_B1TSET_CONTRACT (v);
  if (M_UNLIKELY (v->size >= M_B1TSET_FROM_ALLOC (v->alloc))) {
    // Compute the needed allocation.
    const size_t needAlloc = M_B1TSET_INC_ALLOC_SIZE(v->alloc);
    // Check for integer overflow
    if (M_UNLIKELY_NOMEM (needAlloc <= v->alloc)) {
      M_MEMORY_FULL(m_b1tset_limb_ct, needAlloc);
    }
    // Alloc memory
    m_b1tset_limb_ct *ptr = M_MEMORY_REALLOC (m_context, m_b1tset_limb_ct, v->ptr, v->alloc, needAlloc);
    // Check if success
    if (M_UNLIKELY_NOMEM (ptr == NULL) ) {
      M_MEMORY_FULL(m_b1tset_limb_ct, needAlloc);
    }
    v->ptr = ptr;
    v->alloc = needAlloc;
  }
  M_ASSERT(v->ptr != NULL);

  const size_t i = v->size;
  const size_t offset = i / M_B1TSET_LIMB_BIT;
  const size_t index  = i % M_B1TSET_LIMB_BIT;
  if (M_UNLIKELY(index == 0)) {
    // A new limb if used. Clear it before using it.
    v->ptr[offset] = 0;
  }
  // This is a branchless version as x can only be 0 or 1 with only one variable shift.
  const m_b1tset_limb_ct mask = ((m_b1tset_limb_ct)1)<<index;
  v->ptr[offset] = (v->ptr[offset] & ~mask) | (mask & (0-(m_b1tset_limb_ct)x));
  v->size ++;
  M_B1TSET_CONTRACT (v);
}

/* Resize the bitset to have exactly 'size' bits */
M_P(void, m_bitset, _resize, m_bitset_t v, size_t size)
{
  M_B1TSET_CONTRACT (v);
  // Check for overflow
  if (M_UNLIKELY_NOMEM (size >= ((size_t)-1) - M_B1TSET_LIMB_BIT)) {
    M_MEMORY_FULL(char, -(size_t)1);
  }
  // Compute the needed allocation.
  size_t newAlloc = M_B1TSET_TO_ALLOC (size);
  if (newAlloc > v->alloc) {
    // Allocate more limbs to store the bitset.
    m_b1tset_limb_ct *ptr = M_MEMORY_REALLOC (m_context, m_b1tset_limb_ct, v->ptr, v->alloc, newAlloc);
    if (M_UNLIKELY_NOMEM (ptr == NULL) ) {
      M_MEMORY_FULL(m_b1tset_limb_ct, newAlloc);
    }
    v->ptr = ptr;
    v->alloc = newAlloc;
  }
  // Resize the bitsets
  const size_t old_size = v->size;
  const size_t offset = size / M_B1TSET_LIMB_BIT;
  const size_t index  = size % M_B1TSET_LIMB_BIT;
  const m_b1tset_limb_ct mask = (((m_b1tset_limb_ct)1)<<index)-1;
  if (size < old_size) {
    // Resize down the bitset: clear unused bits
    if (M_LIKELY(index != 0)) {
      // Mask the last limb to clear the last bits
      v->ptr[offset] &= mask;
    }
  } else if (size > old_size) {
    // Resize up the bitset: set to 0 new bits.
    const size_t old_offset = (old_size + M_B1TSET_LIMB_BIT - 1)/ M_B1TSET_LIMB_BIT;
    for(size_t i = old_offset ; i < offset; i++) {
      v->ptr[i] = 0;
    }
    if (M_LIKELY(index != 0)) {
      // Mask the last limb to clear the last bits
      v->ptr[offset] = 0;
    }
  }
  v->size = size;
  M_B1TSET_CONTRACT (v);
}

/* Reserve allocation in the bitset to accommodate at least 'size' bits without reallocation */
M_P(void, m_bitset, _reserve, m_bitset_t v, size_t alloc)
{
  M_B1TSET_CONTRACT (v);
  size_t oldAlloc = M_B1TSET_TO_ALLOC (v->size);
  size_t newAlloc = M_B1TSET_TO_ALLOC (alloc);
  // We refuse to reduce allocation below current size
  if (oldAlloc > newAlloc) {
    newAlloc = oldAlloc;
  }
  if (M_UNLIKELY (newAlloc == 0)) {
    // Free all memory used by the bitsets
    M_MEMORY_FREE (m_context, m_b1tset_limb_ct, v->ptr, v->alloc);
    v->size = v->alloc = 0;
    v->ptr = NULL;
  } else {
    // Allocate more memory or reduce memory usage
    m_b1tset_limb_ct *ptr = M_MEMORY_REALLOC (m_context, m_b1tset_limb_ct, v->ptr, v->alloc, newAlloc);
    if (M_UNLIKELY_NOMEM (ptr == NULL) ) {
      M_MEMORY_FULL(m_b1tset_limb_ct, newAlloc);
    }
    v->ptr = ptr;
    v->alloc = newAlloc;
  }
  M_B1TSET_CONTRACT (v);
}

/* Return the value of the boolean at index 'i'.    
 * NOTE: Interface is a little bit different:
 * It doesn't return a pointer to the data, but the data itself.
 */
M_INLINE bool
m_bitset_get(const m_bitset_t v, size_t i)
{
  M_B1TSET_CONTRACT(v);
  M_ASSERT (v->ptr != NULL);
  M_ASSERT_INDEX(i, v->size);
  size_t offset = i / M_B1TSET_LIMB_BIT;
  size_t index  = i % M_B1TSET_LIMB_BIT;
  return ( v->ptr[offset] & (((m_b1tset_limb_ct)1) << index) ) != 0;
}

/* m_bitset_cget is the exact same service than m_bitset_get */
#define m_bitset_cget m_bitset_get

/* Pop back the last bit in the bitset */
M_INLINE void
m_bitset_pop_back(bool *dest, m_bitset_t v)
{
  M_B1TSET_CONTRACT (v);
  M_ASSERT_INDEX (0, v->size);
  // Remove one item from the bitset
  v->size--;
  // Prepare clearing popped bit
  const size_t offset = v->size / M_B1TSET_LIMB_BIT;
  const size_t index  = v->size % M_B1TSET_LIMB_BIT;
  const m_b1tset_limb_ct mask = ((m_b1tset_limb_ct)1)<<index;
  if (dest) {
    // Read popped bit
    *dest = (v->ptr[offset] & mask) != 0;
  }
  v->ptr[offset] &= mask-1;
  M_B1TSET_CONTRACT (v);
}

/* Return the front bit value in the bitset */
M_INLINE bool
m_bitset_front(const m_bitset_t v)
{
  M_B1TSET_CONTRACT (v);
  M_ASSERT_INDEX (0, v->size);
  return m_bitset_get(v, 0);
}

/* Return the back bit value in the bitset */
M_INLINE bool
m_bitset_back(const m_bitset_t v)
{
  M_B1TSET_CONTRACT (v);
  M_ASSERT_INDEX (0, v->size);
  return m_bitset_get(v, v->size-1);
}

/* Test if the bitset is empty (no bits stored)*/
M_INLINE bool
m_bitset_empty_p(const m_bitset_t v)
{
  M_B1TSET_CONTRACT (v);
  return v->size == 0;
}

/* Return the number of bits of the bitset */
M_INLINE size_t
m_bitset_size(const m_bitset_t v)
{
  M_B1TSET_CONTRACT (v);
  return v->size;
}

/* Return the capacity in bits of the bitset */
M_INLINE size_t
m_bitset_capacity(const m_bitset_t v)
{
  M_B1TSET_CONTRACT (v);
  return M_B1TSET_FROM_ALLOC (v->alloc);
}

/* Swap the bit at index i and j of the bitset */
M_INLINE void
m_bitset_swap_at (m_bitset_t v, size_t i, size_t j)
{
  M_ASSERT_INDEX(i, v->size);
  M_ASSERT_INDEX(j, v->size);

  bool i_val = m_bitset_get(v, i);
  bool j_val = m_bitset_get(v, j);
  m_bitset_set_at (v, i, j_val);
  m_bitset_set_at (v, j, i_val);
}

/* Swap the bitsets */
M_INLINE void
m_bitset_swap (m_bitset_t v1, m_bitset_t v2)
{
  M_B1TSET_CONTRACT (v1);
  M_B1TSET_CONTRACT (v2);
  M_SWAP (size_t, v1->size, v2->size);
  M_SWAP (size_t, v1->alloc, v2->alloc);
  M_SWAP (m_b1tset_limb_ct *, v1->ptr, v2->ptr);
  M_B1TSET_CONTRACT (v1);
  M_B1TSET_CONTRACT (v2);
}

/* (INTERNAL) Left shift of the bitset (ptr+size) by 1 bit,
 * integrating the carry in the lowest position.
 * Return the new carry.
 */
M_INLINE m_b1tset_limb_ct
m_b1tset_lshift(m_b1tset_limb_ct ptr[], size_t n, m_b1tset_limb_ct carry)
{
  for(size_t i = 0; i < n; i++) {
    m_b1tset_limb_ct v = ptr[i];
    ptr[i] = (v << 1) | carry;
    carry = (v >> (M_B1TSET_LIMB_BIT-1) );
  }
  return carry;
}

/* (INTERNAL) Right shift of the bitset (ptr+size) by 1 bit,
 * integrating the carry in the lowest position.
 * Return the new carry.
 */
M_INLINE m_b1tset_limb_ct
m_b1tset_rshift(m_b1tset_limb_ct ptr[], size_t n, m_b1tset_limb_ct carry)
{
  for(size_t i = n - 1; i < n; i--) {
    m_b1tset_limb_ct v = ptr[i];
    ptr[i] = (v >> 1) | (carry << (M_B1TSET_LIMB_BIT-1) );
    carry = v & 1;
  }
  return carry;
}

/* Insert a new bit at position 'key' of value 'value' in the bitset 'set'
  shifting the set accordingly */
M_P(void, m_bitset, _push_at, m_bitset_t set, size_t key, bool value)
{
  M_B1TSET_CONTRACT (set);
  // First push another value to extend the array to the right size
  m_bitset_push_back M_R(set, false);
  M_ASSERT (set->ptr != NULL);
  M_ASSERT_INDEX(key, set->size);

  // Then shift it
  size_t offset = key / M_B1TSET_LIMB_BIT;
  size_t index  = key % M_B1TSET_LIMB_BIT;
  m_b1tset_limb_ct v = set->ptr[offset];
  m_b1tset_limb_ct mask = (((m_b1tset_limb_ct)1)<<index)-1;
  m_b1tset_limb_ct carry = (v >> (M_B1TSET_LIMB_BIT-1) );
  v = (v & mask) | ((unsigned int) value << index) | ((v & ~mask) << 1);
  set->ptr[offset] = v;
  size_t size = (set->size + M_B1TSET_LIMB_BIT - 1) / M_B1TSET_LIMB_BIT;
  M_ASSERT (size >= offset + 1);
  v = m_b1tset_lshift(&set->ptr[offset+1], size - offset - 1, carry);
  // v is unused as it should be zero.
  M_ASSERT(v == 0);
  (void) v;
  M_B1TSET_CONTRACT (set);
}

/* Pop a new bit at position 'key' in the bitset 
 * and return in *dest its value if *dest exists */
M_INLINE void
m_bitset_pop_at(bool *dest, m_bitset_t set, size_t key)
{
  M_B1TSET_CONTRACT (set);
  M_ASSERT (set->ptr != NULL);
  M_ASSERT_INDEX(key, set->size);

   if (dest) {
     *dest = m_bitset_get (set, key);
   }
   // Shift it
   size_t offset = key / M_B1TSET_LIMB_BIT;
   size_t index  = key % M_B1TSET_LIMB_BIT;
   size_t size = (set->size + M_B1TSET_LIMB_BIT - 1) / M_B1TSET_LIMB_BIT;
   m_b1tset_limb_ct v, mask, carry;
   carry = m_b1tset_rshift(&set->ptr[offset+1], size - offset - 1, false);
   v = set->ptr[offset];
   mask = (((m_b1tset_limb_ct)1)<<index)-1;
   v = (v & mask) | ((v>>1) & ~mask) | (carry << (M_B1TSET_LIMB_BIT-1)) ;
   set->ptr[offset] = v;
   // Decrease size
   set->size --;
   M_B1TSET_CONTRACT (set);
}

/* Test if two bitsets are equal */
M_INLINE bool
m_bitset_equal_p (const m_bitset_t set1, const m_bitset_t set2)
{
  M_B1TSET_CONTRACT (set1);
  M_B1TSET_CONTRACT (set2);
  if (set1->size != set2->size)
    return false;
  /* We won't compare each bit individually,
     but instead compare them per limb */
  const size_t limbSize = (set1->size + M_B1TSET_LIMB_BIT -1) / M_B1TSET_LIMB_BIT;
  for(size_t i = 0 ; i < limbSize;i++)
    if (set1->ptr[i] != set2->ptr[i])
      return false;
  return true;
}

/* Initialize an iterator to the first bit of the bitset */
M_INLINE void
m_bitset_it(m_bitset_it_t it, m_bitset_t set)
{
   M_B1TSET_CONTRACT (set);
   it->index = 0;
   it->set = set;
}

/* Initialize an iterator to reference the same bit as the given one*/
M_INLINE void
m_bitset_it_set(m_bitset_it_t it, const m_bitset_it_t itorg)
{
  M_ASSERT (it != NULL && itorg != NULL);
  it->index = itorg->index;
  it->set = itorg->set;
}

/* Initialize an iterator to reference the last bit of the bitset*/
M_INLINE void
m_bitset_it_last(m_bitset_it_t it, m_bitset_t set)
{
   M_B1TSET_CONTRACT (set);
   it->index = set->size-1;
   it->set = set;
}

/* Initialize an iterator to reference no valid bit of the bitset*/
M_INLINE void
m_bitset_it_end(m_bitset_it_t it, m_bitset_t set)
{
   M_B1TSET_CONTRACT (set);
   it->index = set->size;
   it->set = set;
}

/* Test if an iterator references no valid bit of the bitset anymore */
M_INLINE bool
m_bitset_end_p(const m_bitset_it_t it)
{
  M_ASSERT (it != NULL && it->set != NULL);
  return (it->index) >= (it->set->size);
}

/* Test if an iterator references the last (or end) bit of the bitset anymore */
M_INLINE bool
m_bitset_last_p(const m_bitset_it_t it)
{
  M_ASSERT (it != NULL && it->set != NULL);
  /* NOTE: Can not compute 'size-1' due to potential overflow
     if size is 0 */
  return (it->index+1) >= (it->set->size);
}

/* Test if both iterators reference the same bit */
M_INLINE bool
m_bitset_it_equal_p(const m_bitset_it_t it1, const m_bitset_it_t it2)
{
  M_ASSERT (it1 != NULL && it2 != NULL);
  return it1->index == it2->index && it1->set == it2->set;
}

/* Move the iterator to the next bit */
M_INLINE void
m_bitset_next(m_bitset_it_t it)
{
  M_ASSERT (it != NULL && it->set != NULL);
  it->index++;
}

/* Move the iterator to the previous bit */
M_INLINE void
m_bitset_previous(m_bitset_it_t it)
{
  M_ASSERT (it != NULL && it->set != NULL);
  it->index--;
}

// There is no _ref as it is not possible to modify the value using the IT interface

/* Return a pointer to the bit referenced by the iterator 
 * Only one reference is possible at a time per iterator */
M_INLINE const bool *
m_bitset_cref(m_bitset_it_t it)
{
  M_ASSERT (it != NULL && it->set != NULL);
  it->value = m_bitset_get(it->set, it->index);
  return &it->value;
}

/* Output the bitset as a formatted text in a FILE */
M_INLINE void
m_bitset_out_str(FILE *file, const m_bitset_t set)
{
  M_B1TSET_CONTRACT (set);
  M_ASSERT(file != NULL);
  fputc ('[', file);
  for(size_t i = 0; i < set->size; i++) {
    const bool b = m_bitset_get (set, i);
    const char c = b ? '1' : '0';
    fputc (c, file);
  }
  fputc (']', file);
}

/* Input the bitset from a formatted text in a FILE */
M_P(bool, m_bitset, _in_str, m_bitset_t set, FILE *file)
{
  M_B1TSET_CONTRACT (set);
  M_ASSERT(file != NULL);
  m_bitset_reset(set);
  int c = fgetc(file);
  if (M_UNLIKELY (c != '[')) return false;
  c = fgetc(file);
  while (c == '0' || c == '1') {
    const bool b = (c == '1');
    m_bitset_push_back M_R(set, b);
    c = fgetc(file);
  }
  M_B1TSET_CONTRACT (set);
  return c == ']';
}

/* Parse the bitset from a formatted text in a C string */
M_P(bool, m_bitset, _parse_str, m_bitset_t set, const char str[], const char **endptr)
{
  M_B1TSET_CONTRACT (set);
  M_ASSERT(str != NULL);
  bool success = false;
  m_bitset_reset(set);
  char c = *str++;
  if (M_UNLIKELY(c != '[')) goto exit;
  c = *str++;
  do {
    if (M_UNLIKELY(c != '0' && c != '1')) goto exit;
    const bool b = (c == '1');
    m_bitset_push_back M_R(set, b);
    c = *str++;
  } while (c != ']' && c != 0);
  M_B1TSET_CONTRACT (set);
  success = (c == ']');
 exit:
  if (endptr) *endptr = str;
  return success;
}

/* Set the bitset from a formatted text in a C string */
M_P(bool, m_bitset, _set_str, m_bitset_t dest, const char str[])
{
  return m_bitset_parse_str M_R(dest, str, NULL);
}

/* Perform an AND operation between the bitsets,
 * up to the minimum size of both bitsets */
M_INLINE void
m_bitset_and(m_bitset_t dest, const m_bitset_t src)
{
  M_B1TSET_CONTRACT(dest);
  M_B1TSET_CONTRACT(src);
  size_t s = M_MIN(dest->size, src->size);
  size_t n = (s + M_B1TSET_LIMB_BIT -1) / M_B1TSET_LIMB_BIT;
  for(size_t i = 0 ; i < n; i++)
    dest->ptr[i] &= src->ptr[i];
  // Reduce the dest size to the minimum size between both
  dest->size = s;
  M_B1TSET_CONTRACT(dest);
}

/* Perform an OR operation between the bitsets,
 * up to the minimum size of both bitsets */
M_INLINE void
m_bitset_or(m_bitset_t dest, const m_bitset_t src)
{
  M_B1TSET_CONTRACT(dest);
  M_B1TSET_CONTRACT(src);
  size_t s = M_MIN(dest->size, src->size);
  size_t n = (s + M_B1TSET_LIMB_BIT - 1) / M_B1TSET_LIMB_BIT;
  for(size_t i = 0 ; i < n; i++)
    dest->ptr[i] |= src->ptr[i];
  // Reduce the dest size to the minimum size between both
  dest->size = s;
  M_B1TSET_CONTRACT(dest);
}

/* Perform an XOR operation between the bitsets,
 * up to the minimum size of both bitsets */
M_INLINE void
m_bitset_xor(m_bitset_t dest, const m_bitset_t src)
{
  M_B1TSET_CONTRACT(dest);
  M_B1TSET_CONTRACT(src);
  size_t s = M_MIN(dest->size, src->size);
  size_t n = s / M_B1TSET_LIMB_BIT;
  size_t m = s % M_B1TSET_LIMB_BIT;
  for(size_t i = 0 ; i < n; i++)
    dest->ptr[i] ^= src->ptr[i];
  if (M_LIKELY(m)) {
    // Last limb needs to be masked too
    m_b1tset_limb_ct mask = (((m_b1tset_limb_ct)1) << m) - 1;
    dest->ptr[n] = (dest->ptr[n] ^ src->ptr[n]) & mask;
  }
  // Reduce the dest size to the minimum size between both
  dest->size = s;
  M_B1TSET_CONTRACT(dest);
}

/* Perform a NOT operation of the bitset */
M_INLINE void
m_bitset_not(m_bitset_t dest)
{
  M_B1TSET_CONTRACT(dest);
  size_t s = dest->size;
  size_t n = s / M_B1TSET_LIMB_BIT;
  size_t m = s % M_B1TSET_LIMB_BIT;
  for(size_t i = 0 ; i < n; i++)
    dest->ptr[i] = ~ (dest->ptr[i]);
  if (M_LIKELY(m)) {
    // Last limb needs to be masked too
    m_b1tset_limb_ct mask = (((m_b1tset_limb_ct)1) << m) - 1;
    dest->ptr[n] = (~ dest->ptr[n]) & mask;
  }
  M_B1TSET_CONTRACT(dest);
}

/* Compute a hash of the bitset */
M_INLINE size_t
m_bitset_hash(const m_bitset_t set)
{
  M_B1TSET_CONTRACT(set);
  size_t s = set->size;
  size_t n = (s + M_B1TSET_LIMB_BIT-1) / M_B1TSET_LIMB_BIT;
  M_HASH_DECL(hash);
  for(size_t i = 0 ; i < n; i++)
    M_HASH_UP(hash, set->ptr[i]);
  return M_HASH_FINAL (hash);
}

/* Count the number of leading zero */
M_INLINE size_t
m_bitset_clz(const m_bitset_t set)
{
  M_B1TSET_CONTRACT(set);
  size_t s = set->size;
  if (M_UNLIKELY (s == 0)) {
    return 0;
  }
  size_t n = (s -1) / M_B1TSET_LIMB_BIT;
  size_t m = s % M_B1TSET_LIMB_BIT;
  m_b1tset_limb_ct limb = set->ptr[n];
  if (m) {
    m_b1tset_limb_ct mask = (((m_b1tset_limb_ct)1) << m) - 1;
    limb &= mask;
  } else {
    m = M_B1TSET_LIMB_BIT;
  }
  s = 0;
  while (limb == 0 && n > 0) {
    s += m;
    limb = set->ptr[--n];
    m = M_B1TSET_LIMB_BIT;
  }
  s += m_core_clz64(limb) - (M_B1TSET_LIMB_BIT - m);
  return s;
}

/* Count the number of trailing zero */
M_INLINE size_t
m_bitset_ctz(const m_bitset_t set)
{
  M_B1TSET_CONTRACT(set);
  size_t s = set->size;
  if (M_UNLIKELY (s == 0)) {
    return 0;
  }
  size_t i = 0, n = (s -1) / M_B1TSET_LIMB_BIT;
  size_t m = s % M_B1TSET_LIMB_BIT;
  m_b1tset_limb_ct limb = set->ptr[0];
  s = 0;
  while (limb == 0 && i < n) {
    s += M_B1TSET_LIMB_BIT;
    limb = set->ptr[++i];
  }
  if (i == n && m != 0) {
    m_b1tset_limb_ct mask = (((m_b1tset_limb_ct)1) << m) - 1;
    limb &= mask;
  }
  unsigned ctz = m_core_ctz64(limb);
  s += (ctz == 64) ? m : ctz;
  return s;
}

// For GCC or CLANG or ICC
#if defined(__GNUC__)
M_INLINE size_t m_b1tset_popcount64(m_b1tset_limb_ct limb)
{
  return (size_t) __builtin_popcountll(limb);
}
#else
// MSVC __popcnt64 may not exist on the target architecture (no emulation layer)
// Use emulation layer: https://en.wikipedia.org/wiki/Hamming_weight
M_INLINE size_t m_b1tset_popcount64(m_b1tset_limb_ct limb)
{
  limb = limb - ((limb >> 1) & 0x5555555555555555ULL);
  limb = (limb & 0x3333333333333333ULL) + ((limb >> 2) & 0x3333333333333333ULL);
  limb = (limb + (limb >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
  return (limb * 0x0101010101010101ULL) >> 56;
}
#endif

/* Count the number of 1 */
M_INLINE size_t
m_bitset_popcount(const m_bitset_t set)
{
  M_B1TSET_CONTRACT(set);
  size_t s = 0;
  size_t n = (set->size + M_B1TSET_LIMB_BIT - 1) / M_B1TSET_LIMB_BIT;
  for(size_t i = 0 ; i < n; i++)
    s += m_b1tset_popcount64(set->ptr[i]);
  return s;
}

/* Oplist for a bitset */
#ifndef M_USE_CONTEXT
#define M_BITSET_OPLIST                                                       \
  (INIT(m_bitset_init)                                                        \
   ,INIT_SET(m_bitset_init_set)                                               \
   ,INIT_WITH(API_1(M_INIT_VAI))                                              \
   ,SET(m_bitset_set)                                                         \
   ,CLEAR(m_bitset_clear)                                                     \
   ,INIT_MOVE(m_bitset_init_move)                                             \
   ,MOVE(m_bitset_move)                                                       \
   ,SWAP(m_bitset_swap)                                                       \
   ,TYPE(m_bitset_t)                                                          \
   ,SUBTYPE(bool)                                                             \
   ,EMPTY_P(m_bitset_empty_p),                                                \
   ,GET_SIZE(m_bitset_size)                                                   \
   ,IT_TYPE(m_bitset_it_t)                                                    \
   ,IT_FIRST(m_bitset_it)                                                     \
   ,IT_SET(m_bitset_it_set)                                                   \
   ,IT_LAST(m_bitset_it_last)                                                 \
   ,IT_END(m_bitset_it_end)                                                   \
   ,IT_END_P(m_bitset_end_p)                                                  \
   ,IT_LAST_P(m_bitset_last_p)                                                \
   ,IT_EQUAL_P(m_bitset_it_equal_p)                                           \
   ,IT_NEXT(m_bitset_next)                                                    \
   ,IT_PREVIOUS(m_bitset_previous)                                            \
   ,IT_CREF(m_bitset_cref)                                                    \
   ,RESET(m_bitset_reset)                                                     \
   ,PUSH(m_bitset_push_back)                                                  \
   ,POP(m_bitset_pop_back)                                                    \
   ,HASH(m_bitset_hash)                                                       \
   ,GET_STR(m_bitset_get_str)                                                 \
   ,OUT_STR(m_bitset_out_str)                                                 \
   ,PARSE_STR(m_bitset_parse_str)                                             \
   ,IN_STR(m_bitset_in_str)                                                   \
   ,EQUAL(m_bitset_equal_p)                                                   \
   )
#else
#define M_BITSET_OPLIST                                                       \
  (INIT(m_bitset_init)                                                        \
   ,INIT_SET(API_0P(m_bitset_init_set))                                       \
   ,INIT_WITH(API_1(M_INIT_VAI))                                              \
   ,SET(API_0P(m_bitset_set))                                                 \
   ,CLEAR(API_0P(m_bitset_clear))                                             \
   ,INIT_MOVE(m_bitset_init_move)                                             \
   ,MOVE(API_0P(m_bitset_move))                                               \
   ,SWAP(m_bitset_swap)                                                       \
   ,TYPE(m_bitset_t)                                                          \
   ,SUBTYPE(bool)                                                             \
   ,EMPTY_P(m_bitset_empty_p),                                                \
   ,GET_SIZE(m_bitset_size)                                                   \
   ,IT_TYPE(m_bitset_it_t)                                                    \
   ,IT_FIRST(m_bitset_it)                                                     \
   ,IT_SET(m_bitset_it_set)                                                   \
   ,IT_LAST(m_bitset_it_last)                                                 \
   ,IT_END(m_bitset_it_end)                                                   \
   ,IT_END_P(m_bitset_end_p)                                                  \
   ,IT_LAST_P(m_bitset_last_p)                                                \
   ,IT_EQUAL_P(m_bitset_it_equal_p)                                           \
   ,IT_NEXT(m_bitset_next)                                                    \
   ,IT_PREVIOUS(m_bitset_previous)                                            \
   ,IT_CREF(m_bitset_cref)                                                    \
   ,RESET(m_bitset_reset)                                                     \
   ,PUSH(API_0P(m_bitset_push_back))                                          \
   ,POP(m_bitset_pop_back)                                                    \
   ,HASH(m_bitset_hash)                                                       \
   ,GET_STR(API_0P(m_bitset_get_str))                                         \
   ,OUT_STR(m_bitset_out_str)                                                 \
   ,PARSE_STR(API_0P(m_bitset_parse_str))                                     \
   ,IN_STR(API_0P(m_bitset_in_str))                                           \
   ,EQUAL(m_bitset_equal_p)                                                   \
   )
#endif

/* Register the OPLIST as a global one */
#define M_OPL_m_bitset_t() M_BITSET_OPLIST

// TODO: set_at2, insert_v, remove_v

#if M_USE_SMALL_NAME

#define bitset_s m_bitset_s
#define bitset_t m_bitset_t
#define bitset_ptr m_bitset_ptr
#define bitset_srcptr m_bitset_srcptr
#define bitset_it_s m_bitset_it_s
#define bitset_it_t m_bitset_it_t

#define bitset_init m_bitset_init
#define bitset_reset m_bitset_reset
#define bitset_clear m_bitset_clear
#define bitset_set m_bitset_set
#define bitset_init_set m_bitset_init_set
#define bitset_init_move m_bitset_init_move
#define bitset_move m_bitset_move
#define bitset_set_at m_bitset_set_at
#define bitset_flip_at m_bitset_flip_at
#define bitset_push_back m_bitset_push_back
#define bitset_resize m_bitset_resize
#define bitset_reserve m_bitset_reserve
#define bitset_get m_bitset_get
#define bitset_pop_back m_bitset_pop_back
#define bitset_front m_bitset_front
#define bitset_back m_bitset_back
#define bitset_empty_p m_bitset_empty_p
#define bitset_size m_bitset_size
#define bitset_capacity m_bitset_capacity
#define bitset_swap_at m_bitset_swap_at
#define bitset_swap m_bitset_swap
#define bitset_push_at m_bitset_push_at
#define bitset_pop_at m_bitset_pop_at
#define bitset_equal_p m_bitset_equal_p
#define bitset_it m_bitset_it
#define bitset_it_set m_bitset_it_set
#define bitset_it_last m_bitset_it_last
#define bitset_it_end m_bitset_it_end
#define bitset_end_p m_bitset_end_p
#define bitset_last_p m_bitset_last_p
#define bitset_it_equal_p m_bitset_it_equal_p
#define bitset_next m_bitset_next
#define bitset_previous m_bitset_previous
#define bitset_cref m_bitset_cref
#define bitset_out_str m_bitset_out_str
#define bitset_in_str m_bitset_in_str
#define bitset_parse_str m_bitset_parse_str
#define bitset_set_str m_bitset_set_str
#define bitset_and m_bitset_and
#define bitset_or m_bitset_or
#define bitset_xor m_bitset_xor
#define bitset_not m_bitset_not
#define bitset_hash m_bitset_hash
#define bitset_clz m_bitset_clz
#define bitset_ctz m_bitset_ctz
#define bitset_popcount m_bitset_popcount
#define bitset_get_str m_bitset_get_str

#define BITSET_OPLIST    M_BITSET_OPLIST
#define M_OPL_bitset_t   M_OPL_m_bitset_t

#endif

M_END_PROTECTED_CODE

#endif

// NOTE: Define this function only if m-string has been included
#if !defined(MSTARLIB_BITSET_STRING_H) && defined(MSTARLIB_STRING_H)
#define MSTARLIB_BITSET_STRING_H

M_BEGIN_PROTECTED_CODE

/* Output to a m_string_t 'str' the formatted text representation of the bitset 'set'
   or append it to the string (append=true) */
M_P(void, m_bitset, _get_str, m_string_t str, const m_bitset_t set, bool append)
{
  M_B1TSET_CONTRACT (set);
  M_ASSERT(str != NULL);
  (append ? m_string_cat_cstr : m_string_set_cstr) M_R(str, "[");
  for(size_t i = 0; i < set->size; i++) {
    const bool b = m_bitset_get (set, i);
    const char c = b ? '1' : '0';
    m_string_push_back M_R(str, c);
  }
  m_string_push_back M_R(str, ']');
}

M_END_PROTECTED_CODE

#endif
