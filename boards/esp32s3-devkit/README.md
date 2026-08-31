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

Testing
1) Flash boards/esp32s3-devkit/test_buttons.ino to verify wiring and button behavior.
2) Call board_init() from your main setup to integrate the board into the full firmware.
