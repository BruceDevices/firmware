#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "wifi_js.h"

#include "core/wifi/wifi_common.h"
#include "helpers_js.h"
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
    uint8_t returnResponseType = 0; // 0 = string

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

        jsvBody = JS_GetPropertyStr(ctx, argv[1], "method");
        if (!JS_IsUndefined(jsvBody) && JS_IsString(ctx, jsvBody)) {
            requestType = JS_ToCString(ctx, jsvBody, &stringBuffer);
        }

        jsvBody = JS_GetPropertyStr(ctx, argv[1], "responseType");
        if (!JS_IsUndefined(jsvBody) && JS_IsString(ctx, jsvBody)) {
            const char *rts = JS_ToCString(ctx, jsvBody, &stringBuffer);
            returnResponseType = (strcmp(rts ? rts : "string", "string") == 0) ? 0 : 1;
        }

        // headers inside options
        jsvBody = JS_GetPropertyStr(ctx, argv[1], "headers");
        if (!JS_IsUndefined(jsvBody)) {
            if (JS_GetClassID(ctx, jsvBody) == JS_CLASS_ARRAY) {
                JSValue l = JS_GetPropertyStr(ctx, jsvBody, "length");
                if (JS_IsNumber(ctx, l)) {
                    uint32_t len = 0;
                    JS_ToUint32(ctx, &len, l);
                    for (uint32_t i = 0; i + 1 < len; i += 2) {
                        JSValue jsvKey = JS_GetPropertyUint32(ctx, jsvBody, i);
                        JSValue jsvValue = JS_GetPropertyUint32(ctx, jsvBody, i + 1);
                        if (JS_IsString(ctx, jsvKey) && JS_IsString(ctx, jsvValue)) {
                            const char *key = JS_ToCString(ctx, jsvKey, &stringBuffer);
                            const char *value = JS_ToCString(ctx, jsvValue, &stringBuffer);
                            http.addHeader(key ? key : "", value ? value : "");
                        }
                    }
                }
            } else if (JS_IsObject(ctx, jsvBody)) {
                uint32_t prop_count = 0;
                for (uint32_t index = 0;; ++index) {
                    const char *key = JS_GetOwnPropertyByIndex(ctx, index, &prop_count, jsvBody);
                    if (key == NULL) break;
                    JSValue hv = JS_GetPropertyStr(ctx, jsvBody, key);
                    if (!JS_IsUndefined(hv) &&
                        (JS_IsString(ctx, hv) || JS_IsNumber(ctx, hv) || JS_IsBool(hv))) {
                        const char *val = JS_ToCString(ctx, hv, &stringBuffer);
                        http.addHeader(key, val ? val : "");
                    }
                }
            }
        }
    }

    // send
    int httpResponseCode = http.sendRequest(requestType, (uint8_t *)bodyRequest, bodyRequestLength);
    if (httpResponseCode <= 0) {
        return JS_ThrowTypeError(ctx, http.errorToString(httpResponseCode).c_str());
    }

    WiFiClient *stream = http.getStreamPtr();
    String payload = "";
    unsigned long startMillis = millis();
    const unsigned long timeoutMillis = 30000;
    while (http.connected()) {
        if (millis() - startMillis > timeoutMillis) break;
        while (stream->available()) { payload += (char)stream->read(); }
        delay(1);
    }

    JSValue headersObj = JS_NewObject(ctx);
    for (size_t i = 0; i < http.headers(); i++) {
        JS_SetPropertyStr(
            ctx, headersObj, http.headerName(i).c_str(), JS_NewString(ctx, http.header(i).c_str())
        );
    }

    JSValue obj = JS_NewObject(ctx);
    if (returnResponseType == 0) {
        JS_SetPropertyStr(ctx, obj, "body", JS_NewStringLen(ctx, payload.c_str(), payload.length()));
    } else {
        JS_SetPropertyStr(ctx, obj, "body", JS_NewStringLen(ctx, payload.c_str(), payload.length()));
    }
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
