#include "nrf_spectrum.h"
#include "../../core/display.h"
#include "../../core/mykeyboard.h"

#define CHANNELS 80
#define RGB565(r, g, b) ((((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)))
uint8_t channel[CHANNELS];

// Register Access Functions
inline byte getRegister(SPIClass &SSPI, byte r) {

    digitalWrite(bruceConfigPins.NRF24_bus.cs, LOW);
    byte c = SSPI.transfer(r & 0x1F);
    c = SSPI.transfer(0);
    digitalWrite(bruceConfigPins.NRF24_bus.cs, HIGH);

    return c;
}

inline void setRegister(SPIClass &SSPI, byte r, byte v) {

    digitalWrite(bruceConfigPins.NRF24_bus.cs, LOW);
    SSPI.transfer((r & 0x1F) | 0x20);
    SSPI.transfer(v);
    digitalWrite(bruceConfigPins.NRF24_bus.cs, HIGH);
}

inline void powerDown(SPIClass &SSPI) { setRegister(SSPI, 0x00, getRegister(SSPI, 0x00) & ~0x02); }

// scanning channels
#define _BW tftWidth / CHANNELS
String scanChannels(SPIClass *SSPI, bool web) {
    String result = "{";

    uint8_t rpdValues[CHANNELS] = {0};
    digitalWrite(bruceConfigPins.NRF24_bus.io0, LOW);

    for (int i = 0; i < CHANNELS; i++) {
        NRFradio.setChannel(i);
        NRFradio.startListening();
        delayMicroseconds(128);
        NRFradio.stopListening();

        int rpd = NRFradio.testRPD() ? 1 : 0;
        channel[i] = (channel[i] * 3 + rpd * 125) / 4;
        rpdValues[i] = channel[i];
    }

    digitalWrite(bruceConfigPins.NRF24_bus.io0, HIGH);

    for (int i = 0; i < CHANNELS; i++) {
        int level = rpdValues[i];
        int x = i * _BW;
        int c = i;

        tft.drawFastVLine(
            x, tftHeight - (10 + level), level, (i % 2 == 0) ? bruceConfig.priColor : TFT_DARKGREY
        ); // for level display

        tft.drawFastVLine(
            x, 0, tftHeight - (9 + level), (i % 8) ? TFT_BLACK : RGB565(25, 25, 25)
        );                                                    /// for clearing
        tft.drawFastVLine(x, 0, level, bruceConfig.secColor); /// for top display
        // show 5 channel gap only
        if (c % 5 == 0 && c != 0) { tft.drawCentreString(String(c).c_str(), x, tftHeight / 2, 1); }

        if (web) {
            if (i > 0) result += ",";
            result += String(level);
        }
    }

    if (web) result += "}";
    return result; // return a string in this format "{1,32,45,32,84,32 .... 12,54,65}" with 80 values to be
                   // used in the WebUI (Future)
}

void nrf_spectrum(SPIClass *SSPI) {
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.drawString("2.40Ghz", 0, tftHeight - LH);
    tft.drawCentreString("2.44Ghz", tftWidth / 2, tftHeight - LH, 1);
    tft.drawRightString("2.48Ghz", tftWidth, tftHeight - LH, 1);

    if (nrf_start(NRF_MODE_SPI)) { // This function only works on SPI
        NRFradio.setAutoAck(false);
        NRFradio.disableCRC();       // accept any signal we find
        NRFradio.setAddressWidth(2); // a reverse engineering tactic (not typically recommended)
        const uint8_t noiseAddress[][2] = {
            {0x55, 0x55},
            {0xAA, 0xAA},
            {0xA0, 0xAA},
            {0xAB, 0xAA},
            {0xAC, 0xAA},
            {0xAD, 0xAA}
        };
        for (uint8_t i = 0; i < 6; ++i) { NRFradio.openReadingPipe(i, noiseAddress[i]); }
        NRFradio.setDataRate(RF24_1MBPS);

        while (!check(EscPress)) { scanChannels(SSPI); }
        NRFradio.stopListening();
        powerDown(*SSPI);
        delay(250);
        return;

    } else {
        Serial.println("Fail Starting radio");
        displayError("NRF24 not found");
        delay(500);
        return;
    }
}
