// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** Major version number (X.x.x) */
#define ASYNCWEBSERVER_VERSION_MAJOR 3
/** Minor version number (x.X.x) */
#define ASYNCWEBSERVER_VERSION_MINOR 11
/** Patch version number (x.x.X) */
#define ASYNCWEBSERVER_VERSION_PATCH 0

/**
 * Macro to convert version number into an integer
 *
 * To be used in comparisons, such as ASYNCWEBSERVER_VERSION >= ASYNCWEBSERVER_VERSION_VAL(2, 0, 0)
 */
#define ASYNCWEBSERVER_VERSION_VAL(major, minor, patch) ((major << 16) | (minor << 8) | (patch))

/**
 * Current version, as an integer
 *
 * To be used in comparisons, such as ASYNCWEBSERVER_VERSION_NUM >= ASYNCWEBSERVER_VERSION_VAL(2, 0, 0)
 */
#define ASYNCWEBSERVER_VERSION_NUM ASYNCWEBSERVER_VERSION_VAL(ASYNCWEBSERVER_VERSION_MAJOR, ASYNCWEBSERVER_VERSION_MINOR, ASYNCWEBSERVER_VERSION_PATCH)

/**
 * Current version, as string
 */
#define df2xstr(s)             #s
#define df2str(s)              df2xstr(s)
#define ASYNCWEBSERVER_VERSION df2str(ASYNCWEBSERVER_VERSION_MAJOR) "." df2str(ASYNCWEBSERVER_VERSION_MINOR) "." df2str(ASYNCWEBSERVER_VERSION_PATCH)

#ifdef __cplusplus
}
#endif
