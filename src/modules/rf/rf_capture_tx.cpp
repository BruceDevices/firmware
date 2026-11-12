#include "rf_capture_tx.h"
#include "protocols/rf_decoder.h"
#include "protocols/rf_encoder.h"
#include "protocols/rf_registry.h"
#include "rf_send.h" // rfTransmitRawTimings
#include "rf_utils.h"

String rf_identify_capture(const std::vector<int> &durations, RfMatchKind &kind, RfCodes &outCode) {
    kind = RfMatchKind::NONE;
    if (rf_decode_ook(durations, outCode)) {
        kind = RfMatchKind::OOK;
        return outCode.protocol + " (" + String((unsigned long)(outCode.key >> 32), HEX) +
               String((unsigned long)(outCode.key & 0xFFFFFFFFu), HEX) + ")";
    }
    if (rf_decode_keeloq(durations, outCode)) {
        kind = RfMatchKind::KEELOQ;
        return "KeeLoq (rolling code)";
    }
    return "RAW capture, " + String(durations.size()) + " edges";
}

bool rf_transmit_clean_ook(const RfCodes &code, float freq, int repeat) {
    const RfProtocolDef *def = rf_find_protocol(code.protocol);
    if (!def) return false;
    if (!initRfModule("tx", freq)) return false;
    bool ok = rf_tx_protocol(code.key, code.Bit, code.te, def, repeat);
    deinitRfModule();
    return ok;
}

bool rf_transmit_clean_keeloq(uint64_t key, float freq, int repeat) {
    if (!initRfModule("tx", freq)) return false;
    bool ok = rf_tx_keeloq(key, repeat);
    deinitRfModule();
    return ok;
}

// Same zero-terminated signed-duration convention sendRfCommand() builds for
// its "RAW" protocol branch in rf_send.cpp (positive=HIGH, negative=LOW us,
// 0=terminator) -- rfTransmitRawTimings expects exactly this.
bool rf_transmit_raw(const std::vector<int> &durations, float freq) {
    if (durations.empty()) return false;
    int *buf = (int *)calloc(sizeof(int), durations.size() + 1);
    if (!buf) return false;
    for (size_t i = 0; i < durations.size(); i++) buf[i] = durations[i];
    buf[durations.size()] = 0;
    bool ok = initRfModule("tx", freq);
    if (ok) rfTransmitRawTimings(buf);
    free(buf);
    deinitRfModule();
    return ok;
}
