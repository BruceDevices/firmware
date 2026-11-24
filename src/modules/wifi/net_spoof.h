#ifndef __NET_SPOOF_H__
#define __NET_SPOOF_H__

#if !defined(LITE_VERSION)

/**
 * @file net_spoof.h
 * @brief Headless MITM engine for the Bruce JS interpreter.
 *
 * Backs the arpSpoof / dnsSpoof / httpInterceptor JavaScript bindings.
 *
 * Why this works on WPA2 (and why the raw-802.11 injectors do not):
 * every frame here is handed to the driver with esp_wifi_internal_tx() on the
 * *associated station* interface. The station link is already keyed, so the
 * hardware encrypts the frame with the pairwise key before it leaves the radio.
 * A frame built by wifi_raw_inject() is transmitted without a key and is
 * therefore dropped by WPA2 clients. That difference is the whole reason this
 * module talks through the netif instead of through esp_wifi_80211_tx().
 *
 * What it can and cannot do:
 *   - ARP spoofing puts this board between a target and its gateway in both
 *     directions (the classic two-way poison), so the target's traffic arrives
 *     at this board's MAC.
 *   - Because the frame arrives here, a layer-2 hook can read the target's DNS
 *     queries and answer them from this board.
 *   - Plain HTTP is intercepted on ANY TCP port (not just 80/8080): the hook
 *     completes the handshake and answers the request with a 302 or a page.
 *   - HTTPS is served by a real TLS endpoint on NETSPOOF_HTTPS_PORT with a
 *     SELF-SIGNED certificate. While HTTPS interception is active every DNS A
 *     answer points at this board, so the victim's TLS connection reaches the
 *     endpoint; the browser first shows a certificate warning, then (after
 *     continuing, or immediately for a client that does not validate) receives
 *     the same 302/page. A cached IP or a secure-DNS client bypasses this, and
 *     a certificate warning cannot be avoided without a trusted CA.
 *   - There is NO forwarding: the ESP32 cannot NAT/route, so a spoofed target
 *     loses its real internet path for as long as the poison is active. This is
 *     an interception MITM, not a transparent one. netspoof_arp_stop()
 *     (or dropping the interpreter) restores the real ARP mappings.
 */

#include <Arduino.h>
#include <IPAddress.h>
#include <stdint.h>

// ---------------------------------------------------------------------------
// Bounds
// ---------------------------------------------------------------------------

// A /24 can hold 254 hosts, and "all devices" on a busy network is expected to
// cover a couple of hundred of them. Both tables live in PSRAM (allocated on
// first use, internal heap only as a fallback), so the size costs nothing in the
// internal DRAM the radios need.
#define NETSPOOF_MAX_TARGETS 256     // hosts remembered by a discovery scan
#define NETSPOOF_MAX_PAIRS 200       // simultaneous ARP spoof pairs
#define NETSPOOF_MAX_RULES 8         // DNS rules and HTTP rules each
#define NETSPOOF_MAX_CAPTURES 8      // ring buffer served by getInterceptedData()
#define NETSPOOF_CAPTURE_PAYLOAD 320 // bytes of the request kept per capture
#define NETSPOOF_QUEUE_DEPTH 6       // L2 hook -> service task queue
#define NETSPOOF_HOOK_COPY 640       // bytes copied out of a frame by the hook
#define NETSPOOF_FRAME_MAX 1514      // ethernet frame ceiling
#define NETSPOOF_POISON_INTERVAL_MS 250 // ARP re-poison period
#define NETSPOOF_ARP_RETRY_MS 400       // re-ask for a MAC while resolving it
#define NETSPOOF_SVC_TICK_MS 10      // service task tick
#define NETSPOOF_HTTPS_PORT 443      // self-signed TLS endpoint

// Capture rings (NetSpoofCapture.kind).
#define NETSPOOF_CAPTURE_DNS 0
#define NETSPOOF_CAPTURE_HTTP 1
#define NETSPOOF_CAPTURE_HTTPS 2

// ---------------------------------------------------------------------------
// Public data shapes
// ---------------------------------------------------------------------------

