#include "webInterface.h"
#include "core/display.h"    // using displayRedStripe as error msg
#include "core/mykeyboard.h" // using keyboard when calling rename
#include "core/passwords.h"
#include "core/ram_profile.h"
#include "core/sd_functions.h" // using sd functions called to rename and manage sd files
#include "core/serialcmds.h"
#include "core/settings.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h" // using common wifisetup
#include "esp_task_wdt.h"
#include "webFiles.h"
#include <MD5Builder.h>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#include <strings.h>
#include <unistd.h>
#include <esp32-hal-psram.h>
#include <esp_heap_caps.h>
#include <globals.h>

// WiFi as a Client
const int default_webserverporthttp = 80;

// WiFi as an Access Point
IPAddress AP_GATEWAY(172, 0, 0, 1); // Gateway

AsyncWebServer *server = nullptr; // initialise webserver
const char *host = "bruce";
static bool mdnsRunning = false;

// Generate random token
String generateToken(int length = 24) {
    String token = "";
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < length; i++) { token += charset[random(0, sizeof(charset) - 1)]; }
    return token;
}

/**********************************************************************
**  Function: stopWebUi
**  Turn off the WebUI
**********************************************************************/
void stopWebUi() {
    tft.setLogging(false);
    isWebUIActive = false;
    server->end();
    server->~AsyncWebServer();
    free(server);
    server = nullptr;
    if (mdnsRunning) {
        MDNS.end();
        mdnsRunning = false;
    }
}

