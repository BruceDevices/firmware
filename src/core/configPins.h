#pragma once

#include "pins_arduino.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <precompiler_flags.h>
#include <set>
#include <vector>

enum RFIDModules {
    M5_RFID2_MODULE = 0,
    PN532_I2C_MODULE = 1,
    PN532_SPI_MODULE = 2,
    RC522_SPI_MODULE = 3,
    ST25R3916_SPI_MODULE = 4,
    PN532_I2C_SPI_MODULE = 5,
    ST25R3916_I2C_MODULE = 6,
};

enum RFModules {
    M5_RF_MODULE = 0,
    CC1101_SPI_MODULE = 1,
};

// How the microphone is wired. Chosen at runtime (was the MIC_SPM1423 / MIC_INMP441 macros plus
// the presence of a PIN_BCLK -D); the board picks its default in _setup_gpio(), the user can override it from
// pinsMenu() without rebuilding.
enum MicTypes {
    MIC_TYPE_PDM = 0,         // SPM1423 in PDM mode: clk + data only
    MIC_TYPE_I2S_MSB = 1,     // SPM1423 wired as MSB/left-justified I2S: clk(BCLK) + ws + data
    MIC_TYPE_I2S_PHILIPS = 2, // INMP441 & co, standard (Philips) I2S: clk(BCLK) + ws + data
};

class BruceConfigPins {
public:
    struct UARTPins {
        gpio_num_t rx = GPIO_NUM_NC;
        gpio_num_t tx = GPIO_NUM_NC;

        UARTPins(gpio_num_t rx = GPIO_NUM_NC, gpio_num_t tx = GPIO_NUM_NC) : rx(rx), tx(tx) {}

        void fromJson(JsonObject obj) {
            rx = (gpio_num_t)(obj["rx"] | (int)GPIO_NUM_NC);
            tx = (gpio_num_t)(obj["tx"] | (int)GPIO_NUM_NC);
        }

        void toJson(JsonObject obj) const {
            obj["rx"] = rx;
            obj["tx"] = tx;
        }

        bool checkConflict(int8_t p) {
            gpio_num_t pin = (gpio_num_t)p;
            if (rx == pin || tx == pin) return true;
            return false;
        }
    };

    struct I2CPins {
        gpio_num_t sda = GPIO_NUM_NC;
        gpio_num_t scl = GPIO_NUM_NC;

        I2CPins(gpio_num_t sda = GPIO_NUM_NC, gpio_num_t scl = GPIO_NUM_NC) : sda(sda), scl(scl) {}

        void fromJson(JsonObject obj) {
            sda = (gpio_num_t)(obj["sda"] | (int)GPIO_NUM_NC);
            scl = (gpio_num_t)(obj["scl"] | (int)GPIO_NUM_NC);
        }

        void toJson(JsonObject obj) const {
            obj["sda"] = sda;
            obj["scl"] = scl;
        }

        bool checkConflict(int8_t p) {
            gpio_num_t pin = (gpio_num_t)p;
            if (sda == pin || scl == pin) return true;
            return false;
        }
    };

    struct SPIPins {
        gpio_num_t sck = GPIO_NUM_NC;
        gpio_num_t miso = GPIO_NUM_NC;
        gpio_num_t mosi = GPIO_NUM_NC;
        gpio_num_t cs = GPIO_NUM_NC;
        gpio_num_t io0 = GPIO_NUM_NC;
        gpio_num_t io2 = GPIO_NUM_NC;

        SPIPins()
            : sck(GPIO_NUM_NC), miso(GPIO_NUM_NC), mosi(GPIO_NUM_NC), cs(GPIO_NUM_NC), io0(GPIO_NUM_NC),
              io2(GPIO_NUM_NC) {}

        SPIPins(
            gpio_num_t sck_val, gpio_num_t miso_val, gpio_num_t mosi_val, gpio_num_t cs_val,
            gpio_num_t io0_val = GPIO_NUM_NC, gpio_num_t io2_val = GPIO_NUM_NC
        )
            : sck(sck_val), miso(miso_val), mosi(mosi_val), cs(cs_val), io0(io0_val), io2(io2_val) {}

        void fromJson(JsonObject obj) {
            sck = (gpio_num_t)(obj["sck"] | (int)GPIO_NUM_NC);
            miso = (gpio_num_t)(obj["miso"] | (int)GPIO_NUM_NC);
            mosi = (gpio_num_t)(obj["mosi"] | (int)GPIO_NUM_NC);
            cs = (gpio_num_t)(obj["cs"] | (int)GPIO_NUM_NC);
            io0 = (gpio_num_t)(obj["io0"] | (int)GPIO_NUM_NC);
            io2 = (gpio_num_t)(obj["io2"] | (int)GPIO_NUM_NC);
        }

