#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle for the loaded ELF image
typedef struct ElfHandle ElfHandle;

/**
 * Load an ELF (shared/PIE) file from the LittleFS/SD and prepare it for execution.
 *
 * The loader allocates a single contiguous block via heap_caps_malloc(MALLOC_CAP_EXEC)
 * and performs Xtensa-specific relocations so the code can execute from any address.
 *
 * @param path          Absolute path to the .bruce file on the filesystem.
 * @param offset        Byte offset where the ELF data begins (after BAP header).
 * @param outHandle     On success, receives a pointer to an ElfHandle. Caller must free via elf_unload().
 * @return true on success, false on error.
 */
bool elf_load(const char* path, uint32_t offset, ElfHandle** outHandle);

/**
 * Get the entry point function pointer from a loaded ELF.
 * The returned pointer should be cast to: void (*)(BruceAPI*)
 */
void* elf_get_entry(ElfHandle* handle);

/**
 * Unload a previously loaded ELF image, freeing all allocated memory.
 */
void elf_unload(ElfHandle* handle);

#ifdef __cplusplus
}
#endif
