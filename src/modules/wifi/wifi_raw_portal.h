#ifndef __WIFI_RAW_PORTAL_H__
#define __WIFI_RAW_PORTAL_H__

// Raw 802.11 injection/capture engine, evil-twin captive portal and L4
// packet builders, ported from Vandeta. Upstream Bruce has none of this;
// it lives in its own module so wifi_atks.cpp keeps its own history.

#include <Arduino.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <stddef.h>
#include <stdint.h>
#include <vector>

/**
 * @brief Spam 802.11 beacon frames for a bounded amount of time.
 *
 * @param mode 0 = funny SSID list, 1 = rickroll list, 2 = random SSIDs,
 *             4 = single SSID (baseSSID followed by a counter)
 * @param baseSSID base SSID used by mode 4
 * @param durationMs how long to transmit before returning (capped at 5 minutes)
 * @return number of beacon frames sent
 */
int headlessBeaconSpam(uint8_t mode, const String &baseSSID, uint32_t durationMs);

// ============================================================================
// Raw 802.11 injection / capture engine.
//
// Backs the wifi.injectPacket / wifi.sendRaw80211 / wifi.captureStart /
// wifi.captureStop / wifi.getCapturedPackets / wifi.setPromiscuous bindings.
// Everything here is bounded: injections are capped in length and count,
// captures stop on their own deadline, and captured bytes live in PSRAM.
// ============================================================================

/// Largest frame accepted by wifi_raw_inject() and stored by the capture engine.
#define WIFI_RAW_MAX_FRAME 1500
/// Capture is capped at WIFI_CAP_MAX_PACKETS packets or WIFI_CAP_MAX_BYTES bytes.
#define WIFI_CAP_MAX_PACKETS 100
#define WIFI_CAP_MAX_BYTES (50 * 1024)
/// Number of capture sessions that can be held (only one runs at a time).
#define WIFI_CAP_MAX_SESSIONS 2

struct WifiRawTxResult {
    bool ok;
    int sent;
    int failed;
    int len;
    uint8_t channel;
    const char *iface; // "sta" or "ap"
    const char *error; // NULL when ok
};

/**
 * @brief Bring WiFi up in AP+STA (the state raw injection needs) and tune the radio.
 *
 * @param channel 1-14 to move the radio, anything else to keep the current channel
 * @return true when WiFi is usable
 */
bool wifi_raw_prepare(int channel);

/**
 * @brief Transmit a raw 802.11 frame with esp_wifi_80211_tx().
 *
 * Tries the station interface first (the only one supported on C-series parts) and
 * falls back to the AP interface, so the same call works across targets. The AP
 * callback is bypassed here, which is why arbitrary frames get through.
 *
 * @param frame 802.11 frame, starting at the frame control field
 * @param len frame length in bytes (10..WIFI_RAW_MAX_FRAME)
 * @param channel 1-14
 * @param repeat how many times to transmit (clamped to 1..64)
 * @param iface -1 = auto, 0 = station, 1 = AP
 * @param appendFcs append a CRC32 FCS (required by some parts for data frames)
 * @param gapMs delay between repeats (clamped to 0..100)
 */
WifiRawTxResult wifi_raw_inject(
    const uint8_t *frame, size_t len, uint8_t channel, uint8_t repeat = 1, int iface = -1,
    bool appendFcs = false, uint32_t gapMs = 0
);

/**
 * @brief Start a promiscuous capture session.
 *
 * @param channel 1-14
 * @param timeoutMs 0 = one minute; otherwise clamped to 200..60000 ms
 * @return session id (1..WIFI_CAP_MAX_SESSIONS) or -1 on failure
 */
int wifi_raw_capture_start(uint8_t channel, uint32_t timeoutMs);

/**
 * @brief Stop a session. Captured packets stay readable until the slot is reused.
 */
bool wifi_raw_capture_stop(int sessionId);

/**
 * @brief Stop any session whose deadline has passed. Call before using a session.
 * @return true when a session was reaped
 */
bool wifi_raw_capture_poll();

bool wifi_raw_capture_running(int sessionId);
int wifi_raw_capture_packet_count(int sessionId);
int wifi_raw_capture_dropped(int sessionId);
int wifi_raw_capture_channel(int sessionId);

/**
 * @brief Fetch one captured packet. @p data points into the session's PSRAM pool.
 */
