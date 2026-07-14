# System Programming

Understanding the internals of Bruce firmware.

---

## Architecture Overview

Bruce firmware runs on ESP32 / ESP32-S3 microcontrollers. The system uses **FreeRTOS** as the underlying RTOS, with **Arduino** as the HAL framework and **TFT_eSPI** for display rendering.

```
┌──────────────────────────────────────────────────────────┐
│                     BRUCE FIRMWARE                       │
│                                                          │
│  ┌────────────┐  ┌────────────┐  ┌──────────────────┐   │
│  │   Menu     │  │  Settings  │  │  Built-in Modules │   │
│  │  System    │  │   Core     │  │  (WiFi, BLE, IR)  │   │
│  └─────┬──────┘  └────────────┘  └──────────────────┘   │
│        │                                                 │
│  ┌─────▼──────────────────────────────────────────────┐  │
│  │              App Loader Subsystem                  │  │
│  │                                                    │  │
│  │  ┌───────────┐  ┌──────────┐  ┌────────────────┐  │  │
│  │  │ BAP Loader│  │ELF Loader│  │ Memory Manager │  │  │
│  │  │(bap_loader│  │(elf_load)│  │ (Quota + GC)   │  │  │
│  │  └───────────┘  └──────────┘  └────────────────┘  │  │
│  │                                                    │  │
│  │  ┌────────────────────────────────────────────┐    │  │
│  │  │         BruceAPI Jump Table                │    │  │
│  │  │  (bruce_api_table.cpp — singleton struct)  │    │  │
│  │  └────────────────────────────────────────────┘    │  │
│  └────────────────────────────────────────────────────┘  │
│                                                          │
│  ┌────────────────────────────────────────────────────┐  │
│  │                 Hardware Abstraction               │  │
│  │   TFT_eSPI  │  SD/LittleFS  │  WiFi  │  GPIO      │  │
│  └────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────┘
```

## App Loading Pipeline

When a user selects an app from the Apps menu, the following happens:

### Step 1: Read BAP Header

The firmware opens the `.bruce` file from SD card or LittleFS and reads the first 42 bytes — the `BapHeader`:

```c
struct BapHeader {
    char magic[4];        // "BAP1"
    uint8_t arch;         // 0x01 = ESP32, 0x02 = ESP32-S3
    uint8_t version;      // BAP format version
    char name[32];        // App name (null-terminated)
    uint32_t elf_size;    // Size of the ELF payload
};
```

The loader validates:
- **Magic** — Must be `"BAP1"`.
- **Architecture** — Must match the running device (`0x01` for ESP32, `0x02` for ESP32-S3).
- **Name** — Force null-terminated to prevent buffer overread.

### Step 2: Load ELF into RAM

The ELF payload starts immediately after the header (at offset 42). The ELF loader:

1. **Reads ELF header** — Validates magic, machine type (Xtensa), and type (`ET_DYN` for PIE).
2. **Reads section headers** — Determines the virtual memory address range.
3. **Allocates contiguous memory** — Uses `heap_caps_malloc(size, MALLOC_CAP_EXEC | MALLOC_CAP_8BIT)` to get executable RAM (IRAM).
4. **Loads PROGBITS** — Copies code and data sections to the allocated block.
5. **Processes relocations** — Iterates through `SHT_RELA` sections and patches:
   - `R_XTENSA_RELATIVE` — Adjusts VMA offsets to loaded base.
   - `R_XTENSA_32` — Adjusts absolute addresses.

   Relocations are processed in **chunks of 32 entries** to minimize peak RAM usage.

### Step 3: Execute App

The loader resolves the entry point from the ELF header and calls it synchronously:

```c
typedef void (*app_main_t)(BruceAPI*);
app_main_t app_main = (app_main_t)entry;
app_main(get_bruce_api());
```

The app runs in the **same FreeRTOS task** as the firmware menu. This avoids race conditions and ensures the display is fully under the app's control.

### Step 4: Cleanup

When `app_main()` returns:
1. **Garbage Collector** runs — `bap_free_tracked_allocs()` frees any memory the app allocated but forgot to release.
2. **ELF unloaded** — `heap_caps_free()` releases the code block.
3. **Heap delta logged** — Any suspicious memory leak is logged to Serial.

## API Versioning

The `BruceAPI` struct starts with an `api_version` field. This is checked by apps to ensure compatibility:

```c
#define BRUCE_API_VERSION 2  // Current version
```

| API Version | Changes |
|-------------|---------|
| 1 | Initial: display, input, memory, logging, random, time |
| 2 | Added `draw_image`, `draw_pixel` |

**Rule:** When the struct layout changes (new fields added, fields reordered), `BRUCE_API_VERSION` must be incremented. Apps compiled against a newer version will detect the mismatch and exit gracefully.

## Memory Management

### App Memory Quota

Each loaded app has a **64KB memory quota** managed through the `bruce_malloc` / `bruce_free` wrappers. The firmware tracks up to 64 allocations per app.

```
bruce_malloc(size)
    ├── Check: bap_allocated_memory + size > 64KB?  → Return NULL
    ├── malloc(size)
    ├── Track in bap_alloc_table[64]
    └── Return pointer

bruce_free(ptr)
    ├── Untrack from bap_alloc_table
    └── free(ptr)
```

### Garbage Collector

On app exit, `bap_free_tracked_allocs()` iterates through the allocation table and frees any remaining entries, logging them as leaked:

```
[BAP GC] Leaked alloc at slot 3 (1024 bytes), freeing 0x3FFD2000
```

## Source Code Map

| File | Purpose |
|------|---------|
| `src/core/bap_loader.h` | BAP header struct & magic definition |
| `src/core/bap_loader.cpp` | High-level app lifecycle (open, validate, load, run, cleanup) |
| `src/core/elf_loader.h` | ELF loader API |
| `src/core/elf_loader.cpp` | ELF parsing, section loading, Xtensa relocations |
| `src/core/bruce_api_table.cpp` | Jump table instantiation, wrappers, memory tracking, GC |
| `include/bruce_api.h` | Public SDK header (shared with app developers) |
| `src/core/menu_items/AppsMenu.cpp` | UI: scans `/apps/` and displays app list |
