// SPDX-FileCopyrightText: 2026 bmorcelli
//
// SPDX-License-Identifier: MIT
//
// ESP32 Devkit
//
// Launcher envs: headless-esp32-4mb, headless-esp32-8mb, headless-esp32s3-16mb, headless-esp32s3-4mb, headless-esp32s3-8mb

#ifndef DISPLAY_DEVICE_HEADLESS_ESP32_4MB_H
#define DISPLAY_DEVICE_HEADLESS_ESP32_4MB_H

// These are plain defines: pass the same macro as a build flag and the
// compiler will report a redefinition. Edit the profile instead.
// clang-format off

// The panel is described unrotated: TFT_ROTATION is the mounting offset
// baked into the driver, and a profile has no business guessing which way
// round you hold the board. Rotate at runtime with tft.setRotation(n).
#define TFT_ROTATION 0

// No panel at all. Every drawing call is a no-op; see devices/README.md.
#define USE_DUMMY_TFT 1

// clang-format on

#endif // DISPLAY_DEVICE_HEADLESS_ESP32_4MB_H

// Default 6x8 font metrics required by Bruce's shared menu layout.
#define LW 6
#define LH 8