bool wifi_raw_capture_packet(
    int sessionId, int index, uint32_t *timestamp, uint8_t *channel, int8_t *rssi, const uint8_t **data,
    uint16_t *len
);

/**
 * @brief Toggle promiscuous mode. Disabling it also stops any running capture.
 */
bool wifi_raw_set_promiscuous(bool enable, uint8_t channel);

/**
 * @brief Release capture buffers, drop promiscuous mode and undo the WiFi bring-up.
 */
void wifi_raw_cleanup();

// ============================================================================
// Evil-twin captive portal engine.
//
// Backs the wifi.portal() binding. Deliberately generic: it serves whatever
// page (or HTTP 302 redirect) it is handed, hijacks DNS for every name and can
// deauth a real AP so its clients fall onto the twin. The same engine therefore
// drives a rickroll, a fake router login, a "free wifi" splash page, etc.
//
// Run it with wifi_portal_run(), which only returns once the deadline expires,
// the user pressess the back button or a fatal error occurs - it always tears
// the AP and servers down before returning.
// ============================================================================

/// Portal runtime bounds.
#define WIFI_PORTAL_MIN_MS 2000
#define WIFI_PORTAL_MAX_MS 600000

struct WifiPortalConfig {
    String ssid;             // SSID of the twin AP (1-32 chars)
    uint8_t channel = 6;     // 1-14
    String password;         // empty = open network
    String html;             // page served for every request
    String htmlFile;         // used when html is empty, e.g. "/portals/en/rickroll.html"
    String redirect;         // when set, every request 302s here (beats html)
    String gateway;          // twin's IP; empty = bruceConfig.evilPortalGatewayIp
    uint32_t durationMs = 60000; // clamped to WIFI_PORTAL_MIN_MS..WIFI_PORTAL_MAX_MS
    bool hijackDns = true;   // answer every DNS query with the twin's IP
    bool deauth = false;     // kick clients off the real AP so they join the twin
    uint8_t targetBssid[6] = {0};
    bool hasTargetBssid = false;
    // Optional single deauth target (a client MAC found on the channel). Empty
    // means broadcast, which is what "all devices" uses.
    uint8_t targetClient[6] = {0};
    bool hasTargetClient = false;
    uint8_t maxClients = 4;
    // Paint the run screen (ucmitm does; a script does not).
    bool draw = false;
};

struct WifiPortalStats {
    bool ok = false;
    uint8_t channel = 0;       // channel the twin actually started on
    uint32_t requests = 0;     // HTTP requests answered
    uint32_t deauthFrames = 0; // deauth frames sent at the real AP
    int clients = 0;           // stations associated with the twin when it stopped
    uint32_t elapsedMs = 0;
    String ip;                 // twin's IP while it was up
    // Form fields seen (a fake-login twin posting a Wi-Fi password). Persisted
    // to /BruceEvilCreds/ucmitm_creds.csv so the capture outlives the run.
    uint32_t fieldSets = 0;
    String lastFields;
    const char *error = NULL;  // set when ok is false
};

/**
 * @brief Run an evil-twin captive portal until its deadline or the back button.
 *
 * Brings WiFi down and back up as an AP (so the portal owns the radio), serves
 * @p cfg.html (or 302s to @p cfg.redirect) on every request, answers the
 * Windows/macOS/Android/Linux captive-portal probe paths with a redirect so the
 * victim's OS opens its "sign in to network" browser, and optionally deauths the
 * client's real AP to push it onto the twin.
 *
 * @return stats; @c ok is false (with @c error set) when the portal never came up
 */
WifiPortalStats wifi_portal_run(const WifiPortalConfig &cfg);

// ============================================================================
// Application layer injection (DNS / UDP / TCP payloads).
//
// These build a complete 802.11 data frame - 802.11 header, LLC/SNAP, IPv4, then
// UDP or TCP and the payload - and hand it to wifi_raw_inject(), so a script can
// answer a client's DNS query or push an HTTP response at it.
//
// Reach: a forged data frame only lands on an OPEN (or WEP) network. On
// WPA2/WPA3 the client's radio requires the frame to be encrypted with its
// pairwise key, which this code does not have. Also match the values from the
// real packet (destination port, TCP seq/ack, DNS transaction id) or the
// client's stack silently drops the frame - that is what wifi_parse_frame() and
// the wifi.packetInfo() binding are for.
// ============================================================================

