# ODG_S3_ILI9341_SHIELD bring-up

## Status

- DISPLAY_STATUS=ENABLED_REAL
- TOUCH_STATUS=ENABLED_REAL_XPT2046_SEPARATE_SPI
- SD_STATUS=DISABLED_SD_CS_UNKNOWN
- DHT11_STATUS=DISABLED_CONFLICTS_GPIO2
- POT_STATUS=DISABLED_CONFLICTS_GPIO1
- OLED_STATUS=DISABLED_CONFLICTS_GPIO41_GPIO42
- RF_STATUS=DISABLED_FIRST_BUILD
- CLASSIC_UART_STATUS=RESERVED_GPIO17_GPIO18

## Fixed pinout

### Display ILI9341 SPI

- TFT_MOSI=45
- TFT_MISO=46
- TFT_SCLK=3
- TFT_CS=14
- TFT_DC=47
- TFT_RST=21
- TFT_BL=-1
- BACKLIGHT=-1

GPIO3/GPIO45/GPIO46 are ESP32-S3 strapping pins and are reserved exclusively for the TFT bus in this profile.

### Touch XPT2046 separate SPI

- TOUCH_CLK=42
- TOUCH_CS=1
- TOUCH_MOSI=2
- TOUCH_MISO=41
- TOUCH_IRQ=-1

GPIO1/GPIO2/GPIO41/GPIO42 are reserved exclusively for the XPT2046 touch controller.

### Reserved future UART

- GPIO17=S3_TX_TO_CLASSIC_RX
- GPIO18=S3_RX_FROM_CLASSIC_TX

Classic UART remains disabled until display and touch are stable.

## Physical validation sequence

1. DISPLAY_OK: confirm ILI9341 initializes and Bruce UI is visible.
2. TOUCH_RAW_OK: confirm XPT2046 raw readings change when touching the panel.
3. TOUCH_ROTATION_OK: confirm ROTATION=1 maps touch to landscape UI coordinates.
4. SD_PROBE only after SD_CS continuity: microSD remains disabled until SD_CS is known by continuity testing.
5. CLASSIC_UART after display/touch stable: only enable GPIO17/GPIO18 UART once the display and touch profile is proven.
