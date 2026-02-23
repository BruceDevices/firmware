// --- wifi_recover.cpp ---
/*
  WiFi Password Cracker for Bruce (ESP32-S3 / T-Embed)

  Speed optimizations applied (no PlatformIO changes needed):
  ┌─────────────────────────────────────┬────────────┐
  │ Optimization                        │ Gain       │
  ├─────────────────────────────────────┼────────────┤
  │ Dual-core PBKDF2 (core 0 + core 1) │ ~1.8x      │
  │ Pre-computed HMAC pads              │ ~15-20%    │
  │ 240 MHz CPU (was likely 160 MHz)    │ ~1.5x      │
  │ Buffered SD reads (8-32 KB chunks)  │ no stutter │
  └─────────────────────────────────────┴────────────┘
  Measured: ~13-14 passwords/sec on T-Embed S3
*/

// ── Thermal knob ─────────────────────────────────────────────────────────────
// How long the worker sleeps after each password attempt.
// Higher = cooler device, slightly lower speed. At 14/s each extra ms
// of yield costs ~1.4% speed but meaningfully reduces sustained heat.
//   1ms → ~1.4% overhead  (hottest)
//   2ms → ~2.7% overhead  (default, good balance)
//   5ms → ~6.5% overhead  (noticeably cooler)
//  10ms → ~12%  overhead  (cool, ~12/s)
#define CRACK_YIELD_MS 2
// ─────────────────────────────────────────────────────────────────────────────

#include "wifi_recover.h"

#include "core/display.h"
#include "core/menu_items/WifiMenu.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/settings.h"
#include "core/utils.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

/*
 * We intentionally avoid mbedtls SHA1 here.
 * ESP-IDF mbedtls routes SHA1 through the hardware SHA peripheral which
 * has a FreeRTOS mutex. With two cores both calling SHA1 ~16,000 times
 * per password candidate, that mutex becomes a serialization bottleneck
 * that partially defeats the dual-core gain.
 *
 * This software SHA1 runs entirely in CPU registers — both cores compute
 * independently with zero contention. It is also slightly faster than
 * hardware SHA1 for the small block sizes used in PBKDF2 (20-64 bytes)
 * because hardware SHA has DMA setup overhead per call.
 */

#include <string.h>
#include <string>

static const char *TAG = "wifi_crack";

/* ─────────────────────────────────────────────────────────────
   Handshake data
───────────────────────────────────────────────────────────── */
struct HandshakeData {
    bool valid;
    uint8_t ap_mac[6];
    uint8_t sta_mac[6];
    uint8_t anonce[32];
    uint8_t snonce[32];
    uint8_t eapol[256];
    uint16_t eapol_len;
    uint8_t mic[16];
    char ssid[33];
};

/* ─────────────────────────────────────────────────────────────
   Globals
───────────────────────────────────────────────────────────── */
static volatile bool g_abortRequested = false;
static inline uint64_t now_us() { return esp_timer_get_time(); }
static inline uint32_t swap32(uint32_t x) {
    return ((x >> 24) & 0xFF) | ((x >> 8) & 0xFF00) | ((x << 8) & 0xFF0000) | ((x << 24) & 0xFF000000);
}

/* ─────────────────────────────────────────────────────────────
   PCAP structures
───────────────────────────────────────────────────────────── */
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

static String extract_ssid_from_beacon(const uint8_t *frame, size_t len) {
    if (len < 36) return "";
    size_t offset = 36;
    while (offset + 1 < len) {
        uint8_t tag_num = frame[offset];
        uint8_t tag_len = frame[offset + 1];
        if (offset + 2 + tag_len > len) break;
        if (tag_num == 0x00) {
            String ssid = "";
            for (int i = 0; i < tag_len && i < 32; ++i) {
                char c = frame[offset + 2 + i];
                ssid += (c >= 32 && c <= 126) ? c : '_';
            }
            return ssid;
        }
        offset += 2 + tag_len;
    }
    return "";
}

