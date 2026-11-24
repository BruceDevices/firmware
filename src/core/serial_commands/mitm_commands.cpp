#include "mitm_commands.h"
#include "core/sd_functions.h"
#include "modules/wifi/mitm_menu.h"
#include "modules/wifi/mitm_uc.h"
#include "modules/wifi/net_spoof.h"
#include <WiFi.h>
#include <globals.h>

// Console driver for the DNS Inject / HTML Intercept menus so the same engine
// can be exercised (and regression-tested) over the Developer Options console
// without touching the keypad:
//
//   dnsinject status
//   dnsinject payload [path]        load /BruceSpoof/dns.txt (or another file)
//   dnsinject rule <domain|*> <ip>  add one rule
//   dnsinject wildcard [ip]         answer every query (default: this board)
//   dnsinject rules                 list the loaded rules
//   dnsinject clear
//   dnsinject example [path]        write the template when it is missing
//   dnsinject scan [ms]             ARP scan, one line per host
//   dnsinject arp <targetIp> [gatewayIp]
//   dnsinject arpstop
//   dnsinject start | stop
//   dnsinject captures
//
//   htmlint status
//   htmlint match <pattern|*>       what the next payload load matches
//   htmlint payload <path> [match]  load one HTML file as the injection
//   htmlint redirect [path]         load /BruceSpoof/redirect.txt (or another)
//   htmlint clear
//   htmlint example
//   htmlint scan [ms] | arp ... | arpstop
//   htmlint start | stop
//   htmlint captures

#ifndef LITE_VERSION

static String mitmDnsPath = MITM_DNS_PAYLOAD;
static String mitmHtmlPath = "";
static String mitmHtmlMatch = "*";
static String mitmRedirectPath = MITM_REDIRECT_PAYLOAD;

static void mitmPrintPayload(const char *verb, const MitmPayloadResult &r) {
    serialDevice->println(
        String(verb) + " payload path=" + r.path + " ok=" + (r.ok ? "1" : "0") + " rules=" + String(r.loaded) +
        " bad=" + String(r.bad) + " skipped=" + String(r.skipped) + " truncated=" + (r.truncated ? "1" : "0") +
        " error=" + (r.error.length() ? r.error : "-")
    );
}

static void mitmPrintStatus(const char *verb) {
    serialDevice->println(
        String(verb) + " active=" + (netspoof_dns_active() ? "1" : "0") + "/" +
        (netspoof_http_active() ? "1" : "0") + "/" + (netspoof_https_active() ? "1" : "0") +
        " dnsRules=" + String(netspoof_dns_rule_count()) + " injectRules=" +
        String(netspoof_http_inject_count()) + " redirectRules=" + String(netspoof_http_redirect_count()) +
        " queries=" + String(netspoof_dns_queries()) + " answers=" + String(netspoof_dns_answers()) +
        " requests=" + String(netspoof_http_requests()) + " redirected=" + String(netspoof_http_redirected()) +
        " injected=" + String(netspoof_http_injected()) + " https=" + String(netspoof_https_requests()) +
        " quicDropped=" + String(netspoof_quic_dropped()) + " dotDropped=" + String(netspoof_dot_dropped()) +
        " svcbSuppressed=" + String(netspoof_svcb_suppressed()) + " ipv6Dropped=" + String(netspoof_ipv6_dropped()) +
        " httpsRedirected=" + String(netspoof_https_redirected()) + " httpsInjected=" +
        String(netspoof_https_injected()) + " pairs=" + String(netspoof_arp_pair_count()) +
        " frames=" + String(netspoof_arp_frames()) + " hook=" + (netspoof_hook_installed() ? "1" : "0") +
        " station=" + (WiFi.isConnected() ? WiFi.localIP().toString() : String("down"))
    );
}

