#include "esl_app.h"

#include "esl_bmp.h"
#include "esl_fs.h"
#include "esl_ir_driver.h"
#include "esl_menu_labels.h"
#include "esl_nfc.h"
#include "esl_proto.h"
#include "esl_text.h"
#include "esl_tx.h"
#include "esl_wifi.h"
#include "esl_wifi_client.h"
#include "font/tagtinker_font.h"
#include "core/bus_HAL.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/scrollableTextArea.h"
#include "core/sd_functions.h"
#include "core/wifi/wifi_common.h"
#include "modules/ir/TV-B-Gone.h"
#if !defined(LITE_VERSION)
#include "modules/rfid/ST25R3916.h"
#endif
#include "modules/rfid/PN532.h"
#include "modules/rfid/RFID2.h"
#include "modules/rfid/RFIDInterface.h"
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <SD.h>
#include <globals.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ESL_BARCODE_LEN 17
#define ESL_PRE_TX_SETTLE_MS 500
#define TAGTINKER_MAX_SYNCED_IMAGES 24
#define ESL_DROPPED_DIR "/tagtinker/dropped"

/* Upstream's Color 2.6 file cap (TX_COLOR26_BMP_MAX). Do not raise it. */
#define ESL_COLOR26_BMP_MAX 24576u

/* Generic profiles have no upstream file cap because upstream streams rows
 * from SD. The encode-then-transmit rule forces us to hold the file in RAM
 * instead, so a bound is required: 256 KB clears the largest profile with
 * headroom (800x480 two-plane 1bpp is ~96 KB). */
#define ESL_GENERIC_BMP_MAX 262144u

struct EslUiCtx {
    bool aborted;
};

static bool ui_send(void *ctx, const uint8_t *frame, size_t len,
                    uint16_t repeats, uint8_t delay) {
    (void)ctx;
    return esl_ir_transmit(frame, len, repeats, delay);
}

static void ui_settle(void *ctx, uint32_t ms) {
    (void)ctx;
    delay(ms);
}

/* EslTxOps abort: consulted between frames by the sequencing layer. */
static bool ui_aborted(void *ctx) {
    EslUiCtx *c = (EslUiCtx *)ctx;
    if (!c->aborted && check(EscPress)) {
        c->aborted = true;
        esl_ir_stop();
    }
    return c->aborted;
}

/* Driver abort hook: consulted between *repeats* of a single frame. Needed as
 * well as ui_aborted because the 401-repeat wake burst happens inside one
 * esl_ir_transmit call, which the frame-level check cannot interrupt. */
static bool ui_abort_poll(void *ctx) {
    EslUiCtx *c = (EslUiCtx *)ctx;
    if (!c->aborted && check(EscPress)) c->aborted = true;
    return c->aborted;
}

static void ui_progress(void *ctx, size_t done, size_t total) {
    (void)ctx;
    progressHandler((int)done, total, "Sending");
}

static const char *esl_target_label(const EslTarget *t) {
    return t->name[0] ? t->name : t->barcode;
}

/* + Type Barcode. Segment tags are valid here; graphics actions omit them. */
static bool esl_parse_typed_barcode(const String &entered, uint8_t plid[4],
                                    TagTinkerTagProfile *profile) {
    /* Bruce's keyboard returns ESC when the user backs out. Treat that as a
     * silent cancel rather than scolding them about a length they never
     * entered. (ESC is not whitespace, so it survives trim().) */
    if (entered.length() == 0 || entered == "\x1B") return false;

    if (entered.length() != ESL_BARCODE_LEN) {
        displayError("Barcode must be 17 chars", true);
        return false;
    }
    if (!tagtinker_barcode_to_plid(entered.c_str(), plid)) {
        displayError("Bad barcode", true);
        return false;
    }
    if (!tagtinker_barcode_to_profile(entered.c_str(), profile)) {
        displayError("Unknown tag type", true);
        return false;
    }
    return true;
}

/* Mirrors scene_image_options, where the page is the only per-image knob.
 * Position, compression and frame-repeat stay at upstream's defaults. */
static const char *ESL_PAGE_LABELS[8] = {"Page 0", "Page 1", "Page 2",
                                         "Page 3", "Page 4", "Page 5",
                                         "Page 6", "Page 7"};

static bool esl_prompt_page(uint8_t *page) {
    int chosen = -1;
    options.clear();
    for (uint8_t p = 0; p < 8; p++) {
        options.push_back(
            Option(ESL_PAGE_LABELS[p], [&chosen, p]() { chosen = (int)p; }));
    }
    /* Start on the caller's default (resolved page 2 for Color 2.6, else 0)
     * but keep the full 0–7 range so an explicit pick wins verbatim. */
    loopOptions(options, MENU_TYPE_SUBMENU, "Page", (int)*page);
    if (chosen < 0) return false; /* user backed out */
    *page = (uint8_t)chosen;
    return true;
}

static void esl_noop(void);

/* Reads the whole file into PSRAM. Caller frees. */
static uint8_t *esl_read_file(fs::FS &fs, const String &path, size_t max_bytes,
                              size_t *out_len) {
    File f = fs.open(path, FILE_READ);
    if (!f) return nullptr;

    const size_t len = f.size();
    if (len < 54u || len > max_bytes) {
        f.close();
        return nullptr;
    }

    uint8_t *buf = (uint8_t *)ps_malloc(len);
    if (buf == nullptr) {
        f.close();
        return nullptr;
    }
    const size_t got = f.read(buf, len);
    f.close();

    if (got != len) {
        free(buf);
        return nullptr;
    }
    *out_len = len;
    return buf;
}

static bool esl_filename_gave_page(const char *name) {
    if (name == NULL) return false;
    size_t name_len = strlen(name);
    int consumed = 0;
    unsigned w = 0, h = 0;
    if (sscanf(name, "%ux%u%n", &w, &h, &consumed) < 2) return false;
    if ((size_t)consumed >= name_len || name[consumed] != '_' ||
        name[consumed + 1] != 'p') {
        return false;
    }
    unsigned p = 0;
    return sscanf(name + consumed + 2, "%u", &p) == 1 && p <= 7U;
}

static uint8_t esl_seed_image_page(const EslTarget *target, const char *name,
                                   uint8_t parsed_page) {
    if (esl_filename_gave_page(name)) return parsed_page;
    if (tagtinker_profile_needs_wh_swap(&target->profile)) {
        return tagtinker_color26_resolve_page(0);
    }
    return 0;
}

static String esl_basename(const String &path) {
    int slash = path.lastIndexOf('/');
    return slash >= 0 ? path.substring(slash + 1) : path;
}

struct EslBmpChoice {
    String path;
    char job_id[ESL_STORE_JOB_ID_LEN + 1];
    uint8_t page;
    bool resampled;
};

static FS *esl_active_fs(void) {
    setupSdCard();
    FS *fs = nullptr;
    if (!getFsStorage(fs) || fs == nullptr) return nullptr;
    if (!fs->exists("/tagtinker")) fs->mkdir("/tagtinker");
    if (!fs->exists(ESL_DROPPED_DIR)) fs->mkdir(ESL_DROPPED_DIR);
    return fs;
}

static bool esl_already_listed(const std::vector<EslBmpChoice> &out,
                               const String &path) {
    for (size_t i = 0; i < out.size(); i++) {
        if (out[i].path == path) return true;
    }
    return false;
}

static void esl_add_bmp_choice(const String &path, uint16_t tw, uint16_t th,
                               std::vector<EslBmpChoice> &out) {
    if (out.size() >= TAGTINKER_MAX_SYNCED_IMAGES) return;

    String base = esl_basename(path);
    if (base.length() == 0 || base[0] == '.') return;

    char job[ESL_STORE_JOB_ID_LEN + 1];
    uint8_t page = 1;
    bool resampled = true;
    if (!esl_parse_dropped_filename(base.c_str(), tw, th, job, sizeof(job),
                                    &page, &resampled)) {
        return;
    }
    if (esl_already_listed(out, path)) return;

    EslBmpChoice e;
    e.path = path;
    memset(e.job_id, 0, sizeof(e.job_id));
    strncpy(e.job_id, job, ESL_STORE_JOB_ID_LEN);
    e.job_id[ESL_STORE_JOB_ID_LEN] = '\0';
    e.page = page;
    e.resampled = resampled;
    out.push_back(e);
}

