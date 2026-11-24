# Waveshare Parallel Interface LCD Display Modules

> Reference for Waveshare standalone display modules with 8080 parallel and RGB parallel interfaces.
> Last updated: 2026-04-13

## Quick Reference Table

| Model | Size | Resolution | Controller | Interface | Touch | Touch IC | Voltage |
|-------|------|-----------|------------|-----------|-------|----------|---------|
| 3.2inch 320x240 Touch LCD (D) | 3.2" | 320x240 | ILI9341 | 8080 Parallel + SPI | Yes (Resistive) | XPT2046 | 3.3V |
| 4inch Resistive Touch LCD | 4.0" | 480x800 | NT35510 | 8080 Parallel (16/8-bit) | Yes (Resistive) | XPT2046 | 3.3V |

---

## Detailed Specifications

### 3.2inch 320x240 Touch LCD (D)

- **Product URL**: https://www.waveshare.com/3.2inch-320x240-touch-lcd-d.htm
- **Wiki**: https://www.waveshare.com/wiki/3.2inch_320x240_Touch_LCD_(D)
- **LCD Controller**: ILI9341
- **Resolution**: 320 x 240 pixels
- **Display Type**: TN
- **Colors**: 262K RGB
- **Interface**: 8080 16-bit Parallel + SPI
- **Operating Voltage**: 3.3V
- **Pixel Pitch**: 0.198 x 0.198 mm
- **Display Size**: 48.6 x 64.8 mm
- **Module Size**: 94.97 x 62.24 mm
- **Weight**: 0.052 kg
- **Touch**: Yes - XPT2046 resistive touch (SPI)
- **Backlight**: LED, programmable

#### ILI9341 Controller Details

- 262,144-color single-chip TFT-LCD driver
- 240(RGB) x 320 resolution with 172,820 bytes RAM
- Supports 18-bit pixel depth
- Supports 8080-I/II series MCU interfaces (8/9/16/18 bit)
- Supports 3/4-wire SPI serial interface
- Internal system clock and voltage generator

#### XPT2046 Touch Controller Details

- 4-wire resistive touchscreen controller
- 12-bit SAR A/D converter at 125 KHz sampling
- Low-voltage I/O: 1.5V to 5.25V
- Operating temperature: -40 C to +85 C
- SPI interface for touch data

#### Pinout (Direct Header)

This module exposes a direct parallel bus header suitable for STM32 development boards.

| Pin Group | Pins | Description |
|-----------|------|-------------|
| DB[15:0] | 16 data pins | 16-bit parallel data bus |
| nWR | 1 pin | Write strobe, low active |
| nRD | 1 pin | Read strobe, low active |
| RS (DC) | 1 pin | Register select (Data/Command) |
| CS | 1 pin | LCD chip select, low active |
| RST | 1 pin | Reset, low active |
| BL | 1 pin | Backlight control |
| T_CS | 1 pin | Touch SPI chip select |
| T_CLK | 1 pin | Touch SPI clock |
| T_DIN | 1 pin | Touch SPI MOSI |
| T_DO | 1 pin | Touch SPI MISO |
| T_IRQ | 1 pin | Touch interrupt |

---

### 4inch Resistive Touch LCD (480x800, 8080 Parallel)

- **Product URL**: https://www.waveshare.com/4inch-resistive-touch-lcd.htm
- **Wiki**: https://www.waveshare.com/wiki/4inch_Resistive_Touch_LCD
- **LCD Controller**: NT35510 (or equivalent)
- **Resolution**: 480 x 800 pixels
- **Display Type**: IPS
- **Colors**: 65K (RGB565 default, supports RGB888)
- **Interface**: 8080 Parallel (16-bit default, 8-bit selectable via BS resistor)
- **Operating Voltage**: 3.3V
- **Touch**: Yes - XPT2046 resistive touch (SPI)
- **Backlight**: LED, programmable

#### Data Bus Configuration

| Mode | BS Resistor | Data Lines | Color Format |
|------|------------|------------|--------------|
| 16-bit (default) | BS = 1 side | DB[15:0] | RGB565 |
| 8-bit | BS = 0 side | DB[7:0] | RGB565 (2 transfers) |

#### Pinout

| Pin Group | Pins | Description |
|-----------|------|-------------|
| DB[15:0] | 16 data pins | 16-bit parallel data bus |
| nWR | 1 pin | Write strobe |
| nRD | 1 pin | Read strobe |
| RS (DC) | 1 pin | Register select |
| CS | 1 pin | LCD chip select |
| RST | 1 pin | Reset |
| BL | 1 pin | Backlight |
| T_CS | 1 pin | Touch SPI CS |
| T_CLK | 1 pin | Touch SPI CLK |
| T_DIN | 1 pin | Touch SPI MOSI |
| T_DO | 1 pin | Touch SPI MISO |
| T_IRQ | 1 pin | Touch interrupt |

---

## 8080 Parallel Interface Notes

### Signal Timing

The 8080 interface uses these control signals:

| Signal | Direction | Description |
|--------|-----------|-------------|
| CS | Input | Chip select, active low |
| WR (nWR) | Input | Write strobe, data latched on rising edge |
| RD (nRD) | Input | Read strobe, data output on falling edge |
| DC (RS) | Input | Data/Command: 0=Command, 1=Data |
| DB[15:0] or DB[7:0] | Bidirectional | Data bus |

### Write Cycle

1. Set DC (RS) to 0 for command or 1 for data
2. Assert CS low
3. Place data on DB bus
4. Pulse WR low then high (data latched on rising edge)
5. Deassert CS

### ESP32 Compatibility Notes

- **ESP32 (original)**: Can drive 8-bit parallel via GPIO. Limited GPIO count makes 16-bit challenging. Use I2S peripheral for faster 8-bit parallel LCD driving.
- **ESP32-S3**: Best choice for parallel displays. Has dedicated LCD_CAM peripheral supporting 8/16-bit 8080 parallel interface with DMA.
- **ESP32-S2**: Supports 8-bit parallel via I2S LCD mode.
- **ESP32-C3/C6**: Not recommended for parallel -- limited GPIO count, SPI displays preferred.

### ESP32-S3 8080 Parallel Wiring Example

| LCD Pin | ESP32-S3 GPIO | Notes |
|---------|--------------|-------|
| DB0-DB7 | GPIO 9-16 | Contiguous GPIO recommended |
| DB8-DB15 | GPIO 17-24 | For 16-bit mode |
| WR | Any GPIO | Write clock |
| RD | Any GPIO (or VCC) | Tie high if write-only |
| DC | Any GPIO | Data/Command |
| CS | Any GPIO | Chip select |
| RST | Any GPIO | Reset |
| BL | Any GPIO | Backlight (PWM) |

---

## When to Use Parallel vs SPI

| Consideration | SPI | 8080 Parallel |
|--------------|-----|---------------|
| GPIO pins needed | 5-6 | 12-22 |
| Data throughput | Lower (single line) | Higher (8/16 lines) |
| Max practical FPS | ~15-30 FPS (240x320) | ~30-60 FPS (480x800) |
| Best ESP32 variant | Any ESP32 | ESP32-S3 |
| Typical display sizes | Up to 3.5" | 3.2" and larger |
| Wiring complexity | Simple | Complex |
| Ideal for | Small UI, icons, text | Video, animation, full GUI |
