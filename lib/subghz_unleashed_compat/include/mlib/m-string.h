/*
 * M*LIB - Dynamic Size String Module
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

#ifndef MSTARLIB_STRING_H
#define MSTARLIB_STRING_H

#include "m-core.h"

M_BEGIN_PROTECTED_CODE

/********************************** INTERNAL ************************************/

// This macro defines the contract of a string.
// Note: A ==> B is represented as not(A) or B
// Note: use of strlen can slow down a lot the program in some cases.
#define M_STR1NG_CONTRACT(v) do {                                             \
    M_ASSERT (v != NULL);                                                     \
    M_ASSERT_SLOW (m_string_size(v) == strlen(m_string_get_cstr(v)));         \
    M_ASSERT (m_string_get_cstr(v)[m_string_size(v)] == 0);                   \
    M_ASSERT (m_string_size(v) < m_string_capacity(v));                       \
  } while(0)

/* Define the maximum size of a string
   By default it is a little bit less than 2^32 bytes, which should be enough for
   most programs. Otherwise it can be increased. */
#ifdef M_USE_STRING_LARGE_INDEX
typedef uint64_t m_str1ng_size_t;
#define m_str1ng_size_clz m_core_clz64
#else
typedef uint32_t m_str1ng_size_t;
#define m_str1ng_size_clz m_core_clz32
#endif

// The string representation if it heap allocated
typedef struct m_str1ng_heap_s {
  char *ptr;
  m_str1ng_size_t size;
  unsigned char   alloc[sizeof (m_str1ng_size_t)];
} m_str1ng_heap_ct;

/* Define the union between a heap allocated string and its inline representation
  If buffer[sizeof(m_str1ng_heap_ct)-2] == 0 then
    It is a embedded based string where buffer is the string, with a maximum allocation of 15 bytes
    In this case buffer[sizeof(m_str1ng_heap_ct)-1] is the size of the string
  Otherwise it is a heap representation then
    ptr is the pointer to the data, size its size, and alloc a representation of the capacity:
        alloc[sizeof (m_str1ng_size_t)-2] << alloc[sizeof (m_str1ng_size_t)-1]
  we shall have alloc[sizeof (m_str1ng_size_t)-2] == buffer[sizeof(m_str1ng_heap_ct)-2]
  so that in case of heap representation, the field buffer[sizeof(m_str1ng_heap_ct)-2] is != 0
*/
typedef union m_str1ng_union_u {
  m_str1ng_heap_ct  heap;
  char              buffer[sizeof (m_str1ng_heap_ct)];
} m_str1ng_union_ct;

/* State of the UTF8 decoding machine state */
typedef enum {
  M_STR1NG_UTF8_STARTING = 0,
  M_STR1NG_UTF8_DECODING_1 = 8,
  M_STR1NG_UTF8_DECODING_2 = 16,
  M_STR1NG_UTF8_DECODING_3 = 24,
  M_STR1NG_UTF8_ERROR = 32
} m_str1ng_utf8_state_e;

/* Maximum number of bytes per UTF8 code point */
#define M_STR1NG_MAX_BYTE_UTF8 4

/* Contract for a string iterator */
#define M_STR1NG_IT_CONTRACT(it) do {                                         \
  M_ASSERT( (it) != NULL);                                                    \
  M_ASSERT( (it)->ptr != NULL);                                               \
  M_ASSERT( (it)->string != NULL);                                            \
  M_ASSERT( m_string_get_cstr((it)->string) <= (it)->ptr);                    \
  M_ASSERT( (it)->ptr <= &m_string_get_cstr((it)->string)[m_string_size((it)->string)]); \
} while (0)


/****************************** EXTERNAL *******************************/

/***********************************************************************/
/*                                                                     */
/*                           DYNAMIC     STRING                        */
/*                                                                     */
/***********************************************************************/

// The default behavior of M*LIB is to use the C library
// and not rewrite the algorithms. However, the fast code
// generates also smaller code, so we'll use it.
#ifndef M_USE_FAST_STRING_CONV
#define M_USE_FAST_STRING_CONV 1
#endif

/* Index returned in case of error instead of the position within the string */
#define M_STRING_FAILURE ((size_t)-1)

/* This is the main structure of this module, representing a dynamic string */
typedef struct m_string_s {
  m_str1ng_union_ct u;
} m_string_t[1];

// Pointer to a Dynamic string
typedef struct m_string_s *m_string_ptr;

// Constant pointer to a Dynamic string
typedef const struct m_string_s *m_string_srcptr;

/* Input option for m_string_fgets 
   M_STRING_READ_LINE  (read line), 
   M_STRING_READ_PURE_LINE (read line and remove final CR and/or LF)
   M_STRING_READ_FILE (read all file)
*/
typedef enum m_string_fgets_e {
  M_STRING_READ_LINE = 0, M_STRING_READ_PURE_LINE = 1, M_STRING_READ_FILE = 2
} m_string_fgets_t;

/* An unicode code point */
typedef uint32_t m_string_unicode_t;

/* Error in case of decoding */
#define M_STRING_UNICODE_ERROR (UINT_MAX)

/* Iterator on a string over UTF8 encoded code points */
typedef struct m_string_it_s {
  m_string_unicode_t u;            // Decoded Unicode code point for the iterator
  const char *ptr;
  m_string_srcptr string;
} m_string_it_t[1];

/* PREFIX:
   m_str1ng_: private methods
   m_string_: public methods
*/

/* Internal method to test if the characters of the string are fully embedded in the struct, 
   or if the characters are allocated on the heap.
   Embedded representation ensures that the penultimate byte is 0.
   By encoding, we ensure that the penultimate byte of 'alloc' (of the heap representation)
   cannot be 0.
 */
M_INLINE bool
m_str1ng_embedded_p(const m_string_t s)
{
  // Function can be called when contract is not fulfilled
  return (s->u.buffer[sizeof(m_str1ng_heap_ct)-2] == 0);
}

/* Internal method to set the size of the string (excluding final nul char) */
M_INLINE void
m_str1ng_set_size(m_string_t s, size_t size)
{
  // Function can be called when contract is not fulfilled
  if (m_str1ng_embedded_p(s)) {
    M_ASSERT (size < sizeof (m_str1ng_heap_ct) - 1);
    // The size of the string is stored as the last char of the buffer.
    s->u.buffer[sizeof(m_str1ng_heap_ct)-1] = (char) size;
  } else {
    M_ASSUME(size == (m_str1ng_size_t) size);
    s->u.heap.size = (m_str1ng_size_t) size;
  }
}

/* Return the number of bytes of the string (excluding the final nul char) */
M_INLINE size_t
m_string_size(const m_string_t s)
{
  // Function can be called when contract is not fulfilled
  const size_t s_embed = (size_t) s->u.buffer[sizeof(m_str1ng_heap_ct)-1];
  const size_t s_heap  = s->u.heap.size;
  return m_str1ng_embedded_p(s) ?  s_embed : s_heap;
}

/* Return the capacity of the string (including the final nul char) */
M_INLINE size_t
m_string_capacity(const m_string_t s)
{
  // Function can be called when contract is not fulfilled
  const size_t c_embed = sizeof (m_str1ng_heap_ct) - 1;
  const size_t c_heap  = (m_str1ng_size_t) s->u.heap.alloc[sizeof (m_str1ng_size_t)-2] << s->u.heap.alloc[sizeof (m_str1ng_size_t)-1];
  return m_str1ng_embedded_p(s) ?  c_embed : c_heap;
}

/* Internal function to round up the allocation size to the next allowed representation
   Return the rounded allocation and the pair (m,e) used to encode it
   An overflow occurs if the result is strictly lower than 'alloc' before casting. 
    */
M_INLINE m_str1ng_size_t
m_str1ng_round_capacity(unsigned char *m, unsigned char *e, m_str1ng_size_t alloc)
{
  // We will encode 'alloc' as a number m<<e with (m,e) being unsigned char.
  // alloc is strictly greater than 0, but with casting to m_str1ng_size_t, it may be 0.
  // Compute the number of bits needed to represent 'alloc'
  unsigned b = (unsigned) sizeof(m_str1ng_size_t) * CHAR_BIT - m_str1ng_size_clz(alloc);
  if (b <= CHAR_BIT) {
    // It can be fully representable. No rounding needed.
    *m = (unsigned char) alloc;
    *e = 0;
  } else {
    /* It cannot be fully represented: Round it to the next upper representation of m << e
       with m and e unsigned char.
       Keep only the '7' upper bits of alloc, and round it to the next representation so that
       m << e is bigger or equal to alloc.
       Worst case is when alloc = 0x7F01 (i.e. all 7 upper bits are 1)
       Performing the rounding will generate a carry, so we get for mantissa 0x80 which is on 8 bits.
       That's why we shift by '7' and not by '8', so that 0x80 remains representable as an
       unsigned char. Otherwise we would need to take into account this case, and shift mantissa by
       one and increase exponent by one.
       If alloc was >= 0xFE000001, rounding will overflow the max of a m_str1ng_size_t
       and *m == 0.
       This case should be handled by the caller as an overflow error
       (return value is lower than input)
    */
    *m = (unsigned char) ((alloc + ((1U <<(b-(CHAR_BIT-1)))-1))>>(b-(CHAR_BIT-1)));
    *e = (unsigned char) (b - (CHAR_BIT-1));
  }
  // Worst case is when the rounding overflow the size m_str1ng_size_t
  M_ASSERT (((m_str1ng_size_t) *m << *e) >= alloc || *m == 0);
  return (m_str1ng_size_t) *m << *e;
}

/* Internal function to save the allocation size to the next allowed representation
  as returned by m_str1ng_round_capacity */
M_INLINE void
m_str1ng_set_capacity(m_string_t s, unsigned char m, unsigned char e)
{
  // Function can be called when contract is not fulfilled
  M_ASSERT(m != 0);
  s->u.heap.alloc[sizeof (m_str1ng_size_t)-2] = m;
  s->u.heap.alloc[sizeof (m_str1ng_size_t)-1] = e;
}

/* Return a writable pointer to the array of char of the string */
M_INLINE char*
m_str1ng_get_cstr(m_string_t v)
{
  // Function can be called when contract is not fulfilled
  char *const ptr_embed = &v->u.buffer[0];
  char *const ptr_heap  = v->u.heap.ptr;
  return m_str1ng_embedded_p(v) ?  ptr_embed : ptr_heap;
}

/* Return the string view a classic C string (const char *) */
M_INLINE const char*
m_string_get_cstr(const m_string_t v)
{
  // Function cannot be called when contract is not fulfilled
  // but it is called by contract (so no contract check to avoid infinite recursion).
  const char *const ptr_embed = &v->u.buffer[0];
  const char *const ptr_heap  = v->u.heap.ptr;
  return m_str1ng_embedded_p(v) ?  ptr_embed : ptr_heap;
}

/* Initialize the dynamic string (constructor) 
  and make it empty */
M_INLINE void
m_string_init(m_string_t s)
{
  memset(s, 0, sizeof(m_string_t));
  M_STR1NG_CONTRACT(s);
}

/* Clear the Dynamic string (destructor) */
M_P(void, m_string, _clear, m_string_t v)
{
  M_STR1NG_CONTRACT(v);
  if (!m_str1ng_embedded_p(v)) {    
    M_MEMORY_FREE(m_context, char, v->u.heap.ptr, m_string_capacity(v));
    v->u.heap.ptr   = NULL;
  }
  /* This is not needed but is safer to make
     the string invalid so that it can be detected. */
  v->u.buffer[sizeof(m_str1ng_heap_ct)-2] = 0;
  v->u.buffer[sizeof(m_str1ng_heap_ct)-1] = CHAR_MAX;
}

M_INLINE m_string_ptr m_str1ng_init_ref(m_string_t v) { m_string_init(v); return v; }

/* Clear the Dynamic string (destructor)
  and return a heap pointer to the string.
  The ownership of the data is transferred back to the caller
  and the returned pointer has to be released by M_MEMORY_FREE. */
M_P(char *, m_string, _clear_get_cstr, m_string_t v)
{
  M_STR1NG_CONTRACT(v);
  char *p = v->u.heap.ptr;
  if (m_str1ng_embedded_p(v)) {
    // The characters are embedded in the string.
    p = &v->u.buffer[0];
    // Need to allocate a heap string to return the copy.
    size_t alloc = m_string_size(v)+1;
    char *ptr = M_MEMORY_REALLOC (m_context, char, NULL, 0, alloc);
    if (M_UNLIKELY_NOMEM (ptr == NULL)) {
      M_MEMORY_FULL(char, alloc);
    }
    M_ASSERT(ptr != NULL && p != NULL);
    memcpy(ptr, p, alloc);
    p = ptr;
  }
  v->u.buffer[sizeof(m_str1ng_heap_ct)-2] = 0;
  v->u.buffer[sizeof(m_str1ng_heap_ct)-1] = CHAR_MAX;
  return p;
}

/* Make the string empty */
M_INLINE void
m_string_reset(m_string_t v)
{
  M_STR1NG_CONTRACT (v);
  m_str1ng_set_size(v, 0);
  m_str1ng_get_cstr(v)[0] = 0;
  M_STR1NG_CONTRACT (v);
}

/* Return the selected byte-character of the string */
M_INLINE char
m_string_get_char(const m_string_t v, size_t index)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT_INDEX(index, m_string_size(v));
  return m_string_get_cstr(v)[index];
}

/* Set the selected byte-character of the string */
M_INLINE void
m_string_set_char(m_string_t v, size_t index, const char c)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT_INDEX(index, m_string_size(v));
  m_str1ng_get_cstr(v)[index] = c;
}

/* Test if the string is empty or not */
M_INLINE bool
m_string_empty_p(const m_string_t v)
{
  M_STR1NG_CONTRACT (v);
  return m_string_size(v) == 0;
}