        void toJson(JsonObject obj) const {
            obj["sck"] = sck;
            obj["miso"] = miso;
            obj["mosi"] = mosi;
            obj["cs"] = cs;
            obj["io0"] = io0;
            obj["io2"] = io2;
        }

        bool checkConflict(int8_t p) {
            gpio_num_t pin = (gpio_num_t)p;
            if (sck == pin || miso == pin || mosi == pin || cs == pin) return true;
            return false;
        }
    };

    // I2S speaker wiring (HAS_SPEAKER). Was the BCLK/WCLK/DOUT/MCLK -D macros; now set per board
    // in _setup_gpio() and overridable at runtime from pinsMenu().
    struct SpeakerPins {
        gpio_num_t bclk = GPIO_NUM_NC;
        gpio_num_t ws = GPIO_NUM_NC; // LRCLK / word select
        gpio_num_t dout = GPIO_NUM_NC;
        gpio_num_t mclk = GPIO_NUM_NC; // NC when the codec derives MCLK from BCLK (e.g. Cardputer)

        SpeakerPins(
            gpio_num_t bclk = GPIO_NUM_NC, gpio_num_t ws = GPIO_NUM_NC, gpio_num_t dout = GPIO_NUM_NC,
            gpio_num_t mclk = GPIO_NUM_NC
        )
            : bclk(bclk), ws(ws), dout(dout), mclk(mclk) {}

        void fromJson(JsonObject obj) {
            bclk = (gpio_num_t)(obj["bclk"] | (int)GPIO_NUM_NC);
            ws = (gpio_num_t)(obj["ws"] | (int)GPIO_NUM_NC);
            dout = (gpio_num_t)(obj["dout"] | (int)GPIO_NUM_NC);
            mclk = (gpio_num_t)(obj["mclk"] | (int)GPIO_NUM_NC);
        }

        void toJson(JsonObject obj) const {
            obj["bclk"] = bclk;
            obj["ws"] = ws;
            obj["dout"] = dout;
            obj["mclk"] = mclk;
        }

        bool isValid() const { return bclk != GPIO_NUM_NC && ws != GPIO_NUM_NC && dout != GPIO_NUM_NC; }
    };

    // Microphone wiring (HAS_MICROPHONE). Was PIN_CLK/PIN_DATA/PIN_BCLK/PIN_WS + the
    // MIC_SPM1423/MIC_INMP441 -D macros; now set per board in _setup_gpio().
    struct MicPins {
        gpio_num_t clk = GPIO_NUM_NC;  // PDM clock, or BCLK in either I2S mode
        gpio_num_t ws = GPIO_NUM_NC;   // word select / LRCLK; unused (NC) in PDM mode
        gpio_num_t data = GPIO_NUM_NC; // DIN
        int type = MIC_TYPE_PDM;

        MicPins(
            gpio_num_t clk = GPIO_NUM_NC, gpio_num_t data = GPIO_NUM_NC, gpio_num_t ws = GPIO_NUM_NC,
            int type = MIC_TYPE_PDM
        )
            : clk(clk), ws(ws), data(data), type(type) {}

        void fromJson(JsonObject obj) {
            clk = (gpio_num_t)(obj["clk"] | (int)GPIO_NUM_NC);
            ws = (gpio_num_t)(obj["ws"] | (int)GPIO_NUM_NC);
            data = (gpio_num_t)(obj["data"] | (int)GPIO_NUM_NC);
            type = obj["type"] | (int)MIC_TYPE_PDM;
        }

        void toJson(JsonObject obj) const {
            obj["clk"] = clk;
            obj["ws"] = ws;
            obj["data"] = data;
            obj["type"] = type;
        }

        bool isValid() const {
            if (clk == GPIO_NUM_NC || data == GPIO_NUM_NC) return false;
            if (type != MIC_TYPE_PDM && ws == GPIO_NUM_NC) return false;
            return true;
        }
    };

    // An alternate wiring a board can offer for CC1101_bus/NRF24_bus (e.g. a legacy Grove module
    // vs. sharing the SD card's SPI bus, or an M5Stack Cap module). Populated per-board in
    // _setup_gpio(); the generic menus in settings.cpp/NRF24.cpp just list whatever is here, so a
    // new board declares its own alt wiring without editing shared code.
    struct SPIPinPreset {
        const char *label;
        SPIPins pins;
        // Optional wiring-diagram URL shown as a QR code if the module isn't found with this preset.
        const char *wiringQrUrl = nullptr;
    };

    const char *filepath = "/brucePins.conf";

    // SPI Buses

