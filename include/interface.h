#pragma once
#include <Arduino.h>
#include <vector>

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio();

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio();

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery();


/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval);


/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void);

/*********************************************************************
** Function: pollEncoder
** location: interface.cpp (per board)
** Samples the rotary encoder A/B lines unconditionally, every task tick,
** decoupled from AnyKeyPress consumption -- mirrors how the Flipper
** port's encoder_poll() is never gated behind whether the previous
** input event was consumed. No-op on boards without HAS_ENCODER.
**********************************************************************/
void __attribute__((weak)) pollEncoder(void);


/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff();

/*********************************************************************
** Function: goToDeepSleep
** location: mykeyboard.cpp
** Puts the device into DeepSleep
**********************************************************************/
void goToDeepSleep();

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to turnoff the device (name is odd btw)
**********************************************************************/
void checkReboot();

/***************************************************************************************
** Function name: isCharging()
** location: interface.cpp
** Description:   Determines if the device is charging
***************************************************************************************/
bool isCharging();
