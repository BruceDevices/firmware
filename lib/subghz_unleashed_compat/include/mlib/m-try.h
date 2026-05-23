/*
 * M*LIB - try / catch mechanism for M*LIB
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
#ifndef MSTARLIB_TRY_H
#define MSTARLIB_TRY_H

#include "m-core.h"
#include "m-thread.h"

/*
 * Select mechanism to use for support of RAII and exception,
 * so that for each variable defined using M_LET,
 * its destructor is still called when exceptions are thrown.
 * It is either the C++ try,
 * or it uses a GCC extension,
 * or it uses a CLANG extension,
 * or the standard C compliant way through longjmp (slower).
 * The user can override the desired mechanism.
 * 
 * Performance Measures of the mechanism costs to create recursively 100000 empty strings (RAII / no exception)
 *  TCC        : LET(longjmp): 6412 us ON_EXCEPTION(longjmp): 6550 us
 *  CLANG      : LET(longjmp): 3922 us ON_EXCEPTION(longjmp): 3972 us
 *  CLANG+BLOCK: LET(BLOCK):   3549 us ON_EXCEPTION(longjmp): 4196 us
 *  GCC (O2)   : LET(nested):  1828 us ON_EXCEPTION(longjmp): 2033 us
 *  G++ (O2/O3): LET(c++):     1850 us ON_EXCEPTION(c++):     1874 us
 *  GCC (O3)   : LET(nested):  1377 us ON_EXCEPTION(longjmp): 1402 us
 *  CLANG++(O2/O3): LET(c++):   428 us ON_EXCEPTION(c++):      433 us
 * Therefore the C mechanism of try emulation is 3x times slower than the C++ "zero cost" implementation
 * in this scenario where the mechanism cost is the most measurable.
 * Most of the time difference comes from the size of the stack frame (memory bandwidth):
 * * 240 bytes for CLANG(block/longjmp)
 * * 80 bytes for GCC/nested
 * * 96 bytes for GCC/longjmp
 * * 64 bytes for G++ (!)
 * * 16 bytes for CLANG++
 */
#ifndef M_USE_TRY_MECHANISM
# if defined(__has_extension)
#  if __has_extension(blocks)
#   define M_TRY_CLANG_BLOCKS
#  endif
# endif
# if defined(__cplusplus)
#  define M_USE_TRY_MECHANISM 1
# elif defined(M_TRY_CLANG_BLOCKS)
#  define M_USE_TRY_MECHANISM 2
# elif defined(__GNUC__) && !defined(__clang__)
#  define M_USE_TRY_MECHANISM 3
# else
#  define M_USE_TRY_MECHANISM 4
# endif
#endif


/*
 * Start a protected section of code 'name' where all exceptions are caught
 * by the associated CATCH section.
 */
#define M_TRY(name)                                                           \
  M_TRY_B( M_C(m_try_bool_, name), M_C(m_try_buf_, name), name)


/*
 * Catch an exception associated to the TRY block 'name' that matches the given error_code
 * If error_code is 0, it catches all error codes.
 * error code shall be a constant positive integer.
 */
#define M_CATCH(name, error_code) M_CATCH_B(name, error_code)


/*
 * Throw an exception to the upper try block
 * error_code shall be the first argument. 
 * Other arguments are integers or pointers stored in the exception.
 * error code shall be a constant positive integer.
 * There is no genericity of the exception data structure itself.
 */
#define M_THROW(...) do {                                                     \
    M_STATIC_ASSERT(M_RET_ARG1 (__VA_ARGS__) != 0,                            \
                    M_LIB_NOT_A_CONSTANT_NON_NULL_INTEGER,                    \
                    "The error code shall be a non null positive constant");  \
    M_STATIC_ASSERT(M_NARGS (__VA_ARGS__) <= 1+M_USE_MAX_CONTEXT,             \
                    M_LIB_TOO_MANY_ARGUMENTS,                                 \
                    "There are too many arguments for an exception.");        \
    M_IF_NARGS_EQ1(__VA_ARGS__)(M_THROW_1, M_THROW_N)(__VA_ARGS__);           \
  } while (0)


/*
 * Size of the context data that are stored in an exception data structure.
 */
#ifndef M_USE_MAX_CONTEXT
#define M_USE_MAX_CONTEXT 10
#endif

