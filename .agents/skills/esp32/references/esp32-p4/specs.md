# ESP32-P4 Specifications

## 1. Core Architecture
- **CPU:** Dual-core RISC-V, 400 MHz.
- **Wireless:** **None.** (No Wi-Fi, No Bluetooth).
- **Application:** High-performance multimedia, H.264 encoding, camera/display processing.

## 2. Memory
- **SRAM:** 768 KB L2 Cache/SRAM.
- **PSRAM:** Heavy integration support for high-speed external RAM.

## 3. Peripheral Mapping
- **Multimedia:**
  - H.264 Video Encoder.
  - Dual MIPI CSI/DSI interfaces.
  - Hardware Pixel Processing Accelerator (PPA).
  - Image Signal Processor (ISP).
- **GPIO Count:** ~50+.
- **DMA:** High-performance GDMA.
- **Security:** Hardware crypto accelerators, TRNG, Secure Boot.

## 4. Hardware Safety & Constraints
- **Thermal:** High-clock dual-core RISC-V may require thermal management in tight enclosures.
- **Power:** Higher power consumption compared to wireless IoT variants.