/* Internal method to fit the string to the given size
   Ensures that the string capacity is greater than size_alloc
   (size_alloc shall include the final null char).
   It may transform the string from embedded based to heap based.
   The string 'v' no longer has a working size field.
   Return a pointer to the writable string.
*/
M_P(char *, m_str1ng, _fit2size, m_string_t v, size_t size_alloc)
{
  M_ASSERT_INDEX (0, size_alloc);
  // Note: this function may be called in context where the contract
  // is not fulfilled.
  const size_t old_alloc = m_string_capacity(v);
  // This line enables the compiler to completely remove this function
  // for very short constant strings.
  M_ASSUME(old_alloc >= sizeof (m_str1ng_heap_ct) - 1);
  if (M_UNLIKELY (size_alloc > old_alloc)) {
    // Insufficient current allocation to store the new string
    // Perform an allocation on the heap.
    size_t alloc = (size_alloc + size_alloc / 2);
    unsigned char m, e;
    alloc = m_str1ng_round_capacity(&m, &e, (m_str1ng_size_t) alloc);
    if (M_UNLIKELY_NOMEM (alloc <= size_alloc)) {
      /* Overflow in alloc computation */
      M_MEMORY_FULL(char, size_alloc);
    }
    char *ptr = m_str1ng_embedded_p(v) ? NULL : v->u.heap.ptr;
    //FIXME: old_alloc may not be NULL if ptr is NULL.
    ptr = M_MEMORY_REALLOC (m_context, char, ptr, old_alloc, alloc);
    if (M_UNLIKELY_NOMEM (ptr == NULL)) {
      M_MEMORY_FULL(char, alloc);
    }
    // The pointer cannot be the embedded buffer of the string as it is heap allocated
    M_ASSERT(ptr != &v->u.buffer[0]);
    if (m_str1ng_embedded_p(v)) {
      // The string was fully embedded.
      /* Copy the embedded characters into the new heap allocation */
      const size_t size = (size_t) v->u.buffer[sizeof(m_str1ng_heap_ct) - 1];
      memcpy(ptr, &v->u.buffer[0], size+1);
    }
    // The string cannot be fully embedded anymore.
    v->u.heap.ptr = ptr;
    m_str1ng_set_capacity(v, m, e);
    // Size is not set as the function is called in context 
    // where the caller already know the final size of the string,
    // so it will update it.
    return ptr;
  }
  return m_str1ng_get_cstr(v);
}

/* Modify the string capacity to be able to handle at least 'alloc'
   characters (including final nul char).
   It may reduce the allocation of the string if possible */
M_P(void, m_string, _reserve, m_string_t v, size_t alloc)
{
  M_STR1NG_CONTRACT (v);
  const size_t size = m_string_size(v);
  /* NOTE: Reserve below needed size, perform a shrink to fit */
  if (size + 1 > alloc) {
    alloc = size+1;
  }
  M_ASSERT (alloc > 0);
  if (alloc < sizeof (m_str1ng_heap_ct)) {
    // All characters can fit in am embedded structure
    if (!m_str1ng_embedded_p(v)) {
      /* Move the characters from the heap to within the structure */
      char *ptr = &v->u.buffer[0];
      char *oldptr = v->u.heap.ptr;
      memcpy(ptr, oldptr, size+1);
      // Free the allocated memory on the heap
      M_MEMORY_FREE(m_context, char, oldptr, m_string_capacity(v));
      v->u.buffer[sizeof(m_string_t)-2] = 0;
      v->u.buffer[sizeof(m_string_t)-1] = (char) size;
    } else {
      /* Already a fully embedded based type: nothing to do */
    }
  } else {
    // Allocation cannot fit in the embedded structure space
    // Need to allocate in heap space
    unsigned char m, e;
    size_t r_alloc = m_str1ng_round_capacity(&m, &e, (m_str1ng_size_t) alloc);
    if (M_UNLIKELY_NOMEM (alloc > r_alloc)) {
      M_MEMORY_FULL(char, alloc);
    } 
    char *ptr = m_str1ng_embedded_p(v) ? NULL : v->u.heap.ptr;
    ptr = M_MEMORY_REALLOC (m_context, char, ptr, m_string_capacity(v), r_alloc);
    if (M_UNLIKELY_NOMEM (ptr == NULL) ) {
      M_MEMORY_FULL(char, alloc);
    }
    if (m_str1ng_embedded_p(v)) {
      // Copy the characters embedded in the structure to the allocated heap space
      memcpy(ptr, &v->u.buffer[0], size+1);
      M_ASSERT(size == (m_str1ng_size_t) size);
      v->u.heap.size = (m_str1ng_size_t) size;
    }
    v->u.heap.ptr = ptr;
    m_str1ng_set_capacity(v, m, e);
  }
  M_STR1NG_CONTRACT (v);
}

/* Set the string to the C string str */
M_P(void, m_string, _set_cstr, m_string_t v, const char str[])
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT(str != NULL);
  size_t size = strlen(str);
  char *ptr = m_str1ng_fit2size M_R(v, size+1);
  // The memcpy will also copy the final null char of the string
  memcpy(ptr, str, size+1);
  m_str1ng_set_size(v, size);
  M_STR1NG_CONTRACT (v);
}

/* Set the string to the n first characters of the C string str */
M_P(void, m_string, _set_cstrn, m_string_t v, const char str[], size_t n)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT(str != NULL);
  size_t size;
  char *ptr = m_str1ng_fit2size M_R(v, n+1);
  for(size = 0; size < n && str[size] != 0; size++) {
    ptr[size] = str[size];
  }
  // Cannot copy the final null char using memcpy
  ptr[size] = 0;
  m_str1ng_set_size(v, size);
  M_STR1NG_CONTRACT (v);
}

/* Set the string to the other one */
M_P(void, m_string, _set, m_string_t v1, const m_string_t v2)
{
  M_STR1NG_CONTRACT (v1);
  M_STR1NG_CONTRACT (v2);
  if (M_LIKELY (v1 != v2)) {
    const size_t size = m_string_size(v2);
    char *ptr = m_str1ng_fit2size M_R(v1, size+1);
    memcpy(ptr, m_string_get_cstr(v2), size+1);
    m_str1ng_set_size(v1, size);
  }
  M_STR1NG_CONTRACT (v1);
}

/* Set the string to the n first characters of other one */
M_P(void, m_string, _set_n, m_string_t v, const m_string_t ref, size_t offset, size_t length)
{
  M_STR1NG_CONTRACT (v);
  M_STR1NG_CONTRACT (ref);
  M_ASSERT_INDEX (offset, m_string_size(ref) + 1);
  size_t size = M_MIN (m_string_size(ref) - offset, length);
  char *ptr = m_str1ng_fit2size M_R(v, size+1);
  // v may be equal to ref, so a memmove is needed instead of a memcpy
  memmove(ptr, m_string_get_cstr(ref) + offset, size);
  // Cannot copy the final null char using memcpy
  ptr[size] = 0;
  m_str1ng_set_size(v, size);
  M_STR1NG_CONTRACT (v);
}

/* Initialize the string and set it to the other one 
   (constructor) */
M_P(void, m_string, _init_set, m_string_t v1, const m_string_t v2)
{
  // In case of exception, v1 remains initialized,
  // but without any allocation done.
  // Therefore it is safe, not to clear v1 on exception.
  m_string_init(v1);
  m_string_set M_R(v1,v2);
}

/* Initialize the string and set it to the C string
   (constructor) */
M_P(void, m_string, _init_set_cstr, m_string_t v1, const char str[])
{
  // In case of exception, v1 remains initialized,
  // but without any allocation done.
  // Therefore it is safe, not to clear v1 on exception.
  m_string_init(v1);
  m_string_set_cstr M_R(v1, str);
}

/* Initialize the string, set it to the other one,
   and destroy the other one.
   (constructor & destructor) */
M_INLINE void
m_string_init_move(m_string_t v1, m_string_t v2)
{
  M_STR1NG_CONTRACT (v2);
  memcpy(v1, v2, sizeof (m_string_t));
  // Note: nullify v2 to be safer
  v2->u.buffer[sizeof(m_str1ng_heap_ct)-2] = 0;
  v2->u.buffer[sizeof(m_str1ng_heap_ct)-1] = CHAR_MAX;
  M_STR1NG_CONTRACT (v1);
}

/* Swap the two strings v1 and v2 */
M_INLINE void
m_string_swap(m_string_t v1, m_string_t v2)
{
  M_STR1NG_CONTRACT (v1);
  M_STR1NG_CONTRACT (v2);
  m_string_t tmp;
  memcpy(tmp, v1, sizeof(m_string_t));
  memcpy(v1,  v2, sizeof(m_string_t));
  memcpy(v2, tmp, sizeof(m_string_t));
  M_STR1NG_CONTRACT (v1);
  M_STR1NG_CONTRACT (v2);
}

/* Set the string to the other one,
   and destroy the other one.
   (destructor) */
M_P(void, m_string, _move, m_string_t v1, m_string_t v2)
{
  m_string_clear M_R(v1);
  m_string_init_move(v1,v2);
}

/* Push the byte-character 'c' in the string 'v' */
M_P(void, m_string, _push_back, m_string_t v, char c)
{
  M_STR1NG_CONTRACT (v);
  const size_t size = m_string_size(v);
  char *ptr = m_str1ng_fit2size M_R(v, size+2);
  ptr[size+0] = c;
  ptr[size+1] = 0;
  m_str1ng_set_size(v, size+1);
  M_STR1NG_CONTRACT (v);
}

/* Concatenate the string with the C string */
M_P(void, m_string, _cat_cstr, m_string_t v, const char str[])
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT (str != NULL);
  const size_t old_size = m_string_size(v);
  const size_t size = strlen(str);
  char *ptr = m_str1ng_fit2size M_R(v, old_size + size + 1);
  memcpy(&ptr[old_size], str, size + 1);
  m_str1ng_set_size(v, old_size + size);
  M_STR1NG_CONTRACT (v);
}

/* Concatenate the string with the other string */
M_P(void, m_string, _cat, m_string_t v, const m_string_t v2)
{
  M_STR1NG_CONTRACT (v2);
  M_STR1NG_CONTRACT (v);
  const size_t size = m_string_size(v2);
  if (M_LIKELY (size > 0)) {
    const size_t old_size = m_string_size(v);
    char *ptr = m_str1ng_fit2size M_R(v, old_size + size + 1);
    memcpy(&ptr[old_size], m_string_get_cstr(v2), size);
    ptr[old_size + size] = 0;
    m_str1ng_set_size(v, old_size + size);
  }
  M_STR1NG_CONTRACT (v);
}

/* Compare the string to the C string and
  return the sort order (negative if less, 0 if equal, positive if greater) */
M_INLINE int
m_string_cmp_cstr(const m_string_t v1, const char str[])
{
  M_STR1NG_CONTRACT (v1);
  M_ASSERT (str != NULL);
  return strcmp(m_string_get_cstr(v1), str);
}

/* Compare the string to the other string and
  return the sort order (negative if less, 0 if equal, positive if greater) */
M_INLINE int
m_string_cmp(const m_string_t v1, const m_string_t v2)
{
  M_STR1NG_CONTRACT (v1);
  M_STR1NG_CONTRACT (v2);
  return strcmp(m_string_get_cstr(v1), m_string_get_cstr(v2));
}

/* Test if the string is equal to the given C string */
M_INLINE bool
m_string_equal_cstr_p(const m_string_t v1, const char str[])
{
  M_STR1NG_CONTRACT(v1);
  M_ASSERT (str != NULL);
  return m_string_cmp_cstr(v1, str) == 0;
}

/* Test if the string is equal to the other string */
M_INLINE bool
m_string_equal_p(const m_string_t v1, const m_string_t v2)
{
  /* m_string_equal_p can be called with (at most) one string which is an OOR value.
     In case of OOR value, .ptr is NULL and .alloc is the maximum or the maximum-1.
     As it will detect a full embedded based string, it will read the size from the alloc fied
     as 0xFF or 0xFE. In both cases, the size cannot be equal to a normal string
     so the test m_string_size(v1) == m_string_size(v2) is false in this case.
  */
  M_ASSERT(v1 != NULL);
  M_ASSERT(v2 != NULL);
  /* Optimization: both strings shall have at least the same size */
  return m_string_size(v1) == m_string_size(v2)
      && memcmp(m_string_get_cstr(v1), m_string_get_cstr(v2), m_string_size(v1)) == 0;
}

/* Test if the string is equal to the C string 
   (case insensitive according to the current locale)
   Note: doesn't work with UTF-8 strings.
*/
M_INLINE int
m_string_cmpi_cstr(const m_string_t v1, const char p2[])
{
  M_STR1NG_CONTRACT (v1);
  M_ASSERT (p2 != NULL);
  // strcasecmp is POSIX only
  const char *p1 = m_string_get_cstr(v1);
  int c1, c2;
  do {
    // To avoid locale without 1 per 1 mapping.
    c1 = toupper((unsigned char) *p1++);
    c2 = toupper((unsigned char) *p2++);
    c1 = tolower((unsigned char) c1);
    c2 = tolower((unsigned char) c2);
  } while (c1 == c2 && c1 != 0);
  return c1 - c2;
}

/* Test if the string is equal to the other string 
   (case insensitive according to the current locale)
   Note: doesn't work with UTF-8 strings.
*/
M_INLINE int
m_string_cmpi(const m_string_t v1, const m_string_t v2)
{
  return m_string_cmpi_cstr(v1, m_string_get_cstr(v2));
}

/* Search for the position of the character c
   from the position 'start' (include)  in the string 
   Return M_STRING_FAILURE if not found.
   By default, start is zero.
*/
M_INLINE size_t
m_string_search_char (const m_string_t v, char c, size_t start)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT_INDEX (start, m_string_size(v) + 1);
  const char *p = M_ASSIGN_CAST(const char*,
                                strchr(m_string_get_cstr(v)+start, c));
  return p == NULL ? M_STRING_FAILURE : (size_t) (p-m_string_get_cstr(v));
}

