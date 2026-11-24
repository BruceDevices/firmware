# Waveshare Display Interface Reference

Comprehensive reference for hardware interfaces used in Waveshare ESP32 display products. Covers SPI, I2C, 8080 parallel, RGB parallel, QSPI, MIPI-DSI, voltage regulation, and backlight control.

## Table of Contents

- [Interface Summary](#interface-summary)
- [SPI Display Interface](#spi-display-interface)
- [I2C Touch/Display Interface](#i2c-touchdisplay-interface)
- [8080 Parallel Interface](#8080-parallel-interface)
- [RGB Parallel Interface](#rgb-parallel-interface)
- [QSPI Interface](#qspi-interface)
- [MIPI-DSI Interface](#mipi-dsi-interface)
- [Voltage Regulators](#voltage-regulators)
- [Backlight Control](#backlight-control)
- [ESP32 Variant Interface Support Matrix](#esp32-variant-interface-support-matrix)

---

## Interface Summary

| Interface | Pins Required | Max Throughput | GRAM Required | Best For |
|-----------|--------------|----------------|---------------|----------|
| SPI (4-wire) | 5-6 | 80 Mbps (80 MHz) | Yes | Small-mid displays (<= 320x240) |
| I2C | 2 | 3.2 Mbps (3.4 MHz) | Yes | Touch controllers, small OLEDs |
| 8080 Parallel (8-bit) | 12-13 | 160 Mbps (20 MHz) | Yes | Mid displays (320x240 - 480x320) |
| 8080 Parallel (16-bit) | 20-21 | 320 Mbps (20 MHz) | Yes | Larger displays |
| RGB (16-bit) | 22-24 | 400 Mbps (25 MHz) | No | Large displays (480x480+) |
| RGB (24-bit) | 30-32 | 600 Mbps (25 MHz) | No | Large high-color displays |
| QSPI | 7-8 | 320 Mbps (80 MHz) | Yes | Mid displays, fewer pins |
| MIPI-DSI (4-lane) | 10 | 4 Gbps | No | Very large/high-res displays |

---

## SPI Display Interface

### Standard 4-Wire SPI Connection

The most common interface for Waveshare small-to-mid ESP32 display modules.

#### Signal Definitions

| Signal | Alt Names | Direction | Description |
|--------|-----------|-----------|-------------|
| SCLK | SCK, CLK | MCU -> Display | Serial clock |
| MOSI | SDA, SDI, DIN | MCU -> Display | Serial data (Master Out Slave In) |
| MISO | SDO, DOUT | Display -> MCU | Serial data out (rarely used for displays) |
| CS | SS, CE | MCU -> Display | Chip select (active LOW) |
| DC | D/C, RS, A0 | MCU -> Display | Data/Command select (LOW=command, HIGH=data) |
| RST | RES, RESET | MCU -> Display | Hardware reset (active LOW) |
| BLK | BL, LED | MCU -> Display | Backlight control |

#### Waveshare SPI Wiring Patterns

**Pattern A: Dedicated SPI (display only)**

| Function | Typical ESP32 Pin | Typical ESP32-S3 Pin | Notes |
|----------|-------------------|---------------------|-------|
| SCLK | GPIO18 | GPIO12 | VSPI clock |
| MOSI | GPIO23 | GPIO11 | VSPI data |
| CS | GPIO5 | GPIO10 | Active LOW |
| DC | GPIO2 or GPIO27 | GPIO9 | HIGH=data, LOW=command |
| RST | GPIO4 or GPIO33 | GPIO14 | Pull-up recommended |
| BLK | GPIO32 or GPIO22 | GPIO45 | PWM for dimming |

**Pattern B: Shared SPI (display + touch XPT2046)**

| Function | Signal | Pin Notes |
|----------|--------|-----------|
| Display SCLK | Shared | Same physical wire |
| Display MOSI | Shared | Same physical wire |
| Display MISO | Shared | Same physical wire (needed for touch read) |
| Display CS | Separate | Must be different from touch CS |
| Touch CS | Separate | Must be different from display CS |
| Touch IRQ | Separate | Touch interrupt (active LOW) |
| DC | Display only | Not used by XPT2046 |

**CRITICAL:** When sharing SPI bus between display and XPT2046 touch:
- Maximum SPI clock for touch reads is 2.5 MHz (XPT2046 limit)
- Display writes can run at 40-80 MHz
- Must change SPI clock speed when switching between display and touch
- Alternatively, use separate SPI buses on ESP32-S3 (SPI2 + SPI3)
- Assert only one CS at a time

#### SPI Clock Speed by Controller

| Controller | Max Write Clock | Recommended | Read Clock |
|------------|----------------|-------------|------------|
| ST7735S | 15 MHz | 10-15 MHz | 6.66 MHz |
| ST7789V | 62.5 MHz | 40-80 MHz | 20 MHz |
| ILI9341 | 32 MHz | 20-40 MHz | 10 MHz |
| ILI9488 | 20 MHz | 15-20 MHz | 10 MHz |
| GC9A01 | 60 MHz | 40-60 MHz | 20 MHz |

#### SPI Transaction Format

```
[CS LOW] [DC=0: Command byte] [DC=1: Data byte(s)...] [CS HIGH]
```

**Pixel data write sequence:**
1. Send CASET command (0x2A) + column start/end (4 bytes)
2. Send RASET command (0x2B) + row start/end (4 bytes)
3. Send RAMWR command (0x2C)
4. Send pixel data (continuous, 2 bytes/pixel for RGB565)
5. CS HIGH when done

#### DMA Configuration (ESP-IDF)

| Parameter | Typical Value | Notes |
|-----------|---------------|-------|
| DMA channel | Auto (SPI_DMA_CH_AUTO) | Let ESP-IDF choose |
| Max transfer size | 32768 bytes | Multiple of pixel width * 2 |
| Queue depth | 7-10 | Pending DMA transactions |
| Pre/Post transfer CB | Set DC pin | Use pre_cb to set DC for commands vs data |

---

## I2C Touch/Display Interface

### Standard I2C Connection

Used primarily for capacitive touch controllers and small OLED displays.

#### Signal Definitions

| Signal | Direction | Type | Description |
|--------|-----------|------|-------------|
| SDA | Bidirectional | Open-drain | Serial data |
| SCL | MCU -> Device | Open-drain | Serial clock |
| INT | Device -> MCU | Push-pull or open-drain | Interrupt (active LOW typically) |
| RST | MCU -> Device | Push-pull | Hardware reset (active LOW) |

#### Waveshare I2C Wiring Patterns

| Function | Typical ESP32 Pin | Typical ESP32-S3 Pin |
|----------|-------------------|---------------------|
| SDA | GPIO21 | GPIO7 or GPIO8 |
| SCL | GPIO22 | GPIO6 or GPIO9 |
| INT | GPIO36 or GPIO39 | GPIO3 or GPIO4 |
| RST | GPIO33 | GPIO5 |

#### Pull-up Requirements

| Scenario | Pull-up Value | Notes |
|----------|---------------|-------|
| Short trace (on-board) | 4.7 kOhm | Standard, most Waveshare modules include these |
| 400 kHz Fast mode | 2.2 - 4.7 kOhm | Higher speed needs stronger pull-ups |
| Multiple devices on bus | 2.2 kOhm | Lower resistance for capacitance |

**Most Waveshare modules include on-board I2C pull-ups.** Do not add external pull-ups unless communication fails, as excessive pull-up strength wastes current and can cause issues.

#### I2C Transaction Patterns

**Touch controller read (FT6336/GT911):**
```
[START] [ADDR+W] [REG_H] [REG_L] [RESTART] [ADDR+R] [DATA...] [NACK] [STOP]
```

**Touch controller read (CST816S, single-byte address):**
```
[START] [ADDR+W] [REG] [RESTART] [ADDR+R] [DATA...] [NACK] [STOP]
```

**GT911 buffer clear after read:**
```
[START] [ADDR+W] [0x81] [0x4E] [0x00] [STOP]
```

---

## 8080 Parallel Interface

### Overview

Intel 8080-style parallel interface for higher throughput display communication. Uses 8 or 16 data lines with control signals. Requires more GPIO pins but achieves 2-4x the throughput of SPI.

#### Signal Definitions

| Signal | Direction | Description |
|--------|-----------|-------------|
| D[0:7] or D[0:15] | Bidirectional | Data bus (8-bit or 16-bit) |
| WR | MCU -> Display | Write strobe (active LOW, data latched on rising edge) |
| RD | MCU -> Display | Read strobe (active LOW) |
| CS | MCU -> Display | Chip select (active LOW) |
| DC (RS) | MCU -> Display | Data/Command (LOW=command, HIGH=data) |
| RST | MCU -> Display | Hardware reset (active LOW) |

#### ESP32-S3 8080 Configuration

ESP32-S3 has a dedicated LCD_CAM peripheral that supports 8080 parallel mode with DMA:

| Parameter | Value |
|-----------|-------|
| Data pins | Any 8 or 16 GPIO (via GPIO matrix) |
| Max pixel clock | 20 MHz (8-bit), 10 MHz (16-bit) |
| DMA | Yes (LCD_CAM DMA channel) |
| Color format | RGB565 (16-bit) or RGB888 (24-bit) |

**Typical pin assignment (8-bit 8080 on ESP32-S3):**

| Signal | GPIO | Notes |
|--------|------|-------|
| D0 | GPIO39 | Data bit 0 |
| D1 | GPIO40 | Data bit 1 |
| D2 | GPIO41 | Data bit 2 |
| D3 | GPIO42 | Data bit 3 |
| D4 | GPIO45 | Data bit 4 |
| D5 | GPIO46 | Data bit 5 |
| D6 | GPIO47 | Data bit 6 |
| D7 | GPIO48 | Data bit 7 |
| WR | GPIO17 | Write strobe |
| DC | GPIO7 | Data/Command |
| CS | GPIO6 | Chip select |
| RST | GPIO5 | Reset |

**Note:** Pin assignments vary by Waveshare product. Always check the specific board schematic.

#### Performance Comparison: SPI vs 8080

| Metric | SPI (40 MHz) | 8-bit 8080 (20 MHz) | 16-bit 8080 (10 MHz) |
|--------|-------------|---------------------|----------------------|
| Throughput | 40 Mbps | 160 Mbps | 160 Mbps |
| 320x240 RGB565 FPS | ~16 | ~65 | ~65 |
| 480x320 RGB565 FPS | ~8 | ~32 | ~32 |
| GPIO pins | 5-6 | 12-13 | 20-21 |

#### ESP-IDF Setup

```
esp_lcd_i80_bus_config_t bus_config = {
    .dc_gpio_num = DC_PIN,
    .wr_gpio_num = WR_PIN,
    .clk_src = LCD_CLK_SRC_DEFAULT,
    .data_gpio_nums = {D0, D1, D2, D3, D4, D5, D6, D7},
    .bus_width = 8,
    .max_transfer_bytes = DISPLAY_WIDTH * DISPLAY_HEIGHT * 2,
    .dma_burst_size = 64,
};
esp_lcd_new_i80_bus(&bus_config, &i80_bus);
```

---

## RGB Parallel Interface

### Overview

Continuous-refresh parallel interface for large displays. The MCU must constantly stream pixel data to the display; there is no display-side frame buffer (GRAM). Requires a frame buffer in MCU memory (typically PSRAM).

#### Signal Definitions

| Signal | Count | Direction | Description |
|--------|-------|-----------|-------------|
| R[0:4/7] | 5 or 8 | MCU -> Display | Red data |
| G[0:5/7] | 6 or 8 | MCU -> Display | Green data |
| B[0:4/7] | 5 or 8 | MCU -> Display | Blue data |
| PCLK | 1 | MCU -> Display | Pixel clock |
| HSYNC | 1 | MCU -> Display | Horizontal sync |
| VSYNC | 1 | MCU -> Display | Vertical sync |
| DE | 1 | MCU -> Display | Data enable |
| DISP_EN | 1 | MCU -> Display | Display enable (optional) |

#### RGB Modes

| Mode | Data Pins | Bits/Pixel | Notes |
|------|-----------|------------|-------|
| RGB565 | 16 | 16 | R5+G6+B5, most common with ESP32-S3 |
| RGB666 | 18 | 18 | R6+G6+B6 |
| RGB888 | 24 | 24 | R8+G8+B8, highest quality, most pins |

#### Timing Parameters

RGB displays require precise timing configuration:

| Parameter | Description | Typical Values |
|-----------|-------------|----------------|
| PCLK | Pixel clock frequency | 6-25 MHz |
| H_RES | Horizontal resolution | 480, 800, 1024 |
| V_RES | Vertical resolution | 272, 480, 600 |
| HSYNC_PULSE_WIDTH | HSYNC active period | 1-10 clocks |
| HSYNC_BACK_PORCH | Before active data | 8-46 clocks |
| HSYNC_FRONT_PORCH | After active data | 8-50 clocks |
| VSYNC_PULSE_WIDTH | VSYNC active period | 1-10 lines |
| VSYNC_BACK_PORCH | Before active frame | 2-23 lines |
| VSYNC_FRONT_PORCH | After active frame | 2-22 lines |

**These values are display-panel-specific. Always use values from Waveshare example code or datasheet.**

#### ESP32-S3 RGB LCD Peripheral

| Parameter | Capability |
|-----------|------------|
| Max PCLK | 25 MHz (practical, depends on PSRAM bandwidth) |
| Data width | 8/16/24-bit |
| PSRAM requirement | MUST have PSRAM for frame buffer |
| Bounce buffer | Optional, reduces PSRAM bandwidth by using SRAM cache |
| DMA | Automatic continuous refresh from frame buffer |

**Bounce buffer pattern:** ESP32-S3 can use small SRAM bounce buffers that are DMA'd to the RGB peripheral, with PSRAM-to-SRAM copies happening in parallel. This reduces PSRAM bus contention with application code.

#### Frame Buffer Memory Requirements

| Resolution | RGB565 (1 buffer) | RGB565 (2 buffers) | RGB888 (1 buffer) |
|------------|-------------------|--------------------|--------------------|
| 240x240 | 115 KB | 230 KB | 173 KB |
| 320x240 | 150 KB | 300 KB | 225 KB |
| 480x272 | 256 KB | 512 KB | 384 KB |
| 480x480 | 450 KB | 900 KB | 675 KB |
| 800x480 | 750 KB | 1.5 MB | 1.1 MB |
| 1024x600 | 1.2 MB | 2.4 MB | 1.8 MB |

#### SPI Init + RGB Data Pattern

Many RGB displays (GC9503, ST7701S) require SPI initialization before RGB data streaming:

1. Initialize SPI bus (3-wire, 9-bit mode)
2. Send panel init commands via SPI (power, gamma, timing config)
3. Deinitialize SPI bus (free pins)
4. Initialize RGB LCD peripheral
5. Allocate frame buffer in PSRAM
6. Start continuous refresh

**3-wire SPI note:** These controllers use 9-bit SPI (1 D/C bit + 8 data bits). Standard ESP32 SPI supports only 8/16/32-bit transfers, so the 9-bit mode requires the `esp_lcd_panel_io_3wire_spi` component or bit-banging.

---

## QSPI Interface

### Overview

Quad SPI uses 4 data lines simultaneously for 4x throughput of standard SPI, while requiring far fewer pins than parallel interfaces. Supported by newer display controllers like NV3041A.

#### Signal Definitions

| Signal | Direction | Description |
|--------|-----------|-------------|
| SCLK | MCU -> Display | Serial clock |
| CS | MCU -> Display | Chip select (active LOW) |
| D0 (SIO0) | Bidirectional | Data line 0 (MOSI in single-SPI mode) |
| D1 (SIO1) | Bidirectional | Data line 1 (MISO in single-SPI mode) |
| D2 (SIO2) | Bidirectional | Data line 2 |
| D3 (SIO3) | Bidirectional | Data line 3 |
| DC | MCU -> Display | Data/Command (some controllers embed D/C in protocol) |
| RST | MCU -> Display | Hardware reset (active LOW) |

#### Throughput Comparison

| Interface | Clock | Data Lines | Throughput | GPIO Count |
|-----------|-------|------------|------------|------------|
| SPI | 80 MHz | 1 | 80 Mbps | 5-6 |
| QSPI | 80 MHz | 4 | 320 Mbps | 7-8 |
| 8080 (8-bit) | 20 MHz | 8 | 160 Mbps | 12-13 |

QSPI achieves 2x the throughput of 8080 parallel with fewer pins.

#### ESP-IDF QSPI Setup

Use `esp_lcd_new_panel_io_spi` with `SPI_TRANS_MODE_QIO` flag for quad-mode transfers:

```
esp_lcd_panel_io_spi_config_t io_config = {
    .cs_gpio_num = CS_PIN,
    .dc_gpio_num = DC_PIN,
    .spi_mode = 0,
    .pclk_hz = 80 * 1000 * 1000,
    .trans_queue_depth = 10,
    .flags = {
        .quad_mode = true,
    },
};
```

---

## MIPI-DSI Interface

### Overview

Mobile Industry Processor Interface - Display Serial Interface. High-speed serial interface for large, high-resolution displays. Only available on ESP32-P4.

#### Signal Definitions

| Signal | Count | Description |
|--------|-------|-------------|
| D0P/D0N | 2 | Data lane 0 (differential pair) |
| D1P/D1N | 2 | Data lane 1 (differential pair) |
| D2P/D2N | 2 | Data lane 2 (optional) |
| D3P/D3N | 2 | Data lane 3 (optional) |
| CLKP/CLKN | 2 | Clock lane (differential pair) |

#### Specifications

| Parameter | Value |
|-----------|-------|
| Lane count | 1, 2, or 4 data lanes |
| Max bitrate per lane | 1.5 Gbps (MIPI-DSI 1.3) |
| 4-lane max throughput | 6 Gbps (theoretical) |
| Typical PCLK | 50-100 MHz equivalent |
| Video modes | Video Mode (streaming), Command Mode (GRAM-based) |
| Color formats | RGB565, RGB666, RGB888 |

#### ESP32-P4 MIPI-DSI

| Parameter | ESP32-P4 Capability |
|-----------|-------------------|
| Max lanes | 2 data lanes |
| Max resolution | 1024x768 (practical) |
| Color depth | Up to RGB888 |
| Frame buffer | PSRAM required |
| DSI PHY | Built-in |

---

## Voltage Regulators

### Common Regulators on Waveshare Display Modules

| Regulator | Type | Input Range | Output | Max Current | Found On |
|-----------|------|-------------|--------|-------------|----------|
| AMS1117-3.3 | LDO | 4.5-12V | 3.3V | 1A | Many ESP32 dev boards |
| ME6211 | LDO | 2.0-6.0V | 3.3V | 500 mA | Compact modules |
| RT9013 | LDO | 2.2-5.5V | 3.3V | 500 mA | Low-noise modules |
| SGM2036 | LDO | 1.5-5.5V | 1.8V | 300 mA | MIPI PHY power |
| SY8089 | Buck | 4.5-5.5V | 3.3V | 2A | High-current boards |
| TPS63000 | Buck-Boost | 1.8-5.5V | 3.3V | 1.2A | Battery-powered |

### Display Power Rails

| Rail | Voltage | Purpose | Notes |
|------|---------|---------|-------|
| VDD | 3.3V | Controller digital | Must be stable, low noise |
| VCI / AVDD | 2.8V | Analog / VCOM | Internal regulator or external |
| VDDIO | 1.8V or 3.3V | I/O level | Must match MCU logic level |
| VGH | 12-16V | Gate high voltage | Generated by display controller charge pump |
| VGL | -7 to -12V | Gate low voltage | Generated by display controller charge pump |
| AVDD (backlight) | varies | LED backlight | See backlight section |

**Note:** VGH/VGL are generated internally by the display controller IC from VDD/VCI. Do not supply externally unless the datasheet specifically requires it.

### Level Shifting Considerations

| MCU | Logic Level | Display Logic | Level Shift Needed? |
|-----|------------|---------------|-------------------|
| ESP32 | 3.3V | 3.3V | No |
| ESP32-S3 | 3.3V | 3.3V | No |
| ESP32-C3 | 3.3V | 3.3V | No |
| ESP32-P4 | 3.3V / 1.8V | 1.8V (MIPI) | Built into DSI PHY |

**All ESP32 variants operate at 3.3V logic, and all Waveshare display modules are 3.3V compatible. No level shifting is needed for standard configurations.**

---

## Backlight Control

### Methods

| Method | Complexity | Dimming | Power Efficiency | Notes |
|--------|-----------|---------|-----------------|-------|
| GPIO ON/OFF | Lowest | No (on/off only) | Moderate | Simplest, no dimming |
| PWM dimming | Low | Yes (analog) | Good | Most common in Waveshare products |
| LED driver IC | Medium | Yes (precise) | Best | Used on some larger modules |
| Always ON | None | No | N/A | Some modules have fixed backlight |

### PWM Backlight Control

| Parameter | Recommended Value | Notes |
|-----------|-------------------|-------|
| PWM Frequency | 1 - 25 kHz | > 1 kHz avoids visible flicker |
| Duty cycle range | 0-100% | 0%=OFF, 100%=full brightness |
| Default brightness | 50-80% | Good starting point |
| Minimum visible | 5-10% | Below this may flicker or not illuminate |

#### ESP32 PWM (LEDC) Configuration

| Parameter | Value |
|-----------|-------|
| Timer | LEDC_TIMER_0 |
| Channel | LEDC_CHANNEL_0 |
| Frequency | 5000 Hz |
| Resolution | 8-bit (0-255) or 10-bit (0-1023) |
| Speed mode | LEDC_LOW_SPEED_MODE |

```
// ESP-IDF LEDC backlight setup
ledc_timer_config_t timer_conf = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .duty_resolution = LEDC_TIMER_8_BIT,
    .timer_num = LEDC_TIMER_0,
    .freq_hz = 5000,
    .clk_cfg = LEDC_AUTO_CLK,
};
ledc_channel_config_t channel_conf = {
    .gpio_num = BACKLIGHT_PIN,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_0,
    .timer_sel = LEDC_TIMER_0,
    .duty = 200,  // ~78% brightness
    .hpoint = 0,
};
```

### Backlight Circuit Topologies

**Topology A: Direct GPIO drive (small displays)**
```
MCU_GPIO ---[100R]--- LED anode
                      LED cathode --- GND
```
Only for very small backlights (< 20 mA). Most modules use this with an onboard transistor.

**Topology B: MOSFET switch with PWM (most Waveshare modules)**
```
MCU_GPIO ---[10K]--- MOSFET Gate
                     MOSFET Drain --- LED cathode
                     MOSFET Source --- GND
VCC ---[Resistor]--- LED anode
```

**Topology C: LED driver IC (large panels)**
```
MCU_PWM/EN --- Driver IC --- LED string (series/parallel)
```
Used on 7"+ panels where LED current exceeds 100 mA.

### Waveshare Backlight Pin Conventions

| Board Category | Backlight Pin Label | Active Level | Dimming |
|---------------|-------------------|--------------|---------|
| SPI LCD modules | BLK or BL | HIGH = ON | PWM capable |
| ESP32-S3 integrated | LCD_BL | HIGH = ON | PWM capable |
| 7" RGB modules | BACKLIGHT | HIGH = ON | Usually GPIO or driver IC |
| Round LCD | BLK | HIGH = ON | PWM capable |

**WARNING:** Some Waveshare modules have the backlight always ON when powered, with no software control pin. Check the schematic before assuming PWM dimming is available.

---

## ESP32 Variant Interface Support Matrix

| Interface | ESP32 | ESP32-S2 | ESP32-S3 | ESP32-C3 | ESP32-C6 | ESP32-P4 |
|-----------|-------|----------|----------|----------|----------|----------|
| SPI | 2x (SPI2/3) | 2x | 2x | 1x (SPI2) | 1x (SPI2) | 2x |
| I2C | 2x | 2x | 2x | 1x | 1x | 2x |
| 8080 Parallel | Bit-bang only | LCD_CAM | LCD_CAM | No | No | LCD_CAM |
| RGB LCD | No | No | Yes (LCD_CAM) | No | No | Yes |
| QSPI Display | Via SPI | Via SPI | Yes (LCD_CAM) | No | No | Yes |
| MIPI-DSI | No | No | No | No | No | Yes (2-lane) |
| PSRAM | 4 MB SPI | 2-8 MB | 2-32 MB OPI | No | No | 32 MB OPI |
| Max SPI Clock | 80 MHz | 80 MHz | 80 MHz | 80 MHz | 80 MHz | 80 MHz |
| Usable GPIO | ~26 | ~27 | ~36 | ~16 | ~19 | ~40+ |

### Interface Selection Guide

| Display Size | Resolution | Recommended Interface | Recommended ESP32 |
|-------------|------------|----------------------|-------------------|
| 0.96"-1.8" | <= 160x128 | SPI | Any ESP32 variant |
| 1.3"-2.4" | 240x240 - 320x240 | SPI | ESP32, ESP32-S3, ESP32-C3 |
| 2.8"-3.5" | 320x240 - 480x320 | SPI or 8080 | ESP32-S3 (8080 preferred for ILI9488) |
| 4.0"-5.0" | 480x480 | RGB | ESP32-S3 (with PSRAM) |
| 7.0"+ | 800x480 - 1024x600 | RGB | ESP32-S3 (Octal PSRAM) or ESP32-P4 |
| 7.0"+ MIPI | 800x1280+ | MIPI-DSI | ESP32-P4 only |

---
