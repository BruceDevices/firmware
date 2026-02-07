# Display HAL (Arduino_GFX, LovyanGFX, TFT_eSPI, M5GFX)

This folder provides multiple display backends. Each backend expects a different set of macros to be defined
in your `pins_arduino.h` (or board header). Use only the macros that match the backend you compile.

All examples below are complete and can be copied into `pins_arduino.h` and edited with your board values.

Add the one of the following libs to your env:
```
	;moononournation/GFX Library for Arduino @ ^1.5.5
	;lovyan03/LovyanGFX @ ^1.2.7
	;m5stack/M5Unified @ ^0.2.11
	;m5stack/M5gfx @ ^0.2.18

	; For ESP32-S3, Lovyan 1.2.7 isn't up to date for Arduino 3.3.4, use Develop
	;https://github.com/lovyan03/LovyanGFX#develop
```
## LovyanGFX (USE_LOVYANGFX)

LovyanGFX here supports SPI, I2C, and Parallel 8-bit. You must define:
- The panel type (`LOVYAN_PANEL`)
- The bus type (`LOVYAN_BUS`)
- One bus selector macro (`LOVYAN_SPI_BUS` or `LOVYAN_I2C_BUS` or `LOVYAN_8PARALLEL_BUS`)
- The panel config macros used by `lib/HAL/display/lovyan.cpp`

### Required panel macros (all buses)

These are always required:
```c
#define TFT_CS        5
#define TFT_RST       12
#define TFT_BUSY_PIN  -1
#define TFT_WIDTH     135
#define TFT_HEIGHT    240
#define TFT_OFFSET_X  52
#define TFT_OFFSET_Y  40
#define TFT_INVERTION 1      // bool: 0/1
#define TFT_RGB_ORDER 0      // bool: 0=BGR, 1=RGB
#define TFT_MEM_WIDTH  240
#define TFT_MEM_HEIGHT 135
```

### SPI bus (LOVYAN_SPI_BUS)

Required macros for SPI:
```c
#define USE_LOVYANGFX 1
#define LOVYAN_PANEL Panel_ST7789
#define LOVYAN_BUS   Bus_SPI
#define LOVYAN_SPI_BUS 1

#define TFT_SPI_HOST  SPI3_HOST
#define TFT_SPI_MODE  0
#define TFT_WRITE_FREQ 40000000
#define TFT_READ_FREQ  15000000
#define TFT_SPI_3WIRE  true
#define TFT_USE_LOCK  true
#define TFT_SCLK 13
#define TFT_MOSI 15
#define TFT_MISO -1
#define TFT_DC   14

// Panel macros (from the previous section)
#define TFT_CS        5
#define TFT_RST       12
#define TFT_BUSY_PIN  -1
#define TFT_WIDTH     135
#define TFT_HEIGHT    240
#define TFT_OFFSET_X  52
#define TFT_OFFSET_Y  40
#define TFT_INVERTION 1
#define TFT_RGB_ORDER 0
#define TFT_MEM_WIDTH  240
#define TFT_MEM_HEIGHT 320
```

### I2C bus (LOVYAN_I2C_BUS)

Required macros for I2C:
```c
#define USE_LOVYANGFX 1
#define LOVYAN_PANEL Panel_SSD1306
#define LOVYAN_BUS   Bus_I2C
#define LOVYAN_I2C_BUS 1

#define TFT_I2C_PORT  0
#define TFT_I2C_WRITE 400000
#define TFT_I2C_READ  400000
#define TFT_SDA 21
#define TFT_SCL 22
#define TFT_ADDR 0x3C

// Panel macros (from the previous section)
#define TFT_CS        -1
#define TFT_RST       -1
#define TFT_BUSY_PIN  -1
#define TFT_WIDTH     128
#define TFT_HEIGHT    64
#define TFT_OFFSET_X  0
#define TFT_OFFSET_Y  0
#define TFT_INVERTION 0
#define TFT_RGB_ORDER 0
#define TFT_MEM_WIDTH  128
#define TFT_MEM_HEIGHT 64
```

### Parallel 8-bit bus (LOVYAN_8PARALLEL_BUS)

Required macros for 8-bit parallel:
```c
#define USE_LOVYANGFX 1
#define LOVYAN_PANEL Panel_ILI9341
#define LOVYAN_BUS   Bus_Parallel8
#define LOVYAN_8PARALLEL_BUS 1

#define TFT_WRITE_FREQ 20000000
#define TFT_WR  4
#define TFT_RD  -1
#define TFT_DC  2
#define TFT_D0 12
#define TFT_D1 13
#define TFT_D2 26
#define TFT_D3 25
#define TFT_D4 17
#define TFT_D5 16
#define TFT_D6 27
#define TFT_D7 14

// Panel macros (from the previous section)
#define TFT_CS        5
#define TFT_RST       33
#define TFT_BUSY_PIN  -1
#define TFT_WIDTH     240
#define TFT_HEIGHT    320
#define TFT_OFFSET_X  0
#define TFT_OFFSET_Y  0
#define TFT_INVERTION 0
#define TFT_RGB_ORDER 1
#define TFT_MEM_WIDTH  240
#define TFT_MEM_HEIGHT 320
```