/* Reverse Search for the position of the character c
   from the position 'start' (include)  in the string 
   Return M_STRING_FAILURE if not found.
   By default, start is zero.
*/
M_INLINE size_t
m_string_search_rchar (const m_string_t v, char c, size_t start)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT_INDEX (start, m_string_size(v)+1);
  // NOTE: Can be implemented in a faster way than the libc function
  // by scanning backward from the bottom of the string (which is
  // possible since we know the size).
  // However, does it worth the effort?
  const char *p = M_ASSIGN_CAST(const char*,
                                strrchr(m_string_get_cstr(v)+start, c));
  return p == NULL ? M_STRING_FAILURE : (size_t) (p-m_string_get_cstr(v));
}

/* Search for the sub C string in the string from the position start
   Return M_STRING_FAILURE if not found.
   By default, start is zero. */
M_INLINE size_t
m_string_search_cstr(const m_string_t v, const char str[], size_t start)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT_INDEX (start, m_string_size(v)+1);
  M_ASSERT (str != NULL);
  const char *p = M_ASSIGN_CAST(const char*,
                                strstr(m_string_get_cstr(v)+start, str));
  return p == NULL ? M_STRING_FAILURE : (size_t) (p-m_string_get_cstr(v));
}

/* Search for the sub other string v2 in the string v1 from the position start
   Return M_STRING_FAILURE if not found.
   By default, start is zero. */
M_INLINE size_t
m_string_search (const m_string_t v1, const m_string_t v2, size_t start)
{
  M_STR1NG_CONTRACT (v2);
  M_ASSERT_INDEX (start, m_string_size(v1)+1);
  return m_string_search_cstr(v1, m_string_get_cstr(v2), start);
}

/* Search for the first matching character in the given C string 
   in the string v1 from the position start
   Return M_STRING_FAILURE if not found.
   By default, start is zero. */
M_INLINE size_t
m_string_search_pbrk(const m_string_t v1, const char first_of[], size_t start)
{
  M_STR1NG_CONTRACT (v1);
  M_ASSERT_INDEX (start, m_string_size(v1)+1);
  M_ASSERT (first_of != NULL);
  const char *p = M_ASSIGN_CAST(const char*,
                                strpbrk(m_string_get_cstr(v1)+start, first_of));
  return p == NULL ? M_STRING_FAILURE : (size_t) (p-m_string_get_cstr(v1));
}

/* Compare the string to the C string using strcoll */
M_INLINE int
m_string_strcoll_cstr(const m_string_t v, const char str[])
{
  M_STR1NG_CONTRACT (v);
  return strcoll(m_string_get_cstr(v), str);
}

/* Compare the string to the other string using strcoll */
M_INLINE int
m_string_strcoll (const m_string_t v1, const m_string_t v2)
{
  M_STR1NG_CONTRACT (v2);
  return m_string_strcoll_cstr(v1, m_string_get_cstr(v2));
}

/* Return the number of bytes of the segment of s
   that consists entirely of bytes in accept */
M_INLINE size_t
m_string_spn(const m_string_t v1, const char accept[])
{
  M_STR1NG_CONTRACT (v1);
  M_ASSERT (accept != NULL);
  return strspn(m_string_get_cstr(v1), accept);
}

/* Return the number of bytes of the segment of s
   that consists entirely of bytes not in reject */
M_INLINE size_t
m_string_cspn(const m_string_t v1, const char reject[])
{
  M_STR1NG_CONTRACT (v1);
  M_ASSERT (reject != NULL);
  return strcspn(m_string_get_cstr(v1), reject);
}

/* Return the string left truncated to the first 'index' bytes */
M_INLINE void
m_string_left(m_string_t v, size_t index)
{
  M_STR1NG_CONTRACT (v);
  const size_t size = m_string_size(v);
  if (index >= size)
    return;
  m_str1ng_get_cstr(v)[index] = 0;
  m_str1ng_set_size(v,index);
  M_STR1NG_CONTRACT (v);
}

/* Return the string right truncated from the 'index' position to the last position */
M_INLINE void
m_string_right(m_string_t v, size_t index)
{
  M_STR1NG_CONTRACT (v);
  char *ptr = m_str1ng_get_cstr(v);
  const size_t size = m_string_size(v);
  if (index >= size) {
    ptr[0] = 0;
    m_str1ng_set_size(v, 0);
    M_STR1NG_CONTRACT (v);
    return;
  }
  size_t s2 = size - index;
  memmove (&ptr[0], &ptr[index], s2+1);
  m_str1ng_set_size(v, s2);
  M_STR1NG_CONTRACT (v);
}

/* Return the string from position index to size bytes.
   See also m_string_set_n
 */
M_INLINE void
m_string_mid (m_string_t v, size_t index, size_t size)
{
  m_string_right(v, index);
  m_string_left(v, size);
}

/* Replace in the string the first occurrence of the C string str1
   into the C string str2 from start
   By default, start is zero.
*/
M_P(size_t, m_string, _replace_cstr, m_string_t v, const char str1[], const char str2[], size_t start)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT (str1 != NULL && str2 != NULL);
  size_t i = m_string_search_cstr(v, str1, start);
  if (i != M_STRING_FAILURE) {
    const size_t str1_l = strlen(str1);
    const size_t str2_l = strlen(str2);
    const size_t size   = m_string_size(v);
    M_ASSERT(size + 1 + str2_l > str1_l);
    char *ptr = m_str1ng_fit2size M_R(v, size + str2_l - str1_l + 1);
    if (str1_l != str2_l) {
      memmove(&ptr[i+str2_l], &ptr[i+str1_l], size - i - str1_l + 1);
      m_str1ng_set_size(v, size + str2_l - str1_l);
    }
    memcpy (&ptr[i], str2, str2_l);
    M_STR1NG_CONTRACT (v);
  }
  return i;
}

/* Replace in the string the first occurrence of the C string v1
   into the C string v2 from start
   By default, start is zero.
*/
M_P(size_t, m_string, _replace, m_string_t v, const m_string_t v1, const m_string_t v2, size_t start)
{
  M_STR1NG_CONTRACT (v);
  M_STR1NG_CONTRACT (v1);
  M_STR1NG_CONTRACT (v2);
  return m_string_replace_cstr M_R(v, m_string_get_cstr(v1), m_string_get_cstr(v2), start);
}

/* Replace in the string the sub-string at position 'pos' for 'len' bytes
   into the C string str2. */
M_P(void, m_string, _replace_at, m_string_t v, size_t pos, size_t len, const char str2[])
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT(str2 != NULL);
  const size_t str1_l = len;
  const size_t str2_l = strlen(str2);
  const size_t size   = m_string_size(v);
  char *ptr;
  if (str1_l != str2_l) {
    // Move bytes from the string 
    M_ASSERT_INDEX (str1_l, size + str2_l + 1);
    ptr = m_str1ng_fit2size M_R(v, size + str2_l - str1_l + 1);
    M_ASSERT_INDEX (pos + str1_l, size + 1);
    M_ASSUME (pos + str1_l < size + 1);
    memmove(&ptr[pos+str2_l], &ptr[pos+str1_l], size - pos - str1_l + 1);
    m_str1ng_set_size(v, size + str2_l - str1_l);
  } else {
    ptr = m_str1ng_get_cstr(v);
  }
  memcpy (&ptr[pos], str2, str2_l);
  M_STR1NG_CONTRACT (v);
}

/* Replace all occurrences of str1 into str2 when strlen(str1) >= strlen(str2) */
M_INLINE void
m_str1ng_replace_all_cstr_1ge2 (m_string_t v, const char str1[], size_t str1len, const char str2[], size_t str2len)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT(str1len >= str2len);

  /* str1len >= str2len so the string doesn't need to be resized */
  size_t vlen = m_string_size(v);
  char *org = m_str1ng_get_cstr(v);
  char *src = org;
  char *dst = org;

  // Go through all the characters of the string
  while (*src != 0) {
    // Get a new occurrence of str1 in the v string.
    char *occ = strstr(src, str1);
    if (occ == NULL) {
      // No new occurrence
      break;
    }
    M_ASSERT(occ >= src);
    // Copy the data until the new occurrence
    if (src != dst) {
      memmove(dst, src, (size_t) (occ - src));
    }
    dst += (occ - src);
    // Copy the replaced string
    memcpy(dst, str2, str2len);
    dst += str2len;
    // Advance src pointer
    src  = occ + str1len;
  }
  // Finish copying the string until the end
  M_ASSERT (src <= org + vlen );
  if (src != dst) {
    memmove(dst, src, (size_t) (org + vlen + 1 - src) );
  }
  // Update string size
  m_str1ng_set_size(v, (size_t) (dst + vlen - src) );
  M_STR1NG_CONTRACT (v);
}

/* Reverse strstr from the end of the string
  org is the start of the string
  src is the current character pointer (shall be initialized to the end of the string)
  pattern / pattern_size: the pattern to search.
  */
M_INLINE char *
m_str1ng_strstr_r(char org[], char src[], const char pattern[], size_t pattern_size)
{
  M_ASSERT(pattern_size >= 1);
  src -= pattern_size - 1;
  while (org <= src) {
    if (src[0] == pattern[0]
      && src[pattern_size-1] == pattern[pattern_size-1]
      && memcmp(src, pattern, pattern_size) == 0) {
        return src;
    }
    src --;
  }
  return NULL;
}

/* Replace all occurences of str1 into str2 when strlen(str1) < strlen(str2) */
M_P(void, m_str1ng, _replace_all_cstr_1lo2, m_string_t v, const char str1[], size_t str1len, const char str2[], size_t str2len)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT(str1len < str2len);

  /* str1len < str2len so the string may need to be resized
    Worst case if when v is composed fully of str1 substrings.
    Needed size : v.size / str1len * str2len
   */
  size_t vlen = m_string_size(v);
  size_t alloc = 1 + vlen / str1len * str2len;
  char *org = m_str1ng_fit2size M_R(v, alloc);
  char *src = org + vlen - 1;
  char *end = org + m_string_capacity(v);
  char *dst = end;

  // Go through all the characters of the string in reverse !
  while (src >= org) {
    char *occ = m_str1ng_strstr_r(org, src, str1, str1len);
    if (occ == NULL) {
      break;
    }
    M_ASSERT(occ + str1len - 1 <= src);
    // Copy the data until the new occurrence
    dst -= (src - (occ + str1len - 1));
    memmove(dst, occ+str1len, (size_t) (src - (occ + str1len - 1)));
    // Copy the replaced string
    dst -= str2len;
    memcpy(dst, str2, str2len);
    // Advance src pointer
    src  = occ - 1;
  }
  // Finish moving data back to their natural place
  memmove(src + 1, dst, (size_t) (end - dst) );
  // Update string size
  vlen = (size_t) (src - org + end - dst + 1);
  org[vlen] = 0;
  m_str1ng_set_size(v, vlen );
  M_STR1NG_CONTRACT (v);
}

M_P(void, m_string, _replace_all_cstr, m_string_t v, const char str1[], const char str2[])
{
  size_t str1_l = strlen(str1);
  size_t str2_l = strlen(str2);
  M_ASSERT(str1_l > 0);
  if (str1_l >= str2_l) {
    m_str1ng_replace_all_cstr_1ge2(v, str1, str1_l, str2, str2_l );
  } else {
    m_str1ng_replace_all_cstr_1lo2 M_R(v, str1, str1_l, str2, str2_l);
  }
}

M_P(void, m_string, _replace_all, m_string_t v, const m_string_t str1, const m_string_t str2)
{
  size_t str1_l = m_string_size(str1);
  size_t str2_l = m_string_size(str2);
  M_ASSERT(str1_l > 0);
  M_ASSERT(str2_l > 0);
  if (str1_l >= str2_l) {
    m_str1ng_replace_all_cstr_1ge2(v, m_string_get_cstr(str1), str1_l, m_string_get_cstr(str2), str2_l );
  } else {
    m_str1ng_replace_all_cstr_1lo2 M_R(v, m_string_get_cstr(str1), str1_l, m_string_get_cstr(str2), str2_l);
  }
}

// Define the fast integer to string conversions if requested
// or if no support for stdarg.
#if M_USE_FAST_STRING_CONV == 1 || M_USE_STDARG == 0

  // Compute the maximum number of characters needed for the buffer.
#if UINT_MAX == 4294967295U
#define M_STR1NG_INT_MAX_SIZE (10+1)
#elif UINT_MAX <= 18446744073709551615UL
#define M_STR1NG_INT_MAX_SIZE (20+1)
#else
# error Unexpected UINT_MAX value (workaround: Define M_USE_FAST_STRING_CONV to 0).
#endif

M_P(void, m_string, _set_ui, m_string_t v, unsigned int n)
{
  M_STR1NG_CONTRACT (v);
  char buffer[M_STR1NG_INT_MAX_SIZE];
  char *d = m_str1ng_fit2size M_R(v, M_STR1NG_INT_MAX_SIZE+1);
  unsigned i = 0, j = 0;
  do {
    // 0123456789 are mandatory in this order as characters, as per C standard.
    buffer[i++] = (char) ('0' + (n % 10U));
    n /= 10U;
  } while (n != 0);
  M_ASSERT_INDEX(i, M_STR1NG_INT_MAX_SIZE);
  while (i > 0) {
    d[j++] = buffer[--i];
  }
  d[j] = 0;
  m_str1ng_set_size(v, j);
  M_STR1NG_CONTRACT (v);
}

