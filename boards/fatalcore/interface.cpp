#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/buttons.h"
#include <Adafruit_INA219.h>
#include <Wire.h>
#include <globals.h>
#include <interface.h>

static constexpr int8_t kSelectButton = 0;
static constexpr int8_t kUpButton = 41;
static constexpr int8_t kDownButton = 40;
static constexpr int8_t kRightButton = 38;
static constexpr int8_t kLeftButton = 39;
static constexpr int8_t kEscapeButton = 21;

static DeviceButtons buttonsCfg() {
    return DeviceButtons{kLeftButton, kRightButton, kUpButton, kDownButton, kSelectButton, kEscapeButton};
}

#define BATTERY_MIN_MV 3300
#define BATTERY_MAX_MV 4200
#define CHARGING_CURRENT_MA 40
#define BATTERY_VOLTAGE_MULTIPLIER 1.0f
#define BATTERY_CACHE_MS 5000

static Adafruit_INA219 ina219;
static bool ina219Ready = false;
static unsigned long lastBatteryRead = 0;
static int lastBatteryPercent = 100;
static float lastBatteryMa = 0;

static bool readIna219(float &busMv, float &currentMa) {
    if (!ina219Ready) return false;
    busMv = ina219.getBusVoltage_V() * 1000.0f;
    currentMa = ina219.getCurrent_mA();
    return true;
}

void _setup_gpio() {
    bruceConfigPins.sys_i2c = {(gpio_num_t)47, (gpio_num_t)48};
    bruceConfigPins.i2c_bus = {(gpio_num_t)47, (gpio_num_t)48};
    bruceConfigPins.uart_bus = {(gpio_num_t)46, (gpio_num_t)45};
    bruceConfigPins.gps_bus = {(gpio_num_t)46, (gpio_num_t)45};
    bruceConfigPins.rfTx = 42;
    bruceConfigPins.rfRx = 42;
    bruceConfigPins.irTx = 2;
    bruceConfigPins.irRx = 1;
    bruceConfigPins.iButton = 10;
    bruceConfigPins.rotation = 1;

    bruceConfigPins.outer_bus = {(gpio_num_t)17, (gpio_num_t)8, (gpio_num_t)18, (gpio_num_t)11};
    bruceConfigPins.PN532_bus = {(gpio_num_t)17, (gpio_num_t)8, (gpio_num_t)18, (gpio_num_t)11};
    bruceConfigPins.ST25R_bus = {
        (gpio_num_t)17, (gpio_num_t)8, (gpio_num_t)18, (gpio_num_t)11, (gpio_num_t)12, GPIO_NUM_NC
    };
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)17, (gpio_num_t)8, (gpio_num_t)18, (gpio_num_t)9, (gpio_num_t)42, GPIO_NUM_NC
    };
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)17, (gpio_num_t)8, (gpio_num_t)18, (gpio_num_t)13, (gpio_num_t)14
    };
    bruceConfigPins.SDCARD_bus = {
        (gpio_num_t)17, (gpio_num_t)8, (gpio_num_t)18, (gpio_num_t)43
    };
#if !defined(LITE_VERSION)
    bruceConfigPins.LoRa_bus = {
        (gpio_num_t)17, (gpio_num_t)8, (gpio_num_t)18, (gpio_num_t)5, GPIO_NUM_NC, (gpio_num_t)4
    };
    bruceConfigPins.W5500_bus = {
        (gpio_num_t)17, (gpio_num_t)8, (gpio_num_t)18, (gpio_num_t)5, (gpio_num_t)4, (gpio_num_t)3
    };
#endif

    setSysI2CBus(&Wire);
    Wire.setPins(bruceConfigPins.sys_i2c.sda, bruceConfigPins.sys_i2c.scl);
    Wire.begin(bruceConfigPins.sys_i2c.sda, bruceConfigPins.sys_i2c.scl);

    hal_buttons_init(buttonsCfg(), 6);

    const gpio_num_t chipSelectPins[] = {
        bruceConfigPins.CC1101_bus.cs,
        bruceConfigPins.NRF24_bus.cs,
        bruceConfigPins.ST25R_bus.cs,
        bruceConfigPins.SDCARD_bus.cs,
        bruceConfigPins.LoRa_bus.cs,
        (gpio_num_t)TFT_CS,
    };
    for (gpio_num_t pin : chipSelectPins) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, HIGH);
    }

    bruceConfigPins.rfModule = CC1101_SPI_MODULE;
    bruceConfigPins.rfidModule = ST25R3916_SPI_MODULE;

    ina219Ready = ina219.begin(&Wire);
    if (!ina219Ready) Serial.println("[fatalcore] INA219 not found, battery voltage unavailable");
}

int getBattery() {
    float busMv, currentMa;
    if (!readIna219(busMv, currentMa)) return 100;

    if (millis() - lastBatteryRead > BATTERY_CACHE_MS) {
        float mv = busMv * BATTERY_VOLTAGE_MULTIPLIER;
        float percent = ((mv - BATTERY_MIN_MV) / (float)(BATTERY_MAX_MV - BATTERY_MIN_MV)) * 100.0f;
        if (percent <= 0.0f) percent = 1;
        if (percent > 100.0f) percent = 100;

        lastBatteryPercent = (int)(percent + 0.5f);
        lastBatteryMa = currentMa;
        lastBatteryRead = millis();
    }
    return lastBatteryPercent;
}

bool isCharging() {
    if (!ina219Ready) return false;
    return lastBatteryRead != 0 && lastBatteryMa > CHARGING_CURRENT_MA;
}

void _post_setup_gpio() { hal_bright_attach(TFT_BL); }

void _setBrightness(uint8_t brightval) { hal_bright_set(TFT_BL, brightval); }

void InputHandler(void) { hal_buttons_poll_6(buttonsCfg()); }

void powerOff() { goToDeepSleep(); }

void checkReboot() {
    if (digitalRead(kEscapeButton) == LOW) {
        uint32_t timeCount = millis();
        while (digitalRead(kEscapeButton) == LOW) {
            if (millis() - timeCount > 500) {
                tft.setTextSize(1);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                int countDown = (millis() - timeCount) / 1000 + 1;
                if (countDown < 3) {
                    tft.drawCentreString("PWR OFF IN " + String(countDown) + "/2", tftWidth / 2, 12, 1);
                } else {
                    tft.fillScreen(bruceConfig.bgColor);
                    while (digitalRead(kEscapeButton) == LOW) {}
                    delay(200);
                    powerOff();
                }
                delay(10);
            }
        }
        delay(30);
        tft.fillRect(60, 12, tftWidth - 60, tft.fontHeight(1), bruceConfig.bgColor);
    }
}
