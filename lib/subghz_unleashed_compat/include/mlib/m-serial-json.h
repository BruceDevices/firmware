/*
 * M*LIB - Serial JSON
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
#ifndef MSTARLIB_SERIAL_JSON_H
#define MSTARLIB_SERIAL_JSON_H

#include "m-core.h"
#include "m-string.h"

M_BEGIN_PROTECTED_CODE

/********************************************************************************/
/*************************** FILE / WRITE / JSON ********************************/
/********************************************************************************/

/* Write the boolean 'data' into the serial stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_json, _write_boolean, m_serial_write_t serial, const bool data)
{
  M_UNUSED_CONTEXT();
  FILE *f = (FILE *)serial->data[0].p;
  int n = fprintf(f, "%s", data ? "true" : "false");
  return n > 0 ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Write the integer 'data' of 'size_of_type' bytes into the serial stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_json, _write_integer, m_serial_write_t serial,const long long data, const size_t size_of_type)
{
  M_UNUSED_CONTEXT();
  (void) size_of_type; // Ignored
  FILE *f = (FILE *)serial->data[0].p;
  int n = fprintf(f, "%lld", data);
  return n > 0 ? M_SERIAL_OK_DONE : m_core_serial_fail();  
}

/* Write the float 'data' of 'size_of_type' bytes into the serial stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_json, _write_float, m_serial_write_t serial, const long double data, const size_t size_of_type)
{
  M_UNUSED_CONTEXT();
  (void) size_of_type; // Ignored
  FILE *f = (FILE *)serial->data[0].p;
  int n = fprintf(f, "%Lf", data);
  return n > 0 ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Write the null-terminated string 'data'into the serial stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_json, _write_string, m_serial_write_t serial, const char data[], size_t length)
{
  M_UNUSED_CONTEXT();
  M_ASSERT(length == (m_str1ng_size_t) length);
  M_ASSERT_SLOW(length == strlen(data) );
  FILE *f = (FILE *)serial->data[0].p;
  M_ASSERT(f != NULL && data != NULL);
  /* HACK: Build dummy string to reuse m_string_out_str */
  m_string_t v2;
  uintptr_t ptr = (uintptr_t) data;
  v2->u.heap.size = (m_str1ng_size_t) length;
  v2->u.heap.alloc[sizeof(m_str1ng_size_t)-2] = 1;
  v2->u.heap.alloc[sizeof(m_str1ng_size_t)-1] = 31;
  v2->u.heap.ptr = (char*)ptr;
  m_string_out_str(f, v2);
  return M_UNLIKELY(ferror(f)) ? m_core_serial_fail() : M_SERIAL_OK_DONE;
}

/* Start writing an array of 'number_of_elements' objects into the serial stream 'serial'.
   If 'number_of_elements' is 0, then either the array has no data,
   or the number of elements of the array is unkown.
   Initialize 'local' so that it can be used to serialize the array 
   (local is an unique serialization object of the array).
   Return M_SERIAL_OK_CONTINUE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_json, _write_array_start, m_serial_local_t local, m_serial_write_t serial, const size_t number_of_elements)
{
  M_UNUSED_CONTEXT();
  (void) local; // argument not used
  (void) number_of_elements; // Ignored
  FILE *f = (FILE *)serial->data[0].p;
  int n = fprintf(f, "[");
  return n > 0 ? M_SERIAL_OK_CONTINUE : m_core_serial_fail();
}

/* Write an array separator between elements of an array into the serial stream 'serial' if needed.
   Return M_SERIAL_OK_CONTINUE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_json, _write_array_next, m_serial_local_t local, m_serial_write_t serial)
{
  M_UNUSED_CONTEXT();
  (void) local; // argument not used
  FILE *f = (FILE *)serial->data[0].p;
  int n = fprintf(f, ",");
  return n > 0 ? M_SERIAL_OK_CONTINUE : m_core_serial_fail();
}

/* End the writing of an array into the serial stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_json, _write_array_end, m_serial_local_t local, m_serial_write_t serial)
{
  M_UNUSED_CONTEXT();
  (void) local; // argument not used
  FILE *f = (FILE *)serial->data[0].p;
  int n = fprintf(f, "]");
  return n > 0 ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Start writing a map of 'number_of_elements' pairs of objects into the serial stream 'serial'.
   If 'number_of_elements' is 0, then either the map has no data,
   or the number of elements is unkown.
   Initialize 'local' so that it can be used to serialize the map 
   (local is an unique serialization object of the map).
   Return M_SERIAL_OK_CONTINUE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_json, _write_map_start, m_serial_local_t local, m_serial_write_t serial, const size_t number_of_elements)
{
  M_UNUSED_CONTEXT();
  (void) local; // argument not used
  (void) number_of_elements; // Ignored
  FILE *f = (FILE *)serial->data[0].p;
  int n = fprintf(f, "{");
  return n > 0 ? M_SERIAL_OK_CONTINUE : m_core_serial_fail();
}

/* Write a value separator between element of the same pair of a map into the serial stream 'serial' if needed.
   Return M_SERIAL_OK_CONTINUE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_json, _write_map_value, m_serial_local_t local, m_serial_write_t serial)
{
  M_UNUSED_CONTEXT();
  (void) local; // argument not used
  FILE *f = (FILE *)serial->data[0].p;
  int n = fprintf(f, ":");
  return n > 0 ? M_SERIAL_OK_CONTINUE : m_core_serial_fail();
}

/* Write a map separator between elements of a map into the serial stream 'serial' if needed.
   Return M_SERIAL_OK_CONTINUE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_json, _write_map_next, m_serial_local_t local, m_serial_write_t serial)
{
  M_UNUSED_CONTEXT();
  (void) local; // argument not used
  FILE *f = (FILE *)serial->data[0].p;
  int n = fprintf(f, ",");
  return n > 0 ? M_SERIAL_OK_CONTINUE : m_core_serial_fail();
}

/* End the writing of a map into the serial stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_json, _write_map_end, m_serial_local_t local, m_serial_write_t serial)
{
  M_UNUSED_CONTEXT();
  (void) local; // argument not used
  FILE *f = (FILE *)serial->data[0].p;
  int n = fprintf(f, "}");
  return n > 0 ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Start writing a tuple into the serial stream 'serial'.
   Initialize 'local' so that it can serial the tuple 
   (local is an unique serialization object of the tuple).
   Return M_SERIAL_OK_CONTINUE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_json, _write_tuple_start, m_serial_local_t local, m_serial_write_t serial)
{
  M_UNUSED_CONTEXT();
  (void) local; // argument not used
  FILE *f = (FILE *)serial->data[0].p;
  int n = fprintf(f, "{");
  return n > 0 ? M_SERIAL_OK_CONTINUE : m_core_serial_fail();
}

/* Start writing the field named field_name[index] of a tuple into the serial stream 'serial'.
   Return M_SERIAL_OK_CONTINUE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_json, _write_tuple_id, m_serial_local_t local, m_serial_write_t serial, const char *const field_name[], const int max, const int index)
{
  M_UNUSED_CONTEXT();
  (void) local; // argument not used
  (void) max; // Ignored
  FILE *f = (FILE *)serial->data[0].p;
  int n = fprintf(f, "%c\"%s\":", index == 0 ? ' ' : ',', field_name[index]);
  return n > 0 ? M_SERIAL_OK_CONTINUE : m_core_serial_fail();
}

/* End the write of a tuple into the serial stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_json, _write_tuple_end, m_serial_local_t local, m_serial_write_t serial)
{
  M_UNUSED_CONTEXT();
  (void) local; // argument not used
  FILE *f = (FILE *)serial->data[0].p;
  int n = fprintf(f, "}");
  return n > 0 ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Start writing a variant into the serial stream 'serial'.
   If index <= 0, the variant is empty.
     Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise 
   Otherwise, the field 'field_name[index]' will be filled.
     Return M_SERIAL_OK_CONTINUE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_json, _write_variant_start, m_serial_local_t local, m_serial_write_t serial, const char *const field_name[], const int max, const int index)
{
  M_UNUSED_CONTEXT();
  (void) local; // argument not used
  (void) max; // argument not used
  FILE *f = (FILE *)serial->data[0].p;
  int n;
  if (index >= 0) {
    M_ASSERT (index < max);
    n = fprintf(f, "{\"%s\":", field_name[index]);
    return n > 0 ? M_SERIAL_OK_CONTINUE : m_core_serial_fail();
  } else {
    n = fprintf(f, "{}");
    return n > 0 ? M_SERIAL_OK_DONE : m_core_serial_fail();
  }
}

/* End Writing a variant into the serial stream 'serial'. 
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_json, _write_variant_end, m_serial_local_t local, m_serial_write_t serial)
{
  M_UNUSED_CONTEXT();
  (void) local; // argument not used
  FILE *f = (FILE *)serial->data[0].p;
  int n = fprintf(f, "}");
  return n > 0 ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* The internal exported interface of m_serial_write_json
   If it is not used, it will be optimized away by the compiler. */