M_P(void, m_string, _set_si, m_string_t v, int n)
{
  M_STR1NG_CONTRACT (v);
  // Compute the maximum number of characters needed for the buffer.
  char buffer[M_STR1NG_INT_MAX_SIZE];
  char *d = m_str1ng_fit2size M_R(v, M_STR1NG_INT_MAX_SIZE+1);
  unsigned i = 0, j = 0;
  bool neg = n < 0;
  unsigned n0 = neg ? 0U -(unsigned) n : (unsigned) n;
  do {
    // 0123456789 are mandatory in this order as characters, as per C standard.
    buffer[i++] = (char) ('0' + (n0 % 10U));
    n0 /= 10U;
  } while (n0 != 0);
  M_ASSERT_INDEX(i, M_STR1NG_INT_MAX_SIZE);
  if (neg) d[j++] = '-';
  while (i > 0) {
    d[j++] = buffer[--i];
  }
  d[j] = 0;
  m_str1ng_set_size(v, j);
  M_STR1NG_CONTRACT (v);
}
#endif

#if M_USE_STDARG

/* Format in the string the given printf format */
M_P(int, m_string, _vprintf, m_string_t v, const char format[], va_list args)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT (format != NULL);
  int size;
  va_list args_org;
  va_copy(args_org, args);
  char *ptr = m_str1ng_get_cstr(v);
  size_t alloc = m_string_capacity(v);
  size = vsnprintf (ptr, alloc, format, args);
  if (size > 0 && ((size_t) size+1 >= alloc) ) {
    // Reset the string in a clean state in case of exception
    // to keep the object in a clearable state
    M_IF_EXCEPTION( m_str1ng_set_size(v, 0) );
    M_IF_EXCEPTION( m_str1ng_get_cstr(v)[0] = 0 );
    // We have to realloc our string to fit the needed size
    ptr = m_str1ng_fit2size M_R(v, (size_t) size + 1);
    alloc = m_string_capacity(v);
    // and redo the parsing.
    size = vsnprintf (ptr, alloc, format, args_org);
    M_ASSERT (size > 0 && (size_t)size < alloc);
  }
  if (M_LIKELY (size >= 0)) {
    m_str1ng_set_size(v, (size_t) size);
  } else {
    // An error occurred during the conversion: Assign an empty string.
    m_str1ng_set_size(v, 0);
    ptr[0] = 0;
  }
  va_end(args_org);
  M_STR1NG_CONTRACT (v);
  return size;
}

/* Format in the string the given printf format */
// FIXME: BROKEN to add a parameter **after** the '...'!
M_P(int, m_string, _printf, m_string_t v, const char format[], ...)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT (format != NULL);
  va_list args;
  va_start (args, format);
  int ret = m_string_vprintf M_R(v, format, args);
  va_end (args);
  return ret;
}

/* Append to the string the formatted string of the given printf format */
M_P(int, m_string, _cat_vprintf, m_string_t v, const char format[], va_list args)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT (format != NULL);
  va_list args_org;
  va_copy(args_org, args);
  int size;
  size_t old_size = m_string_size(v);
  char  *ptr      = m_str1ng_get_cstr(v);
  size_t alloc    = m_string_capacity(v);
  size = vsnprintf (&ptr[old_size], alloc - old_size, format, args);
  if (size > 0 && (old_size+(size_t)size+1 >= alloc) ) {
    // Reset the string in a clean state in case of exception
    // to keep the object in a clearable state
    M_IF_EXCEPTION( m_str1ng_set_size(v, old_size) );
    M_IF_EXCEPTION( m_str1ng_get_cstr(v)[old_size] = 0 );
    // We have to realloc our string to fit the needed size
    ptr = m_str1ng_fit2size M_R(v, old_size + (size_t) size + 1);
    alloc = m_string_capacity(v);
    // and redo the parsing.
    size = vsnprintf (&ptr[old_size], alloc - old_size, format, args_org);
    M_ASSERT (size >= 0);
  }
  if (size >= 0) {
    m_str1ng_set_size(v, old_size + (size_t) size);
  } else {
    // vsnprintf may have output some characters before returning an error.
    // Undo this to have a clean state
    ptr[old_size] = 0;
  }
  va_end (args_org);
  M_STR1NG_CONTRACT (v);
  return size;
}

/* Append to the string the formatted string of the given printf format */
M_P(int, m_string, _cat_printf, m_string_t v, const char format[], ...)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT (format != NULL);
  va_list args;
  va_start (args, format);
  int ret = m_string_cat_vprintf M_R(v, format, args);
  va_end (args);
  return ret;
}

#if M_USE_FAST_STRING_CONV == 0
M_P(void, m_string, _set_ui, m_string_t v, unsigned int n)
{
  m_string_printf M_R(v, "%u", n);
}
M_P(void, m_string, _set_si, m_string_t v, int n)
{
  m_string_printf M_R(v, "%d", n);
}
#endif

#endif // Have stdarg

#if M_USE_STDIO

/* Get a line/pure-line/file from the FILE and store it in the string */
M_P(bool, m_string, _fgets, m_string_t v, FILE *f, m_string_fgets_t arg)
{
  M_STR1NG_CONTRACT(v);
  M_ASSERT (f != NULL);
  char *ptr = m_str1ng_fit2size M_R(v, 100);
  size_t size = 0;
  size_t alloc = m_string_capacity(v);
  ptr[0] = 0;
  bool retcode = false; /* Nothing has been read yet */
  /* alloc - size is very unlikely to be bigger than INT_MAX
    but fgets accepts an int as the size argument */
  // NOTE: fgets reads at most one less than 'alloc-size' characters from stream
  while (fgets(&ptr[size], (int) M_MIN( (alloc - size), (size_t) INT_MAX ), f) != NULL) {
    retcode = true; /* Something has been read */
    // fgets doesn't return the number of characters read, so we need to count.
    size += strlen(&ptr[size]);
    M_ASSERT(size >= 1);
    if (arg != M_STRING_READ_FILE && ptr[size-1] == '\n') {
      if (arg == M_STRING_READ_PURE_LINE) {
        size --;
        ptr[size] = 0;         /* Remove EOL */
      }
      m_str1ng_set_size(v, size);
      M_STR1NG_CONTRACT(v);
      return retcode; /* Normal termination */
    } else if (alloc <= 1 + size) {
      // Reset the string in a clean state in case of exception
      // to keep the object in a clearable state
      M_IF_EXCEPTION( m_str1ng_set_size(v, size) );
      M_IF_EXCEPTION( m_str1ng_get_cstr(v)[size] = 0 );
      /* The string buffer is not big enough:
         increase it and continue reading */
      /* v cannot be a full embedded type */
      ptr   = m_str1ng_fit2size M_R(v, alloc + alloc/2);
      alloc = m_string_capacity(v);
    }
  }
  m_str1ng_set_size(v, size);
  M_STR1NG_CONTRACT (v);
  return retcode; /* Abnormal termination */
}

/* Get a word from the FILE and store it in the string.
   Words are supposed to be separated each other by the given list of separator
   separator shall be a CONSTANT C array.
 */
M_P(bool, m_string, _fget_word, m_string_t v, const char separator[], FILE *f)
{
  char buffer[128];
  char c = 0;
  int d;
  M_STR1NG_CONTRACT(v);
  M_ASSERT (f != NULL);
  M_ASSERT_INDEX (1+20+2+strlen(separator)+3, sizeof buffer);
  size_t size = 0;
  bool retcode = false;

  /* Skip separator first */
  do {
    d = fgetc(f);
    if (d == EOF) {
      return false;
    }
  } while (strchr(separator, d) != NULL);
  ungetc(d, f);

  size_t alloc = m_string_capacity(v);
  char *ptr    = m_str1ng_get_cstr(v);
  ptr[0] = 0;

  /* NOTE: We generate a buffer which we give to scanf to parse the string,
     that it is to say, we generate the format dynamically!
     The format is like " %49[^ \t.\n]%c"
     So in this case, we parse up to 49 chars, up to the separators char,
     and we read the next char. If the next char is a separator, we successful
     read a word, otherwise we have to continue parsing.
     The user shall give a constant string as the separator argument,
     as a control over this argument may give an attacker
     an opportunity for stack overflow */
  while (snprintf(buffer, sizeof buffer -1, " %%%zu[^%s]%%c", (size_t) alloc-1-size, separator) > 0
         /* We may read one or two argument(s) */
         && m_core_fscanf(f, buffer, m_core_arg_size(&ptr[size], alloc-size), &c) >= 1) {
    retcode = true;
    size += strlen(&ptr[size]);
    /* If we read only one argument 
       or if the final read character is a separator */
    if (c == 0 || strchr(separator, c) != NULL)
      break;
    /* Next char is not a separator: continue parsing */
    m_str1ng_set_size(v, size);
    M_IF_EXCEPTION( m_str1ng_get_cstr(v)[size] = 0);
    ptr = m_str1ng_fit2size M_R(v, alloc + alloc/2);
    alloc = m_string_capacity(v);
    M_ASSERT (alloc > size + 1);
    ptr[size++] = c;
    ptr[size] = 0;
    // Invalid c character for next iteration
    c= 0;
  }
  m_str1ng_set_size(v, size);
  M_STR1NG_CONTRACT(v);  
  return retcode;
}

/* Put the string in the given FILE without formatting */
M_INLINE bool
m_string_fputs(FILE *f, const m_string_t v)
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT (f != NULL);
  return fputs(m_string_get_cstr(v), f) >= 0;
}

#endif // Have stdio

/* Test if the string starts with the given C string */
M_INLINE bool
m_string_start_with_str_p(const m_string_t v, const char str[])
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT (str != NULL);
  const char *v_str = m_string_get_cstr(v);
  while (*str){
    if (*str != *v_str)
      return false;
    str++;
    v_str++;
  }
  return true;
}

/* Test if the string starts with the other string */
M_INLINE bool
m_string_start_with_string_p(const m_string_t v, const m_string_t v2)
{
  M_STR1NG_CONTRACT (v2);
  return m_string_start_with_str_p (v, m_string_get_cstr(v2));
}

/* Test if the string ends with the C string */
M_INLINE bool
m_string_end_with_str_p(const m_string_t v, const char str[])
{
  M_STR1NG_CONTRACT (v);
  M_ASSERT (str != NULL);
  const char *v_str  = m_string_get_cstr(v);
  const size_t v_l   = m_string_size(v);
  const size_t str_l = strlen(str);
  if (v_l < str_l)
    return false;
  return (memcmp(str, &v_str[v_l - str_l], str_l) == 0);
}

/* Test if the string ends with the other string */
M_INLINE bool
m_string_end_with_string_p(const m_string_t v, const m_string_t v2)
{
  M_STR1NG_CONTRACT (v2);
  return m_string_end_with_str_p (v, m_string_get_cstr(v2));
}

/* Compute a hash for the string */
M_INLINE size_t
m_string_hash(const m_string_t v)
{
  M_STR1NG_CONTRACT (v);
  return m_core_hash(m_string_get_cstr(v), m_string_size(v));
}

// Return true if c is a character from the C-String tab
M_INLINE bool
m_str1ng_strim_char(char c, const char tab[])
{
  for(const char *s = tab; *s; s++) {
    if (c == *s)
      return true;
  }
  return false;
}

/* Remove any characters from tab that are present 
   in the beginning of the string and the end of the string. */
M_INLINE void
m_string_strim(m_string_t v, const char tab[])
{
  M_STR1NG_CONTRACT (v);
  char *ptr = m_str1ng_get_cstr(v);
  char *b   = ptr;
  size_t size = m_string_size(v);
  while (size > 0 && m_str1ng_strim_char(b[size-1], tab))
    size --;
  if (size > 0) {
    while (m_str1ng_strim_char(*b, tab))
      b++;
    M_ASSERT (b >= ptr &&  size >= (size_t) (b - ptr) );
    size -= (size_t) (b - ptr);
    memmove (ptr, b, size);
  }
  ptr[size] = 0;
  m_str1ng_set_size(v, size);
  M_STR1NG_CONTRACT (v);
}

/* Test if the string is equal to the OOR value */
M_INLINE bool
m_string_oor_equal_p(const m_string_t s, unsigned char n)
{
  return (s->u.buffer[sizeof (m_str1ng_heap_ct)-2] == 0) & (s->u.buffer[sizeof (m_str1ng_heap_ct)-1] == (char) ~n);
}

/* Set the uninitialized string to the OOR value */
M_INLINE void
m_string_oor_set(m_string_t s, unsigned char n)
{
  s->u.buffer[sizeof (m_str1ng_heap_ct)-2] = 0;
  s->u.buffer[sizeof (m_str1ng_heap_ct)-1] = (char) ~n;
}

/* I/O */
/* Output: "string" with quote around
   Replace " by \" within the string (and \ to \\)
   \n, \t & \r by their standard representation
   and other not printable character with \0xx */

/* Transform the string 'v2' into a formatted string
   and set it to (or append in) the string 'v'. */
M_P(void, m_string, _get_str, m_string_t v, const m_string_t v2, bool append)
{
  M_STR1NG_CONTRACT(v2);
  M_STR1NG_CONTRACT(v);
  M_ASSERT (v != v2); // Limitation
  size_t size = append ? m_string_size(v) : 0;
  size_t v2_size = m_string_size(v2);
  size_t targetSize = size + v2_size + 3;
  char *ptr = m_str1ng_fit2size M_R(v, targetSize);
  ptr[size ++] = '"';
  for(size_t i = 0 ; i < v2_size; i++) {
    const char c = m_string_get_char(v2,i);
    switch (c) {
    case '\\':
    case '"':
    case '\n':
    case '\t':
    case '\r':
      // Special characters which can be displayed in a short form.
      m_str1ng_set_size(v, size);
      ptr = m_str1ng_fit2size M_R(v, ++targetSize);
      ptr[size ++] = '\\';
      // This string acts as a perfect hashmap which supposes an ASCII mapping
      // and (c^(c>>5)) is the hash function
      ptr[size ++] = " tn\" r\\"[(c ^ (c >>5)) & 0x07];
      break;
    default:
      if (M_UNLIKELY (!isprint((unsigned char) c))) {
        targetSize += 3;
        m_str1ng_set_size(v, size);
        M_IF_EXCEPTION(m_str1ng_get_cstr(v)[size] = 0);
        ptr = m_str1ng_fit2size M_R(v, targetSize);
        int d1 = c & 0x07, d2 = (c>>3) & 0x07, d3 = (c>>6) & 0x07;
        ptr[size ++] = '\\';
        ptr[size ++] = (char) ('0' + d3);
        ptr[size ++] = (char) ('0' + d2);
        ptr[size ++] = (char) ('0' + d1);
      } else {
        ptr[size ++] = c;
      }
      break;
    }
  }
  ptr[size ++] = '"';
  ptr[size] = 0;
  m_str1ng_set_size(v, size);
  M_ASSERT (size <= targetSize);
  M_STR1NG_CONTRACT (v);
}