/**********************************************************************
**  Function: cleanlyStopWebUiForWiFiFeature
**  Cleanly stop WebUI and AP mode before starting a WiFi feature
**  This prevents WiFi mode conflicts when features need exclusive control
**********************************************************************/
void cleanlyStopWebUiForWiFiFeature() {
    // Only proceed if WebUI is active
    if (!isWebUIActive && !server) { return; }

    // Brief notification (non-blocking)
    Serial.println("Stopping WebUI for WiFi feature...");

    // Stop the WebUI
    if (server) {
        stopWebUi();
        // Give the web server time to fully shut down
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Disconnect WiFi AP mode if it's the WebUI's AP
    // Check if we're in AP or APSTA mode (used by WebUI)
    wifi_mode_t currentMode = WiFi.getMode();
    if (currentMode == WIFI_MODE_AP || currentMode == WIFI_MODE_APSTA) {
        wifiDisconnect();
        // Give WiFi time to fully disconnect
        vTaskDelay(pdMS_TO_TICKS(250));
    }

    Serial.println("WebUI stopped, starting WiFi feature...");
}
/**********************************************************************
**  Function: loopOptionsWebUi
**  Display options to launch the WebUI
**********************************************************************/
void loopOptionsWebUi() {
    if (isWebUIActive) {
        bool opt = WiFi.getMode() - 1;
        options = {
            {"Stop WebUI", stopWebUi},
            {"WebUi screen", lambdaHelper(startWebUi, opt)}
        };
        addOptionToMainMenu();
        loopOptions(options);
        return;
    }
    options = {
        {"my Network", lambdaHelper(startWebUi, false)},
        {"AP mode",    lambdaHelper(startWebUi, true) },
    };

    loopOptions(options);
    // On fail installing will run the following line
}

/**********************************************************************
**  Function: humanReadableSize
** Make size of files human readable
** source: https://github.com/CelliesProjects/minimalUploadAuthESP32
**********************************************************************/
String humanReadableSize(uint64_t bytes) {
    if (bytes < 1024) return String(bytes) + " B";
    else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " kB";
    else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
    else return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}

/**********************************************************************
**  Function: listFiles
**  list all of the files, if ishtml=true, return html rather than simple text
**********************************************************************/
String listFiles(FS &fs, const String &folder) {
    // log_i("Listfiles Start");
    String returnText = "pa:" + folder + ":0\n";
    // Serial.println("Listing files stored on SD");

    File root = fs.open(folder);

    while (true) {
        bool isDir;
        String fullPath = root.getNextFileName(&isDir);
        String nameOnly = fullPath.substring(fullPath.lastIndexOf("/") + 1);
        if (fullPath == "") { break; }
        // Serial.printf("Path: %s (isDir: %d)\n", fullPath.c_str(), isDir);

        if (esp_get_free_heap_size() > (String("Fo:" + nameOnly + ":0\n").length()) + 1024) {
            if (isDir) {
                // Serial.printf("Directory: %s\n", fullPath.c_str());
                returnText += "Fo:" + nameOnly + ":0\n";
            } else {
                // For files, we need to get the size, so we open the file briefly
                // Serial.printf("Opening file for size check: %s\n", fullPath.c_str());
                File file = fs.open(fullPath);
                // Serial.printf("File size: %llu bytes\n", file.size());
                if (file) {
                    returnText += "Fi:" + nameOnly + ":" + humanReadableSize(file.size()) + "\n";
                    file.close();
                }
            }
        } else break;
        delay(1);
    }
    root.close();
    // log_i("ListFiles End");
    return returnText;
}

/**********************************************************************
**  Function: checkUserWebAuth
** used by server->on functions to discern whether a user has the correct
** httpapitoken OR is authenticated by username and password
**********************************************************************/
bool checkUserWebAuth(AsyncWebServerRequest *request, bool onFailureReturnLoginPage = false) {
    if (request->hasHeader("Cookie")) {
        const AsyncWebHeader *cookie = request->getHeader("Cookie");
        String c = cookie->value();
        int idx = c.indexOf("BRUCESESSION=");
        if (idx != -1) {
            int start = idx + 13;
            int end = c.indexOf(';', start);
            if (end == -1) end = c.length();
            String token = c.substring(start, end);
            if (bruceConfig.isValidWebUISession(token)) { return true; }
        }
    }
    if (onFailureReturnLoginPage) {
        serveWebUIFile(request, "login.html", "text/html", true, login_html, login_html_size);
    } else {
        request->send(401, "text/plain", "Unauthorized");
    }
    return false;
}

namespace {
constexpr char uploadPrefix[] = ".bruce-upload-";

struct WebUpload {
    FS *fs = nullptr;
    String destination;
    String temporary;
    FILE *file = nullptr;
    size_t received = 0;
    bool encrypted = false;
    bool finished = false;

    ~WebUpload() { discard(); }

    void discard() {
        if (file) fclose(file);
        file = nullptr;
        if (temporary.length()) fs->remove(temporary);
        temporary.clear();
    }
};

bool validUploadPath(const String &path, bool absolute) {
    if (!path.length() || path.startsWith("/") != absolute) return false;
    if (absolute && path == "/") return true;
    size_t start = absolute ? 1 : 0;
    while (start < path.length()) {
        const int slash = path.indexOf('/', start);
        const size_t end = slash < 0 ? path.length() : slash;
        if (end == start || path[end - 1] == '.' || path[end - 1] == ' ') return false;
        if (strncasecmp(path.c_str() + start, uploadPrefix, sizeof(uploadPrefix) - 1) == 0) return false;
        for (size_t i = start; i < end; ++i) {
            const unsigned char c = path[i];
            if (c < 32 || c == 127 || strchr("\\:*?\"<>|", c)) return false;
        }
        if (slash < 0) return true;
        start = end + 1;
    }
    return false;
}

bool makeUploadDirectories(FS &fs, const String &directory) {
    for (size_t end = 1; end < directory.length(); ++end) {
        if (directory[end] != '/') continue;
        const String part = directory.substring(0, end);
        if (!fs.exists(part) && !fs.mkdir(part) && !fs.exists(part)) return false;
    }
    return true;
}

WebUpload *uploadState(AsyncWebServerRequest *request) {
    const auto slot = static_cast<WebUpload **>(request->_tempObject);
    return slot ? *slot : nullptr;
}

const AsyncWebParameter *uploadPassword(AsyncWebServerRequest *request) {
    const auto *queryPassword = request->getParam("password");
    return queryPassword ? queryPassword : request->getParam("password", true);
}

void failUpload(AsyncWebServerRequest *request, int status, const char *message) {
    if (auto *upload = uploadState(request)) upload->discard();
    request->send(status, "text/plain", message);
}

WebUpload *beginUpload(AsyncWebServerRequest *request, String filename) {
    if (!checkUserWebAuth(request)) return nullptr;

    const auto *drive = request->getParam("fs");
    const auto *folder = request->getParam("folder");
    if (!drive || !folder || !validUploadPath(filename, false)) {
        failUpload(request, 400, "Invalid upload destination");
        return nullptr;
    }
    String directory = folder->value();
    if (directory.length() > 1 && directory.endsWith("/")) directory.remove(directory.length() - 1);
    if (!validUploadPath(directory, true) || (drive->value() != "SD" && drive->value() != "LittleFS")) {
        failUpload(request, 400, "Invalid upload destination");
        return nullptr;
    }

    const bool useSd = drive->value() == "SD";
    FS &fs = useSd ? static_cast<FS &>(SD) : static_cast<FS &>(LittleFS);
    if ((useSd && !setupSdCard()) || !fs.mountpoint()) {
        failUpload(request, 503, "Upload storage unavailable");
        return nullptr;
    }

    auto state = std::shared_ptr<WebUpload>(new (std::nothrow) WebUpload);
    auto slot = state ? static_cast<WebUpload **>(malloc(sizeof(WebUpload *))) : nullptr;
    if (!slot) {
        failUpload(request, 503, "Unable to start upload");
        return nullptr;
    }
    *slot = state.get();
    request->_tempObject = slot; // ESPAsyncWebServer frees the slot and owns the cleanup callback.
    request->onDisconnect([state] { state->discard(); });

    state->fs = &fs;
    state->encrypted = uploadPassword(request) != nullptr;
    if (state->encrypted) filename += ".enc";
    state->destination = (directory == "/" ? directory : directory + "/") + filename;
    const String parent = state->destination.substring(0, state->destination.lastIndexOf('/') + 1);
    if (!parent.length() || !makeUploadDirectories(fs, parent)) {
        failUpload(request, 507, "Unable to create upload directory");
        return nullptr;
    }

    const char *mount = fs.mountpoint();
    if (!mount) {
        failUpload(request, 503, "Upload storage unavailable");
        return nullptr;
    }
    String staged = String(mount) + parent + uploadPrefix + "XXXXXX";
    const int fd = staged.length() ? mkstemp(staged.begin()) : -1;
    if (fd >= 0) {
        state->temporary = staged.c_str() + strlen(mount);
        if (state->temporary.length()) state->file = fdopen(fd, "wb");
        if (!state->file) {
            close(fd);
            unlink(staged.c_str());
        }
    }
    if (!state->file) {
        failUpload(request, 507, "Unable to open temporary upload");
        return nullptr;
    }
    return state.get();
}

bool publishUpload(WebUpload &upload) {
    FS &fs = *upload.fs;
    const String &target = upload.destination;
    if (!fs.exists(target)) return fs.rename(upload.temporary, target);

    File existing = fs.open(target, FILE_READ);
    if (!existing || existing.isDirectory()) return false;
    existing.close();

    // FAT requires the destination to be absent. Keep the original until the replacement succeeds.
    const String backup = upload.temporary + ".bak";
    if (!backup.length() || fs.exists(backup) || !fs.rename(target, backup)) return false;
    if (!fs.rename(upload.temporary, target)) {
        if (!fs.rename(backup, target)) log_e("Upload original remains at %s", backup.c_str());
        return false;
    }
    if (!fs.remove(backup)) log_w("Upload backup remains at %s", backup.c_str());
    return true;
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (request->getResponse()) return;
    auto *upload = uploadState(request);
    if (!upload) upload = beginUpload(request, filename);
    if (!upload) return;
    if (upload->finished || index != upload->received) {
        failUpload(request, 400, "Expected a single complete upload");
        return;
    }

    const uint8_t *bytes = data;
    size_t count = len;
    String encrypted;
    if (upload->encrypted) {
        if (!final || !len || memchr(data, '\0', len)) {
            failUpload(request, 400, "Encrypted upload must be a single text chunk");
            return;
        }
        String plaintext(reinterpret_cast<const char *>(data), len);
        encrypted = encryptString(plaintext, uploadPassword(request)->value());
        if (!encrypted.length()) {
            failUpload(request, 500, "Unable to encrypt upload");
            return;
        }
        bytes = reinterpret_cast<const uint8_t *>(encrypted.c_str());
        count = encrypted.length();
    }

    if (count && fwrite(bytes, 1, count, upload->file) != count) {
        failUpload(request, 507, "Unable to write complete upload");
        return;
    }
    upload->received += len;
    upload->finished = final;
}

void finishUpload(AsyncWebServerRequest *request) {
    if (request->getResponse() || !checkUserWebAuth(request)) return;
    auto *upload = uploadState(request);
    if (!upload || !upload->finished || (uploadPassword(request) != nullptr) != upload->encrypted) {
        failUpload(request, 400, "Incomplete or invalid upload");
        return;
    }

    const bool synced = fflush(upload->file) == 0 && fsync(fileno(upload->file)) == 0;
    const bool closed = fclose(upload->file) == 0;
    upload->file = nullptr;
    if (!synced || !closed) {
        failUpload(request, 507, "Unable to finish writing upload");
        return;
    }
    if (!publishUpload(*upload)) {
        failUpload(request, 500, "Unable to replace destination file");
        return;
    }
    upload->temporary.clear();
    request->send(200, "text/plain", "File upload completed");
}
} // namespace

void notFound(AsyncWebServerRequest *request) { request->send(404, "text/plain", "Nothing in here Sharky"); }

/**********************************************************************
**  Function: drawWebUiScreen
**  Draw information on screen of WebUI.
**********************************************************************/
void drawWebUiScreen(bool mode_ap) {
    drawMainBorderWithTitle("WebUI", true);

    String txt;
    if (!mode_ap) txt = WiFi.localIP().toString();
    else txt = WiFi.softAPIP().toString();

    int padX = 14;
    int currentY = 55;

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FP);

    if (mode_ap) {
        tft.setCursor(padX, currentY);
        tft.print("Net: BruceNet/brucenet");
        currentY += LH * FP + 6;
    }

    tft.setCursor(padX, currentY);
    if (mdnsRunning) tft.print("Url: http://bruce.local");
    currentY += LH * FP + 6;

    tft.setCursor(padX, currentY);
    tft.print("IP:  " + txt);
    currentY += LH * FP + 6;

    tft.setCursor(padX, currentY);
    tft.print("Usr: " + String(bruceConfig.webUI.user));
    currentY += LH * FP + 6;

    tft.setCursor(padX, currentY);
    tft.print("Pwd: " + String(bruceConfig.webUI.pwd));

    tft.setTextColor(TFT_RED, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.drawCentreString("press Esc to stop", tftWidth / 2, tftHeight - 2 * LH * FP - 5, 1);

#if defined(HAS_TOUCH)
    TouchFooter();
#endif
}

/**********************************************************************
**  Function: color565ToWebHex
**  convert 565 color to web hex format for theme purposes
**********************************************************************/
String color565ToWebHex(uint16_t color565) {
    // Extract RGB components from 565
    uint8_t r = (color565 >> 11) & 0x1F;
    uint8_t g = (color565 >> 5) & 0x3F;
    uint8_t b = color565 & 0x1F;

    // Scale up to 8 bits
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);

    char hex[8];
    snprintf(hex, sizeof(hex), "#%02X%02X%02X", r, g, b);
    return String(hex);
}