static bool parse_pcap_handshake(FS &fs, const String &path, HandshakeData &hs) {
    memset(&hs, 0, sizeof(hs));
    hs.valid = false;

    File f = fs.open(path, FILE_READ);
    if (!f) {
        padprintln("Error: Cannot open PCAP file");
        return false;
    }

    pcap_hdr_t gh;
    if (f.read((uint8_t *)&gh, sizeof(gh)) != sizeof(gh)) {
        padprintln("Error: Bad PCAP header");
        f.close();
        return false;
    }

    bool swapped = false;
    if (gh.magic == 0xa1b2c3d4) {
        swapped = false;
    } else if (gh.magic == 0xd4c3b2a1) {
        swapped = true;
        gh.network = swap32(gh.network);
    } else {
        padprintln("Error: Invalid PCAP magic");
        f.close();
        return false;
    }

    if (gh.network != 105) padprintf("Warning: Network type %u (expected 105)\n", gh.network);

    bool have_m1 = false, have_m2 = false, have_m3 = false, have_beacon = false;
    const size_t MAX_PKT_READ = 8192;

    while (f.available() && !(have_m2 && have_m3)) {
        pcaprec_hdr_t ph;
        if (f.read((uint8_t *)&ph, sizeof(ph)) != sizeof(ph)) break;

        uint32_t incl_len = swapped ? swap32(ph.incl_len) : ph.incl_len;
        if (incl_len == 0 || incl_len > 256 * 1024) {
            if (incl_len > 0 && incl_len < 1000000) f.seek(f.position() + incl_len);
            continue;
        }

        size_t read_sz = (incl_len > MAX_PKT_READ) ? MAX_PKT_READ : incl_len;
        uint8_t *pkt = (uint8_t *)malloc(read_sz);
        if (!pkt) break;

        if (f.read(pkt, read_sz) != (int)read_sz) {
            free(pkt);
            break;
        }
        if (read_sz < incl_len) f.seek(f.position() + (incl_len - read_sz));

        size_t pos = 0;
        if (pos + 24 > incl_len) {
            free(pkt);
            continue;
        }

        uint16_t fc = (uint16_t)(pkt[pos] | (pkt[pos + 1] << 8));
        uint8_t frame_type = (fc & 0x0C) >> 2;
        uint8_t frame_sub = (fc & 0xF0) >> 4;
        bool to_ds = fc & 0x0100;
        bool from_ds = fc & 0x0200;

        uint8_t *addr1 = pkt + pos + 4;
        uint8_t *addr2 = pkt + pos + 10;
        uint8_t *addr3 = pkt + pos + 16;

        uint8_t ap_addr[6], sta_addr[6];
        if (from_ds && !to_ds) {
            memcpy(ap_addr, addr2, 6);
            memcpy(sta_addr, addr1, 6);
        } else if (!from_ds && to_ds) {
            memcpy(ap_addr, addr1, 6);
            memcpy(sta_addr, addr2, 6);
        } else {
            memcpy(ap_addr, addr3, 6);
            memcpy(sta_addr, addr2, 6);
        }

        if (frame_type == 0 && frame_sub == 8 && !have_beacon) {
            String ssid = extract_ssid_from_beacon(pkt, incl_len);
            if (ssid.length() > 0) {
                strncpy(hs.ssid, ssid.c_str(), sizeof(hs.ssid) - 1);
                memcpy(hs.ap_mac, addr2, 6);
                have_beacon = true;
            }
        }

        if (frame_type != 2) {
            free(pkt);
            continue;
        }

        size_t hdr_len = 24 + ((fc & 0x0080) ? 2 : 0);
        pos += hdr_len;

        if (pos + 8 > incl_len || pkt[pos] != 0xAA || pkt[pos + 1] != 0xAA || pkt[pos + 2] != 0x03) {
            free(pkt);
            continue;
        }

        uint16_t ethertype = (uint16_t)((pkt[pos + 6] << 8) | pkt[pos + 7]);
        pos += 8;
        if (ethertype != 0x888E || pos + 4 > incl_len) {
            free(pkt);
            continue;
        }

        uint8_t *eapol = pkt + pos;
        uint16_t eapol_len = (uint16_t)((eapol[2] << 8) | eapol[3]);
        if ((size_t)(pos + 4 + eapol_len) > incl_len) {
            free(pkt);
            continue;
        }

        uint8_t *key = eapol + 4;
        if ((size_t)(key - pkt) + 95 > read_sz) {
            free(pkt);
            continue;
        }

        uint16_t key_info = (uint16_t)((key[1] << 8) | key[2]);
        bool mic_set = key_info & 0x0100;
        bool ack = key_info & 0x0080;
        bool install = key_info & 0x0040;
        bool secure = key_info & 0x0200;

        uint8_t *nonce = key + 13;
        uint8_t *mic = key + 77;

        int msg_num = -1;
        if (ack && !mic_set && !install) msg_num = 1;
        if (!ack && mic_set && !install && !secure) msg_num = 2;
        if (ack && mic_set && install) msg_num = 3;
        if (!ack && mic_set && !install && secure) msg_num = 4;

        if (msg_num == 1) {
            memcpy(hs.anonce, nonce, 32);
            memcpy(hs.ap_mac, ap_addr, 6);
            have_m1 = true;
        }
        if (msg_num == 2) {
            memcpy(hs.snonce, nonce, 32);
            memcpy(hs.mic, mic, 16);
            memcpy(hs.sta_mac, sta_addr, 6);
            memcpy(hs.ap_mac, ap_addr, 6);
            size_t cp = (eapol_len + 4) <= sizeof(hs.eapol) ? (eapol_len + 4) : sizeof(hs.eapol);
            memcpy(hs.eapol, eapol, cp);
            hs.eapol_len = (uint16_t)cp;
            have_m2 = true;
        }
        if (msg_num == 3) {
            memcpy(hs.anonce, nonce, 32);
            memcpy(hs.ap_mac, ap_addr, 6);
            have_m3 = true;
        }

        free(pkt);
    }
    f.close();

    if (!have_m2) {
        padprintln("Error: No M2 in PCAP");
        return false;
    }
    if (!have_m1 && !have_m3) {
        padprintln("Error: Need M1 or M3");
        return false;
    }
    hs.valid = true;
    return true;
}

/* ═════════════════════════════════════════════════════════════
   SOFTWARE SHA1  (optimized for PBKDF2 on ESP32 LX7)
   ═════════════════════════════════════════════════════════════
   Key design decisions:
   1. IRAM_ATTR on hot functions — sha1_transform runs ~16,000×
      per password. Placing it in IRAM avoids flash cache misses.
   2. sha1_final uses memset+direct writes, NOT the old byte-at-a-
      time while loop (which caused up to 55 sha1_update calls just
      for zero padding per finalize — ~450,000 calls per password).
   3. hmac_sha1_20: specialized path for exactly 20-byte inputs.
      8190 of 8192 PBKDF2 HMAC calls use 20 bytes. This path
      skips sha1_update/sha1_final entirely — builds the padded
      64-byte block directly and calls sha1_transform once each
      for inner and outer. Eliminates all loop/branching overhead.
   4. bswap via memcpy+__builtin_bswap32 in sha1_transform:
      one 32-bit load + bswap instead of 4 byte loads + 3 shifts.
───────────────────────────────────────────────────────────── */
struct Sha1Ctx {
    uint32_t state[5];
    uint32_t count[2];
    uint8_t buf[64];
};