static const m_serial_write_interface_t m_ser1al_json_write_interface = {
  m_ser1al_json_write_boolean,
  m_ser1al_json_write_integer,
  m_ser1al_json_write_float,
  m_ser1al_json_write_string,
  m_ser1al_json_write_array_start,
  m_ser1al_json_write_array_next,
  m_ser1al_json_write_array_end,
  m_ser1al_json_write_map_start,
  m_ser1al_json_write_map_value,
  m_ser1al_json_write_map_next,
  m_ser1al_json_write_map_end,
  m_ser1al_json_write_tuple_start,
  m_ser1al_json_write_tuple_id,
  m_ser1al_json_write_tuple_end,
  m_ser1al_json_write_variant_start,
  m_ser1al_json_write_variant_end
};

/* Initialize the JSON serial object for writing any object to JSON format in the given FILE */
M_INLINE void m_serial_json_write_init(m_serial_write_t serial, FILE *f)
{
  serial->m_interface = &m_ser1al_json_write_interface;
  serial->data[0].p = M_ASSIGN_CAST(void*, f);
}

/* CLear the JSON serial object for writing*/
M_INLINE void m_serial_json_write_clear(m_serial_write_t serial)
{
  (void) serial; // Nothing to do
}

/* Define a synonym to the JSON serializer with a proper OPLIST */
typedef m_serial_write_t m_serial_json_write_t;
#define M_OPL_m_serial_json_write_t()                                         \
  (INIT_WITH(m_serial_json_write_init), CLEAR(m_serial_json_write_clear),     \
  TYPE(m_serial_json_write_t), PROPERTIES(( LET_AS_INIT_WITH(1) )) )




/********************************************************************************/
/*************************** FILE / READ  / JSON ********************************/
/********************************************************************************/