/// One host found by the ARP discovery scan.
struct NetSpoofTarget {
    IPAddress ip;
    uint8_t mac[6];
    String macStr;   // "AA:BB:CC:DD:EE:FF"
    bool gateway;    // this entry IS the default gateway
    bool spoofed;    // an active ARP pair covers this IP
    uint8_t pairedWith[6]; // gateway MAC when spoofed
};

/// One DNS or HTTP rule.
struct NetSpoofRule {
    String match;   // domain / URL pattern, lower-cased
    String value;   // answer IP, redirect URL or HTML body
    bool wildcard;  // true = matches everything (startAll('*', ...))
    uint32_t hits;
};

/// One captured (and possibly modified) request.
struct NetSpoofCapture {
    uint32_t ts;      // millis() when it was seen
    uint8_t kind;     // NETSPOOF_CAPTURE_DNS / _HTTP / _HTTPS
    uint8_t action;   // 0 = observed only, 1 = answered/redirected, 2 = injected
    IPAddress clientIp;
    char clientMac[18];
    char method[8];   // "GET", "POST", ... (HTTP) or "DNS"
    char host[48];    // Host header, or the DNS question name
    char path[96];    // request path / query string
    char data[NETSPOOF_CAPTURE_PAYLOAD]; // request snippet or decoded query
};

// ---------------------------------------------------------------------------
// Preconditions
// ---------------------------------------------------------------------------

/**
 * @brief Check that the station is associated (all of this module needs it).
 * @param err filled with a human-readable reason when false is returned
 */
bool netspoof_wifi_ready(String &err);

/// True when the L2 hook is installed on the station netif.
bool netspoof_hook_installed();

// ---------------------------------------------------------------------------
// ARP spoofing
// ---------------------------------------------------------------------------

/**
 * @brief ARP-scan the local subnet and fill the target list.
 *
 * Sends one ARP request per host address and reads the LwIP ARP table, the same
 * method Bruce's ARPScanner / NetCut use. Blocks for the scan duration
 * (bounded by @p timeoutMs).
 *
 * @return number of hosts found (0 is a valid answer, not necessarily an error)
 */
int netspoof_scan_targets(uint32_t timeoutMs, String &err);

int netspoof_target_count();
const NetSpoofTarget *netspoof_target(int idx);
void netspoof_clear_targets();

/// Current default gateway and its MAC (BSSID fallback when the ARP table is empty).
bool netspoof_gateway_info(IPAddress &gw, uint8_t mac[6]);

/// Resolve one IPv4 address to a MAC with an ARP request + table poll.
bool netspoof_resolve_mac(const IPAddress &ip, uint8_t mac[6], uint32_t timeoutMs);

/// This board's station MAC.
void netspoof_my_mac(uint8_t mac[6]);

/**
 * @brief Start (or add) a two-way ARP spoof between @p target and @p gateway.
 *
 * Poisons both ends so target->gateway and gateway->target traffic comes here.
 * The pairing is re-poisoned every NETSPOOF_POISON_INTERVAL_MS by the service
 * task until netspoof_arp_stop(), so it survives while the script does other
 * work. Returns false with @p err set when the station is down, the addresses
 * do not resolve or the pair table is full.
 */
bool netspoof_arp_start(const IPAddress &target, const IPAddress &gateway, String &err);

/**
 * @brief Start a pair for a target whose MAC is already known.
 *
 * Same as netspoof_arp_start(), but skips the unicast ARP resolve and uses
 * @p targetMac - which is what an ARP scan already saw. A host that answers the
 * scan's broadcast request and then ignores the unicast one (phones in doze do)
 * can only be paired this way.
 */
bool netspoof_arp_start_mac(
    const IPAddress &target, const uint8_t targetMac[6], const IPAddress &gateway, String &err
);

/**
 * @brief Restore every real ARP mapping and stop poisoning.
 * Sends the correct gateway MAC to each target and the correct target MAC to
 * the gateway (reply + broadcast request, RFC 826) before dropping the pairs.
 */
bool netspoof_arp_stop(String &err);

/// Restore without touching the error string (used on interpreter exit).
void netspoof_arp_restore_all();

