# ESP32-S3 OLED + Buttons Project

## Overview
This is a complete, Arduino IDE-ready project for the ESP32-S3 DevKit with:
- **Display**: SH1106 128x64 OLED (I2C)
- **Buttons**: 6-button navigation pad (GPIO interrupts)

## Files
- `ESP32_OLED_Buttons.ino` - Main sketch (start here)
- `pins_arduino.h` - GPIO pin definitions
- `display_sh1106_adapter.h` - OLED API header
- `display_sh1106_adapter.cpp` - OLED implementation
- `board_init.cpp` - Board initialization and ISRs

## Setup Instructions

### 1. Arduino IDE Configuration

**Install ESP32 Board Support:**
- Preferences → Additional Boards Manager URLs
- Add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
- Tools → Board Manager → Search "esp32" → Install latest

**Select Board:**
- Tools → Board → ESP32 → **ESP32-S3 Dev Module**

**Configure Settings:**
- USB CDC On Boot: **Enabled**
- USB DFU On Boot: **Disabled**
- CPU Frequency: **240 MHz**
- Flash Size: **16MB** (or match your board)
- Partition Scheme: **Huge APP (3MB No OTA)**
- Upload Speed: **921600**

### 2. Install Libraries

**In Arduino IDE:**
- Sketch → Include Library → Manage Libraries
- Install:
  - **Adafruit GFX** (by Adafruit)
  - **Adafruit SH110X** (by Adafruit)

### 3. Prepare Project

**Option A - Copy All Files:**
1. Download this entire folder
2. Rename folder to `ESP32_OLED_Buttons` (or any name)
3. Open `ESP32_OLED_Buttons.ino` in Arduino IDE
4. Connect ESP32-S3 via USB-C
5. Select Port: Tools → Port → COM# (or /dev/ttyUSB#)
6. Click Upload ✓

**Option B - Quick Test:**
- Just open `ESP32_OLED_Buttons.ino` in IDE → Verify → Upload
- IDE will prompt to save as sketch folder

## Hardware Wiring

### OLED Display (I2C)
```
OLED Pin  →  ESP32-S3
VCC       →  3V3
GND       →  GND
SDA       →  GPIO 8
SCL       →  GPIO 9
```

### Buttons
```
Button    →  GPIO
UP        →  35
DOWN      →  36
LEFT      →  37
RIGHT     →  38
SELECT    →  39
BACK      →  40

All buttons: GND on the other side (active LOW)
```

## Operation

1. Power on ESP32-S3
2. OLED displays "SH1106 initialized" briefly
3. Press any button → Display shows which button was pressed
4. Serial monitor shows debug output at 115200 baud

## Customization

### Change Button Pins
Edit `pins_arduino.h`:
```cpp
#define UP_BTN   35    // Change to your GPIO
#define DOWN_BTN 36
// ... etc
```

### Modify Display Output
Edit `ESP32_OLED_Buttons.ino` `loop()` function

### Add Your Code
Replace or extend the `loop()` function with your application logic

## Troubleshooting

### OLED Not Showing
- Verify I2C address: `Wire.begin(SDA, SCL)` and `sh1106.begin(0x3C, true)`
- Use I2C scanner sketch to find actual address
- Check wiring: SDA=GPIO8, SCL=GPIO9

### Buttons Not Working
- Verify GPIO pins in `pins_arduino.h`
- Check Serial output for ISR triggers
- Ensure buttons connect to GND when pressed

### Upload Issues
- Check USB-C cable quality
- Try different Upload Speed (115200 or 230400)
- Hold BOOT button during upload if stuck

## Serial Output
```
=== ESP32-S3 OLED Button Test ===
Board: esp32s3-devkit
Display: SH1106 128x64 OLED
Initializing...
Board init: esp32s3-devkit
Setup complete! Ready for button input.
UP pressed
DOWN pressed
...
```

## License
Part of the Predatory ESP32 Firmware project
