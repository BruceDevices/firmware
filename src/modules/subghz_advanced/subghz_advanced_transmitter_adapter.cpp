#include "subghz_advanced_transmitter_adapter.h"

#include "core/display.h"
#include "modules/rf/rf_utils.h"
#include "subghz_advanced_decoder_adapter.h"

#include <globals.h>

#include <lib/flipper_format/flipper_format.h>
#include <lib/toolbox/level_duration.h>
#include <subghz/environment.h>
#include <subghz/transmitter.h>

static bool splitLineKV(const String& line, String& key, String& value) {
    const int idx = line.indexOf(':');
    if(idx <= 0) return false;
    key = line.substring(0, idx);
    value = line.substring(idx + 1);
    key.trim();
    value.trim();
    return key.length() > 0;
}

static bool upsertField(String& text, const String& field, const String& value) {
    if(field.length() == 0 || value.length() == 0) return false;

    String out = "";
    bool replaced = false;
    int line_start = 0;

    while(line_start <= (int)text.length()) {
        int line_end = text.indexOf('\n', line_start);
        if(line_end < 0) line_end = text.length();

        String raw_line = text.substring(line_start, line_end);
        String line = raw_line;
        line.replace("\r", "");
        line.trim();

        String key;
        String current;
        if(splitLineKV(line, key, current) && key == field) {
            if(!replaced) {
                out += field + ": " + value + "\n";
                replaced = true;
            }
        } else {
            out += raw_line;
            out += "\n";
        }

        if(line_end == (int)text.length()) break;
        line_start = line_end + 1;
    }

    if(!replaced) out += field + ": " + value + "\n";
    text = out;
    return true;
}

static bool findField(const String& text, const String& field, String& out_value) {
    out_value = "";
    int line_start = 0;
    while(line_start <= (int)text.length()) {
        int line_end = text.indexOf('\n', line_start);
        if(line_end < 0) line_end = text.length();

        String line = text.substring(line_start, line_end);
        line.replace("\r", "");
        line.trim();

        String key;
        String value;
        if(splitLineKV(line, key, value) && key == field) {
            out_value = value;
            return true;
        }

        if(line_end == (int)text.length()) break;
        line_start = line_end + 1;
    }
    return false;
}

static bool isLegacyProtocolName(const String& protocol_name) {
    return protocol_name.length() == 0 || protocol_name == "RcSwitch" || protocol_name == "RAW" ||
           protocol_name == "BinRAW";
}

static bool extractEffectiveProtocolName(
    const String& capture,
    String& protocol_name,
    bool* using_detected_protocol = nullptr) {
    if(using_detected_protocol) *using_detected_protocol = false;
    protocol_name = "";
    if(capture.length() == 0) return false;

    String protocol = "";
    (void)findField(capture, "Protocol", protocol);
    protocol.trim();

    if(!isLegacyProtocolName(protocol)) {
        protocol_name = protocol;
        return protocol_name.length() > 0;
    }

    String detected = "";
    if(findField(capture, "Detected_Protocol", detected)) {
        detected.trim();
        if(!isLegacyProtocolName(detected)) {
            protocol_name = detected;
            if(using_detected_protocol) *using_detected_protocol = true;
            return true;
        }
    }

    protocol_name = protocol;
    return protocol_name.length() > 0;
}

