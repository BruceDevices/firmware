/*
 * M*LIB - Generic module
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
#ifndef MSTARLIB_GENERIC_H
#define MSTARLIB_GENERIC_H

#include "m-core.h"

/* Define the generic macro-functions */
// TODO: m_ prefix?
#define init(x)              M_G3N_CALL_1(INIT, x)
#define init_set(x, y)       M_G3N_CALL_2(INIT_SET, x, TYPE, x, TYPE, y)
#define init_move(x, y)      M_G3N_CALL_2(INIT_MOVE, x, TYPE, x, TYPE, y)
#define move(x, y)           M_G3N_CALL_2(MOVE, x, TYPE, x, TYPE, y)
#define set(x, y)            M_G3N_CALL_2(SET, x, TYPE, x , TYPE, y)
#define clear(x)             M_G3N_CALL_1(CLEAR, x)
#define swap(x, y)           M_G3N_CALL_2(SWAP, x, TYPE, x , TYPE, y)
#define hash(x)              M_G3N_CALL_1(HASH, x)
#define equal(x, y)          M_G3N_CALL_2(EQUAL, x, TYPE, x , TYPE, y)
#define cmp(x, y)            M_G3N_CALL_2(CMP, x, TYPE, x , TYPE, y)
#define sort(x)              M_G3N_CALL_1(SORT, x)
#define splice_back(d, s, i) M_G3N_CALL_3(SPLICE_BACK, d, TYPE, d, TYPE, s, IT_TYPE, i)
#define splice_at(d, id, s, is) M_G3N_CALL_4(SPLICE_BACK, d, TYPE, d, IT_TYPE, id, TYPE, s, IT_TYPE, is)
#define it_type(x)           m_typeof(M_G3N_TYPE_1(IT_TYPE, x))
#define sub_type(x)          m_typeof(M_G3N_TYPE_1(SUBTYPE, x))
#define key_type(x)          m_typeof(M_G3N_TYPE_1(KEY_TYPE, x))
#define value_type(x)        m_typeof(M_G3N_TYPE_1(VALUE_TYPE, x))
#define it_first(x, y)       M_G3N_CALL_2(IT_FIRST, y, IT_TYPE, x, TYPE, y)
#define it_last(x, y)        M_G3N_CALL_i2(IT_LAST, y, IT_TYPE, x, TYPE, y)
#define it_end(x, y)         M_G3N_CALL_i2(IT_END, y, IT_TYPE, x, TYPE, y)
#define it_set(d, s)         M_G3N_CALL_2i(IT_SET, d, IT_TYPE, d, IT_TYPE, s)
#define it_end_p(i)          M_G3N_CALL_1i(IT_END_P, i)
#define it_last_p(i)         M_G3N_CALL_1i(IT_LAST_P, i)
#define it_next(i)           M_G3N_CALL_1i(IT_NEXT, i)
#define it_previous(i)       M_G3N_CALL_1i(IT_PREVIOUS, i)
#define it_ref(i)            M_G3N_CALL_1i(IT_REF, i)
#define it_cref(i)           M_G3N_CALL_1i(IT_CREF, i)
#define it_equal(x, y)       M_G3N_CALL_2i(IT_EQUAL, x, IT_TYPE, x, IT_TYPE, y)
#define it_insert(c, i, o)   M_G2N_CALL_3(IT_INSERT, c, TYPE, c, IT_TYPE, i, SUBTYPE, o)
#define it_remove(c, i)      M_G3N_CALL_2(IT_REMOVE, c, TYPE, c, IT_TYPE, i)
#define empty_p(x)           M_G3N_CALL_1(EMPTY_P, x)
#define add(x, y)            M_G3N_CALL_2(ADD, x, TYPE, x, TYPE, y)
#define sub(x, y)            M_G3N_CALL_2(SUB, x, TYPE, x, TYPE, y)
#define mul(x, y)            M_G3N_CALL_2(MUL, x, TYPE, x, TYPE, y)
#define div(x, y)            M_G3N_CALL_2(DIV, x, TYPE, x, TYPE, y)
#define reset(x)             M_G3N_CALL_1(RESET, x)
#define get(x, y)            M_G3N_CALL_2(GET_KEY, x, TYPE, x, KEY_TYPE, y)
#define set_at(x, y, z)      M_G3N_CALL_3(SET_KEY, x, TYPE, x, KEY_TYPE, y, VALUE_TYPE, z)
#define safe_get(x, y)       M_G3N_CALL_2(SAFE_GET_KEY, x, TYPE, x, KEY_TYPE, y)
#define erase(x, y)          M_G3N_CALL_2(ERASE, x, TYPE, x, KEY_TYPE, y)
#define get_size(x)          M_G3N_CALL_1(GET_SIZE, x)
#define push(x, y)           M_G3N_CALL_2(PUSH, x, TYPE, x, SUBTYPE, y)
#define pop(x, y)            M_G3N_CALL_2(POP, y, SUBTYPE_PTR, x, TYPE, y)
#define push_move(x, y)      M_G3N_CALL_2(PUSH_MOVE, x, TYPE, x, SUBTYPE_PTR, y)
#define pop_move(x, y)       M_G3N_CALL_2(POP_MOVE, y, SUBTYPE_PTR, x, TYPE, y)
#define reverse(x)           M_G3N_CALL_1(REVERSE, x)
#define get_str(s, c, b)     M_G3N_CALL_3(GET_STR, c, string_t, s, TYPE, c, bool, b)
#define parse_str(c, s, e)   M_G3N_CALL_3(PARSE_STR, c, TYPE, c, m_g3n_cstring, s, m_g3n_cstring_end, e)
#define out_str(x, y)        M_G3N_CALL_2(OUT_STR, y, m_g3n_file, x, TYPE, y)
#define in_str(x, y)         M_G3N_CALL_2(IN_STR, x, TYPE, x, m_g3n_file, y)
#define out_serial(x, y)     M_G3N_CALL_2(OUT_SERIAL, y, m_serial_write_t, x, TYPE, y)
#define in_serial(x, y)      M_G3N_CALL_2(IN_SERIAL, x, TYPE, x, m_serial_read_t, y)

