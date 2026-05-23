/*
 * M*LIB - Byte String module
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
#ifndef MSTARLIB_BSTRING_H
#define MSTARLIB_BSTRING_H

#include "m-core.h"

M_BEGIN_PROTECTED_CODE

/* Byte String Type definition*/
typedef struct m_bstring_s {
 size_t size;   // Number of effective BYTES
 size_t alloc;  // Size of the allocated buffer
 size_t offset; // Offset where the effective BYTES are stored in the allocated buffer
 uint8_t  *ptr; // Pointer to the allocated buffer
 } m_bstring_t[1];

// Contract of a bstring
#define M_BSTRING_CONTRACT(o) do {                                            \
    M_ASSERT( (o)->size + (o)->offset >= (o)->size );                         \
    M_ASSERT( (o)->alloc  >= (o)->size + (o)->offset );                       \
    M_ASSERT( (o)->ptr != NULL || (o)->alloc == 0);                           \
} while (0)

M_INLINE size_t
m_bstring_size(const m_bstring_t s)
{
    M_BSTRING_CONTRACT(s);
    return s->size;
}

M_INLINE size_t
m_bstring_capacity(const m_bstring_t s)
{
    M_BSTRING_CONTRACT(s);
    return s->alloc;
}

M_INLINE uint8_t *
m_bstr1ng_cstr(const m_bstring_t v)
{
    return &v->ptr[v->offset];
}

M_INLINE void
m_bstring_init(m_bstring_t s)
{
    s->size   = 0;
    s->alloc  = 0;
    s->offset = 0;
    s->ptr    = NULL;
    M_BSTRING_CONTRACT(s);
}

M_P(void, m_bstring, _clear, m_bstring_t s)
{
    M_BSTRING_CONTRACT(s);
    M_MEMORY_FREE(m_context, uint8_t, s->ptr, s->alloc);
    s->offset = (size_t) -1;
}

M_INLINE void
m_bstring_reset(m_bstring_t s)
{
    M_BSTRING_CONTRACT(s);
    s->size   = 0;
    s->offset = 0;
    M_BSTRING_CONTRACT(s);
}

M_INLINE void
m_bstring_swap(m_bstring_t s1, m_bstring_t s2)
{
    M_BSTRING_CONTRACT(s1);
    M_BSTRING_CONTRACT(s2);
    m_bstring_t tmp;
    memcpy(tmp, s1, sizeof (m_bstring_t));
    memcpy(s1, s2, sizeof (m_bstring_t));
    memcpy(s2, tmp, sizeof (m_bstring_t));
    M_BSTRING_CONTRACT(s1);
    M_BSTRING_CONTRACT(s2);
}

/* Return the selected byte-character of the string */
M_INLINE uint8_t
m_bstring_get_byte(const m_bstring_t v, size_t index)
{
    M_BSTRING_CONTRACT (v);
    M_ASSERT_INDEX(index, v->size);
    return m_bstr1ng_cstr(v)[index];
}

/* Set the selected byte-character of the string */
M_INLINE void
m_bstring_set_byte(m_bstring_t v, size_t index, const uint8_t c)
{
    M_BSTRING_CONTRACT (v);
    M_ASSERT_INDEX(index, v->size);
    m_bstr1ng_cstr(v)[index] = c;
}

/* Test if the string is empty or not */
M_INLINE bool
m_bstring_empty_p(const m_bstring_t v)
{
    M_BSTRING_CONTRACT (v);
    return v->size == 0;
}