#if M_USE_STDIO

/* Transform the string 'v2' into a formatted string
   and output it in the given FILE */
M_INLINE void
m_string_out_str(FILE *f, const m_string_t v)
{
  M_STR1NG_CONTRACT(v);
  M_ASSERT (f != NULL);
  fputc('"', f);
  size_t size = m_string_size(v);
  for(size_t i = 0 ; i < size; i++) {
    const char c = m_string_get_char(v, i);
    switch (c) {
    case '\\':
    case '"':
    case '\n':
    case '\t':
    case '\r':
      fputc('\\', f);
      fputc(" tn\" r\\"[(c ^ c >>5) & 0x07], f);
      break;
    default:
      if (M_UNLIKELY (!isprint(c))) {
        int d1 = c & 0x07, d2 = (c>>3) & 0x07, d3 = (c>>6) & 0x07;
        fputc('\\', f);
        fputc('0' + d3, f);
        fputc('0' + d2, f);
        fputc('0' + d1, f);
      } else {
        fputc(c, f);
      }
      break;
    }
  }
  fputc('"', f);
}

/* Read the formatted string from the FILE
   and set the converted value in the string 'v'.
   Return true in case of success */
M_P(bool, m_string, _in_str, m_string_t v, FILE *f)
{
  M_STR1NG_CONTRACT(v);
  M_ASSERT (f != NULL);
  int c = fgetc(f);
  if (c != '"') return false;
  m_string_reset(v);
  c = fgetc(f);
  while (c != '"' && c != EOF) {
    if (M_UNLIKELY (c == '\\')) {
      c = fgetc(f);
      switch (c) {
      case 'n':
      case 't':
      case 'r':
      case '\\':
      case '\"':
        // This string acts as a perfect hashmap which supposes an ASCII mapping
        // and (c^(c>>5)) is the hash function
        c = " \r \" \n\\\t"[(c^(c>>5))& 0x07];
        break;
      default:
        if (!(c >= '0' && c <= '7'))
          return false;
        int d1 = c - '0';
        c = fgetc(f);
        if (!(c >= '0' && c <= '7'))
          return false;
        int d2 = c - '0';
        c = fgetc(f);
        if (!(c >= '0' && c <= '7'))
          return false;
        int d3 = c - '0';
        c = (d1 << 6) + (d2 << 3) + d3;
        break;
      }
    }
    m_string_push_back M_R(v, (char) c);
    c = fgetc(f);
  }
  return c == '"';
}

#endif // Have stdio

/* Read the formatted string from the C string
   and set the converted value in the string 'v'.
   Return true in case of success
   If endptr is not null, update the position of the parsing.
*/
M_P(bool, m_string, _parse_str, m_string_t v, const char str[], const char **endptr)
{
  M_STR1NG_CONTRACT(v);
  M_ASSERT (str != NULL);
  bool success = false;
  int c = *str++;
  if (c != '"') goto exit;
  m_string_reset(v);
  c = *str++;
  while (c != '"' && c != 0) {
    if (M_UNLIKELY (c == '\\')) {
      c = *str++;
      switch (c) {
      case 'n':
      case 't':
      case 'r':
      case '\\':
      case '\"':
        // This string acts as a perfect hashmap which supposes an ASCII mapping
        // and (c^(c>>5)) is the hash function
        c = " \r \" \n\\\t"[(c^(c>>5))& 0x07];
        break;
      default:
        if (!(c >= '0' && c <= '7'))
          goto exit;
        int d1 = c - '0';
        c = *str++;
        if (!(c >= '0' && c <= '7'))
          goto exit;
        int d2 = c - '0';
        c = *str++;
        if (!(c >= '0' && c <= '7'))
          goto exit;
        int d3 = c - '0';
        c = (d1 << 6) + (d2 << 3) + d3;
        break;
      }
    }
    m_string_push_back M_R(v, (char) c);
    c = *str++;
  }
  success = (c == '"');
 exit:
  if (endptr != NULL) *endptr = str;
  return success;
}

/* Transform the string 'v2' into a formatted string
   and output it in the given serializer
   See serialization for return code.
*/
M_P(m_serial_return_code_t, m_string, _out_serial, m_serial_write_t serial, const m_string_t v)
{
  M_ASSERT (serial != NULL && serial->m_interface != NULL);
  return serial->m_interface->write_string M_R(serial, m_string_get_cstr(v), m_string_size(v) );
}

/* Read the formatted string from the serializer
   and set the converted value in the string 'v'.
   See serialization for return code.
*/
M_P(m_serial_return_code_t, m_string, _in_serial, m_string_t v, m_serial_read_t serial)
{
  M_ASSERT (serial != NULL && serial->m_interface != NULL);
  return serial->m_interface->read_string M_R(serial, v);
}

/* UTF8 character classification:
 * 
 * 0*       --> type 1 byte  A
 * 10*      --> chained byte B
 * 110*     --> type 2 byte  C
 * 1110*    --> type 3 byte  D
 * 11110*   --> type 4 byte  E
 * 111110*  --> invalid      I
 */
/* UTF8 State Transition table:
 *    ABCDEI
 *   +------
 *  S|SI123III
 *  1|ISIIIIII
 *  2|I1IIIIII
 *  3|I2IIIIII
 *  I|IIIIIIII
 */

/* The use of a string enables the compiler/linker to factorize it. */
#define M_STR1NG_UTF8_STATE_TAB                                               \
  "\000\040\010\020\030\040\040\040"                                          \
  "\040\000\040\040\040\040\040\040"                                          \
  "\040\010\040\040\040\040\040\040"                                          \
  "\040\020\040\040\040\040\040\040"                                          \
  "\040\040\040\040\040\040\040\040"

/* Main generic UTF8 decoder
   It shall be (nearly) branchless on any CPU.
   It takes a byte, and the previous state and the previous value of the unicode code point.
   It updates the state and the decoded unicode code point.
   A decoded unicode code point is valid only when the state is STARTING.
 */
M_INLINE void
m_str1ng_utf8_decode(char c, m_str1ng_utf8_state_e *state,
                    m_string_unicode_t *unicode)
{
  const unsigned int type = m_core_clz32((unsigned char)~c) - (unsigned) (sizeof(uint32_t) - 1) * CHAR_BIT;
  const m_string_unicode_t mask1 = (UINT32_MAX - (m_string_unicode_t)(*state != M_STR1NG_UTF8_STARTING) + 1);
  const m_string_unicode_t mask2 = (0xFFU >> type);
  const m_string_unicode_t c1 = (m_string_unicode_t) c;
  *unicode = ((*unicode << 6) & mask1) | (c1 & mask2);
  *state = (m_str1ng_utf8_state_e) M_STR1NG_UTF8_STATE_TAB[(unsigned int) *state + type];
}

/* Check if the given array of characters is a valid UTF8 stream */
/* NOTE: Non-canonical representation are not always rejected */
M_INLINE bool
m_str1ng_utf8_valid_str_p(const char str[])
{
  m_str1ng_utf8_state_e s = M_STR1NG_UTF8_STARTING;
  m_string_unicode_t u = 0;
  while (*str) {
    m_str1ng_utf8_decode(*str, &s, &u);
    if ((s == M_STR1NG_UTF8_ERROR)
        ||(s == M_STR1NG_UTF8_STARTING
           &&(u > 0x10FFFF /* out of range */
              ||(u >= 0xD800 && u <= 0xDFFF) /* surrogate halves */)))
      return false;
    str++;
  }
  return true;
}

/* Test if the given byte is the start of an UTF8 code point */
M_INLINE bool
m_str1ng_utf8_start_p(unsigned char val)
{
  return ((val & 0xC0u) != 0x80u);
}

/* Computer the number of unicode code points are encoded in the UTF8 stream */
M_INLINE size_t
m_str1ng_utf8_length(const char str[])
{
  size_t size = 0;
  while (*str) {
    unsigned char val = (unsigned char) *str++;
    size += m_str1ng_utf8_start_p(val);
  }
  return size;
}

/* Encode an unicode code point into an UTF8 stream */
M_INLINE int
m_str1ng_utf8_encode(char buffer[5], m_string_unicode_t u)
{
  if (M_LIKELY (u <= 0x7Fu)) {
    buffer[0] = (char) u;
    buffer[1] = 0;
    return 1;
  } else if (u <= 0x7FFu) {
    buffer[0] = (char) (0xC0u | (u >> 6));
    buffer[1] = (char) (0x80 | (u & 0x3Fu));
    buffer[2] = 0;
    return 2;
  } else if (u <= 0xFFFFu) {
    buffer[0] = (char) (0xE0u | (u >> 12));
    buffer[1] = (char) (0x80u | ((u >> 6) & 0x3Fu));
    buffer[2] = (char) (0x80u | (u & 0x3Fu));
    buffer[3] = 0;
    return 3;
  } else {
    buffer[0] = (char) (0xF0u | (u >> 18));
    buffer[1] = (char) (0x80u | ((u >> 12) & 0x3Fu));
    buffer[2] = (char) (0x80u | ((u >> 6) & 0x3Fu));
    buffer[3] = (char) (0x80u | (u & 0x3F));
    buffer[4] = 0;
    return 4;
  }
}

/* Start iteration over the UTF8 encoded unicode code point */
M_INLINE void
m_string_it(m_string_it_t it, const m_string_t str)
{
  M_STR1NG_CONTRACT(str);
  M_ASSERT(it != NULL);
  it->ptr      = m_string_get_cstr(str);
  it->u        = 0;
  it->string   = str;
  M_STR1NG_IT_CONTRACT(it);
} 

/* Set the iterator to the end of string 
   The iterator references therefore nothing.
*/
M_INLINE void
m_string_it_end(m_string_it_t it, const m_string_t str)
{
  M_STR1NG_CONTRACT(str);
  M_ASSERT(it != NULL);
  it->ptr      = &m_string_get_cstr(str)[m_string_size(str)];
  it->u        = 0;
  it->string   = str;
  M_STR1NG_IT_CONTRACT(it);
}

/* Set the iterator to the same position than the other one */
M_INLINE void
m_string_it_set(m_string_it_t it, const m_string_it_t itsrc)
{
  M_ASSERT(it != NULL && itsrc != NULL);
  M_STR1NG_IT_CONTRACT(itsrc);
  it->ptr      = itsrc->ptr;
  it->u        = itsrc->u;
  it->string   = itsrc->string;
  M_STR1NG_IT_CONTRACT(it);
}

/* Set the iterator to the given position in the string.
   The given position shall reference a valid code point in the string.
 */
M_INLINE void
m_string_it_pos(m_string_it_t it, const m_string_t str, const size_t n)
{
  M_ASSERT(it != NULL);
  M_STR1NG_CONTRACT(str);
  // The offset shall be within the string
  M_ASSERT(n <= m_string_size(str));
  // The offset shall reference the first Byte of an UTF 8 Code point
  M_ASSERT(m_str1ng_utf8_start_p ((unsigned char)m_string_get_cstr(str)[n]));
  it->ptr      = &m_string_get_cstr(str)[n];
  it->u        = 0;
  it->string   = str;
  M_STR1NG_IT_CONTRACT(it);
}

/* Return the current offset in the string referenced by the iterator.
   This references avalid code point.
 */
M_INLINE size_t
m_string_it_get_pos(m_string_it_t it)
{
  M_STR1NG_IT_CONTRACT(it);
  return (size_t) (it->ptr - m_string_get_cstr(it->string));
}

/* Test if the iterator has reached the end of the string. */
M_INLINE bool
m_string_end_p (m_string_it_t it)
{
  M_STR1NG_IT_CONTRACT(it);
  return it->ptr[0] == 0;
}

/* Test if the iterator is equal to the other one */
M_INLINE bool
m_string_it_equal_p(const m_string_it_t it1, const m_string_it_t it2)
{
  M_STR1NG_IT_CONTRACT(it1);
  M_STR1NG_IT_CONTRACT(it2);
  return it1->ptr == it2->ptr;
}

/* Advance the iterator to the next UTF8 unicode code point */
M_INLINE void
m_string_next (m_string_it_t it)
{
  M_STR1NG_IT_CONTRACT(it);
  const char *ptr = it->ptr;
  while (*ptr != 0) {
    ptr ++;
    if (m_str1ng_utf8_start_p((unsigned char) *ptr) ) {
      /* Start of an UTF 8 code point */
      break;
    }
  }
  it->ptr = ptr;
  return;
}

/* Move the iterator to the previous code point */
M_INLINE void
m_string_previous(m_string_it_t it)
{
  M_STR1NG_IT_CONTRACT(it);
  const char *ptr = it->ptr;
  const char *org = m_string_get_cstr(it->string);
  while (ptr > org) {
    ptr --;
    if (m_str1ng_utf8_start_p((unsigned char) *ptr) ) {
      /* Start of an UTF 8 code point */
      it->ptr = ptr;
      return;
    }
  }
  /* We reach the start of the string: mark the iterator to the end */
  it->ptr = &org[m_string_size(it->string)];
  M_STR1NG_IT_CONTRACT(it);
}