/* Non-recursive: files in `dir` only. Depth-8 / whole-volume walks froze Set
 * Image on a typical Bruce card; dropped/ then SD-root still finds the spec
 * example `296x152_cat.bmp`. */
static void esl_collect_bmp_dir(FS &fs, const String &dir, uint16_t tw,
                                uint16_t th, std::vector<EslBmpChoice> &out) {
    if (out.size() >= TAGTINKER_MAX_SYNCED_IMAGES) return;

    File root = fs.open(dir);
    if (!root || !root.isDirectory()) {
        if (root) root.close();
        return;
    }

    while (out.size() < TAGTINKER_MAX_SYNCED_IMAGES) {
        bool isDir = false;
        String full = root.getNextFileName(&isDir);
        if (full.length() == 0) break;
        if (!full.startsWith("/")) {
            full = (dir == "/") ? ("/" + full) : (dir + "/" + full);
        }
        String base = esl_basename(full);
        if (full == dir || base == "." || base == "..") continue;
        if (isDir) continue;
        esl_add_bmp_choice(full, tw, th, out);
    }
    root.close();
}

static void esl_collect_images(FS &fs, const EslTarget *target,
                               std::vector<EslBmpChoice> &out) {
    const uint16_t tw = target->profile.width;
    const uint16_t th = target->profile.height;
    esl_collect_bmp_dir(fs, ESL_DROPPED_DIR, tw, th, out);
    esl_collect_bmp_dir(fs, "/", tw, th, out);
}

/* Encode the chosen file, then claim IR. Page is sent verbatim. */
static void esl_send_bmp(EslSession *sess, const EslTarget *target, FS &fs,
                         const String &path, uint8_t page) {
    const bool is_color26 = tagtinker_profile_needs_wh_swap(&target->profile);
    const size_t cap = is_color26 ? ESL_COLOR26_BMP_MAX : ESL_GENERIC_BMP_MAX;

    /* Everything that touches the card happens before the IR pin is claimed,
     * because setup_ir_pin() may tear down the SD SPI bus. */
    size_t file_len = 0;
    uint8_t *file = esl_read_file(fs, path, cap, &file_len);
    if (file == nullptr) {
        displayError("Cannot read BMP", true);
        return;
    }

    EslBmpInfo info;
    if (!esl_bmp_parse(file, file_len, &info)) {
        free(file);
        displayError("Unsupported BMP", true);
        return;
    }

    drawMainBorderWithTitle(ESL_UI_APP_NAME);
    displayTextLine("Encoding...");

    TagTinkerImagePayload payload;
    bool encoded = false;
    uint16_t out_w = 0;
    uint16_t out_h = 0;

    if (is_color26) {
        /* Upstream's Color 2.6 send path is what refuses non-plane sources. */
        if (info.bpp != 1u && info.bpp != 2u) {
            free(file);
            displayError("Need 1/2bpp BMP", true);
            return;
        }
        EslColor26BmpCtx ctx = {file, file_len, &info};
        const size_t total = (size_t)TAGTINKER_COLOR26_WIRE_W *
                             TAGTINKER_COLOR26_WIRE_H * 2U;
        encoded = tagtinker_encode_fn_payload(esl_color26_bmp_pixel, &ctx, total,
                                              TagTinkerCompressionAuto, &payload);
    } else {
        /* Accent plane follows the profile's colour capability, matching
         * upstream's use_second_plane with color_clear at its default. */
        out_w = target->profile.width;
        out_h = target->profile.height;
        const bool second_plane = (target->profile.color != TagTinkerTagColorMono);
        EslGenericBmpCtx ctx = {file, file_len, &info, out_w, out_h,
                                second_plane};
        const size_t total =
            (size_t)out_w * out_h * (second_plane ? 2U : 1U);
        encoded = tagtinker_encode_fn_payload(esl_generic_bmp_pixel, &ctx, total,
                                              TagTinkerCompressionAuto, &payload);
    }

    free(file); /* pixels are now baked into the payload */

    if (!encoded) {
        displayError("Encode failed", true);
        return;
    }

    checkIrTxPin();
    if (!esl_ir_init(bruceConfigPins.irTx)) {
        tagtinker_free_image_payload(&payload);
        displayError("IR init failed", true);
        return;
    }

    drawMainBorderWithTitle(ESL_UI_APP_NAME);
    displayTextLine("Sending, Esc aborts");
    delay(ESL_PRE_TX_SETTLE_MS);

    EslUiCtx ui = {false};
    EslTxOps ops = {ui_send, ui_settle, ui_aborted, ui_progress, &ui};

    /* Both abort paths share the same ui.aborted latch: the driver hook covers
     * long single-frame bursts, ops.aborted covers frame boundaries. */
    esl_ir_set_abort_hook(ui_abort_poll, &ui);

    const bool ok =
        is_color26 ? esl_tx_send_color26(&ops, target->plid, &payload, page)
                   : esl_tx_send_generic(&ops, target->plid, &payload, page,
                                         out_w, out_h, 0u, 0u,
                                         sess->settings.data_frame_repeats);

    esl_ir_set_abort_hook(nullptr, nullptr);
    esl_ir_deinit();
    tagtinker_free_image_payload(&payload);

    if (ui.aborted) {
        displayWarning("Aborted", true);
    } else if (ok) {
        displaySuccess("Sent", true);
    } else {
        displayError("TX failed", true);
    }
}

/* Same IR chrome as esl_send_bmp: pin check, init, progress, Esc abort, deinit.
 * No encode or file read — two queued frames only. */
static void esl_led_test(const EslTarget *target) {
    checkIrTxPin();
    if (!esl_ir_init(bruceConfigPins.irTx)) {
        displayError("IR init failed", true);
        return;
    }

    drawMainBorderWithTitle(ESL_UI_APP_NAME);
    displayTextLine("Sending, Esc aborts");
    delay(ESL_PRE_TX_SETTLE_MS);

    EslUiCtx ui = {false};
    EslTxOps ops = {ui_send, ui_settle, ui_aborted, ui_progress, &ui};
    esl_ir_set_abort_hook(ui_abort_poll, &ui);

    const bool ok = esl_tx_send_led_test(&ops, target->plid);

    esl_ir_set_abort_hook(nullptr, nullptr);
    esl_ir_deinit();

    if (ui.aborted) {
        displayWarning("Aborted", true);
    } else if (ok) {
        displaySuccess("Sent", true);
    } else {
        displayError("TX failed", true);
    }
}

static void esl_image_options(EslSession *sess, const EslTarget *target, FS &fs,
                              const String &path, uint8_t page) {
    while (true) {
        bool send = false;
        char page_lbl[16];
        snprintf(page_lbl, sizeof(page_lbl), "Page %u", (unsigned)page);

        options.clear();
        options.push_back({page_lbl, [&]() {
                               uint8_t p = page;
                               if (esl_prompt_page(&p)) page = p;
                           }});
        options.push_back({ESL_SEND_BMP, [&]() { send = true; }});

        int sel = loopOptions(options, MENU_TYPE_SUBMENU,
                              ESL_TARGET_ACTIONS_GRAPHICS[1]);
        if (sel < 0) return;
        if (send) {
            esl_send_bmp(sess, target, fs, path, page);
            return;
        }
    }
}

