# Waveshare Touch Controller ICs Reference

Comprehensive reference for touch controller ICs found across Waveshare ESP32 display products. Used by AI agents for touch input configuration, calibration, gesture handling, and LVGL integration.

## Table of Contents

- [Touch Controller Summary Table](#touch-controller-summary-table)
- [XPT2046](#xpt2046)
- [CST816S](#cst816s)
- [FT6336](#ft6336)
- [GT911](#gt911)
- [FT5x06](#ft5x06)
- [CST328](#cst328)
- [Controller-to-Waveshare-Product Map](#controller-to-waveshare-product-map)
- [LVGL Touch Integration Guide](#lvgl-touch-integration-guide)
- [I2C Address Collision Reference](#i2c-address-collision-reference)

---

## Touch Controller Summary Table

| Controller | Type | Interface | I2C Address | Multi-Touch | Gestures | Max Points |
|------------|------|-----------|-------------|-------------|----------|------------|
| XPT2046 | Resistive | SPI | N/A | No | No | 1 |
| CST816S | Capacitive | I2C | 0x15 | No | Yes (built-in) | 1 |
| FT6336 | Capacitive | I2C | 0x38 | Yes | Yes | 2 |
| GT911 | Capacitive | I2C | 0x5D or 0x14 | Yes | Yes | 5 |
| FT5x06 | Capacitive | I2C | 0x38 | Yes | Yes | 5 |
| CST328 | Capacitive | I2C | 0x1A | Yes | Yes | 5 |

---

## XPT2046

### Overview

Resistive touch screen controller with SPI interface. The most common touch controller on older and budget Waveshare display modules. Shares SPI bus with display but uses separate CS pin.

### Specifications

| Parameter | Value |
|-----------|-------|
| Manufacturer | Shenzhen Xptek (XPT) |
| Touch Type | 4-wire resistive |
| Interface | SPI (slave) |
| SPI Max Clock | 2.5 MHz (during conversion) |
| SPI Mode | Mode 0 (CPOL=0, CPHA=0) |
| Operating Voltage | 2.2V - 5.25V |
| ADC Resolution | 12-bit (4096 levels) |
| Conversion Time | ~15 us per channel |
| Reference Voltage | Internal 2.5V or external |
| Channels | X, Y position + Z pressure + temperature + battery |
| Pressure Detection | Yes (Z1, Z2 readings) |
| Package | TSSOP-16, QFN-16 |
| Power Consumption | 750 uA active, 0.5 uA power-down |

### SPI Protocol

| Command Byte | Bits | Purpose |
|--------------|------|---------|
| 0xD0 | S=1, A2=1, A1=0, A0=1, MODE=0, SER=0 | Read X position (12-bit, differential) |
| 0x90 | S=1, A2=0, A1=0, A0=1, MODE=0, SER=0 | Read Y position (12-bit, differential) |
| 0xB0 | S=1, A2=0, A1=1, A0=1, MODE=0, SER=0 | Read Z1 (pressure) |
| 0xC0 | S=1, A2=1, A1=0, A0=0, MODE=0, SER=0 | Read Z2 (pressure) |

**Transaction format:** Send command byte, clock in 12-bit result (MSB first), optionally chain next command.

### Pin Connections (Waveshare Standard)

| Pin | Function | Notes |
|-----|----------|-------|
| T_CS | SPI Chip Select | Active LOW, separate from display CS |
| T_CLK | SPI Clock | Shared with display SCLK |
| T_DIN | SPI MOSI | Shared with display MOSI |
| T_DO | SPI MISO | Shared with display MISO (or separate) |
| T_IRQ | Interrupt (PENIRQ) | Active LOW when touch detected |

### Calibration Requirements

**Resistive touch ALWAYS requires calibration.** Raw ADC values do not map linearly to screen pixels.

| Calibration Method | Description |
|-------------------|-------------|
| 3-point | Minimum for affine transform (translation + scale + rotation) |
| 5-point | Better accuracy (adds corners for non-linearity correction) |
| Matrix transform | Apply 2D affine matrix: screen_x = a*raw_x + b*raw_y + c |

**Calibration matrix formula:**
```
screen_x = (a * raw_x + b * raw_y + c) / k
screen_y = (d * raw_x + e * raw_y + f) / k
```

**Common issues:**
- X/Y axes may be swapped relative to display orientation
- Coordinate space may be inverted (mirror)
- Drift over temperature changes
- Pressure sensitivity varies by touch area
- Waveshare modules may swap X/Y depending on display rotation

### Noise Filtering

- Read multiple samples (8-16) and average, discarding outliers
- Discard first reading after pen-down (settling time)
- Apply median filter for jitter reduction
- Check pressure (Z) to validate real touch vs noise
- Debounce: ignore readings within 50ms of pen-down

### LVGL Integration

- LVGL v8: Register `lv_indev_drv_t` with type `LV_INDEV_TYPE_POINTER`
- LVGL v9: Register `lv_indev_t` with `lv_indev_create()` and read callback
- Read callback: Read XPT2046 via SPI, apply calibration matrix, return coordinates
- Use IRQ pin to detect touch presence (avoid polling SPI when no touch)
- ESP-IDF: Use `esp_lcd_touch_xpt2046` component

```
// Pseudocode for LVGL read callback
void xpt2046_read(lv_indev_t *indev, lv_indev_data_t *data) {
    if (irq_pin_is_low()) {
        uint16_t raw_x, raw_y;
        spi_read_xpt2046(&raw_x, &raw_y);
        data->point.x = apply_calibration_x(raw_x, raw_y);
        data->point.y = apply_calibration_y(raw_x, raw_y);
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
```

---

## CST816S

### Overview

Single-point capacitive touch controller from Hynitron. Found on Waveshare small-format ESP32-S3 and ESP32-C3 display modules. Built-in gesture recognition without host processing. Very low power.

### Specifications

| Parameter | Value |
|-----------|-------|
| Manufacturer | Hynitron |
| Touch Type | Self-capacitive |
| Interface | I2C |
| I2C Address | 0x15 (fixed, not configurable) |
| I2C Speed | Up to 400 kHz (Fast mode) |
| Operating Voltage | 2.6V - 3.3V |
| Touch Points | 1 (single touch only) |
| Report Rate | Up to 60 Hz |
| Active Current | 1.2 - 1.8 mA |
| Sleep Current | < 10 uA |
| Standby Current | < 3 uA |
| Wake Method | Touch to wake (auto) or I2C command |

### I2C Register Map

| Register | Address | R/W | Description |
|----------|---------|-----|-------------|
| GestureID | 0x01 | R | Gesture type detected |
| FingerNum | 0x02 | R | Number of fingers (0 or 1) |
| XposH | 0x03 | R | X position high 4 bits [11:8] |
| XposL | 0x04 | R | X position low 8 bits [7:0] |
| YposH | 0x05 | R | Y position high 4 bits [11:8] |
| YposL | 0x06 | R | Y position low 8 bits [7:0] |
| BPC0H | 0xB0 | R | (reserved) |
| BPC0L | 0xB1 | R | (reserved) |
| BPC1H | 0xB2 | R | (reserved) |
| BPC1L | 0xB3 | R | (reserved) |
| ChipID | 0xA7 | R | Chip ID (0xB4 for CST816S, 0xB5 for CST816T) |
| ProjID | 0xA8 | R | Project ID |
| FwVersion | 0xA9 | R | Firmware version |
| MotionMask | 0xEC | W | Enable continuous/double-click reporting |
| IrqPulseWidth | 0xED | W | IRQ pulse width (us) |
| NorScanPer | 0xEE | W | Normal scan period (ms) |
| MotionSlAngle | 0xEF | W | Motion slide angle threshold |
| LpScanRaw1H | 0xF0 | R | Low-power scan raw data |
| LpScanRaw1L | 0xF1 | R | Low-power scan raw data |
| LpScanRaw2H | 0xF2 | R | Low-power scan raw data |
| LpScanRaw2L | 0xF3 | R | Low-power scan raw data |
| LpAutoWakeTime | 0xF4 | W | Auto wake time in low-power mode |
| LpScanTH | 0xF5 | W | Low-power scan threshold |
| LpScanWin | 0xF6 | W | Low-power scan window |
| LpScanFreq | 0xF7 | W | Low-power scan frequency |
| LpScanIdac | 0xF8 | W | Low-power scan IDAC |
| AutoSleepTime | 0xF9 | W | Auto sleep time (seconds, 0=disable) |
| IrqCtl | 0xFA | W | IRQ control register |
| AutoReset | 0xFB | W | Auto reset time (seconds) |
| LongPressTime | 0xFC | W | Long press time (seconds) |
| IOCtl | 0xFD | W | I/O control (soft reset, etc.) |
| DisAutoSleep | 0xFE | W | Disable auto sleep (write 0x01) |

### Gesture IDs

| Gesture ID | Hex | Gesture |
|------------|-----|---------|
| 0x00 | 0x00 | None |
| 0x01 | 0x01 | Slide Up |
| 0x02 | 0x02 | Slide Down |
| 0x03 | 0x03 | Slide Left |
| 0x04 | 0x04 | Slide Right |
| 0x05 | 0x05 | Single Click |
| 0x0B | 0x0B | Double Click |
| 0x0C | 0x0C | Long Press |

### Interrupt Pin Behavior

| IrqCtl (0xFA) Value | Behavior |
|---------------------|----------|
| 0x00 | IRQ on touch (rising/falling edges) |
| 0x01 | IRQ on motion (gesture detected) |
| 0x10 | IRQ changes level on touch |
| 0x11 | IRQ pulse on touch detection |
| 0x40 | Periodic pulse during touch |
| 0x60 | Pulse on touch + periodic during touch |
| 0x80 | Long press triggers IRQ |

**Default behavior:** IRQ pin pulses LOW when a touch event occurs. Configure via IrqCtl register.

### Reset Sequence

1. Pull RST pin LOW for at least 1 ms
2. Release RST (HIGH)
3. Wait at least 50 ms before I2C communication
4. Optionally read ChipID (0xA7) to verify communication

### LVGL Integration

- No calibration required (capacitive)
- I2C read: Read registers 0x01-0x06 in single burst (6 bytes)
- Use IRQ pin for event-driven reads (avoid polling)
- ESP-IDF: Use `esp_lcd_touch_cst816s` component
- Gesture support: Map CST816S gestures to LVGL gesture events or handle in application layer
- LVGL v9 supports `LV_INDEV_TYPE_POINTER` with gesture callback

---

## FT6336

### Overview

Dual-point capacitive touch controller from FocalTech. Found on Waveshare mid-size display modules. Also known as FT6336U or FT6236. Part of the FT6x36 family with identical register maps.

### Specifications

| Parameter | Value |
|-----------|-------|
| Manufacturer | FocalTech |
| Touch Type | Mutual capacitive |
| Interface | I2C |
| I2C Address | 0x38 (fixed) |
| I2C Speed | Up to 400 kHz |
| Operating Voltage | 2.8V - 3.3V |
| Touch Points | 2 (dual touch) |
| Report Rate | Up to 100 Hz |
| Active Current | 3 - 5 mA |
| Sleep Current | < 10 uA (hibernate: < 3 uA) |
| Resolution | Up to 320x480 |

### I2C Register Map

| Register | Address | R/W | Description |
|----------|---------|-----|-------------|
| DEV_MODE | 0x00 | R/W | Device mode (0x00=Normal, 0x40=Test) |
| GEST_ID | 0x01 | R | Gesture ID |
| TD_STATUS | 0x02 | R | Number of touch points detected (0-2) |
| P1_XH | 0x03 | R | Point 1: event flag [7:6] + X high [3:0] |
| P1_XL | 0x04 | R | Point 1: X low byte |
| P1_YH | 0x05 | R | Point 1: touch ID [7:4] + Y high [3:0] |
| P1_YL | 0x06 | R | Point 1: Y low byte |
| P1_WEIGHT | 0x07 | R | Point 1: touch weight/pressure |
| P1_MISC | 0x08 | R | Point 1: touch area |
| P2_XH | 0x09 | R | Point 2: event flag [7:6] + X high [3:0] |
| P2_XL | 0x0A | R | Point 2: X low byte |
| P2_YH | 0x0B | R | Point 2: touch ID [7:4] + Y high [3:0] |
| P2_YL | 0x0C | R | Point 2: Y low byte |
| P2_WEIGHT | 0x0D | R | Point 2: touch weight |
| P2_MISC | 0x0E | R | Point 2: touch area |
| TH_GROUP | 0x80 | R/W | Touch threshold for group of channels |
| TH_DIFF | 0x85 | R/W | Filter function coefficient |
| CTRL | 0x86 | R/W | Control register (0=active, 1=monitor) |
| TIMEENTERMONITOR | 0x87 | R/W | Time to enter monitor mode (seconds) |
| PERIODACTIVE | 0x88 | R/W | Active mode scan period (ms) |
| PERIODMONITOR | 0x89 | R/W | Monitor mode scan period (ms) |
| RADIAN_VALUE | 0x91 | R/W | Minimum gesture distance (radian) |
| OFFSET_LR | 0x92 | R/W | Left-right offset for gesture detection |
| OFFSET_UD | 0x93 | R/W | Up-down offset for gesture detection |
| DISTANCE_LR | 0x94 | R/W | Left-right gesture distance threshold |
| DISTANCE_UD | 0x95 | R/W | Up-down gesture distance threshold |
| DISTANCE_ZOOM | 0x96 | R/W | Zoom gesture distance threshold |
| LIB_VER_H | 0xA1 | R | Library version high byte |
| LIB_VER_L | 0xA2 | R | Library version low byte |
| CIPHER | 0xA3 | R | Chip ID (0x06 or 0x36 for FT6336) |
| G_MODE | 0xA4 | R/W | Interrupt mode (0=polling, 1=trigger) |
| PWR_MODE | 0xA5 | R/W | Power mode (0=active, 3=hibernate) |
| FIRMID | 0xA6 | R | Firmware version |
| FOCALTECH_ID | 0xA8 | R | FocalTech panel ID (0x11) |
| STATE | 0xBC | R | Run state |

### Event Flags (P1_XH bits [7:6])

| Value | Event |
|-------|-------|
| 0b00 | Press Down |
| 0b01 | Lift Up |
| 0b10 | Contact (continuous touch) |
| 0b11 | No Event |

### Gesture IDs

| Gesture ID | Hex | Gesture |
|------------|-----|---------|
| 0x10 | 0x10 | Move Up |
| 0x14 | 0x14 | Move Right |
| 0x18 | 0x18 | Move Down |
| 0x1C | 0x1C | Move Left |
| 0x48 | 0x48 | Zoom In |
| 0x49 | 0x49 | Zoom Out |
| 0x00 | 0x00 | No Gesture |

### Interrupt Pin Behavior

- G_MODE register (0xA4) controls interrupt mode
- Mode 0 (Polling): INT pin pulses LOW on each report cycle
- Mode 1 (Trigger): INT pin goes LOW on touch and stays LOW until released
- Recommended: Use trigger mode with GPIO interrupt for event-driven reading

### LVGL Integration

- Register as `LV_INDEV_TYPE_POINTER` (single point for LVGL)
- Multi-touch: Read both points, report primary to LVGL, use second point for custom gestures
- ESP-IDF: Use `esp_lcd_touch_ft5x06` component (compatible with FT6336)
- Read registers 0x02-0x0E in single I2C transaction (13 bytes)
- No calibration required

---

## GT911

### Overview

Multi-point capacitive touch controller from Goodix. Found on Waveshare larger display modules (4.0"+). Supports up to 5 simultaneous touch points. Most feature-rich touch controller in the Waveshare lineup.

### Specifications

| Parameter | Value |
|-----------|-------|
| Manufacturer | Goodix |
| Touch Type | Mutual capacitive |
| Interface | I2C |
| I2C Address | 0x5D (default) or 0x14 (alternate) |
| I2C Speed | Up to 400 kHz |
| Operating Voltage | 2.6V - 3.3V |
| Touch Points | 5 simultaneous |
| Report Rate | Up to 100 Hz (configurable) |
| Active Current | 10 - 20 mA |
| Sleep Current | < 50 uA |
| Resolution | Configurable up to 4096x4096 |
| Stylus Support | Optional (depends on panel) |

### I2C Address Selection

The GT911 I2C address is determined by the INT pin state at reset:

| INT Pin at Reset | I2C Address | Notes |
|-----------------|-------------|-------|
| LOW (0) | 0x5D | Default on most Waveshare boards |
| HIGH (1) | 0x14 | Alternate address |

**Address selection sequence:**
1. Pull INT pin to desired level
2. Assert RST LOW for at least 10 ms
3. Release RST (HIGH)
4. Wait at least 5 ms
5. Release INT pin (configure as input for interrupt)
6. Wait at least 50 ms before I2C communication

**WARNING:** If the INT pin floats during reset, the address is unpredictable. Waveshare boards typically have the INT pull-down/up configured for 0x5D.

### I2C Register Map (Key Registers)

| Register | Address | R/W | Description |
|----------|---------|-----|-------------|
| Command | 0x8040 | R/W | Command register |
| Config_Version | 0x8047 | R/W | Configuration version |
| X_Output_Max_L | 0x8048 | R/W | X resolution low byte |
| X_Output_Max_H | 0x8049 | R/W | X resolution high byte |
| Y_Output_Max_L | 0x804A | R/W | Y resolution low byte |
| Y_Output_Max_H | 0x804B | R/W | Y resolution high byte |
| Touch_Number | 0x804C | R/W | Max touch points (1-5) |
| Module_Switch1 | 0x804D | R/W | Coordinate swap, INT mode |
| Module_Switch2 | 0x804E | R/W | Additional module config |
| Shake_Count | 0x804F | R/W | Touch filter shake count |
| Filter | 0x8050 | R/W | Touch filter strength |
| Large_Touch | 0x8051 | R/W | Large area touch threshold |
| Noise_Reduction | 0x8052 | R/W | Noise reduction setting |
| Screen_Touch_Level | 0x8053 | R/W | Touch detection threshold |
| Screen_Leave_Level | 0x8054 | R/W | Touch release threshold |
| Refresh_Rate | 0x8056 | R/W | Report rate (period in ms) |
| Coord_Status | 0x814E | R | Buffer status + touch count |
| Point1_X_L | 0x8150 | R | Point 1 X low byte |
| Point1_X_H | 0x8151 | R | Point 1 X high byte |
| Point1_Y_L | 0x8152 | R | Point 1 Y low byte |
| Point1_Y_H | 0x8153 | R | Point 1 Y high byte |
| Point1_Size_L | 0x8154 | R | Point 1 touch size low |
| Point1_Size_H | 0x8155 | R | Point 1 touch size high |
| Point1_TrackID | 0x8157 | R | Point 1 track ID |
| Config_Chksum | 0x80FF | R/W | Configuration checksum |
| Config_Fresh | 0x8100 | W | Write 0x01 to apply new config |

**Point N data:** Each point uses 8 bytes starting at 0x8150 + (N-1)*8.

### Coordinate Status Register (0x814E)

| Bit | Description |
|-----|-------------|
| [7] | Buffer status (1=ready, must clear by writing 0) |
| [6:4] | Reserved |
| [3:0] | Number of touch points (0-5) |

**CRITICAL:** After reading touch data, write 0x00 to register 0x814E to clear the buffer status flag. Failure to clear causes stale data.

### Interrupt Modes (Module_Switch1, 0x804D, bits [1:0])

| Mode | Behavior |
|------|----------|
| 0x00 | Rising edge trigger |
| 0x01 | Falling edge trigger |
| 0x02 | Low level while touched |
| 0x03 | High level while touched |

### Configuration Update Procedure

1. Write configuration registers (0x8047-0x80FE)
2. Calculate checksum: complement of sum of all config bytes (0x8047-0x80FE) + 1
3. Write checksum to 0x80FF
4. Write 0x01 to 0x8100 (Config_Fresh) to apply

### LVGL Integration

- Register as `LV_INDEV_TYPE_POINTER` for primary touch point
- Multi-touch: Read all 5 points from registers, report point 0 to LVGL
- For pinch/zoom: Process multi-touch points in application layer
- ESP-IDF: Use `esp_lcd_touch_gt911` component
- Read sequence: Check 0x814E, read N*8 bytes from 0x8150, clear 0x814E
- Use INT pin with falling edge interrupt for event-driven reads
- Resolution auto-adjusts based on configuration registers

---

## FT5x06

### Overview

Multi-point capacitive touch controller family from FocalTech. Includes FT5206, FT5306, FT5406, and FT5x06 variants. Found on Waveshare mid-to-large display modules. Register-compatible with FT6336 but supports up to 5 touch points.

### Specifications

| Parameter | Value |
|-----------|-------|
| Manufacturer | FocalTech |
| Touch Type | Mutual capacitive |
| Interface | I2C |
| I2C Address | 0x38 (fixed) |
| I2C Speed | Up to 400 kHz |
| Operating Voltage | 2.8V - 3.3V |
| Touch Points | 5 simultaneous (FT5406), 2 (FT5206) |
| Report Rate | Up to 100 Hz |
| Active Current | 5 - 15 mA |
| Sleep Current | < 3 uA (hibernate mode) |
| Resolution | Up to 800x480 (depends on variant) |

### I2C Register Map

Register map is a superset of FT6336 with additional touch points:

| Register | Address | R/W | Description |
|----------|---------|-----|-------------|
| TD_STATUS | 0x02 | R | Number of touch points (0-5) |
| P1_XH-P1_MISC | 0x03-0x08 | R | Point 1 data (same as FT6336) |
| P2_XH-P2_MISC | 0x09-0x0E | R | Point 2 data |
| P3_XH-P3_MISC | 0x0F-0x14 | R | Point 3 data |
| P4_XH-P4_MISC | 0x15-0x1A | R | Point 4 data |
| P5_XH-P5_MISC | 0x1B-0x20 | R | Point 5 data |
| ID_G_THGROUP | 0x80 | R/W | Touch threshold |
| ID_G_PERIODACTIVE | 0x88 | R/W | Active period (3-14 ms) |
| ID_G_PERIODMONITOR | 0x89 | R/W | Monitor period |
| ID_G_CIPHER | 0xA3 | R | Chip ID |
| ID_G_MODE | 0xA4 | R/W | Interrupt mode |
| ID_G_FIRMID | 0xA6 | R | Firmware version |
| ID_G_VENDID | 0xA8 | R | FocalTech vendor ID (0x11) |

### Chip ID Values

| Chip ID (0xA3) | Controller |
|----------------|------------|
| 0x55 | FT5206 |
| 0x08 | FT5306 |
| 0x06 | FT5406 |
| 0x36 | FT6336 |
| 0x64 | FT6436 |

### LVGL Integration

- Same driver as FT6336 (register-compatible)
- ESP-IDF: Use `esp_lcd_touch_ft5x06` component (covers entire FT5x/FT6x family)
- Read TD_STATUS first to know how many points to read
- Single I2C burst read from 0x02 to 0x20 (31 bytes) for all 5 points
- Map primary point to LVGL pointer input
- No calibration required

---

## CST328

### Overview

Multi-point capacitive touch controller from Hynitron. Found on some newer Waveshare ESP32-S3 display modules with larger panels. More capable successor to CST816S with multi-touch support.

### Specifications

| Parameter | Value |
|-----------|-------|
| Manufacturer | Hynitron |
| Touch Type | Mutual capacitive |
| Interface | I2C |
| I2C Address | 0x1A (fixed) |
| I2C Speed | Up to 400 kHz |
| Operating Voltage | 2.6V - 3.3V |
| Touch Points | 5 simultaneous |
| Report Rate | Up to 60 Hz |
| Active Current | 5 - 10 mA |
| Sleep Current | < 10 uA |

### I2C Register Map

| Register | Address | R/W | Description |
|----------|---------|-----|-------------|
| FingerNum | 0xD005 | R | Number of touch points |
| P1_XH | 0xD006 | R | Point 1 X high byte |
| P1_XL | 0xD007 | R | Point 1 X low byte |
| P1_YH | 0xD008 | R | Point 1 Y high byte |
| P1_YL | 0xD009 | R | Point 1 Y low byte |
| P1_Pressure | 0xD00A | R | Point 1 pressure |
| ChipType | 0xD204 | R | Chip type identifier |
| ProjectID | 0xD208 | R | Project number |
| FwVersion | 0xD20C | R | Firmware version |

**Note:** CST328 uses 16-bit register addresses (2-byte address prefix in I2C transactions).

### LVGL Integration

- ESP-IDF: Use `esp_lcd_touch_cst328` component (if available) or custom driver
- Read touch data starting from 0xD005
- Map to `LV_INDEV_TYPE_POINTER`
- No calibration required

---

## Controller-to-Waveshare-Product Map

| Controller | Waveshare Product Examples | Display Size |
|------------|---------------------------|--------------|
| XPT2046 | 2.4" Touch LCD, 2.8" Touch LCD, 3.2" Touch LCD, 3.5" RPi LCD | 2.4"-3.5" |
| CST816S | ESP32-S3-Touch-LCD-1.28, ESP32-C3-LCD-1.44 | 1.28"-1.69" |
| FT6336 | ESP32-S3-Touch-LCD-2.1, various 2"-3" capacitive | 2.0"-3.0" |
| GT911 | ESP32-S3-Touch-LCD-4.0, 7.0" modules, ESP32-P4 boards | 4.0"-10.1" |
| FT5x06 | ESP32-S3-Touch-LCD-3.5, some 4.3" modules | 3.5"-5.0" |
| CST328 | Some newer ESP32-S3 4.0" modules | 3.5"-4.0" |

---

## LVGL Touch Integration Guide

### Common Setup Pattern (ESP-IDF + LVGL v9)

```
// 1. Initialize I2C or SPI bus
// 2. Create touch panel handle
esp_lcd_touch_handle_t touch_handle;
esp_lcd_panel_io_i2c_config_t io_config = {
    .dev_addr = TOUCH_I2C_ADDR,
    .scl_speed_hz = 400000,
};
esp_lcd_touch_config_t touch_config = {
    .x_max = DISPLAY_WIDTH,
    .y_max = DISPLAY_HEIGHT,
    .rst_gpio_num = TOUCH_RST_PIN,
    .int_gpio_num = TOUCH_INT_PIN,
};

// 3. Create LVGL input device
lv_indev_t *indev = lv_indev_create();
lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
lv_indev_set_read_cb(indev, touch_read_cb);
```

### Coordinate Transformation

When display rotation changes, touch coordinates must be transformed to match:

| Display Rotation | Touch Transform |
|-----------------|-----------------|
| 0 (Portrait) | x' = x, y' = y |
| 90 (Landscape) | x' = y, y' = max_x - x |
| 180 (Portrait Inv) | x' = max_x - x, y' = max_y - y |
| 270 (Landscape Inv) | x' = max_y - y, y' = x |

**ESP-IDF touch components handle this via `esp_lcd_touch_set_mirror_x/y` and `esp_lcd_touch_set_swap_xy`.**

---

## I2C Address Collision Reference

| Address | Controllers | Collision Risk |
|---------|-------------|----------------|
| 0x15 | CST816S | Low (unique address) |
| 0x1A | CST328 | Low (unique address) |
| 0x14 | GT911 (alternate) | Low |
| 0x38 | FT6336, FT5x06 | HIGH (same address, same family) |
| 0x5D | GT911 (default) | Low |

**Note:** FT6336 and FT5x06 cannot coexist on the same I2C bus (both use 0x38, non-configurable). This is not normally an issue since only one touch controller is used per display module.

---
