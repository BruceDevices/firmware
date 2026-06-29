#include "subghz_advanced_subfile_codec.h"

#include "core/type_convertion.h"

static String jsonEscape(const String& in) {
    String out;
    out.reserve(in.length() + 8);
    for(size_t i = 0; i < in.length(); i++) {
        const char c = in[i];
        switch(c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if((uint8_t)c < 0x20) {
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04X", (uint8_t)c);
                    out += buf;
                } else {
                    out += c;
                }
                break;
        }
    }
    return out;
}

static String normalizeHexLike(const String& in) {
    String out = in;
    out.trim();
    out.replace(" ", "");
    if(out.startsWith("0x") || out.startsWith("0X")) out = out.substring(2);
    out.toUpperCase();
    return out;
}

static bool splitLineKV(const String& line, String& key, String& value) {
    int idx = line.indexOf(':');
    if(idx <= 0) return false;
    key = line.substring(0, idx);
    value = line.substring(idx + 1);
    key.trim();
    value.trim();
    return key.length() > 0;
}

static int countRawPulses(const String& rawJoined) {
    if(rawJoined.length() == 0) return 0;
    int pulses = 1;
    for(size_t i = 0; i < rawJoined.length(); i++) {
        if(rawJoined[i] == ' ') pulses++;
    }
    return pulses;
}

String SubGhzAdvancedFrame::toJson() const {
    String out = "{";
    out += "\"valid\":" + String(valid ? "true" : "false");
    out += ",\"source\":\"" + jsonEscape(source) + "\"";
    out += ",\"filetype\":\"" + jsonEscape(filetype) + "\"";
    out += ",\"protocol_name\":\"" + jsonEscape(protocol_name) + "\"";
    out += ",\"frequency_hz\":" + String(frequency_hz);
    out += ",\"bit_count\":" + String(bit_count);
    out += ",\"key_hex\":\"" + jsonEscape(key_hex) + "\"";
    out += ",\"counter\":\"" + jsonEscape(counter) + "\"";
    out += ",\"button\":\"" + jsonEscape(button) + "\"";
    out += ",\"serial\":\"" + jsonEscape(serial) + "\"";
    out += ",\"raw_summary\":\"" + jsonEscape(raw_summary) + "\"";
    out += ",\"notes\":\"" + jsonEscape(notes) + "\"";
    out += "}";
    return out;
}

SubGhzAdvancedFrame SubGhzAdvancedSubFileCodec::parse(const String& text, const String& source) {
    SubGhzAdvancedFrame frame;
    frame.source = source;

    String protocol = "";
    String key = "";
    String rawData = "";
    String detectedProtocol = "";
    String detectedKey = "";
    String detectedSerial = "";
    String detectedButton = "";
    String detectedCounter = "";

    int lineStart = 0;
    while(lineStart <= (int)text.length()) {
        int lineEnd = text.indexOf('\n', lineStart);
        if(lineEnd < 0) lineEnd = text.length();
        String line = text.substring(lineStart, lineEnd);
        line.replace("\r", "");
        line.trim();

        String k;
        String v;
        if(splitLineKV(line, k, v)) {
            if(k == "Filetype") frame.filetype = v;
            else if(k == "Protocol") protocol = v;
            else if(k == "Frequency") frame.frequency_hz = strtoul(v.c_str(), NULL, 10);
            else if(k == "Bit") frame.bit_count = v.toInt();
            else if(k == "Key") key = v;
            else if(k == "TE") {
                if(frame.notes.length()) frame.notes += "; ";
                frame.notes += "TE=" + v;
            } else if(k == "Counter") frame.counter = v;
            else if(k == "Button") frame.button = v;
            else if(k == "Serial") frame.serial = v;
            else if(k == "RAW_Data" || k == "Data_RAW") {
                if(rawData.length()) rawData += " ";
                rawData += v;
            } else if(k == "Detected_Protocol")
                detectedProtocol = v;
            else if(k == "Detected_Key")
                detectedKey = v;
            else if(k == "Detected_Serial")
                detectedSerial = v;
            else if(k == "Detected_Button")
                detectedButton = v;
            else if(k == "Detected_Counter")
                detectedCounter = v;
            else if(k == "Preset") {
                if(frame.notes.length()) frame.notes += "; ";
                frame.notes += "Preset=" + v;
            } else if(k == "Manufacturer") {
                if(frame.notes.length()) frame.notes += "; ";
                frame.notes += "Manufacturer=" + v;
            }
        }

        if(lineEnd == (int)text.length()) break;
        lineStart = lineEnd + 1;
    }

    frame.protocol_name = protocol.length() ? protocol : "Unknown";
    if(detectedProtocol.length()) {
        if(frame.protocol_name == "RcSwitch" || frame.protocol_name == "RAW" || frame.protocol_name == "Unknown") {
            frame.protocol_name = detectedProtocol;
        } else if(frame.notes.length()) {
            frame.notes += "; Detected=" + detectedProtocol;
        } else {
            frame.notes = "Detected=" + detectedProtocol;
        }
    }

    key = normalizeHexLike(key);
    if(key.length() == 0 && frame.protocol_name == "RcSwitch") {
        // RCSwitch sometimes comes with parsed numeric key only in higher layers.
        frame.key_hex = "";
    } else {
        frame.key_hex = key;
    }

    if(rawData.length()) {
        int pulses = countRawPulses(rawData);
        frame.raw_summary = String(pulses) + " pulses";
        if(frame.key_hex.length() == 0 && frame.protocol_name == "Unknown") frame.protocol_name = "RAW";
    }

    // Normalize flipper key format (AA BB CC -> AABBCC)
    frame.key_hex = normalizeHexLike(frame.key_hex);

    frame.serial = normalizeHexLike(frame.serial);
    detectedKey = normalizeHexLike(detectedKey);
    detectedSerial = normalizeHexLike(detectedSerial);
    detectedButton = normalizeHexLike(detectedButton);
    detectedCounter = normalizeHexLike(detectedCounter);

    if(frame.key_hex.length() == 0 && detectedKey.length()) frame.key_hex = detectedKey;
    if(frame.serial.length() == 0 && detectedSerial.length()) frame.serial = detectedSerial;
    if(frame.button.length() == 0 && detectedButton.length()) frame.button = detectedButton;
    if(frame.counter.length() == 0 && detectedCounter.length()) frame.counter = detectedCounter;

    frame.valid = frame.filetype.length() > 0 || protocol.length() > 0 || key.length() > 0 ||
                  rawData.length() > 0;

    // Extra compatibility cues
    if(frame.filetype == "Bruce SubGhz File") {
        if(frame.notes.length()) frame.notes += "; ";
        frame.notes += "Format=Bruce";
    } else if(frame.filetype.startsWith("Flipper SubGhz")) {
        if(frame.notes.length()) frame.notes += "; ";
        frame.notes += "Format=Flipper";
    }

    if(frame.protocol_name == "Unknown" && rawData.length()) {
        frame.protocol_name = "RAW";
    }

    return frame;
}