static void esl_set_image(EslSession *sess, const EslTarget *target) {
    FS *fs = esl_active_fs();
    /* Collect once. Empty-list helper rows are no-ops and must not walk the
     * FS again; Esc still leaves Set Image. */
    std::vector<EslBmpChoice> images;
    if (fs != nullptr) esl_collect_images(*fs, target, images);

    while (true) {
        int picked = -1;
        options.clear();
        if (images.empty()) {
            const size_t n =
                sizeof(ESL_SET_IMAGE_EMPTY) / sizeof(ESL_SET_IMAGE_EMPTY[0]);
            for (size_t i = 0; i < n; i++) {
                options.push_back({ESL_SET_IMAGE_EMPTY[i], esl_noop});
            }
        } else {
            for (int i = (int)images.size() - 1; i >= 0; i--) {
                const EslBmpChoice &e = images[(size_t)i];
                char label[64];
                snprintf(label, sizeof(label), "%sP%u %s",
                         e.resampled ? "~ " : "", (unsigned)e.page, e.job_id);
                options.push_back({label, [&picked, i]() { picked = i; }});
            }
        }

        int sel = loopOptions(options, MENU_TYPE_SUBMENU,
                              ESL_TARGET_ACTIONS_GRAPHICS[1]);
        if (sel < 0) return;
        if (picked >= 0 && fs != nullptr) {
            const EslBmpChoice &e = images[(size_t)picked];
            String base = esl_basename(e.path);
            uint8_t page = esl_seed_image_page(target, base.c_str(), e.page);
            esl_image_options(sess, target, *fs, e.path, page);
        }
    }
}

/* Flipper text_page_to_index: displayed Page 1–8 stored as img_page. */
static uint8_t esl_text_page_to_index(uint8_t page) {
    if (page == 0U) return 0U;
    if (page > 8U) return 7U;
    return (uint8_t)(page - 1U);
}

/* Flipper tagtinker_prepare_text_tx: page = img_page-1, clamp 0–7. */
static uint8_t esl_text_tx_page(uint8_t img_page) {
    uint8_t page = (img_page > 0U) ? (uint8_t)(img_page - 1U) : 0U;
    if (page > 7U) page = 7U;
    return page;
}

static void esl_text_render_full(uint8_t *primary, uint8_t *secondary, uint16_t w,
                                 uint16_t h, const char *text,
                                 const EslSession *sess,
                                 TagTinkerTagColor color) {
    const bool accent_capable = (color != TagTinkerTagColorMono);
    const bool accent_text = accent_capable && sess->color_clear;
    const size_t pixel_count = (size_t)w * h;

    if (accent_text) {
        uint8_t bg_primary = sess->invert_text ? 0 : 1;
        uint8_t fg_primary = (color == TagTinkerTagColorYellow) ? 0 : 1;
        render_text_ex(primary, w, h, text, bg_primary, fg_primary,
                       sess->text_padding_pct);
        render_text_ex(secondary, w, h, text, 1, 0, sess->text_padding_pct);
    } else {
        render_text_ex(primary, w, h, text, sess->invert_text ? 0 : 1,
                       sess->invert_text ? 1 : 0, sess->text_padding_pct);
        if (secondary) memset(secondary, 1, pixel_count);
    }
}

static void esl_text_render_region(uint8_t *primary, uint8_t *secondary,
                                   uint16_t w, uint16_t h, uint16_t y,
                                   uint16_t actual_h, const char *text,
                                   const EslSession *sess,
                                   TagTinkerTagColor color) {
    const bool accent_capable = (color != TagTinkerTagColorMono);
    const bool accent_text = accent_capable && sess->color_clear;

    if (accent_text) {
        uint8_t bg_primary = sess->invert_text ? 0 : 1;
        uint8_t fg_primary = (color == TagTinkerTagColorYellow) ? 0 : 1;
        render_text_region_ex(primary, w, h, y, actual_h, text, bg_primary,
                              fg_primary, sess->text_padding_pct);
        render_text_region_ex(secondary, w, h, y, actual_h, text, 1, 0,
                              sess->text_padding_pct);
    } else {
        render_text_region_ex(primary, w, h, y, actual_h, text,
                              sess->invert_text ? 0 : 1,
                              sess->invert_text ? 1 : 0, sess->text_padding_pct);
        if (secondary) memset(secondary, 1, (size_t)w * actual_h);
    }
}

static bool esl_text_run_ir(EslSession *sess, const EslTarget *target,
                            const TagTinkerImagePayload *payload, bool color26,
                            uint16_t w, uint16_t h, uint16_t pos_y) {
    checkIrTxPin();
    if (!esl_ir_init(bruceConfigPins.irTx)) {
        displayError("IR init failed", true);
        return false;
    }

    drawMainBorderWithTitle(ESL_UI_APP_NAME);
    displayTextLine("Sending, Esc aborts");
    delay(ESL_PRE_TX_SETTLE_MS);

    EslUiCtx ui = {false};
    EslTxOps ops = {ui_send, ui_settle, ui_aborted, ui_progress, &ui};
    esl_ir_set_abort_hook(ui_abort_poll, &ui);

    const uint8_t page = esl_text_tx_page(sess->img_page);
    const bool ok =
        color26 ? esl_tx_send_color26(&ops, target->plid, payload, page)
                : esl_tx_send_generic(&ops, target->plid, payload, page, w, h,
                                      0u, pos_y, sess->settings.data_frame_repeats);

    esl_ir_set_abort_hook(nullptr, nullptr);
    esl_ir_deinit();

    if (ui.aborted) {
        displayWarning("Aborted", true);
    } else if (ok) {
        displaySuccess("Sent", true);
    } else {
        displayError("TX failed", true);
    }
    return ok;
}

static void esl_send_text_color26(EslSession *sess, const EslTarget *target,
                                  const char *text) {
    const uint16_t gw = TAGTINKER_COLOR26_GLASS_W;
    const uint16_t gh = TAGTINKER_COLOR26_GLASS_H;
    const size_t pixel_count = (size_t)gw * gh;
    const bool accent_capable =
        (target->profile.color != TagTinkerTagColorMono);
    const bool accent_text = accent_capable && sess->color_clear;

    uint8_t *primary = (uint8_t *)ps_malloc(pixel_count);
    uint8_t *secondary =
        accent_text ? (uint8_t *)ps_malloc(pixel_count) : nullptr;
    if (primary == nullptr || (accent_text && secondary == nullptr)) {
        free(primary);
        free(secondary);
        displayError("Encode failed", true);
        return;
    }

    drawMainBorderWithTitle(ESL_UI_APP_NAME);
    displayTextLine("Encoding...");
    esl_text_render_full(primary, secondary, gw, gh, text, sess,
                         target->profile.color);

    EslTextColor26Ctx ctx = {primary, secondary};
    const size_t total = (size_t)TAGTINKER_COLOR26_WIRE_W *
                         TAGTINKER_COLOR26_WIRE_H * 2U;
    TagTinkerImagePayload payload;
    const bool encoded = tagtinker_encode_fn_payload(
        esl_text_color26_pixel, &ctx, total, TagTinkerCompressionAuto, &payload);
    free(primary);
    free(secondary);
    if (!encoded) {
        displayError("Encode failed", true);
        return;
    }

    esl_text_run_ir(sess, target, &payload, true, 0, 0, 0);
    tagtinker_free_image_payload(&payload);
}

static bool esl_send_text_generic_full(EslSession *sess, const EslTarget *target,
                                       const char *text, uint16_t w, uint16_t h,
                                       bool use_second_plane) {
    const size_t pixel_count = (size_t)w * h;
    uint8_t *primary = (uint8_t *)ps_malloc(pixel_count);
    uint8_t *secondary =
        use_second_plane ? (uint8_t *)ps_malloc(pixel_count) : nullptr;
    if (primary == nullptr || (use_second_plane && secondary == nullptr)) {
        free(primary);
        free(secondary);
        return false;
    }

    drawMainBorderWithTitle(ESL_UI_APP_NAME);
    displayTextLine("Encoding...");
    esl_text_render_full(primary, secondary, w, h, text, sess,
                         target->profile.color);

    TagTinkerImagePayload payload;
    const bool encoded = tagtinker_encode_planes_payload(
        primary, secondary, pixel_count, TagTinkerCompressionAuto, &payload);
    free(primary);
    free(secondary);
    if (!encoded) return false;

    esl_text_run_ir(sess, target, &payload, false, w, h, 0);
    tagtinker_free_image_payload(&payload);
    return true;
}

