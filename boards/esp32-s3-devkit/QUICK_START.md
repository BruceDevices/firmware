# ESP32-S3 DevKit OLED - Quick Start Guide

## What You Have

✅ Complete board support for ESP32-S3 DevKit  
✅ SH1106 OLED driver (128x64)  
✅ 6 navigation buttons (UP, DOWN, LEFT, RIGHT, SELECT, BACK)  
✅ Arduino IDE & PlatformIO support  
✅ Test sketches included  

## Hardware Connections (5 minutes)

### OLED Display (I2C)
```
OLED      ESP32-S3
---       --------
GND   →   GND
VCC   →   3.3V
SDA   →   GPIO 8
SCL   →   GPIO 9
```

### Navigation Buttons
All buttons connect to **GND** when pressed (INPUT_PULLUP mode)

```
GPIO 0  →  UP Button
GPIO 1  →  DOWN Button
GPIO 2  →  LEFT Button  
GPIO 4  →  RIGHT Button
GPIO 5  →  SELECT Button
GPIO 6  →  BACK Button
```

## Software Setup (Choose One)

### Option A: Arduino IDE (Easiest for Beginners)

**1. Install Arduino IDE**
- Download from https://www.arduino.cc/en/software
- Install and run

**2. Add ESP32 Support**
- `File` → `Preferences`
- Under "Additional Boards Manager URLs", add:
  ```
  https://dl.espressif.com/dl/package_esp32_index.json
  ```
- Click OK

**3. Install Board Package**
- `Tools` → `Board Manager`
- Search "ESP32"
- Install "esp32" by Espressif Systems

**4. Install Libraries**
- `Tools` → `Manage Libraries`
- Search and install:
  - "Adafruit SH110X" by Adafruit
  - "Adafruit GFX Library" by Adafruit

**5. Select Board**
- `Tools` → `Board` → `ESP32S3 Dev Module`
- `Tools` → `Port` → Select your COM port
- `Tools` → `Upload Speed` → 921600

**6. Upload Test Sketch**
- Open: `boards/esp32-s3-devkit/test_buttons.ino`
- Click Upload (→ button)
- Open Serial Monitor (115200 baud)
- Press buttons and watch output

### Option B: PlatformIO (Recommended for Full Firmware)

**1. Install VS Code**
- Download from https://code.visualstudio.com/

**2. Install PlatformIO**
- Open VS Code
- Extensions → Search "PlatformIO"
- Install "PlatformIO IDE" by PlatformIO
- Restart VS Code

**3. Clone Repository**
```bash
git clone https://github.com/a-rahman241/firmware.git
cd firmware
git checkout esp32-s3-devkit-oled
```

**4. Open in VS Code**
```bash
code .
```

**5. Build & Upload**
- Click PlatformIO icon (alien head) on left sidebar
- Expand `esp32-s3-devkit` environment
- Click **Build** (checkmark icon)
- Connect ESP32-S3 via USB-C
- Click **Upload** (arrow icon)
- Click **Serial Monitor** to view output

## Testing Checklist

### Display Test
- [ ] OLED shows text on boot
- [ ] Text is clear and readable
- [ ] No flickering or artifacts

### Button Test
- [ ] Press UP button → Serial shows "UP pressed"
- [ ] Press DOWN button → Serial shows "DOWN pressed"
- [ ] Press LEFT button → Serial shows "LEFT pressed"
- [ ] Press RIGHT button → Serial shows "RIGHT pressed"
- [ ] Press SELECT button → Serial shows "SEL pressed"
- [ ] Press BACK button → Serial shows "BACK pressed"

### Full System Test
- [ ] Boot completes successfully
- [ ] Menu appears on OLED
- [ ] Buttons navigate menu
- [ ] No compiler errors
- [ ] No runtime crashes

## Troubleshooting

### OLED Shows Nothing
1. Check I2C connections (GPIO 8 & 9)
2. Verify 3.3V power
3. Try I2C address 0x3D instead of 0x3C
4. Open Serial Monitor - should show init messages

### Upload Fails
1. Hold **BOOT** button on DevKit
2. Press **RESET** button while holding BOOT
3. Release BOOT
4. Try upload again
5. Check USB cable (use quality USB-C)

### Buttons Don't Work
1. Verify GPIO connections to GND through button
2. Check all 6 buttons individually
3. Use Serial Monitor to debug:
   ```cpp
   Serial.println(digitalRead(0));  // Should be 1 (not pressed), 0 (pressed)
   ```

### Compilation Errors
1. Make sure all libraries are installed
2. Verify board is "ESP32S3 Dev Module"
3. Check that you're on the right branch: `esp32-s3-devkit-oled`
4. Try: `Sketch` → `Clean` (then rebuild)

## Next Steps

### After Everything Works

1. **Explore the Menu**
   - Use buttons to navigate
   - Try different options
   - Check what features work

2. **Customize for Your Needs**
   - Edit `boards/esp32-s3-devkit/interface.cpp` to change button behavior
   - Modify `platformio.ini` to enable/disable features
   - Add your own modules

3. **Full Bruce Features**
   - WiFi scanning and attacks
   - BLE operations
   - RF attacks
   - RFID emulation
   - And much more!

## File Locations

```
boards/esp32-s3-devkit/
├── test_buttons.ino          ← Start here! Test sketch
├── platformio.ini            ← Build configuration
├── interface.cpp             ← Button handling & init
├── pins_arduino.h            ← GPIO definitions
├── oled_init.cpp             ← OLED setup
├── QUICK_START.md            ← This file
└── README.md                 ← Detailed docs
```

## Documentation

- **QUICK_START.md** (this file) - Get running fast
- **README.md** - Detailed hardware & features
- **COMPILE_INSTRUCTIONS.md** - Advanced compilation
- **arduino_compile_guide.md** - Arduino IDE detailed guide

## Support Resources

- ESP32-S3 DevKit: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3_devkitc-1.html
- Adafruit SH1106 Library: https://github.com/adafruit/Adafruit_SH110X
- PlatformIO Docs: https://docs.platformio.org/
- Arduino IDE Help: https://www.arduino.cc/en/Guide

## Common Issues & Solutions

| Problem | Solution |
|---------|----------|
| OLED blank | Check I2C pins (GPIO 8, 9) and power (3.3V) |
| Buttons unresponsive | Verify GPIO connections to GND |
| Upload fails | Hold BOOT, press RESET, try again |
| Serial shows garbage | Check baud rate is 115200 |
| Compilation error | Install all required libraries |

## You're Ready!

Your ESP32-S3 DevKit is now ready to run Bruce Predatory Firmware with full OLED display and 6-button navigation.

**Start with the test sketch, verify everything works, then compile the full firmware!**

Happy hacking! 🎯
