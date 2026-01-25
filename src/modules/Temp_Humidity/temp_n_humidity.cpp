#include <DHT.h>

#include "core/display.h"
#include "core/utils.h"
#include <stdlib.h>
#include <string.h>

int gpioPinForDHT = 26;
// String DHTModel = "DHT22";

DHT dht(gpioPinForDHT, DHT11);

void setupDHT() {
    Serial.begin(115200);
    dht.begin();
}

/*
void DHTData() {
    setupDHT();
    while (true) {
        if (check(PrevPress)) return;
        // float temperature = dht.readTemperature();
        // float humidity = dht.readHumidity();

        float temperature = 22.57;
        float humidity = 60;

        tft.fillScreen(bruceConfig.bgColor);
        tft.setTextSize(2);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

        tft.drawString(String("Temp: ") + String(temperature, 1) + " C", 10, 10, 1);

        if (check(PrevPress)) return;

        tft.drawString(String("Humid: ") + String(humidity, 0) + " %", 10, 10 + 24, 1);

        if (check(PrevPress)) return;

        delay(180);

        if (check(PrevPress)) return;
    }
}
*/

void firstStart() {
    setupDHT();
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(2);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    tft.drawString(String("Temp: ") + String(temperature, 1) + " C", 10, 10, 1);

    if (check(PrevPress)) return;

    tft.drawString(String("Humid: ") + String(humidity, 0) + " %", 10, 10 + 24, 1);

    if (check(PrevPress)) return;
}

/*
void DHTData2() {
    firstStart();
    while (true) {
        float temperature = 20.1;
        float humidity = 20.1;

        for (int i = 0; i < 10000; ++i) {
            delay(1);
            if (check(PrevPress)) return;
        }

        // check temperature and humidity again
        // if not the same, then refresh screen with new value
        float temperatureNew = 21.1;
        float humidityNew = 21.1;

        if (check(PrevPress)) return;

        while (temperature != temperatureNew or humidity != humidityNew) {
            tft.fillScreen(bruceConfig.bgColor);
            tft.setTextSize(2);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

            tft.drawString(String("Temp: ") + String(temperatureNew, 1) + " C", 10, 10, 1);

            if (check(PrevPress)) return;

            tft.drawString(String("Humid: ") + String(humidityNew, 0) + " %", 10, 10 + 24, 1);

            temperature = temperatureNew;
            humidity = humidityNew;
        }
    }
}

*/

void DHTData3() {
    firstStart();
    while (true) {
        float temperature = dht.readTemperature();
        float humidity = dht.readHumidity();

        for (int i = 0; i < 10000; ++i) {
            delay(1);
            if (check(PrevPress)) return;
        }

        // check temperature and humidity again
        // if not the same, then refresh screen with new value
        float temperatureNew = dht.readTemperature();
        float humidityNew = dht.readHumidity();

        if (check(PrevPress)) return;

        while (temperature != temperatureNew or humidity != humidityNew) {
            tft.fillScreen(bruceConfig.bgColor);
            tft.setTextSize(2);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

            tft.drawString(String("Temp: ") + String(temperatureNew, 1) + " C", 10, 10, 1);

            if (check(PrevPress)) return;

            tft.drawString(String("Humid: ") + String(humidityNew, 0) + " %", 10, 10 + 24, 1);

            temperature = temperatureNew;
            humidity = humidityNew;
        }
    }
}
