/*
  AudioFileSourceHTTPSStream
  HTTPS/TLS capable streaming source with ICY metadata support

  Based on AudioFileSourceHTTPStream/AudioFileSourceICYStream
*/

#if defined(ESP32) || defined(ESP8266)

#ifdef _GNU_SOURCE
#undef _GNU_SOURCE
#endif
#define _GNU_SOURCE

#include "AudioFileSourceHTTPSStream.h"
#include <algorithm>
#include <string.h>

AudioFileSourceHTTPSStream::AudioFileSourceHTTPSStream(bool requestIcy) : requestIcy(requestIcy) {
    saveURL[0] = 0;
}

AudioFileSourceHTTPSStream::AudioFileSourceHTTPSStream(const char *url, bool requestIcy) : requestIcy(requestIcy) {
    saveURL[0] = 0;
    open(url);
}

bool AudioFileSourceHTTPSStream::open(const char *url) { return openInternal(url); }

bool AudioFileSourceHTTPSStream::openInternal(const char *url) {
    http.end();
    pos = 0;
    size = 0;
    icyByteCount = 0;
    lastMeta = "";
    mime = "";

    useSecure = String(url).startsWith("https");
    client = useSecure ? static_cast<WiFiClient *>(&secureClient) : &plainClient;

    if (useSecure) {
        secureClient.stop();
        secureClient.setInsecure(); // allow radio streams without pinned certs
    } else {
        plainClient.stop();
    }

    http.setReuse(true);
#ifndef ESP32
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
#else
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
#endif

    static const char *hdr[] = {"icy-metaint", "icy-name", "icy-genre", "icy-br", "content-type"};
    if (requestIcy) {
        http.addHeader("Icy-MetaData", "1");
        http.collectHeaders(hdr, 5);
    } else {
        http.collectHeaders(hdr, 1);
    }

    if (!http.begin(*client, url)) {
        cb.st(STATUS_HTTPFAIL, PSTR("Unable to start HTTP(S) request"));
        return false;
    }

    lastStatusCode = http.GET();
    if (lastStatusCode != HTTP_CODE_OK) {
        http.end();
        cb.st(STATUS_HTTPFAIL, PSTR("Can't open HTTP(S) stream"));
        return false;
    }

    size = http.getSize();
    mime = http.header("Content-Type");
    if (mime.length() == 0) mime = http.header("content-type");
    icyMetaInt = requestIcy && http.hasHeader(hdr[0]) ? http.header(hdr[0]).toInt() : 0;
    icyBrKbps = http.hasHeader("icy-br") ? http.header("icy-br").toInt() : 0;

    strncpy(saveURL, url, sizeof(saveURL));
    saveURL[sizeof(saveURL) - 1] = 0;
    return true;
}

AudioFileSourceHTTPSStream::~AudioFileSourceHTTPSStream() { http.end(); }

uint32_t AudioFileSourceHTTPSStream::read(void *data, uint32_t len) {
    if (data == nullptr) {
        audioLogger->printf_P(PSTR("ERROR! AudioFileSourceHTTPSStream::read passed NULL data\n"));
        return 0;
    }
    return readInternal(data, len, false);
}

uint32_t AudioFileSourceHTTPSStream::readNonBlock(void *data, uint32_t len) {
    if (data == nullptr) {
        audioLogger->printf_P(PSTR("ERROR! AudioFileSourceHTTPSStream::readNonBlock passed NULL data\n"));
        return 0;
    }
    return readInternal(data, len, true);
}

bool AudioFileSourceHTTPSStream::handleReconnect() {
    cb.st(STATUS_DISCONNECTED, PSTR("Stream disconnected"));
    http.end();
    for (int i = 0; i < reconnectTries; i++) {
        char buff[64];
        sprintf_P(buff, PSTR("Attempting to reconnect, try %d"), i + 1);
        cb.st(STATUS_RECONNECTING, buff);
        delay(reconnectDelayMs);
        if (openInternal(saveURL)) {
            cb.st(STATUS_RECONNECTED, PSTR("Stream reconnected"));
            return true;
        }
    }
    cb.st(STATUS_DISCONNECTED, PSTR("Unable to reconnect"));
    return false;
}

