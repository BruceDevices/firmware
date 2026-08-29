# ESP32-S3 DevKit with SH1106 OLED Display

## Overview
This board configuration adds support for the **ESP32-S3 DevKit** running **Bruce Predatory Firmware** with a **0.96" SH1106 OLED display** and **6 navigation buttons**.

## Hardware Configuration

### Display
- **Type:** SH1106 OLED (128x64 pixels)
- **Interface:** I2C
- **SDA Pin:** GPIO 8
- **SCL Pin:** GPIO 9
- **I2C Address:** 0x3C (default)

### Navigation Buttons
All buttons use **INPUT_PULLUP** mode (active LOW):

| Button | GPIO | Function |
|--------|------|----------|
| UP | 0 | Navigate Up |
| DOWN | 1 | Navigate Down |
| LEFT | 2 | Navigate Previous |
| RIGHT | 4 | Navigate Next |
| SELECT | 5 | Select/Confirm |
| BACK | 6 | Back/Cancel (ESC) |

### UART
- **TX:** GPIO 43
- **RX:** GPIO 44
- **Baud Rate:** 115200

## Installation & Setup

### Option 1: Arduino IDE (Recommended for beginners)

1. **Install Arduino IDE** (2.0 or later)

2. **Add ESP32 board support:**
   - Go to `File` → `Preferences`
   - Add this URL to "Additional Board Manager URLs":
     ```
     https://dl.espressif.com/dl/package_esp32_index.json
     ```
   - Click OK

3. **Install ESP32 board package:**
   - Go to `Tools` → `Board Manager`
   - Search for "ESP32"
   - Install "esp32" by Espressif Systems (latest version)

4. **Select board and port:**
   - `Tools` → `Board` → `ESP32S3 Dev Module`
   - `Tools` → `Port` → Select your COM port

5. **Clone this repository and open the sketch:**
   - Open `src/main.cpp` in Arduino IDE

6. **Upload:**
   - Click the Upload button (→)

### Option 2: PlatformIO (Recommended for advanced users)

1. **Install PlatformIO** in VS Code or PyCharm

2. **Open the project folder** in your IDE

3. **Select the environment:**
   - In PlatformIO, select `esp32-s3-devkit` from the environments list

4. **Build and upload:**
   ```bash
   pio run -e esp32-s3-devkit -t upload
   ```

## Wiring Diagram

```
ESP32-S3 DevKit
┌─────────────────────────────┐
│         SH1106 OLED         │
│  GND ─────────────── GND    │
│  VCC (3.3V) ─────── 3.3V   │
│  SDA (GPIO 8) ────── GPIO 8 │
│  SCL (GPIO 9) ────── GPIO 9 │
└─────────────────────────────┘

Navigation Buttons (all to GND with 10k pull-up):
  GPIO 0 ───── UP Button
  GPIO 1 ───── DOWN Button
  GPIO 2 ───── LEFT Button
  GPIO 4 ───── RIGHT Button
  GPIO 5 ───── SELECT Button
  GPIO 6 ───── BACK Button
```

## Features

- ✅ Full Bruce firmware support
- ✅ 6-button navigation interface
- ✅ SH1106 OLED display (128x64)
- ✅ Button debouncing (50ms)
- ✅ Deep sleep support
- ✅ I2C display communication
- ✅ Arduino IDE & PlatformIO support

## Troubleshooting

### Display not showing anything
1. Check I2C connections (SDA=GPIO8, SCL=GPIO9)
2. Verify OLED address is 0x3C (can be 0x3D on some modules)
3. Check if module is powered (3.3V on VCC pin)

### Buttons not responding
1. Verify GPIO pins are connected to GND through buttons
2. Check pull-up resistors (should be 10k)
3. Ensure buttons are not pressed during power-up

### Upload fails
1. Hold the BOOT button while uploading
2. Check USB cable (use quality USB-C cable)
3. Install CH340 driver if using older DevKit variant

## Library Dependencies

Automatically installed via PlatformIO:
- `adafruit/Adafruit SH110X` - OLED driver
- `adafruit/Adafruit GFX Library` - Graphics library
- All standard Bruce firmware dependencies

## Notes

- This configuration disables battery monitoring, RGB LED, and RF modules to save space
- OLED brightness is not adjustable (hardware limitation)
- Display resolution is 128×64 pixels (fixed)
- All 6 buttons use debouncing for reliable input

## Future Enhancements

- [ ] Add battery voltage monitoring via ADC
- [ ] Add RGB status indicator
- [ ] Implement button long-press detection
- [ ] Add buzzer feedback support
- [ ] Support for SSD1306 display variant

## Contact & Support

For issues specific to this board configuration, please refer to the main Bruce firmware repository.
