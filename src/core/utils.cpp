#include "utils.h"
#include "core/wifi/wifi_common.h" //to return MAC addr
#include "scrollableTextArea.h"
#include <globals.h>

/*********************************************************************
**  Function: backToMenu
**  sets the global var to be be used in the options second parameter
**  and returnToMenu will be user do handle the breaks of all loops

when using loopfunctions with an option to "Back to Menu", use:

add this option:
    options.push_back({"Main Menu", [=]() { backToMenu(); }});

while(1) {
    if(returnToMenu) break; // stop this loop and return to the previous loop

    ...
    loopOptions(options);
    ...
}
*/

void backToMenu() { returnToMenu = true; }

void addOptionToMainMenu() {
    returnToMenu = false;
    options.push_back({"Main Menu", backToMenu});
}

/***************************************************************************************
** Function name: getBattery()
** Description:   Returns the battery value from 1-100
***************************************************************************************/
int getBattery() {
#ifdef ANALOG_BAT_PIN
#ifndef ANALOG_BAT_MULTIPLIER
#define ANALOG_BAT_MULTIPLIER 2.0f
#endif
    static bool adcInitialized = false;
    if (!adcInitialized) {
        pinMode(ANALOG_BAT_PIN, INPUT);
        adcInitialized = true;
    }
    uint32_t adcReading = analogReadMilliVolts(ANALOG_BAT_PIN);
    float actualVoltage = (float)adcReading * ANALOG_BAT_MULTIPLIER;
    const float MIN_VOLTAGE = 3300.0f;
    const float MAX_VOLTAGE = 4150.0f;
    float percent = ((actualVoltage - MIN_VOLTAGE) / (MAX_VOLTAGE - (MIN_VOLTAGE + 50.0f))) * 100.0f;

    if (percent < 0) percent = 1;
    if (percent > 100) percent = 100;
    return (int)percent;
#endif
    return 0;
}

void updateClockTimezone() {
    timeClient.begin();
    timeClient.update();

    timeClient.setTimeOffset(bruceConfig.tmz * 3600);

    localTime = myTZ.toLocal(timeClient.getEpochTime());

#if !defined(HAS_RTC)
    rtc.setTime(timeClient.getEpochTime());
    updateTimeStr(rtc.getTimeStruct());
    clock_set = true;
#endif
}

void updateTimeStr(struct tm timeInfo) {
    // Atualiza timeStr com a hora e minuto
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
}

void showDeviceInfo() {
    ScrollableTextArea area = ScrollableTextArea("DEVICE INFO");

    area.addLine("Bruce Version: " + String(BRUCE_VERSION));
    area.addLine("EEPROM size: " + String(EEPROMSIZE));
    area.addLine("Total heap: " + String(ESP.getHeapSize()));
    area.addLine("Free heap: " + String(ESP.getFreeHeap()));
    if (psramFound()) {
        area.addLine("Total PSRAM: " + String(ESP.getPsramSize()));
        area.addLine("Free PSRAM: " + String(ESP.getFreePsram()));
    }
    area.addLine("LittleFS total: " + String(LittleFS.totalBytes()));
    area.addLine("LittleFS used: " + String(LittleFS.usedBytes()));
    area.addLine("LittleFS free: " + String(LittleFS.totalBytes() - LittleFS.usedBytes()));
    area.addLine("MAC addr: " + String(WiFi.macAddress()));
    area.addLine("");

#ifdef HAS_SCREEN
    area.addLine("[SCREEN]");
    area.addLine("Rotation: " + String(ROTATION));
    area.addLine("Width: " + String(tftWidth) + "px");
    area.addLine("Height: " + String(tftHeight) + "px");
    area.addLine("Brightness: " + String(bruceConfig.bright) + "%");
    area.addLine("");
#endif

    area.addLine("[GPIO]");
    area.addLine("GROVE_SDA: " + String(bruceConfigPins.i2c_bus.sda));
    area.addLine("GROVE_SCL: " + String(bruceConfigPins.i2c_bus.scl));
    area.addLine("SERIAL TX: " + String(bruceConfigPins.uart_bus.tx));
    area.addLine("SERIAL RX: " + String(bruceConfigPins.uart_bus.rx));
    area.addLine("SPI_SCK_PIN: " + String(SPI_SCK_PIN));
    area.addLine("SPI_MOSI_PIN: " + String(SPI_MOSI_PIN));
    area.addLine("SPI_MISO_PIN: " + String(SPI_MISO_PIN));
    area.addLine("SPI_SS_PIN: " + String(SPI_SS_PIN));
    area.addLine("IR TX: " + String(TXLED));
    area.addLine("IR RX: " + String(RXLED));
    area.addLine("");

    area.addLine("[BAT]");
    area.addLine("Charge: " + String(getBattery()) + "%");
#ifdef USE_BQ27220_VIA_I2C
    area.addLine("BQ27220 ADDR: " + String(BQ27220_I2C_ADDRESS));
    area.addLine("Curr Capacity: " + String(bq.getRemainCap()) + "mAh");
    area.addLine("Full Capacity: " + String(bq.getFullChargeCap()) + "mAh");
    area.addLine("Design Capacity: " + String(bq.getDesignCap()) + "mAh");
    area.addLine("Charging: " + String(bq.getIsCharging()));
    area.addLine(
        "Charging Voltage: " + String(((double)bq.getVolt(VOLT_MODE::VOLT_CHARGING) / 1000.0)) + "V"
    );
    area.addLine("Charging Current: " + String(bq.getCurr(CURR_MODE::CURR_CHARGING)) + "mA");
    area.addLine(
        "Time to Empty: " + String((bq.getTimeToEmpty() / 1440)) + " days " +
        String(((bq.getTimeToEmpty() % 1440) / 60)) + " hrs " + String(((bq.getTimeToEmpty() % 1440) % 60)) +
        " mins"
    );
    area.addLine("Avg Power Use: " + String(bq.getAvgPower()) + "mW");
    area.addLine("Voltage: " + String(((double)bq.getVolt(VOLT_MODE::VOLT) / 1000.0)) + "V");
    area.addLine("Raw Voltage: " + String(bq.getVolt(VOLT_MODE::VOLT_RWA)) + "mV");
    area.addLine("Curr Current: " + String(bq.getCurr(CURR_INSTANT)) + "mA");
    area.addLine("Avg Current: " + String(bq.getCurr(CURR_MODE::CURR_AVERAGE)) + "mA");
    area.addLine("Raw Current: " + String(bq.getCurr(CURR_MODE::CURR_RAW)) + "mA");
#endif

    area.show();
}