uint32_t AudioFileSourceHTTPSStream::readInternal(void *data, uint32_t len, bool nonBlock) {
retry:
    if (!http.connected()) {
        if (!handleReconnect()) return 0;
    }
    if ((size > 0) && (pos >= size)) return 0;

    WiFiClient *stream = http.getStreamPtr();

    // Can't read past EOF...
    if ((size > 0) && (len > (uint32_t)(size - pos))) len = size - pos;

    if (!nonBlock) {
        int start = millis();
        while ((stream->available() < (int)len) && (millis() - start < 500)) yield();
    }

    size_t avail = stream->available();
    if (!nonBlock && !avail) {
        cb.st(STATUS_NODATA, PSTR("No stream data available"));
        http.end();
        goto retry;
    }
    if (avail == 0) return 0;
    if (avail < len) len = avail;

    // Ensure we can't possibly read 2 ICY headers in a single go
    if (icyMetaInt > 1) { len = std::min((int)(icyMetaInt >> 1), (int)len); }

    int read = 0;
    int ret = 0;
    // If the read would hit an ICY block, split it up...
    if (((int)(icyByteCount + len) > (int)icyMetaInt) && (icyMetaInt > 0)) {
        int beforeIcy = icyMetaInt - icyByteCount;
        if (beforeIcy > 0) {
            ret = stream->read(reinterpret_cast<uint8_t *>(data), beforeIcy);
            if (ret < 0) ret = 0;
            read += ret;
            pos += ret;
            len -= ret;
            data = (void *)(reinterpret_cast<char *>(data) + ret);
            icyByteCount += ret;
            if (ret != beforeIcy) return read; // Partial read
        }

        // ICY MD handling
        if (!readIcyMetadata(stream)) return read;
        icyByteCount = 0;
    }

    ret = stream->read(reinterpret_cast<uint8_t *>(data), len);
    if (ret < 0) ret = 0;
    read += ret;
    pos += ret;
    icyByteCount += ret;
    return read;
}

bool AudioFileSourceHTTPSStream::seek(int32_t pos, int dir) {
    audioLogger->printf_P(PSTR("ERROR! AudioFileSourceHTTPSStream::seek not implemented!"));
    (void)pos;
    (void)dir;
    return false;
}

bool AudioFileSourceHTTPSStream::close() {
    http.end();
    return true;
}

bool AudioFileSourceHTTPSStream::isOpen() { return http.connected(); }

uint32_t AudioFileSourceHTTPSStream::getSize() { return size; }

uint32_t AudioFileSourceHTTPSStream::getPos() { return pos; }

bool AudioFileSourceHTTPSStream::readIcyMetadata(WiFiClient *stream) {
    int mdSize;
    uint8_t c;
    int mdret = stream->read(&c, 1);
    if (mdret == 0) return false;
    mdSize = c * 16;
    if ((mdret == 1) && (mdSize > 0)) {
        char icyBuff[256 + 16 + 1];
        char *readInto = icyBuff + 16;
        memset(icyBuff, 0, 16); // Ensure no residual matches occur
        while (mdSize) {
            int toRead = mdSize > 256 ? 256 : mdSize;
            int ret = stream->read((uint8_t *)readInto, toRead);
            if (ret < 0) return false;
            if (ret == 0) {
                delay(1);
                continue;
            }
            mdSize -= ret;
            // At this point we have 0...15 = last 15 chars read from prior read plus new data
            int end = 16 + ret; // The last byte of valid data
            char *header = (char *)memmem((void *)icyBuff, end, (void *)"StreamTitle=", 12);
            if (!header) {
                // No match, so move the last 16 bytes back to the start and continue
                memmove(icyBuff, icyBuff + end - 16, 16);
                delay(1);
                continue;
            }
            // Found header, now move it to the front
            int lastValidByte = end - (header - icyBuff) + 1;
            memmove(icyBuff, header, lastValidByte);
            // Now fill the buffer to the end with read data
            while (mdSize && lastValidByte < 255) {
                int toRead = mdSize > (256 - lastValidByte) ? (256 - lastValidByte) : mdSize;
                ret = stream->read((uint8_t *)icyBuff + lastValidByte, toRead);
                if (ret == -1) return false; // error
                if (ret == 0) {
                    delay(1);
                    continue;
                }
                mdSize -= ret;
                lastValidByte += ret;
            }
            // Buffer now contains StreamTitle=....., parse it
            char *p = icyBuff + 12;
            if (*p == '\'' || *p == '"') {
                char closing[] = {*p, ';', '\0'};
                char *psz = strstr(p + 1, closing);
                if (!psz) psz = strchr(&icyBuff[13], ';');
                if (psz) *psz = '\0';
                p++;
            } else {
                char *psz = strchr(p, ';');
                if (psz) *psz = '\0';
            }
            lastMeta = String(p);
            cb.md("StreamTitle", false, p);

            // Now skip rest of MD block
            while (mdSize) {
                int toRead = mdSize > 256 ? 256 : mdSize;
                ret = stream->read((uint8_t *)icyBuff, toRead);
                if (ret < 0) return false;
                if (ret == 0) {
                    delay(1);
                    continue;
                }
                mdSize -= ret;
            }
        }
    }
    return true;
}

#endif