#define SHA1_ROL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define SHA1_BLK(i)                                                                                          \
    (blk[i & 15] = SHA1_ROL(blk[(i + 13) & 15] ^ blk[(i + 8) & 15] ^ blk[(i + 2) & 15] ^ blk[i & 15], 1))

#define R0(v, w, x, y, z, i)                                                                                 \
    z += ((w & (x ^ y)) ^ y) + blk[i] + 0x5A827999u + SHA1_ROL(v, 5);                                        \
    w = SHA1_ROL(w, 30)
#define R1(v, w, x, y, z, i)                                                                                 \
    z += ((w & (x ^ y)) ^ y) + SHA1_BLK(i) + 0x5A827999u + SHA1_ROL(v, 5);                                   \
    w = SHA1_ROL(w, 30)
#define R2(v, w, x, y, z, i)                                                                                 \
    z += (w ^ x ^ y) + SHA1_BLK(i) + 0x6ED9EBA1u + SHA1_ROL(v, 5);                                           \
    w = SHA1_ROL(w, 30)
#define R3(v, w, x, y, z, i)                                                                                 \
    z += (((w | x) & y) | (w & x)) + SHA1_BLK(i) + 0x8F1BBCDCu + SHA1_ROL(v, 5);                             \
    w = SHA1_ROL(w, 30)
#define R4(v, w, x, y, z, i)                                                                                 \
    z += (w ^ x ^ y) + SHA1_BLK(i) + 0xCA62C1D6u + SHA1_ROL(v, 5);                                           \
    w = SHA1_ROL(w, 30)

#define SHA1_80_ROUNDS()                                                                                     \
    R0(a, b, c, d, e, 0);                                                                                    \
    R0(e, a, b, c, d, 1);                                                                                    \
    R0(d, e, a, b, c, 2);                                                                                    \
    R0(c, d, e, a, b, 3);                                                                                    \
    R0(b, c, d, e, a, 4);                                                                                    \
    R0(a, b, c, d, e, 5);                                                                                    \
    R0(e, a, b, c, d, 6);                                                                                    \
    R0(d, e, a, b, c, 7);                                                                                    \
    R0(c, d, e, a, b, 8);                                                                                    \
    R0(b, c, d, e, a, 9);                                                                                    \
    R0(a, b, c, d, e, 10);                                                                                   \
    R0(e, a, b, c, d, 11);                                                                                   \
    R0(d, e, a, b, c, 12);                                                                                   \
    R0(c, d, e, a, b, 13);                                                                                   \
    R0(b, c, d, e, a, 14);                                                                                   \
    R0(a, b, c, d, e, 15);                                                                                   \
    R1(e, a, b, c, d, 16);                                                                                   \
    R1(d, e, a, b, c, 17);                                                                                   \
    R1(c, d, e, a, b, 18);                                                                                   \
    R1(b, c, d, e, a, 19);                                                                                   \
    R2(a, b, c, d, e, 20);                                                                                   \
    R2(e, a, b, c, d, 21);                                                                                   \
    R2(d, e, a, b, c, 22);                                                                                   \
    R2(c, d, e, a, b, 23);                                                                                   \
    R2(b, c, d, e, a, 24);                                                                                   \
    R2(a, b, c, d, e, 25);                                                                                   \
    R2(e, a, b, c, d, 26);                                                                                   \
    R2(d, e, a, b, c, 27);                                                                                   \
    R2(c, d, e, a, b, 28);                                                                                   \
    R2(b, c, d, e, a, 29);                                                                                   \
    R2(a, b, c, d, e, 30);                                                                                   \
    R2(e, a, b, c, d, 31);                                                                                   \
    R2(d, e, a, b, c, 32);                                                                                   \
    R2(c, d, e, a, b, 33);                                                                                   \
    R2(b, c, d, e, a, 34);                                                                                   \
    R2(a, b, c, d, e, 35);                                                                                   \
    R2(e, a, b, c, d, 36);                                                                                   \
    R2(d, e, a, b, c, 37);                                                                                   \
    R2(c, d, e, a, b, 38);                                                                                   \
    R2(b, c, d, e, a, 39);                                                                                   \
    R3(a, b, c, d, e, 40);                                                                                   \
    R3(e, a, b, c, d, 41);                                                                                   \
    R3(d, e, a, b, c, 42);                                                                                   \
    R3(c, d, e, a, b, 43);                                                                                   \
    R3(b, c, d, e, a, 44);                                                                                   \
    R3(a, b, c, d, e, 45);                                                                                   \
    R3(e, a, b, c, d, 46);                                                                                   \
    R3(d, e, a, b, c, 47);                                                                                   \
    R3(c, d, e, a, b, 48);                                                                                   \
    R3(b, c, d, e, a, 49);                                                                                   \
    R3(a, b, c, d, e, 50);                                                                                   \
    R3(e, a, b, c, d, 51);                                                                                   \
    R3(d, e, a, b, c, 52);                                                                                   \
    R3(c, d, e, a, b, 53);                                                                                   \
    R3(b, c, d, e, a, 54);                                                                                   \
    R3(a, b, c, d, e, 55);                                                                                   \
    R3(e, a, b, c, d, 56);                                                                                   \
    R3(d, e, a, b, c, 57);                                                                                   \
    R3(c, d, e, a, b, 58);                                                                                   \
    R3(b, c, d, e, a, 59);                                                                                   \
    R4(a, b, c, d, e, 60);                                                                                   \
    R4(e, a, b, c, d, 61);                                                                                   \
    R4(d, e, a, b, c, 62);                                                                                   \
    R4(c, d, e, a, b, 63);                                                                                   \
    R4(b, c, d, e, a, 64);                                                                                   \
    R4(a, b, c, d, e, 65);                                                                                   \
    R4(e, a, b, c, d, 66);                                                                                   \
    R4(d, e, a, b, c, 67);                                                                                   \
    R4(c, d, e, a, b, 68);                                                                                   \
    R4(b, c, d, e, a, 69);                                                                                   \
    R4(a, b, c, d, e, 70);                                                                                   \
    R4(e, a, b, c, d, 71);                                                                                   \
    R4(d, e, a, b, c, 72);                                                                                   \
    R4(c, d, e, a, b, 73);                                                                                   \
    R4(b, c, d, e, a, 74);                                                                                   \
    R4(a, b, c, d, e, 75);                                                                                   \
    R4(e, a, b, c, d, 76);                                                                                   \
    R4(d, e, a, b, c, 77);                                                                                   \
    R4(c, d, e, a, b, 78);                                                                                   \
    R4(b, c, d, e, a, 79);

