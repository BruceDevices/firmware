#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "wifi_js.h"

#include "core/wifi/wifi_common.h"
#include "helpers_js.h"
#include <HTTPClient.h>
#include <WiFi.h>

duk_ret_t putPropWiFiFunctions(duk_context *ctx, duk_idx_t obj_idx, uint8_t magic) {
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "connected", native_wifiConnected, 0, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "connect", native_wifiConnect, 3, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "connectDialog", native_wifiConnectDialog, 0, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "disconnect", native_wifiDisconnect, 0, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "scan", native_wifiScan, 0, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "httpFetch", native_httpFetch, 2, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "getMACAddress", native_wifiMACAddress, 0, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "getIPAddress", native_ipAddress, 0, magic);
    return 0;
}

duk_ret_t registerWiFi(duk_context *ctx) {
    bduk_register_c_lightfunc(ctx, "wifiConnect", native_wifiConnect, 3);
    bduk_register_c_lightfunc(ctx, "wifiConnectDialog", native_wifiConnectDialog, 0);
    bduk_register_c_lightfunc(ctx, "wifiDisconnect", native_wifiDisconnect, 0);
    bduk_register_c_lightfunc(ctx, "wifiScan", native_wifiScan, 0);
    bduk_register_c_lightfunc(ctx, "httpFetch", native_httpFetch, 2, 0);
    bduk_register_c_lightfunc(ctx, "httpGet", native_httpFetch, 2, 0);
    bduk_register_c_lightfunc(ctx, "wifiMACAddress", native_wifiMACAddress, 0);
    bduk_register_c_lightfunc(ctx, "wifiIPAddress", native_ipAddress, 0);
    return 0;
}

duk_ret_t native_wifiConnected(duk_context *ctx) {
    duk_push_boolean(ctx, wifiConnected);
    return 1;
}

duk_ret_t native_wifiConnectDialog(duk_context *ctx) {
    bool connected = wifiConnectMenu();
    duk_push_boolean(ctx, connected);
    return 1;
}

duk_ret_t native_wifiConnect(duk_context *ctx) {
    // usage: wifiConnect(ssid : string )
    // usage: wifiConnect(ssid : string, timeout_in_seconds : int)
    // usage: wifiConnect(ssid : string, timeout_in_seconds : int, pwd : string)
    String ssid = duk_to_string(ctx, 0);
    int timeout_in_seconds = 10;
    if (duk_is_number(ctx, 1)) timeout_in_seconds = duk_to_int(ctx, 1);

    bool r = false;

    Serial.println("Connecting to: " + ssid);

    WiFi.mode(WIFI_MODE_STA);
    if (duk_is_string(ctx, 2)) {
        String pwd = duk_to_string(ctx, 2);
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
        wifiIP = WiFi.localIP().toString(); // update global var
        wifiConnected = true;
    }

    duk_push_boolean(ctx, r);
    return 1;
}

