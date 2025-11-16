#include "radio.h"
#include "core/display.h"
#include "core/sd_functions.h"
#include "core/utils.h"
#include <ArduinoJson.h>
#include <algorithm>
#include <cstring>
#include <globals.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#include <driver/i2s.h>
#endif

namespace {
    const char *kStationsPath = "/radio/radio_stations.json";
    constexpr size_t kMaxLogLines = 12;
    constexpr unsigned long kRedrawIntervalMs = 600;
    RadioPlayer playerInstance;

#define RADIO_LOGI(fmt, ...) log_i("[radio] " fmt, ##__VA_ARGS__)
#define RADIO_LOGW(fmt, ...) log_w("[radio] " fmt, ##__VA_ARGS__)

    String guessMime(const String &url) {
        String lower = url;
        lower.toLowerCase();
        if (lower.endsWith(".aac") || lower.endsWith(".m4a")) return "audio/aac";
        if (lower.endsWith(".flac")) return "audio/flac";
        if (lower.endsWith(".opus") || lower.endsWith(".ogg")) return "audio/ogg";
        if (lower.endsWith(".wav")) return "audio/wav";
        return "audio/mpeg";
    }

    void clearNavInputs() {
        check(AnyKeyPress);
        check(EscPress);
        check(SelPress);
        check(PrevPress);
        check(NextPress);
        check(UpPress);
        check(DownPress);
    }

    void addDefaultStations(std::vector<RadioStation> &stations) {
        stations.push_back(
            {"BBC World Service", "https://stream.live.vc.bbcmedia.co.uk/bbc_world_service", "audio/aac",
             "Global news and analysis"}
        );
        stations.push_back(
            {"NPR Live", "https://npr-ice.streamguys1.com/live.mp3", "audio/mpeg", "US public radio news"}
        );
        stations.push_back(
            {"Radio Paradise (FLAC)", "https://stream.radioparadise.com/flac", "audio/flac", "Hi-fi eclectic playlist"}
        );
    }
}

RadioPlayer &radioPlayer() { return playerInstance; }

RadioPlayer::~RadioPlayer() { stop(); }

void RadioPlayer::addLog(const String &msg) {
    _log.push_back(msg);
    while (_log.size() > kMaxLogLines) { _log.pop_front(); }
}

void RadioPlayer::setStatus(RadioPlayerStatus status, const String &text) {
    if (_status == status && _statusText == text) return;
    _status = status;
    _statusText = text;
    addLog(text);
}

bool RadioPlayer::configureOutput(AudioOutputI2S *output) {
#if defined(BCLK) && defined(WCLK) && defined(DOUT)
#if defined(ESP_IDF_VERSION_MAJOR) && ESP_IDF_VERSION_MAJOR >= 5
#ifdef MCLK
    return output->SetPinout(BCLK, WCLK, DOUT, MCLK);
#else
    return output->SetPinout(BCLK, WCLK, DOUT);
#endif
#else
    return output->SetPinout(BCLK, WCLK, DOUT);
#endif
#else
    log_w("Skipping audio output configuration: I2S pins not defined");
    return false;
#endif
}

AudioGenerator *RadioPlayer::createGenerator(const String &mime, const String &url) {
    String lowerMime = mime;
    lowerMime.toLowerCase();
    String lowerUrl = url;
    lowerUrl.toLowerCase();

    if (lowerMime.indexOf("aac") != -1 || lowerUrl.endsWith(".aac") || lowerUrl.endsWith(".m4a")) {
        _codec = "AAC";
        return new AudioGeneratorAAC();
    }
    if (lowerMime.indexOf("flac") != -1 || lowerUrl.endsWith(".flac")) {
        _codec = "FLAC";
        return new AudioGeneratorFLAC();
    }
    if (lowerMime.indexOf("opus") != -1 || lowerMime.indexOf("ogg") != -1 || lowerUrl.endsWith(".opus") ||
        lowerUrl.endsWith(".ogg")) {
        _codec = "OPUS";
        return new AudioGeneratorOpus();
    }
    if (lowerMime.indexOf("wav") != -1 || lowerUrl.endsWith(".wav")) {
        _codec = "WAV";
        return new AudioGeneratorWAV();
    }

    _codec = "MP3";
    return new AudioGeneratorMP3();
}

