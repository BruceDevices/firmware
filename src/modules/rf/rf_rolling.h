// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef __RF_ROLLING_H__
#define __RF_ROLLING_H__

// Rolling-code analyzer. Captures KeeLoq (and other decodable) frames, extracts
// the manufacturer / serial / button / counter, and tracks the counter across
// successive presses to show whether the remote is rolling (counter advances)
// or fixed (counter unchanged). CC1101 required.
void rf_rolling_code();

#endif
