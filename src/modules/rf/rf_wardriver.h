#ifndef __RF_WARDRIVER_H
#define __RF_WARDRIVER_H

// SubGHz Wardriver - continuous raw capture of whatever the CC1101 hears.
//
// There is no GPS on this board, so a "wardrive" record is the signal itself:
// every burst that is long enough to be a remote gets its own .sub file under
// /bruce/subghz/wardriver, and an identical replay of the same remote is
// counted but not written twice. The screen shows how many were saved, how
// many duplicates were dropped, and the last signal's statistics.
//
// Settings persist in bruceConfig (RF > Wardriver > Config): frequency, the
// duplicate window and the minimum pulse count that separates a remote from
// line noise.

#include <Arduino.h>

#define RF_WARDRIVER_DIR "/bruce/subghz/wardriver"

// ---------------------------------------------------------------------------
// Mode switch (task 7) - the persisted mode lives in bruceConfig.rfWardriverMode
// (WardriverMode in config.h). These are the values the tuning table seeds; the
// capture loop reads them back by name so `tune set` applies without a rebuild.
// ---------------------------------------------------------------------------
// Dwell per band in Hop. 400 ms covers a whole keyfob press (50-300 ms) on one
// band before moving on, so a press is not cut in half by the hop.
#define WR_HOP_MS_SLOW 400
// Dwell per band in Fast hop. Shorter than most frames, which is why that mode
// re-arms the dwell from the end of a burst instead of from the last hop.
#define WR_HOP_MS_FAST 120
// Widest CC1101 passband. A remote whose carrier is 100-300 kHz off the centre
// still lands inside it, at the cost of collecting far more noise.
#define WR_WIDE_BW_KHZ 812.5f
// Wideband ignores edges shorter than 80 us: the wide filter stretches and
// shreds the slicer edges a narrow capture shows as 100-200 us pulses, so the
// 2 us floor used elsewhere would fill the RMT buffer with chatter.
#define WR_WIDE_MIN_PULSE_NS 80000ULL
// A wide filter rarely shows 30 ms of silence in a busy band, so the burst is
// closed on this cap even if the idle gap never arrives. Bounded files ~2 s.
#define WR_WIDE_HARD_CAP_MS 2000
// Arm the receiver only when the carrier is this strong; below it the wide
// filter is all noise and nothing worth a file is there.
#define WR_WIDE_SQUELCH_DBM -85
// Wide captures are full of sub-80 us edges that a narrow capture does not have,
// so "mostly short pulses" has to move out with the filter.
#define WR_WIDE_SHORT_PULSE_US 80

// Wardriver page: start a capture, browse/replay/identify what was captured,
// and the capture settings.
void rfWardriverMenu();
// `maxSeconds` > 0 stops the capture on its own (console use: `subghz wardriver
// capture 10`); 0 means run until SEL/BACK.
void rfWardriverRun(int maxSeconds = 0);
void rfWardriverConfigMenu();

// Console token for a mode: "hop" / "fasthop" / "wide". Unknown -> "hop".
const char *rfWardriverModeName(uint8_t mode);

// Synthetic check of the duplicate filter and the .sub writer (no radio needed).
// Returns a one-line report ending in PASS or FAIL.
String rfWardriverSelfTest();

// Console helpers: the same code the on-device browser uses.
String rfWardriverListFiles();              // one capture name per line
String rfWardriverIdentify(const String &path); // protocol name, "" if not recognised
bool rfWardriverReplay(const String &path);     // transmit the capture verbatim
String rfWardriverIdentifyAllFiles();           // "ok/total"

#endif
