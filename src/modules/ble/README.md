BLE Security Suite Module

⚠️ DISCLAIMER

For authorized testing and educational purposes only. Success varies by target device, firmware, and patch level. Modern/patched devices will resist most attacks.

About

BLE Suite is a comprehensive Bluetooth Low Energy security testing framework for ESP32 devices running Bruce firmware. Provides reconnaissance, protocol exploitation, and post-exploitation capabilities.

Hardware Integration

· NRF24L01+ - BLE frequency jamming (3 jamming modes, jam & connect attacks)
· FastPair Crypto - mbedTLS-based cryptographic operations (ECDH, AES-CCM, key generation)

Core Components

BLEStateManager

Handles BLE stack lifecycle, client tracking, and cleanup.

ScannerData

Stores discovered devices with service detection:

· HFP detection (UUIDs 111E/111F)
· FastPair detection (UUID FE2C)
· Audio/HID service flags

Attack Engines

HIDExploitEngine

· OS-specific attacks (Apple spoof, Windows bypass, Android JustWorks)
· Boot protocol injection
· Connection parameter manipulation
· Security mode bypass
· Address spoofing
· Service discovery hijacking

WhisperPairExploit

· FastPair cryptographic handshake simulation
· Protocol state confusion
· Crypto overflow attacks
· Memory corruption attempts

AudioAttackService

· AVRCP media control hijacking
· Audio stack crashing
· Telephony alert injection

FastPairExploitEngine

· Device scanning with model identification
· Memory corruption attacks
· State confusion attacks
· Crypto overflow attacks
· Handshake fault attacks
· Rapid connection attacks
· Popup spam (Regular/Fun/Prank/Custom)
· Vulnerability testing

HIDDuckyService

· Full DuckyScript injection
· Keyboard keystroke simulation
· Special key handling
· Combo key support

AuthBypassEngine

· Address spoofing
· Zero-key auth attempts
· Legacy pairing force
· Known device database

MultiConnectionAttack

· Connection flooding
· Advertising spam
· NRF24 jamming coordination
· Jam & connect attacks

Attack Menu (11 Main Items)

Reconnaissance

1. Quick Vulnerability Scan - HFP + FastPair testing
2. Deep Device Profiling - Full service enumeration with characteristic analysis

Protocol Suites

1. FastPair Suite - 6 options (vulnerability test, memory corruption, state confusion, crypto overflow, popup spam, all exploits)
2. HFP Suite - 4 options (CVE-2025-36911 test, connection, full chain, HID pivot)
3. Audio Suite - 5 options (AVRCP, audio stack crash, telephony, all tests)
4. HID Suite - 6 options (vulnerability test, force connection, keystrokes, DuckyScript, OS exploits, all)

Advanced Attacks

1. Memory Corruption Suite - 6 options (FastPair memory corruption, state confusion, crypto overflow, handshake fault, rapid connection, all)
2. DoS Attacks - 4 options (connection flood, advertising spam, jam & connect, protocol fuzzer)
3. Payload Delivery - 3 options (DuckyScript, PIN brute force, auth bypass)
4. Testing Tools - 4 options (write access, audio control, fuzzer, HID test)

Chain Attacks

1. Universal Attack Chain - Attempts HFP → HID → FastPair sequentially based on detected services

Smart Features

· Auto-detection of HFP/FastPair services during scan
· Context-aware attack suggestions
· Seamless pivot chains (HFP→HID)
· Device model identification for FastPair
· RSSI-based device sorting

Dependencies

· NimBLE-Arduino 2.3.7
· mbedTLS (ECDH, AES-CCM)
· TFT_eSPI
· SD card support
· NRF24L01+ (optional)

Flow

Welcome screen (once per session) → Main menu → Select attack → Scan for targets → Execute → Return to menu