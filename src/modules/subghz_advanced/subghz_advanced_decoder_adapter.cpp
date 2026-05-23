#include "subghz_advanced_decoder_adapter.h"

#include "subghz_advanced_subfile_codec.h"

#include <ctype.h>
#include <stdlib.h>
#include <vector>

#include <subghz/environment.h>
#include <subghz/receiver.h>

#include <subghz/protocols/alutech_at_4n.h>
#include <subghz/protocols/ansonic.h>
#include <subghz/protocols/beninca_arc.h>
#include <subghz/protocols/bett.h>
#include <subghz/protocols/came.h>
#include <subghz/protocols/came_atomo.h>
#include <subghz/protocols/came_twee.h>
#include <subghz/protocols/chamberlain_code.h>
#include <subghz/protocols/clemsa.h>
#include <subghz/protocols/dickert_mahs.h>
#include <subghz/protocols/ditec_gol4.h>
#include <subghz/protocols/doitrand.h>
#include <subghz/protocols/dooya.h>
#include <subghz/protocols/elplast.h>
#include <subghz/protocols/faac_slh.h>
#include <subghz/protocols/feron.h>
#include <subghz/protocols/gangqi.h>
#include <subghz/protocols/gate_tx.h>
#include <subghz/protocols/hay21.h>
#include <subghz/protocols/hollarm.h>
#include <subghz/protocols/holtek.h>
#include <subghz/protocols/holtek_ht12x.h>
#include <subghz/protocols/honeywell.h>
#include <subghz/protocols/honeywell_wdb.h>
#include <subghz/protocols/hormann.h>
#include <subghz/protocols/ido.h>
#include <subghz/protocols/intertechno_v3.h>
#include <subghz/protocols/jarolift.h>
#include <subghz/protocols/keeloq.h>
#include <subghz/protocols/kinggates_stylo_4k.h>
#include <subghz/protocols/legrand.h>
#include <subghz/protocols/linear.h>
#include <subghz/protocols/linear_delta3.h>
#include <subghz/protocols/magellan.h>
#include <subghz/protocols/marantec.h>
#include <subghz/protocols/marantec24.h>
#include <subghz/protocols/mastercode.h>
#include <subghz/protocols/megacode.h>
#include <subghz/protocols/nero_radio.h>
#include <subghz/protocols/nero_sketch.h>
#include <subghz/protocols/nice_flo.h>
#include <subghz/protocols/nice_flor_s.h>
#include <subghz/protocols/phoenix_v2.h>
#include <subghz/protocols/power_smart.h>
#include <subghz/protocols/princeton.h>
#include <subghz/protocols/revers_rb2.h>
#include <subghz/protocols/roger.h>
#include <subghz/protocols/secplus_v1.h>
#include <subghz/protocols/secplus_v2.h>
#include <subghz/protocols/smc5326.h>
#include <subghz/protocols/somfy_keytis.h>
#include <subghz/protocols/somfy_telis.h>
#include <subghz/protocols/treadmill37.h>

static const SubGhzProtocol* const kCoreProtocols[] = {
    &subghz_protocol_keeloq,      &subghz_protocol_came,      &subghz_protocol_came_twee,
    &subghz_protocol_came_atomo,  &subghz_protocol_nice_flo,  &subghz_protocol_nice_flor_s,
    &subghz_protocol_faac_slh,    &subghz_protocol_princeton, &subghz_protocol_linear,
    &subghz_protocol_linear_delta3,
    &subghz_protocol_secplus_v1, &subghz_protocol_secplus_v2, &subghz_protocol_holtek,
    &subghz_protocol_holtek_th12x, &subghz_protocol_doitrand, &subghz_protocol_megacode,
    &subghz_protocol_power_smart,
};

static const SubGhzProtocolRegistry kCoreRegistry = {
    .items = kCoreProtocols,
    .size = sizeof(kCoreProtocols) / sizeof(kCoreProtocols[0]),
};

