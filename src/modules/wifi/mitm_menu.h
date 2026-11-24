#ifndef __MITM_MENU_H__
#define __MITM_MENU_H__

#include <Arduino.h>

#if !defined(LITE_VERSION)

/**
 * @file mitm_menu.h
 * @brief On-device UI and payload files for the net_spoof MITM engine.
 *
 * The engine itself (net_spoof.cpp) was written for the JavaScript bindings and
 * is unchanged: arpSpoof / dnsSpoof / httpInterceptor still drive it. This file
 * adds the two missing pieces so the same attacks can be used without a script:
 *
 *   - the `DNS Inject` and `HTML Intercept` entries of WiFi > Wifi Atks, built
 *     on the engine's public API only;
 *   - the payload files those menus load from the card.
 *
 * Payload files (on the mounted filesystem, SD first):
 *
 *   /BruceSpoof/*.mitm          ONE payload for the whole attack (see below)
 *   /BruceSpoof/*.ucmitm        the same language for the unconnected attack
 *                               (ucmitm: no password, twin AP + deauth)
 *   /BruceSpoof/html/*.html     a page a payload can serve (one ethernet frame)
 *
 * `.mitm` is the only connected payload format. The standalone dns.txt /
 * redirect.txt / mitm.txt files this module used to read are gone, and the
 * picker only offers .mitm (or .ucmitm while ucmitm is on).
 *
 * A payload (`.mitm` / `.ucmitm`) is the keypad-free way to describe a whole
 * run. It is a plain-text list of commands,
 * one per line, `#` for comments - a thin, readable front end for the same
 * engine the JavaScript API exposes:
 *
 *   page <match> <file>         httpInterceptor.addInjection()   200 + HTML
 *   redirect <match> <url>      httpInterceptor.addRedirect()    302
 *   dns <name|*> <ip>           dnsSpoof.addRule() / startAll()
 *                               (<ip> may be `gateway` or `this`; the token is
 *                               resolved on each query, so the payload loads
 *                               even while the station is still down)
 *   target all|<ip>             arpSpoof.start() - overrides the page's mode
 *   fallback dns <ip> [secs]    if nothing was injected/redirected in <secs>
 *   fallback redirect <url> [secs]   ... try this instead (default 10 s)
 *                               Chain up to 3 `fallback` lines; each <secs> is
 *                               measured from the start, and a step only fires
 *                               while the attack still has no hit.
 *   info                        log the network facts (SSID/channel/board IP,
 *                               gateway IP + MAC) before the poison starts
 *   probe all|<ip>              log what is actually there: an ARP resolve per
 *                               address, `all` reuses the scan the pairing did
 *   forward off|auto|on         bridge the victim to the real gateway so it
 *                               keeps internet (a redirect can then load)
 *   offline page <file>         served instead of the redirect when the bridge
 *                               cannot start (no internet path without it)
 *
 * `info` / `probe` are read-only reconnaissance - they never start a pair or
 * change a rule, so a payload can report what the target is and still use the
 * same page/redirect/DNS lines afterwards.
 *
 * A `.html` payload is simply `page * <that file>`.
 *
 * Nothing here sets up WiFi on its own beyond asking the station to associate:
 * the engine needs a real association because the frames go out on the keyed
 * station link (see the note at the top of net_spoof.h).
 */

#include "net_spoof.h"

/// Directory holding every MITM payload.
// Windows a talking host keeps the sweep before it rotates on.
#define MITM_SWEEP_WINDOWS 4

// Idle windows a sweep host gets before the run drops it.
#define MITM_SWEEP_IDLE_ROUNDS 3

#define MITM_PAYLOAD_DIR "/BruceSpoof"
/// Extensions the Payload picker offers: connected, and unconnected (ucmitm).
#define MITM_PAYLOAD_FILTER "MITM"
#define MITM_UC_PAYLOAD_FILTER "UCMITM"
/// The one example file the menu can create.
#define MITM_EXAMPLE_PAYLOAD MITM_PAYLOAD_DIR "/example.mitm"
#define MITM_UC_EXAMPLE_PAYLOAD MITM_PAYLOAD_DIR "/example.ucmitm"
#define MITM_DNS_PAYLOAD MITM_PAYLOAD_DIR "/dns.txt"
#define MITM_HTML_DIR MITM_PAYLOAD_DIR "/html"
#define MITM_REDIRECT_PAYLOAD MITM_PAYLOAD_DIR "/redirect.txt"
#define MITM_SCRIPT_PAYLOAD MITM_PAYLOAD_DIR "/mitm.txt"

