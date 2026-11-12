#ifndef RF_REPEATER_H
#define RF_REPEATER_H

// Native capture-and-replay app: listens for an OOK/PPM burst, shows a quick
// decode attempt (named protocol / KeeLoq / raw), then lets the user replay
// it (once or a configured repeat count), save it to SD, or discard it.
// Settings (SEL): frequency, RX bandwidth, AGC, repeat count/delay, and
// auto-delete-after-transmit.
void rf_repeater();

#endif
