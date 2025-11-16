#pragma once

#include "AudioFileSourceHTTPSStream.h"
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorAAC.h>
#include <AudioGeneratorFLAC.h>
#include <AudioGeneratorMP3.h>
#include <AudioGeneratorOpus.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>
#include <deque>
#include <vector>

struct RadioStation {
    String name;
    String url;
    String description;
    String mime;
};

enum class RadioPlayerStatus { Idle = 0, Resolving, Connecting, Buffering, Playing, Stopped, Error };

class RadioPlayer {
public:
    RadioPlayer() = default;
    ~RadioPlayer();

    bool play(const RadioStation &station);
    bool loop();
    void stop();

    RadioPlayerStatus status() const { return _status; }
    String statusText() const { return _statusText; }
    String metadata() const { return _metadata; }
    String codec() const { return _codec; }
    String mime() const { return _mime; }
    float bufferFillPct() const;
    const RadioStation *current() const { return _hasStation ? &_currentStation : nullptr; }
    const std::deque<String> &log() const { return _log; }

    size_t bufferBytesTarget() const { return _bufferBytes; }

private:
    void setStatus(RadioPlayerStatus status, const String &text);
    void addLog(const String &msg);
    bool configureOutput(AudioOutputI2S *output);
    AudioGenerator *createGenerator(const String &mime, const String &url);
    bool resolveStreamUrl(const RadioStation &station, String &resolvedUrl, String &resolvedMime);
    bool isPlaylist(const String &url, const String &mime) const;
    bool parsePlaylist(const String &body, String &outUrl);
    void attachCallbacks(AudioFileSourceHTTPSStream *source, AudioFileSourceBuffer *buffer);
    size_t computeBufferBytes(int bitrateKbps) const;
    bool startPipeline(const String &url, const String &mimeHint);
    void resetAudioChain();

    RadioStation _currentStation = {};
    bool _hasStation = false;
    RadioPlayerStatus _status = RadioPlayerStatus::Idle;
    String _statusText;
    String _metadata;
    String _codec;
    String _mime;
    std::deque<String> _log;
    size_t _bufferBytes = 16384;

    AudioFileSourceHTTPSStream *_source = nullptr;
    AudioFileSourceBuffer *_buffer = nullptr;
    AudioGenerator *_generator = nullptr;
    AudioOutputI2S *_output = nullptr;
    bool _i2sDriverReady = false;
};

bool loadRadioStations(std::vector<RadioStation> &stations, String &error);
void radioMainMenu();
void radioAirMock();
RadioPlayer &radioPlayer();
