#include "rfbits_commands.h"

#include "modules/rf/rf_bits.h"
#include <globals.h>

// `rfbits` is one command with a mode argument, matching `tune`/`patch`/
// `devflash`: this CLI matches whole command words.
//
// Modes:
//   show                          what is loaded
//   load <file.sub>               from the SD card
//   set <bits> <te> <MHz>         a hand-typed capture
//   flip <bit> | trim <n> | stretch <pct> | shorten <pct> | te <us> | repeat <n>
//   tx                            re-modulate and send
//   debruijn <n> [te] [MHz]       generate B(2,n) and force it
static uint32_t rfbitsCallback(cmd *c) {
    Command cmd(c);
    String mode = cmd.getArgument(0).getValue();
    mode.trim();
    String a = cmd.getArgument(1).getValue();
    String b = cmd.getArgument(2).getValue();
    String d = cmd.getArgument(3).getValue();
    a.trim();
    b.trim();
    d.trim();

    if (mode == "load") {
        String err;
        if (RfBits::load(a, err)) serialDevice->println(RfBits::show());
        else serialDevice->println("ERR: " + err);
        return true;
    }
    if (mode == "set") {
        RfBits::set(a, b.toInt(), d.toFloat());
        serialDevice->println(RfBits::show());
        return true;
    }
    if (mode == "flip") {
        serialDevice->println(RfBits::flip(a.toInt()) ? "OK " + RfBits::bits() : "ERR: bit out of range");
        return true;
    }
    if (mode == "trim") {
        serialDevice->println(RfBits::trimLeading(a.toInt()) ? "OK " + RfBits::bits() : "ERR: trim out of range");
        return true;
    }
    if (mode == "stretch" || mode == "shorten") {
        const bool ok = RfBits::scale(a.toInt(), mode == "stretch");
        serialDevice->println(ok ? "OK te " + String(RfBits::te()) + " us" : "ERR: percentage");
        return true;
    }
    if (mode == "te") {
        RfBits::setTe(a.toInt());
        serialDevice->println("OK te " + String(RfBits::te()) + " us");
        return true;
    }
    if (mode == "repeat") {
        RfBits::setRepeat(a.toInt());
        serialDevice->println("OK repeat " + String(RfBits::repeat()));
        return true;
    }
    if (mode == "tx") {
        String err;
        serialDevice->println(RfBits::tx(err) ? "OK " + err : "ERR: " + err);
        return true;
    }
    if (mode == "debruijn") {
        const int n = a.length() ? a.toInt() : 12;
        String seq = RfBits::deBruijn(n);
        if (seq.length() == 0) {
            serialDevice->println("ERR: n must be 1..16");
            return true;
        }
        const int te = b.length() ? b.toInt() : 350;
        const float mhz = d.length() ? d.toFloat() : 433.92f;
        serialDevice->println("debruijn B(2," + String(n) + ") " + String(seq.length()) + " bits, sending...");
        serialDevice->println(RfBits::force(seq, te, mhz, 250));
        return true;
    }
    if (mode.length() == 0 || mode == "show") {
        serialDevice->println(RfBits::show());
        return true;
    }
    serialDevice->println(
        "ERR: rfbits show|load <file>|set <bits> <te> <MHz>|flip <i>|trim <n>|stretch <pct>|shorten <pct>|"
        "te <us>|repeat <n>|tx|debruijn <n> [te] [MHz]"
    );
    return true;
}

void createRfBitsCommands(SimpleCLI *cli) {
    Command rfbits = cli->addCommand("rfbits", rfbitsCallback);
    rfbits.addPosArg("mode", "show");
    rfbits.addPosArg("a", "");
    rfbits.addPosArg("b", "");
    rfbits.addPosArg("d", "");
}
