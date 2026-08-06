#include "bluefish_commands.h"
#include "modules/NRF24/nrf_jammer_detector.h"
#include "modules/ble/ble_spam_detector.h"
#include "modules/ir/ir_attack_detector.h"
#include "modules/rf/rf_jammer_detector.h"
#include <globals.h>

#if !defined(LITE_VERSION)
#include "modules/wifi/deauth_detect.h"
#endif

// All bluefish detectors are blocking for `seconds` (default 15s) — same bounded-scan convention
// `subghz scan` already uses — and report periodic + final status lines to serialDevice, so they
// work identically whether reached over USB serial or the BLE serial bridge.
static uint32_t bfDurationMs(cmd *c) {
    Command cmd(c);
    long seconds = cmd.getArgument("seconds").getValue().toInt();
    return (uint32_t)(seconds > 0 ? seconds : 15) * 1000;
}

uint32_t bfNrf24Callback(cmd *c) {
    jammer_detector_headless(bfDurationMs(c));
    return true;
}

uint32_t bfSubghzCallback(cmd *c) {
    rf_jammer_detector_headless(bfDurationMs(c));
    return true;
}

uint32_t bfIrCallback(cmd *c) {
    ir_attack_detector_headless(bfDurationMs(c));
    return true;
}

uint32_t bfBleSpamCallback(cmd *c) {
    ble_spam_detector_headless(bfDurationMs(c));
    return true;
}

#if !defined(LITE_VERSION)
uint32_t bfDeauthCallback(cmd *c) {
    deauth_detect_headless(bfDurationMs(c));
    return true;
}
#endif

void createBlueFishCommands(SimpleCLI *cli) {
    // e.g. bluefish nrf24 20    -> 2.4GHz jammer detector, headless, for 20s
    //      bluefish blespam    -> BLE spam detector, headless, default 15s
    Command cmd = cli->addCompositeCmd("bluefish");

    Command nrf24Cmd = cmd.addCommand("nrf24", bfNrf24Callback);
    nrf24Cmd.addPosArg("seconds", "15");

    Command subghzCmd = cmd.addCommand("subghz", bfSubghzCallback);
    subghzCmd.addPosArg("seconds", "15");

    Command irCmd = cmd.addCommand("ir", bfIrCallback);
    irCmd.addPosArg("seconds", "15");

    Command blespamCmd = cmd.addCommand("blespam", bfBleSpamCallback);
    blespamCmd.addPosArg("seconds", "15");

#if !defined(LITE_VERSION)
    Command deauthCmd = cmd.addCommand("deauth", bfDeauthCallback);
    deauthCmd.addPosArg("seconds", "15");
#endif
}