/* Internal method to fit the Byte string to the given size
   Ensures that the Byte String capacity is greater than size_alloc+offset
   The returned Byte String 'v' no longer has a **correct** size field.
   If exact_alloc is true, it will request exactly the request size for the buffer
   on reallocation otherwise it will augment the requested size to amortize the future
   allocations.
   Return a pointer to the writable string.
*/
M_P(uint8_t *, m_bstr1ng, _fit2size, m_bstring_t v, size_t size_alloc, bool exact_alloc)
{
  // Very unlikely overflow case
  if (M_UNLIKELY_NOMEM (v->offset > v->offset + size_alloc)) {
    goto allocation_error;
  }
  M_ASSERT_INDEX (v->offset, v->offset + size_alloc);
  // Note: this function may be called in context where the contract is not fulfilled.
  if (M_UNLIKELY (v->offset + size_alloc > v->alloc)) {
    // Insufficient current allocation to store the new string

    // Check if there is a too big offset that can be reduced
    if (v->offset > v->size / 8 && v->offset > 16) {
        // Move back the effective bytes to the beginning of the buffer
        memmove(&v->ptr[0], &v->ptr[v->offset], v->size);
        v->offset = 0;
        // If it is now sufficient, we can return.
        if (size_alloc <= v->alloc)
            return v->ptr;
    }

    // Alloc a new array. Compute the requested allocation size
    size_t alloc = (v->offset + size_alloc + (exact_alloc ? 0 : size_alloc / 2));
    alloc = M_MAX(alloc, 16);
    if (M_UNLIKELY_NOMEM (alloc < size_alloc)) {
        goto allocation_error;
    }
    uint8_t *ptr = M_MEMORY_REALLOC (m_context, uint8_t, v->ptr, v->alloc, alloc);
    if (M_UNLIKELY_NOMEM (ptr == NULL)) {
        goto allocation_error;
    }
    // The string cannot be fully embedded anymore.
    v->ptr = ptr;
    v->alloc = alloc;
    // Size is not set as the function is called in context
    // where the caller already know the final size of the string,
    // so it will update it.
    return &v->ptr[v->offset];
  }
  return &v->ptr[v->offset];
  allocation_error:
    M_MEMORY_FULL(uint8_t, size_alloc);
}

/* Push the byte 'c' in the bstring 'v' */
M_P(void, m_bstring, _push_back, m_bstring_t v, uint8_t c)
{
    M_BSTRING_CONTRACT (v);
    const size_t size = v->size;
    // To overflow size+1, we need to have exhausted all memory
    // which is not possible.
    uint8_t *ptr = m_bstr1ng_fit2size M_R(v, size+1, false);
    ptr[size] = c;
    v->size = size + 1;
    M_BSTRING_CONTRACT (v);
}

/* Push the array of bytes from 'buffer' in the bstring 'v' */
M_P(void, m_bstring, _push_back_bytes, m_bstring_t v, size_t n, const void *buffer)
{
    M_BSTRING_CONTRACT (v);
    const size_t size = v->size;
    if (M_UNLIKELY (n == 0)) return;
    M_ASSERT_INDEX (size, size + n);
    // If size+n overflows, it means we are giving two objects which sizes
    // sum to be greater than the available memory. The memory should have exhausted before that.
    // The case is not possible, or the given size of the buffer is "faked".
    uint8_t *ptr = m_bstr1ng_fit2size M_R(v, size+n, false);
    memcpy(&ptr[size], buffer, n);
    v->size = size + n;
    M_BSTRING_CONTRACT (v);
}

M_P(void, m_bstring, _splice, m_bstring_t dst, m_bstring_t src)
{
    M_BSTRING_CONTRACT (dst);
    M_BSTRING_CONTRACT (src);
    M_ASSERT(src != dst);
    m_bstring_push_back_bytes M_R(dst, m_bstring_size(src), m_bstr1ng_cstr(src));
    m_bstring_reset(src);
}

M_P(void, m_bstring, _init_set, m_bstring_t v, const m_bstring_t org)
{
  m_bstring_init(v);
  m_bstring_push_back_bytes M_R(v, m_bstring_size(org), m_bstr1ng_cstr(org));
}

M_P(void, m_bstring, _set, m_bstring_t v, const m_bstring_t org)
{
  m_bstring_reset(v);
  m_bstring_push_back_bytes M_R(v, m_bstring_size(org), m_bstr1ng_cstr(org));
}

