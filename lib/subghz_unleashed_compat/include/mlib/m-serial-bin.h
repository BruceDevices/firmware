/*
 * M*LIB - Serial BIN
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
#ifndef MSTARLIB_SERIAL_BIN_H
#define MSTARLIB_SERIAL_BIN_H

#include <stdint.h>

#include "m-core.h"
#include "m-string.h"

M_BEGIN_PROTECTED_CODE


/********************************************************************************/
/************************** FILE / WRITE / BIN    *******************************/
/********************************************************************************/

/* Internal service:
 * Write size_t in the stream in a compact form to reduce consumption
 * (and I/O bandwidth)
 */
M_INLINE bool
m_ser1al_bin_write_size(FILE *f, const size_t size)
{
  bool b;
  if (M_LIKELY(size < 253))
  {
    b = EOF != fputc((unsigned char) size, f);
  } else if (size < 1ULL << 16) {
    b = EOF != fputc(253, f);    // Save 16 bits encoding
    b &= EOF != fputc((unsigned char) (size >> 8), f);
    b &= EOF != fputc((unsigned char) size, f);
  } 
// For 32 bits systems, don't encode a 64 bits size_t
 #if SIZE_MAX < 1ULL<< 32
  else {
    b = EOF != fputc(254, f);    // Save 32 bits encoding
    b &= EOF != fputc((unsigned char) (size >> 24), f);
    b &= EOF != fputc((unsigned char) (size >> 16), f);
    b &= EOF != fputc((unsigned char) (size >> 8), f);
    b &= EOF != fputc((unsigned char) size, f);
  }
 #else 
  else if (size < 1ULL<< 32) {
    b = EOF != fputc(254, f);    // Save 32 bits encoding
    b &= EOF != fputc((unsigned char) (size >> 24), f);
    b &= EOF != fputc((unsigned char) (size >> 16), f);
    b &= EOF != fputc((unsigned char) (size >> 8), f);
    b &= EOF != fputc((unsigned char) size, f);
  } else {
    b = EOF != fputc(255, f);    // Save 64 bits encoding
    b &= EOF != fputc((unsigned char) (size >> 56), f);
    b &= EOF != fputc((unsigned char) (size >> 48), f);
    b &= EOF != fputc((unsigned char) (size >> 40), f);
    b &= EOF != fputc((unsigned char) (size >> 32), f);
    b &= EOF != fputc((unsigned char) (size >> 24), f);
    b &= EOF != fputc((unsigned char) (size >> 16), f);
    b &= EOF != fputc((unsigned char) (size >> 8), f);
    b &= EOF != fputc((unsigned char) size, f);
  }
#endif
  return b;
}

/* Internal service:
 * Read size_t in the stream from a compact form to reduce consumption
 * (and I/O bandwidth)
 */
M_INLINE bool
m_ser1al_bin_read_size(FILE *f, size_t *size)
{
  int c;
  c = fgetc(f);
  if (M_UNLIKELY(c == EOF)) return false;
  if (M_LIKELY(c < 253)) {
    *size = (size_t) c;
    return true;
  }
  size_t s = 0;
  int l = (c == 255) ? 8 : (c == 254) ? 4 : 2;
  for(int i = 0; i < l; i++) {
      c = fgetc(f);
      if (M_UNLIKELY(c == EOF)) return false;
      s = (s << 8) | (size_t) c;
  }
  *size = s;
  return true;
}

