# Common Devices Reference

Cross-platform device/module reference for Raspberry Pi and ESP32 GPIO configuration. Used to determine pin requirements, I2C addresses, voltage levels, and platform-specific wiring guidance.

## Table of Contents

- [Category 1: Sensors — Environmental](#category-1-sensors--environmental)
  - [BME280](#bme280-temperaturehumiditypressure)
  - [BMP280](#bmp280-temperaturepressure--no-humidity)
  - [DHT22 / AM2302](#dht22--am2302-temperaturehumidity)
  - [DS18B20](#ds18b20-temperature-1-wire)
  - [SHT31](#sht31-temperaturehumidity)
- [Category 2: Sensors — Motion/Position](#category-2-sensors--motionposition)
  - [MPU6050](#mpu6050-6-axis-imu--accelerometer--gyroscope)
  - [ADXL345](#adxl345-3-axis-accelerometer)
  - [HMC5883L / QMC5883L](#hmc5883l--qmc5883l-magnetometercompass)
  - [NEO-6M / NEO-7M / NEO-M8N](#neo-6m--neo-7m--neo-m8n-gps-module)
- [Category 3: Sensors — Analog/Power](#category-3-sensors--analogpower)
  - [ADS1115](#ads1115-16-bit-adc-4-channel)
  - [INA219](#ina219-currentvoltagepower-monitor)
- [Category 4: Displays](#category-4-displays)
  - [SSD1306](#ssd1306-oled-display-128x64-or-128x32)
  - [ST7789](#st7789-tft-color-display-typically-240x240-or-240x320)
  - [HD44780](#hd44780-character-lcd-16x2-or-20x4-via-pcf8574-i2c-backpack)
  - [e-Paper / e-Ink Display](#e-paper--e-ink-display-waveshare-style)
- [Category 5: Communication](#category-5-communication)
  - [nRF24L01](#nrf24l01-24ghz-radio-transceiver)
  - [HC-05 / HC-06](#hc-05--hc-06-bluetooth-classic-serial)
  - [SX1276 / RFM95W](#sx1276--rfm95w-lora-radio)
  - [MCP2515](#mcp2515-can-bus-controller)
  - [ENC28J60](#enc28j60-spi-ethernet-controller)
- [Category 6: Motor Control](#category-6-motor-control)
  - [PCA9685](#pca9685-16-channel-pwmservo-driver)
  - [L298N](#l298n-dual-h-bridge-motor-driver)
  - [DRV8825 / A4988](#drv8825--a4988-stepper-motor-driver)
- [Category 7: Other](#category-7-other)
  - [MCP23017](#mcp23017-16-channel-gpio-expander)
  - [WS2812B / NeoPixels](#ws2812b--neopixels-addressable-rgb-leds)
  - [Rotary Encoder](#rotary-encoder-incremental-with-push-button)
  - [Relay Modules](#relay-modules-1248-channel)
  - [MAX31855 / MAX6675](#max31855--max6675-thermocouple-interface)
- [I2C Address Collision Map](#i2c-address-collision-map)

---

## Category 1: Sensors — Environmental

### BME280 (Temperature/Humidity/Pressure)

| Field | Value |
|-------|-------|
| Interface | I2C (default), SPI |
| Required Pins | I2C: SDA, SCL. SPI: MOSI, MISO, SCLK, CS |
| I2C Address | 0x76 (SDO→GND), 0x77 (SDO→VDD) |
| Voltage | 1.8–3.6V (most breakouts accept 3.3V or 5V input) |
| Pull-ups | Provided on most breakouts |

**RPi Notes:**
- Use I2C1 (GPIO2 SDA, GPIO3 SCL)
- Enable with `dtparam=i2c_arm=on`
- SPI mode: SPI0 (GPIO10 MOSI, GPIO9 MISO, GPIO11 SCLK, GPIO8 CE0)

**ESP32 Notes:**
- Any GPIO pair for I2C via GPIO matrix
- Default Wire library: GPIO21 SDA, GPIO22 SCL
- SPI VSPI default: GPIO23 MOSI, GPIO19 MISO, GPIO18 SCLK, GPIO5 CS

**Gotchas:**
- ADDRESS COLLISION with BMP280 — both use 0x76/0x77
- Cannot use BME280 + BMP280 on same I2C bus unless one is on alternate address
- Use I2C multiplexer (TCA9548A) if both needed at same address

---

### BMP280 (Temperature/Pressure — no humidity)

| Field | Value |
|-------|-------|
| Interface | I2C (default), SPI |
| Required Pins | I2C: SDA, SCL. SPI: MOSI, MISO, SCLK, CS |
| I2C Address | 0x76 (SDO→GND), 0x77 (SDO→VDD) |
| Voltage | 1.8–3.6V (most breakouts accept 3.3V or 5V input) |
| Pull-ups | Provided on most breakouts |

**RPi Notes:**
- Use I2C1 (GPIO2 SDA, GPIO3 SCL)
- Enable with `dtparam=i2c_arm=on`
- SPI mode: SPI0 (GPIO10 MOSI, GPIO9 MISO, GPIO11 SCLK, GPIO8 CE0)

**ESP32 Notes:**
- Any GPIO pair for I2C via GPIO matrix
- Default Wire library: GPIO21 SDA, GPIO22 SCL
- SPI VSPI default: GPIO23 MOSI, GPIO19 MISO, GPIO18 SCLK, GPIO5 CS

**Gotchas:**
- ADDRESS COLLISION with BME280
- Registers are mostly compatible but not identical — don't assume BME280 libraries work on BMP280
- Cheap clone boards sometimes mislabel BMP280 as BME280

---

### DHT22 / AM2302 (Temperature/Humidity)

| Field | Value |
|-------|-------|
| Interface | Single-wire proprietary protocol (NOT 1-Wire/Dallas) |
| Required Pins | DATA (1 GPIO) |
| I2C Address | N/A |
| Voltage | 3.3V–5V (data pin is 3.3V logic compatible) |
| Pull-ups | 4.7kΩ–10kΩ on data line (some breakouts include it) |

**RPi Notes:**
- Use any available GPIO
- No overlay needed
- Timing-sensitive — occasional read failures are normal; retry in software
- Minimum 2-second sampling interval (do not exceed 0.5 Hz)

**ESP32 Notes:**
- Use any GPIO
- Avoid strapping pins (GPIO0, 2, 5, 12, 15)
- Avoid input-only pins (GPIO34-39) — data pin is bidirectional

**Gotchas:**
- THIS IS NOT 1-Wire PROTOCOL — do not use w1-gpio overlay
- Do not confuse with DS18B20
- Requires specific library (e.g., Adafruit DHT library)
- DHT11 is the cheaper/less accurate variant — same protocol

---

### DS18B20 (Temperature, 1-Wire)

| Field | Value |
|-------|-------|
| Interface | 1-Wire (Dallas protocol) |
| Required Pins | DQ (data, 1 GPIO) |
| I2C Address | N/A |
| Voltage | 3.0–5.5V (supports parasitic power: data + GND only) |
| Pull-ups | 4.7kΩ required on data line |

**RPi Notes:**
- Default: GPIO4 with `dtoverlay=w1-gpio`
- Custom pin: `dtoverlay=w1-gpio,gpiopin=N`
- Parasitic power: `dtoverlay=w1-gpio-pullup,gpiopin=N`
- Multiple sensors can share one data pin (each has unique 64-bit address)

**ESP32 Notes:**
- Any GPIO via OneWire library
- Avoid strapping pins
- Avoid input-only pins (GPIO34-39)

**Gotchas:**
- Multiple sensors on one bus is a key feature — each has factory-programmed unique ID
- Parasitic power mode can cause issues with cable runs >3m
- Counterfeit DS18B20s are common — may have worse accuracy or fail parasitic power

---

### SHT31 (Temperature/Humidity)

| Field | Value |
|-------|-------|
| Interface | I2C |
| Required Pins | SDA, SCL. Optional: ALERT (interrupt), RST (active-low reset) |
| I2C Address | 0x44 (ADDR→GND), 0x45 (ADDR→VDD) |
| Voltage | 2.4–5.5V (most breakouts have regulator) |
| Pull-ups | Provided on most breakouts |

**RPi Notes:**
- Use I2C1 (GPIO2 SDA, GPIO3 SCL)
- Enable with `dtparam=i2c_arm=on`

**ESP32 Notes:**
- Any GPIO pair for I2C via GPIO matrix

**Gotchas:**
- ADDRESS COLLISION — INA219 also uses 0x44/0x45
- Has built-in heater element for self-diagnostics (enable via register)
- Higher accuracy than DHT22

---

## Category 2: Sensors — Motion/Position

### MPU6050 (6-Axis IMU — Accelerometer + Gyroscope)

| Field | Value |
|-------|-------|
| Interface | I2C |
| Required Pins | SDA, SCL. Optional: INT (interrupt — recommended for DMP) |
| I2C Address | 0x68 (AD0→GND), 0x69 (AD0→VDD) |
| Voltage | 3.3V (VDD). Most breakouts accept 3.3V–5V input |
| Pull-ups | Provided on most breakouts |

**RPi Notes:**
- Use I2C1 (GPIO2/3)
- Connect INT to any GPIO if using interrupt-driven reads
- No specific overlay needed beyond `dtparam=i2c_arm=on`

**ESP32 Notes:**
- Any GPIO pair for I2C
- Connect INT to any input-capable GPIO
- Input-only pins (GPIO34-39) are fine for INT output

**Gotchas:**
- ADDRESS COLLISION — 0x68 collides with DS3231 RTC
- If using both, put one on alternate address or use I2C multiplexer
- Has built-in DMP (Digital Motion Processor) — use INT pin for DMP-ready interrupts
- MPU9250 is the 9-axis successor (adds magnetometer) — same I2C address scheme

---

### ADXL345 (3-Axis Accelerometer)

| Field | Value |
|-------|-------|
| Interface | I2C, SPI |
| Required Pins | I2C: SDA, SCL. SPI: MOSI, MISO, SCLK, CS. Optional: INT1, INT2 |
| I2C Address | 0x53 (ALT ADDRESS/SDO→GND), 0x1D (ALT ADDRESS/SDO→VDD) |
| Voltage | 2.0–3.6V (most breakouts accept 3.3V–5V) |
| Pull-ups | Provided on most breakouts |

**RPi Notes:**
- I2C1 (GPIO2/3) or SPI0 (GPIO7-11)
- Interrupt pins to any available GPIO

**ESP32 Notes:**
- Any GPIO pair for I2C, any pins for SPI via GPIO matrix
- Avoid flash pins (GPIO6-11) for SPI

**Gotchas:**
- Default I2C address 0x53 is uncommon in collisions
- SPI mode is activated by pulling CS low — if CS is floating, chip may not respond on I2C
- Has configurable tap detection, freefall detection, and activity/inactivity interrupts

---

### HMC5883L / QMC5883L (Magnetometer/Compass)

| Field | Value |
|-------|-------|
| Interface | I2C |
| Required Pins | SDA, SCL. Optional: DRDY (data ready interrupt) |
| I2C Address | HMC5883L: 0x1E (fixed). QMC5883L: 0x0D (fixed) |
| Voltage | 2.16–3.6V (most breakouts accept 3.3V–5V) |
| Pull-ups | Provided on most breakouts |

**RPi Notes:**
- I2C1 (GPIO2/3)
- No special overlay

**ESP32 Notes:**
- Any GPIO pair for I2C

**Gotchas:**
- HMC5883L and QMC5883L have DIFFERENT I2C addresses and DIFFERENT register maps
- They are NOT interchangeable in software
- Many cheap "HMC5883L" boards actually contain QMC5883L chips — check chip marking
- GY-271 boards may have either chip
- Libraries must match the actual chip

---

### NEO-6M / NEO-7M / NEO-M8N (GPS Module)

| Field | Value |
|-------|-------|
| Interface | UART (primary), I2C (some models), SPI (some models) |
| Required Pins | UART: TX, RX. Optional: PPS (pulse-per-second for precision timing) |
| I2C Address | 0x42 (when I2C interface is used, some modules only) |
| Voltage | 2.7–3.6V (most breakouts accept 3.3V–5V) |
| Pull-ups | N/A for UART |

**RPi Notes:**
- UART0/PL011 on GPIO14 (TX) / GPIO15 (RX)
- On Pi 3/4/Zero2W, Bluetooth uses PL011 — add `dtoverlay=disable-bt` or `dtoverlay=miniuart-bt`
- On Pi 5, no BT conflict
- For PPS: `dtoverlay=pps-gpio,gpiopin=N`
- RPi TX → GPS RX, RPi RX → GPS TX (crossover)

**ESP32 Notes:**
- Use UART1 or UART2 on any GPIO pair via GPIO matrix
- UART0 is typically USB-serial debug — avoid reassigning
- PPS to any input-capable GPIO

**Gotchas:**
- Cold start can take 1-5 minutes to get a fix
- Needs clear sky view for antenna
- NMEA output at 9600 baud by default
- PPS signal is critical for NTP servers or precision timing applications

---

## Category 3: Sensors — Analog/Power

### ADS1115 (16-bit ADC, 4-channel)

| Field | Value |
|-------|-------|
| Interface | I2C |
| Required Pins | SDA, SCL. Optional: ALERT/RDY (interrupt/data-ready) |
| I2C Address | 0x48 (ADDR→GND), 0x49 (ADDR→VDD), 0x4A (ADDR→SDA), 0x4B (ADDR→SCL) |
| Voltage | 2.0–5.5V |
| Pull-ups | Provided on most breakouts |

**RPi Notes:**
- I2C1 (GPIO2/3)
- Essential for RPi projects needing analog inputs — RPi has no built-in ADC
- Enable with `dtparam=i2c_arm=on`

**ESP32 Notes:**
- Any GPIO pair for I2C
- Less commonly needed since ESP32 has built-in 12-bit ADC
- Useful for higher resolution (16-bit) or to avoid ADC2/WiFi conflicts

**Gotchas:**
- 0x48 address collides with TMP102 temperature sensor
- Can run 4 devices on one I2C bus (4 addresses) for up to 16 analog channels
- Programmable gain amplifier (PGA) allows measuring small voltages
- Max sample rate is 860 SPS
- ADS1015 is the cheaper 12-bit variant with same pinout and addresses

---

### INA219 (Current/Voltage/Power Monitor)

| Field | Value |
|-------|-------|
| Interface | I2C |
| Required Pins | SDA, SCL. Also: VIN+, VIN- (power sense pins, not GPIO) |
| I2C Address | 0x40 (A0=GND,A1=GND), 0x41, 0x44, 0x45 (via A0/A1 pins) |
| Voltage | 3.0–5.5V (bus voltage measurement up to 26V) |
| Pull-ups | Provided on most breakouts |

**RPi Notes:**
- I2C1 (GPIO2/3)

**ESP32 Notes:**
- Any GPIO pair for I2C

**Gotchas:**
- ADDRESS COLLISION — 0x40 collides with PCA9685 servo driver
- 0x44/0x45 collide with SHT31
- If using INA219 + PCA9685, must configure different addresses via A0/A1 pins
- Shunt resistor value (typically 0.1Ω) determines current measurement range — must match library calibration

---

## Category 4: Displays

### SSD1306 (OLED Display, 128×64 or 128×32)

| Field | Value |
|-------|-------|
| Interface | I2C (most common), SPI |
| Required Pins | I2C: SDA, SCL. SPI: MOSI, SCLK, CS, DC, RST (optional) |
| I2C Address | 0x3C (common), 0x3D (alternate, set by resistor) |
| Voltage | 3.3V–5V (most modules have regulator) |
| Pull-ups | Provided on most breakouts |

**RPi Notes:**
- I2C1 (GPIO2/3) for I2C mode
- SPI0 for SPI mode
- I2C mode fine for single display, SPI faster for animations

**ESP32 Notes:**
- Any GPIO pair for I2C, any pins for SPI
- SPI recommended for high refresh rate

**Gotchas:**
- 0x3C address collides with SH1106 OLED (similar but different driver)
- 128×32 variant usually has fixed 0x3C address
- Some boards labeled "SSD1306" actually use SH1106 — different memory layout (132×64 with offset)

---

### ST7789 (TFT Color Display, typically 240×240 or 240×320)

| Field | Value |
|-------|-------|
| Interface | SPI only |
| Required Pins | MOSI, SCLK, CS, DC, RST, BL (backlight) |
| I2C Address | N/A |
| Voltage | 3.3V logic. Most modules accept 3.3V–5V power |
| Pull-ups | N/A |

**RPi Notes:**
- SPI0 (GPIO10 MOSI, GPIO11 SCLK, GPIO8 CE0)
- DC on any GPIO (commonly GPIO25)
- RST on any GPIO (commonly GPIO27)
- BL on PWM-capable GPIO (GPIO12 or GPIO18) for brightness control, or tie to 3.3V

**ESP32 Notes:**
- VSPI or any pins via GPIO matrix
- SPI clock can run up to 80MHz — good for smooth display updates

**Gotchas:**
- SPI-only, no I2C option
- ST7735 is the smaller cousin (128×160) — different driver, similar interface
- Some displays omit CS pin (tied to GND) — prevents sharing SPI bus
- BL pin may be active-high or active-low depending on module

---

### HD44780 (Character LCD, 16×2 or 20×4, via PCF8574 I2C Backpack)

| Field | Value |
|-------|-------|
| Interface | Parallel GPIO (6+ pins), I2C via PCF8574 backpack (recommended) |
| Required Pins | I2C: SDA, SCL. Direct: RS, EN, D4-D7 (4-bit = 6 GPIOs) |
| I2C Address | 0x27 (most common), 0x3F (some variants). PCF8574A: 0x38–0x3F |
| Voltage | 5V for LCD. I2C backpack tolerates 3.3V I2C signals |
| Pull-ups | Provided on PCF8574 backpack |

**RPi Notes:**
- Use I2C backpack to save GPIOs
- I2C1 (GPIO2/3)
- RPi's 3.3V I2C usually works with 5V PCF8574 but is out of spec — consider level shifter

**ESP32 Notes:**
- I2C backpack strongly recommended
- ESP32 is 3.3V only — needs level shifter for direct 5V LCD parallel interface

**Gotchas:**
- PCF8574 address 0x27 collides with MCP23017 (0x20-0x27)
- Contrast potentiometer must be adjusted or display appears blank — #1 troubleshooting issue
- Backlight can be controlled via PCF8574 register bit

---

### e-Paper / e-Ink Display (Waveshare-style)

| Field | Value |
|-------|-------|
| Interface | SPI |
| Required Pins | MOSI, SCLK, CS, DC, RST, BUSY (output from display) |
| I2C Address | N/A |
| Voltage | 3.3V logic and power |
| Pull-ups | N/A |

**RPi Notes:**
- SPI0 (GPIO10 MOSI, GPIO11 SCLK, GPIO8 CE0)
- DC, RST, BUSY on any available GPIOs
- Waveshare HAT uses: RST=GPIO17, DC=GPIO25, CS=GPIO8, BUSY=GPIO24

**ESP32 Notes:**
- Any SPI pins via GPIO matrix
- BUSY must be on input-capable GPIO
- Input-only pins (GPIO34-39) are fine for BUSY

**Gotchas:**
- Refresh time is slow (1-15 seconds for full refresh)
- Partial refresh is faster but causes ghosting over time
- Do NOT refresh continuously — damages the panel
- Different Waveshare models (1.54", 2.13", 2.7", 4.2", 7.5") use different drivers — NOT interchangeable in software

---

## Category 5: Communication

### nRF24L01 (2.4GHz Radio Transceiver)

| Field | Value |
|-------|-------|
| Interface | SPI, GPIO |
| Required Pins | MOSI, MISO, SCLK, CSN (chip select), CE (chip enable). Optional: IRQ |
| I2C Address | N/A |
| Voltage | 3.3V ONLY for VCC. SPI data pins are 5V tolerant |
| Pull-ups | N/A |

**RPi Notes:**
- SPI0 (GPIO10 MOSI, GPIO9 MISO, GPIO11 SCLK, GPIO8 CE0 for CSN)
- CE on any GPIO (commonly GPIO17 or GPIO22)
- IRQ on any GPIO
- Add 10µF + 100nF capacitor across VCC/GND close to module — extremely sensitive to power supply noise

**ESP32 Notes:**
- VSPI or any SPI pins via GPIO matrix
- CE on any output GPIO
- Avoid strapping pins for CE
- Power supply decoupling equally important

**Gotchas:**
- PA+LNA variant draws up to 115mA — cannot be powered from typical breadboard 3.3V regulator
- Use dedicated 3.3V supply
- Range: basic module ~100m line of sight, PA+LNA ~1km+
- CSN is NOT the same as CE — CSN is SPI chip select, CE is nRF24-specific enable pin

---

### HC-05 / HC-06 (Bluetooth Classic Serial)

| Field | Value |
|-------|-------|
| Interface | UART |
| Required Pins | TX, RX. Optional: STATE/STATUS, EN/KEY (AT mode on HC-05) |
| I2C Address | N/A |
| Voltage | 3.3V logic (module VCC 3.6–6V). Use voltage divider for 5V TX |
| Pull-ups | N/A |

**RPi Notes:**
- GPIO14 (TX) / GPIO15 (RX)
- On Pi 3/4/Zero2W, use `dtoverlay=disable-bt` or `dtoverlay=miniuart-bt`
- Pi 5 has no conflict
- RPi TX → HC-05/06 RX, RPi RX → HC-05/06 TX (crossover)

**ESP32 Notes:**
- UART1 or UART2 on any GPIO pair
- Less commonly used since ESP32 has built-in Bluetooth
- Useful for bridging to Bluetooth Classic devices

**Gotchas:**
- HC-05 is master/slave capable, HC-06 is slave only
- Default baud rate: 9600 (HC-06), 38400 (HC-05 AT mode)
- HC-05 enters AT mode by holding EN/KEY HIGH during power-up
- Bluetooth Classic (not BLE) — not compatible with iOS without MFi

---

### SX1276 / RFM95W (LoRa Radio)

| Field | Value |
|-------|-------|
| Interface | SPI, GPIO |
| Required Pins | MOSI, MISO, SCLK, NSS/CS, RST, DIO0 (interrupt). Optional: DIO1-DIO5 |
| I2C Address | N/A |
| Voltage | 3.3V |
| Pull-ups | N/A |

**RPi Notes:**
- SPI0 (GPIO10 MOSI, GPIO9 MISO, GPIO11 SCLK, GPIO8 CE0)
- RST on any GPIO
- DIO0 on any GPIO (commonly GPIO25)
- NSS can use GPIO8 (CE0) or any GPIO as software CS

**ESP32 Notes:**
- Any SPI pins via GPIO matrix
- DIO0 to any input-capable GPIO
- Input-only pins (GPIO34-39) are fine for DIO0

**Gotchas:**
- LoRa operates in ISM bands (433/868/915 MHz) — use correct frequency for your country
- Antenna MUST be connected before transmitting or module can be damaged
- RFM95W is the HopeRF clone of SX1276 — pin-compatible and functionally identical

---

### MCP2515 (CAN Bus Controller)

| Field | Value |
|-------|-------|
| Interface | SPI, GPIO |
| Required Pins | MOSI, MISO, SCLK, CS, INT. Needs external CAN transceiver (TJA1050/MCP2551) |
| I2C Address | N/A |
| Voltage | 2.7–5.5V. Most modules include transceiver and run at 5V |
| Pull-ups | N/A |

**RPi Notes:**
- SPI0 with `dtoverlay=mcp2515-can0,oscillator=8000000,interrupt=25`
- Adjust oscillator to match board crystal (8MHz or 16MHz)
- INT on GPIO (commonly GPIO25)
- Adds can0 network interface
- Second bus: `dtoverlay=mcp2515-can1,oscillator=8000000,interrupt=24` on SPI0 CE1

**ESP32 Notes:**
- Any SPI pins
- INT to any input-capable GPIO
- ESP32 has built-in CAN (TWAI) — MCP2515 useful for second CAN bus
- Still needs external transceiver

**Gotchas:**
- Oscillator frequency in overlay MUST match physical crystal — check markings (8MHz or 16MHz common)
- Wrong oscillator value = CAN bus won't work
- CAN bus requires 120Ω termination resistors at both ends
- Some modules include termination resistor — check before adding another

---

### ENC28J60 (SPI Ethernet Controller)

| Field | Value |
|-------|-------|
| Interface | SPI, GPIO |
| Required Pins | MOSI, MISO, SCLK, CS, INT. Optional: RST (can tie to VCC) |
| I2C Address | N/A |
| Voltage | 3.3V |
| Pull-ups | N/A |

**RPi Notes:**
- SPI0 with `dtoverlay=enc28j60,int_pin=25,speed=12000000`
- INT on GPIO (commonly GPIO25)
- Adds eth1 network interface
- Max speed is 10 Mbps (10Base-T only)

**ESP32 Notes:**
- Any SPI pins via GPIO matrix
- Less commonly used since many ESP32 boards have built-in WiFi
- Useful for wired-only industrial applications

**Gotchas:**
- 10 Mbps only — no 100 Mbps or gigabit
- Uses significant CPU time due to SPI polling
- Not suitable for high-bandwidth applications
- MAC address usually set in software
- Consider W5500 (100 Mbps, hardware TCP/IP stack) as modern alternative

---

## Category 6: Motor Control

### PCA9685 (16-Channel PWM/Servo Driver)

| Field | Value |
|-------|-------|
| Interface | I2C |
| Required Pins | SDA, SCL. Optional: OE (output enable, active low) |
| I2C Address | 0x40 (default). Configurable 0x40–0x7F via A0–A5 pins (62 addresses) |
| Voltage | VCC 3.3V–5V for logic. Separate V+ for servos (up to 6V) |
| Pull-ups | Provided on most breakouts |

**RPi Notes:**
- I2C1 (GPIO2/3)
- No special overlay needed
- 12-bit resolution, 24Hz–1526Hz adjustable frequency
- OE pin to any GPIO if needed

**ESP32 Notes:**
- Any GPIO pair for I2C
- Useful for adding PWM channels or servo control

**Gotchas:**
- ADDRESS COLLISION — default 0x40 collides with INA219
- Must change address via A0-A5 pins if both on same bus
- PCA9685 is a PWM driver, NOT a motor driver — generates PWM signals but cannot drive motors directly
- Connect outputs to motor drivers (L298N, DRV8825) or directly to servos
- All 16 channels share same PWM frequency

---

### L298N (Dual H-Bridge Motor Driver)

| Field | Value |
|-------|-------|
| Interface | GPIO (digital + PWM) |
| Required Pins | Per motor: IN1, IN2 (direction), ENA (PWM speed). 2 motors: 6 GPIOs total |
| I2C Address | N/A |
| Voltage | Motor: 5–35V. Logic: 5V (on-board regulator if motor >7V). 3.3V tolerant |
| Pull-ups | N/A |

**RPi Notes:**
- IN1-IN4 on any GPIO
- ENA/ENB on PWM-capable GPIO (GPIO12, GPIO13, GPIO18, GPIO19)
- Use `dtoverlay=pwm` or `dtoverlay=pwm-2chan` for smooth control
- Software PWM may cause audible motor whine

**ESP32 Notes:**
- IN1-IN4 on any output GPIO
- ENA/ENB on any GPIO (all support LEDC PWM)
- Avoid strapping pins

**Gotchas:**
- L298N has ~2V voltage drop across H-bridge — motor sees ~2V less than supply
- Not efficient for battery-powered projects
- 5V regulator on board can supply ~500mA for logic
- TB6612FNG is a more efficient alternative (MOSFET-based, ~0.5V drop)

---

### DRV8825 / A4988 (Stepper Motor Driver)

| Field | Value |
|-------|-------|
| Interface | GPIO (digital) |
| Required Pins | STEP, DIR, ENABLE (active low, optional). MS1-MS3 (microstepping) |
| I2C Address | N/A |
| Voltage | DRV8825: 8.2–45V motor, 3.3V logic. A4988: 8–35V motor, 3.3V/5V logic |
| Pull-ups | ENABLE has internal pull-down. STEP/DIR do not need pull-ups |

**RPi Notes:**
- STEP on any GPIO (consider hardware PWM on GPIO12/13/18/19 for constant speed)
- DIR on any GPIO
- ENABLE on any GPIO

**ESP32 Notes:**
- STEP on any output GPIO
- Use LEDC PWM or RMT peripheral for consistent step timing
- Avoid strapping pins

**Gotchas:**
- DRV8825: up to 1/32 microstepping. A4988: up to 1/16
- Current limit must be set via potentiometer BEFORE connecting motor
- NEVER disconnect motor while driver is powered — back-EMF can destroy driver
- A4988 and DRV8825 have same pinout but different microstepping truth tables

---

## Category 7: Other

### MCP23017 (16-Channel GPIO Expander)

| Field | Value |
|-------|-------|
| Interface | I2C |
| Required Pins | SDA, SCL. Optional: INTA, INTB (interrupts), RST (active-low reset) |
| I2C Address | 0x20–0x27 (A0, A1, A2 = 8 configurable addresses) |
| Voltage | 1.8–5.5V |
| Pull-ups | Provided on most breakouts. GPIO pins have optional internal pull-ups |

**RPi Notes:**
- I2C1 (GPIO2/3)
- Connect INTA/INTB to RPi GPIOs for interrupt-driven input
- Up to 8 expanders = 128 additional GPIOs

**ESP32 Notes:**
- Any GPIO pair for I2C
- Less commonly needed since ESP32 has many GPIOs
- Useful for projects needing many inputs with interrupt support

**Gotchas:**
- Address range 0x20–0x27 collides with PCF8574 (HD44780 I2C backpacks)
- If using both, ensure non-overlapping addresses
- Each output pin can source/sink 25mA (total per port: 125mA)
- PCF8574 is the 8-channel version with same address range

---

### WS2812B / NeoPixels (Addressable RGB LEDs)

| Field | Value |
|-------|-------|
| Interface | Single GPIO (proprietary protocol — NOT Dallas 1-Wire) |
| Required Pins | DIN (1 GPIO). Chain DOUT→DIN between LEDs |
| I2C Address | N/A |
| Voltage | 5V power. Data expects 5V logic (min 3.5V for logic high) |
| Pull-ups | N/A. Add 300–500Ω resistor on data line. Add 1000µF capacitor at power |

**RPi Notes:**
- USE GPIO18 (PWM0/PCM) or GPIO10 (SPI0 MOSI) or GPIO21 (PCM)
- rpi_ws281x library requires specific GPIOs (DMA-based timing)
- GPIO18 most common. GPIO10 requires `dtparam=spi=on`
- Needs sudo or specific permissions for DMA access
- RPi outputs 3.3V — use level shifter (3.3V→5V) for reliable operation

**ESP32 Notes:**
- Any output GPIO via RMT peripheral
- ESP32 outputs 3.3V — same level shifting recommendation
- RMT peripheral handles timing precisely without CPU intervention

**Gotchas:**
- LEVEL SHIFTER WARNING — 3.3V logic is below WS2812B spec (needs >3.5V)
- Works unreliably at short distances, fails with longer data lines
- Use 74AHCT125, 74HCT245, or SN74LV1T34 level shifter
- Each LED draws up to 60mA at full white — 60 LEDs = 3.6A
- Inject power every 30-50 LEDs for long strips
- SK6812 is compatible alternative with RGBW support

---

### Rotary Encoder (Incremental, with push button)

| Field | Value |
|-------|-------|
| Interface | GPIO (digital inputs) |
| Required Pins | CLK/A, DT/B (encoder phases), SW (push button, active low) |
| I2C Address | N/A |
| Voltage | 3.3V–5V (module dependent) |
| Pull-ups | Most modules have 10kΩ on board. Bare encoder needs pull-ups |

**RPi Notes:**
- Use any 3 GPIOs with interrupt support (all RPi GPIOs support edge-triggered interrupts)
- Enable internal pull-ups via gpiod or RPi.GPIO if bare encoder

**ESP32 Notes:**
- Any 3 input-capable GPIOs
- Hardware interrupt on all GPIOs
- Input-only pins (GPIO34-39) work fine
- Avoid strapping pins

**Gotchas:**
- Mechanical encoders generate switch bounce — need debouncing
- Hardware: 100nF capacitor across each pin to GND
- Software: 1-5ms delay
- Optical encoders don't bounce
- Reading must use interrupt or high-frequency polling — slow polling misses steps
- KY-040 is the most common breakout module

---

### Relay Modules (1/2/4/8 channel)

| Field | Value |
|-------|-------|
| Interface | GPIO (digital output) |
| Required Pins | IN1 (one GPIO per channel). Multi-channel: IN1, IN2, IN3, IN4, etc. |
| I2C Address | N/A |
| Voltage | Module VCC typically 5V. Most blue relay modules are ACTIVE LOW |
| Pull-ups | N/A |

**RPi Notes:**
- Any GPIO
- Check if module is active-low or active-high
- Most cheap modules are active-low — relay activates when GPIO goes LOW
- At boot, GPIOs are floating — may cause relay chatter
- Use pull-up resistor to keep relay OFF during boot

**ESP32 Notes:**
- Any output GPIO
- Same active-low/high consideration
- Avoid strapping pins (GPIO0, 2, 5, 12, 15) — boot states could trigger relays
- Avoid GPIO34-39 (input only)

**Gotchas:**
- Most modules have opto-isolated inputs
- Remove VCC-JD jumper for true isolation
- Relay coils cause voltage spikes — modules should have flyback diodes (most do)
- Never switch mains voltage without proper safety knowledge
- For DC loads, consider MOSFETs instead

---

### MAX31855 / MAX6675 (Thermocouple Interface)

| Field | Value |
|-------|-------|
| Interface | SPI (read-only — no MOSI needed) |
| Required Pins | MISO/SO, SCLK/SCK, CS. No MOSI needed |
| I2C Address | N/A |
| Voltage | MAX31855: 3.0–3.6V. MAX6675: 5V. Most breakouts accept both |
| Pull-ups | N/A |

**RPi Notes:**
- SPI0 (GPIO9 MISO, GPIO11 SCLK, GPIO8 CE0)
- MOSI (GPIO10) unused but SPI0 still claims it

**ESP32 Notes:**
- Any SPI pins via GPIO matrix
- MOSI unused — can reassign to other functions

**Gotchas:**
- MAX31855 supports K, J, N, S, R, T type thermocouples — different variants for each
- MAX6675 supports K-type only and is older/less accurate
- Thermocouple wires are polarity-sensitive — red is always negative
- Cold-junction compensation is built in
- Keep chip close to terminal block to minimize cold-junction error
- MAX31855 has open/short fault detection

---

## I2C Address Collision Map

| Address | Devices | Resolution |
|---------|---------|------------|
| 0x76 | BME280, BMP280, MS5611 | Use alternate 0x77 for one device, or I2C multiplexer |
| 0x77 | BME280 (alt), BMP280 (alt) | Only one per bus at this address |
| 0x40 | INA219, PCA9685, HDC1080 | Change address via A0/A1 pins |
| 0x44 | SHT31, INA219 (A1=VDD,A0=GND) | Change INA219 address via A0/A1 |
| 0x45 | SHT31 (alt), INA219 (alt) | Change INA219 address via A0/A1 |
| 0x3C | SSD1306, SH1106 | Different drivers, unlikely to use both |
| 0x3D | SSD1306 (alt) | Rare collision |
| 0x68 | MPU6050, DS3231 RTC | Set MPU6050 to 0x69 (AD0→VDD) |
| 0x69 | MPU6050 (alt) | Rare collision |
| 0x48–0x4B | ADS1115 (configurable), TMP102 | Configure ADS1115 to non-conflicting address |
| 0x20–0x27 | MCP23017 (A0-A2), PCF8574, HD44780 backpack | Configure A0-A2 to avoid overlap |
| 0x27 | PCF8574 (HD44780 backpack), MCP23017 (A0-A2 all high) | Change MCP23017 address |
| 0x42 | NEO-GPS (I2C mode) | Uncommon, rarely conflicts |

When address conflicts cannot be resolved via address pins, use a TCA9548A I2C multiplexer. Each TCA9548A has 8 channels and its own configurable address (0x70-0x77), supporting up to 64 separate I2C buses from a single host bus.