M_INLINE void
m_bstring_init_move(m_bstring_t v, m_bstring_t org)
{
    M_BSTRING_CONTRACT (org);
    memcpy(v, org, sizeof (m_bstring_t));
    // Make org an invalid representation
    org->offset = (size_t) -1;
}

M_P(void, m_bstring, _move, m_bstring_t v, m_bstring_t org)
{
    m_bstring_clear M_R(v);
    m_bstring_init_move(v, org);
}

M_INLINE size_t
m_bstring_hash(const m_bstring_t v)
{
    M_BSTRING_CONTRACT (v);
    return m_core_hash(m_bstr1ng_cstr(v), m_bstring_size(v));
}

M_INLINE int
m_bstring_cmp(const m_bstring_t v1, const m_bstring_t v2)
{
    M_BSTRING_CONTRACT (v1);
    M_BSTRING_CONTRACT (v2);
    const size_t s1 = m_bstring_size(v1);
    const size_t s2 = m_bstring_size(v2);
    int ret = memcmp(m_bstr1ng_cstr(v1), m_bstr1ng_cstr(v2), M_MIN(s1, s2));
    if (ret)
        return ret;
    if (s1 != s2)
        return s1 < s2 ? -1 : 1;
    return 0;
}

M_INLINE int
m_bstring_cmp_bytes(const m_bstring_t v1, size_t s2, const void *p2)
{
    M_BSTRING_CONTRACT (v1);
    const size_t s1 = m_bstring_size(v1);
    int ret = memcmp(m_bstr1ng_cstr(v1), p2, M_MIN(s1, s2));
    if (ret)
        return ret;
    if (s1 != s2)
        return s1 < s2 ? -1 : 1;
    return 0;
}

M_INLINE bool
m_bstring_equal_p(const m_bstring_t v1, const m_bstring_t v2)
{
    M_BSTRING_CONTRACT (v1);
    M_BSTRING_CONTRACT (v2);
    const size_t s1 = m_bstring_size(v1);
    const size_t s2 = m_bstring_size(v2);
    if (s1 != s2)
        return false;
    return memcmp(m_bstr1ng_cstr(v1), m_bstr1ng_cstr(v2), s1) == 0;
}

M_INLINE bool
m_bstring_equal_bytes_p(const m_bstring_t v1, size_t s2, const void *buffer)
{
    M_BSTRING_CONTRACT (v1);
    const size_t s1 = m_bstring_size(v1);
    if (s1 != s2)
        return false;
    return memcmp(m_bstr1ng_cstr(v1), buffer, s1) == 0;
}

M_INLINE uint8_t
m_bstring_pop_back(m_bstring_t v)
{
    M_BSTRING_CONTRACT (v);
    M_ASSERT_INDEX(0, v->size);
    uint8_t c = m_bstr1ng_cstr(v)[v->size-1];
    v->size --;
    M_BSTRING_CONTRACT (v);
    return c;
}

M_INLINE void
m_bstring_pop_back_bytes(size_t n, void *buffer, m_bstring_t v)
{
    M_BSTRING_CONTRACT (v);
    M_ASSERT_INDEX(n-1, v->size);
    memcpy(buffer, &m_bstr1ng_cstr(v)[v->size-n], n);
    v->size -= n;
    M_BSTRING_CONTRACT (v);
}

M_INLINE uint8_t
m_bstring_pop_front(m_bstring_t v)
{
    M_BSTRING_CONTRACT (v);
    M_ASSERT_INDEX(0, v->size);
    uint8_t c = m_bstr1ng_cstr(v)[0];
    v->size --;
    v->offset++;
    M_BSTRING_CONTRACT (v);
    return c;
}

M_INLINE void
m_bstring_pop_front_bytes(size_t n, void *buffer, m_bstring_t v)
{
    M_BSTRING_CONTRACT (v);
    M_ASSERT_INDEX(n-1, v->size);
    memcpy(buffer, &m_bstr1ng_cstr(v)[0], n);
    v->size -= n;
    v->offset += n;
    M_BSTRING_CONTRACT (v);
}

