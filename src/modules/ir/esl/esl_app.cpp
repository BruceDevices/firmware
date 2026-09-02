#include "esl_app.h"

#include "esl_bmp.h"
#include "esl_ir_driver.h"
#include "esl_proto.h"
#include "esl_tx.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "modules/ir/TV-B-Gone.h"
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <SD.h>
#include <globals.h>

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

bool esl_prompt_target(uint8_t plid[4], TagTinkerTagProfile *profile) {
    String entered = keyboard("", ESL_BARCODE_LEN, "Tag barcode (17 chars):");
    entered.trim();

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

void startEslTx() {
    drawMainBorderWithTitle("ESL Image");

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

    drawMainBorderWithTitle("ESL Image");
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