/// Shared `scan`, `arp` and `arpstop` handling for both verbs.
/// @return true when the mode was handled
static bool mitmHandleNetworkMode(const String &verb, const String &mode, const String &a, const String &b) {
    if (mode == "scan" || mode == "targets") {
        String err;
        uint32_t ms = a.toInt();
        if (ms < 1000 || ms > 30000) ms = 8000;
        int found = netspoof_scan_targets(ms, err);
        if (found < 0) {
            serialDevice->println(String(verb) + " scan ok=0 error=" + (err.length() ? err : "scan failed"));
            return true;
        }
        for (int i = 0; i < netspoof_target_count(); i++) {
            const NetSpoofTarget *t = netspoof_target(i);
            if (!t) continue;
            serialDevice->println(
                String("target ip=") + t->ip.toString() + " mac=" + t->macStr + " gw=" + (t->gateway ? "1" : "0") +
                " arp=" + (netspoof_arp_pair_index(t->ip, nullptr) ? "1" : "0")
            );
        }
        serialDevice->println(String(verb) + " scan ok=1 count=" + String(netspoof_target_count()));
        return true;
    }

    if (mode == "arp") {
        IPAddress target, gateway((uint32_t)0);
        if (!target.fromString(a.c_str())) {
            serialDevice->println(String(verb) + " arp ok=0 error=bad IPv4 target (got '" + a + "')");
            return true;
        }
        if (b.length()) {
            if (!gateway.fromString(b.c_str())) {
                serialDevice->println(String(verb) + " arp ok=0 error=bad IPv4 gateway (got '" + b + "')");
                return true;
            }
        }
        String err;
        if (netspoof_arp_start(target, gateway, err)) {
            serialDevice->println(
                String(verb) + " arp ok=1 target=" + target.toString() + " pairs=" + String(netspoof_arp_pair_count())
            );
        } else {
            serialDevice->println(String(verb) + " arp ok=0 error=" + (err.length() ? err : "ARP start failed"));
        }
        return true;
    }

    if (mode == "arpstop" || mode == "restore") {
        String err;
        bool ok = netspoof_arp_stop(err);
        serialDevice->println(
            String(verb) + " arpstop ok=" + (ok ? "1" : "0") + " pairs=" + String(netspoof_arp_pair_count()) +
            " error=" + (err.length() ? err : "-")
        );
        return true;
    }

    return false;
}

uint32_t dnsInjectCallback(cmd *c) {
    Command cmd(c);
    String mode = cmd.getArgument(0).getValue();
    mode.trim();
    mode.toLowerCase();
    String a = cmd.getArgument(1).getValue();
    String b = cmd.getArgument(2).getValue();
    a.trim();
    b.trim();

    if (mode.length() == 0 || mode == "status") {
        mitmPrintStatus("dnsinject");
        return true;
    }

    if (mode == "payload" || mode == "load") {
        if (a.length()) mitmDnsPath = a;
        MitmPayloadResult r = mitmPayloadLoadDns(mitmDnsPath);
        mitmPrintPayload("dnsinject", r);
        return true;
    }

    if (mode == "rule") {
        if (a.length() == 0 || b.length() == 0) {
            serialDevice->println("usage: dnsinject rule <domain|*> <ip>");
            return false;
        }
        bool wildcard = (a == "*");
        String err;
        bool ok = netspoof_dns_add_rule(a, b, wildcard, err);
        serialDevice->println(
            String("dnsinject rule ok=") + (ok ? "1" : "0") + " domain=" + a + " ip=" + b +
            " rules=" + String(netspoof_dns_rule_count()) + " error=" + (err.length() ? err : "-")
        );
        return true;
    }

    if (mode == "wildcard" || mode == "all") {
        String ip = a;
        if (ip.length() == 0) {
            if (!WiFi.isConnected()) {
                serialDevice->println("dnsinject wildcard ok=0 error=station not connected and no ip given");
                return true;
            }
            ip = WiFi.localIP().toString();
        }
        IPAddress parsed;
        if (!parsed.fromString(ip.c_str())) {
            serialDevice->println("dnsinject wildcard ok=0 error=bad IPv4 (got '" + ip + "')");
            return true;
        }
        String err;
        bool ok = netspoof_dns_add_rule("*", ip, true, err);
        serialDevice->println(
            String("dnsinject wildcard ok=") + (ok ? "1" : "0") + " ip=" + ip +
            " rules=" + String(netspoof_dns_rule_count()) + " error=" + (err.length() ? err : "-")
        );
        return true;
    }

    if (mode == "rules" || mode == "list") {
        int n = netspoof_dns_rule_count();
        for (int i = 0; i < n; i++) {
            const NetSpoofRule *r = netspoof_dns_rule(i);
            if (!r) continue;
            serialDevice->println(
                String("dnsrule match=") + r->match + " -> " + r->value + " wildcard=" + (r->wildcard ? "1" : "0") +
                " hits=" + String(r->hits)
            );
        }
        serialDevice->println(String("dnsinject rules count=") + String(n));
        return true;
    }

    if (mode == "clear") {
        netspoof_dns_clear_rules();
        serialDevice->println(String("dnsinject clear ok=1 rules=") + String(netspoof_dns_rule_count()));
        return true;
    }

    if (mode == "example") {
        String path = a.length() ? a : mitmDnsPath;
        String err;
        bool ok = mitmPayloadWriteExample(path, false, &err);
        serialDevice->println(
            String("dnsinject example ok=") + (ok ? "1" : "0") + " path=" + path +
            " error=" + (err.length() ? err : "-")
        );
        return true;
    }

    if (mitmHandleNetworkMode("dnsinject", mode, a, b)) return true;

    if (mode == "start") {
        String err;
        bool ok = netspoof_dns_start(err);
        serialDevice->println(
            String("dnsinject start ok=") + (ok ? "1" : "0") + " active=" + (netspoof_dns_active() ? "1" : "0") +
            " rules=" + String(netspoof_dns_rule_count()) + " pairs=" + String(netspoof_arp_pair_count()) +
            " error=" + (err.length() ? err : "-")
        );
        return true;
    }

    if (mode == "stop") {
        String err;
        netspoof_dns_stop(err);
        serialDevice->println(
            String("dnsinject stop ok=1 active=") + (netspoof_dns_active() ? "1" : "0") +
            " queries=" + String(netspoof_dns_queries()) + " answers=" + String(netspoof_dns_answers())
        );
        return true;
    }

    if (mode == "captures" || mode == "capture") {
        int n = netspoof_capture_count();
        int shown = 0;
        for (int i = 0; i < n; i++) {
            const NetSpoofCapture *cap = netspoof_capture(i);
            if (!cap || cap->kind != 0) continue;
            serialDevice->println(
                String("dnscap host=") + cap->host + " action=" + String(cap->action) + " client=" +
                cap->clientIp.toString() + " mac=" + cap->clientMac + " ageMs=" + String(millis() - cap->ts)
            );
            shown++;
        }
        serialDevice->println(String("dnsinject captures count=") + String(shown));
        return true;
    }

    serialDevice->println("dnsinject modes: status payload rule wildcard rules clear example scan arp arpstop start stop captures");
    return false;
}

