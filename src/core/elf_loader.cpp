#include "elf_loader.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <SD.h>
#include <esp_heap_caps.h>
#include <string.h>

// ----------------------------------------------------------------------------
// Minimal ELF32 Definitions (Xtensa 32-bit)
// ----------------------------------------------------------------------------
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Word;
typedef int32_t  Elf32_Sword;
typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;

#define EI_NIDENT (16)

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
} Elf32_Ehdr;

typedef struct {
    Elf32_Word sh_name;
    Elf32_Word sh_type;
    Elf32_Word sh_flags;
    Elf32_Addr sh_addr;
    Elf32_Off  sh_offset;
    Elf32_Word sh_size;
    Elf32_Word sh_link;
    Elf32_Word sh_info;
    Elf32_Word sh_addralign;
    Elf32_Word sh_entsize;
} Elf32_Shdr;

typedef struct {
    Elf32_Addr  r_offset;
    Elf32_Word  r_info;
    Elf32_Sword r_addend;
} Elf32_Rela;

#define ELF32_R_TYPE(i) ((unsigned char)(i))

// ELF magic
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

// ELF types
#define ET_EXEC 2
#define ET_DYN  3

// Xtensa machine ID
#define EM_XTENSA 94

// Xtensa relocation types
#define R_XTENSA_NONE     0
#define R_XTENSA_32       1
#define R_XTENSA_RELATIVE 50

// Section header types
#define SHT_PROGBITS 1
#define SHT_RELA     4
#define SHT_NOBITS   8

// Section header flags
#define SHF_ALLOC     0x2
#define SHF_EXECINSTR 0x4

// ----------------------------------------------------------------------------
// Loader handle
// ----------------------------------------------------------------------------

struct ElfHandle {
    void*  image;       // Single contiguous allocation (IRAM-capable)
    size_t image_size;
    void*  entry_point;
};

// ----------------------------------------------------------------------------
// Helper: open file from LittleFS or SD
// ----------------------------------------------------------------------------
static File open_file(const char* path) {
    File f = LittleFS.open(path, "r");
    if (f) return f;
    return SD.open(path, "r");
}

// ----------------------------------------------------------------------------
// Loader implementation — single contiguous allocation
//
// Strategy:
//   1. Read section headers, find the lowest and highest VMA among SHF_ALLOC sections.
//   2. Allocate one contiguous block with MALLOC_CAP_EXEC (can hold both code and data).
//   3. Load each section at (image_base + (sh_addr - vma_low)).
//   4. All R_XTENSA_RELATIVE relocations become: *patch = addend + image_base.
//      This works because the ELF is linked as PIE with base 0, so addend == vma offset.
// ----------------------------------------------------------------------------