/// How much of a DNS / redirect payload is read (rules are 8 x ~96 bytes).
#define MITM_DNS_MAX_BYTES 1024
#define MITM_REDIRECT_MAX_BYTES 2048
/// A unified payload is a few command lines; 4 KB is ~100 of them.
#define MITM_SCRIPT_MAX_BYTES 4096
/// How long a `fallback` line waits for an HTTP hit when it names no time is the
/// task 1 tunable `mitm.fallback_delay_ms` (default 10000, the constant this
/// used to be). The parser reads it through mitmParseSeconds(); the struct
/// fields below stay 0 until a line actually carries a time.
/// How many `fallback` lines one payload can chain (tried in file order).
#define MITM_FALLBACK_MAX 3
/// How many `probe <ip>` lines one payload can carry.
#define MITM_PROBE_MAX 4
/// How many `skip <ip>` lines one payload can carry (never poisoned).
#define MITM_SKIP_MAX 8
/// How many hosts a `probe all` line lists (the feed keeps 10 lines anyway).
#define MITM_PROBE_LINES 8
/// ARP window used by `probe all` when the pairing has not scanned yet.
#define MITM_PROBE_SCAN_MS 6000
/// How long a recon run watches the air when ARP pairing is not possible.
#define MITM_RECON_SNIFF_MS 8000
/// Discovery window for the clients of a ucmitm target when nothing was sniffed.
#define MITM_UC_DISCOVERY_MS 4000
/// The "Save logs" toggle writes one run's feed here, one file per network.
///
/// The attack never touches the card while it runs: the feed is buffered in RAM
/// (PSRAM) and written when the run closes, so logging cannot slow it down.
#define MITM_LOG_DIR "/wifi/MITM"
/// How much of one run's feed is buffered before the log says it was cut.
#define MITM_LOG_MAX_BYTES 131072
/// An injected page shares one ethernet frame with the IP/TCP/HTTP headers.
#define MITM_HTML_MAX 1100
/// Shown on screen when a payload file cannot be opened.
#define MITM_PAYLOAD_HINT "Create it from the menu (Create example)."

/// Outcome of loading one payload file.
struct MitmPayloadResult {
    bool ok = false;
    int loaded = 0;    // rules accepted by the engine
    int bad = 0;       // lines rejected (syntax, bad IP, rule table full)
    int skipped = 0;   // blank lines and comments
    bool truncated = false; // file was longer than the size cap
    String path;
    String error;      // set when ok is false
};

// ---------------------------------------------------------------------------
// Payload files
// ---------------------------------------------------------------------------

/**
 * @brief Parse /BruceSpoof/dns.txt style payloads into DNS rules.
 *
 * Replaces the current rule set unless @p clearFirst is false. A malformed line
 * is counted and skipped; it never aborts the load. `*` as the domain makes the
 * rule a wildcard, so it should be the last line of the file.
 */
MitmPayloadResult mitmPayloadLoadDns(const String &path = MITM_DNS_PAYLOAD, bool clearFirst = true);

/// Parse /BruceSpoof/redirect.txt style payloads into HTTP 302 rules.
MitmPayloadResult mitmPayloadLoadRedirects(
    const String &path = MITM_REDIRECT_PAYLOAD, bool clearFirst = true
);

/**
 * @brief Read one HTML file and install it as the injection rule.
 *
 * @param match host/substring to match, `*` (or an empty string) for every
 *              plain-HTTP request
 */
MitmPayloadResult mitmPayloadLoadHtml(const String &path, const String &match);

/// Read at most @p maxBytes of a text payload; the result carries the reason.
String mitmPayloadRead(const String &path, size_t maxBytes, MitmPayloadResult &res);

// ---------------------------------------------------------------------------
// Unified payload (the MITM Attack page)
// ---------------------------------------------------------------------------

/**
 * One fallback step of a payload.
 *
 * A payload may chain up to MITM_FALLBACK_MAX of them (`fallback dns ...` then
 * `fallback redirect ...`). Each @c afterMs is measured from the start of the
 * attack, not from the previous step, so a payload reads as a timetable: DNS
 * after 10 s, a 302 after 25 s. A step only fires while the attack has not
 * produced a single hit yet, so a working rule is never overridden.
 */
struct MitmFallbackStep {
    bool dns = false;       // true = DNS answer, false = 302 rule
    String value;           // IP or URL
    uint32_t afterMs = 0; // filled in by the parser from mitm.fallback_delay_ms
};