/* ── Variant 1: general — full 16-word load from bytes (used by sha1_update) ── */
static IRAM_ATTR __attribute__((optimize("O3"), hot)) void
sha1_transform(uint32_t state[5], const uint8_t buf[64]) {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];
    uint32_t blk[16], tmp;
    for (int i = 0; i < 16; i++) {
        memcpy(&tmp, buf + i * 4, 4);
        blk[i] = __builtin_bswap32(tmp);
    }
    SHA1_80_ROUNDS()
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

/* ── Variant 2: 20-byte byte-input with fixed SHA1 padding ── */
static IRAM_ATTR __attribute__((optimize("O3"), hot, always_inline)) void
sha1_transform_20b(uint32_t state[5], const uint8_t data[20]) {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];
    uint32_t blk[16], tmp;
    memcpy(&tmp, data + 0, 4);
    blk[0] = __builtin_bswap32(tmp);
    memcpy(&tmp, data + 4, 4);
    blk[1] = __builtin_bswap32(tmp);
    memcpy(&tmp, data + 8, 4);
    blk[2] = __builtin_bswap32(tmp);
    memcpy(&tmp, data + 12, 4);
    blk[3] = __builtin_bswap32(tmp);
    memcpy(&tmp, data + 16, 4);
    blk[4] = __builtin_bswap32(tmp);
    blk[5] = 0x80000000u;
    blk[6] = 0;
    blk[7] = 0;
    blk[8] = 0;
    blk[9] = 0;
    blk[10] = 0;
    blk[11] = 0;
    blk[12] = 0;
    blk[13] = 0;
    blk[14] = 0;
    blk[15] = 0x000002A0u;
    SHA1_80_ROUNDS()
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

/* ── Variant 3: 20-byte word-input with fixed SHA1 padding ── */
static IRAM_ATTR __attribute__((optimize("O3"), hot, always_inline)) void
sha1_transform_20w(uint32_t state[5], const uint32_t words[5]) {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];
    uint32_t blk[16];
    blk[0] = words[0];
    blk[1] = words[1];
    blk[2] = words[2];
    blk[3] = words[3];
    blk[4] = words[4];
    blk[5] = 0x80000000u;
    blk[6] = 0;
    blk[7] = 0;
    blk[8] = 0;
    blk[9] = 0;
    blk[10] = 0;
    blk[11] = 0;
    blk[12] = 0;
    blk[13] = 0;
    blk[14] = 0;
    blk[15] = 0x000002A0u;
    SHA1_80_ROUNDS()
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

static inline void sha1_extract(const uint32_t state[5], uint8_t digest[20]) {
    for (int i = 0; i < 5; i++) {
        uint32_t s = __builtin_bswap32(state[i]);
        memcpy(digest + i * 4, &s, 4);
    }
}

static void sha1_init(Sha1Ctx &ctx) {
    ctx.state[0] = 0x67452301u;
    ctx.state[1] = 0xEFCDAB89u;
    ctx.state[2] = 0x98BADCFEu;
    ctx.state[3] = 0x10325476u;
    ctx.state[4] = 0xC3D2E1F0u;
    ctx.count[0] = ctx.count[1] = 0;
}

static void sha1_update(Sha1Ctx &ctx, const uint8_t *data, size_t len) {
    uint32_t j = (ctx.count[0] >> 3) & 63;
    if ((ctx.count[0] += (uint32_t)(len << 3)) < (uint32_t)(len << 3)) ctx.count[1]++;
    ctx.count[1] += (uint32_t)(len >> 29);
    if (j + len > 63) {
        size_t i = 64 - j;
        memcpy(ctx.buf + j, data, i);
        sha1_transform(ctx.state, ctx.buf);
        for (; i + 63 < len; i += 64) sha1_transform(ctx.state, data + i);
        j = 0;
        memcpy(ctx.buf, data + i, len - i);
    } else {
        memcpy(ctx.buf + j, data, len);
    }
}

static void sha1_final(Sha1Ctx &ctx, uint8_t digest[20]) {
    uint64_t total_bits = ((uint64_t)ctx.count[1] << 32) | ctx.count[0];
    uint32_t j = (ctx.count[0] >> 3) & 63;
    ctx.buf[j++] = 0x80;
    if (j > 56) {
        memset(ctx.buf + j, 0, 64 - j);
        sha1_transform(ctx.state, ctx.buf);
        j = 0;
    }
    memset(ctx.buf + j, 0, 56 - j);
    ctx.buf[56] = (uint8_t)(total_bits >> 56);
    ctx.buf[57] = (uint8_t)(total_bits >> 48);
    ctx.buf[58] = (uint8_t)(total_bits >> 40);
    ctx.buf[59] = (uint8_t)(total_bits >> 32);
    ctx.buf[60] = (uint8_t)(total_bits >> 24);
    ctx.buf[61] = (uint8_t)(total_bits >> 16);
    ctx.buf[62] = (uint8_t)(total_bits >> 8);
    ctx.buf[63] = (uint8_t)(total_bits);
    sha1_transform(ctx.state, ctx.buf);
    sha1_extract(ctx.state, digest);
}