/*
 * The exception itself.
 *
 * It is POD data where every fields can be used by the user.
 * It has been decided to have only one exception data structure
 * to simplify error code and because :
 * - using generic types is much harder in C to do (still possible)
 * - it will make exceptions more usable for errors which should not 
 * be handled by exceptions.
 *
 * For C++, we need to encapsulate it in a template,
 * so that it can be a unique type for each error code,
 * which is needed for the catch mechanism.
 * We all need to override the operator -> since the C++
 * throw the type and catch the type, whereas the C back-end
 * throw the type and catch a pointer to the type:
 * within the catch block you are supposed to use the arrow
 * operator to test the content of the exception.
 */
#if M_USE_TRY_MECHANISM == 1
namespace m_lib {
template <unsigned int N>
#endif
struct m_exception_s {
  unsigned   error_code;                   // Error code
  unsigned short line;                     // Line number where the error was detected
  unsigned short num;                      // Number of entries in 'context' table
  const char *filename;                    // filename  where the error was detected
  intptr_t    context[M_USE_MAX_CONTEXT];  // Specific context of the exception
#ifdef __cplusplus
  m_exception_s<N> *operator->() { return this; }
#endif
};
#if M_USE_TRY_MECHANISM == 1
}
#endif

// Typical Error codes (TODO: add more classic?)
#define M_ERROR_MEMORY  1
#define M_ERROR_ACCESS  2
#define M_ERROR_BUSY    3

/*
 * Define all global needed by the try mechanism with a
 * thread attribute. It needs to be defined once in all the program
 */
#define M_TRY_DEF_ONCE() M_TRY_DEF_ONCE_B()


/*
 * Re-throw the last exception
 * It shall be done in a CATCH block.
 */
#define M_RETHROW() m_rethrow()


/*****************************************************************************/
/********************************** INTERNAL *********************************/
/*****************************************************************************/

#undef M_IF_EXCEPTION
#define M_IF_EXCEPTION(...) __VA_ARGS__

#undef M_ON_EXCEPTION
#define M_ON_EXCEPTION(...)                                                   \
  for(bool cont = true; cont; cont = false)                                   \
    M_DEFER_TRY_INJECT_PRE(cont, __VA_ARGS__)                                 \
      for( ; cont ; cont = false)                                             \
        M_DEFER_TRY_INJECT_POST(cont, __VA_ARGS__)                            \
          for( ; cont; cont = false)

/*
 * Define the C++ back-end.
 * It is fully different from C back-end as it reuses the classic try of the C++.
 * Surprisingly it has more constraints than the C one.
 * error_code shall be a positive, constant integer.
 * the catch all block shall always be the last block.
 * at least catch block is mandatory for each try block.
 * Note that theses constraints are meaningless in real code,
 * and simply good behavior.
 * Notice also that you won't have any access to the exception for a catch all error.
 */
#if M_USE_TRY_MECHANISM == 1

// Define the CATCH block. If error_code is 0, it shall catch all errors.
// NOTE: It will even catch non M*LIB errors.
#define M_CATCH_B(name, error_code)                                           \
  M_IF(M_BOOL(error_code))                                                    \
  (catch (m_lib::m_exception_s<error_code> &name), catch (...))

// No global to define in C++
#define M_TRY_DEF_ONCE_B()   /* Nothing to do */

// Reuse the try keyword of the C++
#define M_TRY_B(cont, buf, exception)                                         \
  try

// Reuse the throw keyword of the C++
// by throwing the type m_lib::m_exception_s<error_code>
#define M_THROW_1(error_code)                                                 \
  throw m_lib::m_exception_s<error_code>{ error_code, __LINE__, 0, __FILE__, { 0 } }

// Reuse the throw keyword of the C++
// by throwing the type m_lib::m_exception_s<error_code>
#define M_THROW_N(error_code, ...)                                            \
  throw m_lib::m_exception_s<error_code>{ error_code, __LINE__,               \
      M_NARGS(__VA_ARGS__), __FILE__, { __VA_ARGS__ } }

// Nothing to inject for a pre initialization of a M*LIB object
#define M_LET_TRY_INJECT_PRE_B(cont, oplist, name) /* Nothing to do */

// Code to inject for a post initialization of a M*LIB object
// We create a C++ object with a destructor that will call the CLEAR operator of the M*LIB object
// by using a lambda function.
// If the CLEAR operator is called naturally, we disable the destructor of the C++ object.
#define M_LET_TRY_INJECT_POST_B(cont, oplist, name)                           \
  for(m_lib::m_regclear M_C(m_try_regclear_, name){[&](void) { M_CALL_CLEAR(oplist, name); } } \
        ; cont ; cont = false, M_C(m_try_regclear_, name).disable() )