/* Read from the stream 'serial' a boolean.
   Set '*b' with the boolean value if succeeds 
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_json_read_boolean(m_serial_read_t serial, bool *b){
  FILE *f = (FILE*) serial->data[0].p;
  int c = m_core_fgetc_nospace(f);
  if (c == 't') {
    *b = true;
    c = fgetc(f);
    if (c != 'r') return m_core_serial_fail();
    c = fgetc(f);
    if (c != 'u') return m_core_serial_fail();
    c = fgetc(f);
    if (c != 'e') return m_core_serial_fail();
    return M_SERIAL_OK_DONE;
  } else if (c == 'f') {
    *b = false;
    c = fgetc(f);
    if (c != 'a') return m_core_serial_fail();
    c = fgetc(f);
    if (c != 'l') return m_core_serial_fail();
    c = fgetc(f);
    if (c != 's') return m_core_serial_fail();
    c = fgetc(f);
    if (c != 'e') return m_core_serial_fail();
    return M_SERIAL_OK_DONE;
  } else {
    return m_core_serial_fail();
  }
}

/* Read from the stream 'serial' an integer that can be represented with 'size_of_type' bytes.
   Set '*i' with the integer value if succeeds 
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_json_read_integer(m_serial_read_t serial, long long *i, const size_t size_of_type){
  (void) size_of_type; // Ignored
  FILE *f = (FILE*) serial->data[0].p;
  return m_core_fscanf(f, " %lld", i) == 1 ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Read from the stream 'serial' a float that can be represented with 'size_of_type' bytes.
   Set '*r' with the boolean value if succeeds 
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_json_read_float(m_serial_read_t serial, long double *r, const size_t size_of_type){
  (void) size_of_type; // Ignored
  FILE *f = (FILE*) serial->data[0].p;
  return m_core_fscanf(f, " %Lf", r) == 1 ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Read from the stream 'serial' a string.
   Set 's' with the string if succeeds 
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_json, _read_string, m_serial_read_t serial, struct m_string_s *s)
{
  FILE *f = (FILE*) serial->data[0].p;
  int n = m_core_fscanf(f, " "); // Skip any leading spaces.
  (void) n; // Unused return value.
  return m_string_in_str M_R(s, f) ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Start reading from the stream 'serial' an array.
   Set '*num' with the number of elements, or 0 if it is not known.
   Initialize 'local' so that it can be used to serialize the array 
   (local is an unique serialization object of the array).
   Return M_SERIAL_OK_CONTINUE if it succeeds and the array continue,
   M_SERIAL_OK_DONE if it succeeds and the array ends (the array is empty),
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_json_read_array_start(m_serial_local_t local, m_serial_read_t serial, size_t *num)
{
  (void) local; // argument not used
  FILE *f = (FILE*) serial->data[0].p;
  int final1 = -1, final2 = -1;
  int n = m_core_fscanf(f, " [%n ]%n", &final1, &final2);
  (void) n; // Unused. Parsing is checked through c (more robust)
  *num = 0; // don't know the size of the array.
  // Test how much the parsing succeed.
  if (final1 < 0) return m_core_serial_fail();
  return (final2 < 0) ? M_SERIAL_OK_CONTINUE : M_SERIAL_OK_DONE;
}

/* Continue reading from the stream 'serial' an array.
   Return M_SERIAL_OK_CONTINUE if it succeeds and the array continue,
   M_SERIAL_OK_DONE if it succeeds and the array ends,
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_json_read_array_next(m_serial_local_t local, m_serial_read_t serial)
{
  (void) local; // argument not used
  FILE *f = (FILE*) serial->data[0].p;
  char c = 0;
  int n = m_core_fscanf(f, " %c", m_core_arg_size(&c, 1) );
  (void) n; // Unused. Parsing is checked through c (more robust)
  return c == ',' ? M_SERIAL_OK_CONTINUE : c == ']' ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Start reading from the stream 'serial' a map.
   Set '*num' with the number of elements, or 0 if it is not known.
   Initialize 'local' so that it can be used to serialize the array 
   (local is an unique serialization object of the array).
   Return M_SERIAL_OK_CONTINUE if it succeeds and the map continue,
   M_SERIAL_OK_DONE if it succeeds and the map ends (the map is empty),
   m_core_serial_fail() otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_json_read_map_start(m_serial_local_t local, m_serial_read_t serial, size_t *num)
{
  (void) local; // argument not used
  FILE *f = (FILE*) serial->data[0].p;
  int final1 = -1, final2 = -1;
  int n = m_core_fscanf(f, " {%n }%n", &final1, &final2);
  (void) n; // Unused. Parsing is checked through final (more robust)
  *num = 0; // don't know the size of the map.
  // Test how much the parsing succeed.
  if (final1 < 0) return m_core_serial_fail();
  return (final2 < 0) ? M_SERIAL_OK_CONTINUE : M_SERIAL_OK_DONE;
}

/* Continue reading from the stream 'serial' the value separator
   Return M_SERIAL_OK_CONTINUE if it succeeds and the map continue,
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_json_read_map_value(m_serial_local_t local, m_serial_read_t serial)
{
  (void) local; // argument not used
  FILE *f = (FILE*) serial->data[0].p;
  int final = -1;
  int n = m_core_fscanf(f, " :%n", &final);
  (void) n; // Unused. Parsing is checked through final (more robust)
  return final > 0 ? M_SERIAL_OK_CONTINUE : m_core_serial_fail();
}

/* Continue reading from the stream 'serial' a map.
   Return M_SERIAL_OK_CONTINUE if it succeeds and the map continue,
   M_SERIAL_OK_DONE if it succeeds and the map ends,
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_json_read_map_next(m_serial_local_t local, m_serial_read_t serial)
{
  (void) local; // argument not used
  FILE *f = (FILE*) serial->data[0].p;
  char c = 0;
  int n = m_core_fscanf(f, " %c", m_core_arg_size(&c, 1) );
  (void) n; // Unused. Parsing is checked through c (more robust)
  return c == ',' ? M_SERIAL_OK_CONTINUE : c == '}' ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Start reading a tuple from the stream 'serial'.
   Return M_SERIAL_OK_CONTINUE if it succeeds and the tuple continues,
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_json_read_tuple_start(m_serial_local_t local, m_serial_read_t serial)
{
  (void) local; // argument not used
  FILE *f = (FILE*) serial->data[0].p;
  int final = -1;
  int n = m_core_fscanf(f, " {%n", &final);
  (void) n; // Unused. Parsing is checked through final (more robust)
  return final > 0 ? M_SERIAL_OK_CONTINUE : m_core_serial_fail();
}

/* Continue reading a tuple from the stream 'serial'.
   Set '*id' with the corresponding index of the table 'field_name[max]'
   associated to the parsed field in the stream.
   Return M_SERIAL_OK_CONTINUE if it succeeds and the tuple continues,
   Return M_SERIAL_OK_DONE if it succeeds and the tuple ends,
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_json_read_tuple_id(m_serial_local_t local, m_serial_read_t serial, const char *const field_name [], const int max, int *id)
{
  (void) local; // argument not used
  FILE *f = (FILE*) serial->data[0].p;
  int c = m_core_fgetc_nospace(f);
  if (c == EOF) return m_core_serial_fail();
  if (c == '}') return M_SERIAL_OK_DONE;
  if (c == ',') {
     // If first call of read_tuple_id, it is a failure
     if (*id == -1) return m_core_serial_fail();
  } else {
    // c should be \" but let fscanf parse it.
    ungetc(c, f);
  }
  /* Read the field in the JSON */
  c = -1;
  char field[M_USE_IDENTIFIER_ALLOC+1];
  int nn = m_core_fscanf(f, " \"%" M_AS_STR(M_USE_IDENTIFIER_ALLOC) "[^ \t\n\"]\":%n",
                m_core_arg_size(field, M_USE_IDENTIFIER_ALLOC+1), &c);
  (void) nn ; // Unused. Check is done through 'c' (more robust).
  if (c <= 0)
    return m_core_serial_fail();
  /* Search for field in field_name */
  for(int n = 0; n < max; n++) {
    if (strcmp(field, field_name[n]) == 0) {
      *id = n;
      return M_SERIAL_OK_CONTINUE;
    }
  }
  return m_core_serial_fail();
}

