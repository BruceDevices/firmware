// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

// Saved RAW recordings can contain many packets plus idle/noise pulses.
// These are import limits only; encoder, transmitter and live capture limits
// remain independent. Keep the C and C++ readers in agreement.
#define RF_DECODE_MAX_FILE_BYTES 1048576U
#define RF_DECODE_MAX_PULSES 131072U