M_P(void, m_bstring, _push_bytes_at, m_bstring_t v, size_t pos, size_t n, const void *buffer)
{
    M_BSTRING_CONTRACT (v);
    const size_t size = v->size;
    M_ASSERT_INDEX (pos, size+1); //pos == size ==> equivalent to _push_back_bytes
    M_ASSERT_INDEX (size, size + n);
    // If size+n overflows, it means we are giving two objects which sizes
    // sum to be greater than the available memory. The memory should have exhausted before that.
    // The case is not possible.
    uint8_t *ptr = m_bstr1ng_fit2size M_R(v, size+n, false);
    memmove(&ptr[pos+n], &ptr[pos], size-pos);
    memcpy(&ptr[pos], buffer, n);
    v->size = size + n;
    M_BSTRING_CONTRACT (v);
}

M_INLINE void
m_bstring_pop_bytes_at(size_t n, void *buffer, m_bstring_t v, size_t pos)
{
    M_BSTRING_CONTRACT (v);
    const size_t size = v->size;
    M_ASSERT_INDEX (pos, size);
    M_ASSERT_INDEX(n-1, size-pos);
    uint8_t *ptr = m_bstr1ng_cstr(v);
    memcpy(buffer, &ptr[pos], n);
    memmove(&ptr[pos], &ptr[pos+n], size-pos-n);
    v->size -= n;
    M_BSTRING_CONTRACT (v);
}

M_P(void, m_bstring, _resize, m_bstring_t v, size_t n)
{
    M_BSTRING_CONTRACT (v);
    const size_t size = v->size;
    if (n > size) {
        uint8_t *ptr = m_bstr1ng_fit2size M_R(v, n, true);
        memset(&ptr[size], 0, n-size);
    }
    v->size = n;
    M_BSTRING_CONTRACT (v);
}

M_P(void, m_bstring, _reserve, m_bstring_t v, size_t n)
{
    M_BSTRING_CONTRACT (v);
    const size_t size = v->size;
    if ( (n != 0 || v->size != 0) && n < v->offset + size) {
        n = v->offset + size;
    }
    if (n == 0) {
        M_MEMORY_FREE(m_context, uint8_t, v->ptr, v->alloc);
        v->offset = 0;
        v->size = 0;
        v->alloc = 0;
        v->ptr = NULL;
    } else {
        uint8_t *ptr = M_MEMORY_REALLOC (m_context, uint8_t, v->ptr, v->alloc, n);
        if (M_UNLIKELY_NOMEM (ptr == NULL)) {
            M_MEMORY_FULL(uint8_t, n);
        }
        v->ptr = ptr;
        v->alloc = n;
    }
    M_BSTRING_CONTRACT (v);
}

M_INLINE const uint8_t *
m_bstring_view(const m_bstring_t v, size_t offset, size_t size_requested)
{
    M_BSTRING_CONTRACT (v);
    M_ASSERT_INDEX (offset+size_requested, v->size+1);
    (void) size_requested;
    return &m_bstr1ng_cstr(v)[offset];
}

// Acquire / release direct write access: once acquired, user can directly wite at the returned pointer for the requested size
// All other operations on the bstring are forbidden during direct access.
// This allows passing the bstring buffer to be formatted by different API
M_INLINE uint8_t *
m_bstring_acquire_access(m_bstring_t v, size_t offset, size_t size_requested)
{
    M_BSTRING_CONTRACT (v);
    M_ASSERT_INDEX (offset+size_requested, v->size+1);
    (void) size_requested;
    // Break canonical representation (this will generate assertion if a normal API is called now)
    M_SWAP(size_t, v->size, v->alloc);
    return &m_bstr1ng_cstr(v)[offset];
}

