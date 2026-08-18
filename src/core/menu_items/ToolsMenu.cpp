#include "ToolsMenu.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <WiFi.h>

namespace {
AsyncWebServer *toolsServer = nullptr;
FS *toolsServerFs = nullptr;
String toolsServerFile;
String toolsServerLogFile;
uint16_t toolsServerPort = 8000;
bool toolsServerLogging = true;
uint16_t toolsServerInstance = 0;

const char *contentTypeForPath(const String &path) {
    String extension = path.substring(path.lastIndexOf('.') + 1);
    extension.toLowerCase();
    if (extension == "html" || extension == "htm") return "text/html";
    if (extension == "css") return "text/css";
    if (extension == "js") return "text/javascript";
    if (extension == "json") return "application/json";
    if (extension == "svg") return "image/svg+xml";
    if (extension == "png") return "image/png";
    if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
    if (extension == "gif") return "image/gif";
    if (extension == "ico") return "image/x-icon";
    return "application/octet-stream";
}

void stopToolsServer() {
    if (!toolsServer) return;
    toolsServer->end();
    toolsServer->~AsyncWebServer();
    free(toolsServer);
    toolsServer = nullptr;
    toolsServerFs = nullptr;
    toolsServerFile = "";
    toolsServerLogFile = "";
}
// Logging functions for the server, it's kinda decent
void logToolsServerRequest(AsyncWebServerRequest *request) {
    if (!toolsServerLogging || !toolsServerFs) return;
    File logFile = toolsServerFs->open(toolsServerLogFile, FILE_APPEND);
    if (!logFile) return;
    String query = request->params() > 0 ? "?" : "";
    for (size_t i = 0; i < request->params(); i++) {
        if (i > 0) query += "&";
        query += request->argName(i) + "=" + request->arg(i);
    }
    const char *hostname = Network.getHostname();
    if (!hostname || hostname[0] == '\0') hostname = "unknown";
    logFile.printf(
        "time_ms=%lu method=%s path=%s%s remote=%s:%u device=%s served=%s\n",
        millis(),
        request->methodToString(),
        request->url().c_str(),
        query.c_str(),
        request->client()->remoteIP().toString().c_str(),
        request->client()->remotePort(),
        hostname,
        request->url() == "/" ? toolsServerFile.c_str() : request->url().c_str()
    );
    logFile.close();
}

bool chooseToolsServerFile() {
    FS *fs = nullptr;
    if (!getFsStorage(fs) || !fs) {
        displayError("No storage available", true);
        return false;
    }

    String selected = loopSD(*fs, true, "HTML|HTM");
    if (selected.length() == 0 || selected == "\x1B" || !fs->exists(selected)) {
        displayError("No HTML file selected", true);
        return false;
    }

    toolsServerFs = fs;
    toolsServerFile = selected;
    return true;
}
// Port selection for the server
bool chooseToolsServerPort() {
    String portText = num_keyboard(String(toolsServerPort), 5, "Server port (1-65535)");
    if (portText.length() == 0 || portText == "\x1B") return false;

    for (size_t i = 0; i < portText.length(); i++) {
        if (!isDigit(portText[i])) {
            displayError("Invalid port", true);
            return false;
        }
    }

    unsigned long port = strtoul(portText.c_str(), nullptr, 10);
    if (port < 1 || port > 65535 || (port == 80 && isWebUIActive)) {
        displayError("Port unavailable", true);
        return false;
    }
    // Port variable, relatively safe to cast
    toolsServerPort = static_cast<uint16_t>(port);
    return true;
}

void startToolsServer() {
    stopToolsServer();
    if (!WiFi.isConnected()) {
        if (!wifiConnectMenu(WIFI_STA) || !WiFi.isConnected()) {
            displayError("Wi-Fi connection failed", true);
            return;
        }
    }
    if (!chooseToolsServerFile() || !chooseToolsServerPort()) return;

    if (psramFound()) toolsServer = static_cast<AsyncWebServer *>(ps_malloc(sizeof(AsyncWebServer)));
    else toolsServer = static_cast<AsyncWebServer *>(malloc(sizeof(AsyncWebServer)));
    if (!toolsServer) {
        displayError("Server memory failed", true);
        return;
    }
    toolsServerInstance++;
    if (!toolsServerFs->exists("/ServerPCAP")) toolsServerFs->mkdir("/ServerPCAP");
    toolsServerLogFile = "/ServerPCAP/Instance" + String(toolsServerInstance) + ".log";
    toolsServerLogging = true;
    new (toolsServer) AsyncWebServer(toolsServerPort);
    toolsServer->onNotFound([](AsyncWebServerRequest *request) {
        logToolsServerRequest(request);
        if (toolsServerFs && toolsServerFs->exists(request->url())) {
            request->send(*toolsServerFs, request->url(), contentTypeForPath(request->url()));
        } else {
            request->send(404, "text/plain", "Not found");
        }
    });
    toolsServer->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        logToolsServerRequest(request);
        if (!toolsServerFs || !toolsServerFs->exists(toolsServerFile)) {
            request->send(404, "text/plain", "HTML file unavailable");
            return;
        }
        request->send(*toolsServerFs, toolsServerFile, "text/html");
    });
    toolsServer->begin();

    Serial.printf(
        "Tools server started: http://%s:%u -> %s\n",
        WiFi.localIP().toString().c_str(),
        toolsServerPort,
        toolsServerFile.c_str()
    );
    displayInfo("Server started", true);
}

