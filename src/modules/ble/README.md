# BLE Security Suite Module v4.0

⚠️ **DISCLAIMER**: For authorized testing and educational purposes only. Success varies by target device, firmware, and patch level. Modern/patched devices will resist most attacks.

## About

BLE Suite is a comprehensive Bluetooth Low Energy security testing framework for ESP32 devices running Bruce firmware. Provides reconnaissance, protocol exploitation, and post-exploitation capabilities.

## Hardware Integration

- **NRF24L01+** - BLE frequency jamming (3 jamming modes, jam & connect attacks)
- **FastPair Crypto** - mbedTLS-based cryptographic operations (ECDH, AES-CCM, HKDF, key derivation)

## NimBLE Compatibility Notes

This suite runs on **NimBLE 2.5.0+** which has specific limitations:

### ✅ Fully Supported (Works over GATT)
- AT command injection
- Information disclosure (IMEI, manufacturer, model, firmware)
- Buffer overflow tests
- Call control (answer/hang up/dial)
- DTMF injection
- USSD exploits
- FastPair handshake and crypto
- All software features (scoring, caching, logging, orchestration)

### ⚠️ Limited Support
- SMS spoofing (depends on device support)
- SIM access (often restricted)
- Phonebook access (often restricted)
- Event monitoring (polling-based, no async notifications)

### ❌ Not Supported (NimBLE Limitation)
- SCO audio (no L2CAP CoC support)
- HFP audio routing
- HFP codec negotiation for audio
- LE Audio
- BLE 5.0 extended advertising
- Multiple simultaneous advertising instances
- Random MAC advertising

## Core Components

### BLEStateManager
Handles BLE stack lifecycle, client tracking, and cleanup with automatic initialization recovery.

### ScannerData
Stores discovered devices with service detection:
- HFP detection (UUIDs 111E/111F)
- FastPair detection (UUID FE2C)
- Audio/HID service flags
- Device scoring and stability tracking

### Device Scoring
- RSSI-based scoring with stability tracking
- Attack potential calculation (0-100)
- Automatic sorting of high-value targets
- Activity pattern detection

### Connection Caching
- Stores successful connection parameters
- 60-80% faster reconnections
- MTU and service UUID caching
- Automatic parameter optimization per device type

### Graduated Connection Strategy
- 5-phase connection approach: Probe → Fast → Aggressive → Exploit → Reconnect
- Automatic fallback between strategies
- Device-type specific parameter tuning
- Cached connection re-use

### Robust GATT Client
- Retry logic for reads/writes
- Automatic characteristic refresh
- Notification waiting support
- Service discovery retry with exponential backoff

## Attack Engines

### HIDExploitEngine
- OS-specific attacks (Apple spoof, Windows bypass, Android JustWorks)
- Boot protocol injection
- Connection parameter manipulation
- Security mode bypass
- Address spoofing
- Service discovery hijack

### WhisperPairExploit
- FastPair cryptographic handshake simulation
- Protocol state confusion
- Crypto overflow attacks
- Memory corruption attempts
- Full FastPair v2/v3 handshake support

### AudioAttackService
- AVRCP media control hijacking
- Audio stack crashing
- Telephony alert injection

### FastPairExploitEngine
- Device scanning with model identification
- Memory corruption attacks
- State confusion attacks
- Crypto overflow attacks
- Handshake fault attacks
- Rapid connection attacks
- Popup spam (Regular/Fun/Prank/Custom)
- Vulnerability testing

### HFPExploitEngine
- Full AT command injection (works over GATT)
- Information disclosure (IMEI, manufacturer, model, firmware)
- Buffer overflow testing
- Command injection attacks
- USSD exploit
- SIM access attacks (device dependent)
- Phonebook extraction (device dependent)
- DTMF injection
- Call control (answer/hang up/dial)
- Event monitoring (polling-based)
- Stack crash testing
- **Note:** Audio/SCO features disabled due to NimBLE limitations

### HIDDuckyService
- Full DuckyScript injection
- Keyboard keystroke simulation
- Special key handling
- Combo key support

### AuthBypassEngine
- Address spoofing
- Zero-key auth attempts
- Legacy pairing force
- Known device database

### MultiConnectionAttack
- Connection flooding
- Advertising spam
- NRF24 jamming coordination
- Jam & connect attacks

### Attack Orchestrator
- Chain attacks with priority ordering
- Rollback capability for failed attacks
- Step execution with timeout
- Attack result tracking
- Automatic cleanup on failure

### BLE Mirage
- Clone device advertisements
- Create network of fake devices
- Device spoofing for misdirection
- Defensive/offensive testing capability
- **Note:** Single instance only (NimBLE limitation)

