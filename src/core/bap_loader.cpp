#include "bap_loader.h"
#include "elf_loader.h"
#include "bruce_api.h"
#include "display.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <SD.h>

// BapHeader and BAP_MAGIC are now in bap_loader.h

// Architecture detection
#if CONFIG_IDF_TARGET_ESP32S3
#define CURRENT_ARCH 0x02
#elif CONFIG_IDF_TARGET_ESP32
#define CURRENT_ARCH 0x01
#else
#define CURRENT_ARCH 0x01
#endif

// Declared in bruce_api_table.cpp — frees any leaked app allocations
extern "C" void bap_free_tracked_allocs(void);

void launch_bap_app(const char* path) {
    // --- Open and read BAP header ---
    File file = LittleFS.open(path, "r");
    if (!file) {
        file = SD.open(path, "r");
        if (!file) {
            Serial.printf("[BAP] Cannot open: %s\n", path);
            displayError("App not found", true);
            return;
        }
    }

    BapHeader header;
    if (file.read((uint8_t*)&header, sizeof(header)) != sizeof(header)) {
        Serial.println("[BAP] Header too short");
        displayError("Invalid BAP file", true);
        file.close();
        return;
    }
    file.close();

    // BUG-4 fix: force null-terminate name to prevent buffer overread
    header.name[31] = '\0';

    // --- Validate magic ---
    if (memcmp(header.magic, BAP_MAGIC, 4) != 0) {
        Serial.println("[BAP] Bad magic");
        displayError("Not a BAP file", true);
        return;
    }

    // --- Validate architecture ---
    if (header.arch != CURRENT_ARCH) {
        Serial.printf("[BAP] Arch mismatch: file=0x%02X, system=0x%02X\n", header.arch, CURRENT_ARCH);
        displayError("Wrong architecture!", true);
        return;
    }

    // --- Validate API version ---
    // (header.version is the BAP format version; future use for compatibility checks)

    // --- Load ELF ---
    uint32_t heap_before = ESP.getFreeHeap();
    Serial.printf("[BAP] Loading '%s' (heap: %u bytes free)\n", header.name, heap_before);

    ElfHandle* handle = NULL;
    uint32_t elf_offset = sizeof(BapHeader);

    if (!elf_load(path, elf_offset, &handle)) {
        Serial.println("[BAP] ELF load failed");
        displayError("Load failed", true);
        return;
    }

    void* entry = elf_get_entry(handle);
    if (!entry) {
        Serial.println("[BAP] No entry point");
        displayError("No entry point", true);
        elf_unload(handle);
        return;
    }

    // --- Launch app synchronously ---
    // Running in the calling task avoids race conditions (BUG-5) and prevents
    // the menu from drawing over the app's screen (IMP-4).
    typedef void (*app_main_t)(BruceAPI*);
    app_main_t app_main = (app_main_t)entry;

    displaySuccess("Launching: " + String(header.name));
    delay(400);

    app_main(get_bruce_api());

    // --- Cleanup ---
    bap_free_tracked_allocs();  // GC any leaked app allocations
    elf_unload(handle);

    uint32_t heap_after = ESP.getFreeHeap();
    int32_t heap_diff = (int32_t)heap_after - (int32_t)heap_before;
    Serial.printf("[BAP] '%s' exited. Heap: %u bytes free (%+d)\n", header.name, heap_after, heap_diff);

    if (heap_diff < -256) {
        Serial.printf("[BAP] WARNING: possible memory leak of %d bytes\n", -heap_diff);
    }
}