static String buildAdvancedTxCapture(const String& capture, const String& effective_protocol_name) {
    String out = capture;
    if(out.length() == 0 || effective_protocol_name.length() == 0) return out;

    // Force protocol to selected advanced decoder/encoder when source file is legacy.
    upsertField(out, "Protocol", effective_protocol_name);

    String key = "";
    String serial = "";
    String button = "";
    String counter = "";
    (void)findField(out, "Key", key);
    (void)findField(out, "Serial", serial);
    (void)findField(out, "Button", button);
    (void)findField(out, "Counter", counter);

    String detected_key = "";
    String detected_serial = "";
    String detected_button = "";
    String detected_counter = "";
    (void)findField(capture, "Detected_Key", detected_key);
    (void)findField(capture, "Detected_Serial", detected_serial);
    (void)findField(capture, "Detected_Button", detected_button);
    (void)findField(capture, "Detected_Counter", detected_counter);

    key.trim();
    serial.trim();
    button.trim();
    counter.trim();
    detected_key.trim();
    detected_serial.trim();
    detected_button.trim();
    detected_counter.trim();

    if(key.length() == 0 && detected_key.length()) upsertField(out, "Key", detected_key);
    if(serial.length() == 0 && detected_serial.length()) upsertField(out, "Serial", detected_serial);
    if(button.length() == 0 && detected_button.length()) upsertField(out, "Button", detected_button);
    if(counter.length() == 0 && detected_counter.length()) upsertField(out, "Counter", detected_counter);

    return out;
}

struct TxPresetConfig {
    uint8_t modulation = 2; // ASK/OOK by default
    float deviation = 1.58f;
    float rx_bw = 270.83f;
    float data_rate = 10.0f;
    bool valid = true;
};

static bool resolvePresetConfig(const String& preset_name, TxPresetConfig& cfg) {
    cfg = TxPresetConfig{};
    String preset = preset_name;
    preset.trim();

    if(preset == "FuriHalSubGhzPresetOok270Async") {
        cfg.modulation = 2;
        cfg.rx_bw = 270;
    } else if(preset == "FuriHalSubGhzPresetOok650Async") {
        cfg.modulation = 2;
        cfg.rx_bw = 650;
    } else if(preset == "FuriHalSubGhzPreset2FSKDev238Async") {
        cfg.modulation = 0;
        cfg.deviation = 2.380371f;
        cfg.rx_bw = 238;
    } else if(preset == "FuriHalSubGhzPreset2FSKDev476Async") {
        cfg.modulation = 0;
        cfg.deviation = 47.60742f;
        cfg.rx_bw = 476;
    } else if(preset == "FuriHalSubGhzPresetMSK99_97KbAsync") {
        cfg.modulation = 4;
        cfg.deviation = 47.60742f;
        cfg.data_rate = 99.97f;
    } else if(preset == "FuriHalSubGhzPresetGFSK9_99KbAsync") {
        cfg.modulation = 1;
        cfg.deviation = 19.042969f;
        cfg.data_rate = 9.996f;
    } else if(preset.length() == 0 || preset == "FuriHalSubGhzPresetCustom") {
        cfg.modulation = 2;
        cfg.rx_bw = 270;
    } else {
        // Numeric presets are legacy rc-switch IDs; treat as OOK.
        bool numeric = true;
        for(size_t i = 0; i < preset.length(); i++) {
            if(preset[i] < '0' || preset[i] > '9') {
                numeric = false;
                break;
            }
        }
        if(!numeric) cfg.valid = false;
    }
    return cfg.valid;
}

static bool prepareTxHardware(uint32_t frequency_hz, const String& preset_name, int& tx_pin_out) {
    tx_pin_out = -1;
    if(frequency_hz == 0) frequency_hz = uint32_t(bruceConfigPins.rfFreq * 1000000.0f);
    const float frequency_mhz = frequency_hz / 1000000.0f;

    TxPresetConfig cfg;
    if(!resolvePresetConfig(preset_name, cfg)) return false;
    if(!initRfModule("", frequency_mhz)) return false;

    if(bruceConfigPins.rfModule == CC1101_SPI_MODULE) {
        ELECHOUSE_cc1101.setModulation(cfg.modulation);
        if(cfg.deviation > 0.0f) ELECHOUSE_cc1101.setDeviation(cfg.deviation);
        if(cfg.rx_bw > 0.0f) ELECHOUSE_cc1101.setRxBW(cfg.rx_bw);
        if(cfg.data_rate > 0.0f) ELECHOUSE_cc1101.setDRate(cfg.data_rate);

        tx_pin_out = bruceConfigPins.CC1101_bus.io0;
        pinMode(tx_pin_out, OUTPUT);
        ELECHOUSE_cc1101.setPA(12);
        ioExpander.turnPinOnOff(IO_EXP_CC_RX, LOW);
        ioExpander.turnPinOnOff(IO_EXP_CC_TX, HIGH);
        ELECHOUSE_cc1101.SetTx();
    } else {
        if(cfg.modulation != 2) return false;
        if(!initRfModule("tx", frequency_mhz)) return false;
        tx_pin_out = bruceConfigPins.rfTx;
        pinMode(tx_pin_out, OUTPUT);
    }

    digitalWrite(tx_pin_out, LOW);
    return true;
}

