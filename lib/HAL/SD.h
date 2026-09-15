#ifndef __SDM__H
#define __SDM__H
#include <FS.h>

// #define USE_SD_MMC // test, delete later

#ifdef USE_SD_MMC
#include "sdkconfig.h"
#include "soc/soc_caps.h"
#ifndef SOC_SDMMC_HOST_SUPPORTED

#ifdef ARDUINO
#warning The SDMMC library requires a device with an SDIO Host
#endif

#else
#include "driver/sdmmc_types.h"
#include "sd_defines.h"
#ifndef BOARD_MAX_SDMMC_FREQ
#define BOARD_MAX_SDMMC_FREQ SDMMC_FREQ_HIGHSPEED
#endif
#endif

#else
#include <SPI.h>
#include <sd_diskio2.h>
#endif

namespace fs {

class SDFS : public FS {
#ifdef USE_SD_MMC
protected:
    sdmmc_card_t *_card;
    int8_t _pin_clk = -1;
    int8_t _pin_cmd = -1;
    int8_t _pin_d0 = -1;
    int8_t _pin_d1 = -1;
    int8_t _pin_d2 = -1;
    int8_t _pin_d3 = -1;
#ifdef SOC_SDMMC_IO_POWER_EXTERNAL
    int8_t _power_channel = -1;
#endif
    uint8_t _pdrv = 0xFF;
    bool _mode1bit = false;

private:
    static bool sdmmcDetachBus(void *bus_pointer);

public:
    SDFS(FSImplPtr impl);
    bool setPins(int clk, int cmd, int d0);
    bool setPins(int clk, int cmd, int d0, int d1, int d2, int d3);
#ifdef SOC_SDMMC_IO_POWER_EXTERNAL
    bool setPowerChannel(int power_channel);
#endif
    bool begin(
        const char *mountpoint = "/sdcard", bool mode1bit = false, bool format_if_mount_failed = false,
        int sdmmc_frequency = BOARD_MAX_SDMMC_FREQ, uint8_t maxOpenFiles = 5
    );
    bool begin(int SS) { return begin(); };
#else
protected:
    uint8_t _pdrv;

public:
    SDFS(FSImplPtr impl);
    bool begin(
        uint8_t ssPin = SS, SPIClass &spi = SPI, uint32_t frequency = 4000000, const char *mountpoint = "/sd",
        uint8_t max_files = 5, bool format_if_empty = false
    );
#endif
    void end();
    sdcard_type_t cardType();
    uint64_t cardSize();
    size_t numSectors();
    size_t sectorSize();
    uint64_t totalBytes();
    uint64_t usedBytes();
    bool readRAW(uint8_t *buffer, uint32_t sector);
    bool writeRAW(uint8_t *buffer, uint32_t sector);
};

} // namespace fs

extern fs::SDFS SD;

using namespace fs;
typedef fs::File SDFile;
typedef fs::SDFS SDFileSystemClass;
#define SDFileSystem SD
#endif