static void esl_send_text_generic_chunks(EslSession *sess,
                                         const EslTarget *target,
                                         const char *text, uint16_t w,
                                         uint16_t h, bool use_second_plane) {
    const uint16_t chunk_h = esl_text_chunk_height(w, h, use_second_plane);
    uint8_t *primary = (uint8_t *)malloc((size_t)w * chunk_h);
    uint8_t *secondary =
        use_second_plane ? (uint8_t *)malloc((size_t)w * chunk_h) : nullptr;
    if (primary == nullptr || (use_second_plane && secondary == nullptr)) {
        free(primary);
        free(secondary);
        displayError("Encode failed", true);
        return;
    }

    checkIrTxPin();
    if (!esl_ir_init(bruceConfigPins.irTx)) {
        free(primary);
        free(secondary);
        displayError("IR init failed", true);
        return;
    }

    drawMainBorderWithTitle(ESL_UI_APP_NAME);
    displayTextLine("Sending, Esc aborts");
    delay(ESL_PRE_TX_SETTLE_MS);

    EslUiCtx ui = {false};
    EslTxOps ops = {ui_send, ui_settle, ui_aborted, ui_progress, &ui};
    esl_ir_set_abort_hook(ui_abort_poll, &ui);

    const uint8_t page = esl_text_tx_page(sess->img_page);
    bool ok = true;
    for (uint16_t y = 0; ok && y < h; y = (uint16_t)(y + chunk_h)) {
        uint16_t actual_h = (uint16_t)(h - y);
        if (actual_h > chunk_h) actual_h = chunk_h;

        esl_text_render_region(primary, secondary, w, h, y, actual_h, text, sess,
                               target->profile.color);

        TagTinkerImagePayload payload;
        ok = tagtinker_encode_planes_payload(primary, secondary,
                                             (size_t)w * actual_h,
                                             TagTinkerCompressionAuto, &payload);
        if (ok) {
            ok = esl_tx_send_generic(&ops, target->plid, &payload, page, w,
                                     actual_h, 0u, y,
                                     sess->settings.data_frame_repeats);
            tagtinker_free_image_payload(&payload);
        }
        if (ok && (uint16_t)(y + actual_h) < h) {
            delay(esl_text_chunk_settle_ms(w, actual_h, sess->color_clear));
        }
    }

    esl_ir_set_abort_hook(nullptr, nullptr);
    esl_ir_deinit();
    free(primary);
    free(secondary);

    if (ui.aborted) {
        displayWarning("Aborted", true);
    } else if (ok) {
        displaySuccess("Sent", true);
    } else {
        displayError("TX failed", true);
    }
}

static void esl_send_text(EslSession *sess, const EslTarget *target,
                          const char *text) {
    if (sess == nullptr || target == nullptr || text == nullptr ||
        text[0] == '\0') {
        return;
    }

    if (tagtinker_profile_needs_wh_swap(&target->profile)) {
        esl_send_text_color26(sess, target, text);
        return;
    }

    const uint16_t w = target->profile.width;
    const uint16_t h = target->profile.height;
    const bool accent_capable =
        (target->profile.color != TagTinkerTagColorMono);
    const bool use_second_plane = accent_capable || sess->color_clear;

    if (esl_text_should_send_full(w, h, use_second_plane)) {
        if (!esl_send_text_generic_full(sess, target, text, w, h,
                                        use_second_plane)) {
            displayError("Encode failed", true);
        }
        return;
    }

    esl_send_text_generic_chunks(sess, target, text, w, h, use_second_plane);
}

static void esl_pick_text_page(uint8_t *img_page) {
    options.clear();
    for (uint8_t p = 1; p <= 8; p++) {
        options.push_back({String((unsigned)p), [img_page, p]() { *img_page = p; }});
    }
    loopOptions(options, MENU_TYPE_SUBMENU, "Page",
                (int)esl_text_page_to_index(*img_page));
}

static void esl_pick_text_polarity(EslSession *sess) {
    options = {
        {"B on W", [sess]() { sess->invert_text = false; }},
        {"W on B", [sess]() { sess->invert_text = true; }},
    };
    loopOptions(options, MENU_TYPE_SUBMENU, "Polarity",
                sess->invert_text ? 1 : 0);
}

static void esl_pick_text_padding(EslSession *sess) {
    options.clear();
    for (uint8_t i = 0; i < 9; i++) {
        uint8_t pct = (uint8_t)(i * 5);
        options.push_back({String((unsigned)pct) + "%",
                           [sess, pct]() { sess->text_padding_pct = pct; }});
    }
    uint8_t idx = sess->text_padding_pct / 5;
    if (idx > 8) idx = 8;
    loopOptions(options, MENU_TYPE_SUBMENU, "Padding", (int)idx);
}

static void esl_text_size_picker(EslSession *sess, const EslTarget *target,
                                 const char *text) {
    if (sess->img_page == 0U) sess->img_page = 1U;
    if (sess->img_page > 8U) sess->img_page = 8U;

    while (true) {
        bool tx = false;
        options.clear();
        options.push_back({String("Page  ") + String((unsigned)sess->img_page),
                           [sess]() { esl_pick_text_page(&sess->img_page); }});
        options.push_back({String("Polarity  ") +
                               (sess->invert_text ? "W on B" : "B on W"),
                           [sess]() { esl_pick_text_polarity(sess); }});
        options.push_back({String("Padding  ") +
                               String((unsigned)sess->text_padding_pct) + "%",
                           [sess]() { esl_pick_text_padding(sess); }});
        options.push_back({ESL_TRANSMIT, [&]() { tx = true; }});

        int sel = loopOptions(options, MENU_TYPE_SUBMENU,
                              ESL_TARGET_ACTIONS_GRAPHICS[0]);
        if (sel < 0) return;
        if (!tx) continue;

        sess->esl_width = target->profile.width;
        sess->esl_height = target->profile.height;
        esl_store_recents_add(sess, text);
        esl_fs_save_recents(sess);
        esl_send_text(sess, target, text);
        return;
    }
}

static void esl_set_text(EslSession *sess, const EslTarget *target) {
    while (true) {
        bool add_new = false;
        int recent_idx = -1;

        options.clear();
        options.push_back({ESL_RECENT_NEW, [&]() { add_new = true; }});
        for (uint8_t i = 0; i < sess->recent_count; i++) {
            if (target->profile.width > 0 && target->profile.height > 0) {
                if (sess->recents[i].width != target->profile.width ||
                    sess->recents[i].height != target->profile.height) {
                    continue;
                }
            }
            char label[ESL_STORE_TEXT_LEN + 4];
            snprintf(label, sizeof(label), "\"%s\"", sess->recents[i].text);
            options.push_back({label, [i, &recent_idx]() { recent_idx = (int)i; }});
        }

        int sel = loopOptions(options, MENU_TYPE_SUBMENU, "Recent Pushes");
        if (sel < 0) return;

        if (add_new) {
            String entered = keyboard("", ESL_STORE_TEXT_LEN, "Text to display:");
            if (entered.length() == 0 || entered == "\x1B") continue;
            char text[ESL_STORE_TEXT_LEN + 1];
            memset(text, 0, sizeof(text));
            strncpy(text, entered.c_str(), ESL_STORE_TEXT_LEN);
            text[ESL_STORE_TEXT_LEN] = '\0';
            esl_text_size_picker(sess, target, text);
            continue;
        }

        if (recent_idx < 0 || (uint8_t)recent_idx >= sess->recent_count) {
            continue;
        }

        const EslRecent *r = &sess->recents[recent_idx];
        sess->esl_width = r->width;
        sess->esl_height = r->height;
        sess->img_page = r->page;
        sess->invert_text = r->invert;
        sess->color_clear = r->color_clear;
        sess->text_padding_pct = r->padding;

        char text[ESL_STORE_TEXT_LEN + 1];
        memset(text, 0, sizeof(text));
        memcpy(text, r->text, ESL_STORE_TEXT_LEN);
        text[ESL_STORE_TEXT_LEN] = '\0';

        esl_store_recents_add(sess, text);
        esl_fs_save_recents(sess);
        esl_send_text(sess, target, text);
    }
}

