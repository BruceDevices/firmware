#include "led_control.h"

#include "core/display.h"
#include "core/utils.h"
#include <globals.h>
#ifdef HAS_RGB_LED
#define FASTLED_RMT_BUILTIN_DRIVER 1  // Use the ESP32 RMT built-in driver
#define FASTLED_RMT_MAX_CHANNELS 1    // Maximum number of RMT channels
#define FASTLED_ESP32_RMT_CHANNEL_0 0 // Use RMT channel 0 for FastLED
#include <FastLED.h>
#include <driver/rmt_tx.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

CRGB leds[LED_COUNT];

bool isPreviewLed = false;
CRGB previewLedColor;
int previewLedEffect;
int previewLedEffectSpeed;
int previewLedEffectDirection;

CRGB hsvToRgb(uint16_t h, uint8_t s, uint8_t v) {
    uint8_t f = (h % 60) * 255 / 60;
    uint8_t p = (255 - s) * (uint16_t)v / 255;
    uint8_t q = (255 - f * (uint16_t)s / 255) * (uint16_t)v / 255;
    uint8_t t = (255 - (255 - f) * (uint16_t)s / 255) * (uint16_t)v / 255;
    uint8_t r = 0, g = 0, b = 0;
    switch ((h / 60) % 6) {
        case 0:
            r = v;
            g = t;
            b = p;
            break;
        case 1:
            r = q;
            g = v;
            b = p;
            break;
        case 2:
            r = p;
            g = v;
            b = t;
            break;
        case 3:
            r = p;
            g = q;
            b = v;
            break;
        case 4:
            r = t;
            g = p;
            b = v;
            break;
        case 5:
            r = v;
            g = p;
            b = q;
            break;
    }

    CRGB c;
    c.red = r;
    c.green = g;
    c.blue = b;
    return c;
}

uint32_t alterOneColorChannel(uint32_t color, uint16_t newR, uint16_t newG, uint16_t newB) {
    uint8_t r = ((color >> 16) & 0xFF);
    uint8_t g = ((color >> 8) & 0xFF);
    uint8_t b = (color & 0xFF);

    if (newR != 256) r = newR;
    if (newG != 256) g = newG;
    if (newB != 256) b = newB;

    return ((r << 16) | (g << 8) | b);
}

TaskHandle_t ledEffectTaskHandle = NULL;