/// Outcome of loading one `.mitm` / `.txt` / `.html` payload.
struct MitmScriptResult {
    bool ok = false;
    int pages = 0;      // page rules armed (page)
    int redirects = 0;  // redirect rules armed (redirect)
    int dns = 0;        // DNS rules armed (dns)
    int bad = 0;        // lines rejected
    int skipped = 0;    // comments and blank lines
    bool truncated = false;
    bool fallbackSet = false;
    bool fallbackDns = false;        // false = redirect fallback
    String fallbackValue;            // IP (dns) or URL (redirect)
    uint32_t fallbackAfterMs = 0; // ditto: the parser sets it, not a constant
    // Every `fallback` line in file order; the legacy fields above describe the
    // first one so the console status keeps its old shape.
    MitmFallbackStep fallbacks[MITM_FALLBACK_MAX];
    int fallbackCount = 0;
    // Reconnaissance lines: read-only, logged in the run feed before any rule
    // fires. `probes` holds "all" or an IPv4 address per `probe` line.
    bool wantInfo = false;           // `info` was in the payload
    String probes[MITM_PROBE_MAX];   // `probe` targets, in file order
    int probeCount = 0;
    // Many-target sweep (`sweep [ms]`): pair one host at a time, round-robin,
    // skip a host once served, and never hold more than one ARP pair.
    bool sweep = false;
    uint32_t sweepDwellMs = 1200;
    // Hosts never paired (`skip <ip>`), e.g. the operator's own machine.
    String skips[MITM_SKIP_MAX];
    int skipCount = 0;
    // Transparent forwarding (`forward off|auto|on`, default off). With the
    // bridge up a redirected client keeps its internet path, so a redirect to a
    // real page or video can actually load. `offlinePage` is what is served
    // instead when the bridge cannot start (`offline page <file>`).
    int forward = 0;
    String offlinePage;
    // Unconnected (ucmitm) options, ignored by the connected run.
    bool uc = false;                 // the file was a .ucmitm payload
    bool ucDeauth = true;            // `deauth on|off`
    uint32_t ucSniffMs = 0;          // `sniff <secs>`: passive listen before the twin
    String ucTwinSsid;               // `twin [ssid]`: override the twin's SSID
    String target;                   // "all", an IPv4 address, or empty
    String path;
    String error;
};

/**
 * @brief Load a unified payload into the engine.
 *
 * Clears every DNS/HTTP rule first, then arms what the file describes, so one
 * payload is one attack (loading a second payload replaces the first). A path
 * ending in `.html` is treated as `page * <path>`.
 */
MitmScriptResult mitmPayloadLoadScript(const String &path);

/**
 * @brief Write the example payload for @p path.
 *
 * @param overwrite false keeps an existing file (the menu never clobbers rules)
 */
bool mitmPayloadWriteExample(const String &path, bool overwrite = false, String *error = nullptr);

/// Write dns.txt, redirect.txt and one HTML page, creating /BruceSpoof first.
int mitmPayloadWriteAllExamples(bool overwrite = false);

/// Create /BruceSpoof and /BruceSpoof/html when they are missing.
bool mitmEnsurePayloadDirs();

// ---------------------------------------------------------------------------
// Menus
// ---------------------------------------------------------------------------

/// WiFi > Wifi Atks > MITM Attack: network, targets, one payload, one Start.
void mitmAttackMenu();

// ---------------------------------------------------------------------------
// Headless access (the `mitm` console verb, and how the page is tested)
// ---------------------------------------------------------------------------

/// Set the target mode: "all" or one IPv4 address. False when unparsable.
bool mitmAttackSetTarget(const String &spec);

/// Payload path currently selected on the page (empty when none).
String mitmAttackPayloadPath();

/// Select the payload the page (and a later `mitm run` with no path) uses.
void mitmAttackSetPayload(const String &path);

/// "All devices", the single address, or "Single (none picked)".
String mitmAttackTargetLabel();

/**
 * @brief Arm @p path, run it for @p ms, stop and restore ARP.
 *
 * The same code path Start uses on the page, without the screen or the key
 * watch. @p summary gets the counters of the run, @p error the reason it could
 * not start.
 */
// `draw` paints the same run screen the menu does, so the screen path can be
// exercised (and regression-tested) from the console.
bool mitmAttackRunHeadless(
    const String &path, uint32_t ms, String &summary, String &error, bool draw = false
);

/**
 * @brief True when the station is associated, i.e. when the engine can run.
 *
 * Deliberately a check and nothing more: these pages open while the station is
 * down (so they can be inspected and their payloads edited), and the rows that
 * need the link report it themselves.
 */
bool mitmEnsureStation();

#endif // !LITE_VERSION

#endif // __MITM_MENU_H__