static const char *const ESL_WARNING_LINES[][2] = {
    {"Educational tool for", "infrared ESL study."},
    {"Use only on tags", "you own or may test."},
    {"Unauthorized use", "may be illegal."},
    {"You are responsible", "for your actions."},
};

static void esl_noop(void) {}

typedef struct {
    uint8_t page;
    uint16_t duration;
    bool forever;
    uint16_t repeats;
    bool spam;
} EslBroadcastKnobs;

static const uint16_t ESL_BCAST_DURATIONS[] = {2, 5, 10, 15, 30, 60, 120};
static const char *const ESL_BCAST_DURATION_LBLS[] = {
    "2s", "5s", "10s", "15s", "30s", "60s", "120s"};
#define ESL_BCAST_DURATION_N ((int)(sizeof(ESL_BCAST_DURATIONS) / sizeof(ESL_BCAST_DURATIONS[0])))
static const uint16_t ESL_BCAST_REPEATS[] = {50, 100, 200, 400, 800};
#define ESL_BCAST_REPEATS_N ((int)(sizeof(ESL_BCAST_REPEATS) / sizeof(ESL_BCAST_REPEATS[0])))

static int esl_u16_index(const uint16_t *vals, int n, uint16_t want, int fallback) {
    for (int i = 0; i < n; i++) {
        if (vals[i] == want) return i;
    }
    return fallback;
}

static void esl_pick_bcast_page(uint8_t *page) {
    options.clear();
    for (uint8_t p = 0; p < 8; p++) {
        options.push_back({String((unsigned)p), [page, p]() { *page = p; }});
    }
    loopOptions(options, MENU_TYPE_SUBMENU, "Page", (int)*page);
}

static void esl_pick_bcast_duration(uint16_t *duration) {
    options.clear();
    for (int i = 0; i < ESL_BCAST_DURATION_N; i++) {
        uint16_t d = ESL_BCAST_DURATIONS[i];
        options.push_back(
            {ESL_BCAST_DURATION_LBLS[i], [duration, d]() { *duration = d; }});
    }
    int cur = esl_u16_index(ESL_BCAST_DURATIONS, ESL_BCAST_DURATION_N, *duration, 3);
    loopOptions(options, MENU_TYPE_SUBMENU, "Duration", cur);
}

static void esl_pick_bcast_repeats(uint16_t *repeats) {
    options.clear();
    for (int i = 0; i < ESL_BCAST_REPEATS_N; i++) {
        uint16_t r = ESL_BCAST_REPEATS[i];
        options.push_back({String((unsigned)r), [repeats, r]() { *repeats = r; }});
    }
    int cur = esl_u16_index(ESL_BCAST_REPEATS, ESL_BCAST_REPEATS_N, *repeats, 2);
    loopOptions(options, MENU_TYPE_SUBMENU, "Repeats", cur);
}

static void esl_pick_on_off(bool *val, const char *title) {
    options = {
        {"Off", [val]() { *val = false; }},
        {"On", [val]() { *val = true; }},
    };
    loopOptions(options, MENU_TYPE_SUBMENU, title, *val ? 1 : 0);
}

/* Same IR chrome as LED Test: pin check, init, Tag Tinker title, Esc abort. */
static void esl_broadcast_transmit(const uint8_t *frame, size_t len,
                                   uint16_t repeats, bool spam) {
    checkIrTxPin();
    if (!esl_ir_init(bruceConfigPins.irTx)) {
        displayError("IR init failed", true);
        return;
    }

    drawMainBorderWithTitle(ESL_UI_APP_NAME);
    displayTextLine("Sending, Esc aborts");
    delay(ESL_PRE_TX_SETTLE_MS);

    EslUiCtx ui = {false};
    EslTxOps ops = {ui_send, ui_settle, ui_aborted, ui_progress, &ui};
    esl_ir_set_abort_hook(ui_abort_poll, &ui);

    const bool ok = esl_tx_send_raw(&ops, frame, len, repeats, spam);

    esl_ir_set_abort_hook(nullptr, nullptr);
    esl_ir_deinit();

    if (ui.aborted) {
        displayWarning("Aborted", true);
    } else if (ok) {
        displaySuccess("Sent", true);
    } else {
        displayError("TX failed", true);
    }
}

static void esl_broadcast_knob_screen(EslBroadcastKnobs *k, bool change_page) {
    while (true) {
        bool tx = false;
        options.clear();
        if (change_page) {
            options.push_back({String("Page  ") + String((unsigned)k->page),
                               [k]() { esl_pick_bcast_page(&k->page); }});
            options.push_back({String("Duration  ") + String((unsigned)k->duration) + "s",
                               [k]() { esl_pick_bcast_duration(&k->duration); }});
            options.push_back({String("Forever  ") + (k->forever ? "On" : "Off"),
                               [k]() { esl_pick_on_off(&k->forever, "Forever"); }});
        }
        options.push_back({String("Repeats  ") + String((unsigned)k->repeats),
                           [k]() { esl_pick_bcast_repeats(&k->repeats); }});
        options.push_back({String("Repeat  ") + (k->spam ? "On" : "Off"),
                           [k]() { esl_pick_on_off(&k->spam, "Repeat"); }});
        options.push_back({ESL_TRANSMIT, [&]() { tx = true; }});

        const char *title =
            change_page ? ESL_BROADCAST_ITEMS[0] : ESL_BROADCAST_ITEMS[1];
        int sel = loopOptions(options, MENU_TYPE_SUBMENU, title);
        if (sel < 0) return;
        if (!tx) continue;

        uint8_t frame[TAGTINKER_MAX_FRAME_SIZE];
        size_t len =
            change_page
                ? tagtinker_build_broadcast_page_frame(frame, k->page, k->forever,
                                                       k->duration)
                : tagtinker_build_broadcast_debug_frame(frame);
        esl_broadcast_transmit(frame, len, k->repeats, k->spam);
    }
}

static bool esl_warning_pages(void) {
    const int last = (int)(sizeof(ESL_WARNING_TITLES) / sizeof(ESL_WARNING_TITLES[0])) - 1;
    int page = 0;

    while (true) {
        bool cont = false;
        int next_page = page;

        options.clear();
        options.push_back(Option(ESL_WARNING_LINES[page][0], esl_noop, false, nullptr,
                                 nullptr, false, false));
        options.push_back(Option(ESL_WARNING_LINES[page][1], esl_noop, false, nullptr,
                                 nullptr, false, false));
        if (page > 0) {
            options.push_back({"Prev", [&]() { next_page = page - 1; }});
        }
        if (page < last) {
            options.push_back({"Next", [&]() { next_page = page + 1; }});
        } else {
            options.push_back({"Continue", [&]() { cont = true; }});
        }

        int sel = loopOptions(options, MENU_TYPE_SUBMENU, ESL_WARNING_TITLES[page]);
        if (sel < 0) return false;
        if (cont) return true;
        page = next_page;
    }
}

static void esl_broadcast_menu(void) {
    EslBroadcastKnobs knobs = {0, 15, false, 200, false};
    while (true) {
        options = {
            {ESL_BROADCAST_ITEMS[0],
             [&]() { esl_broadcast_knob_screen(&knobs, true); }},
            {ESL_BROADCAST_ITEMS[1],
             [&]() { esl_broadcast_knob_screen(&knobs, false); }},
        };
        int sel = loopOptions(options, MENU_TYPE_SUBMENU, ESL_MAIN_ITEMS[0]);
        if (sel < 0) return;
    }
}