#define let(...)             M_LET(__VA_ARGS__)
#define print(...)           M_PRINT(__VA_ARGS__)

// for each(item, container)
#define each(item, container)                                                 \
  M_G3N_EACHI(item, container, M_C(m_local_it_, __LINE__), M_C(local_pass_, __LINE__))

/* User code has to register oplists like this:
#define M_GENERIC_ORG_2() (USER)
#define M_GENERIC_ORG_USER_COMP_1() (CORE)
#define M_GENERIC_ORG_USER_COMP_CORE_OPLIST_6() FLT1
#define M_GENERIC_ORG_USER_COMP_CORE_OPLIST_7() STR1
*/


/*****************************************************************************/
/********************************** INTERNAL *********************************/
/*****************************************************************************/

// Instead of using expensive M_SEQ(1,50), precompute it:
#define M_G3N_SEQ_INT                                                         \
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,  \
  21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, \
  40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50

/* Generate a sequence of the oplist that have been registered.

   3 levels of indirection are needed:
    * One level is the organisation.
    * One level is the component of the organisation.
    * One level is the oplist within the component of the organisation
  Return the list of unevaluated oplist that are registered across all organizations, all components.
  M*LIB Organisation (MLIB) is the default organisation and is always registered
  Usage Domain: Nb organizations * Nb components * Nb oplist <= M_MAX_NB_ARGUMENT
*/
#define M_G3N_REGISTERED_ITEMS()                                              \
  M_CROSS_MAP( M_G3N_IS_PRESENT3, ( comp M_CROSS_MAP(M_G3N_IS_PRESENT2, (MLIB M_MAP(M_G3N_IS_PRESENT, M_G3N_SEQ_INT)), ( M_G3N_SEQ_INT ) ) ), ( M_G3N_SEQ_INT ) )