### Panel selection (LOVYAN_PANEL)

Valid values in this project include:
`Panel_ST7789`, `Panel_GC9A01`, `Panel_GDEW0154M09`, `Panel_HX8357B`, `Panel_HX8357D`, `Panel_ILI9163`,
`Panel_ILI9341`, `Panel_ILI9342`, `Panel_ILI9481`, `Panel_ILI9486`, `Panel_ILI9488`, `Panel_IT8951`,
`Panel_RA8875`, `Panel_SH1106`, `Panel_SH1107`, `Panel_SSD1306`, `Panel_SSD1327`, `Panel_SSD1331`,
`Panel_SSD1351`, `Panel_SSD1357`, `Panel_SSD1963`, `Panel_ST7735`, `Panel_ST7735S`, `Panel_ST7796`.

## Arduino_GFX (USE_ARDUINO_GFX)

Arduino_GFX uses two selections:
- `TFT_DATABUS_N` to select the bus type.
- `TFT_DISPLAY_DRIVER_N` to select the panel driver.

### Data Bus selection (`TFT_DATABUS_N`)

`TFT_DATABUS_N` values:
- `0` = `Arduino_HWSPI` (shared SPI)
- `1` = `Arduino_ESP32QSPI` (quad SPI)
- `2` = `Arduino_ESP32PAR8Q` (parallel 8-bit)
- `3` = `Arduino_ESP32RGBPanel` (RGB panel)

Bus 0: `Arduino_HWSPI` (SPI)
```c
// Arduino_GFX: SPI
#define USE_ARDUINO_GFX 1
#define TFT_DATABUS_N 0
#define TFT_DC   14
#define TFT_CS   5
#define TFT_SCLK 13
#define TFT_MOSI 15
#define TFT_MISO -1
```

Bus 1: `Arduino_ESP32QSPI` (Quad SPI)
```c
// Arduino_GFX: QSPI
#define USE_ARDUINO_GFX 1
#define TFT_DATABUS_N 1
#define TFT_CS   5
#define TFT_SCLK 13
#define TFT_D0   15
#define TFT_D1   2
#define TFT_D2   4
#define TFT_D3   12
```

Bus 2: `Arduino_ESP32PAR8Q` (Parallel 8-bit)
```c
// Arduino_GFX: PAR8
#define USE_ARDUINO_GFX 1
#define TFT_DATABUS_N 2
#define TFT_DC  21
#define TFT_CS  22
#define TFT_WR  23
#define TFT_RD  -1
#define TFT_D0  12
#define TFT_D1  13
#define TFT_D2  26
#define TFT_D3  25
#define TFT_D4  17
#define TFT_D5  16
#define TFT_D6  27
#define TFT_D7  14
```

Bus 3: `Arduino_ESP32RGBPanel` (RGB panel)
```c
// Arduino_GFX: RGB panel
#define USE_ARDUINO_GFX 1
#define TFT_DATABUS_N 3
#define TFT_DE   40
#define TFT_VSYNC 41
#define TFT_HSYNC 39
#define TFT_PCLK  42
#define TFT_R0  45
#define TFT_R1  48
#define TFT_R2  47
#define TFT_R3  21
#define TFT_R4  14
#define TFT_G0  5
#define TFT_G1  6
#define TFT_G2  7
#define TFT_G3  15
#define TFT_G4  16
#define TFT_G5  4
#define TFT_B0  8
#define TFT_B1  3
#define TFT_B2  46
#define TFT_B3  9
#define TFT_B4  1
#define TFT_HSYNC_POL         0
#define TFT_HSYNC_FRONT_PORCH 10
#define TFT_HSYNC_PULSE_WIDTH 8
#define TFT_HSYNC_BACK_PORCH  50
#define TFT_VSYNC_POL         0
#define TFT_VSYNC_FRONT_PORCH 10
#define TFT_VSYNC_PULSE_WIDTH 8
#define TFT_VSYNC_BACK_PORCH  20
#define TFT_PCLK_ACTIVE_NEG   0
#define TFT_PREF_SPEED        16000000
```

Bus 4: `Arduino_ESP32DSIPanel` (DSI used in ESP32-P4)
```c
// Example of T-Display P4 TFT
#define TFT_DATABUS_N 4
#define TFT_HSYNC_PULSE_WIDTH 	28
#define TFT_HSYNC_BACK_PORCH	26
#define TFT_HSYNC_FRONT_PORCH	20
#define TFT_VSYNC_PULSE_WIDTH	2
#define TFT_VSYNC_BACK_PORCH	22
#define TFT_VSYNC_FRONT_PORCH	200
#define TFT_PREF_SPEED			60000000

#define TFT_DISPLAY_DRIVER_N 	50
#define TFT_WIDTH				540
#define TFT_HEIGHT				1168
#define TFT_RST					-1
#define TFT_DSI_INIT			hi8561_lcd_init
```
### Display driver selection (`TFT_DISPLAY_DRIVER_N`)