/* Write the boolean 'data' into the serial stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_bin, _write_boolean, m_serial_write_t serial, const bool data)
{
  M_UNUSED_CONTEXT();
  FILE *f = (FILE *)serial->data[0].p;
  size_t n = fwrite (M_ASSIGN_CAST(const void*, &data), sizeof (bool), 1, f);
  return n == 1 ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Write the integer 'data' of 'size_of_type' bytes into the serial stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_bin, _write_integer, m_serial_write_t serial,const long long data, const size_t size_of_type)
{
  M_UNUSED_CONTEXT();
  size_t n;
  
  FILE *f = (FILE *)serial->data[0].p;
  if (size_of_type == 1) {
    int8_t i8 = (int8_t) data;
    n = fwrite (M_ASSIGN_CAST(const void*, &i8), sizeof i8, 1, f);
  } else if (size_of_type == 2) {
    int16_t i16 = (int16_t) data;
    n = fwrite (M_ASSIGN_CAST(const void*, &i16), sizeof i16, 1, f);
  } else if (size_of_type == 4) {
    int32_t i32 = (int32_t) data;
    n = fwrite (M_ASSIGN_CAST(const void*, &i32), sizeof i32, 1, f);
  } else {
    M_ASSERT(size_of_type == 8);
    int64_t i64 = (int64_t) data;
    n = fwrite (M_ASSIGN_CAST(const void*, &i64), sizeof i64, 1, f);
  }
  return n == 1 ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Write the float 'data' of 'size_of_type' bytes into the serial stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_bin, _write_float, m_serial_write_t serial, const long double data, const size_t size_of_type)
{
  M_UNUSED_CONTEXT();
  size_t n;
  
  FILE *f = (FILE *)serial->data[0].p;
  if (size_of_type == sizeof (float) ) {
    float f1 = (float) data;
    n = fwrite (M_ASSIGN_CAST(const void*, &f1), sizeof f1, 1, f);
  } else if (size_of_type == sizeof (double) ) {
    double f2 = (double) data;
    n = fwrite (M_ASSIGN_CAST(const void*, &f2), sizeof f2, 1, f);
  } else {
    M_ASSERT(size_of_type == sizeof (long double) );
    long double f3 = (long double) data;
    n = fwrite (M_ASSIGN_CAST(const void*, &f3), sizeof f3, 1, f);
  }
  return n == 1 ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Write the null-terminated string 'data'into the serial stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_bin, _write_string, m_serial_write_t serial, const char data[], size_t length)
{
  M_UNUSED_CONTEXT();
  M_ASSERT_SLOW(length == strlen(data) );
  FILE *f = (FILE *)serial->data[0].p;
  M_ASSERT(f != NULL && data != NULL);
  // Write first the number of (non null) characters
  if (m_ser1al_bin_write_size(f, length) != true) return m_core_serial_fail();
  // Write the characters (excluding the final null char)
  // NOTE: fwrite supports length == 0.
  size_t n = fwrite (M_ASSIGN_CAST(const void*, data), 1, length, f);
  return (n == length) ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Start writing an array of 'number_of_elements' objects into the serial stream 'serial'.
   If 'number_of_elements' is 0, then either the array has no data,
   or the number of elements of the array is unkown.
   Initialize 'local' so that it can be used to serialize the array 
   (local is an unique serialization object of the array).
   Return M_SERIAL_OK_CONTINUE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_bin, _write_array_start, m_serial_local_t local, m_serial_write_t serial, const size_t number_of_elements)
{
  M_UNUSED_CONTEXT();
  (void) local; //Unused
  if (number_of_elements == (size_t)-1) return M_SERIAL_FAIL_RETRY;
  FILE *f = (FILE *)serial->data[0].p;
  size_t n = fwrite (M_ASSIGN_CAST(const void*, &number_of_elements), sizeof number_of_elements, 1, f);
  return n == 1 ? M_SERIAL_OK_CONTINUE : m_core_serial_fail();
}

/* Write an array separator between elements of an array into the serial stream 'serial' if needed.
   Return M_SERIAL_OK_CONTINUE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_bin, _write_array_next, m_serial_local_t local, m_serial_write_t serial)
{
  M_UNUSED_CONTEXT();
  (void) local; // Unused
  (void) serial; // Unused
  return M_SERIAL_OK_CONTINUE;
}

/* End the writing of an array into the serial stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_bin, _write_array_end, m_serial_local_t local, m_serial_write_t serial)
{
  M_UNUSED_CONTEXT();
  (void) local; // Unused
  (void) serial; // Unused
  return M_SERIAL_OK_CONTINUE;
}

/* Write a value separator between element of the same pair of a map into the serial stream 'serial' if needed.
   Return M_SERIAL_OK_CONTINUE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_bin, _write_map_value, m_serial_local_t local, m_serial_write_t serial)
{
  M_UNUSED_CONTEXT();
  (void) local; // argument not used
  (void) serial;
  return M_SERIAL_OK_CONTINUE;
}

/* Start writing a tuple into the serial stream 'serial'.
   Initialize 'local' so that it can serial the tuple 
   (local is an unique serialization object of the tuple).
   Return M_SERIAL_OK_CONTINUE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_bin, _write_tuple_start, m_serial_local_t local, m_serial_write_t serial)
{
  M_UNUSED_CONTEXT();
  (void) local; // argument not used
  (void) serial;
  return M_SERIAL_OK_CONTINUE;
}

/* Start writing the field named field_name[index] of a tuple into the serial stream 'serial'.
   Return M_SERIAL_OK_CONTINUE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_bin, _write_tuple_id, m_serial_local_t local, m_serial_write_t serial, const char *const field_name[], const int max, const int index)
{
  M_UNUSED_CONTEXT();
  (void) local; // argument not used
  (void) serial;
  (void) field_name;
  (void) max;
  (void) index; // Assume index are write in order from 0 to max.
  return M_SERIAL_OK_CONTINUE;
}

/* End the write of a tuple into the serial stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_bin, _write_tuple_end, m_serial_local_t local, m_serial_write_t serial)
{
  M_UNUSED_CONTEXT();
  (void) local; // argument not used
  (void) serial;
  return M_SERIAL_OK_DONE;
}

/* Start writing a variant into the serial stream 'serial'.
   If index <= 0, the variant is empty.
     Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise 
   Otherwise, the field 'field_name[index]' will be filled.
     Return M_SERIAL_OK_CONTINUE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_bin, _write_variant_start, m_serial_local_t local, m_serial_write_t serial, const char *const field_name[], const int max, const int index)
{
  M_UNUSED_CONTEXT();
  (void) field_name;
  (void) max;
  (void) local;
  FILE *f = (FILE *)serial->data[0].p;
  size_t n = fwrite (M_ASSIGN_CAST(const void*, &index), sizeof index, 1, f);
  return n == 1 ? ((index < 0) ? M_SERIAL_OK_DONE : M_SERIAL_OK_CONTINUE) : m_core_serial_fail();
}

/* End Writing a variant into the serial stream 'serial'. 
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_bin, _write_variant_end, m_serial_local_t local, m_serial_write_t serial)
{
  M_UNUSED_CONTEXT();
  (void) local; // argument not used
  (void) serial;
  return M_SERIAL_OK_DONE;
}

/* The exported interface. */
static const m_serial_write_interface_t m_ser1al_bin_write_interface = {
  m_ser1al_bin_write_boolean,
  m_ser1al_bin_write_integer,
  m_ser1al_bin_write_float,
  m_ser1al_bin_write_string,
  m_ser1al_bin_write_array_start,
  m_ser1al_bin_write_array_next,
  m_ser1al_bin_write_array_end,
  m_ser1al_bin_write_array_start,
  m_ser1al_bin_write_map_value,
  m_ser1al_bin_write_array_next,
  m_ser1al_bin_write_array_end,
  m_ser1al_bin_write_tuple_start,
  m_ser1al_bin_write_tuple_id,
  m_ser1al_bin_write_tuple_end,
  m_ser1al_bin_write_variant_start,
  m_ser1al_bin_write_variant_end
};

