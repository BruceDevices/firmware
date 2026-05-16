// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2026 Hristo Gochkov, Mathieu Carbou, Emil Muratov, Will Miles

#pragma once

#include <Print.h>

class ChunkPrint : public Print {
private:
  uint8_t *_destination;
  size_t _from;
  size_t _len;
  size_t _index;

public:
  ChunkPrint(uint8_t *destination, size_t from, size_t len) : _destination(destination), _from(from), _len(len), _index(0) {}
  size_t write(uint8_t c);
  size_t write(const uint8_t *buffer, size_t size) {
    return this->Print::write(buffer, size);
  }
  size_t written() const {
    return _index;
  }
};
