// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef __RF_REMOTE_ANALYZER_H__
#define __RF_REMOTE_ANALYZER_H__

// "Remote analyzer" (ported from ProtoPirate's RemoteAnalyzer worker): sweeps
// the sub-GHz band for the strongest carrier, pinpoints it with a fine sweep,
// and classifies it as AM (ASK/OOK, steep peak) vs FM/unknown (broad peak).
// CC1101 required.
void rf_remote_analyzer();

#endif