#if defined(HAS_TOUCH)
/*********************************************************************
** Function: touchHeatMap
** Touchscreen Mapping, include this function after reading the touchPoint
**********************************************************************/
void touchHeatMap(struct TouchPoint t) {
    int third_x = tftWidth / 3;
    int third_y = tftHeight / 3;

    if (t.x > third_x * 0 && t.x < third_x * 1 && t.y > third_y) PrevPress = true;
    if (t.x > third_x * 1 && t.x < third_x * 2 && ((t.y > third_y && t.y < third_y * 2) || t.y > tftHeight))
        SelPress = true;
    if (t.x > third_x * 2 && t.x < third_x * 3) NextPress = true;
    if (t.x > third_x * 0 && t.x < third_x * 1 && t.y < third_y) EscPress = true;
    if (t.x > third_x * 1 && t.x < third_x * 2 && t.y < third_y) UpPress = true;
    if (t.x > third_x * 1 && t.x < third_x * 2 && t.y > third_y * 2 && t.y < third_y * 3) DownPress = true;
    /*
                        Touch area Map
                ________________________________ 0
                |   Esc   |   UP    |         |
                |_________|_________|         |_> third_y
                |         |   Sel   |         |
                |         |_________|  Next   |_> third_y*2
                |  Prev   |  Down   |         |
                |_________|_________|_________|_> third_y*3
                |__Prev___|___Sel___|__Next___| 20 pixel touch area where the touchFooter is drawn
                0         L third_x |         |
                                    Lthird_x*2|
                                              Lthird_x*3
    */
}

#endif

String getOptionsJSON() {
    String menutype = "regular_menu";
    if (menuOptionType == 0) menutype = "main_menu";
    else if (menuOptionType == 1) menutype = "sub_menu";

    String response = "{\"width\":" + String(tftWidth) + ", \"height\":" + String(tftHeight) +
                      ",\"menu\":\"" + menutype + "\",\"menu_title\":\"" + menuOptionLabel +
                      "\", \"options\":[";
    int i = 0;
    int sel = 0;
    for (auto opt : options) {
        response += "{\"n\":" + String(i) + ",\"label\":\"" + opt.label + "\"}";
        if (opt.hovered) sel = i;
        i++;
        if (i < options.size()) response += ",";
    }
    response += "], \"active\":" + String(sel) + "}";
    return response;
}

/*********************************************************************
** Function: i2c_bulk_write
** Sends múltiple registers via I2C using a compact table.
   bulk_data example..
   const uint8_t bulk_data[] = {
      2, 0x00, 0x00,       // <- datalen = 2, reg = 0x00, data = 0x00
      3, 0x01, 0x00, 0x02, // <- datalen = 3, reg = 0x01, data = 0x00, 0x02
      0 };                 // <- datalen 0 is end of data.
**********************************************************************/
void i2c_bulk_write(TwoWire *wire, uint8_t addr, const uint8_t *bulk_data) {
    const uint8_t *p = bulk_data;
    while (true) {
        uint8_t datalen = *p++;
        if (datalen == 0) { break; } // --- end of table ---
        uint8_t reg = *p++;
        wire->beginTransmission(addr);
        wire->write(reg);
        for (uint8_t i = 0; i < datalen - 1; i++) { wire->write(*p++); }
        uint8_t error = wire->endTransmission();
        if (error != 0) { log_e("I2C Write error %d", error); }
        delay(1);
    }
}