bool netspoof_arp_active();
int netspoof_arp_pair_count();
uint32_t netspoof_arp_frames();
uint32_t netspoof_arp_uptime_ms();
bool netspoof_arp_pair_info(
    int idx, IPAddress &target, IPAddress &gateway, char *targetMac, char *gatewayMac
);
/// True when @p ip is covered by an active pair.
bool netspoof_arp_pair_index(const IPAddress &ip, int *idx);

// ---------------------------------------------------------------------------
// DNS spoofing (needs an active ARP pair to see the queries)
// ---------------------------------------------------------------------------

bool netspoof_dns_start(String &err);
bool netspoof_dns_stop(String &err);
bool netspoof_dns_active();
uint32_t netspoof_dns_queries();
uint32_t netspoof_dns_answers();

// ---------------------------------------------------------------------------
// Transport fallback (1.4)
// ---------------------------------------------------------------------------

/**
 * @brief Counters for the "keep the client on the IPv4/TCP path" barrier.
 *
 * While any rule set is armed (DNS, HTTP or the HTTPS endpoint) the engine
 * drops QUIC/DoQ, DNS-over-TLS and IPv6 from the victim, and answers AAAA /
 * SVCB / HTTPS with no records. That is what makes a dual-stack or HTTP/3
 * client fall back to the intercepted path instead of bypassing it. Observe-only
 * recon never arms the barrier, so these stay 0 there.
 *
 * `quicDropped` covers UDP/443 and UDP/853, `dotDropped` TCP/853,
 * `svcbSuppressed` counts the empty AAAA/SVCB/HTTPS answers, and
 * `ipv6Dropped` counts the victim's IPv6 frames discarded by the layer-2 hook.
 */
uint32_t netspoof_quic_dropped();
uint32_t netspoof_dot_dropped();
uint32_t netspoof_svcb_suppressed();
uint32_t netspoof_ipv6_dropped();

/**
 * @brief Add a DNS rule.
 * @param domain name to match, or "*" for every query
 * @param ip dotted quad returned for a match
 * @param wildcard true = match every query
 */
bool netspoof_dns_add_rule(const String &domain, const String &ip, bool wildcard, String &err);
void netspoof_dns_clear_rules();
int netspoof_dns_rule_count();
const NetSpoofRule *netspoof_dns_rule(int idx);
/// The IP a query name resolves to, or false when no rule matches.
bool netspoof_dns_lookup(const String &name, IPAddress &ip);

// ---------------------------------------------------------------------------
// Transparent forwarding (the layer-2 bridge)
// ---------------------------------------------------------------------------

/**
 * @brief Bridge poisoned clients to the real gateway so they keep internet.
 *
 * The ARP poison makes every victim frame arrive at this board, and the ESP has
 * no routing/NAT: without a bridge the victim loses its path and can never load
 * a page it was redirected to. Forwarding re-sends those frames to the real
 * gateway (and the replies to the victim) with the MACs rewritten, leaving the
 * IP packet untouched.
 *
 * With the bridge up, the engine intercepts what it can rewrite (DNS answers,
 * plaintext HTTP on any port, TLS addressed to the board after DNS pinning) and
 * bridges the rest - which is what lets a redirect to a real video work.
 *
 * @param err filled with the reason when false is returned
 * @return true when the bridge is armed (station up and gateway known)
 */
bool netspoof_forward_start(String &err);
void netspoof_forward_stop();
bool netspoof_forward_active();
uint32_t netspoof_forward_frames();
uint32_t netspoof_forward_dropped();

/**
 * @brief Keep @p host (and its subdomains) resolving for real.
 *
 * Everything else is pinned to the board while the HTTPS endpoint runs so it
 * is intercepted. The host of every redirect rule is added automatically -
 * otherwise following the redirect would resolve back to this board and loop.
 * Up to NETSPOOF_DNS_PASS_MAX hosts are remembered; the newest wins.
 */
void netspoof_dns_add_passthrough(const String &host);
void netspoof_dns_clear_passthrough();

// ---------------------------------------------------------------------------
// HTTP interception (needs an active ARP pair to see the requests)
// ---------------------------------------------------------------------------