/* Start reading a variant from the stream 'serial'.
   Set '*id' with the corresponding index of the table 'field_name[max]'
   associated to the parsed field in the stream.
   Return M_SERIAL_OK_CONTINUE if it succeeds and the variant continues,
   Return M_SERIAL_OK_DONE if it succeeds and the variant ends(variant is empty),
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_json_read_variant_start(m_serial_local_t local, m_serial_read_t serial, const char *const field_name[], const int max, int*id)
{
  (void) local; // argument not used
  FILE *f = (FILE*) serial->data[0].p;

  int final1 = -1, final2 = -1;
  // TODO: Accept 'null' as empty variant? 
  int nn = m_core_fscanf(f, " {%n }%n", &final1, &final2);
  (void) nn ; // Unused. Check is done through final (more robust).
  // Test how much the parsing succeed.
  if (final1 <= 0) return m_core_serial_fail();
  if (final2 > 0)  return M_SERIAL_OK_DONE;

  /* Read the field in the JSON */
  char field[M_USE_IDENTIFIER_ALLOC+1];
  nn = m_core_fscanf(f, " \"%" M_AS_STR(M_USE_IDENTIFIER_ALLOC) "[^ \t\n\"]\":%n", 
                m_core_arg_size(field, M_USE_IDENTIFIER_ALLOC+1), &final2);
  (void) nn ; // Unused. Check is done through final (more robust).
  if (final2 <= 0) return m_core_serial_fail();

  /* Linear Search for field in field_name */
  for(int n = 0; n < max; n++) {
    if (strcmp(field, field_name[n]) == 0) {
      *id = n;
      return M_SERIAL_OK_CONTINUE;
    }
  }
  // Field not found
  return m_core_serial_fail();
}

/* End reading a variant from the stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds and the variant ends,
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_json_read_variant_end(m_serial_local_t local, m_serial_read_t serial)
{
  (void) local; // argument not used
  FILE *f = (FILE*) serial->data[0].p;
  int final = -1;
  int n = m_core_fscanf(f, " }%n", &final);
  (void) n ; // Unused. Check is done through final (more robust).
  return final > 0 ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

static const m_serial_read_interface_t m_ser1al_json_read_interface = {
  m_ser1al_json_read_boolean,
  m_ser1al_json_read_integer,
  m_ser1al_json_read_float,
  m_ser1al_json_read_string,
  m_ser1al_json_read_array_start,
  m_ser1al_json_read_array_next,
  m_ser1al_json_read_map_start,
  m_ser1al_json_read_map_value,
  m_ser1al_json_read_map_next,
  m_ser1al_json_read_tuple_start,
  m_ser1al_json_read_tuple_id,
  m_ser1al_json_read_variant_start,
  m_ser1al_json_read_variant_end
};

/* Initialize the JSON serial object for reading any object from JSON format in the given FILE */
M_INLINE void m_serial_json_read_init(m_serial_read_t serial, FILE *f)
{
  serial->m_interface = &m_ser1al_json_read_interface;
  serial->data[0].p = M_ASSIGN_CAST(void*, f);
}

/* Clear the JSON serial object for reading from the FILE */
M_INLINE void m_serial_json_read_clear(m_serial_read_t serial)
{
  (void) serial; // Nothing to do
}

/* Define a synonym of m_serial_read_t 
  to the JSON serializer with its proper OPLIST */
typedef m_serial_read_t m_serial_json_read_t;
#define M_OPL_m_serial_json_read_t()                                          \
  (INIT_WITH(m_serial_json_read_init), CLEAR(m_serial_json_read_clear),       \
  TYPE(m_serial_json_read_t) , PROPERTIES(( LET_AS_INIT_WITH(1) )) )



/********************************************************************************/
/************************** STRING / WRITE / JSON *******************************/
/********************************************************************************/

