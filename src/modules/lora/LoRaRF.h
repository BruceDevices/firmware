
// #ifndef __LORA_MENU_H__
// #define __LORA_MENU_H__
#ifndef __LORA_RF_H__
#define __LORA_RF_H__
#if !defined(LITE_VERSION)
#include "HWCDC.h"
#include <RadioLib.h>
#include <vector>

// Shared Globals
extern SPIClass *loraSpi;
extern Module *loraModule;
extern SX1276 *lora1276;
extern SX1278 *lora1278;
extern SX1262 *lora1262;
extern volatile bool loraPacketReceived;
extern volatile bool loraInterruptEnabled;

enum class LoRaRadioVariant { SX1278, SX1276, SX1262 };
extern LoRaRadioVariant loraRadioVariant;

void lorachat();
void loraconf();
void sndfile();
void meshtastic();
void jammyjammer();
// void selectRadioVariant(JsonDocument &doc);

bool startLoraRadio(float bandMHz);
void check(bool &flag); // assuming needed?

#endif
#endif
