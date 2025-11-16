#include "emergencyReboot.h"

#include "display.h"
#include "powerSave.h"
#include <ESP.h>
#include <globals.h>
#include <vector>

namespace {
constexpr uint32_t kHoldDurationMs = 10000;
constexpr uint32_t kWarningDelayMs = 3000;
constexpr uint32_t kReleaseGraceMs = 200; // debounce to ignore brief drops

struct EmergencyRebootState {
    bool backHeld = false;
    bool overlayVisible = false;
    uint32_t pressStartMs = 0;
    uint32_t lastSeenPressMs = 0;
    uint32_t lastShownSeconds = 0;
#ifdef HAS_SCREEN
    std::vector<uint16_t> overlayBackup;
#endif
} state;

#ifdef HAS_SCREEN
struct OverlayRect {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
};

OverlayRect overlayRect() {
    return OverlayRect{10, static_cast<int16_t>(tftHeight / 2 - 13), static_cast<int16_t>(tftWidth - 20), 26};
}

void captureOverlayArea(const OverlayRect &rect) {
    state.overlayBackup.resize(rect.w * rect.h);
    tft.readRect(rect.x, rect.y, rect.w, rect.h, state.overlayBackup.data());
}

void restoreOverlayArea(const OverlayRect &rect) {
    if (!state.overlayBackup.empty()) {
        tft.pushImage(rect.x, rect.y, rect.w, rect.h, state.overlayBackup.data());
    } else {
        tft.fillRoundRect(rect.x, rect.y, rect.w, rect.h, 7, bruceConfig.bgColor);
    }
    state.overlayBackup.clear();
}

void drawCountdown(uint32_t remainingSeconds) {
    if (isSleeping) sleepModeOff();
    else if (isScreenOff || dimmer) wakeUpScreen();

    previousMillis = millis(); // keep power-save timers alive while holding

    const auto rect = overlayRect();
    if (!state.overlayVisible) captureOverlayArea(rect);

    uint8_t oldTextSize = tft.textsize;
    uint32_t oldTextColor = tft.textcolor;
    uint32_t oldBgColor = tft.textbgcolor;

    tft.fillRoundRect(rect.x, rect.y, rect.w, rect.h, 7, TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.setTextSize(FM);

    char buffer[40];
    snprintf(buffer, sizeof(buffer), "Reboot in %lus", static_cast<unsigned long>(remainingSeconds));
    tft.drawCentreString(buffer, tftWidth / 2, rect.y + (rect.h / 2) - 8);

    tft.setTextSize(oldTextSize);
    tft.setTextColor(oldTextColor, oldBgColor);

    state.overlayVisible = true;
}
#endif

void triggerReboot() {
    state.backHeld = false;
#ifdef HAS_SCREEN
    state.overlayVisible = false;
    state.overlayBackup.clear();
#endif
    ESP.restart();
}
} // namespace

namespace EmergencyReboot {
void update(bool backPressed) {
    const uint32_t now = millis();

    if (backPressed) {
        const bool justStarted = !state.backHeld;
        state.lastSeenPressMs = now;
        if (!state.backHeld) {
            state.backHeld = true;
            state.pressStartMs = now;
            state.lastShownSeconds = 0;
        }
        previousMillis = now;
        if (justStarted) {
#ifdef HAS_SCREEN
            if (isSleeping) sleepModeOff();
            else if (isScreenOff || dimmer) wakeUpScreen();
#endif
        }

        const uint32_t elapsed = now - state.pressStartMs;
        if (elapsed >= kHoldDurationMs) {
            triggerReboot();
            return;
        }

#ifdef HAS_SCREEN
        if (elapsed >= kWarningDelayMs) {
            const uint32_t remainingSeconds = (kHoldDurationMs - elapsed + 999) / 1000;
            if (!state.overlayVisible || remainingSeconds != state.lastShownSeconds) {
                drawCountdown(remainingSeconds);
                state.lastShownSeconds = remainingSeconds;
            }
        }
#endif
        return;
    }

    if (!state.backHeld) return;

    // Treat as released only after a small grace window without a press.
    if (now - state.lastSeenPressMs > kReleaseGraceMs) {
        state.backHeld = false;
#ifdef HAS_SCREEN
        if (state.overlayVisible) {
            restoreOverlayArea(overlayRect());
            state.overlayVisible = false;
        }
#endif
    }
}

void cancel() {
    state.backHeld = false;
#ifdef HAS_SCREEN
    if (state.overlayVisible) {
        restoreOverlayArea(overlayRect());
        state.overlayVisible = false;
    }
    state.overlayBackup.clear();
#endif
}
} // namespace EmergencyReboot
