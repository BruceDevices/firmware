#include "nrf_commands.h"
#include "modules/rf/rf_compat.h"
#include "helpers.h"
#include "modules/NRF24/nrf_capture.h"
#include "modules/NRF24/nrf_common.h"
#include <globals.h>

// Console driver for the NRF24 raw capture / replay / analysis engine so it can
// be exercised over the Developer Options console without the on-device menu.
//
//   nrfcap info
//   nrfcap init [channel]
//   nrfcap promisc <0|1>
//   nrfcap channel <n>
//   nrfcap rate <0|1|2>
//   nrfcap power <0..3>
//   nrfcap freqrange <startMhz> <endMhz>
//   nrfcap scan [dwellMs]
//   nrfcap capture <ms> [hop]
//   nrfcap hunt [msPerCombo]   - bring own remote, press it during the sweep
//   nrfcap multifreq [ms]
//   nrfcap list
//   nrfcap replay <hex> [channel] [repeat]
//   nrfcap save <name>
//   nrfcap load <name>
//   nrfcap analyze <hex>

static String nrfHex(const uint8_t *data, uint8_t len) {
    String hex;
    for (int i = 0; i < len; i++) {
        char h[3];
        snprintf(h, sizeof(h), "%02X", data[i]);
        hex += h;
    }
    return hex;
}

static int nrfParseHexLocal(const char *s, uint8_t *out, int maxLen) {
    int n = 0, hi = -1;
    for (const char *p = s; *p && n < maxLen; p++) {
        char c = *p;
        if (c == '0' && (p[1] == 'x' || p[1] == 'X')) {
            p++;
            continue;
        }
        int v = -1;
        if (c >= '0' && c <= '9') v = c - '0';
        else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') v = c - 'A' + 10;
        else continue;
        if (hi < 0) hi = v;
        else {
            out[n++] = (uint8_t)((hi << 4) | v);
            hi = -1;
        }
    }
    return n;
}

static bool nrfEnsure(uint8_t channel) {
    if (nrfCaptureReady()) return true;
    NrfCaptureConfig cfg;
    cfg.startChannel = channel;
    cfg.endChannel = 125;
    return nrfCaptureInit(cfg);
}

// List payloads that appear more than once in the current capture. A remote
// repeats the same frame on every press; ambient 2.4 GHz noise does not.
static int nrfReportRepeats(int minCount) {
    if (minCount < 2) minCount = 2;
    int total = nrfCaptureCount();
    static bool done[NRF_CAP_MAX_PACKETS];
    memset(done, 0, sizeof(done));
    int groups = 0;
    for (int i = 0; i < total; i++) {
        if (done[i]) continue;
        NrfCapturedPacket pi;
        if (!nrfCapturePacket(i, &pi)) continue;
        int count = 0;
        for (int j = i; j < total; j++) {
            if (done[j]) continue;
            NrfCapturedPacket pj;
            if (!nrfCapturePacket(j, &pj)) continue;
            if (pj.len != pi.len) continue;
            bool same = true;
            for (int b = 0; b < pi.len && same; b++)
                if (pj.data[b] != pi.data[b]) same = false;
            if (!same) continue;
            count++;
            done[j] = true;
        }
        if (count >= minCount) {
            groups++;
            serialDevice->println(
                String("repeat x") + String(count) + " len=" + String(pi.len) + " ch=" +
                String(pi.channel) + " freq=" + String(pi.freqMhz) + " data=" + nrfHex(pi.data, pi.len)
            );
        }
    }
    serialDevice->println(String("RESULT repeat groups=") + String(groups) + " of " + String(total));
    return groups;
}