/* Packed worker planes as a top-down 1/2 bpp BMP-like source (same stride
 * as esl_bmp). Color 2.6 uses the existing glass/wire callback. */
static void esl_wifi_planes_as_bmp(const EslWifiRenderHeader *hdr, EslBmpInfo *info) {
    info->data_offset = 0;
    info->row_stride = hdr->row_stride;
    info->width = hdr->width;
    info->height = hdr->height;
    info->bpp = hdr->planes;
    info->top_down = true;
}

static void esl_send_wifi_planes(EslSession *sess, const EslTarget *target,
                                 const uint8_t *planes, size_t plane_len,
                                 const EslWifiRenderHeader *hdr, uint8_t page) {
    EslBmpInfo info;
    esl_wifi_planes_as_bmp(hdr, &info);

    const bool is_color26 = tagtinker_profile_needs_wh_swap(&target->profile);

    drawMainBorderWithTitle(ESL_UI_APP_NAME);
    displayTextLine("Encoding...");

    TagTinkerImagePayload payload;
    bool encoded = false;
    uint16_t out_w = 0;
    uint16_t out_h = 0;

    if (is_color26) {
        EslColor26BmpCtx ctx = {planes, plane_len, &info};
        const size_t total = (size_t)TAGTINKER_COLOR26_WIRE_W *
                             TAGTINKER_COLOR26_WIRE_H * 2U;
        encoded = tagtinker_encode_fn_payload(esl_color26_bmp_pixel, &ctx, total,
                                              TagTinkerCompressionAuto, &payload);
    } else {
        out_w = target->profile.width;
        out_h = target->profile.height;
        const bool second_plane = (target->profile.color != TagTinkerTagColorMono);
        EslGenericBmpCtx ctx = {planes, plane_len, &info, out_w, out_h,
                                second_plane};
        const size_t total =
            (size_t)out_w * out_h * (second_plane ? 2U : 1U);
        encoded = tagtinker_encode_fn_payload(esl_generic_bmp_pixel, &ctx, total,
                                              TagTinkerCompressionAuto, &payload);
    }

    if (!encoded) {
        displayError("Encode failed", true);
        return;
    }

    checkIrTxPin();
    if (!esl_ir_init(bruceConfigPins.irTx)) {
        tagtinker_free_image_payload(&payload);
        displayError("IR init failed", true);
        return;
    }

    drawMainBorderWithTitle(ESL_UI_APP_NAME);
    displayTextLine("Sending, Esc aborts");
    delay(ESL_PRE_TX_SETTLE_MS);

    EslUiCtx ui = {false};
    EslTxOps ops = {ui_send, ui_settle, ui_aborted, ui_progress, &ui};
    esl_ir_set_abort_hook(ui_abort_poll, &ui);

    const bool ok =
        is_color26 ? esl_tx_send_color26(&ops, target->plid, &payload, page)
                   : esl_tx_send_generic(&ops, target->plid, &payload, page,
                                         out_w, out_h, 0u, 0u,
                                         sess->settings.data_frame_repeats);

    esl_ir_set_abort_hook(nullptr, nullptr);
    esl_ir_deinit();
    tagtinker_free_image_payload(&payload);

    if (ui.aborted) {
        displayWarning("Aborted", true);
    } else if (ok) {
        displaySuccess("Sent", true);
    } else {
        displayError("TX failed", true);
    }
}

static bool esl_wifi_prompt_params(const EslWifiPlugin *plugin,
                                   char values[ESL_WIFI_MAX_PARAMS][64]) {
    for (uint8_t i = 0; i < plugin->param_count; i++) {
        const EslWifiParam *sp = &plugin->params[i];
        strncpy(values[i], sp->default_value, 63);
        values[i][63] = '\0';

        if (sp->type == ESL_WIFI_PARAM_ENUM) {
            if (sp->option_count == 0) continue;
            int chosen = -1;
            int cur = 0;
            options.clear();
            for (uint8_t j = 0; j < sp->option_count; j++) {
                if (strcmp(values[i], sp->options[j]) == 0) cur = (int)j;
                options.push_back(
                    {sp->options[j], [j, &chosen]() { chosen = (int)j; }});
            }
            int sel = loopOptions(options, MENU_TYPE_SUBMENU, sp->label, cur);
            if (sel < 0) return false;
            if (chosen >= 0) {
                strncpy(values[i], sp->options[chosen], 63);
                values[i][63] = '\0';
            }
        } else if (sp->type == ESL_WIFI_PARAM_BOOL) {
            bool on = (values[i][0] == '1');
            options = {
                {"Off", [&]() { on = false; }},
                {"On", [&]() { on = true; }},
            };
            int sel = loopOptions(options, MENU_TYPE_SUBMENU, sp->label, on ? 1 : 0);
            if (sel < 0) return false;
            strcpy(values[i], on ? "1" : "0");
        } else if (sp->type == ESL_WIFI_PARAM_INT) {
            int32_t minv = sp->int_min;
            int32_t maxv = sp->int_max;
            if (maxv < minv) {
                int32_t tmp = minv;
                minv = maxv;
                maxv = tmp;
            }
            int32_t cur = atoi(values[i]);
            if (cur < minv) cur = minv;
            if (cur > maxv) cur = maxv;
            const int32_t range = maxv - minv + 1;
            if (range > 0 && range <= 32) {
                int32_t chosen = cur;
                options.clear();
                for (int32_t v = minv; v <= maxv; v++) {
                    options.push_back(
                        {String((int)v), [v, &chosen]() { chosen = v; }});
                }
                int sel = loopOptions(options, MENU_TYPE_SUBMENU, sp->label,
                                      (int)(cur - minv));
                if (sel < 0) return false;
                snprintf(values[i], 64, "%ld", (long)chosen);
            } else {
                String entered = num_keyboard(String((int)cur), 11, sp->label);
                if (entered == "\x1B") return false;
                int32_t v = entered.toInt();
                if (v < minv) v = minv;
                if (v > maxv) v = maxv;
                snprintf(values[i], 64, "%ld", (long)v);
            }
        } else {
            String entered = keyboard(values[i], 63, sp->label);
            if (entered == "\x1B") return false;
            strncpy(values[i], entered.c_str(), 63);
            values[i][63] = '\0';
        }
    }
    return true;
}

static uint8_t esl_wifi_accent_for(const EslTarget *target) {
    if (target->profile.color == TagTinkerTagColorYellow) return 2;
    if (target->profile.color == TagTinkerTagColorRed) return 1;
    return 0;
}