void ledEffectTask(void *pvParameters) {
    short hueStep = 360 / LED_COUNT;
    short offset = 0;
    int currentLED = 0;
    int frame = 0;
    uint64_t start_time = esp_timer_get_time() / 1000;
    while (1) {
        CRGB baseColor = isPreviewLed ? previewLedColor : bruceConfig.ledColor;
        int ledEffect = isPreviewLed ? previewLedEffect : bruceConfig.ledEffect;
        int ledEffectSpeed = isPreviewLed ? previewLedEffectSpeed : bruceConfig.ledEffectSpeed;
        int ledEffectDirection = isPreviewLed ? previewLedEffectDirection : bruceConfig.ledEffectDirection;

        if (ledEffect == LED_EFFECT_COLOR_CYCLE || ledEffect == LED_EFFECT_COLOR_WHEEL) {
            short delayMs = 50;

#ifdef HAS_ENCODER_LED
            if ((ledEffectSpeed == 11 && EncoderLedChange != 0)) {
                offset = (offset + (static_cast<short>(20 / 1000.0f * 360.0f)) * EncoderLedChange) % 360;
                EncoderLedChange = 0;
            } else if (ledEffectSpeed < 11) {
                float speed = 0.2f * ledEffectSpeed;
                offset = (offset + static_cast<short>(speed * delayMs / 1000.0f * 360.0f)) % 360;
            }
#else
            float speed = 0.2f * ledEffectSpeed;
            offset = (offset + static_cast<short>(speed * delayMs / 1000.0f * 360.0f)) % 360;
#endif
            if (ledEffect == LED_EFFECT_COLOR_CYCLE) {
                short hue = ((offset * -ledEffectDirection) % 360 + 360) % 360;
                // Serial.printf("LED Effect Cycle: hue=%d, offset=%d", hue, offset);
                fill_solid(leds, LED_COUNT, hsvToRgb(hue, 255, 255));
            } else if (ledEffect == LED_EFFECT_COLOR_WHEEL) {
                for (uint16_t i = 0; i < LED_COUNT; ++i) {
                    short hue = ((offset + i * -ledEffectDirection * hueStep) % 360 + 360) % 360;
                    // Serial.printf("LED Effect Wheel: i=%d, hue=%d\n", i, hue);
                    leds[i] = hsvToRgb(hue, 255, 255);
                }
            }

        } else if (ledEffect == LED_COLOR_BREATHE) {

            float phase;
#ifdef HAS_ENCODER_LED
            if ((ledEffectSpeed == 11 && EncoderLedChange != 0) || (ledEffectSpeed < 11)) {
                if ((ledEffectSpeed == 11 && EncoderLedChange != 0)) {
                    phase = sinf(frame / 20.0f * PI);
                    frame += EncoderLedChange;
                    EncoderLedChange = 0;
                } else {
                    float time = millis() / 1000.0f;
                    float speed = 0.2f * ledEffectSpeed;
                    phase = sinf(time * speed * PI);
                }
#else
            float time = millis() / 1000.0f;
            float speed = 0.2f * ledEffectSpeed;
            phase = sinf(time * speed * PI);
#endif
                uint8_t value = (uint8_t)((phase + 1.0f) * 127.5f);

                for (int i = 0; i < LED_COUNT; i++) {
                    leds[i] = CRGB(
                        (baseColor.r * value) / 255, (baseColor.g * value) / 255, (baseColor.b * value) / 255
                    );
                }
#ifdef HAS_ENCODER_LED
            }
#endif

#if LED_COUNT > 1
        } else if (ledEffect == LED_EFFECT_CHASE || ledEffect == LED_EFFECT_CHASE_TAIL) {
            uint8_t cycleFrames = 11 - ledEffectSpeed;

#ifdef HAS_ENCODER_LED
            if ((ledEffectSpeed == 11 && EncoderLedChange != 0) ||
                (ledEffectSpeed < 11 && frame % cycleFrames == 0)) {
                if ((ledEffectSpeed == 11 && EncoderLedChange != 0)) {
                    currentLED = (currentLED + EncoderLedChange + LED_COUNT) % LED_COUNT;
                    EncoderLedChange = 0;
                } else {
                    currentLED = (currentLED + ledEffectDirection + LED_COUNT) % LED_COUNT;
                }
#else
            if (frame % cycleFrames == 0) {
                currentLED = (currentLED + ledEffectDirection + LED_COUNT) % LED_COUNT;
#endif

                fill_solid(leds, LED_COUNT, CRGB::Black);

                if (ledEffect == LED_EFFECT_CHASE) {
                    leds[currentLED] = baseColor;
                } else {
                    for (int i = 1; i < LED_COUNT; ++i) {
                        int index = (currentLED - ledEffectDirection * i + LED_COUNT) % LED_COUNT;

                        float fade = powf(0.6f, i);
                        leds[index].r = baseColor.r * fade;
                        leds[index].g = baseColor.g * fade;
                        leds[index].b = baseColor.b * fade;
                    }
                }
            }
            frame++;
#endif
        }

        FastLED.show();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void beginLed() {
#ifdef RGB_LED_CLK
    FastLED.addLeds<LED_TYPE, RGB_LED, RGB_LED_CLK, LED_ORDER>(leds, LED_COUNT);
#else
    FastLED.addLeds<LED_TYPE, RGB_LED, LED_ORDER>(leds, LED_COUNT);
#endif

    /* The default FastLED driver takes over control of the RMT interrupt
     * handler, making it hard to use the RMT device for other
     * (non-FastLED) purposes. You can change it's behavior to use the ESP
     * core driver instead, allowing other RMT applications to
     * co-exist. To switch to this mode, add the following directive
     * before you include FastLED.h:
     *
     *      #define FASTLED_RMT_BUILTIN_DRIVER 1
     *  RMT is also used for RF Spectrum (and for RF readings in the future),
     *  So it is needed to restart the driver in case it had been turned off
     *  by the RF functions, in this case, we are restarting it all the time
     */
    // -- RMT configuration for transmission

    // These configurations made T-Embed (non CC1101) stop working
    // Commented to test if with the FASTLED_RMT_MAX_CHANNELS 1 was sufficient for the other devices to
    // work LED and RF Spectrum and RAW capture and it is working well without it for now.. So I'll keep
    // the code below for the case we find some issue and need to rollback

    /*
        rmt_config_t rmt_tx;
        memset(&rmt_tx, 0, sizeof(rmt_config_t));
        rmt_tx.channel = rmt_channel_t(FASTLED_ESP32_RMT_CHANNEL_0);
        rmt_tx.rmt_mode = RMT_MODE_TX;
        rmt_tx.gpio_num = (gpio_num_t)RGB_LED;
        rmt_tx.mem_block_num = 2;
        rmt_tx.clk_div = 2;
        rmt_tx.tx_config.loop_en = false;
        rmt_tx.tx_config.carrier_level = RMT_CARRIER_LEVEL_LOW;
        rmt_tx.tx_config.carrier_en = false;
        rmt_tx.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
        rmt_tx.tx_config.idle_output_en = true;

        // -- Apply the configuration
        rmt_config(&rmt_tx);
        rmt_driver_uninstall(rmt_channel_t(FASTLED_ESP32_RMT_CHANNEL_0));
        rmt_driver_install(rmt_channel_t(FASTLED_ESP32_RMT_CHANNEL_0), 0, 0);
    */
    ledSetup();

    setLedBrightness(bruceConfig.ledBright);
}

void blinkLed(int blinkTime) {
    if (!bruceConfig.ledBlinkEnabled) return;

    int ledBrightFrom = bruceConfig.ledBright;
    int ledBrightTo = ledBrightFrom > 0 ? 0 : 50;

    beginLed();
    setLedBrightness(ledBrightTo);
    ioExpander.turnPinOnOff(IO_EXP_VIBRO, HIGH);
    delay(blinkTime);
    setLedBrightness(ledBrightFrom);
    ioExpander.turnPinOnOff(IO_EXP_VIBRO, LOW);
}

void setLedColor(CRGB color) {
    if (isPreviewLed && previewLedEffect != LED_EFFECT_SOLID) {
        previewLedColor = color;
#ifdef HAS_ENCODER_LED
        EncoderLedChange = 1;
#endif
    } else {
        for (int i = 0; i < LED_COUNT; i++) leds[i] = color;
        FastLED.show();
    }
}

void setLedEffect(int effect) {
    previewLedEffect = effect;
#ifdef HAS_ENCODER_LED
    if (isPreviewLed && previewLedEffect != LED_EFFECT_SOLID) { EncoderLedChange = 1; }
#endif
}

void setLedBrightness(int value) {
    value = max(0, min(100, value));
    int bright = 255 * value / 100;
    FastLED.setBrightness(bright);
    FastLED.show();
}

#define BrucePurple 9830500 // Custom purple color for Bruce
// TODO: 3852441 -> 3849837
void setLedColorConfig() {
    ledPreviewMode(true);

    struct ColorMapping {
        const char *name;
        CRGB color;
    };

    constexpr ColorMapping colorMappings[] = {
        {"OFF",        CRGB::Black    },
        {"Default",    BrucePurple    },
        {"White",      CRGB::White    },
        {"Red",        CRGB::Red      },
        {"Orange",     CRGB::OrangeRed},
        {"Yellow",     CRGB::Yellow   },
        {"Lime Green", CRGB::LawnGreen},
        {"Green",      CRGB::Green    },
        {"Cyan",       CRGB::Cyan     },
        {"Blue",       CRGB::Blue     },
        {"Magenta",    CRGB::Magenta  },
        {"Pink",       CRGB::DeepPink },
    };

    while (1) {
        options.clear();
        int idx = sizeof(colorMappings) / sizeof(colorMappings[0]);
        int i = 0;
        static CRGB colorStorage[12];
        for (const auto &mapping : colorMappings) {
            if (bruceConfig.ledColor == mapping.color) { idx = i; }
            colorStorage[i] = mapping.color;

            options.emplace_back(
                mapping.name,
                [=, &mapping]() {
                    // bruceConfig.setLedColor(mapping.color);
                    bruceConfig.setLedColor(
                        ((uint32_t)mapping.color.r << 16) | ((uint32_t)mapping.color.g << 8) |
                        ((uint32_t)mapping.color.b)
                    );
                },
                idx == i,
                [](void *pointer, bool shouldRender) {
                    setLedColor(*(CRGB *)(pointer));
                    return false;
                },
                &colorStorage[i]
            );
            ++i;
        }

        options.push_back(
            {"Custom Color",
             [=]() { setCustomColorMenu(); },
             idx == sizeof(colorMappings) / sizeof(colorMappings[0]),
             [](void *pointer, bool shouldRender) {
                 setLedColor(bruceConfig.ledColor);
                 return false;
             }}
        );

        addOptionToMainMenu();

        int selectedOption = loopOptions(options, idx);
        if (selectedOption == -1 || selectedOption == options.size() - 1) {
            ledPreviewMode(false);
            ledSetup();
            return;
        }
    }
}

void setCustomColorMenu() {
    while (1) {
        options = {
            {"Red Channel",   setCustomColorSettingMenuR},
            {"Green Channel", setCustomColorSettingMenuG},
            {"Blue Channel",  setCustomColorSettingMenuB},
            {"Back",          [=]() {}                  },
        };

        int selectedOption = loopOptions(options);
        if (selectedOption == -1 || selectedOption == options.size() - 1) return;
    }
}

void setCustomColorSettingMenu(int rgb, std::function<uint32_t(uint32_t, int)> colorGenerator) {
    uint32_t originalColor = bruceConfig.ledColor;

    options.clear();

    static auto hoverFunction = [](void *pointer, bool shouldRender) -> bool {
        uint32_t colorToSet = *static_cast<uint32_t *>(pointer);
        setLedColor(colorToSet);
        previewLedColor = CRGB(colorToSet);
        return false;
    };

    static uint32_t colorStorage[(int)(255 / LED_COLOR_STEP) + 1];
    short selectedIndex = 0;
    short colorPart = 0;
    short i = 0;
    short index = 0;

    if (rgb == 1) {
        colorPart = (originalColor >> 16) & 0xFF;
    } else if (rgb == 2) {
        colorPart = (originalColor >> 8) & 0xFF;
    } else {
        colorPart = originalColor & 0xFF;
    }

    while (i <= 255) {
        if (i % LED_COLOR_STEP == 0 || i == 255) {
            uint32_t updatedColor = colorGenerator(originalColor, i);
            colorStorage[index] = updatedColor;

            // Select nearest color step rounding down
            if (colorPart >= i && colorPart < i + LED_COLOR_STEP) selectedIndex = index;

            options.emplace_back(
                String(i),
                [updatedColor]() { bruceConfig.setLedColor(updatedColor); },
                selectedIndex == index,
                hoverFunction,
                &colorStorage[index]
            );
            ++index;
        }
        ++i;
    }

    addOptionToMainMenu();

    int selectedOption = loopOptions(options, MENU_TYPE_SUBMENU, "", selectedIndex);
    if (selectedOption == -1 || selectedOption == options.size() - 1) {
        setLedColor(originalColor);
        return;
    }
}

void setCustomColorSettingMenuR() {
    setCustomColorSettingMenu(1, [](uint32_t baseColor, int i) {
        return alterOneColorChannel(baseColor, i, 256, 256);
    });
}

void setCustomColorSettingMenuG() {
    setCustomColorSettingMenu(2, [](uint32_t baseColor, int i) {
        return alterOneColorChannel(baseColor, 256, i, 256);
    });
}

void setCustomColorSettingMenuB() {
    setCustomColorSettingMenu(3, [](uint32_t baseColor, int i) {
        return alterOneColorChannel(baseColor, 256, 256, i);
    });
}

void setLedEffectConfig() {
    ledPreviewMode(true);

    while (1) {
        options = {
            {"Solid Color",
             [=]() { bruceConfig.setLedEffect(LED_EFFECT_SOLID); },
             bruceConfig.ledEffect == LED_EFFECT_SOLID,
             [](void *pointer,                                                                     bool shouldRender) {
                 setLedEffect(LED_EFFECT_SOLID);
                 setLedColor(bruceConfig.ledColor);
                 return false;
             }                                                                    },
            {"Breathe",
             [=]() { bruceConfig.setLedEffect(LED_COLOR_BREATHE); },
             bruceConfig.ledEffect == LED_COLOR_BREATHE,
             [](void *pointer,                                                                     bool shouldRender) {
                 setLedEffect(LED_COLOR_BREATHE);
                 return false;
             }                                                                    },
            {"Color Cycle",
             [=]() { bruceConfig.setLedEffect(LED_EFFECT_COLOR_CYCLE); },
             bruceConfig.ledEffect == LED_EFFECT_COLOR_CYCLE,
             [](void *pointer,                                                                     bool shouldRender) {
                 setLedEffect(LED_EFFECT_COLOR_CYCLE);
                 return false;
             }                                                                    },
#if LED_COUNT > 1
            {"Color Wheel",
             [=]() { bruceConfig.setLedEffect(LED_EFFECT_COLOR_WHEEL); },
             bruceConfig.ledEffect == LED_EFFECT_COLOR_WHEEL,
             [](void *pointer,                                                                     bool shouldRender) {
                 setLedEffect(LED_EFFECT_COLOR_WHEEL);
                 return false;
             }                                                                    },
            {"Chase",
             [=]() { bruceConfig.setLedEffect(LED_EFFECT_CHASE); },
             bruceConfig.ledEffect == LED_EFFECT_CHASE,
             [](void *pointer,                                                                     bool shouldRender) {
                 setLedEffect(LED_EFFECT_CHASE);
                 return false;
             }                                                                    },
            {"Chase Tail",
             [=]() { bruceConfig.setLedEffect(LED_EFFECT_CHASE_TAIL); },
             bruceConfig.ledEffect == LED_EFFECT_CHASE_TAIL,
             [](void *pointer,                                                                     bool shouldRender) {
                 setLedEffect(LED_EFFECT_CHASE_TAIL);
                 return false;
             }                                                                    },
#endif
            {"Config - Speed",
             setLedEffectSpeedConfig,                                     false,
             [](void *pointer,                                                                     bool shouldRender) {
                 previewLedEffect = bruceConfig.ledEffect;
                 previewLedEffectSpeed = bruceConfig.ledEffectSpeed;
                 previewLedEffectDirection = bruceConfig.ledEffectDirection;
                 return false;
             }                                                                    },
            {"Config - Direction", setLedEffectDirectionConfig,           false, [](void *pointer, bool shouldRender) {
                 previewLedEffect = bruceConfig.ledEffect;
                 previewLedEffectSpeed = bruceConfig.ledEffectSpeed;
                 previewLedEffectDirection = bruceConfig.ledEffectDirection;
                 return false;
             }},
        };

        addOptionToMainMenu();

        int selectedOption = loopOptions(options, bruceConfig.ledEffect);
        if (selectedOption == -1 || selectedOption == options.size() - 1) {
            ledPreviewMode(false);
            ledSetup();
            return;
        }
    }
}

void setLedEffectSpeedConfig() {
    options.clear();

    static auto hoverFunction = [](void *pointer, bool shouldRender) -> bool {
        int speedToSet = *static_cast<int *>(pointer);
        previewLedEffectSpeed = speedToSet + 1;
        return false;
    };

#ifdef HAS_ENCODER_LED
    static int speedStorage[11];
#else
    static int speedStorage[10];
#endif
    int i = 0;
    while (i < 10) {
        speedStorage[i] = i;

        String label = String(i + 1);
        bool isSelected = (bruceConfig.ledEffectSpeed == i + 1);

        options.emplace_back(
            label,
            [i]() { bruceConfig.setLedEffectSpeed(i + 1); },
            (bruceConfig.ledEffectSpeed == i + 1),
            hoverFunction,
            &speedStorage[i]
        );
        ++i;
    }

#ifdef HAS_ENCODER_LED
    speedStorage[10] = 11;
    options.emplace_back(
        "Sync To Encoder",
        []() { bruceConfig.setLedEffectSpeed(11); },
        (bruceConfig.ledEffectSpeed == 11),
        hoverFunction,
        &speedStorage[10]
    );
#endif

    addOptionToMainMenu();

    int selectedOption = loopOptions(options, bruceConfig.ledEffectSpeed - 1);
    if (selectedOption == -1 || selectedOption == options.size() - 1) {
        previewLedEffectSpeed = bruceConfig.ledEffectSpeed;
        return;
    }
}

void setLedEffectDirectionConfig() {
    options = {
        {"Clockwise",
         [=]() { bruceConfig.setLedEffectDirection(1); },
         bruceConfig.ledEffectDirection == 1,
         [](void *pointer, bool shouldRender) {
             previewLedEffectDirection = 1;
             return false;
         }},
        {"Anti-Clockwise",
         [=]() { bruceConfig.setLedEffectDirection(-1); },
         bruceConfig.ledEffectDirection == -1,
         [](void *pointer, bool shouldRender) {
             previewLedEffectDirection = -1;
             return false;
         }},
    };

    addOptionToMainMenu();

    int selectedOption = loopOptions(options, (bruceConfig.ledEffectDirection == 1) ? 0 : 1);
    if (selectedOption == -1 || selectedOption == options.size() - 1) {
        previewLedEffectDirection = bruceConfig.ledEffectDirection;
        return;
    }
}

void ledSetup() {
    if (bruceConfig.ledEffect == LED_EFFECT_SOLID) { ledEffects(false); }

    if (bruceConfig.ledEffect > LED_EFFECT_SOLID) {
        ledEffects(true);
    } else setLedColor(bruceConfig.ledColor);
}

void ledEffects(bool enable) {
    if (enable) {
        if (ledEffectTaskHandle == NULL) {
            xTaskCreate(ledEffectTask, "LedEffect", 2048, NULL, 1, &ledEffectTaskHandle);
        }
    } else {
        if (ledEffectTaskHandle != NULL) {
            vTaskDelete(ledEffectTaskHandle);
            ledEffectTaskHandle = NULL;
        }
    }
}

void ledPreviewMode(bool enable) {
    isPreviewLed = enable;
    if (enable) {
        previewLedColor = bruceConfig.ledColor;
        previewLedEffect = bruceConfig.ledEffect;
        previewLedEffectSpeed = bruceConfig.ledEffectSpeed;
        previewLedEffectDirection = bruceConfig.ledEffectDirection;
    }
    ledEffects(enable);
}

void setLedBrightnessConfig() {
    int idx = 0;
    if (bruceConfig.ledBright == 0) idx = 0;
    else if (bruceConfig.ledBright == 10) idx = 1;
    else if (bruceConfig.ledBright == 25) idx = 2;
    else if (bruceConfig.ledBright == 50) idx = 3;
    else if (bruceConfig.ledBright == 75) idx = 4;
    else if (bruceConfig.ledBright == 100) idx = 5;

    options = {
        {"OFF",
         [=]() { bruceConfig.setLedBright(0); },
         bruceConfig.ledBright == 0,
         [](void *pointer, bool shouldRender) {
             setLedBrightness(0);
             return false;
         }},
        {"10 %",
         [=]() { bruceConfig.setLedBright(10); },
         bruceConfig.ledBright == 10,
         [](void *pointer, bool shouldRender) {
             setLedBrightness(10);
             return false;
         }},
        {"25 %",
         [=]() { bruceConfig.setLedBright(25); },
         bruceConfig.ledBright == 25,
         [](void *pointer, bool shouldRender) {
             setLedBrightness(25);
             return false;
         }},
        {"50 %",
         [=]() { bruceConfig.setLedBright(50); },
         bruceConfig.ledBright == 50,
         [](void *pointer, bool shouldRender) {
             setLedBrightness(50);
             return false;
         }},
        {"75 %",
         [=]() { bruceConfig.setLedBright(75); },
         bruceConfig.ledBright == 75,
         [](void *pointer, bool shouldRender) {
             setLedBrightness(75);
             return false;
         }},
        {"100%",
         [=]() { bruceConfig.setLedBright(100); },
         bruceConfig.ledBright == 100,
         [](void *pointer, bool shouldRender) {
             setLedBrightness(100);
             return false;
         }},
    };
    addOptionToMainMenu();

    loopOptions(options, idx);
    setLedBrightness(bruceConfig.ledBright);
}
#endif
