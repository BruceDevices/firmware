// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_protocol_engine.h"
#include "rf_codec.h"
#include "rf_decode_limits.h"
#include "rf_presets.h"
#include "rf_registry.h"
#include "rf_registry_ext.h"
#include <ArduinoJson.h>
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <climits>
#include <map>
#include <sstream>
#include <string>
#include <esp_heap_caps.h>
#if !defined(LITE_VERSION)
#include "rf_stream.h"
#endif

namespace {
using Fields = std::map<std::string, std::string>;
constexpr size_t MaxInput = 262144, MaxPulses = 16384;
constexpr uint64_t MaxTransmitDuration = 10000000;

String jsonString(const JsonDocument &doc) {
    std::string text;
    serializeJson(doc, text);
    return text.c_str();
}
String failure(const char *message) {
    JsonDocument doc;
    doc["ok"] = false;
    doc["error"] = message;
    return jsonString(doc);
}
std::string trim(const std::string &s) {
    const auto start = s.find_first_not_of(" \t\r");
    return start == std::string::npos ? "" : s.substr(start, s.find_last_not_of(" \t\r") - start + 1);
}
bool unsignedValue(const std::string &s, uint64_t &out, uint64_t limit, int base = 10) {
    if (s.empty()) return false;
    for (char c : s)
        if (!(base == 16 ? isxdigit((unsigned char)c) : isdigit((unsigned char)c))) return false;
    errno = 0;
    char *end;
    out = strtoull(s.c_str(), &end, base);
    return !errno && !*end && out <= limit;
}
bool parse(
    const char *input, Fields &fields, std::vector<int> *pulses = nullptr,
    size_t maxInput = MaxInput, size_t maxPulses = MaxPulses, const char **error = nullptr
) {
    auto reject = [&](const char *message) {
        if (error) *error = message;
        return false;
    };
    if (!input || strnlen(input, maxInput + 1) > maxInput)
        return reject("SubGhz file exceeds import size limit");
    std::istringstream stream(input);
    std::string line;
    uint64_t total = 0;
    while (std::getline(stream, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        // Older Bruce captures omitted the colon in this one header field.
        // Accept those saved files without weakening other field validation.
        if (line == "Version 1") {
            const auto filetype = fields.find("Filetype");
            if (filetype != fields.end() && filetype->second == "Bruce SubGhz File")
                line = "Version: 1";
        }
        auto colon = line.find(':');
        if (colon == std::string::npos) return false;
        const auto key = trim(line.substr(0, colon)), value = trim(line.substr(colon + 1));
        if (key.empty() || key.size() > 128 || fields.size() >= 256) return false;
        if (key == "RAW_Data" || key == "Data_RAW") {
            if (!pulses) continue;
            std::istringstream values(value);
            std::string token;
            while (values >> token) {
                errno = 0;
                char *end;
                long duration = strtol(token.c_str(), &end, 10);
                if (errno || *end || end == token.c_str() || !duration || duration < -1000000 ||
                    duration > 1000000)
                    return reject("Invalid RAW pulse duration");
                if (pulses->size() >= maxPulses) return reject("RAW capture exceeds pulse limit");
                total += duration < 0 ? -duration : duration;
                if (total > RF_MAX_CAPTURE_DURATION_US) return reject("RAW capture exceeds 60 seconds");
                pulses->push_back((int)duration);
            }
        } else if (!fields.emplace(key, value).second) return false;
    }
    uint64_t frequency;
    if (!unsignedValue(fields["Frequency"], frequency, UINT32_MAX)) return false;
    if (!((frequency >= 300000000 && frequency <= 348000000) ||
          (frequency >= 387000000 && frequency <= 464000000) ||
          (frequency >= 779000000 && frequency <= 928000000)))
        return false;
    return !fields["Preset"].empty() && !fields["Protocol"].empty();
}

bool nativeName(const String &name) {
    return rf_find_protocol(name) || name == "KeeLoq" || name == "Chamberlain_9bit";
}
bool nativeFields(const Fields &f, RfCodes &out) {
    auto get = [&](const char *key) {
        auto it = f.find(key);
        return it == f.end() ? std::string() : it->second;
    };
    if (!nativeName(get("Protocol").c_str())) return false;
    if (const auto *def = rf_find_protocol(get("Protocol").c_str()))
        if (def->stream) return false;
    uint64_t value;
    std::string key;
    for (char c : get("Key"))
        if (!isspace((unsigned char)c)) key += c;
    if (!unsignedValue(key, value, UINT64_MAX, 16) || key.size() > 16) return false;
    out.key = value;
    if (!unsignedValue(get("Bit"), value, 255) || !value) return false;
    out.Bit = (int)value;
    if (!unsignedValue(get("Frequency"), value, UINT32_MAX)) return false;
    out.frequency = value;
    out.protocol = get("Protocol").c_str();
    if (const auto *def = rf_find_protocol(out.protocol)) out.protocol = def->name;
    out.preset = get("Preset").c_str();
    if (!rf_find_preset(out.preset)) return false;
    struct Number {
        const char *name;
        uint64_t max;
        int base;
    };
    for (const auto &n : {
             Number{"Serial",  UINT32_MAX, 16},
             {"Button",  255,        10},
             {"Counter", 65535,      10},
             {"Seed",    UINT32_MAX, 16},
             {"TE",      1000000,    10},
             {"Fix",     UINT32_MAX, 16},
             {"Hop",     UINT32_MAX, 16}
    }) {
        auto it = f.find(n.name);
        if (it == f.end()) continue;
        auto text = it->second;
        if (n.base == 16) {
            text.clear();
            for (char c : it->second)
                if (!isspace((unsigned char)c)) text += c;
        }
        if (!unsignedValue(text, value, n.max, n.base)) return false;
        if (!strcmp(n.name, "Serial")) out.serial = value;
        else if (!strcmp(n.name, "Button")) out.btn = value;
        else if (!strcmp(n.name, "Counter")) out.cnt = value;
        else if (!strcmp(n.name, "Seed")) out.seed = value;
        else if (!strcmp(n.name, "TE")) out.te = value;
        else if (!strcmp(n.name, "Fix")) out.fix = value;
        else if (!strcmp(n.name, "Hop")) out.hop = value;
    }
    auto manufacturer = get("Manufacture");
    if (manufacturer.empty()) manufacturer = get("Manufacturer");
    if (!manufacturer.empty()) out.mf_name = manufacturer.c_str();
    out.encoderAvailable = rf_protocol_has_encoder(out.protocol);
    return true;
}
String rawFile(const std::vector<int> &pulses, uint32_t frequency, const String &preset) {
    std::ostringstream text;
    text << "Filetype: Flipper SubGhz RAW File\nVersion: 1\nFrequency: " << frequency
         << "\nPreset: " << preset.c_str() << "\nProtocol: RAW\n";
    for (size_t i = 0; i < pulses.size(); i++) {
        if (i % 512 == 0) text << (i ? "\n" : "") << "RAW_Data:";
        text << ' ' << pulses[i];
    }
    text << '\n';
    return text.str().c_str();
}
bool validPulses(
    const std::vector<int> &pulses, uint64_t maxDuration = MaxTransmitDuration, size_t maxPulses = MaxPulses
) {
    if (pulses.empty() || pulses.size() > maxPulses) return false;
    uint64_t sum = 0;
    for (int p : pulses) {
        if (!p || p < -1000000 || p > 1000000) return false;
        sum += p < 0 ? -p : p;
        if (sum > maxDuration) return false;
    }
    return true;
}
bool nativeDecode(const std::vector<int> &pulses, uint32_t frequency, const String &preset, RfCodes &out) {
    RfCodes result;
    if (!rf_decode_keeloq(pulses, result)) {
        result = RfCodes{};
        if (!rf_decode_ook(pulses, result, true)) return false;
    }
    result.frequency = frequency;
    // Keep the profile used by the receiver, not a decoder's nominal default.
    result.preset = preset;
    result.encoderAvailable = rf_protocol_has_encoder(result.protocol);
    out = result;
    return true;
}
void addNativeFrame(JsonArray frames, const RfCodes &code, const char *original = nullptr) {
    JsonObject frame = frames.add<JsonObject>();
    frame["protocol"] = std::string(code.protocol.c_str());
    frame["encoder"] = rf_protocol_has_encoder(code.protocol);
    frame["validation"] = "not-verified";
    // Beta encoders replay Key (CAME Twee uses Serial); editing decoded
    // Button/Counter alone would not regenerate a corresponding payload.
    frame["fieldEditing"] = false;
    frame["data"] = std::string(original ? original : rf_protocol_serialize(code).c_str());
    frame["text"] = std::string(rf_protocol_serialize(code).c_str());
}
bool fromFrame(JsonObjectConst frame, RfCodes &out) {
    const char *data = frame["data"] | "";
    Fields fields;
    if (!parse(data, fields)) return false;
    RfCodes result;
    const auto *definition = rf_find_protocol(frame["protocol"] | "");
    if (nativeName(frame["protocol"] | "") && !(definition && definition->stream)) {
        if (!nativeFields(fields, result)) return false;
    } else {
        // Wide payload/counter and protocol-specific fields are never narrowed.
        result.protocol = frame["protocol"] | "";
        result.preset = fields["Preset"].c_str();
        result.frequency = strtoul(fields["Frequency"].c_str(), nullptr, 10);
        result.Bit = atoi(fields["Bit"].c_str());
        result.decodedFile = data;
        result.decodedText = frame["text"] | "";
        result.encoderAvailable = (frame["encoder"] | false) && strcmp(frame["validation"] | "", "failed");
    }
    out = result;
    return true;
}
} // namespace

bool rf_protocol_has_encoder(const String &name) {
    if (name == "KeeLoq") return true;
    if (const auto *def = rf_find_protocol(name))
        return def->decode ? def->encode != nullptr
                           : def->zero.high && def->zero.low && def->one.high && def->one.low;
    return false; // Extended vehicle capability is returned by its decoder/catalogue.
}

String rf_protocol_catalog_json() {
    JsonDocument doc;
    doc["ok"] = true;
    doc["api"] = 1;
    doc["version"] = "4.0-bruce.2-native";
    doc["emulationEnabled"] = true;
    doc["keyStoreLoaded"] = false;
    doc["engine"] = "Bruce native RF";
    JsonArray protocols = doc["protocols"].to<JsonArray>();
    auto add = [&](const char *name) {
        JsonObject item = protocols.add<JsonObject>();
        item["name"] = name;
        item["encoder"] = rf_protocol_has_encoder(name);
        const auto *def = rf_find_protocol(name);
        item["source"] = def && def->stream ? "ProtoPirate" : "Bruce beta";
    };
    for (int i = 0; i < rf_protocol_count(); i++) add(rf_protocol_at(i)->name);
    for (int i = 0; i < rf_protocol_ext_count(); i++) add(rf_protocol_ext_at(i)->name);
    add("KeeLoq");
    add("Chamberlain_9bit");
    return doc.overflowed() ? failure("Not enough memory") : jsonString(doc);
}

String rf_protocol_serialize(const RfCodes &code) {
    if (!code.decodedFile.isEmpty()) return code.decodedFile;
    char key[24], serial[16], seed[16], fix[16], hop[16];
    snprintf(key, sizeof(key), "%016llX", (unsigned long long)code.key);
    snprintf(serial, sizeof(serial), "%08lX", (unsigned long)code.serial);
    snprintf(seed, sizeof(seed), "%08lX", (unsigned long)code.seed);
    snprintf(fix, sizeof(fix), "%08lX", (unsigned long)code.fix);
    snprintf(hop, sizeof(hop), "%08lX", (unsigned long)code.hop);
    std::ostringstream out;
    out << "Filetype: Flipper SubGhz Key File\nVersion: 1\nFrequency: " << code.frequency
        << "\nPreset: " << code.preset.c_str()
        << "\nProtocol: " << rf_flipper_protocol_name(code.protocol).c_str() << "\nBit: " << code.Bit
        << "\nKey:";
    for (int i = 0; i < 16; i += 2) out << ' ' << key[i] << key[i + 1];
    out << "\nTE: " << code.te << "\nSerial: " << serial << "\nButton: " << unsigned(code.btn)
        << "\nCounter: " << code.cnt << "\nManufacture: " << code.mf_name.c_str() << "\nSeed: " << seed
        << "\nFix: " << fix << "\nHop: " << hop << '\n';
    return out.str().c_str();
}

String rf_protocol_decode_json(const char *input) {
    if (!input) return failure("Missing SubGhz file");
    const size_t inputSize = strnlen(input, RF_DECODE_MAX_FILE_BYTES + 1);
    if (inputSize > RF_DECODE_MAX_FILE_BYTES) return failure("SubGhz file exceeds 1 MiB import limit");
    // The text parser and pulse normalization use temporary buffers. Check
    // headroom before larger imports so boards without enough RAM fail cleanly.
    if (inputSize > 65536 &&
        (heap_caps_get_free_size(MALLOC_CAP_8BIT) <
             inputSize * 4 + RF_DECODE_MAX_PULSES * sizeof(int) * 2 + 131072 ||
         heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) <
             std::max(inputSize + 1, size_t(RF_DECODE_MAX_PULSES) * sizeof(int)) + 65536))
        return failure("Not enough memory for large SubGhz import");
    Fields fields;
    std::vector<int> pulses;
    const char *parseError = "Malformed SubGhz fields";
    if (!parse(input, fields, &pulses, RF_DECODE_MAX_FILE_BYTES, RF_DECODE_MAX_PULSES, &parseError))
        return failure(parseError);
    const auto &name = fields["Protocol"];
    const bool raw = name == "RAW" || name == "BinRAW";
    if (raw && !validPulses(pulses, RF_MAX_CAPTURE_DURATION_US, RF_DECODE_MAX_PULSES))
        return failure("Empty or invalid RAW data");
    const auto *definition = rf_find_protocol(name.c_str());
    JsonDocument doc;
    doc["ok"] = true;
    JsonArray frames = doc["frames"].to<JsonArray>();
#if !defined(LITE_VERSION)
    auto decode = [&](const RfProtocolDef *def) -> const char * {
        std::vector<RfStreamResult> records;
        const char *error = rf_stream_read_frames(def->stream, input, pulses, records);
        if (error) return error;
        for (const auto &record : records) {
            if (frames.size() >= 16) break;
            JsonObject frame = frames.add<JsonObject>();
            frame["protocol"] = def->name;
            frame["encoder"] = record.encoder;
            frame["validation"] = record.invalid ? "failed" : "not-verified";
            frame["fieldEditing"] = true;
            frame["data"] = std::string(record.data.c_str());
            frame["text"] = std::string(record.text.c_str());
        }
        return nullptr;
    };
    if (raw) {
        for (int i = 0; i < rf_protocol_ext_count() && frames.size() < 16; ++i) {
            const auto *def = rf_protocol_ext_at(i);
            if (def->stream)
                if (const char *error = decode(def)) return failure(error);
        }
    } else if (definition && definition->stream) {
        if (const char *error = decode(definition)) return failure(error);
    }
#endif
    if (!raw && nativeName(name.c_str()) && !(definition && definition->stream)) {
        RfCodes code;
        if (!nativeFields(fields, code)) return failure("Invalid native protocol fields");
        addNativeFrame(frames, code, input);
    } else if (raw && frames.size() == 0) {
        RfCodes code;
        if (nativeDecode(
                pulses, strtoul(fields["Frequency"].c_str(), nullptr, 10), fields["Preset"].c_str(), code
            ))
            addNativeFrame(frames, code);
    }
    doc["limitReached"] = frames.size() >= 16;
    doc["serializationError"] = false;
    return doc.overflowed() ? failure("Not enough memory") : jsonString(doc);
}

