# Waveshare Display Controller ICs Reference

Comprehensive reference for display controller ICs found across Waveshare ESP32 display products. Used by AI agents for driver selection, initialization, LVGL integration, and hardware configuration.

## Table of Contents

- [Controller Summary Table](#controller-summary-table)
- [ST7735S](#st7735s)
- [ST7789V](#st7789v)
- [ILI9341](#ili9341)
- [ILI9488](#ili9488)
- [GC9A01](#gc9a01)
- [GC9503](#gc9503)
- [ST7701S](#st7701s)
- [EK9716B](#ek9716b)
- [JD9365](#jd9365)
- [NV3041A](#nv3041a)
- [Controller-to-Waveshare-Product Map](#controller-to-waveshare-product-map)
- [LVGL Driver Compatibility Matrix](#lvgl-driver-compatibility-matrix)

---

## Controller Summary Table

| Controller | Max Resolution | Color Depth | Primary Interface | Typical Use | GRAM |
|------------|---------------|-------------|-------------------|-------------|------|
| ST7735S | 162x132 | 12/16/18-bit | SPI | Small TFTs (0.96"-1.8") | Yes |
| ST7789V | 320x240 | 12/16/18-bit | SPI | Mid TFTs (1.3"-2.4") | Yes |
| ILI9341 | 320x240 | 16/18-bit | SPI, 8080 | Mid TFTs (2.0"-3.2") | Yes |
| ILI9488 | 480x320 | 16/18-bit | SPI, 8080 | Larger TFTs (3.5") | Yes |
| GC9A01 | 240x240 | 12/16/18-bit | SPI | Round displays (1.28") | Yes |
| GC9503 | 480x480 | 24-bit RGB | RGB (SPI init) | Square LCD panels | No |
| ST7701S | 480x480 | 24-bit RGB | RGB (SPI init) | Round/square LCD | No |
| EK9716B | 1024x600 | 24-bit RGB | RGB | Large panels (7"+) | No |
| JD9365 | 800x1280 | 24-bit RGB | MIPI-DSI | Large portrait panels | No |
| NV3041A | 480x272 | 16/18-bit | QSPI, 8080 | Small-mid TFTs | Yes |

---

## ST7735S

### Overview

Small-format TFT controller from Sitronix. Found on Waveshare 0.96", 1.44", and 1.8" SPI TFT modules.

### Specifications

| Parameter | Value |
|-----------|-------|
| Manufacturer | Sitronix |
| Max Resolution | 162x132 (GRAM size: 132x162) |
| Common Resolutions | 80x160, 128x128, 128x160 |
| Color Depth | 12-bit (4096), 16-bit (65K RGB565), 18-bit (262K RGB666) |
| Interface | 4-wire SPI (most common), 3-wire SPI, 8-bit parallel |
| SPI Max Clock | 15 MHz (write), 6.66 MHz (read) |
| SPI Mode | Mode 0 (CPOL=0, CPHA=0) — most common |
| Operating Voltage | 2.4V - 3.3V (I/O: 1.65V - 3.3V) |
| Typical Current | 3-5 mA (display active, no backlight) |
| Sleep Current | < 5 uA |
| GRAM | 132x162x18 bits internal |
| Pixel Format | Set via COLMOD (0x3A): 0x03=12bit, 0x05=16bit, 0x06=18bit |

### Key Registers

| Register | Hex | Purpose |
|----------|-----|---------|
| SWRESET | 0x01 | Software reset |
| SLPOUT | 0x11 | Exit sleep mode |
| INVON | 0x21 | Display inversion ON (often needed) |
| DISPON | 0x29 | Display ON |
| CASET | 0x2A | Column address set |
| RASET | 0x2B | Row address set |
| RAMWR | 0x2C | Memory write (pixel data) |
| MADCTL | 0x36 | Memory data access control (rotation/mirror) |
| COLMOD | 0x3A | Interface pixel format |
| FRMCTR1 | 0xB1 | Frame rate control (normal mode) |
| PWCTR1-5 | 0xC0-0xC4 | Power control |
| GMCTRP1 | 0xE0 | Positive gamma correction |
| GMCTRN1 | 0xE1 | Negative gamma correction |

### MADCTL Rotation Values

| Rotation | MADCTL Value | Notes |
|----------|-------------|-------|
| 0 (Portrait) | 0x00 | Default |
| 90 (Landscape) | 0x60 | MX + MV |
| 180 (Portrait Inv) | 0xC0 | MX + MY |
| 270 (Landscape Inv) | 0xA0 | MY + MV |

**MADCTL bit flags:** MY=0x80, MX=0x40, MV=0x20, ML=0x10, RGB=0x00, BGR=0x08

### Init Sequence Notes

1. Hardware reset (RST LOW 10ms, HIGH, wait 120ms)
2. SWRESET (0x01), wait 150ms
3. SLPOUT (0x11), wait 500ms
4. FRMCTR1 — set frame rate
5. PWCTR1-5 — power settings
6. COLMOD (0x3A, 0x05) — 16-bit color
7. MADCTL (0x36) — orientation
8. INVON (0x21) — some panels require inversion
9. Gamma correction (0xE0, 0xE1)
10. DISPON (0x29), wait 100ms

**Waveshare-specific:** Many Waveshare ST7735S modules need display inversion ON (INVON 0x21) and may require column/row offset depending on the glass cut. Common offsets: 80x160 panel uses col_offset=26, row_offset=1 (or col_offset=1, row_offset=26 in landscape).

### LVGL Integration

- LVGL driver: `lv_st7735` (LVGL v9+) or custom `disp_spi_transfer` callback
- Color format: `LV_COLOR_FORMAT_RGB565` (16-bit, most efficient)
- Flush callback: Send CASET, RASET, RAMWR then pixel data via SPI DMA
- Typical flush: 10-15 FPS at 80x160 with 15 MHz SPI
- Partial refresh: Supported via CASET/RASET windowing

---

## ST7789V

### Overview

Mid-range TFT controller from Sitronix. The most common controller in Waveshare ESP32 SPI display products (1.3" to 2.4" panels). Highly similar command set to ST7735S but supports higher resolution.

### Specifications

| Parameter | Value |
|-----------|-------|
| Manufacturer | Sitronix |
| Max Resolution | 320x240 (GRAM: 240x320) |
| Common Resolutions | 135x240, 240x240, 240x280, 240x320 |
| Color Depth | 12-bit (4096), 16-bit (65K RGB565), 18-bit (262K RGB666) |
| Interface | 4-wire SPI (most common), 3-wire SPI, 8-bit/16-bit parallel |
| SPI Max Clock | 62.5 MHz (write), typically run at 40-80 MHz |
| SPI Mode | Mode 0 (CPOL=0, CPHA=0) or Mode 3 (CPOL=1, CPHA=1) |
| Operating Voltage | 2.4V - 3.3V (I/O: 1.65V - 3.3V) |
| Typical Current | 5-8 mA (display active, no backlight) |
| Sleep Current | < 5 uA |
| GRAM | 240x320x18 bits internal |
| Pixel Format | COLMOD (0x3A): 0x03=12bit, 0x05=16bit, 0x06=18bit |

### Key Registers

| Register | Hex | Purpose |
|----------|-----|---------|
| SWRESET | 0x01 | Software reset |
| SLPOUT | 0x11 | Exit sleep mode |
| INVON | 0x21 | Display inversion ON (usually required) |
| DISPON | 0x29 | Display ON |
| CASET | 0x2A | Column address set |
| RASET | 0x2B | Row address set |
| RAMWR | 0x2C | Memory write |
| MADCTL | 0x36 | Memory data access control |
| COLMOD | 0x3A | Pixel format |
| PORCTRL | 0xB2 | Porch control |
| GCTRL | 0xB7 | Gate control |
| VCOMS | 0xBB | VCOM setting |
| LCMCTRL | 0xC0 | LCM control |
| VDVVRHEN | 0xC2 | VDV and VRH command enable |
| VRHS | 0xC3 | VRH set |
| VDVS | 0xC4 | VDV set |
| FRCTRL2 | 0xC6 | Frame rate control 2 |
| PWCTRL1 | 0xD0 | Power control 1 |
| PVGAMCTRL | 0xE0 | Positive voltage gamma |
| NVGAMCTRL | 0xE1 | Negative voltage gamma |

### MADCTL Rotation Values

| Rotation | MADCTL Value | Notes |
|----------|-------------|-------|
| 0 (Portrait) | 0x00 | 240x320 |
| 90 (Landscape) | 0x60 | 320x240 |
| 180 (Portrait Inv) | 0xC0 | 240x320 |
| 270 (Landscape Inv) | 0xA0 | 320x240 |

**Note:** Some Waveshare panels use BGR ordering (add 0x08 to MADCTL).

### Init Sequence Notes

1. Hardware reset (RST LOW 10ms, HIGH, wait 120ms)
2. SLPOUT (0x11), wait 120ms
3. COLMOD (0x3A, 0x55) — 16-bit color (0x55 = RGB565)
4. MADCTL (0x36) — orientation + color order
5. INVON (0x21) — **almost always required on Waveshare ST7789 modules**
6. PORCTRL (0xB2) — porch timing
7. GCTRL (0xB7, 0x35) — gate control
8. VCOMS (0xBB, 0x19) — VCOM
9. Power settings (0xD0)
10. Gamma (0xE0, 0xE1)
11. DISPON (0x29)

**Waveshare-specific:** The 1.47" 172x320 and 1.69" 240x280 modules need specific column/row offsets. The 1.3" 240x240 module typically uses col_offset=0, row_offset=0 for portrait and may need offsets of 80 for landscape rotation.

### LVGL Integration

- LVGL driver: `lv_st7789` (LVGL v9+) or `esp_lcd_panel_dev_st7789` (ESP-IDF)
- Color format: `LV_COLOR_FORMAT_RGB565`
- SPI DMA flush: 30-60 FPS at 240x240 with 80 MHz SPI
- Partial refresh: Supported, efficient for UI updates
- Buffer strategy: Two half-screen buffers recommended for DMA double-buffering

---

## ILI9341

### Overview

Widely used mid-range TFT controller from Ilitek. Found on Waveshare 2.0"-3.2" display modules. Supports both SPI and 8080 parallel interface. One of the most well-documented and supported display controllers in the embedded ecosystem.

### Specifications

| Parameter | Value |
|-----------|-------|
| Manufacturer | Ilitek |
| Max Resolution | 320x240 (GRAM: 240x320) |
| Color Depth | 16-bit (65K RGB565), 18-bit (262K RGB666) |
| Interface | 4-wire SPI, 3-wire SPI, 8-bit/16-bit 8080 parallel |
| SPI Max Clock | 10 MHz (read), 32 MHz (write, some modules run at 40 MHz) |
| SPI Mode | Mode 0 (CPOL=0, CPHA=0) |
| Operating Voltage | 2.5V - 3.3V (I/O: 1.65V - 3.3V) |
| Typical Current | 7-12 mA (display active, no backlight) |
| Sleep Current | < 5 uA |
| GRAM | 240x320x18 bits internal |
| Touch Support | Often paired with XPT2046 (SPI resistive) |

### Key Registers

| Register | Hex | Purpose |
|----------|-----|---------|
| SWRESET | 0x01 | Software reset |
| RDDID | 0x04 | Read display ID (ILI9341 returns 0x00, 0x93, 0x41) |
| SLPOUT | 0x11 | Exit sleep mode |
| INVON | 0x21 | Display inversion ON |
| DISPON | 0x29 | Display ON |
| CASET | 0x2A | Column address set |
| RASET | 0x2B | Row address set |
| RAMWR | 0x2C | Memory write |
| MADCTL | 0x36 | Memory data access control |
| COLMOD | 0x3A | Pixel format (0x55=16bit, 0x66=18bit) |
| FRMCTR1 | 0xB1 | Frame rate control |
| DFUNCTR | 0xB6 | Display function control |
| PWCTR1 | 0xC0 | Power control 1 |
| PWCTR2 | 0xC1 | Power control 2 |
| VMCTR1 | 0xC5 | VCOM control 1 |
| VMCTR2 | 0xC7 | VCOM control 2 |

### MADCTL Rotation Values

| Rotation | MADCTL Value | Notes |
|----------|-------------|-------|
| 0 (Portrait) | 0x48 | MX + BGR |
| 90 (Landscape) | 0x28 | MV + BGR |
| 180 (Portrait Inv) | 0x88 | MY + BGR |
| 270 (Landscape Inv) | 0xE8 | MY + MX + MV + BGR |

### Init Sequence Notes

1. Hardware reset
2. SWRESET, wait 150ms
3. Power control (0xC0, 0xC1)
4. VCOM (0xC5, 0xC7)
5. MADCTL — rotation (0x36)
6. COLMOD — 16-bit (0x3A, 0x55)
7. Frame rate (0xB1)
8. Display function (0xB6)
9. Gamma (0xE0, 0xE1)
10. SLPOUT, wait 120ms
11. DISPON

**Waveshare-specific:** Most Waveshare ILI9341 modules include XPT2046 touch on separate SPI CS. The display CS and touch CS must not conflict. Shared SPI bus is standard.

### LVGL Integration

- LVGL driver: `lv_ili9341` (LVGL v9+) or `esp_lcd_panel_dev_ili9341` (ESP-IDF)
- Color format: `LV_COLOR_FORMAT_RGB565` with byte swap (big-endian SPI)
- ESP-IDF: Use `esp_lcd_new_panel_io_spi` + `esp_lcd_new_panel_ili9341`
- Parallel mode: Higher throughput via 8080 interface on ESP32-S3
- Buffer: Two half-screen buffers with DMA, 20-40 FPS typical on SPI

---

## ILI9488

### Overview

Larger-format TFT controller from Ilitek. Found on Waveshare 3.5" display modules. Notable for requiring 18-bit color over SPI (no 16-bit SPI mode), which impacts performance.

### Specifications

| Parameter | Value |
|-----------|-------|
| Manufacturer | Ilitek |
| Max Resolution | 480x320 |
| Color Depth | 18-bit (262K RGB666) over SPI, 16/18-bit over parallel |
| Interface | 3/4-wire SPI, 8/16/18-bit 8080 parallel |
| SPI Max Clock | 20 MHz (write), limited by 18-bit constraint |
| SPI Mode | Mode 0 (CPOL=0, CPHA=0) |
| Operating Voltage | 2.5V - 3.3V |
| Typical Current | 10-15 mA (display active, no backlight) |
| Sleep Current | < 5 uA |
| GRAM | 480x320x18 bits internal |

### Key Registers

Same command set as ILI9341 with additions:

| Register | Hex | Purpose |
|----------|-----|---------|
| IFMODE | 0xB0 | Interface mode control |
| FRMCTR1 | 0xB1 | Frame rate control |
| INTCTR | 0xB4 | Display inversion control |
| PRCTR | 0xB5 | Blanking porch control |
| DFUNCTR | 0xB6 | Display function control |
| ETMOD | 0xB7 | Entry mode set |
| PWCTR1 | 0xC0 | Power control 1 |
| PWCTR2 | 0xC1 | Power control 2 |
| PWCTR3 | 0xC2 | Power control 3 (Normal mode) |
| VMCTR1 | 0xC5 | VCOM control |
| ADJCTR3 | 0xF7 | Adjust control 3 |

### Critical SPI Color Limitation

**ILI9488 does NOT support RGB565 (16-bit) over SPI.** Only RGB666 (18-bit) is available in SPI mode, requiring 3 bytes per pixel instead of 2. This means:

- 50% more data per pixel compared to RGB565
- Significantly slower frame rates over SPI
- Software conversion from RGB565 to RGB666 needed in LVGL flush callback
- Parallel interface (8080) supports true RGB565 and is strongly recommended for performance

### LVGL Integration

- LVGL driver: `lv_ili9488` or custom flush with RGB565-to-RGB666 conversion
- SPI mode: Render in RGB565, convert to RGB666 in flush callback (add 0x00 pad byte per pixel, or expand 5-6-5 to 6-6-6)
- Parallel mode: Use RGB565 directly — much better performance
- ESP32-S3 with 8080 parallel: 15-30 FPS at 480x320
- ESP32 SPI only: 5-10 FPS at 480x320 (18-bit overhead)
- Buffer strategy: Full-frame double buffer recommended for parallel

---

## GC9A01

### Overview

Round display controller from GalaxyCore. Found on Waveshare 1.28" round LCD modules. Specifically designed for circular watch-style displays with built-in circular masking.

### Specifications

| Parameter | Value |
|-----------|-------|
| Manufacturer | GalaxyCore |
| Max Resolution | 240x240 |
| Display Shape | Round (circular visible area) |
| Color Depth | 12-bit (4096), 16-bit (65K RGB565), 18-bit (262K RGB666) |
| Interface | 4-wire SPI |
| SPI Max Clock | 60 MHz (write) |
| SPI Mode | Mode 0 (CPOL=0, CPHA=0) |
| Operating Voltage | 2.4V - 3.3V |
| Typical Current | 4-6 mA (display active, no backlight) |
| Sleep Current | < 5 uA |
| GRAM | 240x240x18 bits internal |

### Key Registers

| Register | Hex | Purpose |
|----------|-----|---------|
| SLPOUT | 0x11 | Exit sleep mode |
| INVON | 0x21 | Display inversion ON |
| DISPON | 0x29 | Display ON |
| CASET | 0x2A | Column address set |
| RASET | 0x2B | Row address set |
| RAMWR | 0x2C | Memory write |
| MADCTL | 0x36 | Memory data access control |
| COLMOD | 0x3A | Pixel format |
| Undocumented | 0xEF | Inter-register enable 2 |
| Undocumented | 0xEB | ? (part of init sequence) |
| Undocumented | 0xFE | Inter-register enable 1 |

**Note:** GC9A01 has many undocumented registers used in init sequences. Waveshare provides specific init blobs that should be used as-is.

### Init Sequence Notes

1. Hardware reset
2. 0xEF — inter-register enable
3. 0xEB, 0x14 — undocumented
4. 0xFE — inter-register enable
5. Multiple undocumented register writes (vendor-specific)
6. COLMOD (0x3A, 0x05) — 16-bit
7. MADCTL (0x36) — orientation
8. Various power and gamma settings
9. INVON (0x21)
10. SLPOUT (0x11), wait 120ms
11. DISPON (0x29), wait 20ms

**Waveshare-specific:** Use the exact init sequence from Waveshare example code. The undocumented registers are tuned for the specific panel and backlight configuration.

### LVGL Integration

- LVGL driver: `lv_gc9a01` (LVGL v9+)
- Color format: `LV_COLOR_FORMAT_RGB565`
- Round display: LVGL renders full 240x240 square buffer; the controller masks circular area
- Partial refresh: Supported but less beneficial on small round display
- Performance: 30-60 FPS at 240x240 with 60 MHz SPI
- UI consideration: Use circular layouts, avoid content in corners (not visible)

---

## GC9503

### Overview

RGB interface LCD controller from GalaxyCore. Found on Waveshare 3.4"-4.0" square (480x480) displays. Uses SPI only for initialization; pixel data is streamed via RGB parallel interface.

### Specifications

| Parameter | Value |
|-----------|-------|
| Manufacturer | GalaxyCore |
| Max Resolution | 480x480 |
| Color Depth | 24-bit RGB888 via RGB interface |
| Interface | RGB565/RGB666/RGB888 parallel (data), SPI 3-wire (init only) |
| RGB Clock | Up to 25 MHz PCLK |
| Operating Voltage | 2.5V - 3.3V |
| Typical Current | 15-20 mA |
| GRAM | None (streaming mode, requires continuous refresh from MCU) |

### RGB Interface Signals

| Signal | Count | Description |
|--------|-------|-------------|
| R[0:7] | 8 | Red data (RGB888) or R[3:7] for RGB565 |
| G[0:7] | 8 | Green data |
| B[0:7] | 8 | Blue data |
| PCLK | 1 | Pixel clock |
| HSYNC | 1 | Horizontal sync |
| VSYNC | 1 | Vertical sync |
| DE | 1 | Data enable |

### Init via SPI

The GC9503 requires SPI commands for panel initialization (power, gamma, timing), then switches to RGB mode for pixel data. The SPI interface uses a 9-bit protocol (1 D/C bit + 8 data bits) which complicates standard SPI usage.

**Waveshare approach:** Many Waveshare boards use a dedicated SPI init sequence at boot, then the ESP32 RGB peripheral takes over for continuous frame refresh.

### LVGL Integration

- Requires ESP32-S3 (has RGB LCD peripheral) or ESP32-P4
- ESP-IDF: Use `esp_lcd_new_panel_io_spi` for init, `esp_lcd_new_rgb_panel` for data
- LVGL: `LV_COLOR_FORMAT_RGB565` or `LV_COLOR_FORMAT_RGB888`
- Full-frame buffer in PSRAM required (480x480x2 = 460 KB for RGB565)
- Double buffer: 920 KB in PSRAM
- FPS: 25-40 depending on PSRAM speed and bus width
- No partial refresh — full frame must be pushed each cycle

---

## ST7701S

### Overview

RGB interface LCD controller from Sitronix. Found on Waveshare round and square 480x480 displays. Very similar architecture to GC9503 — SPI for init, RGB for pixel data.

### Specifications

| Parameter | Value |
|-----------|-------|
| Manufacturer | Sitronix |
| Max Resolution | 480x480 |
| Color Depth | 24-bit RGB888 via RGB interface |
| Interface | RGB parallel (data), SPI 3-wire (init only) |
| RGB Clock | Up to 30 MHz PCLK |
| Operating Voltage | 2.5V - 3.3V |
| Typical Current | 15-25 mA |
| GRAM | None (streaming mode) |

### Init Sequence Notes

- Uses Command2 BK0/BK1 register banks (switch via 0xFF command)
- BK0: display timing, gate/source settings
- BK1: power control, voltage settings
- Init must be done via SPI before RGB data begins
- Command format: 9-bit SPI (D/C bit + 8 data)

### Waveshare Products

Found in:
- ESP32-S3-Touch-LCD-1.28 (round, 240x240 variant uses different controller)
- ESP32-S3-LCD-1.47 (some variants)
- Various 480x480 square and round modules

### LVGL Integration

- Same as GC9503: ESP32-S3 RGB peripheral + PSRAM frame buffer
- `esp_lcd_new_panel_st7701` available in ESP-IDF component registry
- Performance characteristics identical to GC9503

---

## EK9716B

### Overview

Large-format RGB interface LCD controller. Found on Waveshare 7" (800x480 and 1024x600) display modules paired with ESP32-S3.

### Specifications

| Parameter | Value |
|-----------|-------|
| Max Resolution | 1024x600 |
| Color Depth | 24-bit RGB888 |
| Interface | RGB parallel only |
| RGB Clock | Up to 50 MHz PCLK |
| Operating Voltage | 3.3V |
| Typical Current | 20-30 mA (controller only, panel draws more) |
| GRAM | None (streaming) |

### LVGL Integration

- Requires ESP32-S3 with 8 MB+ PSRAM (Octal)
- Frame buffer: 1024x600x2 = 1.17 MB per buffer (RGB565)
- Double buffer consumes 2.34 MB PSRAM
- Practical FPS: 15-25 depending on PSRAM bandwidth
- Direct mode rendering recommended for large displays
- ESP32-P4 significantly improves performance for this size

---

## JD9365

### Overview

MIPI-DSI interface controller from Jadard. Found on Waveshare ESP32-P4 display products with 800x1280 panels.

### Specifications

| Parameter | Value |
|-----------|-------|
| Manufacturer | Jadard |
| Max Resolution | 800x1280 |
| Color Depth | 24-bit RGB888 |
| Interface | MIPI-DSI (2 or 4 lanes) |
| DSI Clock | Up to 500 MHz per lane |
| Operating Voltage | 1.8V / 3.3V |
| GRAM | None (video mode) |

### Notes

- Only usable with ESP32-P4 (has MIPI-DSI peripheral)
- Requires MIPI-DSI PHY initialization
- High bandwidth: 4-lane DSI can push 60 FPS at 800x1280
- ESP-IDF MIPI-DSI driver handles lane configuration

---

## NV3041A

### Overview

QSPI-capable TFT controller from Novatek. Found on some Waveshare compact display modules. Supports both traditional 8080 parallel and QSPI (quad SPI) for higher throughput without many data pins.

### Specifications

| Parameter | Value |
|-----------|-------|
| Manufacturer | Novatek |
| Max Resolution | 480x272 |
| Color Depth | 16-bit (RGB565), 18-bit (RGB666) |
| Interface | QSPI, 8-bit 8080 parallel |
| QSPI Max Clock | 80 MHz |
| Operating Voltage | 2.5V - 3.3V |
| Typical Current | 5-8 mA |
| GRAM | Yes |

### QSPI Advantage

QSPI uses 4 data lines simultaneously, achieving 4x throughput of standard SPI:
- Standard SPI at 80 MHz: 80 Mbps
- QSPI at 80 MHz: 320 Mbps
- Sufficient for 30+ FPS at 480x272 RGB565

### LVGL Integration

- Use `esp_lcd_panel_io_qspi` on ESP32-S3
- Color format: `LV_COLOR_FORMAT_RGB565`
- Frame buffer in PSRAM with DMA transfer
- Performance comparable to 8080 parallel with fewer GPIO pins

---

## Controller-to-Waveshare-Product Map

| Controller | Waveshare Product Examples |
|------------|---------------------------|
| ST7735S | 0.96" LCD, 1.44" LCD, 1.8" LCD modules |
| ST7789V | 1.3" LCD, 1.47" LCD, 1.69" LCD, 2.0" LCD, 2.4" LCD |
| ILI9341 | 2.4" Touch LCD, 2.8" Touch LCD, 3.2" Touch LCD |
| ILI9488 | 3.5" Touch LCD (SPI), 3.5" RPi LCD |
| GC9A01 | 1.28" Round LCD |
| GC9503 | ESP32-S3-Touch-LCD-4.0 |
| ST7701S | ESP32-S3-Touch-LCD-1.28 (round 480x480), various 480x480 |
| EK9716B | ESP32-S3-Touch-LCD-7.0 |
| JD9365 | ESP32-P4-NANO, ESP32-P4 boards with 7"+ MIPI panels |
| NV3041A | ESP32-S3 compact QSPI display modules |

---

## LVGL Driver Compatibility Matrix

| Controller | LVGL v8.x | LVGL v9.x | ESP-IDF Component | Interface |
|------------|-----------|-----------|-------------------|-----------|
| ST7735S | Custom flush | `lv_st7735` | `esp_lcd_st7735` | SPI |
| ST7789V | Custom flush | `lv_st7789` | `esp_lcd_st7789` | SPI |
| ILI9341 | Custom flush | `lv_ili9341` | `esp_lcd_ili9341` | SPI/8080 |
| ILI9488 | Custom flush + RGB666 conv | `lv_ili9488` | `esp_lcd_ili9488` | SPI(18bit)/8080 |
| GC9A01 | Custom flush | `lv_gc9a01` | `esp_lcd_gc9a01` | SPI |
| GC9503 | RGB panel driver | RGB panel driver | `esp_lcd_gc9503` | RGB+SPI init |
| ST7701S | RGB panel driver | RGB panel driver | `esp_lcd_st7701` | RGB+SPI init |
| EK9716B | RGB panel driver | RGB panel driver | `esp_lcd_ek9716b` | RGB |
| JD9365 | N/A | MIPI-DSI driver | `esp_lcd_jd9365` | MIPI-DSI |
| NV3041A | Custom QSPI | QSPI driver | `esp_lcd_nv3041a` | QSPI/8080 |

### Common LVGL Buffer Strategies by Interface

| Interface | Buffer Strategy | Memory Location | Notes |
|-----------|----------------|-----------------|-------|
| SPI (small) | 2x partial (10-20 lines) | Internal SRAM | Low memory, decent FPS |
| SPI (mid) | 2x half-screen | Internal SRAM or PSRAM | Good balance |
| 8080 Parallel | 2x half-screen | PSRAM | Higher throughput |
| RGB | 2x full-frame | PSRAM (required) | Continuous refresh needed |
| QSPI | 2x partial or full | PSRAM | Depends on resolution |
| MIPI-DSI | 2x full-frame | PSRAM (required) | Highest bandwidth |

---