#define M_G3N_IS_PRESENT(num)                                                 \
  M_IF(M_PARENTHESIS_P(M_C(M_GENERIC_ORG_, num)() ))(M_DEFERRED_COMMA M_ID M_C(M_GENERIC_ORG_, num)(), )

#define M_G3N_IS_PRESENT2(el1, num)                                           \
  M_IF(M_PARENTHESIS_P(M_C4(M_GENERIC_ORG_, el1, _COMP_, num)()))(M_DEFERRED_COMMA M_APPLY(M_C3, el1, _COMP_, M_ID M_C4(M_GENERIC_ORG_, el1, _COMP_, num)()), )

#define M_G3N_IS_PRESENT3(el1, num)                                           \
  M_IF(M_OPLIST_P(M_C4(M_GENERIC_ORG_, el1, _OPLIST_, num)()))(M_DEFERRED_COMMA M_C4(M_GENERIC_ORG_, el1, _OPLIST_, num), )


// Dummy unused structure. Just to fill in default case of the _Generic
struct m_g3neric_dummy_s;

// Synonymous for some basic pointers doesn't work well with preprocessing code
typedef FILE *m_g3n_file;
typedef const char *m_g3n_cstring;
typedef const char **m_g3n_cstring_end;

// If x is of typex, return y else return (typey){O}
#define M_AS_LINKED_TYPE(typex, x, typey, y) _Generic(((void)0,(x)), typex: (y), default: (typey) {0})

// Return IT_TYPE of oplist promoted to a type suitable for a _Generic statement
// i.e. transform array type into corresponding pointer type
#define M_G3N_GEN_IT_TYPE(oplist)  m_typeof( ((void)0, (M_GET_IT_TYPE oplist()){0}))
#define M_G3N_GEN_TYPE(oplist)     M_GET_GENTYPE oplist()

// Translate type' into the container type if type == TYPE, IT_TYPE into its iterator
// KEY_TYPE, VALUE_TYPE, SUBTYPE into the associated type in the oplist,
// or keep 'type' otherwise
// Handle also specially SUBTYPE_PTR
#define M_G3N_TYPE(type, oplist) M_G3N_TYPE_B(type, oplist()) 
#define M_G3N_TYPE_B(type, oplist) M_GET_METHOD(type, type, SUBTYPE_PTR(M_GET_SUBTYPE oplist *), M_OPFLAT oplist)

// Call the OPERATOR call of the oplist registered to the variable 'x'
// which takes one argument.
#define M_G3N_CALL_1(op, x)                                                   \
  M_G3N_BEGIN_PROTECTED_CODE                                                  \
  _Generic( (x),                                                              \
            M_MAP2(M_G3N_CALL_1_func, (op, x) M_G3N_REGISTERED_ITEMS() )      \
            struct m_g3neric_dummy_s *: /* cannot happen */ (void)0)          \
  M_G3N_END_PROTECTED_CODE
#define M_G3N_CALL_1_func(x, oplist)                                          \
  M_G3N_CALL_1_func_test(M_G3N_GEN_TYPE(oplist), M_PAIR_1 x, M_PAIR_2 x, oplist)
#define M_G3N_CALL_1_func_test(gentype, op, x, oplist)                        \
  M_IF_METHOD(op, oplist())(M_G3N_CALL_1_func_expand, M_EAT)(gentype, op, x, oplist)
#define M_G3N_CALL_1_func_expand(gentype, op, x, oplist)                      \
  gentype: M_C(M_CALL_, op)(oplist(), M_AS_TYPE(gentype, x)),                 \
  const   gentype: M_C(M_CALL_, op)(oplist(), M_AS_LINKED_TYPE(const gentype, x, gentype, x)),

// Call the OPERATOR call of the oplist registered to the variable 'svar'
// which takes 2 arguments of the given type
#define M_G3N_CALL_2(op, svar, type1, var1, type2, var2)                      \
  M_G3N_BEGIN_PROTECTED_CODE                                                  \
  _Generic( ((void)0, (svar)),                                                \
            M_MAP2(M_G3N_CALL_2_func, (op, svar, type1, var1, type2, var2) M_G3N_REGISTERED_ITEMS() ) \
            struct m_g3neric_dummy_s *: /* cannot happen */ (void) 0)         \
  M_G3N_END_PROTECTED_CODE
