# LilyGo T-Deck Pro E-Paper - Bruce Firmware

⚠️ **EXPERIMENTAL SUPPORT** ⚠️

This board configuration is experimental and has significant limitations due to the E-Paper display.

## Hardware Specifications

- **MCU**: ESP32-S3FN16R8 (16MB Flash, 8MB PSRAM)
- **Display**: GDEQ031T10 (3.1" E-Paper, 320x240, UC8253 controller)
- **Touch**: CST328 capacitive touch controller (I2C 0x1A)
- **Keyboard**: TCA8418 QWERTY keyboard controller (I2C 0x34)
- **LoRa**: SX1262 (915/868 MHz)
- **GPS**: MIA-M10Q (UART)
- **4G Modem**: A7682E LTE Cat 1 (optional)
- **Battery**: 1400mAh LiPo with BQ25896 charger and BQ27220 fuel gauge
- **Gyroscope**: BHI260AP (I2C 0x28)
- **SD Card**: MicroSD slot (SPI)

## Pin Configuration

### Display (E-Paper)
- **MOSI**: GPIO 33
- **SCK**: GPIO 36
- **CS**: GPIO 34
- **DC**: GPIO 35
- **BUSY**: GPIO 37
- **RST**: Not connected (-1)

### Touch (CST328)
- **SDA**: GPIO 13
- **SCL**: GPIO 14
- **INT**: GPIO 12
- **RST**: GPIO 45

### Keyboard (TCA8418)
- **SDA**: GPIO 13
- **SCL**: GPIO 14
- **INT**: GPIO 15
- **LED**: GPIO 42

### LoRa (SX1262)
- **MOSI**: GPIO 33
- **MISO**: Not used
- **SCK**: GPIO 36
- **CS**: GPIO 3
- **RST**: GPIO 4
- **DIO1**: GPIO 5
- **BUSY**: GPIO 6

### SD Card
- **MOSI**: GPIO 33
- **SCK**: GPIO 36
- **CS**: GPIO 48

### GPS (MIA-M10Q)
- **TX**: GPIO 16
- **RX**: GPIO 17
- **Baud Rate**: 9600

### Power
- **Power Control**: GPIO 10
- **Battery ADC**: GPIO 4

## Known Limitations

### E-Paper Display Limitations

1. **Slow Refresh Rate**: E-Paper displays take 1-2 seconds to fully refresh
   - Bruce's UI expects fast TFT refresh rates
   - Menus and animations will be very slow and choppy
   - Not suitable for real-time operations

2. **Ghosting**: E-Paper displays show ghosting effects
   - Previous images may remain faintly visible
   - Full screen refreshes needed periodically

3. **Monochrome Only**: E-Paper is black and white
   - No color support (Bruce UI uses colors extensively)
   - Reduced visual feedback

4. **No Backlight**: E-Paper has no backlight
   - Needs external light to be readable
   - Brightness controls do nothing

### Current Implementation Status

✅ **Working:**
- Touch input (CST328)
- Basic GPIO setup
- Power management
- SD Card support
- LoRa module support

⚠️ **Partially Working:**
- Display output (using TFT_eSPI with ST7789 driver as placeholder)
  - **CRITICAL**: E-Paper requires GxEPD2 library, not TFT_eSPI
  - Current config will NOT work correctly with E-Paper
  - This is a temporary workaround for compilation

❌ **Not Implemented:**
- Keyboard input (TCA8418 needs proper driver library)
- 4G Modem (A7682E not supported in Bruce)
- Gyroscope (BHI260AP not used in Bruce)
- Battery fuel gauge (BQ27220 reading)
- Charger status (BQ25896 reading)

## Required Changes for Full E-Paper Support

To properly support the E-Paper display, the following major changes are needed:

1. **Replace TFT_eSPI with GxEPD2**
   - Add `zinggjm/GxEPD2` library dependency
   - Create wrapper layer to adapt GxEPD2 API to Bruce's display API
   - Implement partial refresh strategies to improve performance

2. **Optimize UI for E-Paper**
   - Reduce refresh frequency
   - Simplify animations
   - Use partial refreshes where possible
   - Convert color schemes to high-contrast B&W

3. **Add TCA8418 Keyboard Driver**
   - Implement full TCA8418 driver or add library dependency
   - Map keyboard matrix to ASCII codes
   - Handle function keys and modifiers

4. **Add Battery Management**
   - Implement BQ27220 fuel gauge reading
   - Implement BQ25896 charger status
   - Proper battery percentage calculation

## Building

```bash
pio run -e lilygo-t-deck-pro-epaper
```

## Flashing

```bash
pio run -e lilygo-t-deck-pro-epaper -t upload
```

## Recommendations

**For Production Use:**
- Consider using the regular T-Deck or T-Deck Plus with TFT display
- Bruce is optimized for fast TFT displays, not E-Paper
- E-Paper support would require significant UI redesign

**For Testing:**
- This configuration can be used to test non-display features
- Keyboard, LoRa, GPS, and other peripherals should work
- Display will be slow but functional for basic menu navigation

## References

- [LilyGo T-Deck Pro GitHub](https://github.com/Xinyuan-LilyGO/T-Deck-Pro)
- [LilyGo T-Deck Pro Wiki](https://wiki.lilygo.cc/get_started/en/Wearable/T-Deck-Pro/T-Deck-Pro.html)
- [GxEPD2 Library](https://github.com/ZinggJM/GxEPD2)
- [Bruce Firmware](https://github.com/pr3y/Bruce)

## Contributing

If you want to improve E-Paper support:
1. Implement GxEPD2 wrapper layer
2. Add TCA8418 keyboard library
3. Optimize UI for E-Paper refresh rates
4. Test and submit pull request

---

**Note**: This is an experimental configuration. Use at your own risk.
