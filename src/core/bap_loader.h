#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define BAP_MAGIC "BAP1"

#pragma pack(push, 1)
struct BapHeader {
    char magic[4];        // "BAP1"
    uint8_t arch;         // 0x01 = ESP32, 0x02 = ESP32-S3
    uint8_t version;      // BAP format version
    char name[32];        // App name (null-terminated)
    uint32_t elf_size;    // Size of the ELF payload in bytes
};
#pragma pack(pop)

void launch_bap_app(const char* path);

#ifdef __cplusplus
}
#endif
