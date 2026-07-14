# Miscellaneous

Various useful pieces of information, troubleshooting, and FAQ.

---

## Troubleshooting

### Build Errors (bbt.py)

| Error | Cause | Solution |
|-------|-------|----------|
| `Manifest not found` | No `application.bam` in current directory | Make sure you're in the app's root folder |
| `Manifest missing required field` | A required JSON field is absent | Check your `application.bam` against the [BAM spec](bruce_app_manifest.md) |
| `Compilation failed for X.c` | Syntax error or missing header | Read the GCC error message, fix the code |
| `Linking failed` | Undefined symbol reference | Ensure all functions are called via `api->` pointer, not directly |
| `Cannot find Bruce SDK` | `bruce_api.h` not found | Use `--sdk /path/to/firmware/include` |
| `Pillow is required` | Image assets present but Pillow not installed | `pip install Pillow` |

### Runtime Errors (on device)

| Serial Log | Cause | Solution |
|------------|-------|----------|
| `[BAP] Bad magic` | File is not a valid `.bruce` package | Rebuild with `bbt.py build` |
| `[BAP] Arch mismatch: file=0x01, system=0x02` | App built for ESP32 but running on ESP32-S3 | Rebuild with `--arch esp32s3` |
| `[ELF] Bad magic` | ELF payload is corrupted | Rebuild the app |
| `[ELF] Not Xtensa` | Wrong compiler used | Ensure you're using `xtensa-esp32-elf-gcc` |
| `[ELF] OOM: need X bytes IRAM` | Not enough executable RAM | Reduce app size, close other tasks |
| `[BAP] ERROR: App requested X bytes, exceeds quota` | App exceeded 64KB memory limit | Optimize memory usage |
| `[BAP GC] Leaked alloc at slot X` | App forgot to call `bruce_free` | Add proper cleanup before returning |
| `[ELF] Unsupported relocation type X` | Unsupported ELF relocation | Ensure `-fPIC -mlongcalls` flags are used |

### Environment Errors

| Error | Cause | Solution |
|-------|-------|----------|
| `Python version must be between 3.10 and 3.13` | Python 3.14+ not supported by PlatformIO | Use Python 3.12 or 3.13 |
| `xtensa-esp32-elf-gcc: not found` | Toolchain not in PATH | Run ESP-IDF's `export.sh` / `export.ps1` |
| PowerShell execution policy error | Script execution disabled | `Set-ExecutionPolicy RemoteSigned -Scope CurrentUser` |

---

## FAQ

### General

**Q: Do I need PlatformIO to build apps?**
A: No. BBT calls `xtensa-esp32-elf-gcc` directly. You only need the Xtensa toolchain (included with ESP-IDF).

**Q: Can I use C++ in my apps?**
A: Yes. BBT supports `.cpp`, `.cc`, and `.cxx` files. Your entry point must use C linkage:
```cpp
extern "C" {
    void app_main(BruceAPI* api);
}
```

**Q: What's the maximum app size?**
A: The `.bruce` file itself has no size limit, but the ELF payload must fit in executable RAM (IRAM). Practically, keep apps under 100 KB.

**Q: Can my app access WiFi, Bluetooth, or GPIO?**
A: Not yet. API v2 provides display, input, memory, and utilities. Hardware APIs will be added in future versions.

### Memory

**Q: How much memory can my app use?**
A: 64 KB via `bruce_malloc`. This is a hard limit enforced by the firmware.

**Q: What happens if I forget to free memory?**
A: The Garbage Collector automatically frees leaked allocations when your app exits. However, you should still free memory properly to avoid wasting RAM during execution.

**Q: Can I use standard `malloc()`?**
A: It will work, but allocations won't be tracked by the firmware. This means:
- They won't count toward the 64 KB quota.
- They won't be automatically freed on app exit.
- They could cause permanent memory leaks.

### Build System

**Q: How does BBT find source files?**
A: BBT recursively scans the app directory for `.c`, `.cpp`, `.cc`, `.cxx` files. It excludes `.bbt_build/`, `dist/`, `.git/`, and `__pycache__/`.

**Q: Can I have multiple source files?**
A: Yes. Place them anywhere in your app directory. BBT will find and compile them all.

**Q: How do I add include paths?**
A: BBT automatically adds the app directory, the SDK include directory, and the build directory to the include path. For subdirectories, use relative includes: `#include "libs/mylib.h"`.

### Compatibility

**Q: Will my app work on both ESP32 and ESP32-S3?**
A: You must build separately for each architecture:
```bash
python bbt.py build --arch esp32
python bbt.py build --arch esp32s3
```

**Q: What happens if I run an old app on new firmware?**
A: If the firmware's API version is higher than the app's, the app will still work — new functions are always appended to the end of the struct, so old offsets remain valid.

**Q: What if new firmware removes or changes an API function?**
A: The `BRUCE_API_VERSION` will be incremented. The app should check `api->api_version` at startup and exit gracefully if the version is too low.

---

## Comparison with Flipper Zero

| Feature | Flipper Zero | Bruce |
|---------|-------------|-------|
| App format | `.fap` | `.bruce` |
| Manifest | `application.fam` (Python) | `application.bam` (JSON) |
| Build tool | `fbt` / `ufbt` | `bbt.py` |
| Architecture | ARM Cortex-M4 (STM32) | Xtensa (ESP32/S3) |
| API binding | Symbol table + dynamic linking | Jump table (struct of function pointers) |
| Memory model | MPU-protected regions | Software quota (64 KB) |
| Asset compilation | Built into `fbt` | Built into `bbt.py` |
| App icon format | 10×10 XBM | Not yet supported |
| JS scripting | Yes (JS engine) | Not yet supported |
| Expansion modules | Yes (UART-based) | Not yet supported |

---

## Changelog

| Date | API Version | Changes |
|------|-------------|---------|
| 2026-07-13 | v1 | Initial release: display, input, memory, logging, random, time |
| 2026-07-13 | v2 | Added `draw_image`, `draw_pixel`. BBT build tool. BAM manifests. |
