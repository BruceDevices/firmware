#include "pins_arduino.h"
#include "display_sh1106_adapter.h"
#include "board_init.cpp"
#include "display_sh1106_adapter.cpp"

// ============================================
// Global Button State Variables
// ============================================
volatile bool UpPress = false;
volatile bool DownPress = false;
volatile bool PrevPress = false;  // LEFT button
volatile bool NextPress = false;  // RIGHT button
volatile bool SelPress = false;   // SELECT button
volatile bool EscPress = false;   // BACK button
volatile bool AnyKeyPress = false;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n=== ESP32-S3 OLED Button Test ===");
    Serial.println("Board: esp32s3-devkit");
    Serial.println("Display: SH1106 128x64 OLED");
    Serial.println("Initializing...");
    
    // Initialize board (OLED + buttons)
    board_init();
    
    Serial.println("Setup complete! Ready for button input.");
}

void loop() {
    // Check for button presses and display status
    if (AnyKeyPress) {
        displayClear();
        displaySetCursor(0, 0);
        displayPrint("Button Pressed:");
        
        int line = 1;
        if (UpPress) {
            displaySetCursor(0, 8 * line++);
            displayPrint("UP");
            Serial.println("UP pressed");
            UpPress = false;
        }
        if (DownPress) {
            displaySetCursor(0, 8 * line++);
            displayPrint("DOWN");
            Serial.println("DOWN pressed");
            DownPress = false;
        }
        if (PrevPress) {
            displaySetCursor(0, 8 * line++);
            displayPrint("LEFT (PREV)");
            Serial.println("LEFT pressed");
            PrevPress = false;
        }
        if (NextPress) {
            displaySetCursor(0, 8 * line++);
            displayPrint("RIGHT (NEXT)");
            Serial.println("RIGHT pressed");
            NextPress = false;
        }
        if (SelPress) {
            displaySetCursor(0, 8 * line++);
            displayPrint("SELECT");
            Serial.println("SELECT pressed");
            SelPress = false;
        }
        if (EscPress) {
            displaySetCursor(0, 8 * line++);
            displayPrint("BACK (ESC)");
            Serial.println("BACK pressed");
            EscPress = false;
        }
        
        displayDisplay();
        AnyKeyPress = false;
        delay(500);
    }
    
    delay(10);
}
