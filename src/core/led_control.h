#ifndef __LED_CONTROL_H__
#define __LED_CONTROL_H__
#include <globals.h>

#ifdef HAS_RGB_LED
#include <Arduino.h>
#include <FastLED.h>

#define LED_EFFECT_SOLID 0
#define LED_COLOR_BREATHE 1
#define LED_EFFECT_COLOR_CYCLE 2
#define LED_EFFECT_COLOR_WHEEL 3
#define LED_EFFECT_CHASE 4
#define LED_EFFECT_CHASE_TAIL 5
#define LED_EFFECT_RAINBOW_CHASE 6
#define LED_EFFECT_DISCO 7
#define LED_EFFECT_FIRE 8
#define LED_EFFECT_RAINBOW_BREATHE 9

CRGB hsvToRgb(uint16_t h, uint8_t s, uint8_t v);
uint32_t alterOneColorChannel(uint32_t color, uint16_t newR, uint16_t newG, uint16_t newB);

void beginLed();
void blinkLed(int blinkTime = 50);

void setLedColor(CRGB color);
void setLedEffect(int effect);
void setLedColorConfig();
void setCustomColorMenu();
void setCustomColorSettingMenuR();
void setCustomColorSettingMenuG();
void setCustomColorSettingMenuB();
void setLedEffectConfig();
void setLedEffectSpeedConfig();
void setLedEffectDirectionConfig();
void ledSetup();
void ledEffects(bool enable);
void ledPreviewMode(bool enable);
void setLedBrightness(int value);
void setLedBrightnessConfig();

#else
inline void blinkLed(int blinkTime = 50) {};
#endif

#endif
