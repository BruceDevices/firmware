# Pixiewps port attribution

Source: https://github.com/wiire-a/pixiewps

Exact revision: `20eaf62b3fd2afc67b039d1521460b3b7d165842`.

Copyright (c) 2015–2017 wiire <wi7ire@gmail.com>. GPL-3.0-or-later;
the original license is included as `COPYING.pixiewps`.

`pixie_prng.h` contains the Ralink and eCos PRNG functions from upstream
`src/pixiewps.c`, and the optimized glibc routines from
`src/random/glibc_random_yura.c` (based on @1yura's code). Their upstream
attribution is retained. `pixie.c` ports the search strategy to one cancellable
embedded worker using the native core's Mbed TLS hashing. It removes Linux CLI,
process and pthread dependencies, uses callbacks for progress, bounds message
storage, and reduces eCos simplest to its equivalent 256 observable streams.

The new Pixie port and integrated WIFITE distribution are GPL-3.0-or-later.
The independently written protocol core files retain their stated MIT license;
their license text is in `LICENSE.core`. Existing firmware components retain
their respective licenses. Full modified project source, build instructions,
and tests accompany the binaries. Upstream code is not presented as original
work. This project is not affiliated with Kali, Wifite, Reaver or Pixiewps.

WIFITE's scope and differences from upstream CLI behavior are documented in
`WIFITE.md` at the project root, including standard versus full seed searches
and the lack of passive capture import or custom CLI seed windows.

# ESP-IDF WPS transport adapter (0.5)

The generated WPS translation unit is based on Espressif esp_wps.c from ESP-IDF
v5.4.1, commit 4c2820d377d1375e787bcef612f0c32c1427d183 (Apache-2.0).
It retains the upstream initialization, task, discovery and association code.
The WirelessPi adapter adds selected-BSSID registrar transport and isolates its
EAP worker from the enrollee engine. This is not a port of Wifite's Python CLI
or the complete Reaver executable. The pre-existing WPS message core remains
independently written; its synthetic tests are not hardware validation.

# Upstream comparison for 0.6

Reviewed kimocoder/wifite2 wifite/attack/wps.py, blob SHA
87481fcb63efd74d73ac26c0dc5e0291973a3087. It orchestrates external Reaver/Bully.
Reviewed t6x/reaver-wps-fork-t6x src/libwps/libwps.c, blob SHA
8c1a83eb4ebaf73fe2e6c3c2d2278cda3323668e: lock status comes from the distinct
AP Setup Locked attribute, with an unspecified state when absent. ESP-IDF's
wps_defs.h defines AP Setup Locked as 0x1057 and Setup Locked error as 15.
Our parser already used these values. No Reaver or Wifite code was copied in
0.6; the confirmation/wait policy and default-PIN-first workflow are custom
embedded behavior. No claim of complete feature parity is made.