M_INLINE void m_serial_bin_write_init(m_serial_write_t serial, FILE *f)
{
  serial->m_interface = &m_ser1al_bin_write_interface;
  serial->data[0].p = M_ASSIGN_CAST(void*, f);
}

M_INLINE void m_serial_bin_write_clear(m_serial_write_t serial)
{
  (void) serial; // Nothing to do
}


/* Define a synonym of m_serial_read_t to the BIN serializer with its proper OPLIST */
typedef m_serial_write_t m_serial_bin_write_t;

#define M_OPL_m_serial_bin_write_t()                                          \
  (INIT_WITH(m_serial_bin_write_init), CLEAR(m_serial_bin_write_clear),       \
  TYPE(m_serial_bin_write_t), PROPERTIES(( LET_AS_INIT_WITH(1) )) )



/********************************************************************************/
/************************** FILE / READ  / BIN    *******************************/
/********************************************************************************/

/* Read from the stream 'serial' a boolean.
   Set '*b' with the boolean value if succeeds 
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_bin_read_boolean(m_serial_read_t serial, bool *b){
  FILE *f = (FILE*) serial->data[0].p;
  size_t n = fread (M_ASSIGN_CAST(void*, b), sizeof (bool), 1, f);
  return n == 1 ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Read from the stream 'serial' an integer that can be represented with 'size_of_type' bytes.
   Set '*i' with the integer value if succeeds 
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_bin_read_integer(m_serial_read_t serial, long long *i, const size_t size_of_type){
  int8_t   i8;
  int16_t i16;
  int32_t i32;
  int64_t i64;
  size_t n;
  FILE *f = (FILE *)serial->data[0].p;
  if (size_of_type == 1) {
    n = fread (M_ASSIGN_CAST(void*, &i8), sizeof i8, 1, f);
    *i = i8;
  } else if (size_of_type == 2) {
    n = fread (M_ASSIGN_CAST(void*, &i16), sizeof i16, 1, f);
    *i = i16;
  } else if (size_of_type ==  4) {
    n = fread (M_ASSIGN_CAST(void*, &i32), sizeof i32, 1, f);
    *i = i32;
  } else {
    M_ASSERT(size_of_type == 8);
    n = fread (M_ASSIGN_CAST(void*, &i64), sizeof i64, 1, f);
    *i = i64;
  }
  return n == 1 ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Read from the stream 'serial' a float that can be represented with 'size_of_type' bytes.
   Set '*r' with the boolean value if succeeds 
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_bin_read_float(m_serial_read_t serial, long double *r, const size_t size_of_type){
  float   f1;
  double  f2;
  long double f3;
  size_t n;
  FILE *f = (FILE *)serial->data[0].p;
  if (size_of_type == sizeof f1) {
    n = fread (M_ASSIGN_CAST(void*, &f1), sizeof f1, 1, f);
    *r = f1;
  } else if (size_of_type == sizeof f2) {
    n = fread (M_ASSIGN_CAST(void*, &f2), sizeof f2, 1, f);
    *r = f2;
  } else {
    M_ASSERT(size_of_type == sizeof f3);
    n = fread (M_ASSIGN_CAST(void*, &f3), sizeof f3, 1, f);
    *r = f3;
  }
  return n == 1 ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Read from the stream 'serial' a string.
   Set 's' with the string if succeeds 
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_bin, _read_string, m_serial_read_t serial, struct m_string_s *s)
{
  FILE *f = (FILE*) serial->data[0].p;
  M_ASSERT(f != NULL && s != NULL);
  // First read the number of non null characters
  size_t length;
  if (m_ser1al_bin_read_size(f, &length) != true) return m_core_serial_fail();
  // Use of internal string interface to dimension the string
  char *p = m_str1ng_fit2size M_R(s, length + 1);
  m_str1ng_set_size(s, length);
  // Read the characters excluding the final null one.
  // NOTE: fread supports length == 0.
  size_t n = fread(M_ASSIGN_CAST(void*, p), 1, length, f);
  // Force the final null character
  p[length] = 0;
  return (n == length) ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Start reading from the stream 'serial' an array.
   Set '*num' with the number of elements, or 0 if it is not known.
   Initialize 'local' so that it can be used to serialize the array 
   (local is an unique serialization object of the array).
   Return M_SERIAL_OK_CONTINUE if it succeeds and the array continue,
   M_SERIAL_OK_DONE if it succeeds and the array ends (the array is empty),
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_bin_read_array_start(m_serial_local_t local, m_serial_read_t serial, size_t *num)
{
  FILE *f = (FILE*) serial->data[0].p;
  size_t n = fread (M_ASSIGN_CAST(void*, num), sizeof *num, 1, f);
  local->data[1].s = *num;
  return (n != 1) ? m_core_serial_fail() : (local->data[1].s == 0) ? M_SERIAL_OK_DONE : M_SERIAL_OK_CONTINUE;
}

/* Continue reading from the stream 'serial' an array.
   Return M_SERIAL_OK_CONTINUE if it succeeds and the array continue,
   M_SERIAL_OK_DONE if it succeeds and the array ends,
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_bin_read_array_next(m_serial_local_t local, m_serial_read_t serial)
{
  (void) serial; // Unused
  M_ASSERT(local->data[1].s > 0);
  local->data[1].s --;
  return local->data[1].s == 0 ? M_SERIAL_OK_DONE : M_SERIAL_OK_CONTINUE;
}


/* Continue reading from the stream 'serial' the value separator
   Return M_SERIAL_OK_CONTINUE if it succeeds and the map continue,
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_bin_read_map_value(m_serial_local_t local, m_serial_read_t serial)
{
  (void) local; // argument not used
  (void) serial;
  return M_SERIAL_OK_CONTINUE;
}

/* Start reading a tuple from the stream 'serial'.
   Return M_SERIAL_OK_CONTINUE if it succeeds and the tuple continues,
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_bin_read_tuple_start(m_serial_local_t local, m_serial_read_t serial)
{
  (void) serial;
  local->data[1].i = 0;
  return M_SERIAL_OK_CONTINUE;
}

/* Continue reading a tuple from the stream 'serial'.
   Set '*id' with the corresponding index of the table 'field_name[max]'
   associated to the parsed field in the stream.
   Return M_SERIAL_OK_CONTINUE if it succeeds and the tuple continues,
   Return M_SERIAL_OK_DONE if it succeeds and the tuple ends,
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_bin_read_tuple_id(m_serial_local_t local, m_serial_read_t serial, const char *const field_name [], const int max, int *id)
{
  (void) serial;
  (void) field_name;
  (void) max;
  *id = local->data[1].i;
  local->data[1].i ++;
  return (*id == max) ? M_SERIAL_OK_DONE : M_SERIAL_OK_CONTINUE;
}

/* Start reading a variant from the stream 'serial'.
   Set '*id' with the corresponding index of the table 'field_name[max]'
   associated to the parsed field in the stream.
   Return M_SERIAL_OK_CONTINUE if it succeeds and the variant continues,
   Return M_SERIAL_OK_DONE if it succeeds and the variant ends(variant is empty),
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_bin_read_variant_start(m_serial_local_t local, m_serial_read_t serial, const char *const field_name[], const int max, int*id)
{
  (void) field_name;
  (void) max;
  (void) local; // argument not used
  FILE *f = (FILE*) serial->data[0].p;
  size_t n = fread (M_ASSIGN_CAST(void*, id), sizeof *id, 1, f);
  return n == 1 ? ((*id < 0) ? M_SERIAL_OK_DONE : M_SERIAL_OK_CONTINUE) : m_core_serial_fail();
}

/* End reading a variant from the stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds and the variant ends,
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_bin_read_variant_end(m_serial_local_t local, m_serial_read_t serial)
{
  (void) local; // argument not used
  (void) serial;
  return M_SERIAL_OK_DONE;
}

static const m_serial_read_interface_t m_ser1al_bin_read_interface = {
  m_ser1al_bin_read_boolean,
  m_ser1al_bin_read_integer,
  m_ser1al_bin_read_float,
  m_ser1al_bin_read_string,
  m_ser1al_bin_read_array_start,
  m_ser1al_bin_read_array_next,
  m_ser1al_bin_read_array_start,
  m_ser1al_bin_read_map_value,
  m_ser1al_bin_read_array_next,
  m_ser1al_bin_read_tuple_start,
  m_ser1al_bin_read_tuple_id,
  m_ser1al_bin_read_variant_start,
  m_ser1al_bin_read_variant_end
};

M_INLINE void m_serial_bin_read_init(m_serial_read_t serial, FILE *f)
{
  serial->m_interface = &m_ser1al_bin_read_interface;
  serial->data[0].p = M_ASSIGN_CAST(void*, f);
}

M_INLINE void m_serial_bin_read_clear(m_serial_read_t serial)
{
  (void) serial; // Nothing to do
}

/* Define a synonym of m_serial_read_t to the BIN serializer with its proper OPLIST */
typedef m_serial_read_t m_serial_bin_read_t;
#define M_OPL_m_serial_bin_read_t()                                           \
  (INIT_WITH(m_serial_bin_read_init), CLEAR(m_serial_bin_read_clear),         \
   TYPE(m_serial_bin_read_t), PROPERTIES(( LET_AS_INIT_WITH(1) )) )

M_END_PROTECTED_CODE

#endif
