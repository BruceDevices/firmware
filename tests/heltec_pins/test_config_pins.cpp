#include "configPins.h"
#include "host_fs.h"
#include <cassert>
#include <iostream>

std::function<void()> storageAccess;
SerialStub Serial;
FS LittleFS;
FS SD;
bool sdcardMounted = false;

static void assertFixed(const BruceConfigPins &config) {
    assert(config.SDCARD_bus.sck == 16 && config.SDCARD_bus.miso == 45);
    assert(config.SDCARD_bus.mosi == 15 && config.SDCARD_bus.cs == 3);
    assert(config.SDCARD_bus.io0 == -1 && config.SDCARD_bus.io2 == -1);
    assert(config.LoRa_bus.sck == 9 && config.LoRa_bus.miso == 11);
    assert(config.LoRa_bus.mosi == 10 && config.LoRa_bus.cs == 8);
    assert(config.LoRa_bus.io0 == 12 && config.LoRa_bus.io2 == 14);
    assert(config.sys_i2c.sda == 17 && config.sys_i2c.scl == 18);
    assert(config.gps_bus.rx == 39 && config.gps_bus.tx == 38);
    assert(config.gpsBaudrate == 9600);
}

static void assertDisabled(const BruceConfigPins::SPIPins &bus) {
    assert(bus.sck == -1 && bus.miso == -1 && bus.mosi == -1);
    assert(bus.cs == -1 && bus.io0 == -1 && bus.io2 == -1);
}

int main() {
    BruceConfigPins config;
#ifndef HELTEC_V4_EXPANSION_V2
    // The same translation unit also verifies that another board retains
    // configurable buses, including the corrected W5500 setter destination.
    config.setSDCardPins({6, 5, 4, 3});
    assert(config.SDCARD_bus.sck == 6 && config.SDCARD_bus.miso == 5);
    const auto before = config.LoRa_bus;
    config.setW5500Pins({1, 2, 3, 4, 5, 6});
    assert(config.W5500_bus.sck == 1 && config.LoRa_bus.sck == before.sck);
    std::cout << "Non-Heltec mutable configuration retained\n";
    return 0;
#else
    assertFixed(config);
    assert(config.iButton == -1);
    // Assert every FS entry sees repaired built-in pins, including saveFile's
    // read-before-write path and a failed/missing import.
    storageAccess = [&]() { assertFixed(config); };
    config.createFile();
    config.setSDCardPins({6, 6, 6, 6, 6, 6});
    config.setLoRaPins({6, 6, 6, 6, 6, 6});
    config.gps_bus = {6, 6};
    config.setUARTPins(config.gps_bus);
    config.sys_i2c = {6, 6};
    config.setI2CPins(config.sys_i2c);
    config.setGpsBaudrate(115200);
    assertFixed(config);

    // Feed real ArduinoJson imports through production fromJson/validateConfig.
    JsonDocument imported;
    config.toJson(imported.to<JsonObject>());
    JsonObject root = imported["12:34:56:78:9A:BC"];
    for (const char *name : {"SDCard_Pins", "LoRa_Pins"}) {
        for (const char *field : {"sck", "miso", "mosi", "cs", "io0", "io2"}) root[name][field] = 6;
    }
    root["GPS_bus"]["rx"] = 0;
    root["GPS_bus"]["tx"] = 44;
    root["gpsBaudrate"] = 115200;
    root["sys_i2c"]["sda"] = 0;
    root["sys_i2c"]["scl"] = 44;
    config.fromJson(imported.as<JsonObject>());
    assertFixed(config);

    // All GPIO values, including absent/flash/PSRAM/USB and out-of-range
    // values, must not become configurable optional bus signals.
    for (int pin = -2; pin <= 50; ++pin) {
        for (auto setter :
             {&BruceConfigPins::setCC1101Pins,
              &BruceConfigPins::setNrf24Pins,
              &BruceConfigPins::setPn532Pins,
              &BruceConfigPins::setSR25RPins,
              &BruceConfigPins::setW5500Pins}) {
            (config.*setter)({pin, pin, pin, pin, pin, pin});
            assertDisabled(config.CC1101_bus);
            assertDisabled(config.NRF24_bus);
            assertDisabled(config.PN532_bus);
            assertDisabled(config.ST25R_bus);
            assertDisabled(config.W5500_bus);
            assertFixed(config);
        }
        config.i2c_bus = {pin, pin};
        config.setI2CPins(config.i2c_bus);
        config.uart_bus = {pin, pin};
        config.setUARTPins(config.uart_bus);
        assert(config.i2c_bus.sda == -1 && config.i2c_bus.scl == -1);
        assert(config.uart_bus.rx == -1 && config.uart_bus.tx == -1);
        config.setIrTxPin(pin);
        config.setIrRxPin(pin);
        config.setRfTxPin(pin);
        config.setRfRxPin(pin);
        assert(config.irTx == (pin == 6 ? 6 : -1));
        assert(config.irRx == (pin == 6 ? 6 : -1));
        assert(config.rfTx == (pin == 6 ? 6 : -1));
        assert(config.rfRx == (pin == 6 ? 6 : -1));
    }
    config.setiButtonPin(6);
    assert(config.iButton == 6);
    config.setiButtonPin(0);
    assert(config.iButton == -1);
    // Direct persistence and failed loads repair state before touching storage.
    config.SDCARD_bus.cs = 44;
    config.createFile();
    config.SDCARD_bus.cs = 44;
    config.saveFile();
    config.SDCARD_bus.cs = 44;
    config.fromFile();
    assertFixed(config);
    std::cout << "Heltec configuration: fixed pins, imports, setters and persistence passed\n";
#endif
}