/* ─────────────────────────────────────────────────────────────
   HMAC-SHA1 with pre-computed pads (software, no mutex)
───────────────────────────────────────────────────────────── */
struct HmacSha1Pre {
    Sha1Ctx inner;
    Sha1Ctx outer;
};

static void hmac_sha1_precompute(const uint8_t *key, size_t klen, HmacSha1Pre &out) {
    uint8_t k_ipad[64], k_opad[64];
    memset(k_ipad, 0x36, 64);
    memset(k_opad, 0x5C, 64);
    uint8_t hk[20];
    if (klen > 64) {
        Sha1Ctx t;
        sha1_init(t);
        sha1_update(t, key, klen);
        sha1_final(t, hk);
        key = hk;
        klen = 20;
    }
    for (size_t i = 0; i < klen; i++) {
        k_ipad[i] ^= key[i];
        k_opad[i] ^= key[i];
    }
    sha1_init(out.inner);
    sha1_update(out.inner, k_ipad, 64);
    sha1_init(out.outer);
    sha1_update(out.outer, k_opad, 64);
}

static IRAM_ATTR __attribute__((optimize("O3"), hot, always_inline)) void
hmac_sha1_20(const HmacSha1Pre &pre, const uint8_t data[20], uint8_t out[20]) {
    uint32_t state[5];
    state[0] = pre.inner.state[0];
    state[1] = pre.inner.state[1];
    state[2] = pre.inner.state[2];
    state[3] = pre.inner.state[3];
    state[4] = pre.inner.state[4];
    sha1_transform_20b(state, data);
    uint32_t ih0 = state[0], ih1 = state[1], ih2 = state[2], ih3 = state[3], ih4 = state[4];
    const uint32_t ih[5] = {ih0, ih1, ih2, ih3, ih4};
    state[0] = pre.outer.state[0];
    state[1] = pre.outer.state[1];
    state[2] = pre.outer.state[2];
    state[3] = pre.outer.state[3];
    state[4] = pre.outer.state[4];
    sha1_transform_20w(state, ih);
    sha1_extract(state, out);
}

static IRAM_ATTR __attribute__((optimize("O3"), hot, always_inline)) void
hmac_sha1_20w(const HmacSha1Pre &pre, const uint32_t data[5], uint32_t out[5]) {
    uint32_t state[5];
    state[0] = pre.inner.state[0];
    state[1] = pre.inner.state[1];
    state[2] = pre.inner.state[2];
    state[3] = pre.inner.state[3];
    state[4] = pre.inner.state[4];
    sha1_transform_20w(state, data);
    uint32_t ih[5] = {state[0], state[1], state[2], state[3], state[4]};
    state[0] = pre.outer.state[0];
    state[1] = pre.outer.state[1];
    state[2] = pre.outer.state[2];
    state[3] = pre.outer.state[3];
    state[4] = pre.outer.state[4];
    sha1_transform_20w(state, ih);
    out[0] = state[0];
    out[1] = state[1];
    out[2] = state[2];
    out[3] = state[3];
    out[4] = state[4];
}

static void hmac_sha1_with_pre(const HmacSha1Pre &pre, const uint8_t *data, size_t dlen, uint8_t *out20) {
    uint8_t inner_hash[20];
    Sha1Ctx ctx = pre.inner;
    sha1_update(ctx, data, dlen);
    sha1_final(ctx, inner_hash);
    ctx = pre.outer;
    sha1_update(ctx, inner_hash, 20);
    sha1_final(ctx, out20);
}

/* ─────────────────────────────────────────────────────────────
   PBKDF2-HMAC-SHA1 — fully unrolled for dklen=32 (WPA2 PMK)
───────────────────────────────────────────────────────────── */
static IRAM_ATTR __attribute__((optimize("O3"), hot)) void pbkdf2_precomp(
    const HmacSha1Pre &pre, const uint8_t *salt, size_t slen, unsigned int iters, uint8_t *out,
    size_t /* dklen always 32 */
) {
    uint8_t salt_int[40];
    memcpy(salt_int, salt, slen);

    uint8_t U8[20];
    uint32_t U[5];
    uint32_t T[5];

    /* Block 1 */
    salt_int[slen] = 0;
    salt_int[slen + 1] = 0;
    salt_int[slen + 2] = 0;
    salt_int[slen + 3] = 1;
    hmac_sha1_with_pre(pre, salt_int, slen + 4, U8);
    {
        uint32_t t;
        memcpy(&t, U8 + 0, 4);
        U[0] = __builtin_bswap32(t);
        memcpy(&t, U8 + 4, 4);
        U[1] = __builtin_bswap32(t);
        memcpy(&t, U8 + 8, 4);
        U[2] = __builtin_bswap32(t);
        memcpy(&t, U8 + 12, 4);
        U[3] = __builtin_bswap32(t);
        memcpy(&t, U8 + 16, 4);
        U[4] = __builtin_bswap32(t);
    }
    T[0] = U[0];
    T[1] = U[1];
    T[2] = U[2];
    T[3] = U[3];
    T[4] = U[4];
    for (unsigned int i = 1; i < iters; i++) {
        hmac_sha1_20w(pre, U, U);
        T[0] ^= U[0];
        T[1] ^= U[1];
        T[2] ^= U[2];
        T[3] ^= U[3];
        T[4] ^= U[4];
    }
    sha1_extract(T, out);

    /* Block 2 */
    salt_int[slen + 3] = 2;
    hmac_sha1_with_pre(pre, salt_int, slen + 4, U8);
    {
        uint32_t t;
        memcpy(&t, U8 + 0, 4);
        U[0] = __builtin_bswap32(t);
        memcpy(&t, U8 + 4, 4);
        U[1] = __builtin_bswap32(t);
        memcpy(&t, U8 + 8, 4);
        U[2] = __builtin_bswap32(t);
        memcpy(&t, U8 + 12, 4);
        U[3] = __builtin_bswap32(t);
        memcpy(&t, U8 + 16, 4);
        U[4] = __builtin_bswap32(t);
    }
    T[0] = U[0];
    T[1] = U[1];
    T[2] = U[2];
    T[3] = U[3];
    T[4] = U[4];
    for (unsigned int i = 1; i < iters; i++) {
        hmac_sha1_20w(pre, U, U);
        T[0] ^= U[0];
        T[1] ^= U[1];
        T[2] ^= U[2];
        T[3] ^= U[3];
        T[4] ^= U[4];
    }
    sha1_extract(T, out + 20);
}