static void stopTxHardware(int tx_pin) {
    if(tx_pin >= 0) digitalWrite(tx_pin, LOW);
    deinitRfModule();
}

bool SubGhzAdvancedTransmitterAdapter::extractProtocolName(
    const String& capture,
    String& protocol_name) {
    return extractEffectiveProtocolName(capture, protocol_name);
}

bool SubGhzAdvancedTransmitterAdapter::canHandleCapture(const String& capture, bool full_profile) {
    String protocol_name;
    if(!extractEffectiveProtocolName(capture, protocol_name)) return false;
    if(isLegacyProtocolName(protocol_name)) return false;
    return SubGhzAdvancedDecoderAdapter::isProtocolEnabled(protocol_name, full_profile);
}

bool SubGhzAdvancedTransmitterAdapter::transmitCapture(
    const String& capture,
    bool full_profile,
    bool hideDefaultUI) {
    String protocol_name;
    if(!extractEffectiveProtocolName(capture, protocol_name)) return false;
    if(isLegacyProtocolName(protocol_name)) return false;
    if(!SubGhzAdvancedDecoderAdapter::isProtocolEnabled(protocol_name, full_profile)) return false;

    String tx_capture = buildAdvancedTxCapture(capture, protocol_name);

    String preset = "";
    (void)findField(tx_capture, "Preset", preset);

    String freq = "";
    uint32_t frequency_hz = 0;
    if(findField(tx_capture, "Frequency", freq)) frequency_hz = strtoul(freq.c_str(), NULL, 10);

    SubGhzEnvironment* env = subghz_environment_alloc();
    if(!env) return false;
    subghz_environment_set_protocol_registry(
        env, SubGhzAdvancedDecoderAdapter::getProtocolRegistry(full_profile));
    (void)subghz_environment_load_keystore(env, "/mfcodes");

    FlipperFormat* ff = flipper_format_file_alloc(nullptr);
    if(!ff) {
        subghz_environment_free(env);
        return false;
    }
    if(!flipper_format_load_from_string(ff, tx_capture.c_str())) {
        flipper_format_free(ff);
        subghz_environment_free(env);
        return false;
    }

    SubGhzTransmitter* tx = subghz_transmitter_alloc_init(env, protocol_name.c_str());
    if(!tx) {
        flipper_format_free(ff);
        subghz_environment_free(env);
        return false;
    }

    bool ok = false;
    int tx_pin = -1;
    do {
        if(subghz_transmitter_deserialize(tx, ff) != SubGhzProtocolStatusOk) break;
        if(!prepareTxHardware(frequency_hz, preset, tx_pin)) break;

        if(!hideDefaultUI) displayTextLine("Sending..");

        static constexpr size_t kMaxYields = 250000;
        for(size_t i = 0; i < kMaxYields; i++) {
            LevelDuration ld = subghz_transmitter_yield(tx);
            if(level_duration_is_reset(ld)) {
                ok = true;
                break;
            }
            if(level_duration_is_wait(ld)) {
                delayMicroseconds(100);
                continue;
            }

            uint32_t duration = level_duration_get_duration(ld);
            if(duration == 0 || duration > 2000000UL) continue;
            digitalWrite(tx_pin, level_duration_get_level(ld) ? HIGH : LOW);
            delayMicroseconds(duration);
        }
    } while(false);

    stopTxHardware(tx_pin);
    subghz_transmitter_stop(tx);
    subghz_transmitter_free(tx);
    flipper_format_free(ff);
    subghz_environment_free(env);
    return ok;
}