/// Largest L4 payload an injected frame can carry (frame cap minus headers).
#define WIFI_L4_MAX_PAYLOAD 1400

struct WifiL4Result {
    bool ok;
    int frames;     // transmissions the driver accepted
    int len;        // total 802.11 frame length
    int payloadLen; // L4 payload length
    uint8_t channel;
    const char *error; // NULL when ok
};

/**
 * @brief Build a DNS A-record response payload.
 *
 * @param out destination buffer
 * @param cap space available in @p out
 * @param txid transaction id copied from the client's query
 * @param domain queried name, e.g. "example.com"
 * @param answerIp dotted quad returned as the A record
 * @param ttl answer time to live in seconds
 * @return payload length, or 0 when the inputs do not fit / do not parse
 */
size_t wifi_build_dns_response(
    uint8_t *out, size_t cap, uint16_t txid, const char *domain, const char *answerIp, uint32_t ttl
);

/**
 * @brief Send a raw UDP datagram to one station inside an 802.11 data frame.
 *
 * @param dstMac station to deliver to (addr1); broadcast MAC floods every station
 * @param bssid BSSID used as transmitter/source (addr2/addr3)
 * @param srcIp/dstIp addresses in host byte order; dstIp 0xFFFFFFFF (broadcast)
 *                  is accepted by the receiving IP stack without knowing its IP
 */
WifiL4Result wifi_inject_udp(
    const uint8_t dstMac[6], const uint8_t bssid[6], uint32_t srcIp, uint32_t dstIp, uint16_t srcPort,
    uint16_t dstPort, const uint8_t *payload, size_t payloadLen, uint8_t channel, uint8_t repeat, uint32_t gapMs
);

/**
 * @brief Send a raw TCP segment (payload included) to one station.
 *
 * The segment must carry the sequence/acknowledgement numbers of the real
 * connection, otherwise the receiver's TCP stack discards it: capture the
 * client's request first and pass its values in.
 *
 * @param flags TCP flag byte, e.g. 0x18 = PSH+ACK (the usual data segment)
 */
WifiL4Result wifi_inject_tcp(
    const uint8_t dstMac[6], const uint8_t bssid[6], uint32_t srcIp, uint32_t dstIp, uint16_t srcPort,
    uint16_t dstPort, uint32_t seq, uint32_t ack, uint8_t flags, const uint8_t *payload, size_t payloadLen,
    uint8_t channel, uint8_t repeat, uint32_t gapMs
);

/**
 * @brief Decode a dotted quad ("192.168.4.1") into host byte order.
 *
 * @return false when the string is not a valid IPv4 address
 */
bool wifi_parse_ipv4(const char *s, uint32_t *out);

/// Fields wifi_parse_frame() extracts. Pointer members point into the frame buffer.
struct WifiParsedFrame {
    bool valid;
    uint8_t type;    // 0 management, 1 control, 2 data
    uint8_t subtype;
    bool protectedFrame; // WPA/WPA2 encrypted body: L3 fields cannot be read
    bool toDs, fromDs;
    uint8_t addr1[6], addr2[6], addr3[6]; // receiver, transmitter, bssid
    bool hasIp;
    uint8_t ipProto; // 6 TCP, 17 UDP
    uint32_t ipSrc, ipDst;
    uint16_t ipTotalLen;
    bool hasUdp;
    uint16_t udpSrcPort, udpDstPort;
    bool hasTcp;
    uint16_t tcpSrcPort, tcpDstPort;
    uint32_t tcpSeq, tcpAck;
    uint8_t tcpFlags;
    bool hasDns;
    uint16_t dnsTxId;
    bool dnsIsResponse;
    uint16_t dnsQType, dnsQClass;
    char dnsQName[64]; // first question name, lower-cased, empty when absent
    const char *error; // NULL when valid
};

/**
 * @brief Decode one 802.11 frame (hex from wifi.getCapturedPackets) into fields.
 *
 * Understands data frames carrying LLC/SNAP + IPv4 + UDP/TCP, and the DNS
 * question inside a UDP port 53 payload. Management frames parse to the header
 * fields only.
 */
bool wifi_parse_frame(const uint8_t *frame, size_t len, WifiParsedFrame *out);


#endif
