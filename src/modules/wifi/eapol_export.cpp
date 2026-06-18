#if !defined(LITE_VERSION)
// ═══════════════════════════════════════════════════════════════
// Bruce EAPOL → .hc22000 Exporter
// Converts captured handshake PCAPs to hashcat format
// Reads from /BrucePCAP/handshakes/, outputs .hc22000 files
// ═══════════════════════════════════════════════════════════════

#include "eapol_export.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include <SD.h>
#include <FS.h>
#include <globals.h>

// ── PCAP structures ──
#pragma pack(push, 1)
struct pcap_hdr_t {
    uint32_t magic;
    uint16_t vmaj, vmin;
    int32_t thiszone;
    uint32_t sigfigs, snaplen, network;
};
struct pcaprec_hdr_t {
    uint32_t ts_sec, ts_usec, incl_len, orig_len;
};
#pragma pack(pop)

// ── Helpers ──
static uint32_t swap32(uint32_t x) {
    return ((x>>24)&0xFF) | ((x>>8)&0xFF00) | ((x<<8)&0xFF0000) | ((x<<24)&0xFF000000);
}

static void bytesToHex(const uint8_t *b, int len, char *out) {
    for (int i = 0; i < len; i++) {
        sprintf(out + i*2, "%02X", b[i]);
    }
    out[len*2] = '\0';
}

