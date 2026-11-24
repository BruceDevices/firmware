#include "rf_bits_ui.h"

#include "core/display.h"
#include "core/mykeyboard.h"
#include "rf_bits.h"
#include <globals.h>

// A capture is long; the row label only has room for its beginning. The full
// string stays readable through `rfbits show` and is what TX re-modulates.
static String rfBitsLabel(const String &bits, int maxLen) {
    if ((int)bits.length() <= maxLen) return bits;
    return bits.substring(0, maxLen) + "..";
}

void rfBitsEditor() {
    int last = 0;
    while (true) {
        const bool hasCapture = RfBits::loaded();
        std::vector<Option> opts = {
            {hasCapture ? "Bits: " + rfBitsLabel(RfBits::bits(), 16) : String("Bits: (none)"),
             []() {
                 String v = keyboard(RfBits::loaded() ? RfBits::bits() : String("1010"), 240, "Bit string:");
                 if (v == "\x1B") return;
                 RfBits::set(v, 0, 0.0f);
                 // `set` drops everything that is not a 0 or a 1, so an input
                 // like "hello" leaves no capture instead of an empty one that
                 // still claims to be loaded.
                 if (!RfBits::loaded()) displayError("no 0/1 bits in that input", true);
             }},
            {"TE: " + String(RfBits::te()) + " us",
             []() {
                 String v = num_keyboard(String(RfBits::te()), 5, "TE (us):");
                 if (v == "\x1B") return;
                 const int te = v.toInt();
                 if (te < 50 || te > 5000) {
                     displayError("TE must be 50..5000 us", true);
                     return;
                 }
                 RfBits::setTe(te);
             }},
            {"Repeat: " + String(RfBits::repeat()) + "x",
             []() {
                 String v = num_keyboard(String(RfBits::repeat()), 2, "Repeat:");
                 if (v == "\x1B") return;
                 if (v.toInt() < 1 || v.toInt() > 40) {
                     displayError("repeat must be 1..40", true);
                     return;
                 }
                 RfBits::setRepeat(v.toInt());
             }},
            {"Flip bit",
             []() {
                 if (!RfBits::loaded()) {
                     displayError("no capture loaded", true);
                     return;
                 }
                 String v = num_keyboard("0", 4, "Bit index:");
                 if (v == "\x1B") return;
                 const int index = v.toInt();
                 if (!RfBits::flip(index)) {
                     displayError("bit " + String(index) + " out of range (0.." + String(RfBits::bits().length() - 1) +
                                      ")",
                                  true);
                     return;
                 }
                 displayInfo("bit " + String(index) + " flipped", true);
             }},
            {"Trim preamble",
             []() {
                 if (!RfBits::loaded()) {
                     displayError("no capture loaded", true);
                     return;
                 }
                 String v = num_keyboard("1", 3, "Trim bits:");
                 if (v == "\x1B") return;
                 const int count = v.toInt();
                 if (!RfBits::trimLeading(count)) {
                     displayError("trim " + String(count) + " out of range (1.." +
                                      String((int)RfBits::bits().length() - 1) + ")",
                                  true);
                     return;
                 }
                 displayInfo("trimmed " + String(count) + ", " + String(RfBits::bits().length()) + " left", true);
             }},
            {"Stretch 10%",
             []() {
                 if (!RfBits::scale(10, true)) displayError("no capture loaded", true);
                 else displayInfo("TE " + String(RfBits::te()) + " us", true);
             }},
            {"Shorten 10%",
             []() {
                 if (!RfBits::scale(10, false)) displayError("no capture loaded", true);
                 else displayInfo("TE " + String(RfBits::te()) + " us", true);
             }},
            {"Transmit",
             []() {
                 String err;
                 if (RfBits::tx(err)) displayInfo(err, true);
                 else displayError(err, true);
             }},
            {"Back", []() {}},
        };

        const int selected = loopOptions(opts, MENU_TYPE_SUBMENU, "Bit Editor", last);
        if (selected == -1 || selected == (int)opts.size() - 1) return;
        last = selected;
        // The menu rebuilds so the labels show the value that was just set.
    }
}