All drivers use `TFT_DISPLAY_DRIVER_N`. The grouping below matches the constructor signature used in
`lib/HAL/display/ardgfx.cpp`.

Common macros used by all non-RGB/DSI drivers:
```c
#define TFT_RST       12
#define TFT_ROTATION  3
#define TFT_IPS       0
#define TFT_WIDTH     135
#define TFT_HEIGHT    240
#define TFT_COL_OFS1  0
#define TFT_ROW_OFS1  0
#define TFT_COL_OFS2  0
#define TFT_ROW_OFS2  0
```

Group A: `[bus,rst,r,ips,w,h,ofs1,ofs2]` -> `TFT_DISPLAY_DRIVER_N` 0-22
Drivers: ST7735, ST7789, ST7796, ST77916, ILI9341, GC9A01, GC9C01, GC9D01, GC9106, GC9107,
HX8347C, HX8347D, HX8352C, HX8369A, NT35310, NT35510, NT39125, NV3007, NV3023, NV3041A, OTM8009A,
JBT6K71, AXS15231B.

Example (ST7789, SPI):
```c
#define TFT_DISPLAY_DRIVER_N 1
// plus the common macros above
```

Group B: `[bus,rst,r,ips]` -> `TFT_DISPLAY_DRIVER_N` 23-35
Drivers: ILI9331, ILI9342, ILI9481_18bit, ILI9486, ILI9486_18bit, ILI9488, ILI9488_18bit,
ILI9488_3bit, ILI9806, HX8357A, HX8357B, R61529, RM67162.

Example:
```c
#define TFT_DISPLAY_DRIVER_N 26
#define TFT_RST      12
#define TFT_ROTATION 1
#define TFT_IPS      0
```

Group C: `[bus,rst,r,w,h,ofs1,ofs2]` -> `TFT_DISPLAY_DRIVER_N` 36-43
Drivers: SSD1283A, SSD1331, SSD1351, SH8601, RM690B0, CO5300, JD9613, SEPS525.

Example:
```c
#define TFT_DISPLAY_DRIVER_N 38
#define TFT_RST      12
#define TFT_ROTATION 0
#define TFT_WIDTH    128
#define TFT_HEIGHT   128
#define TFT_COL_OFS1 0
#define TFT_ROW_OFS1 0
#define TFT_COL_OFS2 0
#define TFT_ROW_OFS2 0
```

Group D: `[bus,rst,r]` -> `TFT_DISPLAY_DRIVER_N` 44
Driver: ILI9225.

Example:
```c
#define TFT_DISPLAY_DRIVER_N 44
#define TFT_RST      12
#define TFT_ROTATION 0
```

Group E: `[bus,rst]` -> `TFT_DISPLAY_DRIVER_N` 45-46
Drivers: SPD2010, WEA2012.

Example:
```c
#define TFT_DISPLAY_DRIVER_N 46
#define TFT_RST 12
```

Group F: `[bus,rst,w,h]` -> `TFT_DISPLAY_DRIVER_N` 47-48
Drivers: SSD1306, SH1106.

Example:
```c
#define TFT_DISPLAY_DRIVER_N 47
#define TFT_RST    12
#define TFT_WIDTH  128
#define TFT_HEIGHT 64
```

Group G: `[RGB]` -> `TFT_DISPLAY_DRIVER_N` 49
Driver: Arduino_RGB_Display. Must be used with `TFT_DATABUS_N 3`.

Group H: `[DSI]` -> `TFT_DISPLAY_DRIVER_N` 50
Driver: Arduino_DSI_Display (not supported by the current init in `ardgfx.cpp`).

## TFT_eSPI (USE_TFT_ESPI)

This backend uses the official TFT_eSPI configuration style. Use the macros from
`lib/TFT_eSPI/User_Setups` as your template. Copy one of those setup files and place the macros into
your `pins_arduino.h` or board `.ini` defines.

Example (based on a typical ST7789 User_Setup):
```c
// TFT_eSPI (example)
#define USE_TFT_ESPI 1
#define ST7789_2_DRIVER
#define TFT_WIDTH  135
#define TFT_HEIGHT 240
#define TFT_RGB_ORDER TFT_BGR
#define TFT_MOSI  15
#define TFT_SCLK  13
#define TFT_CS     5
#define TFT_DC    14
#define TFT_RST   12
#define TFT_BL    27
#define TFT_BACKLIGHT_ON 1
```

## M5Unified / M5GFX (USE_M5GFX)

No extra macros are required. The library auto-detects the hardware and configures the display
automatically, at the cost of additional flash usage.

Example:
```c
#define USE_M5GFX 1
```