static const SubGhzProtocol* const kFullProtocols[] = {
    &subghz_protocol_gate_tx,     &subghz_protocol_keeloq,      &subghz_protocol_nice_flo,
    &subghz_protocol_came,        &subghz_protocol_faac_slh,    &subghz_protocol_nice_flor_s,
    &subghz_protocol_came_twee,   &subghz_protocol_came_atomo,  &subghz_protocol_nero_sketch,
    &subghz_protocol_ido,         &subghz_protocol_hormann,     &subghz_protocol_nero_radio,
    &subghz_protocol_somfy_telis, &subghz_protocol_somfy_keytis, &subghz_protocol_princeton,
    &subghz_protocol_linear,      &subghz_protocol_secplus_v2,  &subghz_protocol_secplus_v1,
    &subghz_protocol_megacode,    &subghz_protocol_holtek,      &subghz_protocol_chamb_code,
    &subghz_protocol_power_smart, &subghz_protocol_marantec,    &subghz_protocol_bett,
    &subghz_protocol_doitrand,    &subghz_protocol_phoenix_v2,  &subghz_protocol_honeywell_wdb,
    &subghz_protocol_magellan,    &subghz_protocol_intertechno_v3,
    &subghz_protocol_clemsa,      &subghz_protocol_ansonic,     &subghz_protocol_smc5326,
    &subghz_protocol_holtek_th12x, &subghz_protocol_linear_delta3,
    &subghz_protocol_dooya,       &subghz_protocol_alutech_at_4n,
    &subghz_protocol_kinggates_stylo_4k,
    &subghz_protocol_mastercode, &subghz_protocol_honeywell, &subghz_protocol_legrand,
    &subghz_protocol_dickert_mahs, &subghz_protocol_gangqi,   &subghz_protocol_marantec24,
    &subghz_protocol_hollarm,    &subghz_protocol_hay21,      &subghz_protocol_revers_rb2,
    &subghz_protocol_feron,      &subghz_protocol_roger,      &subghz_protocol_elplast,
    &subghz_protocol_treadmill37, &subghz_protocol_beninca_arc, &subghz_protocol_jarolift,
    &subghz_protocol_ditec_gol4,
};

static const SubGhzProtocolRegistry kFullRegistry = {
    .items = kFullProtocols,
    .size = sizeof(kFullProtocols) / sizeof(kFullProtocols[0]),
};

static bool splitLineKV(const String& line, String& key, String& value) {
    const int idx = line.indexOf(':');
    if(idx <= 0) return false;
    key = line.substring(0, idx);
    value = line.substring(idx + 1);
    key.trim();
    value.trim();
    return key.length() > 0;
}

static String normalizeHexLike(const String& in) {
    String out = in;
    out.trim();
    if(out.startsWith("0x") || out.startsWith("0X")) out = out.substring(2);
    while(out.length() && (out[0] == ':' || out[0] == '=')) out.remove(0, 1);

    String filtered;
    filtered.reserve(out.length());
    for(size_t i = 0; i < out.length(); i++) {
        const char c = out[i];
        if(isxdigit((unsigned char)c)) filtered += c;
        else if(c == ' ') break;
        else if(c == '\t') break;
        else if(c == ';') break;
        else if(c == ',') break;
        else if(c == ')') break;
    }
    filtered.toUpperCase();
    return filtered;
}

static String extractTokenAfter(const String& line, const char* marker) {
    const int start = line.indexOf(marker);
    if(start < 0) return "";
    int pos = start + strlen(marker);
    while(pos < (int)line.length() && isspace((unsigned char)line[pos])) pos++;
    if(pos >= (int)line.length()) return "";

    int end = pos;
    while(end < (int)line.length() && !isspace((unsigned char)line[end])) end++;
    return line.substring(pos, end);
}