uint32_t nrfCapCallback(cmd *c) {
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

    if (mode == "info") {
        if (!nrfEnsure(76)) {
            serialDevice->println("ERR NRF24 not connected");
            return false;
        }
        serialDevice->println(
            String("chip=") + (NRFradio.isChipConnected() ? "ok" : "absent") + " channel=" +
            String(nrfCaptureGetChannel()) + " freq=" + String(2400 + nrfCaptureGetChannel()) + " rate=" +
            String(nrfCaptureGetDataRate()) + " power=" + String(3) + " promisc=" +
            (nrfCaptureIsPromiscuous() ? "1" : "0") + " profile=" + String(nrfCaptureGetProfile()) +
            " range=" + String(nrfCaptureStartFreq()) + "-" +
            String(nrfCaptureEndFreq()) + " packets=" + String(nrfCaptureCount()) + " folder=" +
            nrfCaptureFolder()
        );
        return true;
    }
    if (!nrfEnsure(76)) {
        serialDevice->println("ERR NRF24 not connected");
        return false;
    }

    if (mode == "init") {
        uint8_t ch = a.length() ? (uint8_t)a.toInt() : 76;
        NrfCaptureConfig cfg;
        cfg.startChannel = ch;
        cfg.endChannel = 125;
        serialDevice->println(String("init=") + (nrfCaptureInit(cfg) ? "ok" : "fail"));
        return true;
    }
    if (mode == "promisc") {
        bool on = a == "1" || a.equalsIgnoreCase("on") || a.equalsIgnoreCase("true");
        serialDevice->println(String("promisc=") + (nrfCaptureSetPromiscuous(on) ? "ok" : "fail"));
        return true;
    }
    if (mode == "channel") {
        nrfCaptureSetChannel((uint8_t)a.toInt());
        serialDevice->println(String("channel=") + String(nrfCaptureGetChannel()));
        return true;
    }
    if (mode == "rate") {
        nrfCaptureSetDataRate((uint8_t)a.toInt());
        serialDevice->println(String("rate=") + String(nrfCaptureGetDataRate()));
        return true;
    }
    if (mode == "power") {
        nrfCaptureSetPower((uint8_t)a.toInt());
        serialDevice->println(String("power=") + a);
        return true;
    }
    if (mode == "freqrange") {
        nrfCaptureSetFrequencyRange((uint16_t)a.toInt(), (uint16_t)b.toInt());
        serialDevice->println(
            String("range=") + String(nrfCaptureStartFreq()) + "-" + String(nrfCaptureEndFreq())
        );
        return true;
    }
    if (mode == "scan") {
        uint32_t dwell = a.length() ? a.toInt() : 2;
        static NrfChannelActivity act[126];
        int n = nrfScanChannelActivity(0, 125, dwell, act, 126);
        int busy = 0;
        for (int i = 0; i < n; i++) {
            if (act[i].level == 0) continue;
            busy++;
            serialDevice->println(String("ch=") + String(act[i].channel) + " freq=" + String(act[i].freqMhz) + " level=" + String(act[i].level));
        }
        serialDevice->println(String("busy=") + String(busy));
        return true;
    }
    if (mode == "capture") {
        // No window typed: the task 1 tunable `nrf.cap_ms` (default 3000).
        uint32_t ms = a.length() ? a.toInt() : (uint32_t)tuneGetByName("nrf.cap_ms", nullptr);
        bool hop = b == "1" || b.equalsIgnoreCase("hop");
        int n = nrfCaptureRun(ms, hop);
        // Vandeta let `nrf.capture_variant` pick the layout through its dev
        // console; that console is not part of this port, so the default variant
        // (single) is what runs and what gets reported.
        const char *variant = "single";
        serialDevice->println(
            String("RESULT capture count=") + String(n) + " dropped=" + String(nrfCaptureDropped()) + " hop=" +
            (hop ? "1" : "0") + " variant=" + (variant ? variant : "?")
        );
        for (int i = 0; i < n && i < 16; i++) {
            NrfCapturedPacket p;
            if (!nrfCapturePacket(i, &p)) continue;
            serialDevice->println(
                String("pkt i=") + String(i) + " ch=" + String(p.channel) + " freq=" + String(p.freqMhz) +
                " len=" + String(p.len) + " ts=" + String(p.timestamp) + " data=" + nrfHex(p.data, p.len)
            );
        }
        return true;
    }
    if (mode == "hunt") {
        // nrfcap hunt [msPerCombo] - sweep rate x address width x payload length
        // on the busiest channels and report the parameters of the first frame
        // that repeats. This is the console twin of the menu's Hunt row, so a
        // headless session can identify a remote without looking at the screen.
        uint32_t ms = a.length() ? a.toInt() : 1200;
        serialDevice->println("hunt start combos=48");
        // A full sweep runs for about a minute. Printing one line every 8 combos
        // keeps the console link alive (a silent command looks finished to the
        // host runner) without turning the transcript into 48 lines of noise.
        NrfHuntResult r = nrfCaptureHunt(ms, [](const NrfHuntProgress &p) {
            if (p.step == 0 || (p.step % 8) == 0) {
                const char *rn = p.rate == NRF_RATE_2MBPS ? "2M" : (p.rate == NRF_RATE_250KBPS ? "250k" : "1M");
                serialDevice->println(
                    String("hunt step=") + String(p.step + 1) + "/" + String(p.steps) + " rate=" + rn +
                    " addr=" + String(p.addressWidth) + " len=" + String(p.payloadLen)
                );
            }
            return true;
        });
        if (!r.found) {
            serialDevice->println(
                String("RESULT hunt found=0 aborted=") + (r.aborted ? "1" : "0") + " packets=" +
                String(nrfCaptureCount())
            );
            return true;
        }
        const char *rateName = r.rate == NRF_RATE_2MBPS ? "2M" : (r.rate == NRF_RATE_250KBPS ? "250k" : "1M");
        serialDevice->println(
            String("RESULT hunt found=1 rate=") + rateName + " rateIndex=" + String(r.rate) +
            " addrWidth=" + String(r.addressWidth) + " payloadLen=" + String(r.payloadLen) +
            " channel=" + String(r.channel) + " freq=" + String(2400 + r.channel) +
            " repeats=" + String(r.repeatCount) + " packets=" + String(r.packetCount)
        );
        serialDevice->println(String("payload=") + nrfHex(r.payload, r.payloadLen));
        return true;
    }
    if (mode == "multifreq") {
        uint32_t ms = a.length() ? a.toInt() : 4000;
        nrfCaptureRun(ms, true);
        uint8_t chans[8];
        int n = nrfFilterMultiFreqChannels(chans, 8);
        for (int i = 0; i < n; i++)
            serialDevice->println(String("signal ch=") + String(chans[i]) + " freq=" + String(2400 + chans[i]));
        serialDevice->println(
            String("RESULT multifreq signals=") + String(n) + " dualFreq=" + (n >= 2 ? "1" : "0")
        );
        return true;
    }
    if (mode == "capch" || mode == "channels") {
        // nrfcap capch <c1,c2,c3,...> [ms] [dwellMs]
        uint8_t chans[24];
        int nc = 0;
        const char *s = a.c_str();
        while (*s && nc < 24) {
            while (*s == ',' || *s == ' ' || *s == ';') s++;
            if (!*s) break;
            int v = atoi(s);
            if (v < 0) v = 0;
            if (v > 125) v = 125;
            chans[nc++] = (uint8_t)v;
            while (*s && *s != ',' && *s != ' ' && *s != ';') s++;
        }
        if (nc == 0) {
            serialDevice->println("ERR no channels");
            return false;
        }
        uint32_t ms = b.length() ? b.toInt() : 4000;
        uint32_t dwell = d.length() ? d.toInt() : 2;
        int n = nrfCaptureRunChannels(chans, nc, ms, dwell);
        serialDevice->println(
            String("RESULT capture count=") + String(n) + " dropped=" + String(nrfCaptureDropped()) +
            " channels=" + String(nc)
        );
        for (int i = 0; i < n && i < 16; i++) {
            NrfCapturedPacket p;
            if (!nrfCapturePacket(i, &p)) continue;
            serialDevice->println(
                String("pkt i=") + String(i) + " ch=" + String(p.channel) + " freq=" + String(p.freqMhz) +
                " len=" + String(p.len) + " data=" + nrfHex(p.data, p.len)
            );
        }
        return true;
    }
    if (mode == "lock") {
        // nrfcap lock [topN] [scanDwellMs] [captureMs]
        int topN = a.length() ? a.toInt() : 3;
        if (topN < 1) topN = 1;
        if (topN > 8) topN = 8;
        uint32_t scanDwell = b.length() ? b.toInt() : 12;
        uint32_t ms = d.length() ? d.toInt() : 6000;
        uint8_t chans[8];
        int nc = nrfFindStrongestChannels(chans, topN, scanDwell);
        for (int i = 0; i < nc; i++)
            serialDevice->println(
                String("lock ch=") + String(chans[i]) + " freq=" + String(2400 + chans[i])
            );
        if (nc == 0) {
            serialDevice->println("RESULT lock channels=0");
            return true;
        }
        int n = nrfCaptureRunChannels(chans, nc, ms, 2);
        serialDevice->println(
            String("RESULT capture count=") + String(n) + " channels=" + String(nc)
        );
        for (int i = 0; i < n && i < 16; i++) {
            NrfCapturedPacket p;
            if (!nrfCapturePacket(i, &p)) continue;
            serialDevice->println(
                String("pkt i=") + String(i) + " ch=" + String(p.channel) + " freq=" + String(p.freqMhz) +
                " len=" + String(p.len) + " data=" + nrfHex(p.data, p.len)
            );
        }
        return true;
    }
    if (mode == "repeat") {
        // nrfcap repeat [minCount]  - list payloads seen more than once.
        nrfReportRepeats(a.length() ? a.toInt() : 2);
        return true;
    }
    if (mode == "repeatcap") {
        // nrfcap repeatcap <c1,c2,...> [ms] [dwellMs] - capture then list repeats.
        uint8_t chans[24];
        int nc = 0;
        const char *s = a.c_str();
        while (*s && nc < 24) {
            while (*s == ',' || *s == ' ' || *s == ';') s++;
            if (!*s) break;
            int v = atoi(s);
            if (v < 0) v = 0;
            if (v > 125) v = 125;
            chans[nc++] = (uint8_t)v;
            while (*s && *s != ',' && *s != ' ' && *s != ';') s++;
        }
        if (nc == 0) {
            serialDevice->println("ERR no channels");
            return false;
        }
        uint32_t ms = b.length() ? b.toInt() : 15000;
        uint32_t dwell = d.length() ? d.toInt() : 1;
        int n = nrfCaptureRunChannels(chans, nc, ms, dwell);
        serialDevice->println(
            String("RESULT repeatcap count=") + String(n) + " dropped=" + String(nrfCaptureDropped()) +
            " channels=" + String(nc)
        );
        int groups = nrfReportRepeats(2);
        // Replay every repeated frame (this is the capture-and-replay path).
        if (groups > 0) {
            bool done[NRF_CAP_MAX_PACKETS] = {false};
            for (int i = 0; i < nrfCaptureCount(); i++) {
                NrfCapturedPacket pi;
                if (!nrfCapturePacket(i, &pi) || done[i]) continue;
                int count = 0;
                for (int j = i; j < nrfCaptureCount(); j++) {
                    NrfCapturedPacket pj;
                    if (!nrfCapturePacket(j, &pj) || done[j] || pj.len != pi.len) continue;
                    bool same = true;
                    for (int b = 0; b < pi.len && same; b++)
                        if (pj.data[b] != pi.data[b]) same = false;
                    if (same) {
                        count++;
                        done[j] = true;
                    }
                }
                if (count >= 2)
                    nrfReplayRaw(pi.data, pi.len, pi.channel, 2, nrfCaptureGetRate());
            }
            serialDevice->println("replayed repeats on their channels");
        }
        return true;
    }
    if (mode == "rgb") {
        // nrfcap rgb <profileIdx|99> [ms]
        // Applies the profile (99 = generic noise RX) and captures on its three
        // channels in one command, so a host reset between commands cannot lose
        // the selected profile.
        int idx = a.length() ? a.toInt() : -1;
        if (idx >= NRF_PROFILE_COUNT) idx = NRF_PROFILE_GENERIC;
        uint32_t ms = b.length() ? b.toInt() : 8000;
        uint8_t chans[3];
        int nc = 0;
        const char *pname = "generic";
        if (idx >= 0) {
            nrfCaptureApplyProfile(idx, nrfCaptureGetRate());
            for (int i = 0; i < 3; i++) chans[nc++] = NRF_SIGNAL_PROFILES[idx].channels[i];
            pname = NRF_SIGNAL_PROFILES[idx].name;
        } else {
            nrfCaptureSetPromiscuous(true);
            chans[nc++] = nrfCaptureGetChannel();
        }
        int n = nrfCaptureRunChannels(chans, nc, ms, 2);
        serialDevice->println(
            String("RESULT rgb profile=") + pname + " count=" + String(n) + " channels=" + String(nc)
        );
        for (int i = 0; i < n && i < 20; i++) {
            NrfCapturedPacket p;
            if (!nrfCapturePacket(i, &p)) continue;
            serialDevice->println(
                String("pkt i=") + String(i) + " ch=" + String(p.channel) + " freq=" + String(p.freqMhz) +
                " len=" + String(p.len) + " data=" + nrfHex(p.data, p.len)
            );
        }
        return true;
    }
    if (mode == "profile") {
        // nrfcap profile <index|-1> [rate]   (-1 = generic noise-address RX)
        int idx = a.length() ? a.toInt() : -1;
        if (idx >= NRF_PROFILE_COUNT) idx = -1;
        uint8_t rate = b.length() ? (uint8_t)b.toInt() : nrfCaptureGetRate();
        bool ok = nrfCaptureApplyProfile(idx, rate);
        serialDevice->println(
            String("RESULT profile ") + (idx >= 0 ? NRF_SIGNAL_PROFILES[idx].name : "generic") + " ok=" +
            (ok ? "1" : "0")
        );
        return true;
    }
    if (mode == "profiles") {
        for (int i = 0; i < NRF_PROFILE_COUNT; i++) {
            const NrfSignalProfile &p = NRF_SIGNAL_PROFILES[i];
            serialDevice->println(
                String("profile i=") + String(i) + " name=" + p.name + " ch=" + String(p.channels[0]) +
                "," + String(p.channels[1]) + "," + String(p.channels[2]) + " len=" + String(p.payloadLen) +
                " about=" + p.about
            );
        }
        serialDevice->println(String("current=") + String(nrfCaptureGetProfile()));
        return true;
    }
    if (mode == "list") {
        for (int i = 0; i < nrfCaptureCount(); i++) {
            NrfCapturedPacket p;
            if (!nrfCapturePacket(i, &p)) continue;
            serialDevice->println(
                String("pkt i=") + String(i) + " ch=" + String(p.channel) + " freq=" + String(p.freqMhz) +
                " len=" + String(p.len) + " data=" + nrfHex(p.data, p.len)
            );
        }
        serialDevice->println(String("count=") + String(nrfCaptureCount()));
        return true;
    }
    if (mode == "replay") {
        uint8_t bytes[NRF_CAP_MAX_PAYLOAD];
        int len = nrfParseHexLocal(a.c_str(), bytes, NRF_CAP_MAX_PAYLOAD);
        if (len <= 0) {
            serialDevice->println("ERR bad hex payload");
            return false;
        }
        int channel = b.length() ? b.toInt() : nrfCaptureGetChannel();
        // No count typed: the task 1 tunable `nrf.repeat` (default 3).
        int repeat = d.length() ? d.toInt() : (int)tuneGetByName("nrf.repeat", nullptr);
        bool ok = nrfReplayRaw(bytes, (uint8_t)len, (uint8_t)channel, (uint8_t)repeat, nrfCaptureGetDataRate());
        serialDevice->println(String("RESULT replay ok=") + (ok ? "1" : "0") + " ch=" + String(channel) + " bytes=" + String(len));
        return true;
    }
    if (mode == "save") {
        if (nrfCaptureCount() == 0) {
            serialDevice->println("ERR nothing captured");
            return false;
        }
        // Heap (PSRAM first), never a static 5 KB array: internal DRAM is what
        // Wi-Fi/BLE need and every copy here used to cost the radios.
        NrfCapturedPacket *pkts = nrfAllocPacketBuffer();
        if (pkts == nullptr) {
            serialDevice->println("ERR no memory for capture buffer");
            return false;
        }
        int got = 0;
        for (int i = 0; i < nrfCaptureCount() && got < NRF_CAP_MAX_PACKETS; i++)
            if (nrfCapturePacket(i, &pkts[got])) got++;
        bool ok = nrfSaveSignal(a, pkts, got, "nrfcap");
        free(pkts);
        serialDevice->println(String("RESULT save ok=") + (ok ? "1" : "0") + " packets=" + String(got) + " name=" + a);
        return true;
    }
    if (mode == "load") {
        NrfCapturedPacket *pkts = nrfAllocPacketBuffer();
        if (pkts == nullptr) {
            serialDevice->println("ERR no memory for capture buffer");
            return false;
        }
        int n = nrfLoadSignal(a, pkts, NRF_CAP_MAX_PACKETS, nullptr);
        int kept = nrfCaptureLoad(pkts, n);
        uint8_t chans[4];
        int nc = nrfChooseReplayChannels(pkts, kept, chans, 4);
        free(pkts);
        serialDevice->println(String("RESULT load packets=") + String(kept) + " channels=" + String(nc));
        for (int i = 0; i < nc; i++) serialDevice->println(String("  ch=") + String(chans[i]) + " freq=" + String(2400 + chans[i]));
        return true;
    }
    if (mode == "analyze") {
        uint8_t bytes[NRF_CAP_MAX_PAYLOAD];
        int len = nrfParseHexLocal(a.c_str(), bytes, NRF_CAP_MAX_PAYLOAD);
        NrfSignalAnalysis an = nrfAnalyzeSignal(bytes, (uint8_t)len);
        serialDevice->println(
            String("RESULT analyze len=") + String(an.len) + " unique=" + String(an.uniqueBytes) +
            " transitions=" + String(an.bitTransitions) + " min=" + String(an.minByte) + " max=" +
            String(an.maxByte) + " repeats=" + (an.repeats ? "1" : "0") + " codeLen=" +
            String(an.repeatLength) + " kind=" + an.kind
        );
        return true;
    }
    serialDevice->println(
        "modes: info init promisc channel rate power freqrange scan capture capch lock hunt rgb multifreq "
        "profile profiles repeat list replay save load analyze"
    );
    return false;
}

void createNrfCommands(SimpleCLI *cli) {
    Command cmd = cli->addCommand("nrfcap,nrf", nrfCapCallback);
    cmd.addPosArg("mode");
    cmd.addPosArg("a", "");
    cmd.addPosArg("b", "");
    cmd.addPosArg("c", "");
}
