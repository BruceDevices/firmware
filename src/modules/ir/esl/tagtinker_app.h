/* Compat shim for the vendored TagTinker protocol core.
 *
 * protocol/tagtinker_proto.c includes "../tagtinker_app.h". On the Flipper that
 * is the full app header; here the protocol code only needs COUNT_OF, so this
 * stands in for it and lets the vendored file compile with zero edits. */
#pragma once

#ifndef COUNT_OF
#define COUNT_OF(x) (sizeof(x) / sizeof((x)[0]))
#endif
