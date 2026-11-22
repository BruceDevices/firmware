import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class StorageBootTest(unittest.TestCase):
    def test_only_blank_flash_is_formatted_and_sd_can_still_load(self):
        source = (ROOT / 'src/main.cpp').read_text()
        boot = source[source.index(' **  Function: begin_storage'):source.index(' **  Function: _setup_gpio')]
        boot = boot[boot.index('*/') + 2:boot.rindex('/*')]
        stub = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>

constexpr int ESP_PARTITION_TYPE_DATA = 1, ESP_PARTITION_SUBTYPE_DATA_SPIFFS = 0x82, ESP_OK = 0;
struct esp_partition_t { size_t size = 4096; } partition;
bool havePartition, haveSd, failRead, canFormat, formatted, nonBlank;
int attempts, canMountOnAttempt, mountReads, scanBytes, configLoads;
const esp_partition_t *esp_partition_find_first(int type, int subtype, const char *label) {
    assert(type == ESP_PARTITION_TYPE_DATA && subtype == ESP_PARTITION_SUBTYPE_DATA_SPIFFS);
    assert(std::strcmp(label, "spiffs") == 0);
    return havePartition ? &partition : nullptr;
}
int esp_partition_read_raw(const esp_partition_t *, size_t offset, void *buffer, size_t size) {
    ++mountReads;
    if (failRead) return -1;
    assert(offset + size <= partition.size);
    scanBytes += size;
    std::memset(buffer, 0xff, size);
    if (nonBlank && offset + size == partition.size) static_cast<uint8_t *>(buffer)[size - 1] = 0x00;
    return ESP_OK;
}
bool setupLittleFS() { return ++attempts == canMountOnAttempt; }
bool setupSdCard() { return haveSd; }
void delay(unsigned long) {}
struct { bool format() { formatted = true; return canFormat; } } LittleFS;
[[maybe_unused]] struct SerialStub { void println(const char *) {} } Serial;
struct Config { void fromFile(bool sd) { assert(sd == haveSd); ++configLoads; } } bruceConfig, bruceConfigPins;
#define RAM_LOG(...) ((void)0)

void reset(int mountOn = 0) {
    attempts = mountReads = scanBytes = configLoads = 0;
    formatted = haveSd = failRead = nonBlank = false;
    havePartition = canFormat = true;
    canMountOnAttempt = mountOn;
}
'''
        check = r'''
int main() {
    reset(1);
    begin_storage();
    assert(attempts == 1 && !formatted && mountReads == 0 && configLoads == 2);

    reset(2);
    begin_storage();
    assert(attempts == 2 && !formatted && mountReads == 0 && configLoads == 2);

    reset(3);
    begin_storage();
    assert(attempts == 3 && formatted && scanBytes == 4096 && configLoads == 2);

    reset();
    nonBlank = haveSd = true;
    begin_storage();
    assert(!formatted && scanBytes == 4096 && configLoads == 2);

    reset();
    failRead = true;
    begin_storage();
    assert(!formatted && mountReads == 1 && configLoads == 0);

    reset();
    havePartition = false;
    begin_storage();
    assert(!formatted && mountReads == 0 && configLoads == 0);

    reset();
    canFormat = false;
    begin_storage();
    assert(formatted && attempts == 2 && configLoads == 0);

    reset();
    begin_storage();
    assert(formatted && attempts == 3 && configLoads == 0);
}
'''
        with tempfile.TemporaryDirectory() as directory:
            cpp = Path(directory) / 'storage.cpp'
            binary = cpp.with_suffix('')
            cpp.write_text(stub + boot + check)
            subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], cwd=directory, check=True)