/* Write the boolean 'data' into the serial stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_str_json, _write_boolean, m_serial_write_t serial, const bool data)
{
  struct m_string_s *f = (struct m_string_s *)serial->data[0].p;
//TODO: Simplify code.
  int n = m_string_cat_printf M_R(f, "%s", data ? "true" : "false");
  return n > 0 ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Write the integer 'data' of 'size_of_type' bytes into the serial stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_str_json, _write_integer, m_serial_write_t serial, const long long data, const size_t size_of_type)
{
  (void) size_of_type; // Ignored
  struct m_string_s *f = (struct m_string_s *)serial->data[0].p;
  int n = m_string_cat_printf M_R(f, "%lld", data);
  return n > 0 ? M_SERIAL_OK_DONE : m_core_serial_fail();  
}

/* Write the float 'data' of 'size_of_type' bytes into the serial stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_str_json, _write_float, m_serial_write_t serial, const long double data, const size_t size_of_type)
{
  (void) size_of_type; // Ignored
  struct m_string_s *f = (struct m_string_s *)serial->data[0].p;
  int n = m_string_cat_printf M_R(f, "%Lf", data);
  return n > 0 ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Write the null-terminated string 'data'into the serial stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_str_json, _write_string, m_serial_write_t serial, const char data[], size_t length)
{
  M_ASSERT(length == (m_str1ng_size_t) length);
  M_ASSERT_SLOW(length == strlen(data) );
  struct m_string_s *f = (struct m_string_s *)serial->data[0].p;
  M_ASSERT(f != NULL && data != NULL);
  /* HACK: Build dummy string to reuse m_string_get_str */
  m_string_t v2;
  uintptr_t ptr = (uintptr_t) data;
  v2->u.heap.size = (m_str1ng_size_t) length;
  v2->u.heap.alloc[sizeof(m_str1ng_size_t)-2] = 1;
  v2->u.heap.alloc[sizeof(m_str1ng_size_t)-1] = 31;
  v2->u.heap.ptr = (char*)ptr;
  m_string_get_str M_R(f, v2, true);
  return M_SERIAL_OK_DONE;
}

/* Start writing an array of 'number_of_elements' objects into the serial stream 'serial'.
   If 'number_of_elements' is 0, then either the array has no data,
   or the number of elements of the array is unkown.
   Initialize 'local' so that it can be used to serialize the array 
   (local is an unique serialization object of the array).
   Return M_SERIAL_OK_CONTINUE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_str_json, _write_array_start, m_serial_local_t local, m_serial_write_t serial, const size_t number_of_elements)
{
  (void) local; // argument not used
  (void) number_of_elements; // Ignored
  struct m_string_s *f = (struct m_string_s *)serial->data[0].p;
  m_string_push_back M_R(f, '[');
  return M_SERIAL_OK_CONTINUE;
}

/* Write an array separator between elements of an array into the serial stream 'serial' if needed.
   Return M_SERIAL_OK_CONTINUE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_str_json, _write_array_next, m_serial_local_t local, m_serial_write_t serial)
{
  (void) local; // argument not used
  struct m_string_s *f = (struct m_string_s *)serial->data[0].p;
  m_string_push_back M_R(f, ',');
  return M_SERIAL_OK_CONTINUE;
}

/* End the writing of an array into the serial stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_str_json, _write_array_end, m_serial_local_t local, m_serial_write_t serial)
{
  (void) local; // argument not used
  struct m_string_s *f = (struct m_string_s *)serial->data[0].p;
  m_string_push_back M_R(f, ']');
  return M_SERIAL_OK_CONTINUE;
}

/* Start writing a map of 'number_of_elements' pairs of objects into the serial stream 'serial'.
   If 'number_of_elements' is 0, then either the map has no data,
   or the number of elements is unkown.
   Initialize 'local' so that it can be used to serialize the map 
   (local is an unique serialization object of the map).
   Return M_SERIAL_OK_CONTINUE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_str_json, _write_map_start, m_serial_local_t local, m_serial_write_t serial, const size_t number_of_elements)
{
  (void) local; // argument not used
  (void) number_of_elements; // Ignored
  struct m_string_s *f = (struct m_string_s *)serial->data[0].p;
  m_string_push_back M_R(f, '{');
  return M_SERIAL_OK_CONTINUE;
}

/* Write a value separator between element of the same pair of a map into the serial stream 'serial' if needed.
   Return M_SERIAL_OK_CONTINUE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_str_json, _write_map_value, m_serial_local_t local, m_serial_write_t serial)
{
  (void) local; // argument not used
  struct m_string_s *f = (struct m_string_s *)serial->data[0].p;
  m_string_push_back M_R(f, ':');
  return M_SERIAL_OK_CONTINUE;
}

/* Write a map separator between elements of a map into the serial stream 'serial' if needed.
   Return M_SERIAL_OK_CONTINUE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_str_json, _write_map_next, m_serial_local_t local, m_serial_write_t serial)
{
  (void) local; // argument not used
  struct m_string_s *f = (struct m_string_s *)serial->data[0].p;
  m_string_push_back M_R(f, ',');
  return M_SERIAL_OK_CONTINUE;
}

/* End the writing of a map into the serial stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_str_json, _write_map_end, m_serial_local_t local, m_serial_write_t serial)
{
  (void) local; // argument not used
  struct m_string_s *f = (struct m_string_s *)serial->data[0].p;
  m_string_push_back M_R(f, '}');
  return M_SERIAL_OK_CONTINUE;
}

/* Start writing a tuple into the serial stream 'serial'.
   Initialize 'local' so that it can serial the tuple 
   (local is an unique serialization object of the tuple).
   Return M_SERIAL_OK_CONTINUE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_str_json, _write_tuple_start, m_serial_local_t local, m_serial_write_t serial)
{
  (void) local; // argument not used
  struct m_string_s *f = (struct m_string_s *)serial->data[0].p;
  m_string_push_back M_R(f, '{');
  return M_SERIAL_OK_CONTINUE;
}

/* Start writing the field named field_name[index] of a tuple into the serial stream 'serial'.
   Return M_SERIAL_OK_CONTINUE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_str_json, _write_tuple_id, m_serial_local_t local, m_serial_write_t serial, const char *const field_name[], const int max, const int index)
{
  (void) local; // argument not used
  (void) max; // Ignored
  struct m_string_s *f = (struct m_string_s *)serial->data[0].p;
  int n = m_string_cat_printf M_R(f, "%c\"%s\":", index == 0 ? ' ' : ',', field_name[index]);
  return n > 0 ? M_SERIAL_OK_CONTINUE : m_core_serial_fail();
}

/* End the write of a tuple into the serial stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_str_json, _write_tuple_end, m_serial_local_t local, m_serial_write_t serial)
{
  (void) local; // argument not used
  struct m_string_s *f = (struct m_string_s *)serial->data[0].p;
  m_string_push_back M_R(f, '}');
  return M_SERIAL_OK_CONTINUE;
}

/* Start writing a variant into the serial stream 'serial'.
   If index <= 0, the variant is empty.
     Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise 
   Otherwise, the field 'field_name[index]' will be filled.
     Return M_SERIAL_OK_CONTINUE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_str_json, _write_variant_start, m_serial_local_t local, m_serial_write_t serial, const char *const field_name[], const int max, const int index)
{
  (void) local; // argument not used
  (void) max; // argument not used
  struct m_string_s *f = (struct m_string_s *)serial->data[0].p;
  int n;
  if (index >= 0) {
    M_ASSERT (index < max);
    n = m_string_cat_printf M_R(f, "{\"%s\":", field_name[index]);
    return n > 0 ? M_SERIAL_OK_CONTINUE : m_core_serial_fail();
  } else {
    n = m_string_cat_printf M_R(f, "{}");
    return n > 0 ? M_SERIAL_OK_DONE : m_core_serial_fail();
  }
}

/* End Writing a variant into the serial stream 'serial'. 
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_str_json, _write_variant_end, m_serial_local_t local, m_serial_write_t serial)
{
  (void) local; // argument not used
  struct m_string_s *f = (struct m_string_s *)serial->data[0].p;
  m_string_push_back M_R(f, '}');
  return M_SERIAL_OK_CONTINUE;
}

/* The internal exported interface of m_serial_write_json. */
static const m_serial_write_interface_t m_ser1al_str_json_write_interface = {
  m_ser1al_str_json_write_boolean,
  m_ser1al_str_json_write_integer,
  m_ser1al_str_json_write_float,
  m_ser1al_str_json_write_string,
  m_ser1al_str_json_write_array_start,
  m_ser1al_str_json_write_array_next,
  m_ser1al_str_json_write_array_end,
  m_ser1al_str_json_write_map_start,
  m_ser1al_str_json_write_map_value,
  m_ser1al_str_json_write_map_next,
  m_ser1al_str_json_write_map_end,
  m_ser1al_str_json_write_tuple_start,
  m_ser1al_str_json_write_tuple_id,
  m_ser1al_str_json_write_tuple_end,
  m_ser1al_str_json_write_variant_start,
  m_ser1al_str_json_write_variant_end
};