/* Return the unicode code point associated to the iterator */
M_INLINE m_string_unicode_t
m_string_get_cref (const m_string_it_t it)
{
  M_STR1NG_IT_CONTRACT(it);
  M_ASSERT(*it->ptr != 0);

  m_str1ng_utf8_state_e state =  M_STR1NG_UTF8_STARTING;
  m_string_unicode_t u = 0;
  const char *str = it->ptr;
  do {
    m_str1ng_utf8_decode(*str, &state, &u);
    str++;
  } while (state != M_STR1NG_UTF8_STARTING && state != M_STR1NG_UTF8_ERROR && *str != 0);
  // Save where the current unicode value ends in the UTF8 steam
  M_ASSERT( (str > it->ptr) && (str - it->ptr) <= M_STR1NG_MAX_BYTE_UTF8);
  // Save the decoded unicode value
  return M_UNLIKELY (state == M_STR1NG_UTF8_ERROR) ? M_STRING_UNICODE_ERROR : u;
}

/* Return the unicode code point associated to the iterator */
M_INLINE const m_string_unicode_t *
m_string_cref (m_string_it_t it)
{
  M_STR1NG_IT_CONTRACT(it);
  it->u = m_string_get_cref(it);
  return &it->u;
}

/* Update the value referenced by the iterator to the given value */
M_P(void, m_string, _it_set_ref, m_string_it_t it, m_string_t s, m_string_unicode_t new_u)
{
  M_STR1NG_IT_CONTRACT(it);
  M_STR1NG_CONTRACT(s);
  M_ASSERT(s == it->string);
  char *ptr = m_str1ng_get_cstr(s);
  M_ASSUME( it->ptr >= ptr);
  size_t offset = (size_t) (it->ptr - ptr);
  // Special case if the unicode codepoint is 0
  if (M_UNLIKELY(new_u == 0)) {
    // Null the string
    m_str1ng_set_size(s, offset);
    ptr[offset] = 0;
    M_STR1NG_IT_CONTRACT(it);
    M_STR1NG_CONTRACT(s);
    return;
  }
  // Encode the new unicode code point & compute its size
  char buffer[4+1];
  m_str1ng_utf8_encode(buffer, new_u);
  size_t new_u_size = strlen(buffer);
  // Compute the size of the old unicode code point
  m_str1ng_utf8_state_e state =  M_STR1NG_UTF8_STARTING;
  m_string_unicode_t old_u = 0;
  const char *str = it->ptr;
  do {
    m_str1ng_utf8_decode(*str, &state, &old_u);
    str++;
  } while (state != M_STR1NG_UTF8_STARTING && state != M_STR1NG_UTF8_ERROR && *str != 0);
  M_ASSUME( str > it->ptr);
  size_t old_u_size = (size_t) (str - it->ptr);
  // We need to replace old_u by new_u. Both are variable length
  size_t str_size = m_string_size(s);
  if (M_UNLIKELY (new_u_size != old_u_size)) {
    ptr = m_str1ng_fit2size M_R(s, str_size + new_u_size - old_u_size + 1);
    M_ASSUME( str_size+1 > (offset + old_u_size) );
    memmove(&ptr[offset+new_u_size], &ptr[offset + old_u_size],
            str_size+1 - offset - old_u_size);
    m_str1ng_set_size(s, str_size + new_u_size - old_u_size);
  }
  memcpy(&ptr[offset], &buffer[0], new_u_size);
  it->ptr = &ptr[offset];
  M_STR1NG_IT_CONTRACT(it);
  M_STR1NG_CONTRACT(s);
  return;
}

/* Push unicode code point into string, encoding it in UTF8 */
M_P(void, m_string, _push_u, m_string_t str, m_string_unicode_t u)
{
  M_STR1NG_CONTRACT(str);
  char buffer[4+1];
  m_str1ng_utf8_encode(buffer, u);
  m_string_cat_cstr M_R(str, buffer);
}

/* Pop last unicode code point into string, encoding it in UTF8 */
M_INLINE  bool
m_string_pop_u(m_string_unicode_t *u, m_string_t str)
{
  M_STR1NG_CONTRACT(str);
  char *org = m_str1ng_get_cstr(str);
  size_t len = m_string_size(str);
  // From the last byte in the string
  while (len > 0) {
    len--;
    // Test if it is a start of an UTF8 code point
    if (m_str1ng_utf8_start_p((unsigned char) org[len])) {
      // Yes, so decode the UTF8
      m_str1ng_utf8_state_e state = M_STR1NG_UTF8_STARTING;
      const char *tmp = &org[len];
      // Support of NULL pointer
      m_string_unicode_t u_tmp;
      m_string_unicode_t *u_ptr = u == NULL ? &u_tmp : u;
      *u_ptr = 0;
      do {
        m_str1ng_utf8_decode(*tmp++, &state, u_ptr);
      } while (state != M_STR1NG_UTF8_STARTING && state != M_STR1NG_UTF8_ERROR);
      // Final null char for the string
      org[len] = 0;
      m_str1ng_set_size(str, len);
      M_STR1NG_CONTRACT(str);
      return true;
    }
  }
  // Fail to pop a unicode code
  return false;
}

/* Compute the length in UTF8 code points in the string */
M_INLINE size_t
m_string_length_u(const m_string_t str)
{
  M_STR1NG_CONTRACT(str);
  return m_str1ng_utf8_length(m_string_get_cstr(str));
}

/* Check if a string is a valid UTF8 encoded stream */
M_INLINE bool
m_string_utf8_p(const m_string_t str)
{
  M_STR1NG_CONTRACT(str);
  return m_str1ng_utf8_valid_str_p(m_string_get_cstr(str));
}


/* Define the split & the join functions 
   in case of usage with the algorithm module */
#define M_STR1NG_SPLIT(name, oplist, type_oplist)                             \
  M_P(void, name, _split, M_GET_TYPE oplist cont, const m_string_t str, const char sep) \
  {                                                                           \
    size_t begin = 0;                                                         \
    m_string_t tmp;                                                           \
    size_t size = m_string_size(str);                                         \
    m_string_init(tmp);                                                       \
    M_CALL_RESET(oplist, cont);                                               \
    for(size_t i = 0 ; i < size; i++) {                                       \
      char c = m_string_get_char(str, i);                                     \
      if (c == sep) {                                                         \
        m_string_set_cstrn M_R(tmp, &m_string_get_cstr(str)[begin], i - begin); \
        /* If push move method is available, use it */                        \
        M_IF_METHOD(PUSH_MOVE,oplist)(                                        \
                                      M_CALL_PUSH_MOVE(oplist, cont, &tmp);   \
                                      m_string_init(tmp);                     \
                                      ,                                       \
                                      M_CALL_PUSH(oplist, cont, tmp);         \
                                                                        )     \
          begin = i + 1;                                                      \
      }                                                                       \
    }                                                                         \
    m_string_set_cstrn M_R(tmp, &m_string_get_cstr(str)[begin], size - begin); \
    M_CALL_PUSH(oplist, cont, tmp);                                           \
    /* HACK: if method reverse is defined, it is likely that we have */       \
    /* inserted the items in the wrong order (aka for a list) */              \
    M_IF_METHOD(REVERSE, oplist) (M_CALL_REVERSE(oplist, cont);, )            \
    m_string_clear M_R(tmp);                                                  \
  }                                                                           \
                                                                              \
  M_P(void, name, _join, m_string_t dst, M_GET_TYPE oplist cont, const m_string_t str) \
  {                                                                           \
    bool init_done = false;                                                   \
    m_string_reset (dst);                                                     \
    for M_EACH(item, cont, oplist) {                                          \
        if (init_done) {                                                      \
          m_string_cat M_R(dst, str);                                         \
        }                                                                     \
        m_string_cat M_R(dst, *item);                                         \
        init_done = true;                                                     \
    }                                                                         \
  }                                                                           \


/* Use of Compound Literals to init a constant string.
   NOTE: The use of the additional structure layer is to ensure
   that the pointer to char is properly aligned to an int (this
   is a needed assumption by m_string_hash).
   Otherwise it could have been :
   #define M_STRING_CTE(s)                                                    \
     ((const m_string_t){{.size = sizeof(s)-1, .alloc = sizeof(s),            \
     .ptr = s}})
   which produces faster code.
   Note: This code doesn't work with C++ (use of c99 feature
   of recursive struct definition and compound literal). 
   As such, there is a separate C++ definition.
*/
#ifndef __cplusplus
/* Initialize a constant string with the given C string */
# define M_STRING_CTE(s)                                                      \
  (m_string_srcptr)((const m_string_t){{.u.heap = { .size = sizeof(s)-1,      \
        .alloc = { [sizeof(m_str1ng_size_t)-2] = 1, [sizeof(m_str1ng_size_t)-1] = 31}, \
        .ptr = ((struct { long long _n; char _d[sizeof (s)]; }){ 0, s })._d }}})
#else
namespace m_lib {
  template <unsigned int N>
    struct m_aligned_string {
      m_string_t string;
      union  {
        char str[N];
        long long i;
      };
    inline m_aligned_string(const char lstr[])
      {
        this->string->u.heap.size = N -1;
        this->string->u.heap.alloc[sizeof(m_str1ng_size_t)-2] = 1;
        this->string->u.heap.alloc[sizeof(m_str1ng_size_t)-1] = 31;
        memcpy (this->str, lstr, N);
        this->string->u.heap.ptr = this->str;
      }
    };
}
/* Initialize a constant string with the given C string (C++ mode) */
#define M_STRING_CTE(s)                                                       \
  m_lib::m_aligned_string<sizeof (s)>(s).string
#endif

#ifdef M_USE_CONTEXT
/* Initialize and set a string to the given formatted value. */
#define m_string_init_printf(pool, v, ...)                                    \
  (m_string_printf ( pool, m_str1ng_init_ref(v), __VA_ARGS__) )

/* Initialize and set a string to the given formatted value. */
#define m_string_init_vprintf(pool, v, format, args)                          \
  (m_string_vprintf ( pool, m_str1ng_init_ref(v), format, args) )
#else
#define m_string_init_printf(v, ...)                                          \
  (m_string_printf ( m_str1ng_init_ref(v), __VA_ARGS__) )

/* Initialize and set a string to the given formatted value. */
#define m_string_init_vprintf(v, format, args)                                \
  (m_string_vprintf ( m_str1ng_init_ref(v), format, args) )
#endif

/* Initialize a string with the given list of arguments.
   Check if it is a formatted input or not by counting the number of arguments.
   If there is only one argument, it can only be a set to C string.
   It is much faster in this case to call m_string_init_set_cstr.
   In C11 mode, it uses the fact that m_string_init_set is overloaded to handle both
   C string and string. */
#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L)
#define M_STR1NG_INIT_WITH(v, ...)                                            \
  M_IF_NARGS_EQ1(__VA_ARGS__)(m_string_init_set, m_string_init_printf)M_R(v, __VA_ARGS__)
#else
#define M_STR1NG_INIT_WITH(v, ...)                                            \
  M_IF_NARGS_EQ1(__VA_ARGS__)(m_string_init_set_cstr, m_string_init_printf)M_R(v, __VA_ARGS__)
#endif

/* Define the OPLIST of a STRING */
#ifndef M_USE_CONTEXT
#define M_STRING_OPLIST                                                       \
  (INIT(m_string_init),INIT_SET(m_string_init_set), SET(m_string_set),        \
   INIT_WITH(M_STR1NG_INIT_WITH),                                             \
   INIT_MOVE(m_string_init_move), MOVE(m_string_move),                        \
   SWAP(m_string_swap), RESET(m_string_reset),                                \
   EMPTY_P(m_string_empty_p),                                                 \
   CLEAR(m_string_clear), HASH(m_string_hash), EQUAL(m_string_equal_p),       \
   CMP(m_string_cmp), TYPE(m_string_t), GENTYPE(struct m_string_s*),          \
   PARSE_STR(m_string_parse_str), GET_STR(m_string_get_str),                  \
   OUT_STR(m_string_out_str), IN_STR(m_string_in_str),                        \
   OUT_SERIAL(m_string_out_serial), IN_SERIAL(m_string_in_serial),            \
   EXT_ALGO(M_STR1NG_SPLIT),                                                  \
   OOR_EQUAL(m_string_oor_equal_p), OOR_SET(m_string_oor_set)                 \
   ,SUBTYPE(m_string_unicode_t)                                               \
   ,IT_TYPE(m_string_it_t)                                                    \
   ,IT_FIRST(m_string_it)                                                     \
   ,IT_END(m_string_it_end)                                                   \
   ,IT_SET(m_string_it_set)                                                   \
   ,IT_END_P(m_string_end_p)                                                  \
   ,IT_EQUAL_P(m_string_it_equal_p)                                           \
   ,IT_NEXT(m_string_next)                                                    \
   ,IT_CREF(m_string_cref), PUSH(m_string_push_u)                             \
   ,EMPLACE_TYPE(const char*)                                                 \
   )
#else
#define M_STRING_OPLIST                                                       \
   (INIT(m_string_init),INIT_SET(API_0P(m_string_init_set)), SET(API_0P(m_string_set)), \
    INIT_WITH(M_STR1NG_INIT_WITH),                                            \
    INIT_MOVE(m_string_init_move), MOVE(API_0P(m_string_move)),               \
    SWAP(m_string_swap), RESET(m_string_reset),                               \
    EMPTY_P(m_string_empty_p),                                                \
    CLEAR(API_0P(m_string_clear)), HASH(m_string_hash), EQUAL(m_string_equal_p), \
    CMP(m_string_cmp), TYPE(m_string_t), GENTYPE(struct m_string_s*),         \
    PARSE_STR(API_0P(m_string_parse_str)), GET_STR(API_0P(m_string_get_str)), \
    OUT_STR(m_string_out_str), IN_STR(API_0P(m_string_in_str)),               \
    OUT_SERIAL(API_0P(m_string_out_serial)), IN_SERIAL(API_0P(m_string_in_serial)), \
    EXT_ALGO(M_STR1NG_SPLIT),                                                 \
    OOR_EQUAL(m_string_oor_equal_p), OOR_SET(m_string_oor_set)                \
    ,SUBTYPE(m_string_unicode_t)                                              \
    ,IT_TYPE(m_string_it_t)                                                   \
    ,IT_FIRST(m_string_it)                                                    \
    ,IT_END(m_string_it_end)                                                  \
    ,IT_SET(m_string_it_set)                                                  \
    ,IT_END_P(m_string_end_p)                                                 \
    ,IT_EQUAL_P(m_string_it_equal_p)                                          \
    ,IT_NEXT(m_string_next)                                                   \
    ,IT_CREF(m_string_cref), PUSH(API_0P(m_string_push_u))                    \
    ,EMPLACE_TYPE(const char*)                                                \
    )