/**********************************************************************
**  Function: serveWebUIFile
**  serves files for WebUI and checks for custom WebUI files
**********************************************************************/
void serveWebUIFile(AsyncWebServerRequest *request, const String &filename, const char *contentType) {
    serveWebUIFile(request, filename, contentType, false, nullptr, 0);
}
void serveWebUIFile(
    AsyncWebServerRequest *request, const String &filename, const char *contentType, bool gzip,
    const uint8_t *originaFile, uint32_t originalFileSize
) {
    AsyncWebServerResponse *response = nullptr;
    FS *fs = NULL;
    if (setupSdCard()) {
        if (SD.exists("/BruceWebUI/" + filename)) fs = &SD;
    } else if (LittleFS.exists("/BruceWebUI/" + filename)) {
        fs = &LittleFS;
    }
    if (fs) {
        response = request->beginResponse(*fs, "/BruceWebUI/" + filename, contentType);
    } else {
        if (filename == "theme.css") {
            String css = ":root{--color:" + color565ToWebHex(bruceConfig.priColor) +
                         ";--sec-color:" + color565ToWebHex(bruceConfig.secColor) +
                         ";--background:" + color565ToWebHex(bruceConfig.bgColor) + ";}";
            AsyncWebServerResponse *themeResponse = request->beginResponse(200, "text/css", css);
            request->send(themeResponse);
            return;
        }
        response = request->beginResponse(200, String(contentType), originaFile, originalFileSize);
        if (gzip) {
            if (!response->addHeader("Content-Encoding", "gzip")) log_e("Failed to add gzip header");
        }
    }
    request->send(response);
}