/* Initialize the JSON serial object for writing any object to JSON format in the given FILE */
M_INLINE void m_serial_str_json_write_init(m_serial_write_t serial, m_string_t s)
{
  serial->m_interface = &m_ser1al_str_json_write_interface;
  serial->data[0].p = M_ASSIGN_CAST(struct m_string_s *, s);
}

/* CLear the JSON serial object for writing*/
M_INLINE void m_serial_str_json_write_clear(m_serial_write_t serial)
{
  (void) serial; // Nothing to do
}

/* Define a synonym to the JSON serializer with a proper OPLIST */
typedef m_serial_write_t m_serial_str_json_write_t;
#define M_OPL_m_serial_str_json_write_t()                                     \
  (INIT_WITH(m_serial_str_json_write_init), CLEAR(m_serial_str_json_write_clear), \
  TYPE(m_serial_str_json_write_t) , PROPERTIES(( LET_AS_INIT_WITH(1) )) )



/********************************************************************************/
/************************** STRING / READ  / JSON *******************************/
/********************************************************************************/

/* Helper function to read a character and advance the stream */
M_INLINE char
m_ser1al_str_json_getc(const char **p)
{
  char c = **p;
  // Once it reaches the end of string character (== 0), stop advancing the stream.
  *p = *p + (c != 0);
  return c;
}

/* Helper function to skip space(s) */
M_INLINE void
m_ser1al_str_json_skip (const char **p)
{
  (void) m_core_str_nospace(p);
  (*p)--;
}

/* Helper function to read a string with characters not present in reject */
M_INLINE void
m_ser1al_str_json_gets(size_t alloc, char field[], const char reject[], const char **s)
{
  M_ASSERT (alloc > 0);
  size_t length = strcspn(*s, reject);
  if (length >= alloc-1)
    length = alloc - 1;
  memcpy(field, *s, length);
  field[length] = 0;
  *s += length;
}

/* Read from the stream 'serial' a boolean.
   Set '*b' with the boolean value if succeeds 
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_str_json_read_boolean(m_serial_read_t serial, bool *b){
  const char **f = &serial->data[0].cstr;
  m_ser1al_str_json_skip(f);
  char c = m_ser1al_str_json_getc(f);
  if (c == 't') {
    *b = true;
    c = m_ser1al_str_json_getc(f);
    if (M_UNLIKELY(c != 'r')) return m_core_serial_fail();
    c = m_ser1al_str_json_getc(f);
    if (M_UNLIKELY(c != 'u')) return m_core_serial_fail();
    c = m_ser1al_str_json_getc(f);
    if (M_UNLIKELY(c != 'e')) return m_core_serial_fail();
    return M_SERIAL_OK_DONE;
  } else if (M_LIKELY(c == 'f')) {
    *b = false;
    c = m_ser1al_str_json_getc(f);
    if (M_UNLIKELY(c != 'a')) return m_core_serial_fail();
    c = m_ser1al_str_json_getc(f);
    if (M_UNLIKELY(c != 'l')) return m_core_serial_fail();
    c = m_ser1al_str_json_getc(f);
    if (M_UNLIKELY(c != 's')) return m_core_serial_fail();
    c = m_ser1al_str_json_getc(f);
    if (M_UNLIKELY(c != 'e')) return m_core_serial_fail();
    return M_SERIAL_OK_DONE;
  } else {
    return m_core_serial_fail();
  }
}

/* Read from the stream 'serial' an integer that can be represented with 'size_of_type' bytes.
   Set '*i' with the integer value if succeeds 
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_str_json_read_integer(m_serial_read_t serial, long long *i, const size_t size_of_type){
  (void) size_of_type; // Ignored
  const char **f = &serial->data[0].cstr;
  char *e;
  *i = strtoll(*f, &e, 10);
  bool b = e != *f;
  serial->data[0].cstr = (const char*) e;
  return b ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Read from the stream 'serial' a float that can be represented with 'size_of_type' bytes.
   Set '*r' with the boolean value if succeeds 
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_str_json_read_float(m_serial_read_t serial, long double *r, const size_t size_of_type){
  (void) size_of_type; // Ignored
  const char **f = &serial->data[0].cstr;
  char *e;
  *r = strtold(*f, &e);
  bool b = e != *f;
  serial->data[0].cstr = (const char*) e;
  return b ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Read from the stream 'serial' a string.
   Set 's' with the string if succeeds 
   Return M_SERIAL_OK_DONE if it succeeds, M_SERIAL_FAIL otherwise */