#endif

/* Register the OPLIST as a global one */
#define M_OPL_m_string_t() M_STRING_OPLIST

/* Register the string_t oplist as a generic type */
#define M_GENERIC_ORG_MLIB_COMP_CORE_OPLIST_1() M_STRING_OPLIST


/***********************************************************************/
/*                                                                     */
/*  Macro encapsulation to give a default value of 0 for start offset  */
/*                                                                     */
/***********************************************************************/

/* Search for a character in a string (string, character[, start=0]) */
#define m_string_search_char(...)                                             \
  m_string_search_char(M_DEFAULT_ARGS(3, (0), __VA_ARGS__))

/* Reverse Search for a character in a string (string, character[, start=0]) */
#define m_string_search_rchar(...)                                            \
  m_string_search_rchar(M_DEFAULT_ARGS(3, (0), __VA_ARGS__))

/* Search for a C string in a string (string, c_string[, start=0]) */
#define m_string_search_cstr(...)                                             \
  m_string_search_cstr(M_DEFAULT_ARGS(3, (0), __VA_ARGS__))

/* Search for a string in a string (string, string[, start=0]) */
#define m_string_search(...)                                                  \
  m_string_search(M_DEFAULT_ARGS(3, (0), __VA_ARGS__))

/* PBRK for a string in a string (string, string[, start=0]) */
#define m_string_search_pbrk(...)                                             \
  m_string_search_pbrk(M_DEFAULT_ARGS(3, (0), __VA_ARGS__))

/* Replace a C string to another C string in a string (string, c_src_string, c_dst_string, [, start=0]) */
#define m_string_replace_cstr(...)                                            \
  m_string_replace_cstr(M_DEFAULT_ARGS(4, (0), __VA_ARGS__))

/* Replace a string to another string in a string (string, src_string, dst_string, [, start=0]) */
#define m_string_replace(...)                                                 \
  m_string_replace(M_DEFAULT_ARGS(4, (0), __VA_ARGS__))

/* Strim a string from the given set of characters (default is " \n\r\t") */
#define m_string_strim(...)                                                   \
  m_string_strim(M_DEFAULT_ARGS(2, ("  \n\r\t"), __VA_ARGS__))

/* Concat a set strings (or const char * if C11)) into one string */
#define m_string_cats(a, ...)                                                 \
  M_MAP2_C(m_string_cat, a, __VA_ARGS__)

/* Set a string to a set strings (or const char * if C11)) */
#define m_string_sets(a, ...)                                                 \
  (m_string_reset(a), M_MAP2_C(m_string_cat, a, __VA_ARGS__) )

/* Macro encapsulation for C11 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L

/* Select either the string function or the C string function depending on
   the b parameter of the function.
   func1 is the string function / func2 is the str function. */
# define M_STR1NG_SELECT2(func1,func2,a,b)                                    \
  _Generic((b)+0,                                                             \
           char*: func2,                                                      \
           const char *: func2,                                               \
           default : func1                                                    \
           )(a,b)
# define M_STR1NG_SELECT3(func1,func2,a,b,c)                                  \
  _Generic((b)+0,                                                             \
           char*: func2,                                                      \
           const char *: func2,                                               \
           default : func1                                                    \
           )(a,b,c)

/* Init & Set the string a to the string (or C string) b (constructor) */
#ifdef M_USE_CONTEXT
#define m_string_init_set(p, a,b)                                             \
    _Generic((b)+0,                                                           \
    char*: m_string_init_set_cstr,                                            \
    const char *: m_string_init_set_cstr,                                     \
    default : m_string_init_set                                               \
    )(p,a,b)
#else
#define m_string_init_set(a,b) M_STR1NG_SELECT2(m_string_init_set, m_string_init_set_cstr, a, b)
#endif

/* Set the string a to the string (or C string) b */
#ifdef M_USE_CONTEXT
#define m_string_set(p, a,b)                                                  \
    _Generic((b)+0,                                                           \
    char*: m_string_set_cstr,                                                 \
    const char *: m_string_set_cstr,                                          \
    default : m_string_set                                                    \
    )(p,a,b)
#else
#define m_string_set(a,b) M_STR1NG_SELECT2(m_string_set, m_string_set_cstr, a, b)
#endif

/* Concatenate the string (or C string) b to the string a */
#ifdef M_USE_CONTEXT
#define m_string_cat(p, a,b)                                                  \
    _Generic((b)+0,                                                           \
    char*: m_string_cat_cstr,                                                 \
    const char *: m_string_cat_cstr,                                          \
    default : m_string_cat                                                    \
    )(p,a,b)
#else
#define m_string_cat(a,b) M_STR1NG_SELECT2(m_string_cat, m_string_cat_cstr, a, b)
#endif

/* Compare the string a to the string (or C string) b and return the sort order */
#define m_string_cmp(a,b) M_STR1NG_SELECT2(m_string_cmp, m_string_cmp_cstr, a, b)

/* Compare for equality the string a to the string (or C string) b */
#define m_string_equal_p(a,b) M_STR1NG_SELECT2(m_string_equal_p, m_string_equal_cstr_p, a, b)

/* strcoll the string a to the string (or C string) b */
#define m_string_strcoll(a,b) M_STR1NG_SELECT2(m_string_strcoll, m_string_strcoll_cstr, a, b)

#undef m_string_search
/* Search for a string in a string (or C string) (string, string[, start=0]) */
#define m_string_search(...)                                                  \
  M_APPLY(M_STR1NG_SELECT3, m_string_search, m_string_search_cstr,            \
          M_DEFAULT_ARGS(3, (0), __VA_ARGS__) )
#endif


/***********************************************************************/
/*                                                                     */
/*    Override m-core default macros to integrate m_string_t in core     */
/*                                                                     */
/***********************************************************************/

/* Internal Macro: Provide GET_STR method to default type */
#undef M_GET_STR_METHOD_FOR_DEFAULT_TYPE
#define M_GET_STR_METHOD_FOR_DEFAULT_TYPE GET_STR(M_GET_STRING_ARG)

/* Internal Macro: Provide support of m_string_t to the macro M_PRINT in C11 */

// Extend the format specifier to support m_string_t
#undef M_PRINTF_FORMAT_EXTEND
#define M_PRINTF_FORMAT_EXTEND()                                              \
          , m_string_ptr: "%s"                                                \
          , m_string_srcptr: "%s"

// Add type conversion for m_string_t (into a const char*)
#undef M_CORE_PRINTF_ARG
#define M_CORE_PRINTF_ARG(x) _Generic( ((void)0,(x))                          \
          , m_string_ptr: m_string_get_cstr( M_AS_TYPE(m_string_ptr, x))      \
          , m_string_srcptr: m_string_get_cstr(M_AS_TYPE(m_string_srcptr,x))  \
          , default: x )

/* Internal Macro: Provide GET_STR method to enum type */
#undef M_GET_STR_METHOD_FOR_ENUM_TYPE
#define M_GET_STR_METHOD_FOR_ENUM_TYPE GET_STR(M_ENUM_GET_STR)



/***********************************************************************/
/*                                                                     */
/*                BOUNDED STRING, aka char[N+1]                        */
/*                                                                     */
/***********************************************************************/

/* Define a bounded (fixed) string of exactly 'max_size' characters
 * (excluding the final nul char).
 */
#define M_BOUNDED_STRING_DEF(name, max_size)                                  \
  M_BEGIN_PROTECTED_CODE                                                      \
  M_BOUNDED_STR1NG_DEF_P2(name, max_size, M_F(name, _t) )                     \
  M_END_PROTECTED_CODE

/* Define the OPLIST of a BOUNDED_STRING */
#define M_BOUNDED_STRING_OPLIST(name)                                         \
  (INIT(M_F(name,_init)),                                                     \
   INIT_SET(M_F(name,_init_set)),                                             \
   SET(M_F(name,_set)),                                                       \
   CLEAR(M_F(name,_clear)),                                                   \
   INIT_MOVE(M_COPY_A1_DEFAULT),                                              \
   NAME(name),                                                                \
   INIT_WITH( API_1(M_BOUNDED_STR1NG_INIT_WITH)),                             \
   HASH(M_F(name,_hash)),                                                     \
   EQUAL(M_F(name,_equal_p)),                                                 \
   CMP(M_F(name,_cmp)),                                                       \
   TYPE(M_F(name,_ct)), GENTYPE(struct M_F(name,_s)*),                        \
   OOR_EQUAL(M_F(name,_oor_equal_p)),                                         \
   OOR_SET(M_F(name, _oor_set)),                                              \
   PARSE_STR(M_F(name,_parse_str)),                                           \
   GET_STR(M_F(name,_get_str)),                                               \
   OUT_STR(M_F(name,_out_str)),                                               \
   IN_STR(M_F(name,_in_str)),                                                 \
   OUT_SERIAL(M_F(name,_out_serial)),                                         \
   IN_SERIAL(M_F(name,_in_serial)),                                           \
   )

/************************** INTERNAL ***********************************/

/* Contract of a bounded string.
 * A bounded string last characters is always zero. */
#define M_BOUNDED_STR1NG_CONTRACT(var, max_size) do {                         \
    M_ASSERT(var != NULL);                                                    \
    M_ASSERT(var->s[max_size] == 0);                                          \
  } while (0)

