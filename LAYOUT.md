# Repository Layout

This document describes the Bruce 2.0 repository directory structure and where each subsystem lives.

```text
.
├── boards/                             # Hardware targets and board definitions
│   ├── README.md                       # Step-by-step guide for porting a new board
│   ├── _jsonfiles/                     # Per-MCU PlatformIO board manifests & Arduino pin headers
│   │   ├── esp32.json / esp32.h
│   │   ├── esp32s2.json / esp32s2.h
│   │   ├── esp32s3.json / esp32s3.h
│   │   ├── esp32c3.json / esp32c3.h
│   │   ├── esp32c5.json / esp32c5.h
│   │   ├── esp32c6.json / esp32c6.h
│   │   ├── esp32p4.json / esp32p4_es.json / esp32p4.h
│   │   ├── pins_arduino.h              # Dispatcher that includes the active MCU header
│   │   └── variant.cpp
│   ├── _New-Device-Model/              # Canonical template for porting a new device
│   │   ├── _New-Device-Model.ini       # Annotated build flags and environment config
│   │   └── interface.cpp               # Annotated GPIO, display, power, and input hooks
│   └── <board>/                        # Per-device folder (e.g. m5stack-cardputer, lilygo-t-embed-cc1101)
│       ├── <board>.ini                 # PlatformIO [env:...] configuration and -D flags
│       └── interface.cpp               # Board-specific hardware init and InputHandler()
├── docker/                             # Containerized CI and multi-environment build runners
│   ├── Dockerfile.ci
│   └── run_all_envs.sh
├── embedded_resources/
│   └── web_interface/                  # HTML/CSS/JS sources compiled into include/webFiles.h by patch.py
├── include/                            # Project-wide global C/C++ headers
│   ├── README                          # Guide to global headers and include rules
│   ├── current_year.h                  # Auto-generated build year macro (pre_build_current_year.py)
│   ├── DisplayDrivers_User_Setup.h     # Project-level configuration hook for DisplayDrivers
│   ├── globals.h                       # Shared runtime state, extern singletons, and UI structs
│   ├── interface.h                     # Hardware lifecycle and input hook declarations
│   ├── MenuItemInterface.h             # Base class for top-level menu items and icons
│   ├── precompiler_flags.h             # Fallback defaults for optional board and feature macros
│   ├── SerialDevice.h                  # Abstract serial stream interface (UART / USB CDC)
│   ├── tftLogger.h                     # Display wrapper supporting remote screen logging
│   └── webFiles.h                      # Auto-generated gzipped WebUI assets in PROGMEM
├── lib/                                # Vendored and custom PlatformIO libraries
│   ├── README                          # Overview of vendored libraries and submodules
│   ├── Bad_Usb_Lib/                    # USB TinyUSB HID, BLE HID, and CH9329 keyboard/mouse stack
│   ├── BruceWifite/                    # Native WiFite / WPS Pixie Dust engine
│   ├── CYD-touch/                      # Touch controller helpers for Cheap Yellow Display boards
│   ├── DisplayDrivers/                 # Multi-backend display abstraction (TFT_eSPI, LovyanGFX, M5GFX, Arduino_GFX, EPD)
│   ├── ESP32_Button/                   # ESP-IDF button component for 2-button devices
│   ├── HAL/                            # I2C GPIO expanders (AW9523, PCA9555) and custom SD/SD_MMC drivers
│   ├── mquickjs_headers/               # Pre-compiled MicroQuickJS standard library bytecode headers
│   ├── PN532_SRIX/                     # ST SRIX4K / SRI512 NFC tag driver for PN532
│   ├── RotaryEncoder/                  # Quadrature rotary encoder driver
│   ├── RTC/                            # Real-time clock drivers (BM8563, PCF85063)
│   ├── SensorLib/                      # Touch, IMU, and sensor drivers
│   ├── TFT_eSPI/                       # Customized Bodmer TFT_eSPI driver
│   ├── TFT_eSPI_QRcode/                # QR code generator and renderer
│   ├── utility/                        # Legacy M5Stack PMIC (AXP192), fuel gauge (BQ27220), and Cardputer keyboard drivers
│   └── XPowersLib/                     # PMIC and power management IC library (AXP2101, BQ25896, etc.)
├── media/                              # Documentation images, wiring diagrams, and artwork
├── pcbs/                               # Open-source custom PCB designs and 3D enclosures
├── sd_files/                           # Sample JS scripts, Sub-GHz/IR/NFC payloads, portals, and themes
├── src/                                # Firmware source code
│   ├── main.cpp                        # Firmware entry point (setup, loop, FreeRTOS input task)
│   ├── core/                           # Core firmware services and UI framework
│   │   ├── bus_HAL.h / .cpp            # Shared SPI/I2C bus arbitration and recovery
│   │   ├── config.h / .cpp             # Persistent user settings (/bruce.conf)
│   │   ├── configPins.h / .cpp         # Runtime-configurable peripheral pin assignments (/brucePins.conf)
│   │   ├── connect/                    # ESP-NOW peer-to-peer file and command sharing
│   │   ├── display.h / .cpp            # UI widgets, loopOptions(), status bar, and drawing helpers
│   │   ├── i2c_finder.h / .cpp         # Interactive I2C bus scanner
│   │   ├── led_control.h / .cpp        # Addressable RGB LED effects and status indicators
│   │   ├── main_menu.h / .cpp          # Main menu carousel and grid dispatcher
│   │   ├── massStorage.h / .cpp        # USB MSC (Mass Storage Class) exposure for SD card
│   │   ├── menu_items/                 # MenuItemInterface implementations for each top-level menu
│   │   ├── mifare_keys_manager.h / .cpp# MIFARE Classic key dictionary manager
│   │   ├── mykeyboard.h / .cpp         # On-screen and physical keyboard input handling (`check()`, `inputLock()`)
│   │   ├── net_utils.h / .cpp          # Network formatting and lookup helpers
│   │   ├── passwords.h / .cpp          # Password prompt and credential helpers
│   │   ├── powerSave.h / .cpp          # Screen dimming, sleep, and wake management
│   │   ├── radio_mem.h                 # Shared radio memory buffer helpers
│   │   ├── ram_profile.h / .cpp        # Heap and PSRAM profiling utilities
│   │   ├── scrollableTextArea.h / .cpp # Scrollable text viewer widget
│   │   ├── sd_functions.h / .cpp       # LittleFS and SD/SD_MMC file browser and operations
│   │   ├── serial_commands/            # SimpleCLI command handlers (wifi, rf, ir, rfid, wire, storage, etc.)
│   │   ├── serialcmds.h / .cpp         # Background Serial CLI task and dispatcher
│   │   ├── settings.h / .cpp           # Interactive settings menus
│   │   ├── spectrum_plot.h / .cpp      # Shared waterfall / bar spectrum visualizer
│   │   ├── startup_app.h / .cpp        # Boot auto-run app selector
│   │   ├── tftLogger/                  # Binary screen stream serializer for WebUI/Serial navigation
│   │   ├── theme.h / .cpp              # Custom JSON UI theme loader
│   │   ├── USBSerial/                  # USB CDC serial wrapper
│   │   └── wifi/                       # Wi-Fi manager, WebUI server, WireGuard, and network services
│   ├── hal/                            # Shared board Hardware Abstraction Layer
│   │   ├── README.md                   # Architecture and usage guide for src/hal/
│   │   ├── device.h                    # Pure-data hardware descriptor structs (DeviceButtons, DeviceTouch, etc.)
│   │   ├── bright/                     # Backlight PWM control (`bright.h`, `bright.cpp`)
│   │   ├── inputs/                     # Button, rotary encoder, and touchscreen polling
│   │   └── power/                      # PMIC charger (`pmic.*`) and battery fuel gauge (`gauge.*`) wrappers
│   └── modules/                        # Feature modules
│       ├── badusb_ble/                 # DuckyScript parser, BadUSB, and BadBLE hid execution
│       ├── bjs_interpreter/            # MicroQuickJS JavaScript engine and native C bindings
│       ├── ble/                        # BLE scanner, Apple/Windows/Samsung/Android proximity spam
│       ├── ble_api/                    # Shared NimBLE helper layer
│       ├── ethernet/                   # W5500 Ethernet support and network discovery
│       ├── fm/                         # Si4713 FM/RDS radio broadcast module
│       ├── gps/                        # NMEA GPS tracker and WiGLE wardriving logger
│       ├── ir/                         # Infrared receiver, custom `.ir` transmitter, and TV-B-Gone
│       ├── lora/                       # SX126x LoRa packet sniffer, beacon, and messaging
│       ├── NRF24/                      # 2.4 GHz spectrum analyzer, jammer, and MouseJack / keyboard injection
│       ├── others/                     # Audio player, mic spectrum, iButton, QR codes, timers, and utilities
│       ├── pwnagotchi/                 # Brucegotchi (Pwngrid peer interaction and frame injection)
│       ├── reverseShell/               # Reverse shell payloads and listener tools
│       ├── rf/                         # Sub-GHz (CC1101 / RMT) scanner, protocol codecs, raw replay, and jammer
│       ├── rfid/                       # NFC/RFID tools (PN532, ST25R3916, RC522, 125 kHz, ChameleonUltra, AmiiboLink, EMV)
│       ├── wifi/                       # Deauth, Beacon spam, Evil Portal, ARP spoofing, Responder, SSH/Telnet, Sniffer
│       └── wire/                       # Hardware bus tools (UART terminal, I2C scanner/dump, SPI flash/console, JTAG scanner)
├── tools/
│   └── gen_mfcodes.py                  # Host utility for generating encrypted KeeLoq manufacturer key tables
├── build.py                            # Local interactive build helper script
├── check_submodules.py                 # PlatformIO pre-script ensuring git submodules in lib/ are initialized
├── custom_4Mb.csv / custom_8Mb.csv / custom_16Mb.csv  # ESP32 flash partition tables
├── docker-compose.yml                  # Docker Compose specification for reproducible builds
├── flto_prep.py                        # PlatformIO pre-script enabling whole-program LTO at link time
├── gen_mqjs_headers.py                 # PlatformIO pre-script compiling JS stdlib bindings into lib/mquickjs_headers/
├── patch.py                            # PlatformIO pre-script patching libnet80211.a and bundling WebUI assets
├── patch_library_conflicts.py          # PlatformIO pre-script resolving symbol/header clashes in .pio/libdeps
├── platformio.ini                      # Root PlatformIO configuration, shared dependencies, and build flags
└── pre_build_current_year.py           # PlatformIO pre-script generating include/current_year.h
```
