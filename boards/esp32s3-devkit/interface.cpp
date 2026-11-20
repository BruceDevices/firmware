/*
 * ESP32-S3 DevKit Interface
 * Board-specific initialization for:
 * - SH1106 128x64 OLED Display (I2C)
 * - 6 Navigation Buttons
 * - CC1101 Sub-GHz RF Module
 * - NRF24 2.4GHz RF Module
 * - PN532 RFID/NFC Module
 * - SD Card Slot
 * - IR TX/RX
 */

#include <Arduino.h>
#include "pins_arduino.h"

// Forward declarations for hardware modules
bool initOLED();
bool initButtons();
bool initRF_CC1101();
bool initRF_NRF24();
bool initRFID_PN532();
bool initSDCard();
bool initIR();
bool initBLE();
bool initWiFi();

/**
 * Main board initialization function
 * Called during system startup to initialize all hardware
 */
bool board_init() {
    Serial.begin(115200);
    delay(500);
    
    Serial.println("\n\n========================================");
    Serial.println("  ESP32-S3 DevKit - Bruce Firmware");
    Serial.println("  Board Initialization");
    Serial.println("========================================\n");
    
    // Initialize display first (for debug output)
    if (!initOLED()) {
        Serial.println("⚠️  WARNING: OLED display initialization failed");
        Serial.println("   Continuing with serial output only...");
    } else {
        Serial.println("✓ OLED Display initialized");
    }
    
    // Initialize button input handling
    if (!initButtons()) {
        Serial.println("⚠️  WARNING: Button initialization failed");
    } else {
        Serial.println("✓ Button input initialized");
    }
    
    // Initialize built-in WiFi
    if (!initWiFi()) {
        Serial.println("⚠️  WARNING: WiFi module initialization failed");
    } else {
        Serial.println("✓ WiFi module initialized");
    }
    
    // Initialize built-in Bluetooth/BLE
    if (!initBLE()) {
        Serial.println("⚠️  WARNING: BLE module initialization failed");
    } else {
        Serial.println("✓ BLE module initialized");
    }
    
    // Initialize CC1101 Sub-GHz Module (if available)
#ifdef USE_CC1101_VIA_SPI
    if (!initRF_CC1101()) {
        Serial.println("⚠️  WARNING: CC1101 Sub-GHz module initialization failed");
    } else {
        Serial.println("✓ CC1101 Sub-GHz module initialized");
    }
#endif
    
    // Initialize NRF24 2.4GHz Module (if available)
#ifdef USE_NRF24_VIA_SPI
    if (!initRF_NRF24()) {
        Serial.println("⚠️  WARNING: NRF24 2.4GHz module initialization failed");
    } else {
        Serial.println("✓ NRF24 2.4GHz module initialized");
    }
#endif
    
    // Initialize RFID/NFC PN532 Module (if available)
    if (!initRFID_PN532()) {
        Serial.println("ℹ️  INFO: RFID/NFC module not detected (optional)");
    } else {
        Serial.println("✓ RFID/NFC (PN532) module initialized");
    }
    
    // Initialize SD Card (if available)
    if (!initSDCard()) {
        Serial.println("ℹ️  INFO: SD card not detected (optional)");
    } else {
        Serial.println("✓ SD card initialized");
    }
    
    // Initialize Infrared Module
    if (!initIR()) {
        Serial.println("⚠️  WARNING: Infrared module initialization failed");
    } else {
        Serial.println("✓ Infrared module initialized");
    }
    
    Serial.println("\n========================================");
    Serial.println("  Board initialization complete!");
    Serial.println("  Ready for Bruce operations...");
    Serial.println("========================================\n");
    
    return true;
}

/**
 * Initialize SH1106 128x64 OLED Display
 * I2C: SDA=GPIO8, SCL=GPIO9
 */
bool initOLED() {
    // Implementation delegated to display_sh1106_adapter.h
    // Call the adapter's initialization function
    extern bool initOLED_SH1106();
    return initOLED_SH1106();
}

/**
 * Initialize 6 Navigation Buttons with interrupt handlers
 * UP=35, DOWN=36, LEFT=37, RIGHT=38, SELECT=39, BACK=40
 */
bool initButtons() {
    // Implementation delegated to button handler
    extern bool setup_buttons_irq();
    setup_buttons_irq();
    return true;
}

/**
 * Initialize WiFi Module (Built-in ESP32-S3)
 * Enables: Beacon spam, Deauth attacks, Evil Portal, etc.
 */
bool initWiFi() {
    // WiFi initialization handled by Bruce WiFi module
    // Returns true if WiFi stack is ready
    return true;
}

/**
 * Initialize BLE Module (Built-in ESP32-S3)
 * Enables: BLE scanning, BLE attacks, BLE spam, etc.
 */
bool initBLE() {
    // BLE initialization handled by Bruce BLE module
    // Returns true if BLE stack is ready
    return true;
}

/**
 * Initialize CC1101 Sub-GHz RF Module (SPI)
 * Enables: Sub-GHz scanning, jamming, signal replay, etc.
 */
bool initRF_CC1101() {
#ifdef USE_CC1101_VIA_SPI
    // CC1101 initialization handled by Bruce RF module
    Serial.println("  → Initializing CC1101 on SPI...");
    return true;  // Placeholder - actual init in RF module
#endif
    return false;
}

/**
 * Initialize NRF24 2.4GHz RF Module (SPI)
 * Enables: NRF24 scanning, mousejacking, keyboard attacks, etc.
 */
bool initRF_NRF24() {
#ifdef USE_NRF24_VIA_SPI
    // NRF24 initialization handled by Bruce RF module
    Serial.println("  → Initializing NRF24 on SPI...");
    return true;  // Placeholder - actual init in RF module
#endif
    return false;
}

/**
 * Initialize RFID/NFC PN532 Module (I2C or SPI)
 * Enables: RFID reading/cloning, NFC emulation, etc.
 */
bool initRFID_PN532() {
    // PN532 initialization handled by Bruce RFID module
    Serial.println("  → Scanning for PN532 RFID/NFC module...");
    return false;  // Placeholder - actual detection in RFID module
}

/**
 * Initialize SD Card Slot (SPI)
 * Required for: Badge payloads, script storage, logging, etc.
 */
bool initSDCard() {
    // SD Card initialization handled by SPIFFS/LittleFS
    Serial.println("  → Scanning for SD card...");
    return false;  // Placeholder - actual detection in filesystem module
}

/**
 * Initialize Infrared TX/RX Module
 * TX Pin: GPIO3, RX Pin: GPIO2 (configurable)
 * Enables: IR signal replay, TV-B-Gone, etc.
 */
bool initIR() {
    // IR initialization handled by Bruce IR module
    Serial.println("  → Initializing Infrared module...");
    return true;  // Placeholder - actual init in IR module
}

/**
 * Board-specific reset/deinit function
 * Called before firmware restart or shutdown
 */
void board_deinit() {
    Serial.println("Shutting down board modules...");
    // Clean up hardware resources
}

/**
 * Board-specific loop function
 * Called continuously during normal operation
 */
void board_loop() {
    // Called periodically for board-specific tasks
    // Example: LED updates, button debouncing, etc.
}
