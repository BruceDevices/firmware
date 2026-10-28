// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct RfStreamCodec RfStreamCodec;

// C lifecycle boundary for stateful pulse codecs. The native RF registry owns
// discovery and dispatch; this layer has no registry, JSON, UI or radio access.
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*RfStreamFrame)(void *, const char *, const char *, bool, bool);
typedef void (*RfStreamEncoded)(void *, const int32_t *, size_t, const char *);
bool rf_stream_has_encoder(const RfStreamCodec *codec);

// Keystore functions
bool rf_stream_keystore_add_key(const char *name, uint64_t key, uint16_t type);
void rf_stream_keystore_raw_blob_set(const uint8_t *data, size_t size);
void rf_stream_keystore_clear_keys(void);
const char *rf_stream_read(
    const RfStreamCodec *codec, const char *record, const int32_t *pulses, size_t count, RfStreamFrame frame,
    void *context
);
const char *
rf_stream_write(const RfStreamCodec *codec, const char *record, RfStreamEncoded encoded, void *context);
#ifdef __cplusplus
}

#include "rf_protocol.h"
#include <cstring>
#include <string>
struct RfStreamResult {
    String data, text;
    bool encoder = false, invalid = false;
};
// These wrappers serialize use of the bounded C allocation arena. A busy
// caller receives an error rather than corrupting another operation's state.
const char *rf_stream_read_frames(
    const RfStreamCodec *, const char *, const std::vector<int> &, std::vector<RfStreamResult> &
);
const char *rf_stream_write_pulses(const RfStreamCodec *, const char *, std::vector<int> &, String &updated);

template <const RfStreamCodec *Codec>
bool rf_stream_decode_callback(const std::vector<int> &pulses, RfCodes &out) {
    const uint32_t frequency = out.frequency ? out.frequency : 433920000;
    const String preset = out.preset.isEmpty() ? String("Ook650Async") : out.preset;
    std::string input =
        "Filetype: Flipper SubGhz RAW File\nVersion: 1\nFrequency: " + std::to_string(frequency) +
        "\nPreset: " + preset.c_str() + "\nProtocol: RAW\n";
    std::vector<RfStreamResult> frames;
    if (rf_stream_read_frames(Codec, input.c_str(), pulses, frames) || frames.empty()) return false;
    out.decodedFile = frames[0].data;
    out.decodedText = frames[0].text;
    out.encoderAvailable = frames[0].encoder;
    out.frequency = frequency;
    out.preset = preset;
    const char *record = out.decodedFile.c_str();
    const char *name = strstr(record, "Protocol: ");
    if (name) {
        name += 10;
        const char *end = strchr(name, '\n');
        out.protocol = std::string(name, end ? end - name : strlen(name)).c_str();
    }
    const char *bits = strstr(record, "Bit: ");
    out.Bit = bits ? atoi(bits + 5) : 0;
    return true;
}
template <const RfStreamCodec *Codec>
bool rf_stream_encode_callback(const RfCodes &in, std::vector<int> &out) {
    String updated;
    return !in.decodedFile.isEmpty() && !rf_stream_write_pulses(Codec, in.decodedFile.c_str(), out, updated);
}
template <const RfStreamCodec *Codec> RfProtocolDef rf_stream_definition(const char *name) {
    return {
        name,
        0,
        {0, 0},
        {0, 0},
        {0, 0},
        0,
        false,
        RF_PF_HAS_DECODER,
        rf_stream_decode_callback<Codec>,
        rf_stream_has_encoder(Codec) ? rf_stream_encode_callback<Codec> : nullptr,
        Codec
    };
}
#endif