const char *wifi_enc_types[] = {
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

duk_ret_t native_wifiScan(duk_context *ctx) {
    WiFi.mode(WIFI_MODE_STA);
    int nets = WiFi.scanNetworks();
    duk_idx_t arr_idx = duk_push_array(ctx);
    int arrayIndex = 0;
    duk_idx_t obj_idx;

    for (int i = 0; i < nets; i++) {
        obj_idx = duk_push_object(ctx);
        int enctypeInt = int(WiFi.encryptionType(i));

        const char *enctype = enctypeInt < 12 ? wifi_enc_types[enctypeInt] : "UNKNOWN";
        bduk_put_prop(ctx, obj_idx, "encryptionType", duk_push_string, enctype);
        bduk_put_prop(ctx, obj_idx, "SSID", duk_push_string, WiFi.SSID(i).c_str());
        bduk_put_prop(ctx, obj_idx, "MAC", duk_push_string, WiFi.BSSIDstr(i).c_str());
        duk_put_prop_index(ctx, arr_idx, arrayIndex);
        arrayIndex++;
    }
    return 1;
}

duk_ret_t native_wifiDisconnect(duk_context *ctx) {
    wifiDisconnect();
    return 0;
}

duk_ret_t native_httpFetch(duk_context *ctx) {
    HTTPClient http;

    http.setReuse(false);

    if (WiFi.status() != WL_CONNECTED) wifiConnectMenu();

    if (WiFi.status() != WL_CONNECTED) { return duk_error(ctx, DUK_ERR_ERROR, "WIFI Not Connected"); }

    // Your Domain name with URL path or IP address with path
    http.begin(duk_to_string(ctx, 0));

    // Add Headers if headers are included.
    if (duk_is_array(ctx, 1)) {
        // Get the length of the array
        duk_uint_t len = duk_get_length(ctx, 1);
        for (duk_uint_t i = 0; i < len; i++) {
            // Get each element in the array
            duk_get_prop_index(ctx, 1, i);

            // Ensure it's a string
            if (!duk_is_string(ctx, -1)) {
                duk_pop(ctx);
                return duk_error(
                    ctx, DUK_ERR_TYPE_ERROR, "%s: Header array elements must be strings.", "httpFetch"
                );
            }

            // Get the string
            const char *headerKey = duk_get_string(ctx, -1);
            duk_pop(ctx);
            i++;
            duk_get_prop_index(ctx, 1, i);

            // Ensure it's a string
            if (!duk_is_string(ctx, -1)) {
                return duk_error(
                    ctx, DUK_ERR_TYPE_ERROR, "%s: Header array elements must be strings.", "httpFetch"
                );
            }

            // Get the string
            const char *headerValue = duk_get_string(ctx, -1);
            duk_pop(ctx);
            http.addHeader(headerKey, headerValue);
        }
    }

    const char *bodyRequest = NULL;
    size_t bodyRequestLength = 0U;

    const char *requestType = "GET";
    uint8_t returnResponseType = 0;

    if (duk_is_object(ctx, 1)) {
        if (duk_get_prop_string(ctx, 1, "body")) {
            duk_uint_t arg1Type = duk_get_type_mask(ctx, -1);
            if (arg1Type & (DUK_TYPE_MASK_STRING | DUK_TYPE_MASK_NUMBER | DUK_TYPE_MASK_BOOLEAN)) {
                bodyRequest = duk_to_string(ctx, -1);
            } else if (arg1Type & DUK_TYPE_MASK_OBJECT) {
                // JSON.stringify body if it's object type
                duk_push_global_object(ctx);       /* -> [ global ] */
                duk_push_string(ctx, "JSON");      /* -> [ global "JSON" ] */
                duk_get_prop(ctx, -2);             /* -> [ global JSON ] */
                duk_push_string(ctx, "stringify"); /* -> [ global Object "stringify" ] */
                duk_get_prop(ctx, -2);             /* -> [ global Object stringify ] */

                duk_dup(ctx, 1);
                duk_pcall(ctx, 1);
                bodyRequest = duk_to_string(ctx, -1);
            }
            bodyRequestLength = bodyRequest == NULL ? 0U : strlen(bodyRequest);
        }

        if (duk_get_prop_string(ctx, 1, "method")) { requestType = duk_get_string_default(ctx, -1, "GET"); }

        if (duk_get_prop_string(ctx, 1, "responseType")) {
            const char *returnResponseTypeString = duk_get_string_default(ctx, -1, "string");
            returnResponseType = (strcmp(returnResponseTypeString, "string") == 0);
        }

        if (duk_get_prop_string(ctx, 1, "headers")) {
            bool headersIsArray = duk_is_array(ctx, -1);

            duk_enum(ctx, -1, 0);
            while (duk_next(ctx, -1, 1)) {
                const char *headerKey = NULL;
                const char *headerValue = duk_get_string(ctx, -1);
                if (!headersIsArray) { // If headers is object
                    headerKey = duk_get_string(ctx, -2);
                } else { // If headers is array
                    if (duk_is_string(ctx, -1)) {
                        headerKey = duk_get_string(ctx, -1);
                        duk_pop_2(ctx);
                        duk_bool_t isNextValue = duk_next(ctx, -1, 1);
                        if (!isNextValue) break;
                        headerValue = duk_get_string(ctx, -1);
                    } else if (duk_is_array(ctx, -1)) {
                        duk_get_prop_index(ctx, -1, 0);
                        headerKey = duk_get_string(ctx, -1);
                        duk_get_prop_index(ctx, -2, 1);
                        headerValue = duk_get_string(ctx, -1);
                        if (!duk_is_string(ctx, -1) || !duk_is_string(ctx, -2)) {
                            duk_error(
                                ctx,
                                DUK_ERR_TYPE_ERROR,
                                "%s: Header array elements must be strings.",
                                "httpFetch"
                            );
                        }
                        duk_pop_2(ctx);
                    } else {
                        duk_error(
                            ctx, DUK_ERR_TYPE_ERROR, "%s: Header array elements must be strings.", "httpFetch"
                        );
                    }
                }
                duk_pop_2(ctx);
                http.addHeader(headerKey, headerValue);
            }
        }
    }

    // HTTPClient doesn't store headers unless you explicitly use collectHeaders
    // TODO: Collect all headers manually
    const char *headersKeys[] = {
        "Content-Type", "Content-Length", "Transfer-Encoding", "Connection", "Cache-Control", "Date", "Server"
    };
    http.collectHeaders(headersKeys, 7);

    // Send HTTP request
    // MEMO: Docs is wrong: sendRequest returns httpResponseCode not
    // Content-Length
    int httpResponseCode = http.sendRequest(requestType, (uint8_t *)bodyRequest, bodyRequestLength);

    if (httpResponseCode <= 0) {
        return duk_error(ctx, DUK_ERR_ERROR, http.errorToString(httpResponseCode).c_str());
    }

    WiFiClient *stream = http.getStreamPtr();

    int contentLength = http.getSize();
    bool isChunked = false;
    if (contentLength == -1) {
        String transferEncoding = http.header("transfer-encoding");
        isChunked = transferEncoding.equalsIgnoreCase("chunked");
    }

    duk_idx_t headersObjectIdx = duk_push_object(ctx);
    for (size_t i = 0; i < http.headers(); i++) {
        bduk_put_prop(
            ctx, headersObjectIdx, http.headerName(i).c_str(), duk_push_string, http.header(i).c_str()
        );
    }

    bool psramFoundValue = psramFound();
    int payloadSize = 1; // MEMO: 1 for null terminated string
    char *payload = NULL;
    duk_idx_t obj_idx = duk_push_object(ctx);
    if (!isChunked) {
        payloadSize = contentLength < 1 ? (psramFoundValue ? 16384 : 4096) : contentLength + 1;
        payload = (char *)duk_push_fixed_buffer(ctx, payloadSize);

        if (payload == NULL) {
            return duk_error(ctx, DUK_ERR_ERROR, "%s: Memory allocation failed!", "httpFetch");
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

            payloadSize += chunkSize;
            if (payload == NULL) {
                payload = (char *)duk_push_dynamic_buffer(ctx, payloadSize);
            } else {
                payload = (char *)duk_resize_buffer(ctx, -1, payloadSize);
            }

            if (payload == NULL) {
                return duk_error(ctx, DUK_ERR_ERROR, "%s: Memory allocation failed!", "httpFetch");
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
                if ((bytesRead + toRead + 1) > payloadSize) break;
                int bytesReceived = stream->readBytes(payload + bytesRead, toRead);

                bytesRead += bytesReceived;
            } else {
                delay(1);
            }
            if ((bytesRead + 1) >= payloadSize) break;
        }
        startMillis = millis();
    }

    if (returnResponseType == 0) {
        void *buffer_data = duk_get_buffer_data(ctx, -1, NULL);
        if (buffer_data != NULL && bytesRead > 0) {
            // Create string from buffer data while buffer is still on stack
            duk_push_lstring(ctx, (const char *)buffer_data, bytesRead);
            duk_remove(ctx, -2); // Remove the buffer, keep the string
        } else {
            duk_pop(ctx);             // Remove buffer from stack
            duk_push_string(ctx, ""); // Empty string if no data
        }
    } else {
        duk_push_buffer_object(ctx, -1, 0, bytesRead, DUK_BUFOBJ_UINT8ARRAY);
    }
    duk_put_prop_string(ctx, obj_idx, "body");
    bduk_put_prop(ctx, obj_idx, "length", duk_push_int, contentLength);
    bduk_put_prop(ctx, obj_idx, "response", duk_push_int, httpResponseCode);
    bduk_put_prop(ctx, obj_idx, "status", duk_push_int, httpResponseCode);
    bduk_put_prop(ctx, obj_idx, "ok", duk_push_boolean, httpResponseCode >= 200 && httpResponseCode < 300);

    // Free resources
    http.end();
    return 1;
}

duk_ret_t native_wifiMACAddress(duk_context *ctx) {
    String macAddress = WiFi.macAddress();
    duk_push_string(ctx, macAddress.c_str());
    return 1;
}

duk_ret_t native_ipAddress(duk_context *ctx) {
    if (wifiConnected) {
        String ipAddress = WiFi.localIP().toString();
        duk_push_string(ctx, ipAddress.c_str());
    } else {
        duk_push_null(ctx);
    }
    return 1;
}
#endif
