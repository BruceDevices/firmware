#include "esl_app.h"

#include "esl_ir_driver.h"
#include "esl_proto.h"
#include "esl_tx.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "modules/ir/TV-B-Gone.h"
#include <Arduino.h>
#include <globals.h>

#define ESL_BARCODE_LEN 17

/* Settle before blasting IR, mirroring the upstream app's pre-TX pause. */
#define ESL_PRE_TX_SETTLE_MS 500

/* Latches once pressed so the outcome can be reported as an abort rather than
 * a transmit failure. */
static bool s_ui_aborted = false;

/* Polled by the driver between repeats so a long burst is interruptible while
 * transmit stays synchronous on this task. */
static bool ui_abort_poll(void *ctx) {
    (void)ctx;
    if (!s_ui_aborted && check(EscPress)) s_ui_aborted = true;
    return s_ui_aborted;
}

/* Prompts for the tag barcode and derives its address and profile. The barcode
 * is always entered by the user — upstream never compiles one in. */
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

void startEslTx() {
    drawMainBorderWithTitle("ESL Image");

    uint8_t plid[4] = {0};
    TagTinkerTagProfile profile;
    if (!esl_prompt_target(plid, &profile)) {
        returnToMenu = true;
        return;
    }

    checkIrTxPin();
    if (!esl_ir_init(bruceConfigPins.irTx)) {
        displayError("IR init failed", true);
        returnToMenu = true;
        return;
    }

    /* Install the abort poll before transmitting so the status line below is
     * truthful: the driver checks this between repeats. */
    s_ui_aborted = false;
    esl_ir_set_abort_hook(ui_abort_poll, nullptr);

    drawMainBorderWithTitle("ESL Image");
    displayTextLine("Wake frames, Esc aborts");
    delay(ESL_PRE_TX_SETTLE_MS);

    /* M0 sends only the wake frame, so the waveform can be measured against a
     * real addressed frame. Image payloads arrive in Task 7. */
    uint8_t frame[TAGTINKER_MAX_FRAME_SIZE];
    const size_t len = tagtinker_make_wake_frame(frame, plid);
    const bool ok = esl_ir_transmit(frame, len, ESL_COLOR26_WAKE_REPEATS,
                                    ESL_FRAME_DELAY_UNITS);

    esl_ir_set_abort_hook(nullptr, nullptr);
    esl_ir_deinit();

    if (s_ui_aborted) {
        displayWarning("Aborted", true);
    } else if (ok) {
        displaySuccess("Wake sent", true);
    } else {
        displayError("TX failed", true);
    }
    returnToMenu = true;
}