bool RadioPlayer::isPlaylist(const String &url, const String &mime) const {
    String lowerUrl = url;
    lowerUrl.toLowerCase();
    String lowerMime = mime;
    lowerMime.toLowerCase();

    if (lowerUrl.endsWith(".m3u") || lowerUrl.endsWith(".m3u8") || lowerUrl.endsWith(".pls")) return true;
    if (lowerMime.indexOf("mpegurl") != -1 || lowerMime.indexOf("scpls") != -1 || lowerMime.indexOf("playlist") != -1)
        return true;
    return false;
}

bool RadioPlayer::parsePlaylist(const String &body, String &outUrl) {
    int fileIndex = body.indexOf("File1=");
    if (fileIndex >= 0) {
        int endIdx = body.indexOf('\n', fileIndex);
        String line = body.substring(fileIndex, endIdx >= 0 ? endIdx : body.length());
        int valueIdx = line.indexOf('=');
        if (valueIdx >= 0) {
            outUrl = line.substring(valueIdx + 1);
            outUrl.trim();
            return outUrl.length() > 0;
        }
    }

    int start = 0;
    while (start < body.length()) {
        int end = body.indexOf('\n', start);
        if (end < 0) end = body.length();
        String line = body.substring(start, end);
        line.trim();
        start = end + 1;
        if (line.length() == 0 || line.startsWith("#")) continue;
        if (line.startsWith("http")) {
            outUrl = line;
            return true;
        }
    }
    return false;
}

bool RadioPlayer::resolveStreamUrl(const RadioStation &station, String &resolvedUrl, String &resolvedMime) {
    resolvedUrl = station.url;
    resolvedMime = station.mime.length() ? station.mime : guessMime(station.url);

    if (!isPlaylist(resolvedUrl, resolvedMime)) { return true; }
    addLog("Resolving playlist...");
    RADIO_LOGI("Resolving playlist url=%s mime=%s", resolvedUrl.c_str(), resolvedMime.c_str());

    HTTPClient http;
    WiFiClientSecure secure;
    WiFiClient plain;
    bool secureMode = resolvedUrl.startsWith("https");
    WiFiClient *cli = secureMode ? static_cast<WiFiClient *>(&secure) : &plain;
    if (secureMode) secure.setInsecure();
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.setTimeout(6000);
    if (!http.begin(*cli, resolvedUrl)) {
        addLog("Playlist open failed");
        return false;
    }

    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        addLog("Playlist HTTP " + String(code));
        http.end();
        return false;
    }

    String body = http.getString();
    http.end();

    String streamUrl;
    if (!parsePlaylist(body, streamUrl)) {
        addLog("No stream in playlist");
        RADIO_LOGW("Playlist parsed without stream (url=%s)", resolvedUrl.c_str());
        return false;
    }

    if (streamUrl.startsWith("//")) streamUrl = (secureMode ? "https:" : "http:") + streamUrl;

    resolvedUrl = streamUrl;
    resolvedMime = "";
    RADIO_LOGI("Playlist resolved to %s", resolvedUrl.c_str());
    return true;
}

void RadioPlayer::attachCallbacks(AudioFileSourceHTTPSStream *source, AudioFileSourceBuffer *buffer) {
    if (source) {
        source->RegisterStatusCB(
            [](void *cbData, int code, const char *string) {
                auto *self = static_cast<RadioPlayer *>(cbData);
                String msg = "SRC(" + String(code) + "): " + (string ? String(string) : "");
                self->addLog(msg);
                if (code == AudioFileSourceHTTPSStream::STATUS_RECONNECTING) {
                    self->setStatus(RadioPlayerStatus::Connecting, "Reconnecting...");
                }
                if (code == AudioFileSourceHTTPSStream::STATUS_RECONNECTED) {
                    self->setStatus(RadioPlayerStatus::Buffering, "Reconnected");
                }
            },
            this
        );
        source->RegisterMetadataCB(
            [](void *cbData, const char *type, bool isStr, const char *value) {
                auto *self = static_cast<RadioPlayer *>(cbData);
                if (!type || !value) return;
                if (strcmp(type, "StreamTitle") == 0 && isStr) {
                    self->_metadata = String(value);
                    self->addLog("ICY: " + self->_metadata);
                }
            },
            this
        );
    }
    if (buffer) {
        buffer->RegisterStatusCB(
            [](void *cbData, int code, const char *string) {
                auto *self = static_cast<RadioPlayer *>(cbData);
                String msg = "BUF(" + String(code) + "): " + (string ? String(string) : "");
                self->addLog(msg);
                if (code == AudioFileSourceBuffer::STATUS_FILLING) {
                    self->setStatus(RadioPlayerStatus::Buffering, "Buffering stream");
                }
            },
            this
        );
    }
}