void showToolsServerStatus() {
    if (!toolsServer) {
        displayInfo("Server inactive", true);
        return;
    }
    displayInfo("Server active", false);
    tft.println("Port: " + String(toolsServerPort));
    tft.println("Log: " + String(toolsServerLogging ? "on" : "off"));
    tft.println("IP: " + WiFi.localIP().toString());
    tft.println("Esc to close");
    while (!check(EscPress)) vTaskDelay(pdMS_TO_TICKS(70));
}

void changeToolsServer() {
    if (!toolsServer) {
        startToolsServer();
        return;
    }
    stopToolsServer();
    startToolsServer();
}
// Disables logging for the server
void disableToolsServerLogging() {
    toolsServerLogging = false;
    displayInfo("Logging disabled", true);
}

void toolsServerMenu() {
    if (!toolsServer) {
        startToolsServer();
        return;
    }

    bool close = false;
    options = {
        {"Change Server",   changeToolsServer        },
        {"Disable Logging", disableToolsServerLogging},
        {"New Server",      startToolsServer         },
        {"Status",          showToolsServerStatus    },
        {"Stop Server",     [&]() {
             stopToolsServer();
             close = true;
         }                    },
    };
    loopOptions(options, MENU_TYPE_SUBMENU, "Server");
    options.clear();
}
} // namespace

static void helloWorldPlaceholder() { displayInfo("Hello Friend", true); }
static void testWifiPlaceholder() { displayInfo("Test Wi-Fi", true); }

void ToolsMenu::optionsMenu() {
    returnToMenu = false;
    options = {
        {"Launch Server", toolsServerMenu      },
        {"Hello Friend",  helloWorldPlaceholder},
        {"Test Wi-Fi",    testWifiPlaceholder  },
    };
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "Tools");
    options.clear();
}
// Creates a weird ass icon, supposed to be a wrench
void ToolsMenu::drawIcon(float scale) {
    clearIconArea();
    int wrenchSize = scale * 55;
    int handleLen = scale * 30;
    int handleWidth = scale * 7;
    int centerX = iconCenterX;
    int centerY = iconCenterY;
    tft.drawRoundRect(
        centerX - handleLen / 2, centerY - handleWidth / 2, handleLen, handleWidth, 3, bruceConfig.priColor
    );
    tft.fillRect(centerX - handleLen / 2 - 12, centerY - 14, 20, 28, bruceConfig.priColor);
    tft.fillRect(centerX + handleLen / 2 - 8, centerY - 14, 20, 28, bruceConfig.priColor);
    tft.fillRect(centerX - 3, centerY - wrenchSize / 2, 6, wrenchSize, bruceConfig.priColor);
    tft.fillRect(centerX - wrenchSize / 2, centerY - 3, wrenchSize, 6, bruceConfig.priColor);
    for (int i = -2; i <= 2; i++) {
        tft.drawLine(
            centerX + i * 8,
            centerY - wrenchSize / 2,
            centerX + i * 8,
            centerY - wrenchSize / 2 - 8,
            bruceConfig.priColor
        );
        tft.drawLine(
            centerX + i * 8,
            centerY + wrenchSize / 2,
            centerX + i * 8,
            centerY + wrenchSize / 2 + 8,
            bruceConfig.priColor
        );
    }
}
