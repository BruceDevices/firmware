#ifndef USE_SD_MMC
#include "../SD.h"
#include "FS.h"
#include "ff.h"
// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "sd_diskio2.h"
#include "SDVFS.h"

using namespace fs;

SDFS::SDFS(FSImplPtr impl) : FS(std::make_shared<SdFsImpl>(std::move(impl))), _pdrv(0xFF) {}

bool SDFS::begin(
    uint8_t ssPin, SPIClass &spi, uint32_t frequency, const char *mountpoint, uint8_t max_files,
    bool format_if_empty
) {
    auto &fs = sdFs();
    std::lock_guard<std::recursive_mutex> lock(fs.mutex);
    if (fs._rawAccess) return false;
    if (_pdrv != 0xFF) { return true; }

    spi.begin();

    _pdrv = sdcard_init(ssPin, &spi, frequency);
    if (_pdrv == 0xFF) { return false; }

    if (!sdcard_mount(_pdrv, mountpoint, max_files, format_if_empty, &fs._fat)) {
        sdcard_unmount(_pdrv);
        sdcard_uninit(_pdrv);
        _pdrv = 0xFF;
        return false;
    }

    fs.setMountpoint(mountpoint);
    return true;
}

void SDFS::end() {
    auto &fs = sdFs();
    std::lock_guard<std::recursive_mutex> lock(fs.mutex);
    if (fs._rawAccess || fs._openFiles != 0) return;
    if (_pdrv != 0xFF) {
        fs.setMountpoint(nullptr);
        fs._fat = nullptr;
        sdcard_unmount(_pdrv);

        sdcard_uninit(_pdrv);
        _pdrv = 0xFF;
    }
}

sdcard_type_t SDFS::cardType() {
    std::lock_guard<std::recursive_mutex> lock(sdFs().mutex);
    if (_pdrv == 0xFF) { return CARD_NONE; }
    return sdcard_type(_pdrv);
}

uint64_t SDFS::cardSize() {
    std::lock_guard<std::recursive_mutex> lock(sdFs().mutex);
    if (_pdrv == 0xFF) { return 0; }
    size_t sectors = sdcard_num_sectors(_pdrv);
    size_t sectorSize = sdcard_sector_size(_pdrv);
    return (uint64_t)sectors * sectorSize;
}

size_t SDFS::numSectors() {
    std::lock_guard<std::recursive_mutex> lock(sdFs().mutex);
    if (_pdrv == 0xFF) { return 0; }
    return sdcard_num_sectors(_pdrv);
}

size_t SDFS::sectorSize() {
    std::lock_guard<std::recursive_mutex> lock(sdFs().mutex);
    if (_pdrv == 0xFF) { return 0; }
    return sdcard_sector_size(_pdrv);
}

bool SDFS::readRAW(uint8_t *buffer, uint32_t sector) {
    auto &fs = sdFs();
    std::lock_guard<std::recursive_mutex> lock(fs.mutex);
    return fs._rawAccess && buffer && sector < sdcard_num_sectors(_pdrv) &&
           sd_read_raw(_pdrv, buffer, sector);
}

bool SDFS::writeRAW(uint8_t *buffer, uint32_t sector) {
    auto &fs = sdFs();
    std::lock_guard<std::recursive_mutex> lock(fs.mutex);
    return fs._rawAccess && buffer && sector < sdcard_num_sectors(_pdrv) &&
           sd_write_raw(_pdrv, buffer, sector);
}

SDFS SD = SDFS(FSImplPtr(new VFSImpl()));
#endif
