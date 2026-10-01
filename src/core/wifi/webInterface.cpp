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
#include <esp32-hal-psram.h>
#include <esp_heap_caps.h>
#include <globals.h>

// miniz for ZIP extraction (miniz-esp32 from lib_deps).
#include <miniz.h>

File uploadFile;
FS _webFS = LittleFS;
// WiFi as a Client
const int default_webserverporthttp = 80;

// WiFi as an Access Point
IPAddress AP_GATEWAY(172, 0, 0, 1); // Gateway

AsyncWebServer *server = nullptr; // initialise webserver
const char *host = "bruce";
String uploadFolder = "";
static bool mdnsRunning = false;

// tracked upload state for zip handling
static bool g_uploadIsZip = false;
static String g_uploadZipPath = "";
static String g_uploadTempPath = ""; // where the .zip actually landed on disk

// state exposed to the frontend via GET /upload_state.
static bool g_lastUploadWasZip = false;
static String g_lastUploadZipPath = "";

// Serialization guard: only one long operation (extract or zip upload) at a
// time. Prevents concurrent requests from corrupting miniz globals, FS
// handles, or the WebUI task stack.
static volatile bool g_longOpInProgress = false;

struct LongOpGuard {
    LongOpGuard() { g_longOpInProgress = true; }
    ~LongOpGuard() { g_longOpInProgress = false; }
};

// temp directory for uploaded zips.
static const char *ZIP_TMP_DIR_SD = "/.bruce_tmp";
static const char *ZIP_TMP_DIR_LFS = "/.bruce_tmp";

// Upper bound for the in-memory fallback path in extractZipTo().
static const size_t ZIP_MEM_FALLBACK_LIMIT = 2 * 1024 * 1024; // 2 MB

// Diagnostic toggle: when true, the fallback (memory-based) extraction path
// is disabled and extractZipTo() will return -1 if the fast path fails.
// Useful for isolating which path is crashing. Set to false in normal use.
static const bool DIAG_DISABLE_MEM_FALLBACK = false;

// Generate random token
String generateToken(int length = 24) {
    String token = "";
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < length; i++) { token += charset[random(0, sizeof(charset) - 1)]; }
    return token;
}

