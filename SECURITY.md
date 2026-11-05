# Security Policy

## 1. Supported Versions

Security fixes and vulnerability patches are applied to the active Bruce `2.x` development branch (`main` / `dev`) and included in the next official release.

| Version | Supported |
| :--- | :--- |
| `2.x` (`main` / latest release) | :white_check_mark: Yes |
| `1.x` (Legacy Arduino-ESP32 v2.x builds) | :x: No |

For board-level hardware security notes or peripheral isolation details, see [boards/README.md](boards/README.md) and [src/hal/README.md](src/hal/README.md).

---

## 2. Reporting a Vulnerability

If you discover a security vulnerability in Bruce firmware, its embedded WebUI, ESP-NOW communication layer, or official Bruce hardware designs (`pcbs/`):

1. **Do NOT open a public GitHub issue or post exploit details in public Discord channels.**
2. **Report privately via GitHub Security Advisories:**
   - Navigate to the repository's **Security** tab on GitHub and click **"Report a vulnerability"** to open a private advisory draft with the maintainers.
   - Alternatively, contact the core maintainers directly via the official [Bruce Discord Server](https://discord.gg/WJ9XF9czVT) by asking a moderator/developer for a private security contact channel before sharing technical details.

### What to Include in Your Report
- Affected component (e.g., `src/core/wifi/webInterface.cpp`, `src/core/connect/`, specific board target, or build script).
- Firmware version or commit SHA and target device environment.
- Step-by-step reproduction instructions or proof-of-concept payload.
- Impact assessment (e.g., remote code execution, heap/stack buffer overflow, authentication bypass, credential leak, or denial of service).

We aim to acknowledge vulnerability reports within **72 hours** and work with reporters on coordinated disclosure once a fix is released.

---

## 3. Operational Safety & Default Credentials

Bruce is a portable security research and Red Team firmware. Keep the following operational security considerations in mind when deploying or testing a device:

### Default WebUI & Access Point Credentials
Out of the box (`src/core/config.h`), Bruce uses well-known default credentials:
- **Wi-Fi Access Point SSID / Password:** `BruceNet` / `brucenet`
- **WebUI HTTP Login (`http://bruce.local`):** `admin` / `bruce`

**Recommendation:** Change both the AP password and WebUI credentials immediately in **Config** or `/bruce.conf` before connecting Bruce to untrusted Wi-Fi networks or enabling the WebUI during engagements.

### Sensitive Data on SD Card & LittleFS
- Configuration files (`/bruce.conf`), Wi-Fi passwords, Wigle API tokens, captured PCAPs, Evil Portal logs, and RFID/Sub-GHz dumps are stored unencrypted on LittleFS or the microSD card.
- Remove or sanitize SD cards and format LittleFS before lending, selling, or RMAing a device.

### Legal & Regulatory Compliance
Bruce includes modules capable of transmitting across Sub-GHz (CC1101), 2.4 GHz / 5 GHz Wi-Fi, Bluetooth Low Energy (BLE), NRF24, LoRa, FM/RDS, and NFC/RFID bands, as well as USB/BLE HID injection (BadUSB / BadBLE):
- **Authorized Use Only:** Only use active transmission, deauthentication, jamming, spoofing, or HID injection features on networks, frequencies, and devices you own or have explicit written authorization to test.
- **Spectrum Regulations:** Ensure your transmit frequencies and power levels comply with local radio spectrum regulations (FCC, CE, IC, ANATEL, etc.).