/* ─────────────────────────────────────────────────────────────
   PTK Derivation (PRF-512)
───────────────────────────────────────────────────────────── */
static bool derive_ptk(
    const uint8_t *pmk, const uint8_t *ap_mac, const uint8_t *sta_mac, const uint8_t *anonce,
    const uint8_t *snonce, uint8_t *ptk_out
) {
    uint8_t data[76];
    size_t pos = 0;
    if (memcmp(ap_mac, sta_mac, 6) < 0) {
        memcpy(data + pos, ap_mac, 6);
        pos += 6;
        memcpy(data + pos, sta_mac, 6);
        pos += 6;
    } else {
        memcpy(data + pos, sta_mac, 6);
        pos += 6;
        memcpy(data + pos, ap_mac, 6);
        pos += 6;
    }
    if (memcmp(anonce, snonce, 32) < 0) {
        memcpy(data + pos, anonce, 32);
        pos += 32;
        memcpy(data + pos, snonce, 32);
        pos += 32;
    } else {
        memcpy(data + pos, snonce, 32);
        pos += 32;
        memcpy(data + pos, anonce, 32);
        pos += 32;
    }

    const char *label = "Pairwise key expansion";
    const size_t label_len = 22;

    HmacSha1Pre pmk_pre;
    hmac_sha1_precompute(pmk, 32, pmk_pre);

    uint8_t msg[label_len + 1 + 76 + 1];
    memcpy(msg, label, label_len);
    msg[label_len] = 0x00;
    memcpy(msg + label_len + 1, data, pos);

    for (int i = 0; i < 4; i++) {
        if (g_abortRequested) return false;
        msg[label_len + 1 + pos] = (uint8_t)i;
        uint8_t hash[20];
        hmac_sha1_with_pre(pmk_pre, msg, label_len + 1 + pos + 1, hash);
        size_t cp = (i == 3) ? 4 : 20;
        memcpy(ptk_out + (i * 20), hash, cp);
    }
    return true;
}

/* ─────────────────────────────────────────────────────────────
   MIC Verification
───────────────────────────────────────────────────────────── */
static bool verify_mic(const HandshakeData &hs, const uint8_t *ptk) {
    uint8_t eapol_copy[256];
    memcpy(eapol_copy, hs.eapol, hs.eapol_len);
    memset(eapol_copy + 81, 0, 16);
    HmacSha1Pre kck_pre;
    hmac_sha1_precompute(ptk, 16, kck_pre);
    uint8_t computed_mic[20];
    hmac_sha1_with_pre(kck_pre, eapol_copy, hs.eapol_len, computed_mic);
    return memcmp(computed_mic, hs.mic, 16) == 0;
}

/* ─────────────────────────────────────────────────────────────
   Dual-core cracking via FreeRTOS queue
───────────────────────────────────────────────────────────── */
#define PW_MAX_LEN 64
#define QUEUE_DEPTH 8

struct PwEntry {
    char pw[PW_MAX_LEN];
    uint8_t len;
};

struct CrackShared {
    const HandshakeData *hs;
    QueueHandle_t queue;
    volatile bool found;
    char found_pw[PW_MAX_LEN];
    volatile uint32_t attempts;
    volatile bool abort;
    SemaphoreHandle_t done_sem;
};

static __attribute__((optimize("O3"), hot)) bool
try_password(const CrackShared &S, const char *pw, uint8_t pw_len) {
    const HandshakeData &hs = *S.hs;
    const size_t ssid_len = strlen(hs.ssid);
    HmacSha1Pre pw_pre;
    hmac_sha1_precompute((const uint8_t *)pw, pw_len, pw_pre);
    uint8_t pmk[32];
    pbkdf2_precomp(pw_pre, (const uint8_t *)hs.ssid, ssid_len, 4096, pmk, 32);
    uint8_t ptk[64];
    if (!derive_ptk(pmk, hs.ap_mac, hs.sta_mac, hs.anonce, hs.snonce, ptk)) return false;
    return verify_mic(hs, ptk);
}

