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

/* Prompts for the tag barcode and derives its address and profile. The barcode
 * is always entered by the user — upstream never compiles one in. */
bool esl_prompt_target(uint8_t plid[4], TagTinkerTagProfile *profile) {
    String entered = keyboard("", ESL_BARCODE_LEN, "Tag barcode (17 chars):");
    entered.trim();

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

    drawMainBorderWithTitle("ESL Image");
    displayTextLine("Wake frames, Esc aborts");
    delay(ESL_PRE_TX_SETTLE_MS);

    /* M0 sends only the wake frame, so the waveform can be measured against a
     * real addressed frame. Image payloads arrive in Task 7. */
    uint8_t frame[TAGTINKER_MAX_FRAME_SIZE];
    const size_t len = tagtinker_make_wake_frame(frame, plid);
    const bool ok = esl_ir_transmit(frame, len, ESL_COLOR26_WAKE_REPEATS,
                                    ESL_FRAME_DELAY_UNITS);

    esl_ir_deinit();

    if (ok) {
        displaySuccess("Wake sent", true);
    } else {
        displayError("TX failed", true);
    }
    returnToMenu = true;
}