bool RadioPlayer::startPipeline(const String &url, const String &mimeHint) {
    _mime = mimeHint.length() ? mimeHint : guessMime(url);

    _output = new AudioOutputI2S();
    if (!_output || !configureOutput(_output)) {
        setStatus(RadioPlayerStatus::Error, "Missing I2S pinout");
        displayError("Radio: I2S init failed", true);
        RADIO_LOGW("I2S pinout missing/configure failed");
        returnToMenu = true;
        resetAudioChain();
        return false;
    }
    if (!_output->begin()) {
        setStatus(RadioPlayerStatus::Error, "I2S driver init failed");
        displayError("Radio: I2S init failed", true);
        RADIO_LOGW("I2S driver begin failed");
        returnToMenu = true;
        resetAudioChain();
        return false;
    }
    _i2sDriverReady = true;
    _output->SetGain(((float)bruceConfig.soundVolume) / 100.0f);
    _output->SetOutputModeMono(true);

    _source = new AudioFileSourceHTTPSStream(true);
    _source->SetReconnect(6, 750);

    if (!_source->open(url.c_str())) {
        setStatus(RadioPlayerStatus::Error, "Stream open failed");
        displayError("Radio: stream open failed", true);
        RADIO_LOGW("Stream open failed");
        returnToMenu = true;
        resetAudioChain();
        return false;
    }

    _bufferBytes = computeBufferBytes(_source->icyBitrateKbps());
    _buffer = new AudioFileSourceBuffer(_source, _bufferBytes);
    if (!_buffer) {
        setStatus(RadioPlayerStatus::Error, "Buffer alloc failed");
        displayError("Radio: buffer alloc failed", true);
        RADIO_LOGW("Stream buffer allocation failed");
        returnToMenu = true;
        resetAudioChain();
        return false;
    }

    attachCallbacks(_source, _buffer);

    if (_source->contentType().length()) _mime = _source->contentType();
    _generator = createGenerator(_mime, url);

    if (!_generator) {
        setStatus(RadioPlayerStatus::Error, "Unsupported codec");
        displayError("Radio: codec unsupported", true);
        RADIO_LOGW("Unsupported codec: mime=%s url=%s", _mime.c_str(), url.c_str());
        returnToMenu = true;
        resetAudioChain();
        return false;
    }

    if (!_generator->begin(_buffer, _output)) {
        setStatus(RadioPlayerStatus::Error, "Decoder start error");
        displayError("Radio: decoder start failed", true);
        RADIO_LOGW("Decoder begin failed");
        returnToMenu = true;
        resetAudioChain();
        return false;
    }

    setStatus(RadioPlayerStatus::Buffering, "Buffering stream");
    RADIO_LOGI("Playback pipeline ready codec=%s mime=%s buffer=%u", _codec.c_str(), _mime.c_str(), (unsigned)_bufferBytes);
    return true;
}

bool RadioPlayer::play(const RadioStation &station) {
#if !defined(HAS_NS4168_SPKR)
    setStatus(RadioPlayerStatus::Error, "Speaker not available");
    displayWarning("Speaker not available", true);
    return false;
#endif
    if (!bruceConfig.soundEnabled) {
        setStatus(RadioPlayerStatus::Error, "Sound disabled");
        displayWarning("Sound disabled", true);
        return false;
    }
    if (WiFi.status() != WL_CONNECTED) {
        setStatus(RadioPlayerStatus::Error, "WiFi disconnected");
        displayWarning("WiFi disconnected", true);
        return false;
    }

    stop();
    _log.clear();
    _currentStation = station;
    _hasStation = true;
    setStatus(RadioPlayerStatus::Resolving, "Resolving stream...");
    RADIO_LOGI("Play requested: %s (%s)", station.name.c_str(), station.url.c_str());
    _i2sDriverReady = false;

    String resolvedUrl;
    String resolvedMime;
    if (!resolveStreamUrl(station, resolvedUrl, resolvedMime)) {
        setStatus(RadioPlayerStatus::Error, "Failed resolving stream");
        RADIO_LOGW("Stream resolve failed");
        return false;
    }

    setStatus(RadioPlayerStatus::Connecting, "Opening stream...");
    if (!startPipeline(resolvedUrl, resolvedMime)) return false;

    RADIO_LOGI("Playback started codec=%s mime=%s", _codec.c_str(), _mime.c_str());
    return true;
}

