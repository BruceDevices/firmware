#include "wifi_commands.h"
#include "core/wifi/webInterface.h"
#include "core/wifi/wifi_common.h" //to return MAC addr
#include <globals.h>
#include <modules/ethernet/ARPScanner.h>
#include "esp_netif.h"          
#include "esp_netif_net_stack.h"
#include "modules/wifi/tcp_utils.h"
#include "modules/wifi/sniffer.h"
//#include "modules/wifi/responder.h"

uint32_t wifiCallback(cmd *c) {
    Command cmd(c);
    Argument statusArg = cmd.getArgument("status");
    String status = statusArg.getValue();
    status.trim();

    Argument ssidArg = cmd.getArgument("ssid");
    String ssid = ssidArg.getValue();
    ssid.trim();

    Argument pwdArg = cmd.getArgument("pwd");
    String pwd = pwdArg.getValue();
    pwd.trim();

    if (status == "off") {
        wifiDisconnect();
        return true;
    } else if (status == "on") {
        if (wifiConnected) {
            serialDevice->println("Wifi already connected");
            return true;
        }
        if (wifiConnecttoKnownNet()) return true;
        wifiDisconnect();
        return _setupAP();

    } else if (status == "add" && ssid != "" && pwd != "") {
        bruceConfig.addWifiCredential(ssid, pwd);
        return true;
    } else {
        serialDevice->println(
            "Invalid status: " + status +
            "\n"
            "Possible commands: \n"
            "-> wifi off (Disconnects Wifi)\n"
            "-> wifi on  (Connects to a known Wifi network. if there's no known network, starts in AP Mode)\n"
            "-> wifi add SSID Password (adds a network to the list)"
        );
        return false;
    }
}

uint32_t webuiCallback(cmd *c) {
    Command cmd(c);

    Argument arg = cmd.getArgument("noAp");
    bool noAp = arg.isSet();

    serialDevice->println("Starting Web UI " + !noAp ? "AP" : "STA");
    serialDevice->println("Press ESC to quit");
    startWebUi(!noAp); // MEMO: will quit when check(EscPress)

    return true;
}

uint32_t scanHostsCallback(cmd *c) {
    esp_netif_t *esp_netinterface =
      esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
      if (esp_netinterface == nullptr) {
          Serial.println("Failed to get netif handle\nTry connecting to a network first");
          return false;
      }

    ARPScanner{esp_netinterface};

    return true;
}

uint32_t snifferCallback(cmd *c) {
    sniffer_setup();

    return true;
}

uint32_t listenTCPCallback(cmd *c) {
    if (!wifiConnected) Serial.println("Connect to a WiFi first."); return false;

    listenTcpPort();

    return true;
}

/*
uint32_t responderCallback(cmd *c) {
    if (!wifiConnected) Serial.println("Connect to a WiFi first."); return false;

    responder();

    return true;
}
*/

void createWifiCommands(SimpleCLI *cli) {
    Command webuiCmd = cli->addCommand("webui", webuiCallback);
    webuiCmd.addFlagArg("noAp");

    Command wifiCmd = cli->addCommand("wifi", wifiCallback);
    wifiCmd.addPosArg("status");
    wifiCmd.addPosArg("ssid", "");
    wifiCmd.addPosArg("pwd", "");

    #if !defined(LITE_VERSION)

    Command ScanHostsCmd = cli->addCommand("arp", scanHostsCallback);

    Command listenTCPCmd = cli->addCommand("listen", listenTCPCallback); //TODO: make possible to select port to open via Serial
    
    Command snifferCmd = cli->addCommand("sniffer", snifferCallback); //TODO: be able to exit from it from Serial
    
    #endif
    //Command responderCmd = cli->addCommand("responder", responderCallback); TODO
}
