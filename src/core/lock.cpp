#include "lock.h"
#include "display.h"
#include "mykeyboard.h"
#include <globals.h>

static bool isPinAllDigits(const String &s) {
    for (char c : s) {
        if (c < '0' || c > '9') return false;
    }
    return true;
}

static void drawLockScreen() {
    drawMainBorderWithTitle("Locked");

    int midY = tftHeight / 2;

    setTftDisplay(0, 0, bruceConfig.priColor, FM, bruceConfig.bgColor);
    tft.drawCentreString("Device Locked", tftWidth / 2, midY - LH * FM, 1);

    setTftDisplay(0, 0, bruceConfig.secColor, FP, bruceConfig.bgColor);
    tft.drawCentreString("Press SELECT to unlock", tftWidth / 2, midY + LH * FM, 1);
}

void lockScreen() {
    int failedAttempts = 0;

    while (true) {
        drawLockScreen();

        // Wait for SELECT press
        while (!check(SelPress)) { delay(50); }

        // Prompt for PIN
        String entered;
        if (isPinAllDigits(bruceConfig.lockPin)) {
            entered = num_keyboard("", 16, "Enter PIN:", true);
        } else {
            entered = keyboard("", 16, "Enter PIN:", true);
        }

        // Cancelled or ESC — redraw and keep waiting
        if (entered == "\x1B" || entered.length() == 0) continue;

        if (entered == bruceConfig.lockPin) return; // Correct — unlock

        // Wrong PIN
        failedAttempts++;
        displayError("Wrong PIN");

        // Exponential backoff: 15s base, doubles each failure, cap 5 min
        if (failedAttempts >= 3) {
            int timeoutSec = 15 * (1 << (failedAttempts - 3)); // 15, 30, 60, 120, 240 ...
            if (timeoutSec > 300) timeoutSec = 300;

            unsigned long deadline = millis() + (unsigned long)timeoutSec * 1000;
            while (millis() < deadline) {
                int remaining = (int)((deadline - millis()) / 1000) + 1;
                drawMainBorderWithTitle("Locked");
                setTftDisplay(0, 0, TFT_RED, FM, bruceConfig.bgColor);
                tft.drawCentreString("Too many attempts!", tftWidth / 2, tftHeight / 2 - LH * FM, 1);
                setTftDisplay(0, 0, bruceConfig.secColor, FP, bruceConfig.bgColor);
                String msg = "Try again in " + String(remaining) + "s";
                tft.drawCentreString(msg, tftWidth / 2, tftHeight / 2 + LH * FM, 1);
                delay(1000);
            }
        }
    }
}
