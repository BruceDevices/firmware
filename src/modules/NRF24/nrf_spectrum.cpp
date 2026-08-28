#include "nrf_spectrum.h"
#include "core/display.h"
#include "core/mykeyboard.h"

#define CHANNELS 80
#define RGB565(r, g, b) ((((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)))
uint8_t channel[CHANNELS];

#if defined(CARDPUTER_ADV_3IN1_MUX)
// Implemented by boards/m5stack-cardputer/interface.cpp.
// The mutex prevents the TCA8418 keyboard task from taking GPIO8/9 while
// an NRF24 receive window is in progress.
extern void adv3in1Nrf24CriticalBegin();
extern void adv3in1Nrf24CriticalEnd();
#endif


// scanning channels
#define _BW tftWidth / CHANNELS
String scanChannels(bool web) {
    String result = "{";

    uint8_t rpdValues[CHANNELS] = {0};

#if defined(CARDPUTER_ADV_3IN1_MUX)
    adv3in1Nrf24CriticalBegin();
#endif

    // CE must be inactive before changing channels.
    digitalWrite(bruceConfigPins.NRF24_bus.io0, LOW);

    for (int i = 0; i < CHANNELS; i++) {
        NRFradio.setChannel(i);
        NRFradio.startListening();

#if defined(CARDPUTER_ADV_3IN1_MUX)
        // The 3-in-1 expansion needs the RPD sample while CE still owns GPIO8.
        // Keep the hardware-validated tolerant detection strategy scoped to
        // this board so other NRF24 devices retain the upstream scanner.
        delayMicroseconds(130);
        bool foundSignal = NRFradio.testRPD();
        NRFradio.stopListening();
        foundSignal = foundSignal || NRFradio.testRPD() || NRFradio.available();

        if (foundSignal) {
            NRFradio.flush_rx();
        }

        int rpd = foundSignal ? 1 : 0;
#else
        delayMicroseconds(128);
        NRFradio.stopListening();

        int rpd = NRFradio.testRPD() ? 1 : 0;
#endif
        channel[i] = (channel[i] * 3 + rpd * 125) / 4;
        rpdValues[i] = channel[i];
    }

#if defined(CARDPUTER_ADV_3IN1_MUX)
    // Leave NRF24 electrically inactive while the TCA8418 borrows GPIO8/9.
    digitalWrite(bruceConfigPins.NRF24_bus.io0, LOW);
    digitalWrite(bruceConfigPins.NRF24_bus.cs, HIGH);
    adv3in1Nrf24CriticalEnd();
#else
    // Preserve upstream behaviour for every other NRF24 configuration.
    digitalWrite(bruceConfigPins.NRF24_bus.io0, HIGH);
#endif


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

void nrf_spectrum() {
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.drawString("2.40Ghz", 0, tftHeight - LH);
    tft.drawCentreString("2.44Ghz", tftWidth / 2, tftHeight - LH, 1);
    tft.drawRightString("2.48Ghz", tftWidth, tftHeight - LH, 1);


#if defined(CARDPUTER_ADV_3IN1_MUX)
    adv3in1Nrf24CriticalBegin();
#endif

    bool nrfStarted = nrf_start(NRF_MODE_SPI); // This function only works on SPI

    if (nrfStarted) {

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
    }

#if defined(CARDPUTER_ADV_3IN1_MUX)
    adv3in1Nrf24CriticalEnd();
#endif

    if (nrfStarted) {
        while (!check(EscPress)) {
            scanChannels();
            // The NRF24 mux is released before this yield. Pending TCA8418
            // events can therefore be serviced without corrupting the RF scan.
            vTaskDelay(pdMS_TO_TICKS(1));
        }
#if defined(CARDPUTER_ADV_3IN1_MUX)
        adv3in1Nrf24CriticalBegin();
#endif
        NRFradio.stopListening();
        NRFradio.powerDown();
#if defined(CARDPUTER_ADV_3IN1_MUX)
        digitalWrite(bruceConfigPins.NRF24_bus.io0, LOW);
        digitalWrite(bruceConfigPins.NRF24_bus.cs, HIGH);
        adv3in1Nrf24CriticalEnd();
#endif
        delay(250);
        return;

    } else {
        Serial.println("Fail Starting radio");
        displayError("NRF24 not found");
        delay(500);
        return;
    }
}