bool RadioPlayer::loop() {
    if (!_generator) return false;
    if (!_generator->isRunning()) {
        setStatus(RadioPlayerStatus::Stopped, "Playback finished");
        stop();
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        setStatus(RadioPlayerStatus::Error, "WiFi lost");
        stop();
        return false;
    }

    if (_buffer) _buffer->loop();
    bool ok = _generator->loop();
    if (!ok) {
        setStatus(RadioPlayerStatus::Error, "Playback loop error");
        stop();
        return false;
    }
    if (_status == RadioPlayerStatus::Buffering) setStatus(RadioPlayerStatus::Playing, "Playing");
    return true;
}

void RadioPlayer::resetAudioChain() {
    if (_generator) {
        _generator->stop();
        delete _generator;
        _generator = nullptr;
    }
    if (_output) { _output->stop(); }
#if !defined(ESP8266)
    if (_i2sDriverReady) {
        esp_err_t err = i2s_driver_uninstall((i2s_port_t)0);
        if (err != ESP_OK) { RADIO_LOGW("i2s_driver_uninstall failed: %d", err); }
    }
#endif
    if (_output) {
        delete _output;
        _output = nullptr;
    }
    if (_buffer) {
        _buffer->close();
        delete _buffer;
        _buffer = nullptr;
    }
    if (_source) {
        _source->close();
        delete _source;
        _source = nullptr;
    }
    _i2sDriverReady = false;
}

void RadioPlayer::stop() {
    RADIO_LOGI("Stop requested");
    if (_status != RadioPlayerStatus::Idle) { addLog("Stopping playback"); }
    resetAudioChain();
    _hasStation = false;
    _status = RadioPlayerStatus::Idle;
    _statusText = "Idle";
    _metadata = "";
    _codec = "";
    _mime = "";
}

float RadioPlayer::bufferFillPct() const {
    if (!_buffer) return 0.0f;
    return (static_cast<float>(_buffer->getFillLevel()) / static_cast<float>(_bufferBytes)) * 100.0f;
}

size_t RadioPlayer::computeBufferBytes(int bitrateKbps) const {
    const int seconds = 5;
    int kbps = bitrateKbps > 0 ? bitrateKbps : 128;
    size_t bytes = (size_t)((kbps * 1000 / 8) * seconds);
    const size_t minBuf = 32 * 1024;
    const size_t maxBuf = 192 * 1024;
    if (bytes < minBuf) bytes = minBuf;
    if (bytes > maxBuf) bytes = maxBuf;
    return bytes;
}

bool loadRadioStations(std::vector<RadioStation> &stations, String &error) {
    stations.clear();

    std::vector<FS *> filesystems;
    if (setupSdCard()) filesystems.push_back(&SD);
    filesystems.push_back(&LittleFS);

    for (FS *fs : filesystems) {
        if (!fs->exists(kStationsPath)) continue;

        File file = fs->open(kStationsPath, FILE_READ);
        if (!file) {
            error = "Cannot open " + String(kStationsPath);
            continue;
        }

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, file);
        file.close();
        if (err) {
            error = "Invalid radio_stations.json";
            continue;
        }

        JsonArray arr;
        if (doc.is<JsonArray>()) arr = doc.as<JsonArray>();
        else if (doc["stations"].is<JsonArray>()) arr = doc["stations"].as<JsonArray>();
        else {
            error = "Stations file format not supported";
            continue;
        }

        for (JsonVariant v : arr) {
            RadioStation st;
            st.name = v["name"] | "Station";
            st.url = v["url"] | "";
            st.description = v["description"] | "";
            st.mime = v["mime"] | guessMime(st.url);
            st.name.trim();
            st.url.trim();
            st.mime.trim();
            if (st.url.length() == 0) continue;
            stations.push_back(st);
        }
        if (!stations.empty()) {
            error = "";
            return true;
        }
    }

    addDefaultStations(stations);
    error = "Using built-in station list";
    return true;
}

