#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "wifi_js.h"

#include "core/sd_functions.h"
#include "core/wifi/wifi_common.h"
#include "helpers_js.h"
#include "storage_js.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

static const char *wifi_enc_types[] = {
    "OPEN",
    "WEP",
    "WPA_PSK",
    "WPA2_PSK",
    "WPA_WPA2_PSK",
    "ENTERPRISE",
    "WPA2_ENTERPRISE",
    "WPA3_PSK",
    "WPA2_WPA3_PSK",
    "WAPI_PSK",
    "WPA3_ENT_192",
    "MAX"
};

JSValue native_wifiConnected(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return JS_NewBool(wifiConnected);
}

JSValue native_wifiConnectDialog(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    bool connected = wifiConnectMenu();
    return JS_NewBool(connected);
}

JSValue native_wifiConnect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc < 1 || !JS_IsString(ctx, argv[0]))
        return JS_ThrowTypeError(ctx, "wifiConnect(ssid:string, timeout?:int, pwd?:string)");

    JSCStringBuf ssb;
    const char *ssid = JS_ToCString(ctx, argv[0], &ssb);
    int timeout_in_seconds = 10;
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &timeout_in_seconds, argv[1]);

    bool r = false;
    Serial.println(String("Connecting to: ") + (ssid ? ssid : ""));

    WiFi.mode(WIFI_MODE_STA);
    if (argc > 2 && JS_IsString(ctx, argv[2])) {
        JSCStringBuf psb;
        const char *pwd = JS_ToCString(ctx, argv[2], &psb);
        WiFi.begin(ssid, pwd);
    } else {
        WiFi.begin(ssid);
    }

    int i = 0;
    do {
        delay(1000);
        i++;
        if (i > timeout_in_seconds) {
            Serial.println("timeout");
            break;
        }
    } while (WiFi.status() != WL_CONNECTED);

    if (WiFi.status() == WL_CONNECTED) {
        r = true;
        wifiIP = WiFi.localIP().toString();
        wifiConnected = true;
    }

    return JS_NewBool(r);
}

JSValue native_wifiScan(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    WiFi.mode(WIFI_MODE_STA);
    int nets = WiFi.scanNetworks();
    JSValue arr = JS_NewArray(ctx, nets);
    uint32_t idx = 0;
    for (int i = 0; i < nets; i++) {
        JSValue obj = JS_NewObject(ctx);
        int enctypeInt = int(WiFi.encryptionType(i));
        const char *enctype = enctypeInt < 12 ? wifi_enc_types[enctypeInt] : "UNKNOWN";
        JS_SetPropertyStr(ctx, obj, "encryptionType", JS_NewString(ctx, enctype));
        JS_SetPropertyStr(ctx, obj, "SSID", JS_NewString(ctx, WiFi.SSID(i).c_str()));
        JS_SetPropertyStr(ctx, obj, "MAC", JS_NewString(ctx, WiFi.BSSIDstr(i).c_str()));
        JS_SetPropertyStr(ctx, obj, "RSSI", JS_NewInt32(ctx, WiFi.RSSI(i)));
        JS_SetPropertyStr(ctx, obj, "channel", JS_NewInt32(ctx, WiFi.channel(i)));
        JS_SetPropertyUint32(ctx, arr, idx++, obj);
    }
    return arr;
}

JSValue native_wifiDisconnect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    wifiDisconnect();
    return JS_UNDEFINED;
}