/**********************************************************************
**  Function: startMdnsResponder
**  Try to start mDNS only if there is enough internal heap available
**********************************************************************/
static bool startMdnsResponder() {
    RAM_LOG("before MDNS");

    if (!MDNS.begin(host)) {
        RAM_LOG("MDNS failed");
        Serial.printf("Error setting up MDNS responder!\n");
        return false;
    }

    RAM_LOG("after MDNS");
    return true;
}

/**********************************************************************
**  Function: configureWebServer
**  configure web server
**********************************************************************/
void configureWebServer() {
    mdnsRunning = startMdnsResponder();
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    server->onNotFound(notFound);

    // Index
    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (checkUserWebAuth(request, true)) {
            serveWebUIFile(request, "index.html", "text/html", true, index_html, index_html_size);
        }
    });

    // Login
    server->on("/login", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("username", true) && request->hasParam("password", true)) {
            String username = request->getParam("username", true)->value();
            String password = request->getParam("password", true)->value();

            if (username == bruceConfig.webUI.user && password == bruceConfig.webUI.pwd) {
                String token = generateToken();
                AsyncWebServerResponse *response = request->beginResponse(302);
                response->addHeader("Location", "/");
                response->addHeader("Set-Cookie", "BRUCESESSION=" + token + "; Path=/; HttpOnly");
                request->send(response);
                bruceConfig.addWebUISession(token);
                return;
            }
        }
        AsyncWebServerResponse *response = request->beginResponse(302);
        response->addHeader("Location", "/?failed");
        request->send(response);
    });

    // Logout
    server->on("/logout", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasHeader("Cookie")) {
            const AsyncWebHeader *cookie = request->getHeader("Cookie");
            String c = cookie->value();
            int idx = c.indexOf("BRUCESESSION=");
            if (idx != -1) {
                int start = idx + 13;
                int end = c.indexOf(';', start);
                if (end == -1) end = c.length();
                String token = c.substring(start, end);
                bruceConfig.removeWebUISession(token);
            }
        }
        AsyncWebServerResponse *response = request->beginResponse(302);
        response->addHeader("Location", "/?loggedout");
        response->addHeader("Set-Cookie", "BRUCESESSION=0; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT");
        request->send(response);
    });

    // Static files
    server->on("/theme.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        serveWebUIFile(request, "theme.css", "text/css");
    });
    server->on("/index.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        serveWebUIFile(request, "index.css", "text/css", true, index_css, index_css_size);
    });
    server->on("/index.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        serveWebUIFile(request, "index.js", "text/javascript", true, index_js, index_js_size);
    });

    // System Info
    server->on("/systeminfo", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (checkUserWebAuth(request)) {
            char response_body[300];
            uint64_t LittleFSTotalBytes = LittleFS.totalBytes();
            uint64_t LittleFSUsedBytes = LittleFS.usedBytes();
            uint64_t SDTotalBytes = SD.totalBytes();
            uint64_t SDUsedBytes = SD.usedBytes();
            snprintf(
                response_body,
                sizeof(response_body),
                "{\"%s\":\"%s\",\"SD\":{\"%s\":\"%s\",\"%s\":\"%s\",\"%s\":\"%s\"},"
                "\"LittleFS\":{\"%s\":\"%s\",\"%s\":\"%s\",\"%s\":\"%s\"}}",
                "BRUCE_VERSION",
                BRUCE_VERSION,
                "free",
                humanReadableSize(SDTotalBytes - SDUsedBytes).c_str(),
                "used",
                humanReadableSize(SDUsedBytes).c_str(),
                "total",
                humanReadableSize(SDTotalBytes).c_str(),
                "free",
                humanReadableSize(LittleFSTotalBytes - LittleFSUsedBytes).c_str(),
                "used",
                humanReadableSize(LittleFSUsedBytes).c_str(),
                "total",
                humanReadableSize(LittleFSTotalBytes).c_str()
            );
            request->send(200, "application/json", response_body);
        }
    });

    // Get Screen
    server->on("/getscreen", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (checkUserWebAuth(request)) {
            static uint8_t *screenBinBuffer = nullptr;
            static size_t screenBinBufferSize = 0;

            if (!screenBinBuffer) {
                size_t desiredSize = MAX_LOG_ENTRIES * MAX_LOG_SIZE;
                if (psramFound()) screenBinBuffer = static_cast<uint8_t *>(ps_malloc(desiredSize));
                if (!screenBinBuffer) screenBinBuffer = static_cast<uint8_t *>(malloc(desiredSize));
                if (!screenBinBuffer) {
                    request->send(503, "text/plain", "Insufficient memory for screen buffer");
                    return;
                }
                screenBinBufferSize = desiredSize;
            }

            size_t binSize = 0;
            tft.getBinLog(screenBinBuffer, binSize);
            if (binSize > screenBinBufferSize) {
                request->send(500, "text/plain", "Screen buffer overflow");
                return;
            }
            request->send(200, "application/octet-stream", (const uint8_t *)screenBinBuffer, binSize);
        }
    });

    // Rename file or folder
    server->on("/rename", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (checkUserWebAuth(request)) {
            if (request->hasArg("fileName") && request->hasArg("filePath")) {
                String fs = request->arg("fs").c_str();
                String fileName = request->arg("fileName").c_str();
                String filePath = request->arg("filePath").c_str();
                String filePath2 = filePath.substring(0, filePath.lastIndexOf('/') + 1) + fileName;
                // Rename the file of folder
                if (fs == "SD") {
                    if (SD.rename(filePath, filePath2))
                        request->send(200, "text/plain", filePath + " renamed to " + filePath2);
                    else request->send(200, "text/plain", "Fail renaming file.");
                } else {
                    if (LittleFS.rename(filePath, filePath2))
                        request->send(200, "text/plain", filePath + " renamed to " + filePath2);
                    else request->send(200, "text/plain", "Fail renaming file.");
                }
            }
        }
    });

    // Route to send a generic command (Tasmota compatible API)
    // https://tasmota.github.io/docs/Commands/#with-web-requests
    server->on("/cm", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!checkUserWebAuth(request)) { return; }
        if (request->hasArg("cmnd")) {
            String cmnd = request->arg("cmnd");
            if (cmnd.startsWith("nav")) {
                volatile bool *var = &SelPress;
                if (cmnd.startsWith("nav sel")) var = &SelPress;
                if (cmnd.startsWith("nav esc")) var = &EscPress;
                if (cmnd.startsWith("nav up")) var = &UpPress;
                if (cmnd.startsWith("nav down")) var = &DownPress;
                if (cmnd.startsWith("nav next")) var = &NextPress;
                if (cmnd.startsWith("nav prev")) var = &PrevPress;
                request->send(200, "text/plain", "command " + cmnd + " success");
                int time;
                if (cmnd.endsWith("0")) time = cmnd.substring(cmnd.lastIndexOf(' ')).toInt();
                else time = 10;
                auto tmp = millis() + time;
                while (tmp > millis()) {
                    AnyKeyPress = true;
                    SerialCmdPress = true;
                    *var = true;
                    if (!LongPress) vTaskDelay(pdMS_TO_TICKS(190));
                    else vTaskDelay(pdMS_TO_TICKS(50));
                }
            } else {
                if (parseSerialCommand(cmnd, false)) {
                    request->send(200, "text/plain", "command " + cmnd + " queued");
                } else {
                    request->send(400, "text/plain", "command failed, check the serial log for details");
                }
            }
        } else {
            request->send(400, "text/plain", "http request missing required arg: cmnd");
        }
    });

    // Reboot device
    server->on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (checkUserWebAuth(request)) { ESP.restart(); }
    });

    // List files
    server->on("/listfiles", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (checkUserWebAuth(request)) {
            String folder = "/";
            if (request->hasArg("folder")) { folder = request->arg("folder"); }
            if (strcmp(request->arg("fs").c_str(), "SD") == 0) {
                request->send(200, "text/plain", listFiles(SD, folder));
            } else {
                request->send(200, "text/plain", listFiles(LittleFS, folder));
            }
        }
    });

    // Download, create folder and delete
    server->on("/file", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (checkUserWebAuth(request)) {
            if (request->hasArg("name") && request->hasArg("action")) {
                String fileName = request->arg("name").c_str();
                String fileAction = request->arg("action").c_str();
                String fileSys = request->arg("fs").c_str();
                bool useSD = false;
                if (fileSys == "SD") useSD = true;

                FS *fs;
                if (useSD) {
                    fs = &SD;
                } else fs = &LittleFS;

                log_i("filename: %s\n", fileName.c_str());
                log_i("fileAction: %s\n", fileAction.c_str());

                if (!fs->exists(fileName)) {
                    if (strcmp(fileAction.c_str(), "create") == 0) {
                        if (fs->mkdir(fileName)) {
                            request->send(200, "text/plain", "Created new folder: " + String(fileName));
                        } else {
                            request->send(200, "text/plain", "FAIL creating folder: " + String(fileName));
                        }
                    } else if (strcmp(fileAction.c_str(), "createfile") == 0) {
                        File newFile = fs->open(fileName, FILE_WRITE, true);
                        if (newFile) {
                            newFile.close();
                            request->send(200, "text/plain", "Created new file: " + String(fileName));
                        } else {
                            request->send(200, "text/plain", "FAIL creating file: " + String(fileName));
                        }
                    } else request->send(400, "text/plain", "ERROR: file does not exist");

                } else {
                    if (strcmp(fileAction.c_str(), "download") == 0) {
                        request->send(*fs, fileName, "application/octet-stream", true);
                    } else if (strcmp(fileAction.c_str(), "image") == 0) {
                        String extension = fileName.substring(fileName.lastIndexOf('.') + 1);
                        // https://www.iana.org/assignments/media-types/media-types.xhtml#image
                        if (extension == "jpg") extension = "jpeg"; // www.rfc-editor.org/rfc/rfc2046.html
                        request->send(*fs, fileName, "image/" + extension);
                    } else if (strcmp(fileAction.c_str(), "delete") == 0) {
                        if (deleteFromSd(*fs, fileName)) {
                            request->send(200, "text/plain", "Deleted : " + String(fileName));
                        } else {
                            request->send(200, "text/plain", "FAIL deleting: " + String(fileName));
                        }
                    } else if (strcmp(fileAction.c_str(), "create") == 0) {
                        if (fs->mkdir(fileName)) {
                            request->send(200, "text/plain", "Created new folder: " + String(fileName));
                        } else {
                            request->send(200, "text/plain", "FAIL creating folder: " + String(fileName));
                        }
                    } else if (strcmp(fileAction.c_str(), "createfile") == 0) {
                        File newFile = fs->open(fileName, FILE_WRITE, true);
                        if (newFile) {
                            newFile.close();
                            request->send(200, "text/plain", "Created new file: " + String(fileName));
                        } else {
                            request->send(200, "text/plain", "FAIL creating file: " + String(fileName));
                        }

                    } else if (strcmp(fileAction.c_str(), "edit") == 0) {
                        File editFile = fs->open(fileName, FILE_READ);
                        if (editFile) {
                            String fileContent = editFile.readString();
                            request->send(200, "text/plain", fileContent);
                            editFile.close();
                        } else {
                            request->send(500, "text/plain", "Failed to open file for reading");
                        }

                    } else {
                        request->send(400, "text/plain", "ERROR: invalid action param supplied");
                    }
                }
            } else {
                request->send(400, "text/plain", "ERROR: name and action params required");
            }
        }
    });

    // Edit file
    server->on("/edit", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (checkUserWebAuth(request)) {
            if (request->hasArg("name") && request->hasArg("content") && request->hasArg("fs")) {
                String fileName = request->arg("name");
                String fileContent = request->arg("content");
                bool useSD = false;

                if (strcmp(request->arg("fs").c_str(), "SD") == 0) { useSD = true; }

                fs::FS *fs = useSD ? (fs::FS *)&SD : (fs::FS *)&LittleFS;
                String fsType = useSD ? "SD" : "LittleFS";

                if (useSD) {              // LittleFS is already mounted
                    if (!setupSdCard()) { // only tries to mount SD if editting on SD
                        request->send(500, "text/plain", "Failed to initialize file system: " + fsType);
                        return;
                    }
                }

                File editFile = fs->open(fileName, FILE_WRITE);
                if (editFile) {
                    if (editFile.write((const uint8_t *)fileContent.c_str(), fileContent.length())) {
                        request->send(200, "text/plain", "File edited: " + fileName);
                    } else {
                        request->send(500, "text/plain", "Failed to write to file: " + fileName);
                    }
                    editFile.close();
                } else {
                    request->send(500, "text/plain", "Failed to open file for writing: " + fileName);
                }

            } else {
                request->send(400, "text/plain", "ERROR: name, content, and fs parameters required");
            }
        }
    });

    // File upload
    server->on("/upload", HTTP_POST, finishUpload, handleUpload);

    // Wi-Fi configuration
    server->on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (checkUserWebAuth(request)) {
            if (request->hasArg("usr") && request->hasArg("pwd")) {
                const char *usr = request->arg("usr").c_str();
                const char *pwd = request->arg("pwd").c_str();
                bruceConfig.setWebUICreds(usr, pwd);
                request->send(
                    200, "text/plain", "User: " + String(usr) + " configured with password: " + String(pwd)
                );
            }
        }
    });
    server->begin();
    Serial.println("Webserver started");
}

