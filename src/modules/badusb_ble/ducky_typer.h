#ifndef __DUCKY_TYPER_H
#define __DUCKY_TYPER_H
#if !defined(LITE_VERSION)
#include <Arduino.h>
#include <SD.h>
#include <USB.h>
#include <globals.h>

#ifdef USB_as_HID
#include <USBHIDKeyboard.h>
#else
#include <CH9329_Keyboard.h>
#endif
#include <BleKeyboard.h>

extern HIDInterface *hid_usb;
extern HIDInterface *hid_ble;
extern int activeBLEInstances;

struct DuckyCommand;
struct DuckyCommandLookup;
struct DuckyCombination;

// Start badUSB or badBLE ducky runner
void ducky_setup(HIDInterface *&hid, bool ble = false);

// Setup the keyboard for badUSB or badBLE
// functionId: 0=Keyboard, 1=Media, 2=BadUSB, 3=Presenter
void ducky_startKb(HIDInterface *&hid, bool ble, int functionId = 0);

// Parses a file to run in the badUSB
void key_input(FS fs, const String &bad_script, HIDInterface *hid);

// Sends a simple command through USB
void key_input_from_string(const String &text);

// Use device as a keyboard (USB or BLE)
void ducky_keyboard(HIDInterface *&hid, bool ble = false);

// Send media commands through BLE or USB HID
void MediaCommands(HIDInterface *hid, bool ble = false);

DuckyCommandLookup *findDuckyCommand(const char *cmd);
DuckyCombination *findDuckyCombination(const char *cmd);

void sendAltChar(HIDInterface *hid, uint8_t charCode);
void sendAltString(HIDInterface *hid, const String &text);

void printHeaderBadUSBBLE(const String &bad_script);
void printStatusBadUSBBLE(const String &status);
void printTFTBadUSBBLE(const String &text, uint16_t color = TFT_WHITE, bool newline = false);

void printDecimalTime(uint32_t milliseconds);

bool waitForButtonPress();
bool handlePauseResume();

// Presenter mode - press button to advance slides
void PresenterMode(HIDInterface *&hid, bool ble = true);

// Shared cleanup for ducky_typer BLE functions - cleans a specific instance
void cleanupDuckyBLE(HIDInterface *&hid);

// Double cleanup with cooling delay
void safeCleanupDuckyBLE(HIDInterface *&hid);

#endif
#endif