/* Expand the functions for a bounded string */
#define M_BOUNDED_STR1NG_DEF_P2(name, max_size, bounded_t)                    \
                                                                              \
  /* Define of an array with one more to store the final nul char */          \
  typedef char M_F(name, _array_t)[max_size+1];                               \
                                                                              \
  typedef struct M_F(name, _s) {                                              \
    char s[max_size+1];                                                       \
  } bounded_t[1];                                                             \
                                                                              \
  /* Internal types for oplist */                                             \
  typedef bounded_t M_F(name, _ct);                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _init)(bounded_t s)                                               \
  {                                                                           \
    M_ASSERT(s != NULL);                                                      \
    M_STATIC_ASSERT(max_size >= 1, M_LIB_INTERNAL, "M*LIB: max_size parameter shall be greater than 0."); \
    s->s[0] = 0;                                                              \
    s->s[max_size] = 0;                                                       \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _clear)(bounded_t s)                                              \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    /* Make the object illegal to be able to detect use after free */         \
    s->s[max_size] = 0x1F;                                                    \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _reset)(bounded_t s)                                              \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    s->s[0] = 0;                                                              \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
  }                                                                           \
                                                                              \
  M_INLINE size_t                                                             \
  M_F(name, _size)(const bounded_t s)                                         \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    return strlen(s->s);                                                      \
  }                                                                           \
                                                                              \
  M_INLINE size_t                                                             \
  M_F(name, _capacity)(const bounded_t s)                                     \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    (void)s; /* unused */                                                     \
    return max_size+1;                                                        \
  }                                                                           \
                                                                              \
  M_INLINE char                                                               \
  M_F(name, _get_char)(const bounded_t s, size_t index)                       \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_ASSERT_INDEX(index, max_size);                                          \
    return s->s[index];                                                       \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _empty_p)(const bounded_t s)                                      \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    return s->s[0] == 0;                                                      \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _set_cstr)(bounded_t s, const char str[])                         \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    m_core_strncpy(s->s, max_size+1, str, max_size);                          \
    s->s[max_size] = 0;                                                       \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _set_cstrn)(bounded_t s, const char str[], size_t n)              \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_ASSERT(str != NULL);                                                    \
    size_t len = M_MIN(max_size, n);                                          \
    m_core_strncpy(s->s, max_size+1, str, len);                               \
    s->s[len] = 0;                                                            \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
  }                                                                           \
                                                                              \
  M_INLINE const char *                                                       \
  M_F(name, _get_cstr)(const bounded_t s)                                     \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    return s->s;                                                              \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _set)(bounded_t s, const bounded_t str)                           \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_BOUNDED_STR1NG_CONTRACT(str, max_size);                                 \
    M_F(name, _set_cstr)(s, str->s);                                          \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _set_n)(bounded_t s, const bounded_t str,                         \
                    size_t offset, size_t length)                             \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_BOUNDED_STR1NG_CONTRACT(str, max_size);                                 \
    M_ASSERT_INDEX (offset, max_size+1);                                      \
    M_F(name, _set_cstrn)(s, str->s+offset, length);                          \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _init_set)(bounded_t s, const bounded_t str)                      \
  {                                                                           \
    M_F(name,_init)(s);                                                       \
    M_F(name,_set)(s, str);                                                   \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _init_set_cstr)(bounded_t s, const char str[])                    \
  {                                                                           \
    M_F(name,_init)(s);                                                       \
    M_F(name,_set_cstr)(s, str);                                              \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _cat_cstr)(bounded_t s, const char str[])                         \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_ASSERT (str != NULL);                                                   \
    M_ASSERT_INDEX (strlen(s->s), max_size+1);                                \
    m_core_strncat(s->s, max_size+1, str, max_size-strlen(s->s));             \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _cat)(bounded_t s, const bounded_t  str)                          \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(str, max_size);                                 \
    M_F(name, _cat_cstr)(s, str->s);                                          \
  }                                                                           \
                                                                              \
  M_INLINE int                                                                \
  M_F(name, _cmp_cstr)(const bounded_t s, const char str[])                   \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_ASSERT(str != NULL);                                                    \
    return strcmp(s->s, str);                                                 \
  }                                                                           \
                                                                              \
  M_INLINE int                                                                \
  M_F(name, _cmp)(const bounded_t s, const bounded_t str)                     \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_BOUNDED_STR1NG_CONTRACT(str, max_size);                                 \
    return strcmp(s->s, str->s);                                              \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _equal_cstr_p)(const bounded_t s, const char str[])               \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_ASSERT(str != NULL);                                                    \
    return strcmp(s->s, str) == 0;                                            \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _equal_p)(const bounded_t s, const bounded_t str)                 \
  {                                                                           \
    /* _equal_p may be called in context OOR. So contract cannot be verified */ \
    return (s->s[max_size] == str->s[max_size]) & (strcmp(s->s, str->s) == 0); \
  }                                                                           \
                                                                              \
  M_INLINE int                                                                \
  M_F(name, _printf)(bounded_t s, const char format[], ...)                   \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_ASSERT(format != NULL);                                                 \
    va_list args;                                                             \
    int ret;                                                                  \
    va_start (args, format);                                                  \
    ret = vsnprintf (s->s, max_size+1, format, args);                         \
    va_end (args);                                                            \
    return ret;                                                               \
  }                                                                           \
                                                                              \
  M_INLINE int                                                                \
  M_F(name, _cat_printf)(bounded_t s, const char format[], ...)               \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_ASSERT(format != NULL);                                                 \
    va_list args;                                                             \
    int ret;                                                                  \
    va_start (args, format);                                                  \
    size_t length = strlen(s->s);                                             \
    M_ASSERT(length <= max_size);                                             \
    ret = vsnprintf (&s->s[length], max_size+1-length, format, args);         \
    va_end (args);                                                            \
    return ret;                                                               \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _fgets)(bounded_t s, FILE *f, m_string_fgets_t arg)               \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_ASSERT(f != NULL);                                                      \
    M_ASSERT(arg != M_STRING_READ_FILE);                                      \
    char *ret = fgets(s->s, max_size+1, f);                                   \
    s->s[max_size] = 0;                                                       \
    if (ret != NULL && arg == M_STRING_READ_PURE_LINE) {                      \
      size_t length = strlen(s->s);                                           \
      if (length > 0 && s->s[length-1] == '\n')                               \
        s->s[length-1] = 0;                                                   \
    }                                                                         \
    return ret != NULL;                                                       \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _fputs)(FILE *f, const bounded_t s)                               \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_ASSERT(f != NULL);                                                      \
    return fputs(s->s, f) >= 0;                                               \
  }                                                                           \
                                                                              \
  M_INLINE size_t                                                             \
  M_F(name, _hash)(const bounded_t s)                                         \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    /* Cannot use m_core_hash: alignment guarantee is not sufficient */       \
    return m_core_cstr_hash(s->s);                                            \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _oor_equal_p)(const bounded_t s, unsigned char n)                 \
  {                                                                           \
    /* s may be invalid contract */                                           \
    M_ASSERT (s != NULL);                                                     \
    M_ASSERT ( (n == 0) || (n == 1));                                         \
    return s->s[max_size] == (char) (n+1);                                    \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _oor_set)(bounded_t s, unsigned char n)                           \
  {                                                                           \
    /* s may be invalid contract */                                           \
    M_ASSERT (s != NULL);                                                     \
    M_ASSERT ( (n == 0) || (n == 1));                                         \
    s->s[max_size] = (char) (n+1);                                            \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _get_str)(m_string_t v, const bounded_t s, bool append)           \
  {                                                                           \
    M_STR1NG_CONTRACT(v);                                                     \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    /* Build dummy string to reuse m_string_get_str */                        \
    uintptr_t ptr = (uintptr_t) &s->s[0];                                     \
    m_string_t v2;                                                            \
    v2->u.heap.size = (m_str1ng_size_t) strlen(s->s);                         \
    v2->u.heap.alloc[sizeof(m_str1ng_size_t)-2] = 1;                          \
    v2->u.heap.alloc[sizeof(m_str1ng_size_t)-1] = 31;                         \
    v2->u.heap.ptr = (char*)ptr;                                              \
    m_string_get_str(v, v2, append);                                          \
  }                                                                           \
                                                                              \
  M_INLINE void                                                               \
  M_F(name, _out_str)(FILE *f, const bounded_t s)                             \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(s, max_size);                                   \
    M_ASSERT(f != NULL);                                                      \
    /* Build dummy string to reuse m_string_get_str */                        \
    uintptr_t ptr = (uintptr_t) &s->s[0];                                     \
    m_string_t v2;                                                            \
    v2->u.heap.size = (m_str1ng_size_t) strlen(s->s);                         \
    v2->u.heap.alloc[sizeof(m_str1ng_size_t)-2] = 1;                          \
    v2->u.heap.alloc[sizeof(m_str1ng_size_t)-1] = 31;                         \
    v2->u.heap.ptr = (char*)ptr;                                              \
    m_string_out_str(f, v2);                                                  \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _in_str)(bounded_t v, FILE *f)                                    \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(v, max_size);                                   \
    M_ASSERT(f != NULL);                                                      \
    m_string_t v2;                                                            \
    m_string_init(v2);                                                        \
    bool ret = m_string_in_str(v2, f);                                        \
    m_core_strncpy(v->s, max_size+1, m_string_get_cstr(v2), max_size);        \
    m_string_clear(v2);                                                       \
    return ret;                                                               \
  }                                                                           \
                                                                              \
  M_INLINE bool                                                               \
  M_F(name, _parse_str)(bounded_t v, const char str[], const char **endptr)   \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(v, max_size);                                   \
    M_ASSERT(str != NULL);                                                    \
    m_string_t v2;                                                            \
    m_string_init(v2);                                                        \
    bool ret = m_string_parse_str(v2, str, endptr);                           \
    m_core_strncpy(v->s, max_size+1, m_string_get_cstr(v2), max_size);        \
    m_string_clear(v2);                                                       \
    return ret;                                                               \
  }                                                                           \
                                                                              \
  M_P(m_serial_return_code_t, name, _out_serial, m_serial_write_t serial, const bounded_t v) \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(v, max_size);                                   \
    M_ASSERT (serial != NULL && serial->m_interface != NULL);                 \
    return serial->m_interface->write_string M_R(serial, v->s, strlen(v->s) ); \
  }                                                                           \
                                                                              \
  M_P(m_serial_return_code_t, name, _in_serial, bounded_t v, m_serial_read_t serial) \
  {                                                                           \
    M_BOUNDED_STR1NG_CONTRACT(v, max_size);                                   \
    M_ASSERT (serial != NULL && serial->m_interface != NULL);                 \
    m_string_t tmp;                                                           \
    /* TODO: Not optimum */                                                   \
    m_string_init(tmp);                                                       \
    m_serial_return_code_t r = serial->m_interface->read_string M_R(serial, tmp); \
    m_core_strncpy(v->s, max_size+1, m_string_get_cstr(tmp), max_size);       \
    m_string_clear M_R(tmp);                                                  \
    M_BOUNDED_STR1NG_CONTRACT(v, max_size);                                   \
    return r;                                                                 \
  }


   
/* Init a constant bounded string.
   Try to do a clean cast */
/* Use of Compound Literals to init a constant string.
   See above */
#ifndef __cplusplus
#define M_BOUNDED_STRING_CTE(name, string)                                    \
  ((const struct M_F(name, _s) *)((M_F(name, _array_t)){string}))
#else
namespace m_lib {
  template <unsigned int N>
    struct m_bounded_string {
      char s[N];
      inline m_bounded_string(const char lstr[])
      {
        memset(this->s, 0, N);
        m_core_strncpy(this->s, N, lstr, N-1);
      }
    };
}
#define M_BOUNDED_STRING_CTE(name, string)                                    \
  ((const struct M_F(name, _s) *)(m_lib::m_bounded_string<sizeof (M_F(name, _t))>(string).s))
#endif


/* Initialize a bounded string with the given list of arguments.
   Check if it is a formatted input or not by counting the number of arguments.
   If there is only one argument, it can only be a set to C string.
   It is much faster in this case to call m_string_init_set_cstr.
*/
#define M_BOUNDED_STR1NG_INIT_WITH(oplist, v, ...)                            \
  M_IF_NARGS_EQ1(__VA_ARGS__)(M_C(M_GET_NAME oplist, _init_set_cstr)(v, __VA_ARGS__), M_BOUNDED_STR1NG_INIT_PRINTF(oplist, v, __VA_ARGS__))

#define M_BOUNDED_STR1NG_INIT_PRINTF(oplist, v, ...)                          \
  (M_GET_INIT oplist (v), M_C(M_GET_NAME oplist, _printf)(v, __VA_ARGS__))



/********************************************************************************/
/*                                                                              */
/* Define the small name (i.e. without the prefix) of the API provided by this  */
/* header if it is needed                                                       */
/*                                                                              */
/********************************************************************************/
#if M_USE_SMALL_NAME

#define string_t    m_string_t
#define string_s    m_string_s
#define STRING_FAILURE M_STRING_FAILURE
#define string_ptr m_string_ptr
#define string_srcptr m_string_srcptr
#define STRING_READ_LINE M_STRING_READ_LINE
#define STRING_READ_PURE_LINE M_STRING_READ_PURE_LINE
#define STRING_READ_FILE M_STRING_READ_FILE
#define string_fgets_t m_string_fgets_t
#define string_unicode_t m_string_unicode_t
#define STRING_UNICODE_ERROR M_STRING_UNICODE_ERROR
#define string_it_t m_string_it_t

#define string_size m_string_size
#define string_capacity m_string_capacity
#define string_get_cstr m_string_get_cstr
#define string_init m_string_init
#define string_clear m_string_clear
#define string_clear_get_str m_string_clear_get_cstr
#define string_reset m_string_reset
#define string_get_char m_string_get_char
#define string_set_char m_string_set_char
#define string_empty_p m_string_empty_p
#define string_reserve m_string_reserve
#define string_set_str m_string_set_cstr
#define string_set_strn m_string_set_cstrn
#define string_set m_string_set
#define string_set_n m_string_set_n
#define string_init_set m_string_init_set
#define string_init_set_str m_string_init_set_cstr
#define string_init_move m_string_init_move
#define string_swap m_string_swap
#define string_move m_string_move
#define string_push_back m_string_push_back
#define string_cat_str m_string_cat_cstr
#define string_cat m_string_cat
#define string_cmp_str m_string_cmp_cstr
#define string_cmp m_string_cmp
#define string_equal_str_p m_string_equal_cstr_p
#define string_equal_p m_string_equal_p
#define string_cmpi_str m_string_cmpi_cstr
#define string_cmpi m_string_cmpi
#define string_search_char m_string_search_char
#define string_search_rchar m_string_search_rchar
#define string_search_str m_string_search_cstr
#define string_search m_string_search
#define string_search_pbrk m_string_search_pbrk
#define string_strcoll_str m_string_strcoll_cstr
#define string_strcoll m_string_strcoll
#define string_spn m_string_spn
#define string_cspn m_string_cspn
#define string_left m_string_left
#define string_right m_string_right
#define string_mid m_string_mid
#define string_replace_str m_string_replace_cstr
#define string_replace m_string_replace
#define string_replace_at m_string_replace_at
#define string_replace_all_str m_string_replace_all_cstr
#define string_replace_all m_string_replace_all
#define string_vprintf m_string_vprintf
#define string_printf m_string_printf
#define string_cat_vprintf m_string_cat_vprintf
#define string_cat_printf m_string_cat_printf
#define string_fgets m_string_fgets
#define string_fget_word m_string_fget_word
#define string_fputs m_string_fputs
#define string_start_with_str_p m_string_start_with_str_p
#define string_start_with_string_p m_string_start_with_string_p
#define string_end_with_str_p m_string_end_with_str_p
#define string_end_with_string_p m_string_end_with_string_p
#define string_hash m_string_hash
#define string_strim m_string_strim
#define string_oor_equal_p m_string_oor_equal_p
#define string_oor_set m_string_oor_set
#define string_get_str m_string_get_str
#define string_out_str m_string_out_str
#define string_in_str m_string_in_str
#define string_parse_str m_string_parse_str
#define string_out_serial m_string_out_serial
#define string_in_serial m_string_in_serial
#define string_it m_string_it
#define string_it_end m_string_it_end
#define string_it_set m_string_it_set
#define string_end_p m_string_end_p
#define string_it_equal_p m_string_it_equal_p
#define string_next m_string_next
#define string_get_cref m_string_get_cref
#define string_cref m_string_cref
#define string_push_u m_string_push_u
#define string_pop_u m_string_pop_u
#define string_length_u m_string_length_u
#define string_utf8_p m_string_utf8_p
#define string_set_ui m_string_set_ui
#define string_set_si m_string_set_si
#define string_it_pos m_string_it_pos
#define string_it_get_pos m_string_it_get_pos
#define string_previous m_string_previous
#define string_it_set_ref m_string_it_set_ref

#define STRING_CTE M_STRING_CTE
#define STRING_OPLIST M_STRING_OPLIST
#define M_OPL_string_t M_OPL_m_string_t
#define string_sets m_string_sets
#define string_cats m_string_cats
#define string_init_printf m_string_init_printf
#define string_init_vprintf m_string_init_vprintf

#define BOUNDED_STRING_DEF M_BOUNDED_STRING_DEF
#define BOUNDED_STRING_OPLIST M_BOUNDED_STRING_OPLIST
#define BOUNDED_STRING_CTE M_BOUNDED_STRING_CTE

#endif

M_END_PROTECTED_CODE

#endif