/**
 * @brief Start plaintext HTTP interception on every TCP port.
 *
 * Also brings up the HTTPS endpoint (netspoof_https_start()) so a redirect
 * rule covers https:// as well. If the TLS server cannot start (usually RAM),
 * the plaintext path still runs and the reason is logged.
 */
bool netspoof_http_start(String &err);
bool netspoof_http_stop(String &err);
bool netspoof_http_active();
uint32_t netspoof_http_requests();
uint32_t netspoof_http_redirected();
uint32_t netspoof_http_injected();

// ---------------------------------------------------------------------------
// HTTPS interception (self-signed TLS endpoint, same rules as HTTP)
// ---------------------------------------------------------------------------

/**
 * @brief Start the self-signed HTTPS endpoint and point DNS at this board.
 *
 * The endpoint listens on NETSPOOF_HTTPS_PORT and applies the same redirect /
 * injection rules as the plaintext interceptor. It needs the station to be
 * associated. While it is active every DNS A query is answered with the
 * board's own address (so the victim's TLS connection lands here); the normal
 * DNS rules are bypassed for A records until it stops.
 *
 * A TLS client only accepts a certificate it trusts, so the first visit shows
 * the browser's certificate warning. That is a protocol limit, not a bug.
 */
bool netspoof_https_start(String &err);
bool netspoof_https_stop(String &err);
bool netspoof_https_active();
uint32_t netspoof_https_requests();
uint32_t netspoof_https_redirected();
uint32_t netspoof_https_injected();
uint16_t netspoof_https_port();

bool netspoof_http_add_redirect(const String &pattern, const String &url, bool wildcard, String &err);
bool netspoof_http_add_injection(const String &pattern, const String &html, bool wildcard, String &err);
/// Page answered to the OS captive-portal probe (see the `portal` payload line).
void netspoof_http_set_portal_page(const String &html);
String netspoof_http_portal_page();
void netspoof_http_clear_rules();
/// Drop only the injection (page) rules, leaving redirects in place.
void netspoof_http_clear_injections();
/// Drop only the redirect rules, leaving the page payload in place.
void netspoof_http_clear_redirects();
int netspoof_http_redirect_count();
int netspoof_http_inject_count();
const NetSpoofRule *netspoof_http_redirect_rule(int idx);
const NetSpoofRule *netspoof_http_inject_rule(int idx);

// ---------------------------------------------------------------------------
// Observe-only recon (no rule armed, nothing answered)
// ---------------------------------------------------------------------------

/**
 * @brief Watch the traffic without changing it.
 *
 * A recon payload (`forward auto` + `info` + `probe all`, no page/redirect/DNS
 * line) must show which domains a network visits without redirecting anybody.
 * With no rule armed the normal engines answer nothing - and therefore record
 * nothing either, which is why such a payload used to look broken. This mode
 * installs the same L2 hook but only reads: the DNS question name, the Host
 * header of every plaintext request and the SNI of every TLS ClientHello are
 * pushed as captures (action 0 = observed), and the frame is handed straight to
 * the forwarding bridge. Nothing is answered, nothing is redirected, so the
 * client's requests and its internet path stay exactly as they were.
 *
 * Needs the station to be associated (the hook lives on the station netif) and
 * `forward` for the frames to reach the real gateway.
 */
bool netspoof_observe_start(String &err);
void netspoof_observe_stop();
bool netspoof_observe_active();

// ---------------------------------------------------------------------------
// Captured data
// ---------------------------------------------------------------------------

int netspoof_capture_count();
/// Monotonic count of captures since boot. capture_count() saturates at the
/// ring size, so a poller that remembers this can tell new entries apart even
/// when several captures share one millisecond.
uint32_t netspoof_capture_total();
/// @param idx 0 = newest capture
const NetSpoofCapture *netspoof_capture(int idx);
/// Short name for a capture kind: "dns", "http" or "https".
const char *netspoof_capture_kind_name(uint8_t kind);
void netspoof_capture_clear();

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

/// Stop interception, restore ARP and release the service task/queue.
void netspoof_cleanup();

#endif // !LITE_VERSION
#endif // __NET_SPOOF_H__
