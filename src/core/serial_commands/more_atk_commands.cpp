#include "more_atk_commands.h"
#include "core/wifi/wifi_common.h"
#include "helpers.h"
#include "modules/wifi/more_atks.h"
#include "modules/wifi/wifi_atks.h"
#include <WiFi.h>
#include <globals.h>

// Console driver for the additional WiFi attacks so they can be exercised over
// the Developer Options console (or any serial terminal) without touching the UI.
//
//   moreatk scan
//   moreatk authflood <seconds>
//   moreatk probe <seconds> [ssid]
//   moreatk eapolstart <seconds>
//   moreatk eapollogoff <bssid> <channel> <seconds>
//   moreatk chanswitch <bssid> <channel> <newChannel> <seconds>
//   moreatk badmsg <bssid> <channel> <seconds>
//   moreatk auth <bssid> <channel> <seconds>
//   moreatk disassoc <bssid> <channel> <seconds>
//   moreatk dhcpstarve <bssid> <channel> <ssid> <seconds> [password]

static void morePrintResult(const char *name, const MoreAtkResult &r) {
    serialDevice->println(
        String("RESULT name=") + name + " ok=" + (r.ok ? "1" : "0") + " frames=" + String(r.frames) +
        " seconds=" + String(r.seconds) + " channel=" + String(r.channel) + " targets=" + String(r.targets) +
        " detail=" + r.detail + " error=" + (r.error ? r.error : "-")
    );
}

uint32_t moreAtkCallback(cmd *c) {
#ifndef LITE_VERSION
    Command cmd(c);
    String mode = cmd.getArgument(0).getValue();
    mode.trim();
    mode.toLowerCase();

    String a = cmd.getArgument(1).getValue();
    String b = cmd.getArgument(2).getValue();
    String d = cmd.getArgument(3).getValue();
    a.trim();
    b.trim();
    d.trim();

    uint32_t secs = a.toInt();
    if (secs == 0) secs = 15;
    uint32_t ms = secs * 1000;

    if (mode == "scan") {
        int nets = 0;
        {
            // A scan needs the station interface, not the attacker AP: starting
            // the softAP first cost ~0.4 s per call and had to be torn down again
            // afterwards, for a radio (and RSSI list) that is identical either way.
            if (!wifiEnsureRadioMem()) {
                serialDevice->println("ERR low RAM for wifi");
                return false;
            }
            WiFi.mode(WIFI_MODE_STA);
            nets = wifiScanNets(false);
            for (int i = 0; i < nets; i++) {
                serialDevice->println(
                    String("ap=") + (WiFi.SSID(i).length() ? WiFi.SSID(i) : String("<hidden>")) +
                    " mac=" + WiFi.BSSIDstr(i) + " ch=" + String(WiFi.channel(i)) + " rssi=" + String(WiFi.RSSI(i))
                );
            }
            serialDevice->println(String("count=") + String(nets));
            WiFi.scanDelete();
            WiFi.mode(WIFI_MODE_NULL); // the scan brought the station up; release it

        }
        return true;
    }

    if (mode == "authflood") {
        morePrintResult("authflood", more_auth_flood(ms));
        return true;
    }
    if (mode == "probe") {
        bool randomSSID = (b.length() == 0);
        morePrintResult("probe", more_probe_flood(ms, b, randomSSID));
        return true;
    }
    if (mode == "eapolstart" || mode == "eapolattack") {
        morePrintResult("eapolstart", more_eapol_start_flood(ms));
        return true;
    }
    // Targeted modes: <bssid> <channel> [<extra>] <seconds>
    uint8_t bssid[6];
    if (!moreParseMac(a, bssid)) {
        serialDevice->println("ERR bad bssid (expected AA:BB:CC:DD:EE:FF)");
        return false;
    }
    uint8_t channel = (uint8_t)b.toInt();
    if (channel < 1 || channel > 14) {
        serialDevice->println("ERR channel must be 1-14");
        return false;
    }

    if (mode == "eapollogoff") {
        uint32_t s = d.toInt();
        if (s == 0) s = 15;
        MoreAtkResult r = more_eapol_logoff(bssid, channel, s * 1000);
        morePrintResult("eapollogoff", r);
        return true;
    }
    if (mode == "badmsg") {
        uint32_t s = d.toInt();
        if (s == 0) s = 15;
        morePrintResult("badmsg", more_bad_message(bssid, channel, s * 1000));
        return true;
    }
    if (mode == "auth") {
        uint32_t s = d.toInt();
        if (s == 0) s = 15;
        morePrintResult("auth", more_auth_attack(bssid, channel, s * 1000));
        return true;
    }
    if (mode == "disassoc") {
        uint32_t s = d.toInt();
        if (s == 0) s = 15;
        morePrintResult("disassoc", more_disassoc_attack(bssid, channel, s * 1000));
        return true;
    }
    if (mode == "dhcpstarve") {
        // <bssid> <channel> <ssid> <seconds> [password]. The password may be
        // omitted when the board already has one saved for that SSID.
        String ssid = cmd.getArgument(3).getValue();
        ssid.trim();
        uint32_t s = cmd.getArgument(4).getValue().toInt();
        String pw = cmd.getArgument(5).getValue();
        pw.trim();
        if (ssid.length() == 0) {
            serialDevice->println("ERR dhcpstarve needs <bssid> <channel> <ssid> <seconds> [password]");
            return false;
        }
        if (s == 0) s = 15;
        if (pw.length() == 0) pw = bruceConfig.getWifiPassword(ssid);
        morePrintResult("dhcpstarve", more_dhcp_starvation(bssid, ssid, pw, channel, s * 1000));
        return true;
    }
    if (mode == "chanswitch") {
        // <bssid> <channel> <ssid> <newChannel> <seconds>
        String ssid = d;
        uint8_t newCh = (uint8_t)cmd.getArgument(4).getValue().toInt();
        uint32_t s = cmd.getArgument(5).getValue().toInt();
        if (newCh < 1 || newCh > 14) newCh = 11;
        if (s == 0) s = 15;
        morePrintResult("chanswitch", more_channel_switch(bssid, ssid.c_str(), channel, newCh, s * 1000));
        return true;
    }

    serialDevice->println(
        "modes: scan authflood probe eapolstart eapolattack eapollogoff badmsg auth disassoc chanswitch "
        "dhcpstarve"
    );
    return false;
#else
    return false;
#endif
}

void createMoreCommands(SimpleCLI *cli) {
#if !defined(LITE_VERSION)
    Command cmd = cli->addCommand("moreatk,wifi-atk", moreAtkCallback);
    cmd.addPosArg("mode");
    cmd.addPosArg("a", "");
    cmd.addPosArg("b", "");
    cmd.addPosArg("c", "");
    cmd.addPosArg("d", "");
    cmd.addPosArg("e", "");
#endif
}
