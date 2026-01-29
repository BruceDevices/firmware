#ifndef __DUCKY_TYPER_H
#define __DUCKY_TYPER_H
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
extern uint8_t _Ask_for_restart;
// Start badUSB or badBLE ducky runner
void ducky_setup(HIDInterface *&hid, bool ble = false);

// Setup the keyboard for badUSB or badBLE
void ducky_startKb(HIDInterface *&hid, bool ble);

// Parses a file to run in the badUSB
void key_input(FS fs, String bad_script, HIDInterface *hid);

// Sends a simple command through USB
void key_input_from_string(String text);

// Use device as a keyboard (USB or BLE)
void ducky_keyboard(HIDInterface *&hid, bool ble = false);

// Send media commands through BLE or USB HID
void MediaCommands(HIDInterface *hid, bool ble = false);

// Presenter mode - press button to advance slides
void PresenterMode(HIDInterface *&hid, bool ble = true);

#endif