M_P(m_serial_return_code_t, m_ser1al_str_json, _read_string, m_serial_read_t serial, struct m_string_s *s)
{
  const char **f = &serial->data[0].cstr;
  m_ser1al_str_json_skip(f);
  const char *e;
  bool b = m_string_parse_str M_R(s, *f, &e);
  serial->data[0].cstr = e;
  return b ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Start reading from the stream 'serial' an array.
   Set '*num' with the number of elements, or 0 if it is not known.
   Initialize 'local' so that it can be used to serialize the array 
   (local is an unique serialization object of the array).
   Return M_SERIAL_OK_CONTINUE if it succeeds and the array continue,
   M_SERIAL_OK_DONE if it succeeds and the array ends (the array is empty),
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_str_json_read_array_start(m_serial_local_t local, m_serial_read_t serial, size_t *num)
{
  (void) local; // argument not used
  const char **f = &serial->data[0].cstr;
  m_ser1al_str_json_skip(f);
  char c = m_ser1al_str_json_getc(f);
  if (M_UNLIKELY(c != '[')) {
    return m_core_serial_fail();
  }
  *num = 0; // Don't know the size of the array.
  m_ser1al_str_json_skip(f);
  if (M_UNLIKELY (**f == ']')) {
    c = m_ser1al_str_json_getc(f);
    assert( c == ']');
    return M_SERIAL_OK_DONE;
  } else {
    return M_SERIAL_OK_CONTINUE;
  }
}

/* Continue reading from the stream 'serial' an array.
   Return M_SERIAL_OK_CONTINUE if it succeeds and the array continue,
   M_SERIAL_OK_DONE if it succeeds and the array ends,
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_str_json_read_array_next(m_serial_local_t local, m_serial_read_t serial)
{
  (void) local; // argument not used
  const char **f = &serial->data[0].cstr;
  m_ser1al_str_json_skip(f);
  char c = m_ser1al_str_json_getc(f);
  return c == ',' ? M_SERIAL_OK_CONTINUE : c == ']' ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Start reading from the stream 'serial' a map.
   Set '*num' with the number of elements, or 0 if it is not known.
   Initialize 'local' so that it can be used to serialize the array 
   (local is an unique serialization object of the array).
   Return M_SERIAL_OK_CONTINUE if it succeeds and the map continue,
   M_SERIAL_OK_DONE if it succeeds and the map ends (the map is empty),
   m_core_serial_fail() otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_str_json_read_map_start(m_serial_local_t local, m_serial_read_t serial, size_t *num)
{
  (void) local; // argument not used
  const char **f = &serial->data[0].cstr;
  m_ser1al_str_json_skip(f);
  char c = m_ser1al_str_json_getc(f);
  if (M_UNLIKELY(c != '{')) {
    return m_core_serial_fail();
  }
  *num = 0; // Don't know the size of the map
  m_ser1al_str_json_skip(f);
  if (M_UNLIKELY (**f == '}')) {
    c = m_ser1al_str_json_getc(f);
    assert(c == '}');
    return M_SERIAL_OK_DONE;
  } else {
    return M_SERIAL_OK_CONTINUE;
  }
}

/* Continue reading from the stream 'serial' the value separator
   Return M_SERIAL_OK_CONTINUE if it succeeds and the map continue,
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_str_json_read_map_value(m_serial_local_t local, m_serial_read_t serial)
{
  (void) local; // argument not used
  const char **f = &serial->data[0].cstr;
  m_ser1al_str_json_skip(f);
  char c = m_ser1al_str_json_getc(f);
  return c ==':' ? M_SERIAL_OK_CONTINUE : m_core_serial_fail();
}

/* Continue reading from the stream 'serial' a map.
   Return M_SERIAL_OK_CONTINUE if it succeeds and the map continue,
   M_SERIAL_OK_DONE if it succeeds and the map ends,
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_str_json_read_map_next(m_serial_local_t local, m_serial_read_t serial)
{
  (void) local; // argument not used
  const char **f = &serial->data[0].cstr;
  m_ser1al_str_json_skip(f);
  char c = m_ser1al_str_json_getc(f);
  return c == ',' ? M_SERIAL_OK_CONTINUE : c == '}' ? M_SERIAL_OK_DONE : m_core_serial_fail();
}

/* Start reading a tuple from the stream 'serial'.
   Return M_SERIAL_OK_CONTINUE if it succeeds and the tuple continues,
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_str_json_read_tuple_start(m_serial_local_t local, m_serial_read_t serial)
{
  (void) local; // argument not used
  const char **f = &serial->data[0].cstr;
  m_ser1al_str_json_skip(f);
  char c = m_ser1al_str_json_getc(f);
  return c == '{' ? M_SERIAL_OK_CONTINUE : m_core_serial_fail();
}

/* Continue reading a tuple from the stream 'serial'.
   Set '*id' with the corresponding index of the table 'field_name[max]'
   associated to the parsed field in the stream.
   Return M_SERIAL_OK_CONTINUE if it succeeds and the tuple continues,
   Return M_SERIAL_OK_DONE if it succeeds and the tuple ends,
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_str_json_read_tuple_id(m_serial_local_t local, m_serial_read_t serial, const char *const field_name [], const int max, int *id)
{
  (void) local; // argument not used
  const char **f = &serial->data[0].cstr;
  m_ser1al_str_json_skip(f);
  char c = m_ser1al_str_json_getc(f);
  if (c == 0) return m_core_serial_fail();
  if (c == '}') return M_SERIAL_OK_DONE;
  if (c == ',') {
    // If first call of read_tuple_id, it is a failure
    if (M_UNLIKELY (*id == -1)) return m_core_serial_fail();
    m_ser1al_str_json_skip(f);
    c = m_ser1al_str_json_getc(f);
  }

  /* Read the field in the JSON */
  char field[M_USE_IDENTIFIER_ALLOC+1];
  if (M_UNLIKELY(c != '"')) return m_core_serial_fail();
  m_ser1al_str_json_gets(M_USE_IDENTIFIER_ALLOC+1, field, " \t\n\"", f);
  c = m_ser1al_str_json_getc(f);
  if (M_UNLIKELY (c != '"')) return m_core_serial_fail();
  c = m_ser1al_str_json_getc(f);
  if (M_UNLIKELY (c != ':')) return m_core_serial_fail();

  /* Search for field in field_name */
  for(int n = 0; n < max; n++) {
    if (strcmp(field, field_name[n]) == 0) {
      *id = n;
      return M_SERIAL_OK_CONTINUE;
    }
  }
  return m_core_serial_fail();
}

