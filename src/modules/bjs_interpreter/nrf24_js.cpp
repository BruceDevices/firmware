#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "nrf24_js.h"
#include "helpers_js.h"
#include "modules/NRF24/nrf_common.h"

JSValue native_nrf24Begin(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: nrf24.begin(channel)
    // Initialize the NRF24 module via SPI
    bool ok = nrf_start(NRF_MODE_SPI);
    if (!ok) {
        return JS_NewBool(false);
    }

    int channel = 76; // default
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) {
        JS_ToInt32(ctx, &channel, argv[0]);
    }
    if (channel < 0) channel = 0;
    if (channel > 125) channel = 125;

    NRFradio.setChannel(channel);
    NRFradio.setPALevel(RF24_PA_MAX);
    NRFradio.setDataRate(RF24_2MBPS);

    return JS_NewBool(true);
}

JSValue native_nrf24Send(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: nrf24.send(address_string_hex, data_uint8array)
    // address is a 5-byte hex string like "01:23:45:67:89"
    if (argc < 2) return JS_NewBool(false);

    const char *addrStr = NULL;
    JSCStringBuf addr_buf;
    if (JS_IsString(ctx, argv[0])) {
        addrStr = JS_ToCString(ctx, argv[0], &addr_buf);
    }
    if (!addrStr) return JS_NewBool(false);

    // Parse address from hex string
    uint8_t address[5] = {0};
    String addrS = String(addrStr);
    addrS.replace(":", "");
    if (addrS.length() >= 10) {
        for (int i = 0; i < 5; i++) {
            address[i] = (uint8_t)strtol(addrS.substring(i * 2, i * 2 + 2).c_str(), NULL, 16);
        }
    }

    // Get data from typed array
    if (!JS_IsTypedArray(ctx, argv[1])) return JS_NewBool(false);

    size_t dataLen = 0;
    const char *rawData = JS_GetTypedArrayBuffer(ctx, &dataLen, argv[1]);
    if (!rawData || dataLen == 0) return JS_NewBool(false);
    const uint8_t *data = (const uint8_t *)rawData;

    // Limit to 32 bytes (NRF24 max payload)
    if (dataLen > 32) dataLen = 32;

    NRFradio.stopListening();
    NRFradio.openWritingPipe(address);
    bool success = NRFradio.write(data, dataLen);

    return JS_NewBool(success);
}

JSValue native_nrf24Receive(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: nrf24.receive(timeout_ms)
    // returns: hex string of received data, or empty string on timeout
    int timeout = 5000;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) {
        JS_ToInt32(ctx, &timeout, argv[0]);
    }
    if (timeout < 100) timeout = 100;
    if (timeout > 30000) timeout = 30000;

    uint8_t address[5] = {0x01, 0x23, 0x45, 0x67, 0x89};
    NRFradio.openReadingPipe(1, address);
    NRFradio.startListening();

    unsigned long start = millis();
    while (millis() - start < (unsigned long)timeout) {
        if (NRFradio.available()) {
            uint8_t buf[32] = {0};
            NRFradio.read(buf, sizeof(buf));
            NRFradio.stopListening();

            // Return as hex string
            String hexStr = "";
            for (int i = 0; i < 32; i++) {
                if (buf[i] == 0 && i > 0) break;
                char hex[3];
                sprintf(hex, "%02X", buf[i]);
                hexStr += hex;
            }
            return JS_NewString(ctx, hexStr.c_str());
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    NRFradio.stopListening();
    return JS_NewString(ctx, "");
}

JSValue native_nrf24SetChannel(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: nrf24.setChannel(channel)
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) {
        int channel;
        JS_ToInt32(ctx, &channel, argv[0]);
        if (channel < 0) channel = 0;
        if (channel > 125) channel = 125;
        NRFradio.setChannel(channel);
    }
    return JS_UNDEFINED;
}

JSValue native_nrf24IsConnected(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return JS_NewBool(NRFradio.isChipConnected());
}

#endif