// M_DEFER Injection / pre initialization
#define M_DEFER_TRY_INJECT_PRE_B(cont, ...) /* Nothing to do */

// M_DEFER Injection / post initialization
// Register the stack frame and tests for the longjmp.
// In which case call the 'clear' operations (...), unstack the error list and rethrow the error.
#define M_DEFER_TRY_INJECT_POST_B(cont, ...)                                  \
  for(m_lib::m_regclear M_C(m_try_regclear_, cont){[&](void) { __VA_ARGS__; } } \
        ; cont ; M_C(m_try_regclear_, cont).disable() )

// Definition of the C++ object wrapper
// The registered function is called by the destructor,
// except if the disable function has been called.
#include <functional>
namespace m_lib {
  class m_regclear {
    std::function<void(void)> function;
    bool done;
  public:
    inline m_regclear(const std::function<void(void)> &f) : function{f}, done{false} { }
    inline void disable(void) { done = true; }
    inline ~m_regclear() { if (done == false) { function(); done = true; } }
  };
}

// Rethrow is simply throw without any argument 
#define m_rethrow() throw


/*****************************************************************************/

/* The C back-end. 
 * It is fully different from the C++ back-end and is based on setjmp/lonjmp
 * (classic implementation).
 * The main difficulty is the mechanism to register the CLEAR operators
 * to call when throwing an exception.
 * Contrary to the C++ back-end, it is not cost-free as it adds some 
 * instructions to the normal behavior of the program, usually 5 store instructions.
 */
#else

#if (M_USE_TRY_MECHANISM == 3)
// Use of builtin setjmp / longjmp for GCC
// There are at least twice faster at worst, and reduce stack consumption
// See https://gcc.gnu.org/onlinedocs/gcc/Nonlocal-Gotos.html
// CLANG doesn't support these builtins officially (https://groups.google.com/g/llvm-dev/c/9QgfdW23K8M)
#define m_try_setjmp(x)     __builtin_setjmp(x)
#define m_try_longjmp(x,v)  __builtin_longjmp(x, v)
typedef intptr_t           m_try_jmp_buf[5];
#define m_try_jmp_buf      m_try_jmp_buf
#else
// C compliant setjmp
#include <setjmp.h>
#define m_try_setjmp(x)     setjmp(x)
#define m_try_longjmp(x,v)  longjmp(x, v)
#define m_try_jmp_buf       jmp_buf
#endif

// Define the CATCH block associated to the 'name' TRY to catch the exception
// associated to 'error_code' and provide 'name' as a pointer to the exception
// if the exception matches the error code.
// If error code is 0, it matches all errors.
#define M_CATCH_B(name, error_code)                                           \
  else if (m_catch( M_C(m_try_buf_, name), (error_code), &name))

// Define the operator to define nested functions (GCC) or blocks (CLANG)
#if M_USE_TRY_MECHANISM == 2
# define M_TRY_FUNC_OPERATOR ^
#else
# define M_TRY_FUNC_OPERATOR *
#endif

// Define the linked structure used to identify what is present in the C stack.
// We create for each M_TRY and each M_LET a new node in the stack that represents
// this point in the stack frame. Each nodes are linked together, so that we can
// analyze the stack frame on exception.
typedef struct m_try_s {
  enum { M_STATE_TRY,                       // Try block starts
         M_STATE_EXCEPTION_IN_PROGRESS,     // A try block or jmpbuf is being in progress (before being caught)
         M_STATE_EXCEPTION_CAUGHT,          // An exception in progress has been successfully caught (==> no rethrow)
         M_STATE_CLEAR_JMPBUF,              // Need to jmpbuf here to clear things
         M_STATE_CLEAR_CB                   // Need to call the function to clear things
      } kind;
  struct m_try_s *next;                     // Next try block or clear mechanism on the stack frame.
  union {
    m_try_jmp_buf buffer;                   // To use for M_STATE_CLEAR_JMPBUF
    struct { void (M_TRY_FUNC_OPERATOR func)(void*); void *data; } clear; // To use for M_STATE_CLEAR_CB
  } data;
} m_try_t[1];

