#pragma once
#include <pgmspace.h>
#include <FS.h>
#include <Arduino.h>

struct EmbeddedIRBinFile {
    const char *filename;
    const uint8_t *data;
    size_t decomp_size;    // decompressed structured blob size
    size_t text_size;      // decompressed .ir text size
    size_t comp_size;      // LZ4 compressed size in PROGMEM
};

const EmbeddedIRBinFile* embedded_ir_bin_lookup(const char *filename);
void embedded_ir_bin_write_all(FS &fs, const String &dir_path);
