# ESP32-S3 DevKit (SH1106 + Nav Buttons)

Wiring
- SH1106 0.9" I2C (4-pin):
  - VCC -> 3.3V
  - GND -> GND
  - SDA -> GPIO8
  - SCL -> GPIO9
  - (Reset not used, adapter uses -1)
  - I2C address: 0x3C (common)

- Navigation buttons (use pull-up, connect other side to GND):
  - UP    -> GPIO35
  - DOWN  -> GPIO36
  - LEFT  -> GPIO37
  - RIGHT -> GPIO38
  - SEL   -> GPIO39
  - BACK  -> GPIO40

Notes
- The board_init() function in board_init.cpp initializes the SH1106 and attaches interrupts for the buttons.
- ISRs set the firmware's volatile flags (UpPress, DownPress, SelPress, EscPress, NextPress, PrevPress, AnyKeyPress) so existing menu code in the firmware that uses check() will work unchanged.
- Ensure I2C pull-ups (4.7k recommended) are present on SDA/SCL.
- If your module uses a different I2C address or pins, update pins_arduino.h and display_sh1106_adapter.cpp accordingly.


## Upload using PlatformIO in Visual Studio Code (step-by-step)

This section assumes you have Visual Studio Code installed and the PlatformIO IDE extension added.

Prerequisites
1. Install Visual Studio Code: https://code.visualstudio.com/
2. In VS Code, open Extensions (Ctrl+Shift+X), search for "PlatformIO IDE" and install it.
3. Install Git and clone the repository locally (or use VS Code "Clone Repository").
   - Example: `git clone https://github.com/a77336402-rgb/firmware.git`
4. Make sure you have drivers installed for your ESP32-S3 USB-serial chip (CP210x/CH340) if required.
5. Connect the ESP32-S3 devkit to your computer via a data-capable USB cable.

Open the project in VS Code
1. In VS Code: File → Open Folder... → select the cloned `firmware` folder.
2. In the lower-left or the status bar you should see the PlatformIO icons. If PlatformIO does not start automatically, open the Command Palette (Ctrl+Shift+P) and run `PlatformIO: Home`.
3. Switch to the branch `esp32s3-sh1106-buttons` (the branch containing the board files we added):
   - Using terminal inside VS Code: `git checkout esp32s3-sh1106-buttons`
   - Or use the Source Control view to switch branches.

Ensure PlatformIO environment is configured
- Open `platformio.ini` at the project root. The repo has many board environments; if you do not have an environment for the ESP32‑S3 devkit, add one. Example minimal environment to add to `platformio.ini`:

```ini
[env:esp32s3-devkit]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
lib_deps =
  adafruit/Adafruit GFX Library@^1.0.10
  adafruit/Adafruit SH110X@^1.0.0

build_flags =
  -DARDUINO_BOARD="ESP32S3_DEVKIT"
```

- Save platformio.ini after editing. PlatformIO will download required libraries automatically on build.

Build the firmware
1. In the PlatformIO toolbar (left vertical icon bar) click the Alien head to open PlatformIO.
2. Under "Project Tasks" expand the environment you want to use (choose `esp32s3-devkit` from the list you added or any existing ESP32-S3 env that matches your board).
3. Click `Build` (checkmark). This compiles the firmware. Watch the Output panel for errors.

Upload (flash) to the board
Method A — Using the PlatformIO GUI in VS Code (recommended):
1. Connect the board to your PC via USB.
2. In PlatformIO Project Tasks for the chosen environment, click `Upload` (right arrow). PlatformIO will build (if needed) and upload via esptool.
3. If upload fails with a connection error ("Timed out waiting for packet header"), try one of these:
   - Press and hold the **BOOT** (sometimes labelled IO0) button on the devkit, click Upload, and release BOOT when the uploader begins.
   - Press RESET just before or after clicking Upload (timing varies by board).
   - Try manual boot-mode: hold BOOT, press RESET, release RESET, then release BOOT.

Method B — Use PlatformIO CLI in VS Code terminal
1. Open the integrated terminal (Ctrl+`) in VS Code, ensure you are in the repository folder.
2. Build: `pio run -e esp32s3-devkit`
3. Upload: `pio run -e esp32s3-devkit -t upload` (add `--upload-port /dev/ttyUSB0` or `--upload-port COM3` if needed)

Open the Serial Monitor
1. In Project Tasks for the environment, click `Monitor` to open the serial monitor at the `monitor_speed` baud (115200). Or use the command palette: `PlatformIO: Monitor`.
2. You can also open a terminal and run: `pio device monitor -p /dev/ttyUSB0 -b 115200` (use COMx on Windows).
3. After uploading, you should see boot messages. If you called `board_init()` in your main `setup()`, you will see lines like "Board init: esp32s3-devkit" and "SH1106 initialized" if initialization succeeded.

Quick test using the example sketch
1. Flash `boards/esp32s3-devkit/test_buttons.ino` (open that file in the editor, use the "Upload" button in the upper right of the editor or use the Project Tasks -> Upload while that file is the active file). It prints debug to Serial and shows button state on the OLED.
2. Open the Serial Monitor and press the navigation buttons. You should see messages like "UP pressed" printed and the OLED updated.

Troubleshooting
- Build errors about missing libraries: verify `lib_deps` contains `Adafruit GFX` and `Adafruit SH110X`, then rebuild.
- No display output: check the I2C wiring (SDA/SCL swapped is common). Run an I2C scanner to verify address 0x3C.
- Buttons not detected: make sure one side of each button is connected to the GPIO pin and the other to GND (INPUT_PULLUP is used in software).
- Upload fails multiple times: try different USB cable, different USB port, and the BOOT/RESET sequences described above.

Integrating with the full firmware
- To use this board in the main firmware, call `board_init()` early in your `setup()` so the adapter and ISRs are initialized:

```cpp
void setup() {
  Serial.begin(115200);
  delay(100);
  board_init();
  // rest of setup
}
```

If you'd like, I can add a `platformio.ini` environment entry to the repo automatically, or create a small I2C scanner sketch in the `boards/esp32s3-devkit` folder to help verify wiring — tell me and I'll push it to the branch.