static void esl_wifi_run_plugin(EslSession *sess, const EslTarget *target,
                                const EslWifiPlugin *plugin) {
    char values[ESL_WIFI_MAX_PARAMS][64];
    memset(values, 0, sizeof values);
    if (!esl_wifi_prompt_params(plugin, values)) return;

    const char *keys[ESL_WIFI_MAX_PARAMS];
    const char *vals[ESL_WIFI_MAX_PARAMS];
    for (uint8_t i = 0; i < plugin->param_count; i++) {
        keys[i] = plugin->params[i].key;
        vals[i] = values[i];
    }

    uint16_t tw = target->profile.width;
    uint16_t th = target->profile.height;
    if (tagtinker_profile_needs_wh_swap(&target->profile)) {
        tw = TAGTINKER_COLOR26_GLASS_W;
        th = TAGTINKER_COLOR26_GLASS_H;
    }

    drawMainBorderWithTitle(plugin->name[0] ? plugin->name : "WiFi Plugins");
    displayTextLine("Loading...");

    EslWifiRenderHeader hdr;
    uint8_t *body = nullptr;
    size_t body_len = 0;
    if (!esl_wifi_client_fetch_render(plugin->id, tw, th, esl_wifi_accent_for(target),
                                      keys, vals, plugin->param_count, &hdr, &body,
                                      &body_len)) {
        return;
    }

    uint8_t page = tagtinker_profile_needs_wh_swap(&target->profile)
                       ? tagtinker_color26_resolve_page(0)
                       : 0;
    while (true) {
        bool tx = false;
        char page_lbl[16];
        snprintf(page_lbl, sizeof page_lbl, "Page %u", (unsigned)page);
        options.clear();
        options.push_back({page_lbl, [&]() {
                               uint8_t p = page;
                               if (esl_prompt_page(&p)) page = p;
                           }});
        options.push_back({ESL_TRANSMIT, [&]() { tx = true; }});
        int sel = loopOptions(options, MENU_TYPE_SUBMENU,
                              plugin->name[0] ? plugin->name : "WiFi Plugins");
        if (sel < 0) {
            free(body);
            return;
        }
        if (tx) {
            esl_send_wifi_planes(sess, target, body, body_len, &hdr, page);
            free(body);
            return;
        }
    }
}

static void esl_wifi_plugins(EslSession *sess, const EslTarget *target) {
    EslWifiManifest *manifest =
        (EslWifiManifest *)ps_malloc(sizeof(EslWifiManifest));
    if (manifest == nullptr) manifest = (EslWifiManifest *)malloc(sizeof(EslWifiManifest));
    if (manifest == nullptr) {
        displayError("plugin fetch failed", true);
        return;
    }
    memset(manifest, 0, sizeof *manifest);

    auto refresh = [&]() {
        drawMainBorderWithTitle("WiFi Plugins");
        displayTextLine("Loading plugins...");
        if (!esl_wifi_client_fetch_plugins(manifest)) {
            memset(manifest, 0, sizeof *manifest);
        }
    };
    refresh();

    while (true) {
        int plugin_idx = -1;
        bool do_setup = false;
        bool do_forget = false;
        bool do_refresh = false;

        char title[40];
        snprintf(title, sizeof title, "WiFi Plugins [%s]",
                 wifiConnected ? "OK" : "off");

        options.clear();
        if (manifest->count == 0) {
            options.push_back({"(no plugins yet)", [&]() { do_refresh = true; }});
        } else {
            for (uint8_t i = 0; i < manifest->count; i++) {
                options.push_back({manifest->plugins[i].name,
                                   [i, &plugin_idx]() { plugin_idx = (int)i; }});
            }
        }
        options.push_back({"WiFi Setup", [&]() { do_setup = true; }});
        options.push_back({"Forget WiFi", [&]() { do_forget = true; }});
        options.push_back({"Refresh Plugins", [&]() { do_refresh = true; }});

        int sel = loopOptions(options, MENU_TYPE_SUBMENU, title);
        if (sel < 0) {
            free(manifest);
            return;
        }
        if (do_setup) {
            wifiConnectMenu();
            continue;
        }
        if (do_forget) {
            wifiDisconnect();
            continue;
        }
        if (do_refresh) {
            refresh();
            continue;
        }
        if (plugin_idx >= 0 && (uint8_t)plugin_idx < manifest->count) {
            esl_wifi_run_plugin(sess, target, &manifest->plugins[plugin_idx]);
        }
    }
}

static void esl_show_tag_info(const EslTarget *t) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "--- Tag Info ---\n"
             "Model: %s\n"
             "Type: %u (%s)\n"
             "Size: %ux%u\n"
             "Color: %s\n"
             "Barcode:\n"
             "%s",
             t->profile.model_name ? t->profile.model_name : "Unknown",
             (unsigned)t->profile.type_code,
             esl_store_profile_kind_label(t->profile.kind),
             (unsigned)t->profile.width, (unsigned)t->profile.height,
             esl_store_profile_color_label(t->profile.color), t->barcode);

    ScrollableTextArea area = ScrollableTextArea(ESL_UI_APP_NAME);
    area.fromString(buf);
    area.show();
}

static void esl_rename_tag(EslSession *s, uint8_t idx) {
    EslTarget *t = &s->targets[idx];
    String entered = keyboard(t->name, ESL_STORE_NAME_LEN, "Target name:");
    if (entered == "\x1B") return;

    for (size_t i = 0; i < (size_t)entered.length(); i++) {
        char c = entered[i];
        if (c == '|' || c == '\r' || c == '\n') entered.setCharAt((unsigned)i, ' ');
    }

    if (entered.length() == 0) {
        esl_store_set_default_name(s, t);
    } else {
        strncpy(t->name, entered.c_str(), ESL_STORE_NAME_LEN);
        t->name[ESL_STORE_NAME_LEN] = '\0';
    }
    esl_fs_save_targets(s);
}

static bool esl_delete_tag(EslSession *s, uint8_t idx) {
    char body[96];
    snprintf(body, sizeof(body), "Delete %s and its\nsaved images?",
             esl_target_label(&s->targets[idx]));
    drawMainBorder(true);
    if (displayMessage(body, "Back", nullptr, "Delete", TFT_RED) != 1) return false;
    if (!esl_store_delete_target(s, idx)) return false;
    esl_fs_save_targets(s);
    return true;
}

static void esl_target_actions(EslSession *s, uint8_t idx) {
    s->selected_target = (int8_t)idx;
    while (true) {
        if (idx >= s->target_count) return;
        EslTarget *t = &s->targets[idx];
        bool deleted = false;

        options.clear();
        options.push_back({ESL_TARGET_ACTIONS_ALWAYS[0], [t]() { esl_show_tag_info(t); }});
        options.push_back({ESL_TARGET_ACTIONS_ALWAYS[1], [s, idx]() { esl_rename_tag(s, idx); }});
        if (esl_store_target_supports_graphics(t)) {
            options.push_back({ESL_TARGET_ACTIONS_GRAPHICS[0],
                               [s, t]() { esl_set_text(s, t); }});
            options.push_back({ESL_TARGET_ACTIONS_GRAPHICS[1],
                               [s, t]() { esl_set_image(s, t); }});
            options.push_back({ESL_TARGET_ACTIONS_GRAPHICS[2],
                               [s, t]() { esl_wifi_plugins(s, t); }});
        }
        options.push_back({ESL_TARGET_ACTIONS_TAIL[0], [t]() { esl_led_test(t); }});
        options.push_back({ESL_TARGET_ACTIONS_TAIL[1], [&]() {
                               if (esl_delete_tag(s, idx)) deleted = true;
                           }});

        int sel = loopOptions(options, MENU_TYPE_SUBMENU, esl_target_label(t));
        if (sel < 0 || deleted) return;
    }
}

static RFIDInterface *esl_new_rfid(void) {
    switch (bruceConfigPins.rfidModule) {
        case PN532_I2C_MODULE: return new PN532(PN532::CONNECTION_TYPE::I2C);
#ifdef M5STICK
        case PN532_I2C_SPI_MODULE: return new PN532(PN532::CONNECTION_TYPE::I2C_SPI);
#endif
        case PN532_SPI_MODULE: return new PN532(PN532::CONNECTION_TYPE::SPI);
        case RC522_SPI_MODULE: return new RFID2(false);
#if !defined(LITE_VERSION)
        case ST25R3916_SPI_MODULE: return new ST25R3916(ST25R3916::SPI_MODE);
        case ST25R3916_I2C_MODULE: return new ST25R3916(ST25R3916::I2C_MODE);
#endif
        case M5_RFID2_MODULE:
        default: return new RFID2();
    }
}

static void esl_free_rfid(RFIDInterface *rfid) {
    delete rfid;
    releaseI2CBus();
}

/* UL dump lines only: "Page N: AA BB CC DD". Classic 16-byte "Page N:"
 * lines and T4T NDEF text must not count as a finished Ultralight dump. */