#define M_G3N_CALL_2_func(x, oplist)                                          \
  M_APPLY(M_G3N_CALL_2_func_test, M_G3N_GEN_TYPE(oplist), M_ID x, oplist)
#define M_G3N_CALL_2_func_test(gentype, op, svar, type1, var1, type2, var2, oplist) \
  M_IF_METHOD(op, oplist())(M_G3N_CALL_2_func_expand, M_EAT)(gentype, op, svar, M_G3N_TYPE(type1, oplist), var1, M_G3N_TYPE(type2, oplist), var2, oplist)
#define M_G3N_CALL_2_func_expand(gentype, op, svar, type1, var1, type2, var2, oplist) \
  gentype: M_C(M_CALL_, op)(oplist(), M_AS_LINKED_TYPE(gentype, svar, type1, var1), M_AS_LINKED_TYPE(gentype, svar, type2, var2) ), \
  const gentype: M_C(M_CALL_, op)(oplist(), M_AS_LINKED_TYPE(const gentype, svar, type1, var1), M_AS_LINKED_TYPE(const gentype, svar, type2, var2) ),

// Call the OPERATOR call of the oplist registered to the variable 'svar'
// which takes 3 arguments of the given type
#define M_G3N_CALL_3(op, svar, type1, var1, type2, var2, type3, var3)         \
  M_G3N_BEGIN_PROTECTED_CODE                                                  \
  _Generic( ((void)0, (svar)),                                                \
            M_MAP2(M_G3N_CALL_3_func, (op, svar, type1, var1, type2, var2, type3, var3) M_G3N_REGISTERED_ITEMS() ) \
            struct m_g3neric_dummy_s *: /* cannot happen */ (void) 0)         \
  M_G3N_END_PROTECTED_CODE
#define M_G3N_CALL_3_func(x, oplist)                                          \
  M_APPLY(M_G3N_CALL_3_func_test, M_G3N_GEN_TYPE(oplist), M_ID x, oplist)
#define M_G3N_CALL_3_func_test(gentype, op, svar, type1, var1, type2, var2, type3, var3, oplist) \
  M_IF_METHOD(op, oplist())(M_G3N_CALL_3_func_expand, M_EAT)(gentype, op, svar, M_G3N_TYPE(type1, oplist), var1, M_G3N_TYPE(type2, oplist), var2, M_G3N_TYPE(type3, oplist), var3, oplist)
#define M_G3N_CALL_3_func_expand(gentype, op, svar, type1, var1, type2, var2, oplist) \
  gentype: M_C(M_CALL_, op)(oplist(), M_AS_LINKED_TYPE(gentype, svar, type1, var1), M_AS_LINKED_TYPE(gentype, svar, type2, var2) ), \
  const gentype: M_C(M_CALL_, op)(oplist(), M_AS_LINKED_TYPE(const gentype, svar, type1, var1), M_AS_LINKED_TYPE(const gentype, svar, type2, var2) ),

// Call the OPERATOR call of the oplist registered to the variable 'svar'
// which takes 4 arguments of the given type
#define M_G3N_CALL_4(op, svar, type1, var1, type2, var2, type3, var3, type4, var4) \
  M_G3N_BEGIN_PROTECTED_CODE                                                  \
  _Generic( ((void)0, (svar)),                                                \
            M_MAP2(M_G3N_CALL_4_func, (op, svar, type1, var1, type2, var2, type3, var3, type4, var4) M_G3N_REGISTERED_ITEMS() ) \
            struct m_g3neric_dummy_s *: /* cannot happen */ (void) 0)         \
  M_G3N_END_PROTECTED_CODE
#define M_G3N_CALL_4_func(x, oplist)                                          \
  M_APPLY(M_G3N_CALL_4_func_test, M_G3N_GEN_TYPE(oplist), M_ID x, oplist)
