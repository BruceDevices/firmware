/* Include this instead of protocol/tagtinker_proto.h from C++.
 *
 * The vendored header has no extern "C" guard of its own (and must not be
 * edited), so its non-inline functions would otherwise be name-mangled by the
 * C++ compiler and fail to link against the C-compiled protocol object. */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "protocol/tagtinker_proto.h"

#ifdef __cplusplus
}
#endif