/* Start reading a variant from the stream 'serial'.
   Set '*id' with the corresponding index of the table 'field_name[max]'
   associated to the parsed field in the stream.
   Return M_SERIAL_OK_CONTINUE if it succeeds and the variant continues,
   Return M_SERIAL_OK_DONE if it succeeds and the variant ends(variant is empty),
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_str_json_read_variant_start(m_serial_local_t local, m_serial_read_t serial, const char *const field_name[], const int max, int*id)
{
  (void) local; // argument not used
  const char **f = &serial->data[0].cstr;
  m_ser1al_str_json_skip(f);
  int c = m_ser1al_str_json_getc(f);
  if (M_UNLIKELY (c != '{'))
    // TODO: Accept 'null' as empty variant? 
    return m_core_serial_fail();
  m_ser1al_str_json_skip(f);
  c = m_ser1al_str_json_getc(f);
  if (M_UNLIKELY (c == '}')) return M_SERIAL_OK_DONE; // Empty variant

  /* Read the field in the JSON */
  char field[M_USE_IDENTIFIER_ALLOC+1];
  if (M_UNLIKELY(c != '"')) return m_core_serial_fail();
  m_ser1al_str_json_gets(M_USE_IDENTIFIER_ALLOC+1, field, " \t\n\"", f);
  c = m_ser1al_str_json_getc(f);
  if (M_UNLIKELY (c != '"')) return m_core_serial_fail();
  c = m_ser1al_str_json_getc(f);
  if (M_UNLIKELY (c != ':')) return m_core_serial_fail();

  /* Linear Search for field in field_name */
  for(int n = 0; n < max; n++) {
    if (strcmp(field, field_name[n]) == 0) {
      *id = n;
      return M_SERIAL_OK_CONTINUE;
    }
  }
  // Field not found
  return m_core_serial_fail();
}

/* End reading a variant from the stream 'serial'.
   Return M_SERIAL_OK_DONE if it succeeds and the variant ends,
   M_SERIAL_FAIL otherwise */
M_INLINE  m_serial_return_code_t
m_ser1al_str_json_read_variant_end(m_serial_local_t local, m_serial_read_t serial)
{
  (void) local; // argument not used
  const char **f = &serial->data[0].cstr;
  m_ser1al_str_json_skip(f);
  int c = m_ser1al_str_json_getc(f);
  return (M_UNLIKELY (c != '}')) ?  m_core_serial_fail() : M_SERIAL_OK_DONE;
}

static const m_serial_read_interface_t m_ser1al_str_json_read_interface = {
  m_ser1al_str_json_read_boolean,
  m_ser1al_str_json_read_integer,
  m_ser1al_str_json_read_float,
  m_ser1al_str_json_read_string,
  m_ser1al_str_json_read_array_start,
  m_ser1al_str_json_read_array_next,
  m_ser1al_str_json_read_map_start,
  m_ser1al_str_json_read_map_value,
  m_ser1al_str_json_read_map_next,
  m_ser1al_str_json_read_tuple_start,
  m_ser1al_str_json_read_tuple_id,
  m_ser1al_str_json_read_variant_start,
  m_ser1al_str_json_read_variant_end
};

/* Initialize the JSON serial object for reading any object from JSON format in the given const string */
M_INLINE void m_serial_str_json_read_init(m_serial_read_t serial, const char str[])
{
  serial->m_interface = &m_ser1al_str_json_read_interface;
  serial->data[0].cstr = str;
}

/* Clear the JSON serial object for reading from the FILE */
M_INLINE const char *m_serial_str_json_read_clear(m_serial_read_t serial)
{
  // Nothing to clear. Return pointer to the last data parsed.
  return serial->data[0].cstr;
}

/* Define a synonym of m_serial_read_t 
  to the JSON serializer with its proper OPLIST */
typedef m_serial_read_t m_serial_str_json_read_t;
#define M_OPL_m_serial_str_json_read_t()                                      \
  (INIT_WITH(m_serial_str_json_read_init), CLEAR(m_serial_str_json_read_clear), \
  TYPE(m_serial_str_json_read_t), PROPERTIES(( LET_AS_INIT_WITH(1) )) )


M_END_PROTECTED_CODE

#endif
