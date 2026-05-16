// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** Major version number (X.x.x) */
#define ASYNCTCP_VERSION_MAJOR 3
/** Minor version number (x.X.x) */
#define ASYNCTCP_VERSION_MINOR 4
/** Patch version number (x.x.X) */
#define ASYNCTCP_VERSION_PATCH 10

/**
 * Macro to convert version number into an integer
 *
 * To be used in comparisons, such as ASYNCTCP_VERSION >= ASYNCTCP_VERSION_VAL(2, 0, 0)
 */
#define ASYNCTCP_VERSION_VAL(major, minor, patch) ((major << 16) | (minor << 8) | (patch))

/**
 * Current version, as an integer
 *
 * To be used in comparisons, such as ASYNCTCP_VERSION_NUM >= ASYNCTCP_VERSION_VAL(2, 0, 0)
 */
#define ASYNCTCP_VERSION_NUM ASYNCTCP_VERSION_VAL(ASYNCTCP_VERSION_MAJOR, ASYNCTCP_VERSION_MINOR, ASYNCTCP_VERSION_PATCH)

/**
 * Current version, as string
 */
#define df2xstr(s)       #s
#define df2str(s)        df2xstr(s)
#define ASYNCTCP_VERSION df2str(ASYNCTCP_VERSION_MAJOR) "." df2str(ASYNCTCP_VERSION_MINOR) "." df2str(ASYNCTCP_VERSION_PATCH)

#ifdef __cplusplus
}
#endif
