#pragma once
#include <pgmspace.h>
#include <FS.h>
#include <Arduino.h>

struct EmbeddedIRFile {
    const char *filename;
    const char *data;
    size_t length;
};

const EmbeddedIRFile* embedded_ir_lookup(const char *filename);
void embedded_ir_write_all(FS &fs, const String &dir_path);
