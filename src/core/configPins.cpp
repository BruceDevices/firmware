#include "configPins.h"
#include "esp_mac.h"
#include "sd_functions.h"
#include <globals.h>
String getMacAddress() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return String(macStr);
}

void BruceConfigPins::fromJson(JsonObject obj) {
    int count = 0;
    String mac = getMacAddress();

    if (obj[mac].isNull()) return saveFile();

    JsonObject root = obj[mac].as<JsonObject>();

    if (!root["rot"].isNull()) {
        rotation = root["rot"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }

    if (!root["bleName"].isNull()) {
        bleName = root["bleName"].as<String>();
    } else {
        count++;
        log_e("Fail");
    }

    if (!root["irTx"].isNull()) {
        irTx = root["irTx"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!root["irTxRepeats"].isNull()) {
        irTxRepeats = root["irTxRepeats"].as<uint8_t>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!root["irRx"].isNull()) {
        irRx = root["irRx"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }

    if (!root["rfTx"].isNull()) {
        rfTx = root["rfTx"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!root["rfRx"].isNull()) {
        rfRx = root["rfRx"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!root["rfModule"].isNull()) {
        rfModule = root["rfModule"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!root["rfFreq"].isNull()) {
        rfFreq = root["rfFreq"].as<float>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!root["rfFxdFreq"].isNull()) {
        rfFxdFreq = root["rfFxdFreq"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }
    if (!root["rfScanRange"].isNull()) {
        rfScanRange = root["rfScanRange"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }

    if (!root["rfidModule"].isNull()) {
        rfidModule = root["rfidModule"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }

    if (!root["iButton"].isNull()) {
        int val = root["iButton"].as<int>();
        if (val < GPIO_NUM_MAX) iButton = val;
        else log_w("iButton pin not set");
    } else {
        count++;
        log_e("Fail");
    }

    if (!root["gpsBaudrate"].isNull()) {
        gpsBaudrate = root["gpsBaudrate"].as<int>();
    } else {
        count++;
        log_e("Fail");
    }

    if (!root["CC1101_Pins"].isNull()) {
        SPIPins def = CC1101_bus;
        CC1101_bus.fromJson(root["CC1101_Pins"].as<JsonObject>());
        if (CC1101_bus.sck == GPIO_NUM_NC && def.sck != GPIO_NUM_NC) {
            CC1101_bus = def;
            count++;
        }
    } else {
        count++;
        log_e("Fail");
    }

    if (!root["NRF24_Pins"].isNull()) {
        SPIPins def = NRF24_bus;
        NRF24_bus.fromJson(root["NRF24_Pins"].as<JsonObject>());
        if (NRF24_bus.sck == GPIO_NUM_NC && def.sck != GPIO_NUM_NC) {
            NRF24_bus = def;
            count++;
        }
    } else {
        count++;
        log_e("Fail");
    }

    if (!root["SDCard_Pins"].isNull()) {
        SPIPins def = SDCARD_bus;
        SDCARD_bus.fromJson(root["SDCard_Pins"].as<JsonObject>());
        if (SDCARD_bus.sck == GPIO_NUM_NC && def.sck != GPIO_NUM_NC) {
            SDCARD_bus = def;
            count++;
        }
    } else {
        count++;
        log_e("Fail");
    }
#if !defined(LITE_VERSION)
    if (!root["W5500_Pins"].isNull()) {
        W5500_bus.fromJson(root["W5500_Pins"].as<JsonObject>());
    } else {
        count++;
        log_e("Fail");
    }

    if (!root["LoRa_Pins"].isNull()) {
        SPIPins def = LoRa_bus;
        LoRa_bus.fromJson(root["LoRa_Pins"].as<JsonObject>());
        if (LoRa_bus.sck == GPIO_NUM_NC && def.sck != GPIO_NUM_NC) {
            LoRa_bus = def;
            count++;
        }
    } else {
        count++;
        log_e("Fail");
    }
#endif
    // if (!root["sys_i2c"].isNull()) {
    //     sys_i2c.fromJson(root["sys_i2c"].as<JsonObject>());
    // } else {
    //     count++;
    //     log_e("Fail");
    // }
    if (!root["i2c_bus"].isNull()) {
        i2c_bus.fromJson(root["i2c_bus"].as<JsonObject>());
    } else {
        count++;
        log_e("Fail");
    }
    if (!root["uart_bus"].isNull()) {
        uart_bus.fromJson(root["uart_bus"].as<JsonObject>());
    } else {
        count++;
        log_e("Fail");
    }
    if (!root["GPS_bus"].isNull()) {
        gps_bus.fromJson(root["GPS_bus"].as<JsonObject>());
    } else {
        count++;
        log_e("Fail");
    }
    validateConfig();
    if (count > 0) saveFile();
}

void BruceConfigPins::toJson(JsonObject obj) const {
    JsonObject root = obj[getMacAddress()].to<JsonObject>();

    root["rot"] = rotation;
    root["irTx"] = irTx;
    root["irTxRepeats"] = irTxRepeats;
    root["irRx"] = irRx;
    root["rfTx"] = rfTx;
    root["rfRx"] = rfRx;
    root["rfModule"] = rfModule;
    root["rfFreq"] = rfFreq;
    root["rfFxdFreq"] = rfFxdFreq;
    root["rfScanRange"] = rfScanRange;
    root["bleName"] = bleName;
    root["rfidModule"] = rfidModule;
    root["gpsBaudrate"] = gpsBaudrate;
    root["iButton"] = iButton;

    JsonObject _CC1101 = root["CC1101_Pins"].to<JsonObject>();
    CC1101_bus.toJson(_CC1101);

    JsonObject _NRF = root["NRF24_Pins"].to<JsonObject>();
    NRF24_bus.toJson(_NRF);

    JsonObject _SD = root["SDCard_Pins"].to<JsonObject>();
    SDCARD_bus.toJson(_SD);

#if !defined(LITE_VERSION)
    JsonObject _W5500 = root["W5500_Pins"].to<JsonObject>();
    W5500_bus.toJson(_W5500);

    JsonObject _LoRa = root["LoRa_Pins"].to<JsonObject>();
    LoRa_bus.toJson(_LoRa);
#endif
    // JsonObject _si2c = root["sys_i2c"].as<JsonObject>();
    // sys_i2c.toJson(_si2c);
    JsonObject _di2c = root["i2c_bus"].to<JsonObject>();
    i2c_bus.toJson(_di2c);
    JsonObject _uart = root["uart_bus"].to<JsonObject>();
    uart_bus.toJson(_uart);
    JsonObject _gps = root["GPS_bus"].to<JsonObject>();
    gps_bus.toJson(_gps);
}

void BruceConfigPins::loadFile(JsonDocument &jsonDoc, bool checkFS) {
    FS *fs;
    if (checkFS) {
        if (!getFsStorage(fs)) return;
    } else {
        if (checkLittleFsSize()) fs = &LittleFS;
        else return;
    }

    if (!fs->exists(filepath)) return createFile();

    File file;
    file = fs->open(filepath, FILE_READ);
    if (!file) {
        log_e("Config pins file not found. Using default values");
        return;
    }

    if (deserializeJson(jsonDoc, file)) {
        log_e("Failed to read config pins file, using default configuration");
        return;
    }
    file.close();

    serializeJsonPretty(jsonDoc, Serial);
}

void BruceConfigPins::fromFile(bool checkFS) {
    JsonDocument jsonDoc;
    loadFile(jsonDoc, checkFS);

    if (!jsonDoc.isNull()) fromJson(jsonDoc.as<JsonObject>());
    jsonDoc.clear();
}

void BruceConfigPins::createFile() {
    JsonDocument jsonDoc;
    toJson(jsonDoc.to<JsonObject>());
    serializeJsonPretty(jsonDoc, Serial);

    // Open file for writing
    FS *fs = &LittleFS;
    File file = fs->open(filepath, FILE_WRITE);
    if (!file) {
        log_e("Failed to open config file");
        file.close();
        return;
    };

    // Serialize JSON to file
    if (serializeJsonPretty(jsonDoc, file) < 5) log_e("Failed to write config file");
    else log_i("config file written successfully");

    file.close();
    jsonDoc.clear();

    // don't try to mount SD Card if not previously mounted
    if (sdcardMounted) copyToFs(LittleFS, SD, filepath, false);
}

void BruceConfigPins::saveFile() {
    JsonDocument jsonDoc;
    loadFile(jsonDoc);

    if (jsonDoc.isNull()) return createFile();

    jsonDoc.remove(getMacAddress());
    toJson(jsonDoc.as<JsonObject>());

    // Open file for writing
    FS *fs = &LittleFS;
    File file = fs->open(filepath, FILE_WRITE);
    if (!file) {
        log_e("Failed to open config file");
        return;
    };

    // Serialize JSON to file
    if (serializeJsonPretty(jsonDoc, file) < 5) log_e("Failed to write config file");
    else log_i("config file written successfully");

    file.close();
    jsonDoc.clear();
    // don't try to mount SD Card if not previously mounted
    if (sdcardMounted) copyToFs(LittleFS, SD, filepath, false);
}

void BruceConfigPins::factoryReset() {
    FS *fs = &LittleFS;
    fs->rename(String(filepath), "/bak." + String(filepath).substring(1));
    // don't try to mount SD Card if not previously mounted
    if (sdcardMounted) SD.rename(String(filepath), "/bak." + String(filepath).substring(1));
}

void BruceConfigPins::validateConfig() {
    validateRotationValue();
    validateRfScanRangeValue();
    validateRfModuleValue();
    validateRfidModuleValue();
    validateGpsBaudrateValue();
#if !defined(LITE_VERSION)
    validateSpiPins(LoRa_bus);
    validateSpiPins(W5500_bus);
#endif
    validateSpiPins(CC1101_bus);
    validateSpiPins(NRF24_bus);
    validateSpiPins(SDCARD_bus);
    validateI2CPins(i2c_bus);
    validateUARTPins(uart_bus);
    validateUARTPins(gps_bus);
}
#if !defined(LITE_VERSION)
void BruceConfigPins::setLoRaPins(SPIPins value) {
    LoRa_bus = value;
    validateSpiPins(LoRa_bus);
    saveFile();
}
void BruceConfigPins::setW5500Pins(SPIPins value) {
    LoRa_bus = value;
    validateSpiPins(W5500_bus);
    saveFile();
}
#endif
void BruceConfigPins::setCC1101Pins(SPIPins value) {
    CC1101_bus = value;
    validateSpiPins(CC1101_bus);
    saveFile();
}

void BruceConfigPins::setNrf24Pins(SPIPins value) {
    NRF24_bus = value;
    validateSpiPins(NRF24_bus);
    saveFile();
}

void BruceConfigPins::setSDCardPins(SPIPins value) {
    SDCARD_bus = value;
    validateSpiPins(SDCARD_bus);
    saveFile();
}

void BruceConfigPins::setSpiPins(SPIPins value) {
    validateSpiPins(value);
    saveFile();
}

void BruceConfigPins::setI2CPins(I2CPins value) {
    validateI2CPins(value);
    saveFile();
}

void BruceConfigPins::setUARTPins(UARTPins value) {
    validateUARTPins(value);
    saveFile();
}
void BruceConfigPins::validateSpiPins(SPIPins value) {
    if (value.sck < 0 || value.sck > GPIO_PIN_COUNT) value.sck = GPIO_NUM_NC;
    if (value.miso < 0 || value.miso > GPIO_PIN_COUNT) value.miso = GPIO_NUM_NC;
    if (value.mosi < 0 || value.mosi > GPIO_PIN_COUNT) value.mosi = GPIO_NUM_NC;
    if (value.cs < 0 || value.cs > GPIO_PIN_COUNT) value.cs = GPIO_NUM_NC;
    if (value.io0 < 0 || value.io0 > GPIO_PIN_COUNT) value.io0 = GPIO_NUM_NC;
    if (value.io2 < 0 || value.io2 > GPIO_PIN_COUNT) value.io2 = GPIO_NUM_NC;
}

void BruceConfigPins::validateI2CPins(I2CPins value) {
    if (value.sda < 0 || value.sda > GPIO_PIN_COUNT) value.sda = GPIO_NUM_NC;
    if (value.scl < 0 || value.scl > GPIO_PIN_COUNT) value.scl = GPIO_NUM_NC;
}

void BruceConfigPins::validateUARTPins(UARTPins value) {
    if (value.rx < 0 || value.rx > GPIO_PIN_COUNT) value.rx = GPIO_NUM_NC;
    if (value.tx < 0 || value.tx > GPIO_PIN_COUNT) value.tx = GPIO_NUM_NC;
}

void BruceConfigPins::setRotation(int value) {
    rotation = value;
    validateRotationValue();
    saveFile();
}

void BruceConfigPins::validateRotationValue() {
    if (rotation < 0 || rotation > 3) rotation = 1;
}

void BruceConfigPins::setBleName(String value) {
    bleName = value;
    saveFile();
}

void BruceConfigPins::setIrTxPin(int value) {
    irTx = value;
    saveFile();
}

void BruceConfigPins::setIrTxRepeats(uint8_t value) {
    irTxRepeats = value;
    saveFile();
}

void BruceConfigPins::setIrRxPin(int value) {
    irRx = value;
    saveFile();
}

void BruceConfigPins::setRfTxPin(int value) {
    rfTx = value;
    saveFile();
}

void BruceConfigPins::setRfRxPin(int value) {
    rfRx = value;
    saveFile();
}

void BruceConfigPins::setRfModule(RFModules value) {
    rfModule = value;
    validateRfModuleValue();
    saveFile();
}

void BruceConfigPins::validateRfModuleValue() {
    if (rfModule != M5_RF_MODULE && rfModule != CC1101_SPI_MODULE) { rfModule = M5_RF_MODULE; }
}

void BruceConfigPins::setRfFreq(float value, int fxdFreq) {
    rfFreq = value;
    if (fxdFreq > 1) rfFxdFreq = fxdFreq;
    saveFile();
}

void BruceConfigPins::setRfFxdFreq(float value) {
    rfFxdFreq = value;
    saveFile();
}

void BruceConfigPins::setRfScanRange(int value, int fxdFreq) {
    rfScanRange = value;
    rfFxdFreq = fxdFreq;
    validateRfScanRangeValue();
    saveFile();
}

void BruceConfigPins::validateRfScanRangeValue() {
    if (rfScanRange < 0 || rfScanRange > 3) rfScanRange = 3;
}

void BruceConfigPins::setRfidModule(RFIDModules value) {
    rfidModule = value;
    validateRfidModuleValue();
    saveFile();
}

void BruceConfigPins::validateRfidModuleValue() {
    if (rfidModule != M5_RFID2_MODULE && rfidModule != PN532_I2C_MODULE && rfidModule != PN532_SPI_MODULE &&
        rfidModule != RC522_SPI_MODULE && rfidModule != PN532_I2C_SPI_MODULE) {
        rfidModule = M5_RFID2_MODULE;
    }
}

void BruceConfigPins::setiButtonPin(int value) {
    if (value < GPIO_NUM_MAX) {
        iButton = value;
        saveFile();
    } else log_e("iButton: Gpio pin not set, incompatible with this device\n");
}
void BruceConfigPins::setGpsBaudrate(int value) {
    gpsBaudrate = value;
    validateGpsBaudrateValue();
    saveFile();
}

void BruceConfigPins::validateGpsBaudrateValue() {
    if (gpsBaudrate != 9600 && gpsBaudrate != 19200 && gpsBaudrate != 57600 && gpsBaudrate != 38400 &&
        gpsBaudrate != 115200)
        gpsBaudrate = 9600;
}
