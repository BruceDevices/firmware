/**
 *
 * @license MIT License
 *
 * Copyright (c) 2024 lewis he
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @file      XPowersLib_Version.h
 * @author    Lewis He (lewishe@outlook.com)
 * @date      2024-12-12
 *
 */

#pragma once

/** Major version number (X.x.x) */
#define XPOWERSLIB_VERSION_MAJOR   0
/** Minor version number (x.X.x) */
#define XPOWERSLIB_VERSION_MINOR   3
/** Patch version number (x.x.X) */
#define XPOWERSLIB_VERSION_PATCH   0

/**
 * Macro to convert XPowersLib version number into an integer
 *
 * To be used in comparisons, such as XPOWERSLIB_VERSION >= XPOWERSLIB_VERSION_VAL(2, 0, 0)
 */
#define XPOWERSLIB_VERSION_VAL(major, minor, patch) ((major << 16) | (minor << 8) | (patch))

/**
 * Current XPowersLib version, as an integer
 *
 * To be used in comparisons, such as XPOWERSLIB_VERSION >= XPOWERSLIB_VERSION_VAL(2, 0, 0)
 */
#define XPOWERSLIB_VERSION  XPOWERSLIB_VERSION_VAL(XPOWERSLIB_VERSION_MAJOR, \
                                             XPOWERSLIB_VERSION_MINOR, \
                                             XPOWERSLIB_VERSION_PATCH)
