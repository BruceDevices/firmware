# ESP32 Specifics Reference

## Table of Contents

- [1. Hardware Architecture & Chip Families](#1-hardware-architecture--chip-families)
- [2. Memory Management Unit (MMU) & Heap Allocation](#2-memory-management-unit-mmu--heap-allocation)
- [3. Peripherals, Timers & Interrupts](#3-peripherals-timers--interrupts)
- [4. Strapping Pins Deep Dive](#4-strapping-pins-deep-dive)
- [5. ADC2/WiFi Conflict Explained](#5-adc2wifi-conflict-explained)
- [6. Flash and PSRAM Pin Details](#6-flash-and-psram-pin-details)
- [7. Input-Only Pins](#7-input-only-pins)
- [8. RTC Domain and Deep Sleep](#8-rtc-domain-and-deep-sleep)

---

## 1. Hardware Architecture & Chip Families

When advising on hardware selection, apply the following matrix of architectures, wireless protocols, and target use cases:

* **ESP32 (Original):** Dual-core Xtensa LX6, 240 MHz, Wi-Fi 4, BT Classic, BLE 4.2. Best for legacy projects requiring Bluetooth Classic.
* **ESP32-S2:** Single-core Xtensa LX7, 240 MHz, Wi-Fi 4. Best for ultra-low power and USB OTG/HID.
* **ESP32-S3 (Performance):** Dual-core Xtensa LX7, 240 MHz, Wi-Fi 4, BLE 5.0. Features Vector instructions for AI/ML and complex GUIs.
* **ESP32-C2:** Single-core RISC-V, 120 MHz, Wi-Fi 4, BLE 5.0. Cost-sensitive ESP8266 replacements.
* **ESP32-C3 (Connectivity):** Single-core RISC-V, 160 MHz, Wi-Fi 4, BLE 5.0. Standard budget IoT node.
* **ESP32-C5:** Single-core RISC-V, 240 MHz, Dual-band Wi-Fi 6 (2.4/5GHz), BLE 5, Zigbee/Thread.
* **ESP32-C6:** Single-core RISC-V, 160 MHz, Wi-Fi 6 (2.4GHz), BLE 5.3, Zigbee/Thread. Next-gen Matter/mesh nodes.
* **ESP32-H2 (Hub/Home):** Single-core RISC-V, 96 MHz, BLE 5.0, Zigbee/Thread. *Crucial Note: No Wi-Fi*.
* **ESP32-P4 (Powerhouse):** Dual-core RISC-V, 400 MHz. *Crucial Note: No wireless capabilities*. Features H.264 encoding, dual MIPI, and heavy multimedia processing.

---

## 2. Memory Management Unit (MMU) & Heap Allocation

Understand the complex memory hierarchy managed by the MMU:

* **DRAM (Data RAM):** Holds standard variables and the heap. Single-byte accessible (`MALLOC_CAP_8BIT`).
* **IRAM (Instruction RAM):** Strictly holds executable code operating at full CPU speed. Code handling interrupts or flash writes MUST reside here. Generic access requires 32-bit alignment (`MALLOC_CAP_32BIT`).
* **D/IRAM:** Flexible RAM usable dynamically on both data and instruction buses.
* **RTC Memory:** Fast memory runs the ULP coprocessor; Slow memory holds `RTC_DATA_ATTR` variables during Deep Sleep.
* **PSRAM (Pseudo-Static RAM):** External/integrated RAM mapped to virtual address space for heavy workloads.
* **Heap Allocation:** Use capabilities-based allocation: `heap_caps_malloc(size, MALLOC_CAP_DMA)` for hardware DMA, `MALLOC_CAP_SPIRAM` for external PSRAM, and `MALLOC_CAP_SIMD` for 16-byte aligned SIMD operations.

---

## 3. Peripherals, Timers & Interrupts

### Timers & Watchdogs
* **GPTimer:** In ESP-IDF v5.0+, the legacy Timer Group is deprecated and replaced by GPTimer (`gptimer.h`). Requires `gptimer_config_t` (clock source, direction, resolution). Operations include `gptimer_start()`, `stop()`, and `set_raw_count()`.
* **Watchdog Timers:** Interrupt WDT (IWDT) monitors CPU stalls from long ISRs. Task WDT (TWDT) triggers if a task fails to yield via `vTaskDelay`.

### Interrupt Architecture
* **Xtensa:** Uses internal Xtensa controller (High-level assembly, Low-level C).
* **RISC-V:** Uses standard PLIC and CLIC.
* **Interrupt Matrix:** Maps physical hardware interrupts to specific CPU lines (Core 0 vs Core 1).

### Specialized Peripherals & Accelerators
* **Comm Interfaces:** I2S (digital audio/PDM), TWAI (CAN 2.0B bus), USB-OTG (S2/S3 Host/Device), USB-Serial/JTAG (C3, C6, S3, H2 direct OpenOCD debug).
* **Motor/Lighting:** LEDC (Hardware PWM), MCPWM (Advanced BLDC driving, dead-time), PCNT (Quadrature encoders).
* **P-Series / S-Series Accelerators:** Hardware JPEG/H.264 codecs, ISP for MIPI cameras, PPA pixel blending, GDMA engines, Crypto/TRNG accelerators for Secure Boot.

---

## 4. Strapping Pins Deep Dive

ESP32 reads the state of five GPIO pins at boot to determine operating mode. These pins are safe to use for other purposes after boot completes, but their state at reset/power-on matters.

### GPIO0 — Boot Mode Selection

| State at Boot | Result |
|---------------|--------|
| HIGH (default) | Normal execution from flash |
| LOW | Download mode (firmware upload via UART) |

**Details:**
- Has internal pull-up resistor (default HIGH)
- Safe to use after boot for any function
- Commonly used for: buttons, capacitive touch, general I/O

**Warning:** If using GPIO0 as a button input, ensure it is not held LOW during reset/power-on, or the chip will enter download mode instead of running your program.

---

### GPIO12 (MTDI) — DANGER — Flash Voltage Selection

| State at Boot | Result |
|---------------|--------|
| **LOW (required)** | 3.3V flash voltage — CORRECT for most modules |
| **HIGH** | 1.8V flash voltage — WILL FAIL TO BOOT on 3.3V flash modules |

**THIS IS THE MOST DANGEROUS STRAPPING PIN**

**Safe usage patterns:**
- Use external pull-DOWN resistor if GPIO12 must be used
- Ensure any connected device doesn't drive HIGH during boot
- A permanent eFuse override exists. Treat it as a separate hardware provisioning operation.

**Recovery from "bricked" state:**
1. Disconnect anything from GPIO12
2. Add pull-down resistor (10kΩ to GND) if needed
3. Hold GPIO0 LOW
4. Power cycle the module
5. Flash should now be accessible via esptool

---

## 5. ADC2/WiFi Conflict Explained

### The Problem
ADC2 shares internal hardware resources with the WiFi RF calibration and transmission circuits. When WiFi (or Bluetooth on original ESP32) is active, ADC2 readings are unreliable or completely invalid.

### ADC1 Channels (Always Safe)
ADC1 has its own dedicated hardware and works regardless of WiFi state:
- **GPIO32-39** (Original ESP32)

### Variants Without This Conflict
- **ESP32-C3:** Single ADC, always available
- **ESP32-C6:** Single ADC, always available

---

## 6. Flash and PSRAM Pin Details

### SPI Flash Pins (GPIO6-11) — NEVER USE
These pins are hardwired to the SPI flash memory chip inside the module. Using them will immediately crash the ESP32.

### PSRAM Pins (GPIO16-17) — WROVER Only
ESP32-WROVER modules include onboard PSRAM that uses GPIO16 and GPIO17.

---

## 7. Input-Only Pins

### GPIO34, 35, 36, 39 — Hardware Limitations
These four pins CANNOT be used for digital output, I2C, SPI MOSI/SCLK, or UART TX. They have no internal pull resistors.

---

## 8. RTC Domain and Deep Sleep

### RTC GPIO Overview
Only specific GPIOs can function during deep sleep or wake the ESP32 from deep sleep.

**Deep Sleep Wake Sources:**
- EXT0: Single RTC GPIO, level-triggered
- EXT1: Multiple RTC GPIOs
- Touch pad wake
- ULP coprocessor