static int extractBitCountFromLine(const String& line) {
    const int bit_idx = line.indexOf("bit");
    if(bit_idx <= 0) return 0;

    int i = bit_idx - 1;
    while(i >= 0 && isspace((unsigned char)line[i])) i--;
    int end = i;
    while(i >= 0 && isdigit((unsigned char)line[i])) i--;
    if(end < 0 || end < i + 1) return 0;
    return line.substring(i + 1, end + 1).toInt();
}

static void parseDecoderString(String text, SubGhzAdvancedFrame& frame) {
    text.replace("\r", "");
    int line_start = 0;
    bool first_line = true;

    while(line_start <= (int)text.length()) {
        int line_end = text.indexOf('\n', line_start);
        if(line_end < 0) line_end = text.length();
        String line = text.substring(line_start, line_end);
        line.trim();

        if(line.length()) {
            if(first_line && frame.bit_count <= 0) {
                frame.bit_count = extractBitCountFromLine(line);
            }

            if(frame.key_hex.length() == 0 && line.indexOf("Key:") >= 0) {
                frame.key_hex = normalizeHexLike(extractTokenAfter(line, "Key:"));
            }
            if(frame.counter.length() == 0) {
                if(line.indexOf("Cnt:") >= 0) frame.counter = normalizeHexLike(extractTokenAfter(line, "Cnt:"));
                else if(line.indexOf("PsCn:") >= 0)
                    frame.counter = normalizeHexLike(extractTokenAfter(line, "PsCn:"));
            }
            if(frame.button.length() == 0 && line.indexOf("Btn:") >= 0) {
                frame.button = normalizeHexLike(extractTokenAfter(line, "Btn:"));
            }
            if(frame.serial.length() == 0) {
                if(line.indexOf("Sn:") >= 0) frame.serial = normalizeHexLike(extractTokenAfter(line, "Sn:"));
                else if(line.indexOf("Serial:") >= 0)
                    frame.serial = normalizeHexLike(extractTokenAfter(line, "Serial:"));
            }
            if(line.startsWith("MF:")) {
                if(frame.notes.length()) frame.notes += "; ";
                frame.notes += line;
            }
        }

        first_line = false;
        if(line_end == (int)text.length()) break;
        line_start = line_end + 1;
    }
}

static bool parseRawDurations(const String& text, std::vector<int32_t>& out) {
    out.clear();

    int line_start = 0;
    while(line_start <= (int)text.length()) {
        int line_end = text.indexOf('\n', line_start);
        if(line_end < 0) line_end = text.length();

        String line = text.substring(line_start, line_end);
        line.replace("\r", "");
        line.trim();

        String k;
        String v;
        if(splitLineKV(line, k, v) && (k == "RAW_Data" || k == "Data_RAW")) {
            const char* p = v.c_str();
            while(*p) {
                while(*p && isspace((unsigned char)*p)) p++;
                if(!*p) break;

                char* end = nullptr;
                long n = strtol(p, &end, 10);
                if(end == p) break;

                if(n > INT32_MAX) n = INT32_MAX;
                if(n < INT32_MIN) n = INT32_MIN;
                out.push_back((int32_t)n);
                p = end;
            }
        }

        if(line_end == (int)text.length()) break;
        line_start = line_end + 1;
    }

    return !out.empty();
}

static int frameScore(const SubGhzAdvancedFrame& frame) {
    int score = 0;
    if(frame.protocol_name.length() && frame.protocol_name != "Unknown" && frame.protocol_name != "RAW")
        score += 50;
    if(frame.key_hex.length()) score += 20;
    if(frame.counter.length()) score += 12;
    if(frame.button.length()) score += 10;
    if(frame.serial.length()) score += 8;
    if(frame.bit_count > 0) score += frame.bit_count;
    return score;
}

struct DecodeContext {
    SubGhzAdvancedFrame best;
    int best_score = -1;
    uint32_t frequency_hz = 0;
    size_t pulse_count = 0;
};

