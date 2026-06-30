#include "nrf_jammer.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "nrf_common.h"
#include <globals.h>

static void shuffleChannels(uint8_t *arr, size_t count) {
    for (size_t i = count - 1; i > 0; i--) {
        size_t j = esp_random() % (i + 1);
        uint8_t tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

void nrf_jammer() {
    int OnX = 0;
    uint8_t hopping_mode = 0; // 0: sequential, 1: random
    NRF24_MODE mode = nrf_setMode();
    int NRFOnline = 1;

    byte Test_channels[] = {50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 78, 80, 2,  4,  6,  8,
                            10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48};

    byte wifi_channels[] = {2, 7, 12, 17, 22, 27, 32, 37, 42, 47, 52, 57, 62, 67, 72, 77};

    byte ble_channels[] = {2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                           22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41};

    byte ble_adv_priority[] = {37, 38, 39, 1, 2, 3, 25, 26, 27, 79, 80, 81};

    byte bluetooth_channels[] = {2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
                                 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
                                 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
                                 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65,
                                 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80};
    byte usb_channels[] = {32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70};
    byte video_channels[] = {60, 62, 64, 66,  68,  70,  72,  74,  76,  78,  80,  82,  84,  86,  88,  90, 92,
                             94, 96, 98, 100, 102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124};
    byte rc_channels[] = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39};

    byte full_channels[] = {1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,
                            17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,
                            33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
                            49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,
                            65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,
                            81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,
                            97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112,
                            113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124};
    byte zigbee_channels[] = {
        4,  5,  6,  // ch11
        9,  10, 11, // ch12
        14, 15, 16, // ch13
        19, 20, 21, // ch14
        24, 25, 26, // ch15
        29, 30, 31, // ch16
        34, 35, 36, // ch17
        39, 40, 41, // ch18
        44, 45, 46, // ch19
        49, 50, 51, // ch20
        54, 55, 56, // ch21
        59, 60, 61, // ch22
        64, 65, 66, // ch23
        69, 70, 71, // ch24
        74, 75, 76, // ch25
        79, 80, 81  // ch26
    };

    struct jamMode {
        const char *name;
        byte *channels;
        size_t count;
    };

    jamMode modes[] = {
        {"Test        ", Test_channels,      sizeof(Test_channels) / sizeof(Test_channels[0])          },
        {"WiFi        ", wifi_channels,      sizeof(wifi_channels) / sizeof(wifi_channels[0])          },
        {"BLEch       ", ble_channels,       sizeof(ble_channels) / sizeof(ble_channels[0])            },
        {"BLE Adv Pri ", ble_adv_priority,   sizeof(ble_adv_priority) / sizeof(ble_adv_priority[0])    },
        {"Bluetooth   ", bluetooth_channels, sizeof(bluetooth_channels) / sizeof(bluetooth_channels[0])},
        {"USB         ", usb_channels,       sizeof(usb_channels) / sizeof(usb_channels[0])            },
        {"Video Stream", video_channels,     sizeof(video_channels) / sizeof(video_channels[0])        },
        {"RC          ", rc_channels,        sizeof(rc_channels) / sizeof(rc_channels[0])              },
        {"Zigbee      ", zigbee_channels,    sizeof(zigbee_channels) / sizeof(zigbee_channels[0])      },
        {"Full        ", full_channels,      sizeof(full_channels) / sizeof(full_channels[0])          }
    };

    if (nrf_start(mode)) {

        int modeIndex = 0;
        int hopIndex = 0;
        bool redraw = true;
        bool need_reshuffle = true;
        uint8_t shuffled_idx[124];
        if (CHECK_NRF_SPI(mode)) {
            NRFradio.setPALevel(RF24_PA_MAX);
            NRFradio.startConstCarrier(RF24_PA_MAX, 50);
            NRFradio.setAddressWidth(5);
            NRFradio.setPayloadSize(2);
            if (!NRFradio.setDataRate(RF24_2MBPS)) {
                Serial.println("Failed to set data rate to 2Mbps, trying 1Mbps");
                if (!NRFradio.setDataRate(RF24_1MBPS)) {
                    Serial.println("Failed to set data rate to 1Mbps, trying 250kbps");
                    if (!NRFradio.setDataRate(RF24_250KBPS)) {
                        Serial.println("Failed to set data rate to 250kbps, giving up");
                    }
                }
            }
        }

        drawMainBorder();

        NRFSerial.println("RADIOS");
        vTaskDelay(50 / portTICK_PERIOD_MS);

        while (true) {

            if ((CHECK_NRF_UART(mode)) || (CHECK_NRF_BOTH(mode))) {
                if (OnX == 0) {
                    NRFSerial.println("RADIOS");
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                }

                if (NRFSerial.available()) {
                    String incomingNRFs = NRFSerial.readStringUntil('\n');
                    incomingNRFs.trim();
                    if (incomingNRFs.length() == 1 && isDigit(incomingNRFs.charAt(0))) {
                        OnX = 1;
                        NRFOnline = (incomingNRFs.toInt());
                        if (CHECK_NRF_BOTH(mode)) { NRFOnline = (incomingNRFs.toInt()) + 1; }
                        redraw = true;
                    }
                }
            }

            if (redraw) {
                drawMainBorderWithTitle("NRF JAMMER", false);
                printSubtitle("NRF function Jammer");
                padprintln("STATUS : " + String(NRFOnline) + " ACTIVE");
                String _modeName = String(modes[modeIndex].name) + "            ";
                _modeName = _modeName.substring(0, 13);
                padprintln("MODE : " + _modeName);
                padprintln("HOP  : " + String(hopping_mode == 0 ? "Sequential " : "FHSS        "));
                padprintln("");
                padprintln("> Switch Mode: Next/Prev");
                padprintln("> Hop Mode: Sel");
                padprintln("> Exit: Esc");

                tft.drawRoundRect(5, 5, tftWidth - 10, tftHeight - 10, 5, bruceConfig.priColor);
                if ((CHECK_NRF_UART(mode)) || (CHECK_NRF_BOTH(mode))) {
                    String Mode = modes[modeIndex].name;
                    Mode.replace(" ", "");
                    NRFSerial.println(Mode);
                }
                redraw = false;
            }

            hopIndex++;
            if (hopIndex >= (int)modes[modeIndex].count) {
                hopIndex = 0;
                need_reshuffle = true;
            }
            uint8_t ch_idx;
            if (hopping_mode == 1) {
                if (need_reshuffle) {
                    size_t cnt = modes[modeIndex].count;
                    for (size_t i = 0; i < cnt; i++) shuffled_idx[i] = i;
                    shuffleChannels(shuffled_idx, cnt);
                    need_reshuffle = false;
                }
                ch_idx = shuffled_idx[hopIndex];
            } else {
                ch_idx = hopIndex;
            }
            if (CHECK_NRF_SPI(mode)) { NRFradio.setChannel(modes[modeIndex].channels[ch_idx]); }

            if (check(NextPress)) {
                modeIndex++;
                if (modeIndex >= (int)(sizeof(modes) / sizeof(modes[0]))) modeIndex = 0;
                hopIndex = 0;
                need_reshuffle = true;
                redraw = true;
            }
            if (check(PrevPress)) {
                modeIndex--;
                if (modeIndex < 0) modeIndex = (sizeof(modes) / sizeof(modes[0])) - 1;
                hopIndex = 0;
                need_reshuffle = true;
                redraw = true;
            }
            if (check(EscPress)) break;
            if (check(SelPress)) {
                hopping_mode++;
                if (hopping_mode > 1) hopping_mode = 0;
                hopIndex = 0;
                need_reshuffle = true;
                redraw = true;
            }
        }

        if (CHECK_NRF_SPI(mode)) NRFradio.stopConstCarrier();
        if ((CHECK_NRF_UART(mode)) || (CHECK_NRF_BOTH(mode))) { NRFSerial.println("OFF"); }

    } else {
        displayError("NRF24 not found", true);
    }
}
