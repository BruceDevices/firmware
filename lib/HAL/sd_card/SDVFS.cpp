#include "../SD.h"

#if !defined(USE_SD_MMC) || defined(SOC_SDMMC_HOST_SUPPORTED)
#include "SDVFS.h"
#include "diskio.h"

using namespace fs;

SdFsImpl &SDFS::sdFs() const { return static_cast<SdFsImpl &>(*_impl); }

bool SDFS::beginRawAccess() {
    auto &fs = sdFs();
    std::unique_lock<std::recursive_mutex> lock(fs.mutex, std::try_to_lock);
    if (!lock || _pdrv == 0xFF || fs._rawAccess || fs._openFiles != 0) return false;

    const char drive[] = {static_cast<char>('0' + _pdrv), ':', '\0'};
#ifdef USE_SD_MMC
    if (!fs._fat) {
        // The SDMMC mount helper keeps the registered FATFS in a private context.
        DWORD freeClusters;
        FATFS *fat;
        if (f_getfree(drive, &freeClusters, &fat) != FR_OK) return false;
        fs._fat = fat;
    }
#endif
    if (!fs._fat) return false;
    if (disk_ioctl(_pdrv, CTRL_SYNC, nullptr) != RES_OK) return false;

    // Leave the VFS context and disk I/O registration alive. With FatFs
    // detached, the host can own the physical sectors without a stale FAT cache.
    if (f_mount(nullptr, drive, 0) != FR_OK) return false;
    fs._rawAccess = true;
    return true;
}

bool SDFS::endRawAccess() {
    auto &fs = sdFs();
    std::lock_guard<std::recursive_mutex> lock(fs.mutex);
    if (!fs._rawAccess) return false;
    const char drive[] = {static_cast<char>('0' + _pdrv), ':', '\0'};
    const bool mounted = f_mount(fs._fat, drive, 1) == FR_OK;
    fs._rawAccess = false;
    if (mounted) return true;

    // The host may have removed or reformatted the filesystem. Release the
    // failed mount so the ordinary begin path can retry later without formatting.
    end();
    return false;
}

bool SDFS::rawAccessActive() const {
    return sdFs()._rawAccess;
}

namespace {
struct FatFileKey {
    LBA_t directory;
    uintptr_t entry;

    bool operator==(const FatFileKey &other) const {
        return directory == other.directory && entry == other.entry;
    }
};

FatFileKey fatFileKey(const FIL &file) {
#if FF_FS_EXFAT
    if (file.obj.fs->fs_type == FS_EXFAT) return {file.obj.c_scl, file.obj.c_ofs};
#endif
    // FatFs points dir_ptr into the same volume window for both lookups.
    // With the filesystem gate held, that window remains alive and its address is stable.
    return {file.dir_sect, reinterpret_cast<uintptr_t>(file.dir_ptr)};
}

bool distinctCopyTarget(const char *drive, const String &source, const String &destination) {
    String from(drive), to(drive);
    if (from.isEmpty() || to.isEmpty() || !from.concat(source) || !to.concat(destination)) return false;

    // Reuse one read-only FatFs object for both directory-entry lookups.
    FIL probe{};
    const FRESULT result = f_open(&probe, to.c_str(), FA_READ);
    if (result == FR_NO_FILE) return true;
    if (result != FR_OK) return false;
    const auto targetKey = fatFileKey(probe);
    if (f_close(&probe) != FR_OK || f_open(&probe, from.c_str(), FA_READ) != FR_OK) return false;
    const auto sourceKey = fatFileKey(probe);
    return f_close(&probe) == FR_OK && !(sourceKey == targetKey);
}
} // namespace

bool SDFS::openCopyFiles(
    const String &source, const String &destination, File &input, File &output
) {
    auto &fs = sdFs();
    std::lock_guard<std::recursive_mutex> lock(fs.mutex);
    if (_pdrv == 0xFF || fs._rawAccess) return false;
    const char drive[] = {static_cast<char>('0' + _pdrv), ':', '\0'};
    if (!distinctCopyTarget(drive, source, destination)) return false;

    input = open(source, FILE_READ);
    if (!input || input.isDirectory()) return false;
    output = open(destination, FILE_WRITE);
    return static_cast<bool>(output);
}

uint64_t SDFS::totalBytes() {
    auto &fs = sdFs();
    std::lock_guard<std::recursive_mutex> lock(fs.mutex);
    if (_pdrv == 0xFF || fs._rawAccess) return 0;

    const char drive[] = {static_cast<char>('0' + _pdrv), ':', '\0'};
    DWORD freeClusters;
    FATFS *fat;
    if (f_getfree(drive, &freeClusters, &fat) != FR_OK) return 0;
    return static_cast<uint64_t>(fat->csize) * (fat->n_fatent - 2) * sectorSize();
}

uint64_t SDFS::usedBytes() {
    auto &fs = sdFs();
    std::lock_guard<std::recursive_mutex> lock(fs.mutex);
    if (_pdrv == 0xFF || fs._rawAccess) return 0;

    const char drive[] = {static_cast<char>('0' + _pdrv), ':', '\0'};
    DWORD freeClusters;
    FATFS *fat;
    if (f_getfree(drive, &freeClusters, &fat) != FR_OK) return 0;
    return static_cast<uint64_t>(fat->csize) * (fat->n_fatent - 2 - freeClusters) * sectorSize();
}
#endif