// Define the TRY block.
// Classic usage of the for trick to push destructor on the exit path.
#define M_TRY_B(cont, buf, exception)                                         \
 for(bool cont = true ; cont ; cont = false)                                  \
   for(m_try_t buf ; cont ; m_try_clear(buf), cont = false )                  \
     for(const struct m_exception_s *exception = (m_try_init(buf), NULL); cont; cont = false, exception = exception) \
       if (m_try_setjmp((buf->data.buffer)) == 0)

// Throw the error code
#define M_THROW_1(error_code)                                                 \
  m_throw( &(const struct m_exception_s) { error_code, __LINE__, 0, __FILE__, { 0 } } )

// Throw the error code and the associated data.
#define M_THROW_N(error_code, ...)                                            \
  m_throw( &(const struct m_exception_s) { error_code, __LINE__, M_NARGS(__VA_ARGS__), __FILE__, \
        { __VA_ARGS__ } } )

// Copy an exception to another.
M_INLINE void
m_exception_set(struct m_exception_s *out, const struct m_exception_s *in)
{
  if (in != out) {
    memcpy(out, in, sizeof *out);
  }
}

// The global thread attribute variables and functions.
extern M_THREAD_ATTR struct m_try_s *m_global_error_list;
extern M_THREAD_ATTR struct m_exception_s m_global_exception;
extern M_ATTR_NO_RETURN M_ATTR_COLD_FUNCTION void m_throw(const struct m_exception_s *exception);

// Macro to add once in one source file to define theses global:
#define M_TRY_DEF_ONCE_B()                                                    \
  /* Define the global variables */                                           \
  M_THREAD_ATTR struct m_try_s *m_global_error_list;                          \
  M_THREAD_ATTR struct m_exception_s m_global_exception;                      \
                                                                              \
  /* Throw the given exception                                                \
     This function should be rarely called. */                                \
  M_ATTR_NO_RETURN M_ATTR_COLD_FUNCTION void                                  \
  m_throw(const struct m_exception_s *exception)                              \
  {                                                                           \
    /* Analyze the error list to see what has been registered */              \
    struct m_try_s *e = m_global_error_list;                                  \
    while (e != NULL) {                                                       \
      /* A CLEAR by CB operator has been registered: call it */               \
      if (e->kind == M_STATE_CLEAR_CB) {                                      \
        e->data.clear.func(e->data.clear.data);                               \
      }                                                                       \
      else {                                                                  \
        /* A JUMP command has been registered.                                \
         * Either due to the M_TRY block or                                   \
         * because of the jump to the CLEAR operator of the object to clear. */ \
        M_ASSERT(e->kind == M_STATE_TRY || e->kind == M_STATE_CLEAR_JMPBUF);  \
        /* If the exception is already m_global_exception, it won't be copied */ \
        m_exception_set(&m_global_exception, exception);                      \
        e->kind = M_STATE_EXCEPTION_IN_PROGRESS;                              \
        m_global_error_list = e;                                              \
        m_try_longjmp(e->data.buffer, 1);                                     \
      }                                                                       \
      /* Next stack frame */                                                  \
      e = e->next;                                                            \
    }                                                                         \
    /* No try block found.                                                    \
       Display the information and halt program . */                          \
    M_RAISE_FATAL("Exception '%u' raised by (%s:%d) is not caught. Program aborted.\n", \
                  exception->error_code, exception->filename, exception->line); \
  }

// Rethrow the error
M_INLINE void
m_rethrow(void)
{
  M_ASSERT(m_global_error_list != NULL);
  m_throw(&m_global_exception);
}

// Catch the error code associated to the TRY block state
// and provide a pointer to the exception (which is a global).
M_INLINE bool
m_catch(m_try_t state, unsigned error_code, const struct m_exception_s **exception)
{
  M_ASSERT(m_global_error_list == state);
  M_ASSERT(state->kind == M_STATE_EXCEPTION_IN_PROGRESS);
  *exception = &m_global_exception;
  // If error_code is 0, we want to catch **all** errors.
  // Otherwise we want to cache only the given error
  // Return false if we don't catcht it.
  if (error_code != 0 && m_global_exception.error_code != error_code)
    return false;
  // The exception has been caught.
  state->kind = M_STATE_EXCEPTION_CAUGHT;
  // Unstack the try block, so that next throw command in the CATCH block
  // will reach the upper TRY block.
  m_global_error_list = state->next;
  return true;
}