uint32_t htmlInterceptCallback(cmd *c) {
    Command cmd(c);
    String mode = cmd.getArgument(0).getValue();
    mode.trim();
    mode.toLowerCase();
    String a = cmd.getArgument(1).getValue();
    String b = cmd.getArgument(2).getValue();
    a.trim();
    b.trim();

    if (mode.length() == 0 || mode == "status") {
        mitmPrintStatus("htmlint");
        serialDevice->println(
            String("htmlint state payload=") + (mitmHtmlPath.length() ? mitmHtmlPath : String("none")) + " match=" +
            mitmHtmlMatch + " redirect=" + mitmRedirectPath
        );
        return true;
    }

    if (mode == "match") {
        if (a.length() == 0) {
            serialDevice->println("htmlint match pattern=" + mitmHtmlMatch);
            return true;
        }
        mitmHtmlMatch = (a == "*") ? String("*") : a;
        serialDevice->println("htmlint match ok=1 pattern=" + mitmHtmlMatch);
        return true;
    }

    if (mode == "payload" || mode == "load") {
        if (a.length()) mitmHtmlPath = a;
        if (b.length()) mitmHtmlMatch = b;
        if (mitmHtmlPath.length() == 0) {
            serialDevice->println("usage: htmlint payload <path> [match]");
            return false;
        }
        MitmPayloadResult r = mitmPayloadLoadHtml(mitmHtmlPath, mitmHtmlMatch);
        mitmPrintPayload("htmlint", r);
        return true;
    }

    if (mode == "redirect" || mode == "redirects") {
        if (a.length()) mitmRedirectPath = a;
        MitmPayloadResult r = mitmPayloadLoadRedirects(mitmRedirectPath);
        mitmPrintPayload("htmlint", r);
        return true;
    }

    if (mode == "rules" || mode == "list") {
        int inj = netspoof_http_inject_count();
        for (int i = 0; i < inj; i++) {
            const NetSpoofRule *r = netspoof_http_inject_rule(i);
            if (!r) continue;
            serialDevice->println(
                String("htmlrule kind=inject match=") + (r->wildcard ? "*" : r->match.c_str()) + " bytes=" +
                String(r->value.length()) + " hits=" + String(r->hits)
            );
        }
        int redir = netspoof_http_redirect_count();
        for (int i = 0; i < redir; i++) {
            const NetSpoofRule *r = netspoof_http_redirect_rule(i);
            if (!r) continue;
            serialDevice->println(
                String("htmlrule kind=redirect match=") + (r->wildcard ? "*" : r->match.c_str()) + " url=" + r->value +
                " hits=" + String(r->hits)
            );
        }
        serialDevice->println(
            String("htmlint rules inject=") + String(inj) + " redirect=" + String(redir)
        );
        return true;
    }

    if (mode == "clear") {
        netspoof_http_clear_rules();
        serialDevice->println(String("htmlint clear ok=1 rules=") + String(netspoof_http_inject_count()));
        return true;
    }

    if (mode == "example") {
        int written = mitmPayloadWriteAllExamples(false);
        // Idempotent: ok when both payload formats are on the card afterwards,
        // whether this call wrote them or they were already there.
        FS *fs = nullptr;
        int present = 0;
        if (getFsStorage(fs) && fs) {
            if (fs->exists(MITM_EXAMPLE_PAYLOAD)) present++;
            if (fs->exists(MITM_UC_EXAMPLE_PAYLOAD)) present++;
        }
        serialDevice->println(
            String("htmlint example ok=") + (present == 2 ? "1" : "0") + " written=" + String(written) +
            " present=" + String(present) + " dir=" + MITM_PAYLOAD_DIR
        );
        return true;
    }

    if (mitmHandleNetworkMode("htmlint", mode, a, b)) return true;

    if (mode == "https") {
        String sub = a;
        sub.toLowerCase();
        if (sub == "off" || sub == "stop") {
            String err;
            netspoof_https_stop(err);
            serialDevice->println(
                String("htmlint https ok=1 active=") + (netspoof_https_active() ? "1" : "0") +
                " requests=" + String(netspoof_https_requests())
            );
            return true;
        }
        String err;
        bool ok = netspoof_https_start(err);
        serialDevice->println(
            String("htmlint https ok=") + (ok ? "1" : "0") + " active=" + (netspoof_https_active() ? "1" : "0") +
            " port=" + String(netspoof_https_port()) + " station=" +
            (WiFi.isConnected() ? WiFi.localIP().toString() : String("down")) +
            " error=" + (err.length() ? err : "-")
        );
        return true;
    }

    if (mode == "start") {
        String err;
        bool ok = netspoof_http_start(err);
        serialDevice->println(
            String("htmlint start ok=") + (ok ? "1" : "0") + " active=" + (netspoof_http_active() ? "1" : "0") +
            " https=" + (netspoof_https_active() ? "1" : "0") +
            " inject=" + String(netspoof_http_inject_count()) + " redirect=" + String(netspoof_http_redirect_count()) +
            " pairs=" + String(netspoof_arp_pair_count()) + " error=" + (err.length() ? err : "-")
        );
        return true;
    }

    if (mode == "stop") {
        String err;
        netspoof_http_stop(err);
        serialDevice->println(
            String("htmlint stop ok=1 active=") + (netspoof_http_active() ? "1" : "0") +
            " https=" + (netspoof_https_active() ? "1" : "0") +
            " requests=" + String(netspoof_http_requests()) + " redirected=" + String(netspoof_http_redirected()) +
            " injected=" + String(netspoof_http_injected()) + " httpsRequests=" +
            String(netspoof_https_requests()) + " httpsRedirected=" + String(netspoof_https_redirected()) +
            " httpsInjected=" + String(netspoof_https_injected())
        );
        return true;
    }

    if (mode == "captures" || mode == "capture") {
        int n = netspoof_capture_count();
        int shown = 0;
        for (int i = 0; i < n; i++) {
            const NetSpoofCapture *cap = netspoof_capture(i);
            if (!cap) continue;
            if (cap->kind != NETSPOOF_CAPTURE_HTTP && cap->kind != NETSPOOF_CAPTURE_HTTPS) continue;
            serialDevice->println(
                String("httpcap kind=") + netspoof_capture_kind_name(cap->kind) + " method=" + cap->method +
                " host=" + cap->host + " path=" + cap->path + " action=" + String(cap->action) +
                " client=" + cap->clientIp.toString() + " mac=" + cap->clientMac
            );
            shown++;
        }
        serialDevice->println(String("htmlint captures count=") + String(shown));
        return true;
    }

    serialDevice->println("htmlint modes: status match payload redirect rules clear example scan arp arpstop https start stop captures");
    return false;
}

