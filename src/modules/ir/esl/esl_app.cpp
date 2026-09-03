#include "esl_app.h"

#include "esl_bmp.h"
#include "esl_fs.h"
#include "esl_ir_driver.h"
#include "esl_menu_labels.h"
#include "esl_proto.h"
#include "esl_tx.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/scrollableTextArea.h"
#include "core/sd_functions.h"
#include "modules/ir/TV-B-Gone.h"
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <SD.h>
#include <globals.h>
#include <stdio.h>
#include <string.h>

#define ESL_BARCODE_LEN 17
#define ESL_PRE_TX_SETTLE_MS 500

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
    progressHandler((int)done, total, "Sending ESL");
}

static const char *esl_target_label(const EslTarget *t) {
    return t->name[0] ? t->name : t->barcode;
}

/* Shared by + Type Barcode and the leftover BMP prompt. Segment tags are
 * valid here; image TX still refuses them after this returns. */
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

static bool esl_prompt_target(uint8_t plid[4], TagTinkerTagProfile *profile) {
    String entered = keyboard("", ESL_BARCODE_LEN, "Tag barcode (17 chars):");
    entered.trim();
    if (!esl_parse_typed_barcode(entered, plid, profile)) return false;
    /* Segment tags have no image page, matching supports_graphics upstream. */
    if (profile->kind != TagTinkerTagKindDotMatrix) {
        displayError("Tag has no image page", true);
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
    loopOptions(options, MENU_TYPE_SUBMENU, "Image page", (int)*page);
    if (chosen < 0) return false; /* user backed out */
    *page = (uint8_t)chosen;
    return true;
}

/* Picks a BMP from the SD card, falling back to LittleFS. The chosen
 * filesystem is returned with the path so the reader does not guess. */
struct EslPickedBmp {
    String path;
    fs::FS *fs;
};

static EslPickedBmp esl_pick_bmp() {
    if (setupSdCard()) {
        String path = loopSD(SD, true, "BMP", "/");
        if (path != "") return {path, &SD};
    }
    return {loopSD(LittleFS, true, "BMP", "/"), &LittleFS};
}

/* Reads the whole file into PSRAM from the filesystem the picker chose.
 * Caller frees. */
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

/* Task 4 rehomes this into Set Image. Kept file-static so the encode/send
 * path is not deleted; it is not the Infrared entry anymore. */
static void esl_image_tx_from_prompts(void) __attribute__((unused));
static void esl_image_tx_from_prompts(void) {
    drawMainBorderWithTitle(ESL_UI_APP_NAME);

    uint8_t plid[4] = {0};
    TagTinkerTagProfile profile;
    if (!esl_prompt_target(plid, &profile)) {
        returnToMenu = true;
        return;
    }

    const EslPickedBmp picked = esl_pick_bmp();
    if (picked.path == "") { /* user cancelled */
        returnToMenu = true;
        return;
    }

    const bool is_color26 = tagtinker_profile_needs_wh_swap(&profile);
    /* Color 2.6 default is resolve_page(0) so the picker starts off the
     * barcode page; generic stays on 0. The user's subsequent pick is raw. */
    uint8_t page = is_color26 ? tagtinker_color26_resolve_page(0) : 0;
    if (!esl_prompt_page(&page)) {
        returnToMenu = true;
        return;
    }

    const size_t cap = is_color26 ? ESL_COLOR26_BMP_MAX : ESL_GENERIC_BMP_MAX;

    /* --- Everything that touches the SD card happens before the IR pin is
     * claimed, because setup_ir_pin() may tear down the SD SPI bus. --- */
    size_t file_len = 0;
    uint8_t *file = esl_read_file(*picked.fs, picked.path, cap, &file_len);
    if (file == nullptr) {
        displayError("Cannot read BMP", true);
        returnToMenu = true;
        return;
    }

    EslBmpInfo info;
    if (!esl_bmp_parse(file, file_len, &info)) {
        free(file);
        displayError("Unsupported BMP", true);
        returnToMenu = true;
        return;
    }

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
            returnToMenu = true;
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
        out_w = profile.width;
        out_h = profile.height;
        const bool second_plane = (profile.color != TagTinkerTagColorMono);
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
        returnToMenu = true;
        return;
    }

    checkIrTxPin();
    if (!esl_ir_init(bruceConfigPins.irTx)) {
        tagtinker_free_image_payload(&payload);
        displayError("IR init failed", true);
        returnToMenu = true;
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
        is_color26
            ? esl_tx_send_color26(&ops, plid, &payload, page)
            : esl_tx_send_generic(&ops, plid, &payload, page, out_w, out_h, 0u,
                                  0u, ESL_GENERIC_DATA_REPEATS);

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
    returnToMenu = true;
}

static const char *const ESL_WARNING_LINES[][2] = {
    {"Educational tool for", "infrared ESL study."},
    {"Use only on tags", "you own or may test."},
    {"Unauthorized use", "may be illegal."},
    {"You are responsible", "for your actions."},
};

static void esl_noop(void) {}

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
    while (true) {
        options = {
            {ESL_BROADCAST_ITEMS[0], esl_noop},
            {ESL_BROADCAST_ITEMS[1], esl_noop},
        };
        int sel = loopOptions(options, MENU_TYPE_SUBMENU, ESL_MAIN_ITEMS[0]);
        if (sel < 0) return;
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
            options.push_back({ESL_TARGET_ACTIONS_GRAPHICS[0], esl_noop});
            options.push_back({ESL_TARGET_ACTIONS_GRAPHICS[1], esl_noop});
            options.push_back({ESL_TARGET_ACTIONS_GRAPHICS[2], esl_noop});
        }
        options.push_back({ESL_TARGET_ACTIONS_TAIL[0], esl_noop});
        options.push_back({ESL_TARGET_ACTIONS_TAIL[1], [&]() {
                               if (esl_delete_tag(s, idx)) deleted = true;
                           }});

        int sel = loopOptions(options, MENU_TYPE_SUBMENU, esl_target_label(t));
        if (sel < 0 || deleted) return;
    }
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
        options.push_back({ESL_TARGET_MENU_PREFIX[0], esl_noop});
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
