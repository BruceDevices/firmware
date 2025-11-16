/*
  AudioFileSourceHTTPSStream
  HTTPS/TLS capable streaming source with ICY metadata support

  Derived from AudioFileSourceHTTPStream/ICYStream (Earle F. Philhower)
*/

#pragma once

#if defined(ESP32) || defined(ESP8266)

#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <SPIFFS.h>

#ifdef ESP32
#include <HTTPClient.h>
#else
#include <ESP8266HTTPClient.h>
#endif

#include <ESP8266Audio.h>

class AudioFileSourceHTTPSStream : public AudioFileSource {
public:
    AudioFileSourceHTTPSStream(bool requestIcy = true);
    AudioFileSourceHTTPSStream(const char *url, bool requestIcy = true);
    virtual ~AudioFileSourceHTTPSStream() override;

    virtual bool open(const char *url) override;
    virtual uint32_t read(void *data, uint32_t len) override;
    virtual uint32_t readNonBlock(void *data, uint32_t len) override;
    virtual bool seek(int32_t pos, int dir) override;
    virtual bool close() override;
    virtual bool isOpen() override;
    virtual uint32_t getSize() override;
    virtual uint32_t getPos() override;
    bool SetReconnect(int tries, int delayms) {
        reconnectTries = tries;
        reconnectDelayMs = delayms;
        return true;
    }
    void enableIcy(bool enabled) { requestIcy = enabled; }
    String contentType() const { return mime; }
    int icyBitrateKbps() const { return icyBrKbps; }
    String lastMetadata() const { return lastMeta; }
    String lastUrl() const { return String(saveURL); }
    int lastHttpCode() const { return lastStatusCode; }

    enum {
        STATUS_HTTPFAIL = 2,
        STATUS_DISCONNECTED,
        STATUS_RECONNECTING,
        STATUS_RECONNECTED,
        STATUS_NODATA
    };

private:
    uint32_t readInternal(void *data, uint32_t len, bool nonBlock);
    bool openInternal(const char *url);
    bool handleReconnect();
    bool readIcyMetadata(WiFiClient *stream);

private:
    WiFiClientSecure secureClient;
    WiFiClient plainClient;
    WiFiClient *client = nullptr;
    HTTPClient http;
    bool requestIcy = true;
    bool useSecure = false;

    int pos = 0;
    int size = 0;
    int reconnectTries = 3;
    int reconnectDelayMs = 500;
    int lastStatusCode = 0;
    int icyMetaInt = 0;
    int icyByteCount = 0;
    int icyBrKbps = 0;
    String lastMeta;
    String mime;
    char saveURL[192] = {0};
};

#endif