#if defined(__GNUC__)
#if __GNUC__ >= 12
/* Warnings disabled for GNU C in C mode (Affecting a global variable to the address of a local variable) */
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wdangling-pointer\"")
#endif
#endif

// Initialize the state to a TRY state.
M_INLINE void
m_try_init(m_try_t state)
{
  state->kind = M_STATE_TRY;
  state->next = m_global_error_list;
  m_global_error_list = state;
  // setjmp cannot be done here
}

// Disable the current TRY block.
M_INLINE void
m_try_clear(m_try_t state)
{
  // Even if there is a CATCH block and an unstack of the exception
  // m_global_error_list won't be changed.
  m_global_error_list = state->next;
  if (M_UNLIKELY (state->kind == M_STATE_EXCEPTION_IN_PROGRESS)) {
    // There was no catch for this error.
    // Forward it to the upper level.
     m_rethrow();
  }
}


// Implement the M_LET injection macros, so that the CLEAR operator is called on exception
// Helper functions
// Each mechanism provide 3 helper functions:
// * pre: which is called before the constructor
// * post: which is called after the constructor
// * final: which is called before the destructor.

// There is two mechanims: calling a CLEAR callback (faster but need compiler extension)
// or using an intermediary jmpbuf_t longjmp (slower but standard compliant).

// We register a call to the CLEAR callback.
// We don't modify m_global_error_list until we have successfully called the INIT operator
// to avoid registering the CLEAR operator on exception whereas the object is not initialized yet.
// However we register the position in the stack frame now so that in case of partial initialization
// of the object (if the INIT operator of the object calls other INIT operators of composed fields),
// since partial initialization will be unstacked naturally by the composing object.
M_INLINE bool
m_try_cb_pre(m_try_t state)
{
  state->kind = M_STATE_CLEAR_CB;
  state->next = m_global_error_list;
  return true;
}

// We register the function to call of the initialized object.
M_INLINE bool
m_try_cb_post(m_try_t state, void (M_TRY_FUNC_OPERATOR func)(void*), void *data)
{
  state->data.clear.func = func;
  state->data.clear.data = data;
  m_global_error_list = state;
  return true;
}

// The object will be cleared.
// We can pop the stack frame of the errors.
M_INLINE void
m_try_cb_final(m_try_t state)
{
  m_global_error_list = state->next;
}

// We register a longjmp here to handle the cleaning of the exception at this function.
// Pre initialization function.
M_INLINE bool
m_try_jump_pre(m_try_t state)
{
  state->kind = M_STATE_CLEAR_JMPBUF;
  state->next = m_global_error_list;
  return true;
}

// Post initialization function. Save the stack frame for a longjmp
M_INLINE void
m_try_jump_post(m_try_t state)
{
  M_ASSERT(state->next == m_global_error_list);
  m_global_error_list = state;
}

// The object will be cleared.
// We can pop the stack frame of the errors.
M_INLINE void
m_try_jump_final(m_try_t state)
{
  m_global_error_list = state->next;
}

#if defined(__GNUC__)
#if __GNUC__ >= 12
_Pragma("GCC diagnostic pop")
#endif
#endif

// Implement the M_LET injection macros, so that the CLEAR operator is called on exception
//
#if M_USE_TRY_MECHANISM == 1
# error M*LIB: Internal error. C++ back-end requested within C implementation.

#elif M_USE_TRY_MECHANISM == 2
// Use of CLANG blocks, and register them as callback with a pointer to the variable

#define M_LET_TRY_INJECT_PRE_B(cont, oplist, name)                            \
  for(m_try_t M_C(m_try_state_, name); cont &&                                \
        m_try_cb_pre(M_C(m_try_state_, name) ); cont = false )

#define M_LET_TRY_INJECT_POST_B(cont, oplist, name)                           \
  for(m_try_cb_post(M_C(m_try_state_, name),                                  \
                      ^ void (void *_data) { M_GET_TYPE oplist *_t = _data; M_CALL_CLEAR(oplist, *_t); }, \
                      (void*) &name); cont; cont = false, m_try_cb_final(M_C(m_try_state_, name)) )

#elif M_USE_TRY_MECHANISM == 3
// Use of GCC nested functions, and register them as callback with a pointer to the variable

#define M_LET_TRY_INJECT_PRE_B(cont, oplist, name)                            \
  for(m_try_t M_C(m_try_state_, name); cont &&                                \
        m_try_cb_pre(M_C(m_try_state_, name) ); cont = false )

