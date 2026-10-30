Native WirelessPi WiFite 0.8.1 app/backend port for Bruce native T-Embed integration.
Original source: salikisazazil/Flipper-firmware-Flashable-files WirelessPi-Native-Tools-1.0-source.tar.xz.
Core, Pixiewps, and SDK licenses retained. Vendored Espressif private headers and esp_wps.c from ESP-IDF v5.5.4 (Apache-2.0/BSD as per file).
The adapted SDK translation unit overrides the archive implementation in full T-Embed CC1101 builds (T_EMBED_1101 without LITE_VERSION); global framework installation is not modified.