static void macToHex(const uint8_t *mac, char *out) {
    sprintf(out, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void ssidToHex(const char *ssid, char *out) {
    int len = strlen(ssid);
    if (len > 32) len = 32;
    for (int i = 0; i < len; i++) {
        sprintf(out + i*2, "%02X", (uint8_t)ssid[i]);
    }
    out[len*2] = '\0';
}

// ── EAPOL PMKID extraction (from msg1 RSN IE) ──
static bool extractPMKID(const uint8_t *payload, int len, uint8_t *pmkidOut) {
    // EAPOL offset after 802.11 header (24) + LLC/SNAP (8)
    int eapolOff = 24 + 8;
    if (len < eapolOff + 4 + 95 + 2 + 22) return false;

    // Key Data Length at bytes 93-94 from EAPOL start
    int kdLenOff = eapolOff + 4 + 93;
    uint16_t keyDataLen = (payload[kdLenOff] << 8) | payload[kdLenOff + 1];
    int kdStart = eapolOff + 4 + 95;

    if (len < kdStart + keyDataLen || keyDataLen < 22) return false;

    // Search for PMKID KDE: Tag=0xDD, Length=20, OUI=00:0F:AC, Type=04
    const uint8_t *kd = payload + kdStart;
    int pos = 0;
    while (pos + 2 <= (int)keyDataLen) {
        uint8_t tag = kd[pos];
        uint8_t tagLen = kd[pos + 1];
        if (pos + 2 + tagLen > (int)keyDataLen) break;
        if (tag == 0xDD && tagLen >= 20 &&
            kd[pos+2]==0x00 && kd[pos+3]==0x0F &&
            kd[pos+4]==0xAC && kd[pos+5]==0x04) {
            memcpy(pmkidOut, kd + pos + 6, 16);
            return true;
        }
        pos += 2 + tagLen;
    }
    return false;
}

// ── Convert a single PCAP to .hc22000 ──
static bool convertPcapToHC22000(const char *pcapPath) {
    char hcPath[128];
    // Derive .hc22000 filename from .pcap filename
    strncpy(hcPath, pcapPath, sizeof(hcPath)-1);
    hcPath[sizeof(hcPath)-1] = '\0';
    char *dot = strrchr(hcPath, '.');
    if (dot) strcpy(dot, ".hc22000");
    else strcat(hcPath, ".hc22000");

    // ── Parse PCAP ──
    File f = SD.open(pcapPath, FILE_READ);
    if (!f) return false;

    pcap_hdr_t gh;
    if (f.read((uint8_t*)&gh, sizeof(gh)) != sizeof(gh)) { f.close(); return false; }
    bool swapped = false;
    if (gh.magic == 0xd4c3b2a1) swapped = true;

    // Parsed handshake data
    uint8_t pmkid[16]; bool hasPMKID = false;
    uint8_t apMAC[6]={0}, staMAC[6]={0}, mic[16]={0}, anonce[32]={0};
    uint8_t eapolMsg2[256]; int eapolMsg2Len = 0;
    char ssid[33] = "";
    bool haveM1 = false, haveM2 = false, haveBeacon = false;

    while (f.available()) {
        pcaprec_hdr_t ph;
        if (f.read((uint8_t*)&ph, sizeof(ph)) != sizeof(ph)) break;
        uint32_t incl_len = swapped ? swap32(ph.incl_len) : ph.incl_len;
        if (incl_len < 30 || incl_len > 2500) {
            if (incl_len > 0 && incl_len < 1000000) f.seek(f.position() + incl_len);
            continue;
        }
        uint8_t *pkt = (uint8_t*)malloc(incl_len);
        if (!pkt) break;
        if (f.read(pkt, incl_len) != (int)incl_len) { free(pkt); break; }

        uint16_t fc = (uint16_t)(pkt[0] | (pkt[1] << 8));
        uint8_t type = (fc & 0x0C) >> 2;
        uint8_t sub = (fc & 0xF0) >> 4;
        bool toDS = fc & 0x0100, fromDS = fc & 0x0200;

        uint8_t *a1 = pkt + 4, *a2 = pkt + 10, *a3 = pkt + 16;
        uint8_t bssid[6], client[6];
        if (fromDS && !toDS) { memcpy(bssid, a2, 6); memcpy(client, a1, 6); }
        else if (!fromDS && toDS) { memcpy(bssid, a1, 6); memcpy(client, a2, 6); }
        else { memcpy(bssid, a3, 6); memcpy(client, a2, 6); }

        // Beacon → SSID
        if (type == 0 && sub == 8 && !haveBeacon) {
            int off = 36;
            while (off + 1 < (int)incl_len) {
                uint8_t tag = pkt[off], tlen = pkt[off+1];
                if (off + 2 + tlen > (int)incl_len) break;
                if (tag == 0x00 && tlen > 0 && tlen < 33) {
                    memcpy(ssid, pkt+off+2, tlen);
                    ssid[tlen] = '\0';
                    haveBeacon = true;
                    break;
                }
                off += 2 + tlen;
            }
        }

        // Data frame → EAPOL
        if (type == 2) {
            int hdrLen = 24 + ((fc & 0x0080) ? 2 : 0);
            int pos = hdrLen;
            if (pos + 8 > (int)incl_len || pkt[pos]!=0xAA || pkt[pos+1]!=0xAA || pkt[pos+2]!=0x03) { free(pkt); continue; }
            uint16_t etype = (uint16_t)((pkt[pos+6]<<8)|pkt[pos+7]);
            pos += 8;
            if (etype != 0x888E || pos+4 > (int)incl_len) { free(pkt); continue; }

            int eapolOff = pos;
            int eapolLen = 4 + ((pkt[pos+2]<<8)|pkt[pos+3]);
            if (eapolOff + eapolLen > (int)incl_len) { free(pkt); continue; }

            uint8_t *key = pkt + eapolOff + 4;
            if ((int)(key - pkt) + 95 > (int)incl_len) { free(pkt); continue; }

            uint16_t keyInfo = (uint16_t)((key[1]<<8)|key[2]);
            bool micSet = keyInfo & 0x0100, ack = keyInfo & 0x0080;
            bool install = keyInfo & 0x0040, secure = keyInfo & 0x0200;

            uint8_t *nonce = key + 13;
            int msgNum = -1;
            if (ack && !micSet && !install)         msgNum = 1;
            if (!ack && micSet && !install && !secure) msgNum = 2;
            if (ack && micSet && install)            msgNum = 3;
            if (!ack && micSet && !install && secure) msgNum = 4;

            if (msgNum == 1) {
                memcpy(anonce, nonce, 32);
                memcpy(apMAC, bssid, 6);
                if (!extractPMKID(pkt, incl_len, pmkid)) memset(pmkid, 0, 16);
                else hasPMKID = true;
                haveM1 = true;
            }
            if (msgNum == 2) {
                memcpy(mic, key+77, 16);
                memcpy(staMAC, client, 6);
                memcpy(apMAC, bssid, 6);
                int cp = eapolLen; if (cp > 256) cp = 256;
                memcpy(eapolMsg2, pkt+eapolOff, cp);
                eapolMsg2Len = cp;
                // Zero out MIC in EAPOL copy
                if (eapolMsg2Len >= 81+16) memset(eapolMsg2+81, 0, 16);
                haveM2 = true;
            }
            if (msgNum == 3) {
                memcpy(anonce, nonce, 32);
                memcpy(apMAC, bssid, 6);
            }
        }
        free(pkt);
    }
    f.close();
    (void)haveM1;

    if (!haveM2) return false; // Can't export without at least msg2

    // ── Write .hc22000 ──
    File hc = SD.open(hcPath, FILE_WRITE);
    if (!hc) return false;

    char pmkidHex[33]="", apHex[13]="", staHex[13]="", ssidHex[65]="";
    macToHex(apMAC, apHex);
    macToHex(staMAC, staHex);
    ssidToHex(ssid, ssidHex);

    // PMKID line: WPA*01*PMKID*APMAC*STAMAC*SSID***[optional]*\n
    if (hasPMKID) {
        bytesToHex(pmkid, 16, pmkidHex);
        hc.printf("WPA*01*%s*%s*%s*%s***\n", pmkidHex, apHex, staHex, ssidHex);
    }

    // Full handshake line: WPA*02*MIC*APMAC*STAMAC*SSID*ANONCE*EAPOL_MSG2\n
    if (haveM2 && eapolMsg2Len > 0) {
        char micHex[33]="", anonceHex[65]="", eapolHex[513]="";
        bytesToHex(mic, 16, micHex);
        bytesToHex(anonce, 32, anonceHex);
        bytesToHex(eapolMsg2, eapolMsg2Len, eapolHex);
        hc.printf("WPA*02*%s*%s*%s*%s*%s*%s\n", micHex, apHex, staHex, ssidHex, anonceHex, eapolHex);
    }

    hc.close();
    return true;
}

// ── Menu entry point ──
void eapol_export_menu() {
    FS *fs = &SD;
    if (!setupSdCard()) {
        displayError("No SD card found", true);
        return;
    }

    if (!fs->exists("/BrucePCAP/handshakes")) {
        displayError("No handshakes dir", true);
        return;
    }

    File dir = fs->open("/BrucePCAP/handshakes");
    if (!dir || !dir.isDirectory()) { displayError("Cannot open dir", true); return; }

    // Collect .pcap files
    std::vector<String> pcapFiles;
    File entry = dir.openNextFile();
    while (entry) {
        String name = entry.name();
        if (!entry.isDirectory() && name.endsWith(".pcap")) {
            pcapFiles.push_back(name);
        }
        entry.close();
        entry = dir.openNextFile();
    }
    dir.close();

    if (pcapFiles.empty()) {
        displayError("No .pcap files found", true);
        return;
    }

    while (true) {
        std::vector<Option> opts;
        for (size_t i = 0; i < pcapFiles.size(); i++) {
            String label = pcapFiles[i];
            if (label.length() > 28) label = label.substring(0, 25) + "...";
            opts.push_back({label, [i, &pcapFiles, &fs]() {
                String f = pcapFiles[i];
                String fullPath = "/BrucePCAP/handshakes/" + f;
                drawMainBorderWithTitle("Converting...", true);
                tft.setTextSize(FP);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                tft.setCursor(10, BORDER_PAD_Y + 10);
                tft.print("Converting: " + f);
                tft.drawPixel(0, 0, 0);

                if (convertPcapToHC22000(fullPath.c_str())) {
                    displayInfo("Exported: " + f + ".hc22000\nFile removed from list", true);
                    pcapFiles.erase(pcapFiles.begin() + i);
                } else {
                    displayError("Export failed (no handshake data)", true);
                }
            }});
        }
        opts.push_back({"Back", []() {}});

        int sel = loopOptions(opts, MENU_TYPE_SUBMENU, "EAPOL Export");
        if (sel < 0 || sel == (int)opts.size()-1) break;        // Back or cancel
        if (pcapFiles.empty()) break;                            // All converted
    }
}
#endif