static void crack_worker_task(void *arg) {
    CrackShared &S = *(CrackShared *)arg;
    PwEntry entry;

    while (true) {
        if (xQueueReceive(S.queue, &entry, portMAX_DELAY) != pdTRUE) break;
        if (entry.len == 0) break; // sentinel
        if (S.found || S.abort) {
            __atomic_fetch_add(&S.attempts, 1, __ATOMIC_RELAXED);
            continue;
        }

        if (try_password(S, entry.pw, entry.len)) {
            S.found = true;
            memcpy(S.found_pw, entry.pw, entry.len + 1);
        }
        __atomic_fetch_add(&S.attempts, 1, __ATOMIC_RELAXED);

        // Thermal throttle + TWDT keep-alive.
        // vTaskDelay(pdMS_TO_TICKS(CRACK_YIELD_MS)) blocks long enough
        // for the idle task to run (which resets the 5s watchdog) and
        // reduces CPU duty cycle to limit heat. Tune CRACK_YIELD_MS at
        // the top of the file — default 2ms costs ~2.7% speed, saves heat.
        vTaskDelay(pdMS_TO_TICKS(CRACK_YIELD_MS));
    }

    xSemaphoreGive(S.done_sem);
    vTaskDelete(NULL);
}

/* ─────────────────────────────────────────────────────────────
   Buffered wordlist reader
───────────────────────────────────────────────────────────── */
struct WordlistReader {
    File *file;
    uint8_t *buf;
    size_t cap, len, pos;
    bool eof;

    bool init(File *f) {
        file = f;
        // Always use internal SRAM for the wordlist buffer.
        // PSRAM (when present) causes D-cache pressure that slows
        // the SHA1 hot path on Core 0. 8KB in internal SRAM is faster
        // than 32KB in PSRAM for this sequential-read workload.
        cap = 8192u;
        buf = (uint8_t *)malloc(cap);
        len = pos = 0;
        eof = false;
        return buf != nullptr;
    }
    void deinit() {
        if (buf) {
            free(buf);
            buf = nullptr;
        }
    }

    void refill() {
        if (eof) return;
        size_t rem = len - pos;
        if (rem) memmove(buf, buf + pos, rem);
        len = rem;
        pos = 0;
        size_t space = cap - len;
        if (!space) return;
        size_t rd = file->read(buf + len, space);
        len += rd;
        if (rd == 0) eof = true;
    }

    bool nextLine(char *out, size_t max_len) {
        while (true) {
            for (size_t i = pos; i < len; i++) {
                if (buf[i] == '\n') {
                    size_t ll = i - pos;
                    if (ll > 0 && buf[pos + ll - 1] == '\r') ll--;
                    size_t cp = (ll < max_len) ? ll : max_len;
                    memcpy(out, buf + pos, cp);
                    out[cp] = '\0';
                    pos = i + 1;
                    return true;
                }
            }
            if (eof) {
                size_t rem = len - pos;
                if (!rem) return false;
                if (buf[pos + rem - 1] == '\r') rem--;
                size_t cp = (rem < max_len) ? rem : max_len;
                memcpy(out, buf + pos, cp);
                out[cp] = '\0';
                pos = len;
                return cp > 0;
            }
            refill();
        }
    }

    bool available() const { return !eof || pos < len; }
};