/**********************************************************************
**  Function: startWebUi
**  Start the WebUI
**********************************************************************/
void startWebUi(bool mode_ap) {
    bool keepWifiConnected = false;
    if (!WiFi.isConnected()) {
        if (mode_ap) wifiConnectMenu(WIFI_AP);
        else wifiConnectMenu(WIFI_STA);
    } else {
        keepWifiConnected = true;
    }

    // configure web server

    if (!server) {
        // Clear this vector to free stack memory
        options.clear();

        Serial.println("Configuring Webserver ...");
        if (psramFound()) server = (AsyncWebServer *)ps_malloc(sizeof(AsyncWebServer));
        else server = (AsyncWebServer *)malloc(sizeof(AsyncWebServer));

        new (server) AsyncWebServer(default_webserverporthttp);

        configureWebServer();

        isWebUIActive = true;
    }
    tft.setLogging();
    drawWebUiScreen(mode_ap);

    while (!check(EscPress)) {
        // nothing here, just to hold the screen until the server is on.
        vTaskDelay(pdMS_TO_TICKS(70));
    }

    bool closeServer = false;

    options.clear();
    options.emplace_back("Run in background", []() {});
    options.emplace_back("Exit", [&closeServer]() { closeServer = true; });

    loopOptions(options);

    if (closeServer) {
        stopWebUi();
        vTaskDelay(pdMS_TO_TICKS(100));
        if (!keepWifiConnected) { wifiDisconnect(); }
    }
}
