# Electrical Constraints Reference

## Table of Contents

- [Platform Comparison Summary](#platform-comparison-summary)
- [Raspberry Pi Electrical Details](#raspberry-pi-electrical-details)
  - [Voltage Levels](#voltage-levels)
  - [Current Limits](#current-limits)
  - [Power Rails](#power-rails)
  - [Internal Pull Resistors](#internal-pull-resistors)
  - [Safe Driving Patterns](#safe-driving-patterns)
- [ESP32 Electrical Details](#esp32-electrical-details)
  - [Voltage Levels](#voltage-levels)
  - [Current Limits](#current-limits)
  - [Power Consumption](#power-consumption)
  - [Internal Pull Resistors](#internal-pull-resistors)
  - [Drive Strength Configuration](#drive-strength-configuration)
- [Pull-up and Pull-down Resistors](#pull-up-and-pull-down-resistors)
  - [When Required](#when-required)
  - [Calculation Formula](#calculation-formula)
  - [Strength Guidelines](#strength-guidelines)
  - [Consequences of Wrong Value](#consequences-of-wrong-value)
- [Level Shifting](#level-shifting)
  - [When Required](#when-required)
  - [Method 1: Voltage Divider (5V → 3.3V, Unidirectional)](#method-1-voltage-divider-5v--33v-unidirectional)
  - [Method 2: N-Channel MOSFET (Bidirectional)](#method-2-n-channel-mosfet-bidirectional)
  - [Method 3: Dedicated Level Shifter ICs](#method-3-dedicated-level-shifter-ics)
  - [Method 4: Direct Connection (3.3V → 5V Input)](#method-4-direct-connection-33v--5v-input)
- [Common Mistakes and Warnings](#common-mistakes-and-warnings)
  - [NEVER Do This](#never-do-this)
  - [ALWAYS Do This](#always-do-this)
- [Quick Reference Card](#quick-reference-card)
  - [Formulas](#formulas)
  - [Quick Values](#quick-values)
  - [Current Limits Summary](#current-limits-summary)
  - [Voltage Summary](#voltage-summary)
  - [Common Pin Restrictions](#common-pin-restrictions)

---

## Platform Comparison Summary

| Parameter | Raspberry Pi | ESP32 |
|-----------|--------------|-------|
| Logic voltage | 3.3V | 3.3V |
| Max per-pin current | 16mA source/sink | 40mA max (20mA recommended) |
| Aggregate GPIO current | **50mA total** | ~1200mA total (chip limit) |
| 5V tolerant | **NO** | **NO** |
| Internal pull-up | ~50kΩ | ~45kΩ typical |
| Internal pull-down | ~50kΩ | ~45kΩ typical |
| Drive strength | Fixed | Configurable (5-40mA) |
| Input threshold (VIH) | ~1.8V | ~2.0V |
| Input threshold (VIL) | ~0.8V | ~0.8V |

---

## Raspberry Pi Electrical Details

### Voltage Levels

- **All GPIO pins operate at 3.3V ONLY**
- **5V on any GPIO pin WILL PERMANENTLY DAMAGE the SoC**
- No built-in overvoltage protection
- No built-in ESD protection (handle with care)

### Current Limits

| Limit Type | Value | Consequence of Exceeding |
|------------|-------|--------------------------|
| Per-pin source | 16mA | Voltage droop, pin damage |
| Per-pin sink | 16mA | Voltage rise, pin damage |
| **Total GPIO** | **50mA** | Instability, crashes, permanent damage |

**Critical:** The 50mA limit is across ALL GPIO pins combined, not per-bank.

### Power Rails

| Rail | Source | Available Current | Notes |
|------|--------|-------------------|-------|
| 3.3V | Onboard regulator | ~50mA for peripherals | Shared with Pi's 3.3V needs |
| 5V | USB/PSU direct | 1-2A minus Pi consumption | No regulation, direct pass-through |
| GND | Common ground | N/A | 8 ground pins on header |

### Internal Pull Resistors

| GPIO Range | Default State | Resistance |
|------------|---------------|------------|
| GPIO0-8 | Pull-UP | ~50kΩ |
| GPIO9-27 | Pull-DOWN | ~50kΩ |

**Note:** Internal pulls are too weak for I2C (need 4.7kΩ external).

### Safe Driving Patterns

| Load | Method | Notes |
|------|--------|-------|
| Single LED | 330Ω-1kΩ series resistor | 3-10mA safe |
| Multiple LEDs | Transistor driver | If total >50mA |
| Relay | Transistor/MOSFET + flyback diode | Never direct from GPIO |
| Motor | Motor driver IC (L298N, DRV8833) | Never direct from GPIO |
| Buzzer (passive) | Transistor driver | Inductive load |
| Buzzer (active) | Direct if <16mA | Check current draw |

---

## ESP32 Electrical Details

### Voltage Levels

- **All GPIO pins operate at 3.3V ONLY**
- **5V on any GPIO pin WILL DAMAGE the chip**
- No built-in overvoltage protection
- Some ESD protection but don't rely on it

### Current Limits

| Limit Type | Value | Notes |
|------------|-------|-------|
| Per-pin max | 40mA | Absolute maximum |
| Per-pin recommended | 20mA | For reliability/longevity |
| Total chip | ~1200mA | Includes WiFi, BT, CPU |

### Power Consumption

| Mode | Current Draw | Notes |
|------|--------------|-------|
| Active + WiFi TX | 80-240mA | Peaks during transmission |
| Active + WiFi idle | 20-68mA | Connected but not transmitting |
| Active, no radio | 20-68mA | CPU running |
| Modem sleep | 3-20mA | WiFi paused, CPU active |
| Light sleep | 0.8mA | CPU paused, RTC running |
| Deep sleep | 10-150µA | Only RTC + ULP available |

### Internal Pull Resistors

| GPIO Type | Pull-up | Pull-down | Resistance |
|-----------|---------|-----------|------------|
| Standard GPIO | Yes | Yes | ~45kΩ |
| GPIO34-39 | **NO** | **NO** | N/A (input-only) |

**Note:** Internal pulls are too weak for I2C (need 4.7kΩ external).

### Drive Strength Configuration

| Setting | Current | Use Case |
|---------|---------|----------|
| 5mA | Weakest | Low power, slow signals |
| 10mA | Low | General purpose |
| 20mA | Default | Most applications |
| 40mA | Maximum | Fast edges, heavy loads |

Higher drive strength = faster edges but more EMI/noise.

---

## Pull-up and Pull-down Resistors

### When Required

| Situation | Pull Type | Typical Value |
|-----------|-----------|---------------|
| I2C bus (SDA) | Pull-UP | **4.7kΩ** |
| I2C bus (SCL) | Pull-UP | **4.7kΩ** |
| 1-Wire bus (DQ) | Pull-UP | **4.7kΩ** |
| Button to GND | Pull-UP | 10kΩ |
| Button to VCC | Pull-DOWN | 10kΩ |
| SPI CS line | Pull-UP | 10kΩ |
| Open-drain output | Pull-UP | 1-10kΩ |
| UART RX (optional) | Pull-UP | 10kΩ (noise immunity) |
| Reset line | Pull-UP | 10kΩ |

### Calculation Formula

```
R = (VCC - VOL) / IOL

Where:
  VCC = Supply voltage (3.3V)
  VOL = Output low voltage (~0.4V)
  IOL = Required sink current (3mA for I2C)

Example (I2C):
  R = (3.3V - 0.4V) / 3mA = 967Ω minimum
  Typical choice: 4.7kΩ (provides margin)
```

### Strength Guidelines

| Resistance | Use Case | Notes |
|------------|----------|-------|
| 1kΩ | Long wires, high capacitance, fast I2C | Strong pull, higher current |
| 4.7kΩ | Standard I2C, 1-Wire, general purpose | Most common choice |
| 10kΩ | Buttons, CS lines, low-power | Standard digital pull |
| 47-100kΩ | Wake-up inputs, ultra-low power | Very weak, slow rise time |

### Consequences of Wrong Value

**Too high (weak pull):**
- Slow signal rise times
- Noise susceptibility
- Communication errors at higher speeds
- May not reach valid HIGH level

**Too low (strong pull):**
- Excessive current consumption
- Device may not be able to pull line LOW
- Wasted power in battery applications

---

## Level Shifting

### When Required

- 5V logic output → 3.3V GPIO input
- 3.3V GPIO output → 5V input (if device doesn't recognize 3.3V as HIGH)
- Bidirectional communication between 3.3V and 5V systems

### Method 1: Voltage Divider (5V → 3.3V, Unidirectional)

**Use for:** Slow signals (<100kHz), input direction only

```
5V Signal ──[1kΩ]──┬──> 3.3V GPIO Input
                   │
                 [2kΩ]
                   │
                  GND

Output: 5V × (2kΩ / 3kΩ) = 3.33V
```

| Pros | Cons |
|------|------|
| Simple, cheap | Input direction only |
| 2 resistors | Slow (RC time constant) |
| No active components | Loads the signal |

### Method 2: N-Channel MOSFET (Bidirectional)

**Use for:** I2C, 1-Wire, open-drain signals up to 400kHz

```
3.3V Side                              5V Side
    │                                      │
  [4.7kΩ]                              [4.7kΩ]
    │                                      │
    ├────────┬──────────────────┬──────────┤
    │        │                  │          │
   SDA    Source              Drain      SDA
  (3.3V)     └───── BSS138 ─────┘       (5V)
                      │
                    Gate
                      │
                    3.3V
```

**Operation:**
- Gate tied to LOW side voltage (3.3V)
- When LOW side pulls down, MOSFET conducts, pulling HIGH side down
- When HIGH side pulls down, body diode conducts, pulling LOW side down
- Pull-ups restore HIGH state on both sides

| Component | Specification |
|-----------|---------------|
| MOSFET | BSS138, 2N7000 (through-hole) |
| Pull-ups | 4.7kΩ on each side |
| Voltage | 3.3V on gate, low side; 5V on high side |

### Method 3: Dedicated Level Shifter ICs

| Chip | Channels | Type | Speed | I2C Safe? |
|------|----------|------|-------|-----------|
| TXB0104 | 4 | Auto-direction | 100 Mbps | **NO** |
| TXB0108 | 8 | Auto-direction | 100 Mbps | **NO** |
| PCA9306 | 2 | I2C-specific | 1 MHz | **YES** |
| PCA9517 | 2 | I2C buffer | 400 kHz | **YES** |
| 74LVC245 | 8 | Unidirectional | 100 MHz | N/A (direction pin) |
| BSS138 modules | 4 | Bidirectional | 400 kHz | **YES** |

**CRITICAL WARNING:** TXB-series level shifters do **NOT** work reliably with open-drain protocols (I2C, 1-Wire). They fight the pull-up resistors and cause communication errors. Use BSS138-based modules or PCA9306 for I2C.

### Method 4: Direct Connection (3.3V → 5V Input)

Many 5V devices recognize 3.3V as logic HIGH:

| Parameter | Typical 5V TTL | Typical 5V CMOS |
|-----------|----------------|-----------------|
| VIH (HIGH threshold) | 2.0V | 3.5V |
| VIL (LOW threshold) | 0.8V | 1.5V |

**Check datasheet for VIH.** If VIH < 3.0V, direct connection usually works.

**NEVER** connect 5V output directly to 3.3V input — level shift or divide required.

---

## Common Mistakes and Warnings

### NEVER Do This

| Mistake | Consequence |
|---------|-------------|
| Connect 5V directly to any GPIO | **Permanent chip damage** |
| Drive relay coil directly from GPIO | Inductive kickback damages GPIO |
| Drive motor directly from GPIO | Overcurrent, voltage spikes |
| Exceed 50mA total on RPi GPIO | Voltage instability, damage |
| Use ESP32 GPIO6-11 (WROOM) | Flash pins — chip crashes |
| Pull GPIO12 HIGH at boot (ESP32) | **Flash voltage brick** |
| Forget pull-ups on I2C | Communication failure |
| Forget pull-up on 1-Wire | Bus doesn't work |
| Use TXB-series for I2C | Unreliable communication |
| Assume GPIO is 5V tolerant | It's not — damage results |

### ALWAYS Do This

| Practice | Reason |
|----------|--------|
| Use current-limiting resistor for LEDs | Prevents overcurrent (220-330Ω) |
| Use flyback diode with relays/motors | Catches inductive voltage spike |
| Use level shifter for 5V ↔ 3.3V | Protects GPIO from overvoltage |
| Check total current draw | Prevent exceeding limits |
| Verify I2C addresses before wiring | Detect conflicts early |
| Add 100nF decoupling capacitor near ICs | Reduces noise, improves stability |
| Use external pull-ups for I2C (4.7kΩ) | Internal pulls too weak |
| Check ESP32 pin restrictions | Strapping, flash, input-only |
| Use transistor for loads >16mA | Protects GPIO |
| Add ESD protection for external connectors | Protects against static |

---

## Quick Reference Card

### Formulas

**LED Resistor:**
```
R = (VCC - Vf) / If
R = (3.3V - 2.0V) / 10mA = 130Ω minimum
Recommended: 220-330Ω (5-10mA, plenty bright)
```

**Voltage Divider:**
```
Vout = Vin × (R2 / (R1 + R2))
For 5V → 3.3V: R1=1kΩ, R2=2kΩ
```

**Pull-up Resistor:**
```
R = (VCC - VOL) / IOL
Standard: 4.7kΩ for I2C/1-Wire, 10kΩ for buttons
```

### Quick Values

| Application | Value |
|-------------|-------|
| LED resistor (3.3V, red/green) | 220-330Ω |
| LED resistor (3.3V, blue/white) | 100-150Ω |
| I2C pull-up | **4.7kΩ** to 3.3V |
| 1-Wire pull-up | **4.7kΩ** to 3.3V |
| Button pull-up/down | 10kΩ |
| SPI CS pull-up | 10kΩ |
| 5V → 3.3V divider | 1kΩ + 2kΩ |
| Flyback diode | 1N4148 or 1N4007 |
| Decoupling capacitor | 100nF ceramic |

### Current Limits Summary

| Platform | Per Pin | Total GPIO |
|----------|---------|------------|
| Raspberry Pi | 16mA | **50mA** |
| ESP32 | 20mA recommended | ~1200mA chip total |

### Voltage Summary

| Parameter | RPi | ESP32 |
|-----------|-----|-------|
| Logic HIGH | 3.3V | 3.3V |
| Logic LOW | 0V | 0V |
| Max input | 3.3V | 3.3V |
| 5V tolerant | **NO** | **NO** |

### Common Pin Restrictions

**Raspberry Pi:**
- GPIO0/1: Reserved (HAT EEPROM)
- GPIO14/15: UART/BT conflict (Pi 3/4/Zero2W)

**ESP32:**
- GPIO6-11: Flash pins — **NEVER USE**
- GPIO12: Strapping — **DANGER** (flash voltage)
- GPIO16-17: PSRAM (WROVER only)
- GPIO34-39: Input only, no pulls

---