void drawPlaybackScreen(const RadioStation &station, const RadioPlayer &player) {
    resetTftDisplay(0, 0, bruceConfig.priColor, FM, bruceConfig.bgColor, bruceConfig.bgColor);
    drawMainBorderWithTitle("ONLINE RADIO");

    setPadCursor(1, 1);
    padprintln("Station: " + station.name, 1);
    if (station.description.length()) padprintln("About: " + station.description, 1);
    padprintln("Status: " + player.statusText(), 1);
    padprintln("Codec: " + player.codec() + "  MIME: " + player.mime(), 1);

    padprintf("Buffer: %d%% (%u KB)\n", (int)player.bufferFillPct(), (unsigned)(player.bufferBytesTarget() / 1024));
    padprintln("Now: " + (player.metadata().length() ? player.metadata() : String("waiting...")), 1);

    padprintln("Debug:", 1);
    int shown = 0;
    for (auto it = player.log().rbegin(); it != player.log().rend() && shown < 5; ++it, ++shown) {
        padprintln("- " + *it, 2);
    }

    padprintln("Press ESC or SEL to stop", 1);
}

void showPlayback(const RadioStation &station) {
    if (!playerInstance.play(station)) {
        String msg = playerInstance.statusText().length() ? playerInstance.statusText() : String("Radio start failed");
        displayError(msg, true);
        return;
    }
    RADIO_LOGI("Entered playback loop");
    clearNavInputs();

    unsigned long lastDraw = 0;
    while (true) {
        const unsigned long now = millis();
        if (now - lastDraw > kRedrawIntervalMs) {
            drawPlaybackScreen(station, playerInstance);
            lastDraw = now;
        }
        if (!playerInstance.loop()) break;
        if (check(EscPress) || check(SelPress)) {
            RADIO_LOGI("Playback stop requested by input");
            break;
        }
        delay(10);
    }

    RADIO_LOGI("Playback loop exit: status=%d", (int)playerInstance.status());
    playerInstance.stop();
    clearNavInputs();
}

void radioStopPlayback() {
    if (playerInstance.status() == RadioPlayerStatus::Idle) {
        displayWarning("Radio not playing", true);
        return;
    }
    playerInstance.stop();
    displayWarning("Playback stopped", true);
}

void radioStationsMenu();

void radioMainMenu() {
    options.clear();
    options.push_back({"Stations", radioStationsMenu});

    if (playerInstance.current()) {
        options.push_back({"Now playing", [=]() { showPlayback(*playerInstance.current()); }});
        options.push_back({"Stop playback", radioStopPlayback});
    }

    options.push_back({"Back", backToMenu});
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "Radio");
}

void radioStationsMenu() {
    std::vector<RadioStation> stations;
    String err;
    if (!loadRadioStations(stations, err)) {
        displayError(err, true);
        return;
    }
    if (stations.empty()) {
        displayWarning("Station list empty", true);
        return;
    }

    options.clear();
    for (auto station : stations) {
        options.push_back({station.name, [station]() { showPlayback(station); }});
    }
    options.push_back({"Back", radioMainMenu});
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "Stations");
}

void radioAirMock() {
    resetTftDisplay(0, 0, bruceConfig.priColor, FM, bruceConfig.bgColor, bruceConfig.bgColor);
    drawMainBorderWithTitle("AIR RADIO (WIP)");

    setPadCursor(1, 1);
    padprintln("Future live broadcasting goes here.", 1);
    padprintln("Planned: HQ encoder, mic mix, jingles,", 1);
    padprintln("RDS/metadata injection and monitoring.", 1);
    padprintln("", 1);
    padprintln("Press any key to return", 1);

    while (!check(AnyKeyPress) && !check(EscPress) && !check(SelPress)) { delay(50); }
}