static void subghzRxCallback(
    SubGhzReceiver* receiver,
    SubGhzProtocolDecoderBase* decoder_base,
    void* context) {
    UNUSED(receiver);
    DecodeContext* ctx = (DecodeContext*)context;
    if(!ctx || !decoder_base || !decoder_base->protocol) return;

    SubGhzAdvancedFrame current;
    current.valid = true;
    current.protocol_name = decoder_base->protocol->name ? decoder_base->protocol->name : "Unknown";
    current.frequency_hz = ctx->frequency_hz;
    current.raw_summary = String((uint32_t)ctx->pulse_count) + " pulses";

    FuriString* s = furi_string_alloc();
    if(s) {
        if(subghz_protocol_decoder_base_get_string(decoder_base, s)) {
            parseDecoderString(String(furi_string_get_cstr(s)), current);
        }
        furi_string_free(s);
    }

    const int score = frameScore(current);
    if(score >= ctx->best_score) {
        ctx->best = current;
        ctx->best_score = score;
    }
}

bool SubGhzAdvancedDecoderAdapter::decodeRawSubText(
    const String& text,
    uint32_t frequency_hz,
    bool full_profile,
    SubGhzAdvancedFrame& frame) {
    std::vector<int32_t> durations;
    if(!parseRawDurations(text, durations)) return false;

    SubGhzEnvironment* env = subghz_environment_alloc();
    if(!env) return false;

    subghz_environment_set_protocol_registry(env, full_profile ? &kFullRegistry : &kCoreRegistry);
    (void)subghz_environment_load_keystore(env, "/mfcodes");

    SubGhzReceiver* rx = subghz_receiver_alloc_init(env);
    if(!rx) {
        subghz_environment_free(env);
        return false;
    }

    DecodeContext ctx;
    ctx.frequency_hz = frequency_hz;
    ctx.pulse_count = durations.size();

    subghz_receiver_set_rx_callback(rx, subghzRxCallback, &ctx);
    subghz_receiver_set_filter(rx, (SubGhzProtocolFlag)0xFFFFFFFFu);

    for(const int32_t d : durations) {
        if(d == 0) continue;
        const bool level = d > 0;
        uint32_t duration = (uint32_t)labs((long)d);
        if(duration == 0 || duration > 2000000UL) continue;
        subghz_receiver_decode(rx, level, duration);
    }

    subghz_receiver_free(rx);
    subghz_environment_free(env);

    if(ctx.best_score < 0 || !ctx.best.valid) return false;
    frame = ctx.best;
    return true;
}

SubGhzAdvancedFrame SubGhzAdvancedDecoderAdapter::decodeBruceCapture(
    const String& capture,
    const String& source,
    bool full_profile) {
    SubGhzAdvancedFrame parsed = SubGhzAdvancedSubFileCodec::parse(capture, source);
    SubGhzAdvancedFrame decoded;

    if(decodeRawSubText(capture, parsed.frequency_hz, full_profile, decoded)) {
        decoded.source = source;
        if(decoded.frequency_hz == 0) decoded.frequency_hz = parsed.frequency_hz;
        if(decoded.bit_count == 0) decoded.bit_count = parsed.bit_count;
        if(decoded.key_hex.length() == 0) decoded.key_hex = parsed.key_hex;
        if(decoded.counter.length() == 0) decoded.counter = parsed.counter;
        if(decoded.button.length() == 0) decoded.button = parsed.button;
        if(decoded.serial.length() == 0) decoded.serial = parsed.serial;
        if(decoded.raw_summary.length() == 0) decoded.raw_summary = parsed.raw_summary;
        if(parsed.notes.length()) {
            if(decoded.notes.length()) decoded.notes += "; ";
            decoded.notes += parsed.notes;
        }
        decoded.filetype = parsed.filetype;
        decoded.valid = true;
        return decoded;
    }

    return parsed;
}