M_INLINE void
m_bstring_release_access(m_bstring_t v)
{
    M_ASSERT( v->size  >= v->alloc + v->offset );
    // Restore canonical representation
    M_SWAP(size_t, v->size, v->alloc);
    M_BSTRING_CONTRACT (v);
}

// No Iterator planned for this container (No added value)

#if M_USE_STDIO

M_INLINE size_t
m_bstring_fwrite(FILE *f, const m_bstring_t v)
{
  M_BSTRING_CONTRACT (v);
  M_ASSERT (f != NULL);
  if (M_UNLIKELY(m_bstring_size(v) == 0)) {
    return 0;
  }
  M_ASSERT(m_bstr1ng_cstr(v) != NULL);
  return fwrite(m_bstr1ng_cstr(v), 1, m_bstring_size(v), f);
}

M_P(bool, m_bstring, _fread, m_bstring_t v, FILE *f, size_t num)
{
  M_BSTRING_CONTRACT (v);
  M_ASSERT (f != NULL);
  m_bstring_reset(v);
  //TODO: option to read all the file num=-1?
  if (M_UNLIKELY(num == 0)) {
    return true;
  }
  uint8_t *ptr = m_bstr1ng_fit2size M_R(v, num, true);
  size_t n = fread(ptr, 1, num, f);
  M_ASSERT (n <= num);
  v->size = n;
  return n == num;
}

#endif // Have stdio

// No formatted string input/output. Only serializer

/* Transform the Byte String 'v2'
   and output it in the given serializer
   See serialization for return code.
*/
M_P(m_serial_return_code_t, m_bstring, _out_serial, m_serial_write_t serial, const m_bstring_t v)
{
  M_ASSERT (serial != NULL && serial->m_interface != NULL);
  M_BSTRING_CONTRACT (v);
  m_serial_local_t local;
  m_serial_return_code_t ret;
  size_t size = m_bstring_size(v);
  uint8_t *ptr = m_bstr1ng_cstr(v);
  ret = serial->m_interface->write_array_start M_R(local, serial, size);
  if (size > 0) {
    ret |= serial->m_interface->write_integer M_R(serial, ptr[0], 1);
    for(size_t i = 1; i < size; i++) {
        ret |= serial->m_interface->write_array_next M_R(local, serial);
        ret |= serial->m_interface->write_integer M_R(serial, ptr[i], 1);
    }
  }
  ret |= serial->m_interface->write_array_end M_R(local, serial);
  return ret;
}

/* Read from the serializer
   and set the converted value in the byte string 'v'.
   See serialization for return code.
*/
M_P(m_serial_return_code_t, m_bstring, _in_serial, m_bstring_t v, m_serial_read_t f)
{
  M_ASSERT (f != NULL && f->m_interface != NULL);
  M_BSTRING_CONTRACT (v);
  m_serial_return_code_t ret;
  m_serial_local_t local;
  size_t estimated_size = 0, real_size = 0;
  m_bstring_reset(v);
  ret = f->m_interface->read_array_start(local, f, &estimated_size);
  if (M_UNLIKELY (ret != M_SERIAL_OK_CONTINUE)) {
    return ret;
  }
  uint8_t *ptr = m_bstr1ng_fit2size M_R(v, estimated_size, true);
  do {
    long long val;
    ret = f->m_interface->read_integer(f, &val, 1);
    if (ret != M_SERIAL_OK_DONE) { break; }
    ptr[real_size++] = (uint8_t) val;
    ret = f->m_interface->read_array_next(local, f);
  } while (ret == M_SERIAL_OK_CONTINUE);
  v->size = real_size;
  M_BSTRING_CONTRACT (v);
  return ret;
}