/**********************************************************************
**  Function: stopWebUi
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
**********************************************************************/
void cleanlyStopWebUiForWiFiFeature() {
    if (!isWebUIActive && !server) { return; }
    Serial.println("Stopping WebUI for WiFi feature...");
    if (server) {
        stopWebUi();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    wifi_mode_t currentMode = WiFi.getMode();
    if (currentMode == WIFI_MODE_AP || currentMode == WIFI_MODE_APSTA) {
        wifiDisconnect();
        vTaskDelay(pdMS_TO_TICKS(250));
    }
    Serial.println("WebUI stopped, starting WiFi feature...");
}

/**********************************************************************
**  Function: loopOptionsWebUi
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
}

/**********************************************************************
**  Function: humanReadableSize
**********************************************************************/
String humanReadableSize(uint64_t bytes) {
    if (bytes < 1024) return String(bytes) + " B";
    else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " kB";
    else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
    else return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}

/**********************************************************************
**  Function: listFiles
**********************************************************************/
String listFiles(FS &fs, const String &folder) {
    String returnText = "pa:" + folder + ":0\n";
    _webFS = fs;
    File root = fs.open(folder);
    uploadFolder = folder;
    while (true) {
        bool isDir;
        String fullPath = root.getNextFileName(&isDir);
        String nameOnly = fullPath.substring(fullPath.lastIndexOf("/") + 1);
        if (fullPath == "") { break; }
        if (esp_get_free_heap_size() > (String("Fo:" + nameOnly + ":0\n").length()) + 1024) {
            if (isDir) {
                returnText += "Fo:" + nameOnly + ":0\n";
            } else {
                File file = fs.open(fullPath);
                if (file) {
                    returnText += "Fi:" + nameOnly + ":" + humanReadableSize(file.size()) + "\n";
                    file.close();
                }
            }
        } else break;
        delay(1);
    }
    root.close();
    return returnText;
}

/**********************************************************************
**  Function: checkUserWebAuth
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

/**********************************************************************
**  Function: createDirRecursive
**********************************************************************/
void createDirRecursive(const String &path, FS fs) {
    String currentPath = "";
    int startIndex = 0;
    while (startIndex < path.length()) {
        int endIndex = path.indexOf("/", startIndex);
        if (endIndex == -1) endIndex = path.length();
        currentPath += path.substring(startIndex, endIndex);
        if (currentPath.length() > 0) {
            if (!fs.exists(currentPath)) { fs.mkdir(currentPath); }
        }
        if (endIndex < path.length()) { currentPath += "/"; }
        startIndex = endIndex + 1;
    }
}

// =============================================================================
// ZIP extraction support
// =============================================================================

static String sanitizeZipEntry(const String &entry) {
    if (entry.length() == 0) return "";
    if (entry.endsWith("/")) return "";

    String clean = entry;
    if (clean.indexOf("..") >= 0) return "";
    while (clean.startsWith("/")) clean = clean.substring(1);
    if (clean.indexOf(":") >= 0) return "";
    if (clean.length() == 0) return "";
    return clean;
}

static String basenameOf(const String &path) {
    int slash = path.lastIndexOf('/');
    if (slash >= 0) return path.substring(slash + 1);
    int backslash = path.lastIndexOf('\\');
    if (backslash >= 0) return path.substring(backslash + 1);
    return path;
}

// Write callback for mz_zip_reader_extract_to_callback — streams decompressed
// bytes straight to the target File without buffering the whole entry in RAM.
struct ZipWriteCtx {
    File *out;
    bool writeError;
};

static size_t zipWriteCb(void *pOpaque, mz_uint64 file_ofs, const void *pBuf, size_t n) {
    (void)file_ofs;
    ZipWriteCtx *ctx = (ZipWriteCtx *)pOpaque;
    if (!ctx || !ctx->out || !*ctx->out) return 0;
    size_t written = ctx->out->write((const uint8_t *)pBuf, n);
    if (written != n) {
        ctx->writeError = true;
        return 0;
    }
    return n;
}

// extractZipTo — tries mz_zip_reader_init_file first (fast, no extra RAM);
// if that fails, falls back to loading the whole archive into heap and using
// mz_zip_reader_init_mem (capped at ZIP_MEM_FALLBACK_LIMIT).
//
// Both mz_zip_archive and mz_zip_archive_file_stat are hoisted out of the
// loop to keep the WebUI task's stack usage bounded.
int extractZipTo(FS &fs, const String &zipPath, const String &targetFolder, bool deleteAfter) {
    if (!fs.exists(zipPath)) return -1;

    Serial.printf("[extract] free heap at start: %u\n", (unsigned)esp_get_free_heap_size());

    mz_zip_archive zip;
    mz_zip_archive_file_stat stat;
    uint8_t *memBuf = nullptr;

    // --- Attempt 1: open by path (uses miniz's own file layer) ---
    memset(&zip, 0, sizeof(zip));
    bool opened = mz_zip_reader_init_file(&zip, zipPath.c_str(), 0);
    if (opened) {
        Serial.printf("[extract] opened by path: %s, free heap: %u\n",
                      zipPath.c_str(), (unsigned)esp_get_free_heap_size());
    } else if (DIAG_DISABLE_MEM_FALLBACK) {
        Serial.println("[extract] path init failed (fallback disabled by diagnostic flag)");
        return -1;
    } else {
        Serial.printf("[extract] path init failed, trying memory fallback. free heap: %u\n",
                      (unsigned)esp_get_free_heap_size());

        // --- Attempt 2: load into memory via Arduino FS API ---
        File zipFile = fs.open(zipPath, FILE_READ);
        if (!zipFile) {
            Serial.println("[extract] fs.open failed too");
            return -1;
        }
        size_t zipSize = zipFile.size();
        if (zipSize == 0 || zipSize > ZIP_MEM_FALLBACK_LIMIT) {
            Serial.printf("[extract] refusing memory fallback (size=%u, limit=%u)\n",
                          (unsigned)zipSize, (unsigned)ZIP_MEM_FALLBACK_LIMIT);
            zipFile.close();
            return -1;
        }
        memBuf = (uint8_t *)malloc(zipSize);
        if (!memBuf) {
            Serial.printf("[extract] malloc(%u) failed. free heap: %u\n",
                          (unsigned)zipSize, (unsigned)esp_get_free_heap_size());
            zipFile.close();
            return -1;
        }
        size_t got = zipFile.read(memBuf, zipSize);
        zipFile.close();
        if (got != zipSize) {
            Serial.printf("[extract] read short (%u of %u)\n", (unsigned)got, (unsigned)zipSize);
            free(memBuf);
            return -1;
        }

        memset(&zip, 0, sizeof(zip));
        opened = mz_zip_reader_init_mem(&zip, memBuf, zipSize, 0);
        if (!opened) {
            Serial.println("[extract] memory init failed too");
            free(memBuf);
            return -1;
        }
        Serial.printf("[extract] opened by memory buffer (%u bytes). free heap: %u\n",
                      (unsigned)zipSize, (unsigned)esp_get_free_heap_size());
    }

    // --- Extraction loop (same regardless of how we opened the archive) ---
    mz_uint numFiles = mz_zip_reader_get_num_files(&zip);
    int extracted = 0;

    Serial.printf("[extract] %u file(s) in archive\n", (unsigned)numFiles);

    String baseFolder = targetFolder;
    if (baseFolder.length() == 0 || baseFolder == "/") baseFolder = "";
    if (baseFolder.length() > 0 && !baseFolder.startsWith("/")) baseFolder = "/" + baseFolder;
    if (baseFolder.length() > 0 && baseFolder.endsWith("/")) baseFolder = baseFolder.substring(0, baseFolder.length() - 1);
    if (!fs.exists(baseFolder) && baseFolder.length() > 0) createDirRecursive(baseFolder, fs);

    for (mz_uint i = 0; i < numFiles; i++) {
        Serial.printf("[extract] entry %u/%u. free heap: %u\n",
                      (unsigned)(i + 1), (unsigned)numFiles, (unsigned)esp_get_free_heap_size());

        if (!mz_zip_reader_file_stat(&zip, i, &stat)) continue;

        String rawName = String(stat.m_filename);
        String entryName = sanitizeZipEntry(rawName);
        if (entryName.length() == 0) {
            Serial.println("[extract]   (skipped: sanitized to empty)");
            continue;
        }

        String fullPath = baseFolder + "/" + entryName;
        String dirPath = fullPath.substring(0, fullPath.lastIndexOf("/"));
        if (dirPath.length() > 0) createDirRecursive(dirPath, fs);

        File out = fs.open(fullPath, FILE_WRITE);
        if (!out) {
            Serial.printf("[extract]   fs.open(%s) failed\n", fullPath.c_str());
            continue;
        }

        ZipWriteCtx ctx{&out, false};
        bool ok = mz_zip_reader_extract_to_callback(&zip, i, zipWriteCb, &ctx, 0);
        out.close();

        if (ok && !ctx.writeError) {
            extracted++;
        } else {
            Serial.printf("[extract]   extraction of %s failed (ok=%d, writeError=%d)\n",
                          fullPath.c_str(), (int)ok, (int)ctx.writeError);
            fs.remove(fullPath);
        }

        // Yield to the RTOS so the watchdog doesn't fire during long
        // extractions and so the WebUI task doesn't hog the CPU.
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    mz_zip_reader_end(&zip);
    if (memBuf) free(memBuf);

    Serial.printf("[extract] done. extracted=%d. free heap: %u\n",
                  extracted, (unsigned)esp_get_free_heap_size());

    if (deleteAfter && extracted >= 0) { fs.remove(zipPath); }
    return extracted;
}

static void cleanZipTempDir(FS &fs, const char *dir) {
    if (!fs.exists(dir)) return;
    File root = fs.open(dir);
    if (!root) return;
    while (true) {
        bool isDir = false;
        String p = root.getNextFileName(&isDir);
        if (p.length() == 0) break;
        if (!isDir && p.endsWith(".zip")) fs.remove(p);
    }
    root.close();
}

static FS *pickTempFs(String &tempDirOut) {
    if (setupSdCard()) {
        tempDirOut = ZIP_TMP_DIR_SD;
        return &SD;
    }
    tempDirOut = ZIP_TMP_DIR_LFS;
    return &LittleFS;
}

// =============================================================================
// Upload handler
// =============================================================================

void handleUpload(
    AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final
) {
    if (!checkUserWebAuth(request)) return;

    if (uploadFolder == "/") uploadFolder = "";

    if (!index) {
        if (g_longOpInProgress) {
            Serial.println("[upload] refused: another long op in progress");
            return;
        }

        if (request->hasArg("password")) filename = filename + ".enc";

        String lower = filename;
        lower.toLowerCase();
        g_uploadIsZip = lower.endsWith(".zip");

        if (g_uploadIsZip) {
            g_longOpInProgress = true;

            String tempDir;
            FS *tmpFs = pickTempFs(tempDir);
            if (!tmpFs->exists(tempDir)) tmpFs->mkdir(tempDir);

            String safeName = basenameOf(filename);
            g_uploadTempPath = tempDir + "/" + safeName;

            request->_tempFile = tmpFs->open(g_uploadTempPath, "w");
        } else {
            String relativePath = filename;
            String fullPath = uploadFolder + "/" + relativePath;
            String dirPath = fullPath.substring(0, fullPath.lastIndexOf("/"));
            if (dirPath.length() > 0) { createDirRecursive(dirPath, _webFS); }

            request->_tempFile = _webFS.open(uploadFolder + "/" + filename, "w");
        }
    }

    if (len) {
        if (request->hasArg("password")) {
            static int chunck_no = 0;
            if (chunck_no != 0) {
                request->send(404, "text/html", "file is too big");
                return;
            } else chunck_no += 1;
            String enc_password = request->arg("password");
            String plaintext = String((char *)data).substring(0, len);
            String cyphertxt = encryptString(plaintext, enc_password);
            if (cyphertxt == "") { return; }
            if (request->_tempFile)
                request->_tempFile.write((const uint8_t *)cyphertxt.c_str(), cyphertxt.length());
        } else {
            if (request->_tempFile) request->_tempFile.write(data, len);
        }
    }

    if (final) {
        if (request->_tempFile) request->_tempFile.close();

        if (g_uploadIsZip) {
            g_lastUploadWasZip = true;
            g_lastUploadZipPath = g_uploadTempPath;
            Serial.printf("[upload] zip stored at %s\n", g_uploadTempPath.c_str());

            vTaskDelay(pdMS_TO_TICKS(200));
            g_longOpInProgress = false;
        } else {
            g_lastUploadWasZip = false;
            g_lastUploadZipPath = "";
        }
    }
}

void notFound(AsyncWebServerRequest *request) { request->send(404, "text/plain", "Nothing in here Sharky"); }

/**********************************************************************
**  Function: drawWebUiScreen
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
**********************************************************************/
String color565ToWebHex(uint16_t color565) {
    uint8_t r = (color565 >> 11) & 0x1F;
    uint8_t g = (color565 >> 5) & 0x3F;
    uint8_t b = color565 & 0x1F;
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);
    char hex[8];
    snprintf(hex, sizeof(hex), "#%02X%02X%02X", r, g, b);
    return String(hex);
}

/**********************************************************************
**  Function: serveWebUIFile
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
**********************************************************************/
void configureWebServer() {
    mdnsRunning = startMdnsResponder();
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    server->onNotFound(notFound);

    {
        String tempDir;
        FS *tmpFs = pickTempFs(tempDir);
        cleanZipTempDir(*tmpFs, tempDir.c_str());
    }

    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (checkUserWebAuth(request, true)) {
            serveWebUIFile(request, "index.html", "text/html", true, index_html, index_html_size);
        }
    });

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

    server->on("/theme.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        serveWebUIFile(request, "theme.css", "text/css");
    });
    server->on("/index.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        serveWebUIFile(request, "index.css", "text/css", true, index_css, index_css_size);
    });
    server->on("/index.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        serveWebUIFile(request, "index.js", "text/javascript", true, index_js, index_js_size);
    });

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

    server->on("/upload_state", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!checkUserWebAuth(request)) return;
        Serial.printf(
            "[upload_state] isZip=%d tempPath=%s\n",
            (int)g_lastUploadWasZip,
            g_lastUploadZipPath.c_str()
        );
        String json;
        if (g_lastUploadWasZip) {
            json = "{\"isZip\":true,\"tempPath\":\"" + g_lastUploadZipPath + "\"}";
        } else {
            json = "{\"isZip\":false}";
        }
        request->send(200, "application/json", json);
    });

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

    server->on("/rename", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (checkUserWebAuth(request)) {
            if (request->hasArg("fileName") && request->hasArg("filePath")) {
                String fs = request->arg("fs").c_str();
                String fileName = request->arg("fileName").c_str();
                String filePath = request->arg("filePath").c_str();
                String filePath2 = filePath.substring(0, filePath.lastIndexOf('/') + 1) + fileName;
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

    server->on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (checkUserWebAuth(request)) { ESP.restart(); }
    });

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

    server->on("/file", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (checkUserWebAuth(request)) {
            if (request->hasArg("name") && request->hasArg("action")) {
                String fileName = request->arg("name").c_str();
                String fileAction = request->arg("action").c_str();
                String fileSys = request->arg("fs").c_str();
                bool useSD = false;
                if (fileSys == "SD") useSD = true;

                FS *fs;
                if (useSD) fs = &SD;
                else fs = &LittleFS;

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
                        if (extension == "jpg") extension = "jpeg";
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

    server->on("/edit", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (checkUserWebAuth(request)) {
            if (request->hasArg("name") && request->hasArg("content") && request->hasArg("fs")) {
                String fileName = request->arg("name");
                String fileContent = request->arg("content");
                bool useSD = false;

                if (strcmp(request->arg("fs").c_str(), "SD") == 0) { useSD = true; }

                fs::FS *fs = useSD ? (fs::FS *)&SD : (fs::FS *)&LittleFS;
                String fsType = useSD ? "SD" : "LittleFS";

                if (useSD) {
                    if (!setupSdCard()) {
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

    // /extract endpoint.
    server->on("/extract", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!checkUserWebAuth(request)) return;

        if (g_longOpInProgress) {
            request->send(409, "text/plain", "Another operation is in progress");
            return;
        }
        LongOpGuard guard;

        if (!request->hasArg("zipPath") || !request->hasArg("targetFolder")) {
            request->send(400, "text/plain", "ERROR: zipPath and targetFolder required");
            return;
        }

        String zipPath = request->arg("zipPath");
        String targetFolder = request->arg("targetFolder");
        bool useSD = true;
        if (request->hasArg("fs")) { useSD = (strcmp(request->arg("fs").c_str(), "SD") == 0); }
        bool deleteAfter = false;
        if (request->hasArg("deleteAfter")) {
            deleteAfter = (strcmp(request->arg("deleteAfter").c_str(), "1") == 0);
        }

        FS *fs = useSD ? (FS *)&SD : (FS *)&LittleFS;
        if (useSD && !setupSdCard()) {
            request->send(500, "text/plain", "SD not mounted");
            return;
        }

        if (!fs->exists(zipPath)) {
            request->send(404, "text/plain", "Zip not found at " + zipPath);
            return;
        }

        if (request->hasArg("moveOnly") && strcmp(request->arg("moveOnly").c_str(), "1") == 0) {
            String baseName = basenameOf(zipPath);

            String dest = targetFolder;
            if (dest.length() == 0) dest = "/";
            if (!dest.endsWith("/")) dest += "/";
            dest += baseName;

            if (fs->rename(zipPath, dest)) {
                request->send(200, "application/json", "{\"moved\":\"" + dest + "\"}");
                return;
            }

            File src = fs->open(zipPath, FILE_READ);
            if (!src) {
                request->send(500, "text/plain", "Source zip missing");
                return;
            }
            File dst = fs->open(dest, FILE_WRITE);
            if (!dst) {
                src.close();
                request->send(500, "text/plain", "Cannot create " + dest);
                return;
            }
            uint8_t buf[512];
            while (src.available()) {
                size_t n = src.read(buf, sizeof(buf));
                if (n == 0) break;
                dst.write(buf, n);
            }
            src.close();
            dst.close();
            fs->remove(zipPath);
            request->send(200, "application/json", "{\"moved\":\"" + dest + "\"}");
            return;
        }

        int n = extractZipTo(*fs, zipPath, targetFolder, deleteAfter);
        if (n < 0) {
            request->send(500, "text/plain", "Extraction failed");
        } else {
            vTaskDelay(pdMS_TO_TICKS(200));
            char buf[96];
            snprintf(buf, sizeof(buf), "{\"extracted\":%d,\"target\":\"%s\"}", n, targetFolder.c_str());
            request->send(200, "application/json", buf);
        }
    });

    server->on(
        "/upload",
        HTTP_POST,
        [](AsyncWebServerRequest *request) { request->send(200, "text/plain", "File upload completed"); },
        handleUpload
    );

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
**********************************************************************/
void startWebUi(bool mode_ap) {
    bool keepWifiConnected = false;
    if (!WiFi.isConnected()) {
        if (mode_ap) wifiConnectMenu(WIFI_AP);
        else wifiConnectMenu(WIFI_STA);
    } else {
        keepWifiConnected = true;
    }

    if (!server) {
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
#ifdef HAS_SCREEN
    while (!check(EscPress)) { vTaskDelay(pdMS_TO_TICKS(70)); }

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
#endif
}