/// `mitm` - the whole MITM Attack page without the keypad.
///
///   mitm status
///   mitm payload [path]     load one unified payload (dns+page+fallback)
///   mitm target all|<ip>    target mode
///   mitm run [secs] [path]  arm it, run for <secs> (default 30), stop, restore
///   mitm stop               stop engines and restore ARP now
uint32_t mitmCallback(cmd *c) {
    Command cmd(c);
    String mode = cmd.getArgument(0).getValue();
    String a = cmd.getArgument(1).getValue();
    String b = cmd.getArgument(2).getValue();
    mode.trim();
    mode.toLowerCase();
    a.trim();
    b.trim();

    if (mode.length() == 0 || mode == "status") {
        serialDevice->println(
            String("mitm status payload=") + (mitmAttackPayloadPath().length() ? mitmAttackPayloadPath() : String("none")) +
            " target=" + mitmAttackTargetLabel() + " dnsRules=" + String(netspoof_dns_rule_count()) +
            " injectRules=" + String(netspoof_http_inject_count()) +
            " redirectRules=" + String(netspoof_http_redirect_count()) +
            " quicDropped=" + String(netspoof_quic_dropped()) + " dotDropped=" + String(netspoof_dot_dropped()) +
            " svcbSuppressed=" + String(netspoof_svcb_suppressed()) + " ipv6Dropped=" + String(netspoof_ipv6_dropped()) +
            " https=" + (netspoof_https_active() ? "1" : "0") + " pairs=" +
            String(netspoof_arp_pair_count()) +
            " station=" + (WiFi.isConnected() ? WiFi.localIP().toString() : String("down"))
        );
        return true;
    }

    if (mode == "payload" || mode == "load") {
        String path = a.length() ? a : mitmAttackPayloadPath();
        if (path.length() == 0) {
            serialDevice->println("usage: mitm payload <path>");
            return false;
        }
        MitmScriptResult r = mitmPayloadLoadScript(path);
        if (r.ok) mitmAttackSetPayload(path);
        serialDevice->println(
            String("mitm payload path=") + r.path + " ok=" + (r.ok ? "1" : "0") + " pages=" + String(r.pages) +
            " redirects=" + String(r.redirects) + " dns=" + String(r.dns) + " bad=" + String(r.bad) +
            " fallback=" + (r.fallbackSet ? (r.fallbackDns ? "dns:" : "302:") + r.fallbackValue : String("none")) +
            " fallbacks=" + String(r.fallbackCount) + " info=" + String(r.wantInfo ? 1 : 0) +
            " probes=" + String(r.probeCount) + " sweep=" + String(r.sweep ? r.sweepDwellMs : 0) +
            " target=" + (r.target.length() ? r.target : String("menu")) + " error=" +
            (r.error.length() ? r.error : "-")
        );
        return r.ok;
    }

    if (mode == "uc") {
        String sub = a;
        sub.toLowerCase();
        if (sub.length() == 0) {
            serialDevice->println(
                String("mitm uc on=") + (mitmUcEnabled() ? "1" : "0") + " ssid=" + mitmUcSsid() +
                " ch=" + String(mitmUcChannel()) + " target=" + mitmUcTargetLabel() + " clients=" +
                String(mitmUcClientCount()) + " seeks=" + String(mitmUcProbeCount())
            );
            return true;
        }
        if (sub == "on" || sub == "off") {
            mitmUcSetEnabled(sub == "on");
            serialDevice->println(String("mitm uc on=") + (mitmUcEnabled() ? "1" : "0"));
            return true;
        }
        if (sub == "net" || sub == "network") {
            String err;
            bool ok = mitmUcSelectNetwork(b, err);
            serialDevice->println(
                String("mitm uc net ok=") + (ok ? "1" : "0") + " ssid=" + mitmUcSsid() + " ch=" +
                String(mitmUcChannel()) + " error=" + (ok ? String("-") : err)
            );
            return ok;
        }
        if (sub == "clients" || sub == "scan") {
            uint32_t ms = b.toInt();
            if (ms < 1000 || ms > 30000) ms = 6000;
            int n = mitmUcScanClients(ms);
            for (int i = 0; i < n; i++)
                serialDevice->println(String("ucclient idx=") + String(i) + " mac=" + mitmUcClientLabel(i));
            for (int i = 0; i < mitmUcProbeCount(); i++)
                serialDevice->println(String("ucprobe ssid=") + mitmUcProbeSsid(i));
            serialDevice->println(
                String("mitm uc clients count=") + String(n) + " seeks=" + String(mitmUcProbeCount())
            );
            return true;
        }
        if (sub == "target") {
            bool ok = true;
            if (b.length() == 0 || b.equalsIgnoreCase("all")) mitmUcTargetAll();
            else ok = mitmUcSetTargetClient(b.toInt());
            serialDevice->println(
                String("mitm uc target ok=") + (ok ? "1" : "0") + " target=" + mitmUcTargetLabel()
            );
            return ok;
        }
        serialDevice->println("mitm uc modes: on|off net <ssid> clients [ms] target all|<idx>");
        return false;
    }

    if (mode == "target") {
        if (a.length() == 0) {
            serialDevice->println(String("mitm target mode=") + mitmAttackTargetLabel());
            return true;
        }
        bool ok = mitmAttackSetTarget(a);
        serialDevice->println(
            String("mitm target ok=") + (ok ? "1" : "0") + " mode=" + mitmAttackTargetLabel()
        );
        return ok;
    }

    if (mode == "run") {
        uint32_t secs = a.toInt();
        if (secs == 0 || secs > 600) secs = 30;
        String path = b.length() ? b : mitmAttackPayloadPath();
        String extra = cmd.getArgument(3).getValue();
        extra.trim();
        extra.toLowerCase();
        if (path.length() == 0) {
            serialDevice->println("usage: mitm run [secs] <payload> [draw]");
            return false;
        }
        String summary, err;
        bool ok = mitmAttackRunHeadless(path, secs * 1000, summary, err, extra == "draw");
        serialDevice->println(
            String("mitm run ok=") + (ok ? "1" : "0") + " seconds=" + String(secs) +
            (extra == "draw" ? " draw=1" : String()) +
            (ok ? " " + summary : " error=" + (err.length() ? err : String("run failed"))) + " pairs=" +
            String(netspoof_arp_pair_count())
        );
        return ok;
    }

    if (mode == "stop") {
        String err;
        if (netspoof_forward_active()) netspoof_forward_stop();
        if (netspoof_https_active()) netspoof_https_stop(err);
        if (netspoof_http_active()) netspoof_http_stop(err);
        if (netspoof_dns_active()) netspoof_dns_stop(err);
        if (netspoof_arp_active()) netspoof_arp_stop(err);
        serialDevice->println(
            String("mitm stop ok=1 pairs=") + String(netspoof_arp_pair_count()) + " http=" +
            (netspoof_http_active() ? "1" : "0") + " https=" + (netspoof_https_active() ? "1" : "0") +
            " dns=" + (netspoof_dns_active() ? "1" : "0") + " forward=" +
            (netspoof_forward_active() ? "1" : "0")
        );
        return true;
    }

    serialDevice->println("mitm modes: status payload target uc run stop");
    return false;
}

#endif // !LITE_VERSION

void createMitmCommands(SimpleCLI *cli) {
#if !defined(LITE_VERSION)
    Command dns = cli->addCommand("dnsinject", dnsInjectCallback);
    dns.addPosArg("mode", "");
    dns.addPosArg("a", "");
    dns.addPosArg("b", "");

    Command html = cli->addCommand("htmlint", htmlInterceptCallback);
    html.addPosArg("mode", "");
    html.addPosArg("a", "");
    html.addPosArg("b", "");

    Command mitm = cli->addCommand("mitm", mitmCallback);
    mitm.addPosArg("mode", "");
    mitm.addPosArg("a", "");
    mitm.addPosArg("b", "");
#endif
}