/* Define the OPLIST of a BYTE STRING */
#ifndef M_USE_CONTEXT
#define M_BSTRING_OPLIST                                                      \
  (INIT(m_bstring_init),INIT_SET(m_bstring_init_set), SET(m_bstring_set),     \
   INIT_MOVE(m_bstring_init_move), MOVE(m_bstring_move),                      \
   SWAP(m_bstring_swap), RESET(m_bstring_reset), EMPTY_P(m_bstring_empty_p),  \
   SUBTYPE(uint8_t), PUSH(m_bstring_push),                                    \
   CLEAR(m_bstring_clear), HASH(m_bstring_hash), EQUAL(m_bstring_equal_p),    \
   CMP(m_bstring_cmp), TYPE(m_bstring_t), GENTYPE(struct m_bstring_s*),       \
   OUT_SERIAL(m_bstring_out_serial), IN_SERIAL(m_bstring_in_serial),          \
   )
#else
#define M_BSTRING_OPLIST                                                      \
  (INIT(m_bstring_init),INIT_SET(API_0P(m_bstring_init_set)), SET(API_0P(m_bstring_set)), \
   INIT_MOVE(m_bstring_init_move), MOVE(API_0P(m_bstring_move)),              \
   SWAP(m_bstring_swap), RESET(m_bstring_reset), EMPTY_P(m_bstring_empty_p),  \
   SUBTYPE(uint8_t), PUSH(API_0P(m_bstring_push)),                            \
   CLEAR(API_0P(m_bstring_clear)), HASH(m_bstring_hash), EQUAL(m_bstring_equal_p), \
   CMP(m_bstring_cmp), TYPE(m_bstring_t), GENTYPE(struct m_bstring_s*),       \
   OUT_SERIAL(API_0P(m_bstring_out_serial)), IN_SERIAL(API_0P(m_bstring_in_serial=) \
   )
#endif

/* Register the OPLIST as a global one */
#define M_OPL_m_bstring_t() M_BSTRING_OPLIST


/********************************************************************************/
/*                                                                              */
/* Define the small name (i.e. without the prefix) of the API provided by this  */
/* header if it is needed                                                       */
/*                                                                              */
/********************************************************************************/
#if M_USE_SMALL_NAME

#define bstring_t m_bstring_t
#define bstring_size m_bstring_size
#define bstring_capacity m_bstring_capacity
#define bstring_init m_bstring_init
#define bstring_clear m_bstring_clear
#define bstring_reset m_bstring_reset
#define bstring_get_byte m_bstring_get_byte
#define bstring_set_byte m_bstring_set_byte
#define bstring_empty_p m_bstring_empty_p
#define bstring_push_back m_bstring_push_back
#define bstring_push_back_bytes m_bstring_push_back_bytes
#define bstring_init_set m_bstring_init_set
#define bstring_set m_bstring_set
#define bstring_init_move m_bstring_init_move
#define bstring_move m_bstring_move
#define bstring_cmp m_bstring_cmp
#define bstring_cmp_bytes m_bstring_cmp_bytes
#define bstring_equal_p m_bstring_equal_p
#define bstring_equal_bytes_p m_bstring_equal_bytes_p
#define bstring_pop_back m_bstring_pop_back
#define bstring_pop_back_bytes m_bstring_pop_back_bytes
#define bstring_pop_front m_bstring_pop_front
#define bstring_pop_front_bytes m_bstring_pop_front_bytes
#define bstring_push_bytes_at m_bstring_push_bytes_at
#define bstring_pop_bytes_at m_bstring_pop_bytes_at
#define bstring_resize m_bstring_resize
#define bstring_reserve m_bstring_reserve
#define bstring_view m_bstring_view
#define bstring_acquire_access m_bstring_acquire_access
#define bstring_release_access m_bstring_release_access
#define bstring_hash m_bstring_hash
#define bstring_fread m_bstring_fread
#define bstring_fwrite m_bstring_fwrite
#define bstring_out_serial m_bstring_out_serial
#define bstring_in_serial m_bstring_in_serial
#define bstring_swap m_bstring_swap
#define bstring_splice m_bstring_splice
#define BSTRING_OPLIST M_BSTRING_OPLIST
#define M_OPL_bstring_t() M_BSTRING_OPLIST

#endif

M_END_PROTECTED_CODE

#endif
