// SPDX-License-Identifier: GPL-3.0-or-later
//
// RTL433 — rtl_433 sensor decoder for the native RF menu, ported from the
// Flipper Zero firmware RTL433 app (applications_user/rtl433) to Bruce.
// The full rtl_433 decoder catalogue lives in lib/RTL433; this module wires
// Bruce's CC1101/GPIO receive path to that engine and renders results with
// the native capture-list UI (same presentation as ProtoPirate).
#pragma once
#if !defined(LITE_VERSION) && !defined(RTL433_DISABLED)
void rf_rtl433();
#endif
