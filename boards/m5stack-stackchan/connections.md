# M5Stack StackChan (SKU K151)

Head = stock **CoreS3** (16MB flash / 8MB PSRAM). Build with `pio run -e m5stack-stackchan`.

Body wiring below follows the official docs
([StackChan](https://docs.m5stack.com/en/StackChan),
[StackChan-BSP](https://github.com/m5stack/StackChan-BSP)).

## Robot body

| Function | ESP32-S3 pin | Notes |
|---|---|---|
| IR transmitter | G5 | body IR LED |
| IR receiver | G10 | IRM-56384 |
| Body I2C SDA | G12 | shared with the head's sys I2C |
| Body I2C SCL | G11 | shared with the head's sys I2C |
| Servo serial TX / RX | G6 / G7 | **not driven by Bruce** (SCS bus servos; use the factory firmware / StackChan-BSP) |
| Grove Port A | G2 / G1 | |
| Grove Port B | G9 / G8 | default CC1101/NRF24 chip-select lands here (no radio is onboard) |
| Grove Port C | G17 / G18 | |

### Body I2C devices (on the sys bus, G12/G11)

| Device | Address | Used by Bruce |
|---|---|---|
| ST25R3916 NFC | 0x50 (8-bit write) | RFID module (ST25R3916 I2C), default in this build |
| INA226 battery monitor | 0x41 | not used (battery % comes from the AXP2101) |
| Si12T top touch panel | 0x68 | not used (screen FT6336U is the touch source) |
| PY32L020 IO expander | 0x6F | not used (gates servo power / RGB ring) |

The ST25R3916 has no IRQ line to the ESP32; the RFAL fork polls its interrupt
registers over I2C, so `ST25R_bus` is configured with SDA/SCL in the sck/miso slots
and no CS (`_initI2C()` detects that shape and uses those pins as an I2C bus — they
match `sys_i2c`, so it shares the M5Unified bus handle instead of starting a second one).

### M5Stack NFC Unit (U216) on Grove

The unit ([shop](https://shop.m5stack.com/products/nfc-universal-unit-st25r3916)) is the
same ST25R3916 at the same I2C address (0x50), just on the Grove I2C bus (Port A: G2/G1)
with no IRQ line — which is fine, the driver polls. `_initI2C()` probes the Grove bus
first and falls back to the body preset pins, so both work without touching the config:
unit plugged in → it wins; unit absent → the body NFC is used. A saved `/brucePins.conf`
overrides all of this (as with every pin).

## Audio

AW88298 I2S amp (BCK=G34, WCK=G33, DATO=G13) and ES7210 mic codec (MCLK=G0, DIN=G14)
are CoreS3 head hardware, driven through M5Unified. `HAS_SPEAKER` enables Bruce's
tones/file playback; mic recording is not wired up (ES7210 needs I2C codec setup).

## Known overlaps

- G6/G7/G5/G10 stay reserved for the body's servo UART and IR pair; the IR/RF pin
  menus list the Grove ports instead.