static unsigned esl_parse_ul_pages(const String &dump, uint8_t pages[][4],
                                   unsigned cap) {
    unsigned highest = 0;
    bool any = false;
    int start = 0;
    while (start < (int)dump.length()) {
        int nl = dump.indexOf('\n', start);
        if (nl < 0) nl = (int)dump.length();
        String line = dump.substring(start, nl);
        line.trim();
        start = nl + 1;
        if (line.length() == 0) continue;

        unsigned n = 0, b0 = 0, b1 = 0, b2 = 0, b3 = 0;
        int consumed = 0;
        if (sscanf(line.c_str(), "Page %u: %x %x %x %x%n", &n, &b0, &b1, &b2,
                   &b3, &consumed) != 5) {
            continue;
        }
        const char *rest = line.c_str() + consumed;
        while (*rest == ' ' || *rest == '\t') rest++;
        if (*rest != '\0') continue;
        if (n >= cap) continue;
        pages[n][0] = (uint8_t)b0;
        pages[n][1] = (uint8_t)b1;
        pages[n][2] = (uint8_t)b2;
        pages[n][3] = (uint8_t)b3;
        if (!any || n + 1u > highest) highest = n + 1u;
        any = true;
    }
    return any ? highest : 0;
}

static void esl_nfc_popup(const char *header, const char *body) {
    drawMainBorderWithTitle(header);
    setPadCursor(1, 1);
    String rest = body;
    int start = 0;
    while (start < (int)rest.length()) {
        int nl = rest.indexOf('\n', start);
        if (nl < 0) nl = (int)rest.length();
        padprintln(rest.substring(start, nl));
        start = nl + 1;
    }
    while (!check(AnyKeyPress)) delay(10);
}

static void esl_scan_nfc(EslSession *s) {
    RFIDInterface *rfid = esl_new_rfid();
    if (!rfid->begin()) {
        displayError("RFID module not found!", true);
        esl_free_rfid(rfid);
        return;
    }

    drawMainBorderWithTitle("Scan NFC Tag");
    setPadCursor(1, 1);
    padprintln("Hold ESL tag");
    padprintln("to the NFC reader");

    bool cancelled = false;
    uint8_t pages[64][4];
    unsigned pages_read = 0;
    memset(pages, 0, sizeof(pages));

    while (true) {
        if (check(EscPress)) {
            cancelled = true;
            break;
        }
        /* PN532::read() returns SUCCESS on ISO14443A detect even when
         * read_data_blocks() failed (pageReadSuccess false). Keep polling
         * until a complete Ultralight dump is in hand, like Flipper. */
        if (rfid->read() != RFIDInterface::SUCCESS || !rfid->pageReadSuccess) {
            delay(100);
            continue;
        }
        memset(pages, 0, sizeof(pages));
        pages_read = esl_parse_ul_pages(rfid->strAllPages, pages, 64);
        if (pages_read < 11) {
            delay(100);
            continue;
        }
        break;
    }

    esl_free_rfid(rfid);

    if (cancelled) return;

    char barcode[18];
    memset(barcode, 0, sizeof(barcode));
    if (!esl_nfc_decode_ul_pages(pages, pages_read, barcode)) {
        esl_nfc_popup("Not an ESL tag", "Tag detected but\nno valid ESL data");
        return;
    }

    int idx = esl_store_ensure_target(s, barcode);
    if (idx < 0) {
        esl_nfc_popup("Decode Error", "Tag read but\nbarcode invalid");
        return;
    }
    esl_fs_save_targets(s);
    esl_target_actions(s, (uint8_t)idx);
}

static void esl_type_barcode(EslSession *s) {
    String entered = keyboard("", ESL_BARCODE_LEN, "Tag barcode (17 chars):");
    entered.trim();

    uint8_t plid[4] = {0};
    TagTinkerTagProfile profile;
    if (!esl_parse_typed_barcode(entered, plid, &profile)) return;

    int idx = esl_store_ensure_target(s, entered.c_str());
    if (idx < 0) return;
    esl_fs_save_targets(s);
    esl_target_actions(s, (uint8_t)idx);
}

static void esl_targeted_menu(EslSession *s) {
    while (true) {
        options.clear();
        options.push_back({ESL_TARGET_MENU_PREFIX[0], [s]() { esl_scan_nfc(s); }});
        options.push_back({ESL_TARGET_MENU_PREFIX[1], [s]() { esl_type_barcode(s); }});
        for (uint8_t i = 0; i < s->target_count; i++) {
            options.push_back({esl_target_label(&s->targets[i]),
                               [s, i]() { esl_target_actions(s, i); }});
        }
        int sel = loopOptions(options, MENU_TYPE_SUBMENU, ESL_MAIN_ITEMS[1]);
        if (sel < 0) return;
    }
}

static void esl_pick_startup_warning(EslSession *s) {
    options = {
        {"Off", [&]() { s->settings.show_startup_warning = false; }},
        {"On",  [&]() { s->settings.show_startup_warning = true; } },
    };
    int cur = s->settings.show_startup_warning ? 1 : 0;
    int sel = loopOptions(options, MENU_TYPE_SUBMENU, ESL_SETTINGS_ITEMS[0], cur);
    if (sel >= 0) esl_fs_save_settings(s);
}

static void esl_pick_frame_repeat(EslSession *s) {
    options.clear();
    for (uint8_t i = 1; i <= 10; i++) {
        options.push_back({String((int)i), [s, i]() { s->settings.data_frame_repeats = i; }});
    }
    int cur = (int)s->settings.data_frame_repeats - 1;
    if (cur < 0) cur = 0;
    if (cur > 9) cur = 9;
    int sel = loopOptions(options, MENU_TYPE_SUBMENU, ESL_SETTINGS_ITEMS[1], cur);
    if (sel >= 0) esl_fs_save_settings(s);
}

static void esl_settings_menu(EslSession *s) {
    while (true) {
        String warn = String(ESL_SETTINGS_ITEMS[0]) + "  " +
                      (s->settings.show_startup_warning ? "On" : "Off");
        String rep = String(ESL_SETTINGS_ITEMS[1]) + "  " +
                     String((unsigned)s->settings.data_frame_repeats);
        options = {
            {warn,                  [&]() { esl_pick_startup_warning(s); }},
            {rep,                   [&]() { esl_pick_frame_repeat(s); }   },
            {ESL_SETTINGS_ITEMS[2], [&]() {
                 s->recent_count = 0;
                 esl_fs_save_recents(s);
                 displayTextLine("Cleared!", true);
             }},
        };
        int sel = loopOptions(options, MENU_TYPE_SUBMENU, ESL_MAIN_ITEMS[2]);
        if (sel < 0) return;
    }
}

static void esl_about(void) {
    ScrollableTextArea area = ScrollableTextArea(ESL_UI_APP_NAME);
    const size_t n = sizeof(ESL_ABOUT_LINES) / sizeof(ESL_ABOUT_LINES[0]);
    for (size_t i = 0; i < n; i++) area.addLine(ESL_ABOUT_LINES[i]);
    area.show();
}

static void esl_main_menu(EslSession *s) {
    while (true) {
        options = {
            {ESL_MAIN_ITEMS[0], [&]() { esl_broadcast_menu(); }},
            {ESL_MAIN_ITEMS[1], [&]() { esl_targeted_menu(s); }},
            {ESL_MAIN_ITEMS[2], [&]() { esl_settings_menu(s); }},
            {ESL_MAIN_ITEMS[3], [&]() { esl_about(); }         },
        };
        int sel = loopOptions(options, MENU_TYPE_SUBMENU, ESL_UI_APP_NAME);
        if (sel < 0) return;
    }
}

void startTagTinker(void) {
    EslSession sess;
    esl_fs_load_session(&sess);

    if (sess.settings.show_startup_warning) {
        if (!esl_warning_pages()) {
            returnToMenu = true;
            return;
        }
    }

    esl_main_menu(&sess);
    returnToMenu = true;
}