JSValue native_httpFetch(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    HTTPClient http;
    http.setReuse(false);

    JSCStringBuf stringBuffer;

    if (WiFi.status() != WL_CONNECTED) wifiConnectMenu();
    if (WiFi.status() != WL_CONNECTED) return JS_ThrowTypeError(ctx, "WIFI Not Connected");

    if (argc < 1 || !JS_IsString(ctx, argv[0]))
        return JS_ThrowTypeError(ctx, "httpFetch(url:string, options?:object|headers?:array)");

    const char *url = JS_ToCString(ctx, argv[0], &stringBuffer);
    http.begin(url);

    // handle headers if a simple array of pairs was passed as second arg
    if (argc > 1 && JS_GetClassID(ctx, argv[1]) == JS_CLASS_ARRAY) {
        JSValue jsvArrayLength = JS_GetPropertyStr(ctx, argv[1], "length");
        if (JS_IsNumber(ctx, jsvArrayLength)) {
            uint32_t arrayLength = 0;
            JS_ToUint32(ctx, &arrayLength, jsvArrayLength);
            for (uint32_t i = 0; i + 1 < arrayLength; i += 2) {
                JSValue jsvKey = JS_GetPropertyUint32(ctx, argv[1], i);
                JSValue jsvValue = JS_GetPropertyUint32(ctx, argv[1], i + 1);
                if (JS_IsString(ctx, jsvKey) && JS_IsString(ctx, jsvValue)) {
                    const char *key = JS_ToCString(ctx, jsvKey, &stringBuffer);
                    const char *value = JS_ToCString(ctx, jsvValue, &stringBuffer);
                    http.addHeader(key ? key : "", value ? value : "");
                }
            }
        }
    }

    // options object handling (body, method, responseType, headers)
    const char *bodyRequest = NULL;
    size_t bodyRequestLength = 0U;
    const char *requestType = "GET";
    uint8_t returnResponseType = 0; // 0 = string, 1 = arraybuffer, 2 = json

    if (argc > 1 && JS_IsObject(ctx, argv[1])) {
        JSValue jsvBody = JS_GetPropertyStr(ctx, argv[1], "body");
        if (!JS_IsUndefined(jsvBody)) {
            if (JS_IsString(ctx, jsvBody) || JS_IsNumber(ctx, jsvBody) || JS_IsBool(jsvBody)) {
                bodyRequest = JS_ToCString(ctx, jsvBody, &stringBuffer);
            } else if (JS_IsObject(ctx, jsvBody)) {
                JSValue global = JS_GetGlobalObject(ctx);
                JSValue json = JS_GetPropertyStr(ctx, global, "JSON");
                JSValue stringify = JS_GetPropertyStr(ctx, json, "stringify");
                if (JS_IsFunction(ctx, stringify)) {
                    JS_PushArg(ctx, jsvBody);
                    JS_PushArg(ctx, stringify);
                    JS_PushArg(ctx, json);
                    JSValue jsvBodyRequest = JS_Call(ctx, 1);
                    if (!JS_IsException(jsvBodyRequest) &&
                        (JS_IsString(ctx, jsvBodyRequest) || JS_IsNumber(ctx, jsvBodyRequest) ||
                         JS_IsBool(jsvBodyRequest))) {
                        bodyRequest = JS_ToCString(ctx, jsvBodyRequest, &stringBuffer);
                    }
                }
            }
            bodyRequestLength = bodyRequest == NULL ? 0U : strlen(bodyRequest);
        }

        JSValue jsvMethod = JS_GetPropertyStr(ctx, argv[1], "method");
        if (!JS_IsUndefined(jsvMethod) && JS_IsString(ctx, jsvMethod)) {
            requestType = JS_ToCString(ctx, jsvMethod, &stringBuffer);
        }

        JSValue jsvResponseType = JS_GetPropertyStr(ctx, argv[1], "responseType");
        if (!JS_IsUndefined(jsvResponseType) && JS_IsString(ctx, jsvResponseType)) {
            const char *responseType = JS_ToCString(ctx, jsvResponseType, &stringBuffer);
            if (strcmp(responseType, "binary") == 0) {
                returnResponseType = 1;
            } else if (strcmp(responseType, "json") == 0) {
                returnResponseType = 2;
            }
        }

        // headers inside options
        JSValue jsvHeaders = JS_GetPropertyStr(ctx, argv[1], "headers");
        if (!JS_IsUndefined(jsvHeaders)) {
            if (JS_GetClassID(ctx, jsvHeaders) == JS_CLASS_ARRAY) {
                JSValue l = JS_GetPropertyStr(ctx, jsvHeaders, "length");
                if (JS_IsNumber(ctx, l)) {
                    uint32_t len = 0;
                    JS_ToUint32(ctx, &len, l);
                    for (uint32_t i = 0; i + 1 < len; i += 2) {
                        JSValue jsvKey = JS_GetPropertyUint32(ctx, jsvHeaders, i);
                        JSValue jsvValue = JS_GetPropertyUint32(ctx, jsvHeaders, i + 1);
                        if (JS_IsString(ctx, jsvKey) && JS_IsString(ctx, jsvValue)) {
                            const char *key = JS_ToCString(ctx, jsvKey, &stringBuffer);
                            const char *value = JS_ToCString(ctx, jsvValue, &stringBuffer);
                            http.addHeader(key ? key : "", value ? value : "");
                        }
                    }
                }
            } else if (JS_IsObject(ctx, jsvHeaders)) {
                uint32_t prop_count = 0;
                for (uint32_t index = 0;; ++index) {
                    const char *key = JS_GetOwnPropertyByIndex(ctx, index, &prop_count, jsvHeaders);
                    if (key == NULL) break;
                    JSValue hv = JS_GetPropertyStr(ctx, jsvHeaders, key);
                    if (!JS_IsUndefined(hv) &&
                        (JS_IsString(ctx, hv) || JS_IsNumber(ctx, hv) || JS_IsBool(hv))) {
                        const char *val = JS_ToCString(ctx, hv, &stringBuffer);
                        http.addHeader(key, val ? val : "");
                    }
                }
            }
        }
    }

    http.collectAllHeaders(true);

    // Send HTTP request
    // MEMO: Docs is wrong: sendRequest returns httpResponseCode not
    // Content-Length
    int httpResponseCode = http.sendRequest(requestType, (uint8_t *)bodyRequest, bodyRequestLength);
    if (httpResponseCode <= 0) {
        return JS_ThrowInternalError(ctx, http.errorToString(httpResponseCode).c_str());
    }

    WiFiClient *stream = http.getStreamPtr();

    int contentLength = http.getSize();
    bool isChunked = false;
    if (contentLength == -1) {
        String transferEncoding = http.header("transfer-encoding");
        isChunked = transferEncoding.equalsIgnoreCase("chunked");
    }

    bool psramFoundValue = psramFound();

    size_t payloadCap = 1; // MEMO: 1 for null terminated string
    char *payload = NULL;
    if (!isChunked) {
        payloadCap = contentLength < 1 ? (psramFoundValue ? 16384 : 4096) : (size_t)contentLength + 1;
        payload = (char *)(psramFoundValue ? ps_malloc(payloadCap) : malloc(payloadCap));
        if (payload == NULL) {
            http.end();
            return JS_ThrowInternalError(ctx, "httpFetch: Memory allocation failed!");
        }
    }

    unsigned long startMillis = millis();
    const unsigned long timeoutMillis = 30000;

    size_t bytesRead = 0;
    while (http.connected()) {
        if (millis() - startMillis > timeoutMillis) {
            Serial.println("Timeout while reading response!");
            break;
        }

        if (isChunked) { // if header Transfer-Encoding: chunked
            // Read chunk size
            String chunkSizeStr = stream->readStringUntil('\r');
            stream->read();                                         // Consume '\n'
            int chunkSize = strtol(chunkSizeStr.c_str(), NULL, 16); // Convert hex to int
            if (chunkSize == 0) break;                              // Last chunk
            contentLength += chunkSize;
            if (payload == NULL) {
                payloadCap = (size_t)contentLength + 1;
                payload = (char *)(psramFoundValue ? ps_malloc(payloadCap) : malloc(payloadCap));
            } else {
                payloadCap = (size_t)contentLength + 1;
                payload = (char *)(psramFoundValue ? ps_realloc(payload, payloadCap)
                                                   : realloc(payload, payloadCap));
            }
            if (payload == NULL) {
                http.end();
                return JS_ThrowInternalError(ctx, "httpFetch: Memory allocation failed!");
            }
            // Read chunk data
            int toRead = chunkSize;
            while (toRead > 0) {
                int readNow = stream->readBytes(payload + bytesRead, toRead);
                if (readNow <= 0) break;
                bytesRead += readNow;
                toRead -= readNow;
            }
            // Consume trailing "\r\n" after chunk
            stream->read();
            stream->read();
        } else {
            int streamSize = stream->available();
            if (streamSize > 0) {
                size_t toRead = (streamSize > 512) ? 512 : streamSize;
                if ((bytesRead + toRead + 1) > payloadCap) break;
                int bytesReceived = stream->readBytes(payload + bytesRead, toRead);
                bytesRead += bytesReceived;
            } else {
                delay(1);
            }
            if ((bytesRead + 1) >= payloadCap) break;
        }
        delay(1);
    }

    JSValue headersObj = JS_NewObject(ctx);
    for (size_t i = 0; i < http.headers(); i++) {
        JS_SetPropertyStr(
            ctx, headersObj, http.headerName(i).c_str(), JS_NewString(ctx, http.header(i).c_str())
        );
    }

    JSValue obj = JS_NewObject(ctx);

    // Check if save option is present
    bool hasSave = false;
    if (argc > 1 && JS_IsObject(ctx, argv[1])) {
        JSValue jsvSave = JS_GetPropertyStr(ctx, argv[1], "save");
        hasSave = !JS_IsUndefined(jsvSave);
    }

    // Only set body property if save is not specified
    if (!hasSave) {
        if (returnResponseType == 0) {
            JS_SetPropertyStr(ctx, obj, "body", JS_NewStringLen(ctx, (const char *)payload, bytesRead));
        } else if (returnResponseType == 1) {
            JS_SetPropertyStr(
                ctx, obj, "body", JS_NewUint8ArrayCopy(ctx, (const uint8_t *)payload, bytesRead)
            );
        } else {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload, bytesRead);
            if (error) {
                free(payload);
                http.end();
                return JS_ThrowInternalError(ctx, "deserializeJson failed: %s", error.c_str());
            }
            JS_SetPropertyStr(ctx, obj, "body", js_value_from_json_variant(ctx, doc.as<JsonVariantConst>()));
        }
    }

    // Handle save option
    if (argc > 1 && JS_IsObject(ctx, argv[1])) {
        JSValue jsvSave = JS_GetPropertyStr(ctx, argv[1], "save");
        if (!JS_IsUndefined(jsvSave)) {
            // Prepare arguments for native_storageWrite
            JSValue storageWriteArgs[4];
            int storageArgc = 2; // path and data are required

            // Set path argument
            storageWriteArgs[0] = jsvSave; // This handles both string and object forms

            // Set data argument - create JSValue from payload
            storageWriteArgs[1] = JS_NewStringLen(ctx, (const char *)payload, bytesRead);

            // Handle optional mode and position from save object
            if (JS_IsObject(ctx, jsvSave)) {
                JSValue jsvMode = JS_GetPropertyStr(ctx, jsvSave, "mode");
                if (!JS_IsUndefined(jsvMode) && JS_IsString(ctx, jsvMode)) {
                    storageWriteArgs[2] = jsvMode;
                    storageArgc = 3;

                    JSValue jsvPosition = JS_GetPropertyStr(ctx, jsvSave, "position");
                    if (!JS_IsUndefined(jsvPosition) &&
                        (JS_IsNumber(ctx, jsvPosition) || JS_IsString(ctx, jsvPosition))) {
                        storageWriteArgs[3] = jsvPosition;
                        storageArgc = 4;
                    }
                }
            }

            // Call native_storageWrite
            JSValue writeResult = native_storageWrite(ctx, this_val, storageArgc, storageWriteArgs);
            bool saveSuccess = JS_ToBool(ctx, writeResult);

            // Get the actual saved path for response
            FileParamsJS fileParams;
            JSValue tempArgv[1] = {jsvSave};
            fileParams = js_get_path_from_params(ctx, tempArgv, true);
            if (!fileParams.path.startsWith("/")) fileParams.path = "/" + fileParams.path;

            JS_SetPropertyStr(ctx, obj, "saved", JS_NewBool(saveSuccess));
            JS_SetPropertyStr(ctx, obj, "savedPath", JS_NewString(ctx, fileParams.path.c_str()));
        }
    }

    free(payload);
    JS_SetPropertyStr(ctx, obj, "length", JS_NewInt32(ctx, contentLength));
    JS_SetPropertyStr(ctx, obj, "headers", headersObj);
    JS_SetPropertyStr(ctx, obj, "response", JS_NewInt32(ctx, httpResponseCode));
    JS_SetPropertyStr(ctx, obj, "status", JS_NewInt32(ctx, httpResponseCode));
    JS_SetPropertyStr(ctx, obj, "ok", JS_NewBool(httpResponseCode >= 200 && httpResponseCode < 300));

    http.end();
    return obj;
}

JSValue native_wifiMACAddress(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    String macAddress = WiFi.macAddress();
    return JS_NewString(ctx, macAddress.c_str());
}

JSValue native_ipAddress(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (wifiConnected) {
        String ipAddress = WiFi.localIP().toString();
        return JS_NewString(ctx, ipAddress.c_str());
    }
    return JS_NULL;
}
#endif