#define M_LET_TRY_INJECT_POST_B(cont, oplist, name)                           \
  for(m_try_cb_post(M_C(m_try_state_, name),                                  \
                      __extension__ ({ __extension__ void _callback (void *_data) { M_GET_TYPE oplist *_t = _data; M_CALL_CLEAR(oplist, *_t); } _callback; }), \
                      (void*) &name); cont; cont = false, m_try_cb_final(M_C(m_try_state_, name)) )

#elif M_USE_TRY_MECHANISM == 4
// STD C compliant (without compiler extension): use of setjmp
// This is the basic implementation in case of unknown compiler.
// It uses setjmp/longjmp, and as such, is much slower than
// other implementations.

// M_LET Injection / pre initialization
// Initialize the stack frame.
#define M_LET_TRY_INJECT_PRE_B(cont, oplist, name)                            \
  for(m_try_t M_C(m_try_state_, name); cont &&                                \
        m_try_jump_pre(M_C(m_try_state_, name)); cont = false)

// M_LET Injection / post initialization
// Register the stack frame and tests for the longjmp.
// In which case call the CLEAR operator, unstack the error list and rethrow the error.
#define M_LET_TRY_INJECT_POST_B(cont, oplist, name)                           \
  for(m_try_jump_post(M_C(m_try_state_, name)); cont ; cont = false, m_try_jump_final(M_C(m_try_state_, name))) \
    if (m_try_setjmp(M_C(m_try_state_, name)->data.buffer) != 0) { M_CALL_CLEAR(oplist, name); m_try_jump_final(M_C(m_try_state_, name)); m_rethrow(); } else

#else
# error M*LIB: Invalid value for M_USE_TRY_MECHANISM [1..4]
#endif


// M_DEFER Injection / pre initialization
// Initialize the stack frame.
#define M_DEFER_TRY_INJECT_PRE_B(cont, ...)                                   \
  for(m_try_t M_C(m_try_state_, cont); cont &&                                \
        m_try_jump_pre(M_C(m_try_state_, cont)); )

// M_DEFER Injection / post initialization
// Register the stack frame and tests for the longjmp.
// In which case call the CLEAR operator, unstack the error list and rethrow the error.
#define M_DEFER_TRY_INJECT_POST_B(cont, ...)                                  \
  for(m_try_jump_post(M_C(m_try_state_, cont)); cont ; m_try_jump_final(M_C(m_try_state_, cont))) \
  if (m_try_setjmp(M_C(m_try_state_, cont)->data.buffer) != 0) { __VA_ARGS__; m_try_jump_final(M_C(m_try_state_, cont)); m_rethrow(); } else

#endif /* cplusplus */

/*****************************************************************************/

// Macro injection for M_LET.
// If the oplist defined NOCLEAR property, we won't register this variable for clear on exception
#undef  M_LET_TRY_INJECT_PRE
#define M_LET_TRY_INJECT_PRE(cont, oplist, name)                              \
  M_IF(M_GET_PROPERTY(oplist, NOCLEAR))(M_EAT, M_LET_TRY_INJECT_PRE_B)        \
  (cont, oplist, name)

#undef  M_LET_TRY_INJECT_POST
#define M_LET_TRY_INJECT_POST(cont, oplist, name)                             \
  M_IF(M_GET_PROPERTY(oplist, NOCLEAR))(M_EAT, M_LET_TRY_INJECT_POST_B)       \
  (cont, oplist, name)


// Macro injection for M_DEFER.
#undef  M_DEFER_TRY_INJECT_PRE
#define M_DEFER_TRY_INJECT_PRE(cont, ...)  M_DEFER_TRY_INJECT_PRE_B(cont, __VA_ARGS__)
#undef  M_DEFER_TRY_INJECT_POST
#define M_DEFER_TRY_INJECT_POST(cont, ...) M_DEFER_TRY_INJECT_POST_B(cont, __VA_ARGS__)


// In case of MEMORY FULL errors, throw an error instead of aborting.
#undef  M_MEMORY_FULL
#define M_MEMORY_FULL(type, size)    M_THROW(M_ERROR_MEMORY, (intptr_t)sizeof(type), (intptr_t)(size))

// Add attribute volatile if exceptions are enabled
#undef  m_volatile
#define m_volatile volatile

#endif
