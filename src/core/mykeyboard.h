#pragma once
#include "display.h"
#include <globals.h>

String keyboard(String mytext, int maxSize = 76, String msg = "Type your message:", bool mask_input = false);
String
hex_keyboard(String mytext, int maxSize = 76, String msg = "Type you HEX value:", bool mask_input = false);
String
num_keyboard(String mytext, int maxSize = 76, String msg = "Insert your number:", bool mask_input = false);

// Opens a menu to pick the keyboard language and saves the choice to bruceConfig
void setKeyboardLanguage();

void __attribute__((weak)) powerOff();
void __attribute__((weak)) goToDeepSleep();

void __attribute__((weak)) checkReboot();

// Shortcut logic

keyStroke _getKeyPress(); // This function must be implemented in the interface.h of the device, in order to
                          // return the key pressed to use as shortcut or input in keyboard environment
                          // by using the flag HAS_KEYBOARD

// Core functions, depends on the implementation of the funtions above in the interface.h
void checkShortcutPress();
int checkNumberShortcutPress();
char checkLetterShortcutPress();
