#include "nrf_jammer.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include <globals.h>

/* **************************************************************************************
 ** name : nrf_jammer
 ** details : Starts 2.4GHz jammer using NRF24
 ************************************************************************************** */

void nrf_jammer() {
    int OnX = 0;
    NRF24_MODE mode = nrf_setMode();
    int NRFOnline = 1;
    byte Test_channels[] = {50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 78, 80, 2,  4,  6,  8,
                            10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48};

    // Channel groups
    byte wifi_channels[] = {
        2,
        7,
        12,
        17,
        22,
        27,
        32,
        37, // WiFi 2412–2442 MHz
        42,
        47,
        52,
        57,
        62,
        67,
        72,
        77 // WiFi 2447–2487 MHz
    };
    byte ble_channels[] = {2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                           22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41};

    byte ble_adv[] = {1, 2, 3, 25, 26, 27, 79, 80, 81};

    byte bluetooth_channels[] = {2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
                                 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
                                 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
                                 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65,
                                 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80};
    byte usb_channels[] = {40, 50, 60};
    byte video_channels[] = {70, 75, 80};
    byte rc_channels[] = {1, 3, 5, 7};
    byte full_channels[] = {1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,
                            17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,
                            33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
                            49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,
                            65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,
                            81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,
                            97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112,
                            113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124};

    struct jamMode {
        const char *name;
        byte *channels;
        size_t count;
    };
    jamMode modes[] = {
        {"Test        ", Test_channels,      sizeof(Test_channels) / sizeof(Test_channels[0])          },
        {"WiFi        ", wifi_channels,      sizeof(wifi_channels) / sizeof(wifi_channels[0])          },
        {"BLEch ",       ble_channels,       sizeof(ble_channels) / sizeof(ble_channels[0])            },
        {"BLE Adv ",     ble_adv,            sizeof(ble_adv) / sizeof(ble_adv[0])                      },
        {"Bluetooth   ", bluetooth_channels, sizeof(bluetooth_channels) / sizeof(bluetooth_channels[0])},
        {"USB         ", usb_channels,       sizeof(usb_channels) / sizeof(usb_channels[0])            },
        {"Video Stream", video_channels,     sizeof(video_channels) / sizeof(video_channels[0])        },
        {"RC          ", rc_channels,        sizeof(rc_channels) / sizeof(rc_channels[0])              },
        {"Full        ", full_channels,      sizeof(full_channels) / sizeof(full_channels[0])          }
    };

    if (nrf_start(mode)) {
        Serial.println("NRF24 turned On");

        int modeIndex = 0;
        int hopIndex = 0;
        bool redraw = true;
        if (CHECK_NRF_SPI(mode)) {
            NRFradio.setPALevel(RF24_PA_MAX);
            NRFradio.startConstCarrier(RF24_PA_MAX, 50);
            NRFradio.setAddressWidth(5);
            NRFradio.setPayloadSize(2);
            if (!NRFradio.setDataRate(RF24_2MBPS)) Serial.println("Fail setting data Rate");
        }

        drawMainBorder();

        if ((CHECK_NRF_UART(mode)) || (CHECK_NRF_BOTH(mode))) {
            NRFSerial.println("RADIOS");
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);

        while (!check(SelPress)) {

            if ((CHECK_NRF_UART(mode)) || (CHECK_NRF_BOTH(mode))) {

                if (OnX == 0) {
                    NRFSerial.println("RADIOS");
                    vTaskDelay(250 / portTICK_PERIOD_MS);
                }

                if (NRFSerial.available()) {
                    String incomingNRFs = NRFSerial.readStringUntil('\n');
                    incomingNRFs.trim();
                    if (incomingNRFs.length() == 1 && isDigit(incomingNRFs.charAt(0))) {
                        OnX = 1;
                            NRFOnline = (incomingNRFs.toInt());
                        if (CHECK_NRF_BOTH(mode)) {
                            NRFOnline = (incomingNRFs.toInt()) + 1;
                        }
                        redraw = true;
                    }
                }
            }

            if (redraw) {
                tft.setCursor(10, 35);
                tft.setTextSize(FM);
                tft.println("NRF X Jammer");
                tft.setCursor(10, tft.getCursorY() + 25);
                tft.println("STATUS : " + String(NRFOnline) + " ACTIVE");
                tft.setCursor(10, 100);
                tft.fillRect(10, 100, tftWidth - 20, FM * LH, bruceConfig.bgColor);
                tft.print("MODE : " + String(modes[modeIndex].name));
                tft.drawRoundRect(5, 5, tftWidth - 10, tftHeight - 10, 5, bruceConfig.priColor);
                Serial.println(modes[modeIndex].name);
                if ((CHECK_NRF_UART(mode)) || (CHECK_NRF_BOTH(mode))) {
                    String Mode = modes[modeIndex].name;
                    Mode.replace(" " ,"");
                    NRFSerial.println(Mode);
                }
                redraw = false;
                vTaskDelay(200 / portTICK_PERIOD_MS);
            }

            // Hop through channels
            hopIndex++;
            if (hopIndex >= modes[modeIndex].count) hopIndex = 0;
            if (CHECK_NRF_SPI(mode)) { NRFradio.setChannel(modes[modeIndex].channels[hopIndex]); }

            // Change mode
            if (check(NextPress)) {
                modeIndex++;
                if (modeIndex >= (int)(sizeof(modes) / sizeof(modes[0]))) modeIndex = 0;
                hopIndex = 0;
                redraw = true;
            }
            if (check(PrevPress)) {
                modeIndex--;
                if (modeIndex < 0) modeIndex = (sizeof(modes) / sizeof(modes[0])) - 1;
                hopIndex = 0;
                redraw = true;
            }
        }

        if (CHECK_NRF_SPI(mode)) NRFradio.stopConstCarrier();
        if ((CHECK_NRF_UART(mode)) || (CHECK_NRF_BOTH(mode))) {
             NRFSerial.println("OFF");
        }

    } else {
        Serial.println("Fail Starting radio");
        displayError("NRF24 not found");
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

/* **************************************************************************************
 ** name : nrf_channel_jammer
 ** details : Steps manually through each channel 1–125
 ************************************************************************************** */
void nrf_channel_jammer() {
    int OnX = 0;
    NRF24_MODE mode = nrf_setMode();
    uint8_t NRFOnline = 1;
    uint8_t NRFSPI = 0;
    if (nrf_start(mode)) {
        Serial.println("NRF24 turned On");

        int channel = 50; /// we start at 50 as on lower channel the cw wont work correctly
        bool redraw = true;
        if (CHECK_NRF_SPI(mode)) {
            NRFradio.setPALevel(RF24_PA_MAX);
            NRFradio.startConstCarrier(RF24_PA_MAX, channel);
            NRFradio.setAddressWidth(3);
            NRFradio.setPayloadSize(2);
            if (!NRFradio.setDataRate(RF24_2MBPS)) Serial.println("Fail setting data Rate");
            NRFSPI = 1;
        }

        drawMainBorder();
        vTaskDelay(50 / portTICK_PERIOD_MS);
        NRFSerial.println("RADIOS");
        vTaskDelay(50 / portTICK_PERIOD_MS);

        while (!check(SelPress)) {
            if (CHECK_NRF_UART(mode) || CHECK_NRF_BOTH(mode)) {
                if (OnX == 0) {
                    NRFSerial.println("RADIOS");
                    vTaskDelay(250 / portTICK_PERIOD_MS);
                }
                if (NRFSerial.available()) {
                    String incomingNRFs = NRFSerial.readStringUntil('\n');
                    incomingNRFs.trim();
                    if (incomingNRFs.length() == 1 && isDigit(incomingNRFs.charAt(0))) {

                        NRFOnline = (incomingNRFs.toInt());
                        if (CHECK_NRF_BOTH(mode)) {
                            NRFOnline = (incomingNRFs.toInt()) + NRFSPI;
                        }
                        redraw = true;
                        OnX = 1;
                    }
                }
            }

            if (redraw) {
                int freq = 2400 + channel; // MHz
                tft.setCursor(10, 35);
                tft.setTextSize(FM);
                tft.println("NRF Channel Jammer");
                tft.setCursor(10, tft.getCursorY() + 25);
                tft.println("STATUS : " + String(NRFOnline) + " ACTIVE");
                tft.fillRect(10, 100, tftWidth - 20, FM * LH, bruceConfig.bgColor);
                tft.setCursor(10, 100);
                tft.print("MODE : CH " + String(channel));
                tft.setCursor(10, 116);
                tft.fillRect(10, 116, tftWidth - 20, FM * LH, bruceConfig.bgColor);
                tft.printf("Freq : %d MHz", freq);
                Serial.println("CH " + String(channel) + " (" + String(freq) + " MHz)");
                if (CHECK_NRF_UART(mode) || CHECK_NRF_BOTH(mode)) {
                     NRFSerial.println("CH_"+String(channel));
                }
                tft.drawRoundRect(5, 5, tftWidth - 10, tftHeight - 10, 5, bruceConfig.priColor);
                redraw = false;
                vTaskDelay(200 / portTICK_PERIOD_MS);
            }

            // Next/Prev channel
            if (check(NextPress)) {

                channel++;
                if (channel > 125) channel = 1;
                if (CHECK_NRF_SPI(mode)) {
                    NRFradio.setChannel(channel);
                    NRFradio.startConstCarrier(RF24_PA_MAX, channel);
                }

                redraw = true;
            }
            if (check(PrevPress)) {

                channel--;
                if (channel < 1) channel = 125;
                if (CHECK_NRF_SPI(mode)) {
                    NRFradio.setChannel(channel);
                    NRFradio.startConstCarrier(RF24_PA_MAX, channel);
                }
                redraw = true;
            }
        }

        if (CHECK_NRF_SPI(mode)) NRFradio.stopConstCarrier();
        if (CHECK_NRF_UART(mode) || CHECK_NRF_BOTH(mode)) {
            NRFSerial.println("OFF");
        }

    } else {
        Serial.println("Fail Starting radio");
        displayError("NRF24 not found");
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void nrf_channel_hopper() {
    NRF24_MODE mode = nrf_setMode();
    uint8_t NRFOnline = 0;
    uint8_t NRFSPI = 0;

    if (!nrf_start(mode)) {
        Serial.println("Fail Starting radio");
        displayError("NRF24 not found");
        vTaskDelay(100 / portTICK_PERIOD_MS);
        return;
    }

    Serial.println("NRF24 turned On");
    if (CHECK_NRF_SPI(mode)) {
        NRFradio.setPALevel(RF24_PA_MAX);
        NRFradio.startConstCarrier(RF24_PA_MAX, 50);

        if (!NRFradio.setDataRate(RF24_2MBPS)) Serial.println("Fail setting data Rate");
        NRFSPI = 1;
    }

    int startChannel = 0;
    int stopChannel = 80;
    int stepSize = 2;

    int menuIndex = 0;
    bool redraw = true;
    bool editMode = false;

    bool runJammer = false;
    bool hopmenu = true;

    vTaskDelay(350 / portTICK_PERIOD_MS);
    NRFSerial.println("RADIOS");
    vTaskDelay(100 / portTICK_PERIOD_MS);

    while (hopmenu) {
        if (CHECK_NRF_UART(mode) || CHECK_NRF_BOTH(mode)) {
            if (NRFSerial.available()) {
                String incomingNRFs = NRFSerial.readStringUntil('\n');
                incomingNRFs.trim();
                if (incomingNRFs.length() == 1 && isDigit(incomingNRFs.charAt(0))) {
                    NRFOnline = (incomingNRFs.toInt());
                    if (CHECK_NRF_BOTH(mode)) {
                         NRFOnline = (incomingNRFs.toInt()) + NRFSPI;
                    }
                    redraw = true;
                }
            }
        }

        if (redraw) {
            drawMainBorder();
            tft.setCursor(10, 35);
            tft.setTextSize(FM);
            tft.println("NRF Hopper Config");

            tft.setCursor(10, 70);
            tft.printf("Start : CH %d", startChannel);
            tft.setCursor(10, 90);
            tft.printf("Stop  : CH %d", stopChannel);
            tft.setCursor(10, 110);
            tft.printf("Step  : %d mhz", stepSize);
            tft.setCursor(10, 130);
            tft.print("Start Jammer");
            tft.setCursor(10, 150);
            tft.print("Exit");

            int yHighlight;
            if (menuIndex == 0) yHighlight = 70;
            if (menuIndex == 1) yHighlight = 90;
            if (menuIndex == 2) yHighlight = 110;
            if (menuIndex == 3) yHighlight = 130;
            if (menuIndex == 4) yHighlight = 150;

            tft.drawRect(5, yHighlight - 2, tftWidth - 10, 18, bruceConfig.priColor);
            if (CHECK_NRF_UART(mode) || CHECK_NRF_BOTH(mode)) {
                 NRFSerial.println("HOPPER_" + String(startChannel) + "_" + String(stopChannel) + "_" + String(stepSize));
            }
            redraw = false;
        }

        if (check(EscPress)) {
            hopmenu = false;
            return;
        }

        if (check(NextPress)) {
            if (editMode) {
                if (menuIndex == 0) startChannel = (startChannel % 125) + 1;
                if (menuIndex == 1) stopChannel = (stopChannel % 125) + 1;
                if (menuIndex == 2) stepSize = (stepSize % 10) + 1;
            } else {
                menuIndex = (menuIndex + 1) % 5;
            }
            redraw = true;
            vTaskDelay(150 / portTICK_PERIOD_MS);
        }

        if (check(PrevPress)) {
            if (editMode) {
                if (menuIndex == 0) startChannel = (startChannel - 2 + 125) % 125 + 1;
                if (menuIndex == 1) stopChannel = (stopChannel - 2 + 125) % 125 + 1;
                if (menuIndex == 2) stepSize = (stepSize - 2 + 10) % 10 + 1;
            } else {
                menuIndex = (menuIndex - 1 + 5) % 5;
            }
            redraw = true;
            vTaskDelay(150 / portTICK_PERIOD_MS);
        }

        if (check(SelPress)) {
            if (menuIndex == 3 && !editMode) {

                runJammer = true;
                hopmenu = false;
            } else if (menuIndex == 4 && !editMode) {

                hopmenu = false;
                return;
            } else {
                if (menuIndex < 3) editMode = !editMode;
            }
            redraw = true;
            vTaskDelay(150 / portTICK_PERIOD_MS);
        }
    }

    if (runJammer) {
        int channel = startChannel;
        drawMainBorder();
        tft.setCursor(10, 35);
        tft.setTextSize(FM);
        tft.println("NRF Hopper Jammer");
        tft.setCursor(10, 70);
        tft.println("STATUS : " + String(NRFOnline) + " ACTIVE");
        tft.setCursor(10, 90);
        tft.printf("Range : %d - %d", startChannel, stopChannel);
        tft.setCursor(10, 110);
        tft.printf("Step  : %d", stepSize);

        while (!check(EscPress)) {
            channel += stepSize;
            if (channel > stopChannel) channel = startChannel;
            if (CHECK_NRF_SPI(mode)) NRFradio.setChannel(channel);
        }

        if (CHECK_NRF_SPI(mode)) NRFradio.stopConstCarrier();
        if (CHECK_NRF_UART(mode) || CHECK_NRF_BOTH(mode)) {
             NRFSerial.println("OFF");
        }
        Serial.println("Jammer Stopped");
    }
}
