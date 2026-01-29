#include "core/powerSave.h"
#include <interface.h>

/*
 * ESP-WROOM-32 U.FL Board Interface Implementation
 * 
 * This board is a generic ESP-WROOM-32 dev board with external U.FL antenna.
 * It has no built-in display, so it defaults to WebUI mode for interaction.
 * 
 * Features:
 * - No built-in screen (uses WebUI)
 * - No built-in battery
 * - Boot button on GPIO0 for input
 * - LED on GPIO2
 */

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   Initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    // Start WebUI by default since this board has no display
    bruceConfig.startupApp = "WebUI";
    
    // Configure boot button as input
    pinMode(BTN_PIN, INPUT_PULLUP);
    
    // Configure onboard LED
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_OFF);
    
    // Initialize Serial for debugging
    Serial.begin(115200);
    
    // Set default modules configuration
    bruceConfigPins.rfModule = CC1101_SPI_MODULE;
    bruceConfigPins.rfidModule = PN532_I2C_MODULE;
    bruceConfigPins.irRx = RXLED;
    bruceConfigPins.irTx = TXLED;
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   Second stage GPIO setup after main initialization
***************************************************************************************/
void _post_setup_gpio() {
    // Nothing additional needed for this board
}

/***************************************************************************************
** Function name: getBattery()
** Location: display.cpp
** Description:   Returns battery level 0-100. This board has no battery monitoring.
***************************************************************************************/
int getBattery() { 
    return 0; // No battery monitoring available
}

/***************************************************************************************
** Function name: isCharging()
** Description:   Returns charging status. This board has no battery.
***************************************************************************************/
bool isCharging() { 
    return false; 
}

/*********************************************************************
** Function: setBrightness
** Location: settings.cpp
** Description: Set display brightness - no effect on this board (no display)
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    // No display to adjust brightness
}

/*********************************************************************
** Function: InputHandler
** Description: Handles button input. This board only has the boot button.
**********************************************************************/
void InputHandler(void) {
    checkPowerSaveTime();
    
    // Reset all button states
    PrevPress = false;
    NextPress = false;
    SelPress = false;
    AnyKeyPress = false;
    EscPress = false;
    
    // Check boot button (GPIO0)
    static unsigned long lastPress = 0;
    static bool buttonDown = false;
    
    if (digitalRead(BTN_PIN) == BTN_ACT) {
        if (!buttonDown) {
            buttonDown = true;
            lastPress = millis();
        } else if (millis() - lastPress > 1000) {
            // Long press = Escape/Back
            if (!wakeUpScreen()) EscPress = true;
            buttonDown = false;
        }
    } else {
        if (buttonDown) {
            if (millis() - lastPress < 1000) {
                // Short press = Select
                if (!wakeUpScreen()) {
                    SelPress = true;
                    AnyKeyPress = true;
                }
            }
            buttonDown = false;
        }
    }
}

/*********************************************************************
** Function: powerOff
** Location: mykeyboard.cpp
** Description: Turns off the device. ESP32 doesn't have true power off.
**********************************************************************/
void powerOff() {
    // ESP32 deep sleep as pseudo power off
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: checkReboot
** Location: mykeyboard.cpp
** Description: Check if reboot is requested (long press boot button)
**********************************************************************/
void checkReboot() {
    // Long press handling is done in InputHandler
}
