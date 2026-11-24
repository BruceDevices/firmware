# Protocol Quick Reference

## Table of Contents

- [I2C (Inter-Integrated Circuit)](#i2c-inter-integrated-circuit)
  - [Overview](#overview)
  - [Signal Lines](#signal-lines)
  - [Speed Modes](#speed-modes)
  - [Pull-up Requirements](#pull-up-requirements)
  - [Common I2C Addresses (Collision Detection)](#common-i2c-addresses-collision-detection)
  - [Platform Notes](#platform-notes)
- [SPI (Serial Peripheral Interface)](#spi-serial-peripheral-interface)
  - [Overview](#overview)
  - [Signal Lines](#signal-lines)
  - [Speed](#speed)
  - [Clock Modes](#clock-modes)
  - [Pull-up Requirements](#pull-up-requirements)
  - [Platform Notes](#platform-notes)
- [UART (Universal Asynchronous Receiver/Transmitter)](#uart-universal-asynchronous-receivertransmitter)
  - [Overview](#overview)
  - [Signal Lines](#signal-lines)
  - [Common Baud Rates](#common-baud-rates)
  - [Frame Format](#frame-format)
  - [Voltage Levels](#voltage-levels)
  - [Level Shifting](#level-shifting)
  - [Platform Notes](#platform-notes)
- [PWM (Pulse Width Modulation)](#pwm-pulse-width-modulation)
  - [Overview](#overview)
  - [Key Parameters](#key-parameters)
  - [Frequency by Application](#frequency-by-application)
  - [Servo Control Specifics](#servo-control-specifics)
  - [Platform Notes](#platform-notes)
- [1-Wire](#1-wire)
  - [Overview](#overview)
  - [Signal Line](#signal-line)
  - [Pull-up Requirement](#pull-up-requirement)
  - [Common 1-Wire Devices](#common-1-wire-devices)
  - [Platform Notes](#platform-notes)
- [CAN (Controller Area Network)](#can-controller-area-network)
  - [Overview](#overview)
  - [Signal Lines](#signal-lines)
  - [Common Transceivers](#common-transceivers)
  - [Speed and Termination](#speed-and-termination)
  - [Platform Notes](#platform-notes)
- [ADC (Analog-to-Digital Converter)](#adc-analog-to-digital-converter)
  - [Overview](#overview)
  - [Key Parameters](#key-parameters)
  - [Platform Comparison](#platform-comparison)
  - [ESP32 ADC Attenuation](#esp32-adc-attenuation)
  - [External ADC Options](#external-adc-options)
  - [Input Protection](#input-protection)
  - [Platform Notes](#platform-notes)

---

## I2C (Inter-Integrated Circuit)

### Overview

- Two-wire synchronous serial bus
- Multi-device: multiple slaves on same bus
- Addressable: 7-bit (128) or 10-bit (1024) addresses
- Open-drain: requires external pull-up resistors
- Half-duplex: bidirectional on single data line

### Signal Lines

| Signal | Direction | Type | Description |
|--------|-----------|------|-------------|
| SDA | Bidirectional | Open-drain | Serial data |
| SCL | Master→Slave | Open-drain | Serial clock |

### Speed Modes

| Mode | Speed | Notes |
|------|-------|-------|
| Standard | 100 kHz | Universal compatibility |
| Fast | 400 kHz | Most common for sensors |
| Fast Plus | 1 MHz | Requires stronger pull-ups |
| High Speed | 3.4 MHz | Rarely used in hobby projects |

### Pull-up Requirements

**REQUIRED** on both SDA and SCL lines.

| Speed Mode | Recommended Value | Notes |
|------------|-------------------|-------|
| Standard/Fast | 4.7kΩ | Most common choice |
| Fast Plus | 2.2kΩ | Stronger pull needed |
| High Speed / Long wires | 1kΩ | Compensates for capacitance |

**Calculation Formula:**
```
R = (VCC - VOL) / IOL
R = (3.3V - 0.4V) / 3mA = 967Ω minimum
```

**Consequences of Wrong Value:**
- **Too high (>10kΩ):** Slow rise times, communication errors, fails at higher speeds
- **Too low (<1kΩ):** Excessive current draw, devices cannot pull line LOW, bus contention

### Common I2C Addresses (Collision Detection)

| Address | Device(s) |
|---------|-----------|
| 0x20-0x27 | MCP23017 GPIO expander, PCF8574 |
| 0x27, 0x3F | PCF8574 LCD backpack |
| 0x29 | VL53L0X ToF distance sensor |
| 0x39 | APDS9960 gesture/color sensor |
| 0x3C, 0x3D | SSD1306 OLED display |
| 0x40 | INA219 current sensor, PCA9685 PWM |
| 0x48-0x4B | ADS1115/ADS1015 ADC |
| 0x50-0x57 | AT24C EEPROM |
| 0x5A | MLX90614 IR thermometer |
| 0x60 | Si5351 clock generator |
| 0x68 | DS3231 RTC, MPU6050 IMU |
| 0x76, 0x77 | BME280/BMP280 sensor |

**Note:** Many devices have address pins (A0, A1, A2) to resolve conflicts.

### Platform Notes

**Raspberry Pi:**
- I2C1 (GPIO2/3) is the primary user-accessible bus
- I2C0 (GPIO0/1) is **reserved** for HAT EEPROM detection — do not use
- Enable with `dtparam=i2c_arm=on` in config.txt
- Pi 4/5 have additional I2C buses via dtoverlay

**ESP32:**
- Any GPIO pair works via GPIO matrix — no fixed pins
- Convention: GPIO21 (SDA), GPIO22 (SCL)
- Do NOT use input-only pins (GPIO34-39) — they cannot drive SDA

---

## SPI (Serial Peripheral Interface)

### Overview

- Four-wire synchronous serial bus
- Full-duplex: simultaneous send and receive
- One chip select (CS) per slave device
- No addressing: CS line selects device
- Push-pull drivers: no pull-ups required on data/clock

### Signal Lines

| Signal | Direction | Description |
|--------|-----------|-------------|
| MOSI | Master→Slave | Master Out, Slave In |
| MISO | Slave→Master | Master In, Slave Out |
| SCLK | Master→Slave | Serial clock |
| CS/SS | Master→Slave | Chip Select (active LOW) |

### Speed

- Typical: 1-40 MHz (device dependent)
- Check slave device datasheet for maximum
- Longer wires = lower reliable speed

### Clock Modes

| Mode | CPOL | CPHA | Clock Idle | Data Sampled On |
|------|------|------|------------|-----------------|
| 0 | 0 | 0 | LOW | Rising edge |
| 1 | 0 | 1 | LOW | Falling edge |
| 2 | 1 | 0 | HIGH | Falling edge |
| 3 | 1 | 1 | HIGH | Rising edge |

**Note:** Mode 0 is most common. Check device datasheet.

### Pull-up Requirements

- **MOSI, MISO, SCLK:** Generally NOT required (push-pull drivers)
- **CS lines:** 10kΩ pull-up recommended to prevent floating during boot/reset

### Platform Notes

**Raspberry Pi:**
- SPI0 (GPIO7-11) is primary bus — CE0 (GPIO8), CE1 (GPIO7)
- SPI1 (GPIO16-21) available but **conflicts with PCM/I2S audio**
- Enable with `dtparam=spi=on` in config.txt

**ESP32:**
- VSPI (SPI3): GPIO23 (MOSI), GPIO19 (MISO), GPIO18 (SCLK), GPIO5 (CS) — **recommended**
- HSPI (SPI2): GPIO13 (MOSI), GPIO12 (MISO), GPIO14 (SCLK), GPIO15 (CS)
- **WARNING:** HSPI pins overlap strapping pins! GPIO12 can brick the module if HIGH at boot.
- Any GPIO can be used via GPIO matrix (except input-only pins for outputs)

---

## UART (Universal Asynchronous Receiver/Transmitter)

### Overview

- Two-wire asynchronous serial communication
- Point-to-point: one transmitter, one receiver per pair
- No clock line: baud rate must match on both ends
- Simple: widely supported, easy to debug

### Signal Lines

| Signal | Direction | Description |
|--------|-----------|-------------|
| TX | Output | Transmit data (connect to peer's RX) |
| RX | Input | Receive data (connect to peer's TX) |
| RTS | Output | Request to Send (optional flow control) |
| CTS | Input | Clear to Send (optional flow control) |

**Critical:** TX connects to RX, RX connects to TX (crossover).

### Common Baud Rates

| Baud Rate | Use Case |
|-----------|----------|
| 9600 | Legacy devices, GPS modules |
| 19200 | Some sensors |
| 38400 | Bluetooth modules |
| 57600 | Faster sensors |
| 115200 | Most common default |
| 230400 | High-speed peripherals |
| 460800 | ESP32 flash programming |
| 921600 | Fast data transfer |

### Frame Format

Standard: **8N1** (8 data bits, No parity, 1 stop bit)

Other formats exist (7E1, 8E1, etc.) but 8N1 covers 95%+ of use cases.

### Voltage Levels

| Standard | Voltage | Common Devices |
|----------|---------|----------------|
| TTL 3.3V | 0V / 3.3V | RPi, ESP32, modern MCUs |
| TTL 5V | 0V / 5V | Arduino, many modules |
| RS-232 | ±12V | PC serial ports |

**CRITICAL:** RS-232 levels (±12V) will **DESTROY** 3.3V GPIO instantly. Use MAX232 or similar transceiver.

### Level Shifting

- 5V TX → 3.3V RX: Use voltage divider (1kΩ + 2kΩ) or level shifter
- 3.3V TX → 5V RX: Often works directly (check VIH threshold)
- Always use bidirectional level shifter for RTS/CTS

### Platform Notes

**Raspberry Pi:**
- UART0 (GPIO14/15) is primary serial port
- **Conflicts with Bluetooth** on Pi 3/4/Zero2W — use `dtoverlay=disable-bt` or `dtoverlay=miniuart-bt`
- Pi 4/5 have additional UARTs via dtoverlay

**ESP32:**
- UART0 (GPIO1/3) is **USB serial debug** — avoid for peripherals
- UART1 and UART2 are freely available
- Any GPIO can be assigned via GPIO matrix

---

## PWM (Pulse Width Modulation)

### Overview

- Digital approximation of analog voltage
- Square wave at fixed frequency
- Duty cycle controls average voltage
- Used for: LED dimming, motor speed, servo position, audio

### Key Parameters

| Parameter | Description | Typical Range |
|-----------|-------------|---------------|
| Frequency | Pulses per second | 50 Hz - 100 kHz |
| Duty Cycle | HIGH time percentage | 0-100% |
| Resolution | Steps of duty control | 8-bit (256) to 16-bit (65536) |

**Average Voltage:** Vavg = VCC × (Duty Cycle / 100)

### Frequency by Application

| Application | Frequency | Reason |
|-------------|-----------|--------|
| LED dimming | 500-5000 Hz | >500Hz avoids visible flicker |
| Servo control | 50 Hz | Standard RC servo protocol (20ms period) |
| Motor control | 1-20 kHz | Higher = less audible whine |
| Audio generation | 20-100 kHz | Above audible range |
| Switching PSU | 50-500 kHz | Efficiency vs. noise tradeoff |

### Servo Control Specifics

- Period: 20ms (50 Hz)
- Pulse width: 1ms (0°) to 2ms (180°)
- Neutral: 1.5ms (90°)
- Duty cycle: 5% (1ms) to 10% (2ms) at 50Hz

### Platform Notes

**Raspberry Pi:**
- **2 hardware PWM channels** only
- PWM0: GPIO12 (preferred) or GPIO18 (conflicts with audio)
- PWM1: GPIO13 (preferred) or GPIO19 (conflicts with audio)
- Software PWM available on any pin but less precise (jitter)
- Enable with `dtoverlay=pwm` or `dtoverlay=pwm-2chan`

**ESP32:**
- **LEDC peripheral:** 16 channels of hardware PWM
- Can output on **any output-capable GPIO**
- Cannot use input-only pins (GPIO34-39)
- Configurable resolution (1-16 bit) and frequency
- Motor Control PWM (MCPWM) for advanced motor control

---

## 1-Wire

### Overview

- Single-wire bidirectional bus
- Parasitic power option (power over data line)
- Each device has unique 64-bit ROM ID
- Multiple devices on same bus (addressed by ROM)
- Open-drain: requires pull-up resistor

### Signal Line

| Signal | Type | Description |
|--------|------|-------------|
| DQ | Bidirectional, Open-drain | Data and (optionally) power |

### Pull-up Requirement

**REQUIRED:** 4.7kΩ to VCC (3.3V or 5V depending on devices)

- Stronger pull-up (2.2kΩ-1kΩ) for long cables or many devices
- Parasitic power mode may need stronger pull-up during temperature conversion

### Common 1-Wire Devices

| Device | Function | Notes |
|--------|----------|-------|
| DS18B20 | Temperature sensor | Most popular 1-Wire device |
| DS18S20 | Temperature sensor | Older, 9-bit only |
| DS2401 | Serial number | Silicon serial number |
| DS2413 | GPIO | 2-channel I/O |
| iButton | Various | Key fobs, access control |

### Platform Notes

**Raspberry Pi:**
- Default pin: GPIO4
- Enable with `dtoverlay=w1-gpio`
- Change pin with `dtoverlay=w1-gpio,gpiopin=N`
- Kernel driver handles protocol automatically

**ESP32:**
- Any GPIO can be used via OneWire library
- GPIO4 is common convention
- Requires software library (no hardware peripheral)

---

## CAN (Controller Area Network)

### Overview

- Differential two-wire bus (noise immune)
- Multi-master: any node can initiate
- Message-based: no addresses, messages have IDs
- Priority: lower message ID = higher priority
- Error detection: CRC, ACK, bit stuffing
- Common in: automotive, industrial, robotics

### Signal Lines

| Signal | Description |
|--------|-------------|
| CAN_H | CAN High (dominant = 3.5V) |
| CAN_L | CAN Low (dominant = 1.5V) |

**Note:** Requires transceiver chip (GPIO cannot drive CAN directly)

### Common Transceivers

| Chip | Voltage | Notes |
|------|---------|-------|
| MCP2551 | 5V | Classic, widely available |
| SN65HVD230 | 3.3V | Good for ESP32/RPi |
| TJA1050 | 5V | Automotive grade |

### Speed and Termination

| Speed | Max Bus Length | Use Case |
|-------|----------------|----------|
| 125 kbps | 500m | Long distance |
| 250 kbps | 250m | General purpose |
| 500 kbps | 100m | Automotive |
| 1 Mbps | 40m | High speed |

**Termination:** 120Ω resistor at **each end** of bus (two total). Many transceiver modules have onboard termination jumper.

### Platform Notes

**Raspberry Pi:**
- No built-in CAN controller
- Requires external MCP2515 (SPI-to-CAN) + transceiver
- Enable with `dtoverlay=mcp2515-can0,oscillator=8000000,interrupt=25`
- Uses SocketCAN interface

**ESP32:**
- Built-in TWAI controller (CAN 2.0B compatible)
- Only needs external transceiver (e.g., SN65HVD230)
- Common pins: GPIO4 (TX), GPIO5 (RX) — but any GPIO works
- ESP-IDF and Arduino libraries available

---

## ADC (Analog-to-Digital Converter)

### Overview

- Converts continuous analog voltage to discrete digital value
- Key parameters: resolution, reference voltage, sample rate
- Input must not exceed reference voltage

### Key Parameters

| Parameter | Description |
|-----------|-------------|
| Resolution | Bits of precision (10-bit = 1024 steps, 12-bit = 4096) |
| Reference | Full-scale input voltage (typically VCC or internal ref) |
| Sample Rate | Conversions per second (SPS) |
| Input Range | Allowable input voltage (0 to Vref typically) |

### Platform Comparison

| Parameter | Raspberry Pi | ESP32 |
|-----------|--------------|-------|
| Built-in ADC | **No** | Yes (2 ADCs) |
| Resolution | N/A | 12-bit (4096 levels) |
| Channels | N/A | ADC1: 8ch, ADC2: 10ch |
| Reference | N/A | 0-3.3V (with attenuation) |
| Sample Rate | N/A | Up to 2 MSPS |
| WiFi Conflict | N/A | **ADC2 unusable with WiFi** |

### ESP32 ADC Attenuation

| Attenuation | Input Range | Notes |
|-------------|-------------|-------|
| 0 dB | 0-1.1V | Highest accuracy |
| 2.5 dB | 0-1.5V | |
| 6 dB | 0-2.2V | |
| 11 dB | 0-3.3V | Full range, lower accuracy |

### External ADC Options

| Chip | Interface | Resolution | Channels | Notes |
|------|-----------|------------|----------|-------|
| ADS1115 | I2C | 16-bit | 4 | Programmable gain, slow (860 SPS) |
| ADS1015 | I2C | 12-bit | 4 | Faster than ADS1115 (3300 SPS) |
| MCP3008 | SPI | 10-bit | 8 | Simple, cheap, fast |
| MCP3208 | SPI | 12-bit | 8 | Higher resolution MCP3008 |
| ADS7828 | I2C | 12-bit | 8 | 8-channel I2C option |

### Input Protection

- **Never exceed reference voltage** — will damage ADC or give invalid readings
- Use voltage divider for higher voltages
- Add clamp diodes (Schottky to VCC and GND) for unknown inputs
- Add RC filter (100Ω + 100nF) to reduce noise

### Platform Notes

**Raspberry Pi:**
- No built-in ADC — external ADC required for any analog input
- MCP3008 (SPI) or ADS1115 (I2C) are most common choices
- Many HATs include ADC chips

**ESP32:**
- ADC1 (GPIO32-39): **Always available**, even with WiFi active
- ADC2 (GPIO0-27 subset): **Unusable when WiFi or Bluetooth active**
- Design rule: Use ADC1 pins for analog if project uses WiFi
- Non-linear at extremes — calibration improves accuracy

---
