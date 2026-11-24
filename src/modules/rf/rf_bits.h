#ifndef __RF_BITS_H__
#define __RF_BITS_H__

#include <Arduino.h>

// Task 2: sub-GHz bit-level editor and de Bruijn forcer.
//
// A capture is held as a bit string, not as RfCodes, because that is what the
// edit verbs operate on: flip one bit, drop the preamble, stretch the symbol
// timing, change the repeat count. `tx` re-modulates it as PWM OOK (+te/-3te
// for 0, +3te/-te for 1), which every classic fixed-code receiver accepts.
namespace RfBits {

bool load(const String &path, String &err);
void set(const String &bits, int te, float freqMHz);
String show();

bool flip(int index);       // 0 = leftmost bit
bool trimLeading(int count);
bool scale(int percent, bool grow);
void setRepeat(int repeat);
void setTe(int te);

bool tx(String &err);

// de Bruijn B(2, n): every n-bit word appears exactly once in 2^n bits.
String deBruijn(int n);

// Transmit a sequence in chunks so the RSSI watchdog can run between them.
// Returns a human summary; stops early when the RSSI rises above the floor.
String force(const String &bits, int te, float freqMHz, int watchWindowMs);

bool busy();
bool loaded(); // a capture (loaded or hand-typed) is in the editor
const String &bits();
int te();
int repeat();

} // namespace RfBits

#endif