    // No fallback macro — every board sets these explicitly in _setup_gpio().
    SPIPins CC1101_bus;
    SPIPins NRF24_bus;
    // Alternate wirings for CC1101_bus/NRF24_bus; empty unless the board pushes presets into it.
    std::vector<SPIPinPreset> CC1101_presets;
    std::vector<SPIPinPreset> NRF24_presets;
    SPIPins PN532_bus;
    SPIPins ST25R_bus;
    SPIPins SDCARD_bus = {
        (gpio_num_t)SDCARD_SCK, (gpio_num_t)SDCARD_MISO, (gpio_num_t)SDCARD_MOSI, (gpio_num_t)SDCARD_CS
    };

#if !defined(LITE_VERSION)
    SPIPins W5500_bus;
    SPIPins LoRa_bus;
#endif
    // Board's default/generic SPI bus (used directly by drivers that don't have their own
    // dedicated bus, e.g. the RC522-SPI RFID2 driver and some M5Stack Cap presets). Set per-board
    // in _setup_gpio(); no fallback macro (was SPI_SCK_PIN/SPI_MISO_PIN/SPI_MOSI_PIN/SPI_SS_PIN,
    // removed from boards/**/*.ini).
    SPIPins outer_bus;
    // sys_i2c has no fallback macro (SYS_I2C_SDA/SCL are only ever board-local -D's now, e.g.
    // CYD-2432S028's GT911 variant) — every board sets it explicitly in _setup_gpio().
    I2CPins sys_i2c;
    I2CPins i2c_bus = {(gpio_num_t)GROVE_SDA, (gpio_num_t)GROVE_SCL};
    UARTPins uart_bus;
    UARTPins gps_bus;
    // BadUSB CH9329 UART (used on devices without native USB-OTG)
    UARTPins badusb_bus;

    // Screen Rotation
    int rotation = ROTATION > 1 ? 3 : 1;

    // BLE
    String bleName = String("Keyboard_" + String((uint8_t)(ESP.getEfuseMac() >> 32), HEX));

    // IR
    int irTx = -1;
    uint8_t irTxRepeats = 0;
    int irRx = -1;

    // RF
    int rfTx = GROVE_SDA;
    int rfRx = GROVE_SCL;
    int rfModule = M5_RF_MODULE;
    float rfFreq = 433.92;
    int rfFxdFreq = 1;
    int rfScanRange = 3;

    // iButton Pin
    int iButton = 0;

    // RFID
    int rfidModule = M5_RFID2_MODULE;

    // GPS
    int gpsBaudrate = 9600;

    // Audio. No fallback macros -- every board sets these in _setup_gpio().
    // The buzzer is always compiled in and stays silent while `buzzer` is not a usable pin; a
    // board with HAS_SPEAKER uses the I2S speaker instead.
    SpeakerPins speaker_bus;
    MicPins mic_bus;
    int buzzer = -1;

    /////////////////////////////////////////////////////////////////////////////////////
    // Constructor
    /////////////////////////////////////////////////////////////////////////////////////
    BruceConfigPins() {};

    /////////////////////////////////////////////////////////////////////////////////////
    // Operations
    /////////////////////////////////////////////////////////////////////////////////////
    void createFile();
    void saveFile();
    void fromFile(bool checkFS = true);
    void loadFile(JsonDocument &jsonDoc, bool checkFS = true);
    void factoryReset();
    void validateConfig();
    void fromJson(JsonObject obj);
    void toJson(JsonObject obj) const;

    void setCC1101Pins(SPIPins value);
    void setNrf24Pins(SPIPins value);
    void setPn532Pins(SPIPins value);
    void setSDCardPins(SPIPins value);
#if !defined(LITE_VERSION)
    void setSR25RPins(SPIPins value);
    void setLoRaPins(SPIPins value);
    void setW5500Pins(SPIPins value);
#endif
    void setSpiPins(SPIPins value);
    void setI2CPins(I2CPins value);
    void setUARTPins(UARTPins value);
    void validateSpiPins(SPIPins value);
    void validateI2CPins(I2CPins value);
    void validateUARTPins(UARTPins value);

    // Screen Rotation
    void setRotation(int value);
    void validateRotationValue();
    // BLE
    void setBleName(const String name);

    // IR
    void setIrTxPin(int value);
    void setIrTxRepeats(uint8_t value);
    void setIrRxPin(int value);

    // RF
    void setRfTxPin(int value);
    void setRfRxPin(int value);
    void setRfModule(RFModules value);
    void validateRfModuleValue();
    void setRfFreq(float value, int fxdFreq = 1);
    void setRfFxdFreq(float value);
    void setRfScanRange(int value, int fxdFreq = 0);
    void validateRfScanRangeValue();

    // iButton
    void setiButtonPin(int value);

    // RFID
    void setRfidModule(RFIDModules value);
    void validateRfidModuleValue();

    // GPS
    void setGpsBaudrate(int value);
    void validateGpsBaudrateValue();

    // Audio
    void setSpeakerPins(SpeakerPins value);
    void setMicPins(MicPins value);
    void setBuzzerPin(int value);
    void validateSpeakerPins(SpeakerPins &value);
    void validateMicPins(MicPins &value);
    void validateBuzzerPin();
};
