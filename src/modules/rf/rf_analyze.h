// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef __RF_ANALYZE_H__
#define __RF_ANALYZE_H__

// "Analyze" feature: captures a sub-GHz signal, auto-detects its modulation
// (ASK/OOK vs 2-FSK) and symbol timing, then runs the protocol decoder to
// suggest what it is. CC1101 required.
void rf_analyze();

#endif