#define M_G3N_CALL_4_func_test(gentype, op, svar, type1, var1, type2, var2, type3, var3, type4, var4, oplist) \
  M_IF_METHOD(op, oplist())(M_G3N_CALL_4_func_expand, M_EAT)(gentype, op, svar, M_G3N_TYPE(type1, oplist), var1, M_G3N_TYPE(type2, oplist), var2, M_G3N_TYPE(type3, oplist), var3, M_G3N_TYPE(type4, oplist), var4, oplist)
#define M_G3N_CALL_4_func_expand(gentype, op, svar, type1, var1, type2, var2, oplist) \
  gentype: M_C(M_CALL_, op)(oplist(), M_AS_LINKED_TYPE(gentype, svar, type1, var1), M_AS_LINKED_TYPE(gentype, svar, type2, var2) ), \
  const gentype: M_C(M_CALL_, op)(oplist(), M_AS_LINKED_TYPE(const gentype, svar, type1, var1), M_AS_LINKED_TYPE(const gentype, svar, type2, var2) ),

// Call the OPERATOR call of the oplist registered to the variable 'x'
// which takes one argument, it_type
#define M_G3N_CALL_1i(op, x)                                                  \
  M_G3N_BEGIN_PROTECTED_CODE                                                  \
  _Generic( (x),                                                              \
            M_MAP2(M_G3N_CALL_1i_func, (op, x) M_G3N_REGISTERED_ITEMS() )     \
            struct m_g3neric_dummy_s *: /* cannot happen */ (void)0)          \
  M_G3N_END_PROTECTED_CODE
#define M_G3N_CALL_1i_func(x, oplist)                                         \
  M_G3N_CALL_1_func_test(M_G3N_GEN_IT_TYPE(oplist), M_PAIR_1 x, M_PAIR_2 x, oplist)

// Call the OPERATOR call of the oplist registered to the variable 'svar'
// which takes 2 arguments of the given it_type
#define M_G3N_CALL_2i(op, svar, type1, var1, type2, var2)                     \
  M_G3N_BEGIN_PROTECTED_CODE                                                  \
  _Generic( ((void)0, (svar)),                                                \
            M_MAP2(M_G3N_CALL_2i_func, (op, svar, type1, var1, type2, var2) M_G3N_REGISTERED_ITEMS() ) \
            struct m_g3neric_dummy_s *: /* cannot happen */ (void) 0)         \
  M_G3N_END_PROTECTED_CODE
#define M_G3N_CALL_2i_func(x, oplist)                                         \
  M_APPLY(M_G3N_CALL_2_func_test, M_G3N_GEN_IT_TYPE(oplist), M_ID x, oplist)

// Create a variable of type OPERATOR call of the oplist registered to the variable 'x'
#define M_G3N_TYPE_1(op, x)                                                   \
  M_G3N_BEGIN_PROTECTED_CODE                                                  \
  _Generic( (x),                                                              \
            M_MAP2(M_G3N_TYPE_1_func, (op, x) M_G3N_REGISTERED_ITEMS() )      \
            struct m_g3neric_dummy_s *: /* cannot happen */ (void)0)          \
  M_G3N_END_PROTECTED_CODE
#define M_G3N_TYPE_1_func(x, oplist)                                          \
  M_G3N_TYPE_1_func_test(M_G3N_GEN_TYPE(oplist), M_PAIR_1 x, M_PAIR_2 x, oplist)
#define M_G3N_TYPE_1_func_test(gentype, op, x, oplist)                        \
  M_IF_METHOD(op, oplist())(M_G3N_TYPE_1_func_expand, M_EAT)(gentype, op, x, oplist)
#define M_G3N_TYPE_1_func_expand(gentype, op, x, oplist)                      \
  gentype: (M_G3N_TYPE(op, oplist)){0},                                       \
  const gentype: (M_G3N_TYPE(op, oplist)){0},