/* ─────────────────────────────────────────────────────────────
   Main cracking function
───────────────────────────────────────────────────────────── */
void wifi_crack_handshake(const String &wordlist_path, const String &pcap_path) {
    g_abortRequested = false;

    // Boost to 240 MHz for maximum cracking speed.
    // Restored to firmware default on every exit path below.
    setCpuFrequencyMhz(240);

    resetTftDisplay();
    drawMainBorderWithTitle("WiFi Password Recover", true);
    padprintln("");

    FS *fs = nullptr;
    if (!getFsStorage(fs)) {
        setCpuFrequencyMhz(CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
        displayError("No filesystem available", true);
        return;
    }

    HandshakeData hs;
    if (!parse_pcap_handshake(*fs, pcap_path, hs)) {
        setCpuFrequencyMhz(CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
        displayError("Failed to parse handshake", true);
        vTaskDelay(pdMS_TO_TICKS(3000));
        return;
    }

    padprintf("SSID: %s\n", hs.ssid[0] ? hs.ssid : "(not found)");
    padprintf(
        "AP: %02X:%02X:%02X:%02X:%02X:%02X\n",
        hs.ap_mac[0],
        hs.ap_mac[1],
        hs.ap_mac[2],
        hs.ap_mac[3],
        hs.ap_mac[4],
        hs.ap_mac[5]
    );
    padprintln("");

    if (hs.ssid[0] == '\0') {
        padprintln("SSID not found in PCAP");
        String ssid = keyboard("", 32, "Enter SSID:");
        if (ssid.length() == 0) {
            setCpuFrequencyMhz(CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
            displayError("SSID required", true);
            return;
        }
        strncpy(hs.ssid, ssid.c_str(), sizeof(hs.ssid) - 1);
        resetTftDisplay();
        drawMainBorderWithTitle("WiFi Password Recover", true);
        padprintln("");
        padprintf("SSID: %s\n", hs.ssid);
        padprintln("");
    }

    File wf = fs->open(wordlist_path, FILE_READ);
    if (!wf) {
        setCpuFrequencyMhz(CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
        displayError("Cannot open wordlist", true);
        return;
    }

    WordlistReader reader;
    if (!reader.init(&wf)) {
        setCpuFrequencyMhz(CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
        displayError("Out of memory", true);
        return;
    }

    CrackShared shared;
    shared.hs = &hs;
    shared.found = false;
    shared.abort = false;
    shared.attempts = 0;
    memset(shared.found_pw, 0, sizeof(shared.found_pw));
    shared.queue = xQueueCreate(QUEUE_DEPTH, sizeof(PwEntry));
    shared.done_sem = xSemaphoreCreateBinary();

    if (!shared.queue || !shared.done_sem) {
        setCpuFrequencyMhz(CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
        displayError("Queue alloc failed", true);
        if (shared.queue) vQueueDelete(shared.queue);
        if (shared.done_sem) vSemaphoreDelete(shared.done_sem);
        return;
    }

    xTaskCreatePinnedToCore(
        crack_worker_task,
        "crack_w",
        24576, // 24KB: covers blk[16] + Sha1Ctx frames nested 5 deep
        &shared,
        5, // priority
        NULL,
        0 // core 0
    );

    padprintln("Recovering...");
    padprintln("(Press SEL to abort)");
    padprintln("");

    uint64_t start_time = now_us();
    uint64_t last_ui = start_time;
    PwEntry entry;
    bool producer_done = false;

    while (!shared.found && !shared.abort) {

        if (check(AnyKeyPress)) {
            shared.abort = true;
            g_abortRequested = true;
            padprintln("");
            padprintln("Aborted by user");
            break;
        }

        if (!producer_done) {
            if (!reader.available()) {
                producer_done = true;
            } else if (reader.nextLine(entry.pw, PW_MAX_LEN - 1)) {
                entry.len = (uint8_t)strlen(entry.pw);
                if (entry.len >= 8 && entry.len <= 63) {
                    if (xQueueSend(shared.queue, &entry, 0) != pdTRUE) {
                        if (!shared.found) {
                            if (try_password(shared, entry.pw, entry.len)) {
                                shared.found = true;
                                memcpy(shared.found_pw, entry.pw, entry.len + 1);
                            }
                            __atomic_fetch_add(&shared.attempts, 1, __ATOMIC_RELAXED);
                        }
                    }
                }
            } else {
                producer_done = true;
            }
        } else {
            if (uxQueueMessagesWaiting(shared.queue) == 0) break;
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        uint64_t now = now_us();
        if (now - last_ui > 1000000ULL) {
            double elapsed = (now - start_time) / 1000000.0;
            double rate = shared.attempts / (elapsed > 0 ? elapsed : 1.0);
            padprintf("\r%lu | %.1f/s | %.1fs     ", (uint32_t)shared.attempts, rate, elapsed);
            last_ui = now;
        }
    }

    PwEntry sentinel;
    sentinel.len = 0;
    xQueueSend(shared.queue, &sentinel, pdMS_TO_TICKS(500));
    xSemaphoreTake(shared.done_sem, pdMS_TO_TICKS(5000));

    vQueueDelete(shared.queue);
    vSemaphoreDelete(shared.done_sem);
    reader.deinit();
    wf.close();

    uint64_t total_time = now_us() - start_time;
    double seconds = total_time / 1000000.0;

    padprintln("");
    padprintln("");
    // padprintf("Tested: %u passwords\n", (uint32_t)shared.attempts);
    // padprintf("Time: %.1f sec (%.1f/sec)\n", seconds, shared.attempts / (seconds > 0 ? seconds : 1.0));
    padprintln("");

    if (shared.found) {
        resetTftDisplay();
        drawMainBorderWithTitle("WiFi Password Cracker", true);
        padprintln("");
        tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
        padprintln("PASSWORD FOUND!");
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        padprintln("");
        padprintf("SSID: %s\n", hs.ssid[0] ? hs.ssid : "(not found)");

        String display_pw = String(shared.found_pw);
        const int MAX_DISPLAY = 28;
        if ((int)display_pw.length() > MAX_DISPLAY) {
            int tail = MAX_DISPLAY - 17;
            if (tail < 3) tail = 3;
            display_pw =
                display_pw.substring(0, 14) + "..." + display_pw.substring(display_pw.length() - tail);
        }
        padprintf("Password: %s\n", display_pw.c_str());
        padprintln("");
        padprintln("Press any key to continue...");
        while (!check(AnyKeyPress)) vTaskDelay(pdMS_TO_TICKS(50));

    } else if (!g_abortRequested) {
        tft.setTextColor(TFT_RED, bruceConfig.bgColor);
        padprintln("Password not found");
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        displayError("No match", true);
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    // Restore CPU frequency to firmware default before returning
    setCpuFrequencyMhz(CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);

    vTaskDelay(pdMS_TO_TICKS(200));
}

/* ─────────────────────────────────────────────────────────────
   Menu entry point
───────────────────────────────────────────────────────────── */
void wifi_recover_menu() {
    resetTftDisplay();

    FS *fs = nullptr;
    if (!getFsStorage(fs)) {
        displayError("No filesystem", true);
        return;
    }

    const String WORDLIST_DIR = "/wordlists";
    if (!(*fs).exists(WORDLIST_DIR)) {
        if ((*fs).mkdir(WORDLIST_DIR)) padprintf("Created: %s\n", WORDLIST_DIR.c_str());
        else padprintf("Warning: failed to create %s\n", WORDLIST_DIR.c_str());
    }

    String wordlist = loopSD(*fs, true, "txt|lst|csv|*", WORDLIST_DIR);
    if (wordlist.length() == 0) {
        displayInfo("Cancelled", true);
        return;
    }

    const String PCAP_DIR = "/BrucePCAP";
    if (!(*fs).exists(PCAP_DIR)) {
        if ((*fs).mkdir(PCAP_DIR)) padprintf("Created: %s\n", PCAP_DIR.c_str());
        else padprintf("Warning: failed to create %s\n", PCAP_DIR.c_str());
    }

    resetTftDisplay();
    String pcap = loopSD(*fs, true, "pcap|cap|*", PCAP_DIR);
    if (pcap.length() == 0) {
        displayInfo("Cancelled", true);
        return;
    }

    wifi_crack_handshake(wordlist, pcap);
    while (!check(AnyKeyPress)) vTaskDelay(pdMS_TO_TICKS(50));
}
