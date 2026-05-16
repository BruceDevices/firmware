// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

#include <ChunkPrint.h>

size_t ChunkPrint::write(uint8_t c) {
  // handle case where len is zero
  if (!_len) {
    return 0;
  }
  // skip first bytes until from is zero (bytes were already sent by previous chunk)
  if (_from) {
    _from--;
    return 1;
  }
  // write a maximum of len bytes
  if (_len - _index) {
    _destination[_index++] = c;
    return 1;
  }
  // we have finished writing len bytes, ignore the rest
  return 0;
}