// for each item in the container
// Same as M_EACH except it uses the generic macros
#define M_G3N_EACHI(item, container, l_it, l_pass)                            \
  (bool l_pass = true; l_pass; l_pass = false)                                \
  for(sub_type(container) *item; l_pass ; l_pass = false)                     \
    for(it_type(container) l_it; l_pass ; l_pass = false)                     \
      for(it_first(l_it, container) ;                                         \
          !it_end_p(l_it)                                                     \
            && (item = it_ref(l_it), true) ;                                  \
          it_next(l_it))


// TODO: init_with ? How to handle the different type of parameters ? Using EMPLACE_TYPE ?
// TODO: support of PUSH_EMPLACE ?
// TODO: support of "let" directly to avoid using the post-processing registration of M_LET?

// Overwrite M_FPRINT & M_PRINT to support global oplists registration
#define M_FPRINT_ARG_G3N(f, x)                                                \
  M_G3N_BEGIN_PROTECTED_CODE                                                  \
  _Generic(((void)0,(x)),                                                     \
           char: fprintf(f, "%c", M_AS_TYPE(char,x)),                         \
           bool: fprintf(f, "%d", M_AS_TYPE(bool,x)),                         \
           signed char: fprintf(f, "%hhd", M_AS_TYPE(signed char,x)),         \
           unsigned char: fprintf(f, "%hhu", M_AS_TYPE(unsigned char,x)),     \
           signed short: fprintf(f, "%hd", M_AS_TYPE(signed short,x)),        \
           unsigned short: fprintf(f, "%hu", M_AS_TYPE(unsigned short,x)),    \
           signed int: fprintf(f, "%d", M_AS_TYPE(signed int,x)),             \
           unsigned int: fprintf(f, "%u", M_AS_TYPE(unsigned int,x)),         \
           long int: fprintf(f, "%ld", M_AS_TYPE(long int,x)),                \
           unsigned long int: fprintf(f, "%lu", M_AS_TYPE(unsigned long int,x)), \
           long long int: fprintf(f, "%lld", M_AS_TYPE(long long int,x)),     \
           unsigned long long int: fprintf(f, "%llu", M_AS_TYPE(unsigned long long int,x)), \
           float: fprintf(f, "%f", M_AS_TYPE(float,x)),                       \
           double: fprintf(f, "%f", M_AS_TYPE(double,x)),                     \
           long double: fprintf(f, "%Lf", M_AS_TYPE(long double,x)),          \
           const char *: fprintf(f, "%s", M_AS_TYPE(const char*,x)),          \
           char *: fprintf(f, "%s", M_AS_TYPE(char*,x)),                      \
           const void *: fprintf(f, "%p", M_AS_TYPE(const void *,x)),         \
           void *: fprintf(f, "%p", M_AS_TYPE(void *,x)),                     \
           M_MAP2(M_G3N_CALL_2_func, (OUT_STR, x, m_g3n_file, f, TYPE, x) M_G3N_REGISTERED_ITEMS() ) \
           struct m_g3neric_dummy_s *: /* cannot happen */ (void) 0)          \
  M_G3N_END_PROTECTED_CODE

#define M_G3N_FPRINT_ARG(f, x)                                                \
  M_IF(M_PARENTHESIS_P(x))                                                    \
    ( M_FPRINT_ARG_OUT_STR(f, M_PAIR_2 x, M_PAIR_1 x),                        \
      M_FPRINT_ARG_G3N(f, x) )

/* Override M_PRINT */
#undef M_PRINT
#define M_PRINT(...)   do { M_REDUCE2(M_G3N_FPRINT_ARG, M_SEPARATE_PER_SEMICOLON, stdout, __VA_ARGS__); } while (0)

#undef M_FPRINT
#define M_FPRINT(f,...)  do { M_REDUCE2(M_G3N_FPRINT_ARG, M_SEPARATE_PER_SEMICOLON, f, __VA_ARGS__); } while (0)

// Register CORE component of M*LIB
#define M_GENERIC_ORG_MLIB_COMP_1() (CORE)


#endif
