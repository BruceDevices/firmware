// Native SubGHZ app: a Flipper-Unleashed-style umbrella menu (Read / Read RAW
// / Saved / Add Manually / Frequency Analyzer / Radio Settings) built almost
// entirely on top of Bruce's existing RF primitives (RFScan, readSubFile/
// rfSaveSignal/loopEmulate, the OOK registry + KeeLoq cipher) rather than
// duplicating them -- see rf_subghz.cpp's top comment for the map of what's
// reused vs. genuinely new.
#pragma once
void rf_subghz();