bool elf_load(const char* path, uint32_t offset, ElfHandle** outHandle) {
    if (!path || !outHandle) return false;

    File file = open_file(path);
    if (!file) {
        Serial.printf("[ELF] Cannot open: %s\n", path);
        return false;
    }

    // --- Read ELF header ---
    Elf32_Ehdr ehdr;
    file.seek(offset);
    if (file.read((uint8_t*)&ehdr, sizeof(ehdr)) != sizeof(ehdr)) {
        Serial.println("[ELF] Failed to read header");
        file.close();
        return false;
    }

    // --- Validate ---
    if (ehdr.e_ident[0] != ELFMAG0 || ehdr.e_ident[1] != ELFMAG1 ||
        ehdr.e_ident[2] != ELFMAG2 || ehdr.e_ident[3] != ELFMAG3) {
        Serial.println("[ELF] Bad magic");
        file.close();
        return false;
    }
    if (ehdr.e_machine != EM_XTENSA) {
        Serial.println("[ELF] Not Xtensa");
        file.close();
        return false;
    }
    if (ehdr.e_type != ET_DYN && ehdr.e_type != ET_EXEC) {
        Serial.println("[ELF] Unsupported type (need DYN or EXEC)");
        file.close();
        return false;
    }
    if (ehdr.e_shnum == 0) {
        Serial.println("[ELF] No sections");
        file.close();
        return false;
    }

    // --- Read section headers ---
    size_t shdrs_bytes = ehdr.e_shnum * sizeof(Elf32_Shdr);
    Elf32_Shdr* shdrs = (Elf32_Shdr*)malloc(shdrs_bytes);
    if (!shdrs) {
        Serial.println("[ELF] OOM: section headers");
        file.close();
        return false;
    }
    file.seek(offset + ehdr.e_shoff);
    if (file.read((uint8_t*)shdrs, shdrs_bytes) != shdrs_bytes) {
        Serial.println("[ELF] Failed to read section headers");
        free(shdrs);
        file.close();
        return false;
    }

    // --- Determine VMA range of all ALLOC sections ---
    uint32_t vma_low  = 0xFFFFFFFF;
    uint32_t vma_high = 0;
    for (int i = 0; i < ehdr.e_shnum; i++) {
        if (!(shdrs[i].sh_flags & SHF_ALLOC)) continue;
        uint32_t end = shdrs[i].sh_addr + shdrs[i].sh_size;
        if (shdrs[i].sh_addr < vma_low)  vma_low  = shdrs[i].sh_addr;
        if (end > vma_high)              vma_high = end;
    }

    if (vma_low >= vma_high) {
        Serial.println("[ELF] No loadable sections found");
        free(shdrs);
        file.close();
        return false;
    }

    size_t image_size = vma_high - vma_low;

    // --- Single contiguous allocation with EXEC capability ---
    void* image = heap_caps_malloc(image_size, MALLOC_CAP_EXEC | MALLOC_CAP_8BIT);
    if (!image) {
        Serial.printf("[ELF] OOM: need %u bytes IRAM\n", image_size);
        free(shdrs);
        file.close();
        return false;
    }
    memset(image, 0, image_size);  // zero BSS sections automatically

    uint32_t base = (uint32_t)image;

    // --- Load PROGBITS sections ---
    for (int i = 0; i < ehdr.e_shnum; i++) {
        if (!(shdrs[i].sh_flags & SHF_ALLOC)) continue;
        if (shdrs[i].sh_type != SHT_PROGBITS) continue;  // skip BSS (NOBITS)

        uint32_t dest_offset = shdrs[i].sh_addr - vma_low;
        file.seek(offset + shdrs[i].sh_offset);
        file.read((uint8_t*)image + dest_offset, shdrs[i].sh_size);
    }

    // --- Perform relocations ---
    for (int i = 0; i < ehdr.e_shnum; i++) {
        if (shdrs[i].sh_type != SHT_RELA) continue;
        if (shdrs[i].sh_size == 0) continue;

        // sh_info points to the section being relocated
        uint32_t target_sec = shdrs[i].sh_info;
        if (target_sec >= (uint32_t)ehdr.e_shnum || !(shdrs[target_sec].sh_flags & SHF_ALLOC)) {
            continue;
        }

        file.seek(offset + shdrs[i].sh_offset);
        
        int num_relas = shdrs[i].sh_size / sizeof(Elf32_Rela);
        int relas_processed = 0;
        
        // Process relocations in small chunks to avoid large memory allocations
        #define RELA_CHUNK_SIZE 32
        Elf32_Rela relas_chunk[RELA_CHUNK_SIZE];

        while (relas_processed < num_relas) {
            int chunk_count = num_relas - relas_processed;
            if (chunk_count > RELA_CHUNK_SIZE) {
                chunk_count = RELA_CHUNK_SIZE;
            }

            size_t bytes_to_read = chunk_count * sizeof(Elf32_Rela);
            if (file.read((uint8_t*)relas_chunk, bytes_to_read) != bytes_to_read) {
                Serial.println("[ELF] Failed to read relocation chunk");
                heap_caps_free(image);
                free(shdrs);
                file.close();
                return false;
            }

            for (int r = 0; r < chunk_count; r++) {
                int type = ELF32_R_TYPE(relas_chunk[r].r_info);

                if (type == R_XTENSA_NONE) {
                    continue;
                } else if (type == R_XTENSA_RELATIVE) {
                    // addend is a VMA offset from base 0; adjust to loaded base
                    uint32_t patch_vma = shdrs[target_sec].sh_addr + relas_chunk[r].r_offset;
                    uint32_t* patch_addr = (uint32_t*)(base + (patch_vma - vma_low));
                    *patch_addr = (uint32_t)(base + (relas_chunk[r].r_addend - vma_low));
                } else if (type == R_XTENSA_32) {
                    // Absolute 32-bit relocation: value = symbol + addend
                    // For PIE with no external symbols, symbol value is a VMA
                    uint32_t patch_vma = shdrs[target_sec].sh_addr + relas_chunk[r].r_offset;
                    uint32_t* patch_addr = (uint32_t*)(base + (patch_vma - vma_low));
                    *patch_addr += (uint32_t)(base - vma_low);
                } else {
                    Serial.printf("[ELF] Unsupported relocation type %d at offset 0x%08x\n",
                                  type, relas_chunk[r].r_offset);
                }
            }
            relas_processed += chunk_count;
        }
    }

    // --- Resolve entry point ---
    void* entry_addr = NULL;
    if (ehdr.e_entry >= vma_low && ehdr.e_entry < vma_high) {
        entry_addr = (void*)(base + (ehdr.e_entry - vma_low));
    }

    free(shdrs);
    file.close();

    if (!entry_addr) {
        Serial.println("[ELF] Entry point outside loadable range");
        heap_caps_free(image);
        return false;
    }

    // --- Build handle ---
    ElfHandle* handle = (ElfHandle*)malloc(sizeof(ElfHandle));
    if (!handle) {
        Serial.println("[ELF] OOM: handle");
        heap_caps_free(image);
        return false;
    }
    handle->image       = image;
    handle->image_size  = image_size;
    handle->entry_point = entry_addr;

    *outHandle = handle;
    Serial.printf("[ELF] Loaded %u bytes at %p, entry %p\n", image_size, image, entry_addr);
    return true;
}

void* elf_get_entry(ElfHandle* handle) {
    if (!handle) return NULL;
    return handle->entry_point;
}

void elf_unload(ElfHandle* handle) {
    if (!handle) return;
    if (handle->image) {
        heap_caps_free(handle->image);
    }
    free(handle);
}
