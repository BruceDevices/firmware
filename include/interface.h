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

/***************************************************************************************
** Function name: getBatteryVoltage()
** location: display.cpp
** Description:   Returns the current battery voltage (in Volts) if supported
***************************************************************************************/
float getBatteryVoltage();


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