bool rf_protocol_load(const char *input, RfCodes &out) {
    JsonDocument doc;
    String response = rf_protocol_decode_json(input);
    return !deserializeJson(doc, response.c_str()) && (doc["ok"] | false) && doc["frames"].size() > 0 &&
           fromFrame(doc["frames"][0].as<JsonObjectConst>(), out);
}
bool rf_protocol_decode_capture(
    const std::vector<int> &durations, uint32_t frequency, const String &preset, RfCodes &out
) {
    if (!validPulses(durations, RF_MAX_CAPTURE_DURATION_US)) return false;
    return rf_protocol_load(rawFile(durations, frequency, preset).c_str(), out);
}

bool rf_protocol_encode(const RfCodes &code, int repeat, std::vector<int> &out) {
    out.clear();
    if (repeat < 1 || repeat > 20) return false;
    if (!code.decodedFile.isEmpty()) {
        if (!code.encoderAvailable) return false;
        JsonDocument doc;
        String response = rf_protocol_encode_json(code.decodedFile.c_str());
        Fields fields;
        return !deserializeJson(doc, response.c_str()) && (doc["ok"] | false) &&
               parse(doc["raw"] | "", fields, &out) && validPulses(out);
    }
    std::vector<int> frame;
    if (code.protocol == "KeeLoq") {
        if (!rf_keeloq_durations(code.key, frame)) return false;
    } else {
        const auto *def = rf_find_protocol(code.protocol);
        if (!def || !rf_protocol_has_encoder(code.protocol)) return false;
        if (def->encode) {
            if (!def->encode(code, frame)) return false;
        } else {
            if (code.Bit < 1 || code.Bit > 64 ||
                !rf_encode_protocol(code.key, code.Bit, code.te, def, repeat, out))
                return false;
            return validPulses(out);
        }
    }
    if (frame.size() > MaxPulses / (size_t)repeat) return false;
    for (int i = 0; i < repeat; i++) out.insert(out.end(), frame.begin(), frame.end());
    return validPulses(out);
}
String rf_protocol_encode_json(const char *input) {
    Fields fields;
    if (!input || strlen(input) > 16384 || !parse(input, fields)) return failure("Invalid decoded file");
    const auto *definition = rf_find_protocol(fields["Protocol"].c_str());
#if !defined(LITE_VERSION)
    if (definition && definition->stream) {
        std::vector<int> pulses;
        String updated;
        if (const char *error = rf_stream_write_pulses(definition->stream, input, pulses, updated))
            return failure(error);
        String raw =
            rawFile(pulses, strtoul(fields["Frequency"].c_str(), nullptr, 10), fields["Preset"].c_str());
        // Preserve custom CC1101 register data, exactly as the capture recorded it.
        if (fields.count("Custom_preset_data")) {
            std::string text = raw.c_str();
            const auto at = text.find("RAW_Data:");
            text.insert(
                at, "Custom_preset_module: CC1101\nCustom_preset_data: " + fields["Custom_preset_data"] + "\n"
            );
            raw = text.c_str();
        }
        JsonDocument doc;
        doc["ok"] = true;
        doc["pulses"] = pulses.size();
        doc["raw"] = std::string(raw.c_str());
        doc["updated"] = std::string(updated.c_str());
        return doc.overflowed() ? failure("Not enough memory") : jsonString(doc);
    }
#endif
    if (!nativeName(fields["Protocol"].c_str())) return failure("Protocol unavailable in this build");
    if (fields.count("Bruce_Buffer_Full") && fields["Bruce_Buffer_Full"] != "0")
        return failure("Truncated capture");
    RfCodes code;
    if (!nativeFields(fields, code)) return failure("Invalid native protocol fields");
    uint64_t repeat = 3;
    if (fields.count("Repeat") && (!unsignedValue(fields["Repeat"], repeat, 20) || !repeat))
        return failure("Invalid repeat count");
    std::vector<int> pulses;
    if (!rf_protocol_encode(code, (int)repeat, pulses))
        return failure("No compatible encoder or invalid payload");
    JsonDocument doc;
    doc["ok"] = true;
    doc["pulses"] = pulses.size();
    doc["raw"] = std::string(rawFile(pulses, code.frequency, code.preset).c_str());
    doc["updated"] = input; // Preserve all extra fields; encoding never advances counters implicitly.
    return doc.overflowed() ? failure("Not enough memory") : jsonString(doc);
}