### Attack Scheduler
- Analyzes device activity patterns
- Predicts optimal attack windows
- Suggests best timing for attacks
- Tracks device wake patterns

### Device Fingerprinting
- Builds device personality profiles
- Tracks response times and MTU preferences
- Detects notification/indication support
- Service characteristic ordering analysis

## Attack Menu (16 Main Items)

### Reconnaissance
1. **Quick Vulnerability Scan** - HFP + FastPair testing
2. **Deep Device Profiling** - Full service enumeration with characteristic analysis
3. **Smart Recon** - Scoring-based target prioritization
4. **Device Fingerprinting** - Builds device personality profiles

### Protocol Suites
5. **FastPair Suite** - 9 options (vulnerability test, memory corruption, state confusion, crypto overflow, popup spam, all exploits, smart exploit)
6. **HFP Suite** - 6 options (vulnerability test, connection, full chain, command injection, info disclosure, stack crash)
7. **Audio Suite** - 5 options (AVRCP, audio stack crash, telephony, all tests, media control)
8. **HID Suite** - 6 options (vulnerability test, force connection, keystrokes, DuckyScript, OS exploits, all)

### Advanced Attacks
9. **Memory Corruption Suite** - 6 options (FastPair memory corruption, state confusion, crypto overflow, handshake fault, rapid connection, all)
10. **DoS Attacks** - 4 options (connection flood, advertising spam, jam & connect, protocol fuzzer)
11. **Orchestrated Attack** - Chain attacks with rollback
12. **Mirage Attack** - Device cloning and spoofing
13. **Attack Scheduler** - Optimal timing analysis
14. **Payload Delivery** - 3 options (DuckyScript, PIN brute force, auth bypass)
15. **Testing Tools** - 4 options (write access, audio control, fuzzer, HID test)

### Chain Attacks
16. **Universal Attack Chain** - Attempts HFP → HID → FastPair sequentially based on detected services

## Attack Logging
- JSON export of all attack attempts
- Timestamp, target, attack type, success/failure
- Connection quality metrics
- Duration tracking
- Export to SD/LittleFS

## Smart Features
- Auto-detection of HFP/FastPair services during scan
- Context-aware attack suggestions
- Seamless pivot chains (HFP→HID)
- Device model identification for FastPair
- RSSI-based device sorting with scoring
- Connection parameter optimization per device type
- Automatic BLE stack recovery
- NimBLE limitation awareness in HFP engine

## Dependencies
- NimBLE-Arduino 2.5.0+
- mbedTLS (ECDH, AES-CCM, HKDF)
- TFT_eSPI
- SD card support
- NRF24L01+ (optional)

## Flow
Main menu → Select attack → Scan for targets → Execute → Return to menu

## NimBLE-Specific Features

The HFP engine automatically detects NimBLE limitations and:
- Disables SCO/audio features gracefully
- Uses polling instead of async notifications
- Provides clear warnings when features are unavailable
- Falls back to GATT-based operations where possible

## Changelog

### v4.0 (24/08/2026)
- Added Device Scoring system with attack potential calculation
- Added Connection Caching for 60-80% faster reconnections
- Added Graduated Connection Strategy (5-phase connection)
- Added Robust GATT Client with retry logic
- Added Attack Orchestrator with rollback capability
- Added BLE Mirage for device cloning
- Added Attack Scheduler for optimal timing
- Added Device Fingerprinting for personality profiles
- Added Attack Logging with JSON export
- Enhanced HFP Engine with full AT command support (NimBLE-optimized)
- Enhanced FastPair Crypto with HKDF and proper key derivation
- Added NimBLE compatibility notes and graceful fallbacks
- Updated UI with clean theme integration
- Bumped version to 4.0

### v3.1 (21/07/2026)
- Added Samsung MAC OUI detection
- Expanded FastPair model database
- Enhanced manufacturer parsing
- BLE Sniffer improvements

## Known Limitations (NimBLE)

| Feature | Status | Workaround |
|---------|--------|------------|
| SCO Audio | ❌ Not supported | N/A |
| HFP Audio Routing | ❌ Not supported | N/A |
| Async Notifications | ❌ Not supported | Polling |
| Multiple Advertising | ❌ Not supported | Single instance |
| Random MAC | ❌ Not supported | Use public MAC |
| LE Audio | ❌ Not supported | N/A |
| BLE 5.0 Ext Adv | ❌ Not supported | N/A |
| AT Commands | ✅ Supported | Works over GATT |
| FastPair | ✅ Supported | Works over GATT |
| HID Injection | ✅ Supported | Works over GATT |