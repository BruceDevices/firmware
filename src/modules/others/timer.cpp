/**
 * @file timer.cpp
 * @author Aleksei Gor (https://github.com/AlekseiGor) - Reviewed and optimized by Senape3000
 * @brief Timer - Optimized implementation
 * @version 0.2
 * @date 2026-01-25
 */

#include "timer.h"
#include "core/display.h"
#include "core/utils.h"
#include "modules/others/audio.h"

// Constants for better maintainability
#define DELAY_VALUE 150
#define INPUT_POLL_DELAY 50          // Delay between input checks to save CPU
#define DISPLAY_UPDATE_INTERVAL 1000 // Update display every second
#define MAX_HOURS 99
#define MAX_MINUTES 59
#define MAX_SECONDS 59

// Enum for better readability
enum SettingMode {
    SETTING_HOURS = 0,
    SETTING_MINUTES = 1,
    SETTING_SECONDS = 2,
    SETTING_SOUND = 3,
    SETTING_COMPLETE = 4
};

Timer::Timer() { setup(); }

Timer::~Timer() {
    tft.fillScreen(bruceConfig.bgColor);
    backToMenu();
}

void Timer::setup() {
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    playSoundOnFinish = true; // Default: sound enabled
    SettingMode settingMode = SETTING_HOURS;

    char timeString[12];

    tft.fillScreen(bruceConfig.bgColor);
    delay(DELAY_VALUE);

    // Setup loop: configure timer duration and options
    while (true) {
        // Format and display time string
        snprintf(timeString, sizeof(timeString), "%02d:%02d:%02d", hours % 100, minutes % 100, seconds % 100);

        drawMainBorderWithTitle("Set a timer", false);
        tft.setTextSize(fontSize);
        tft.drawCentreString(timeString, timerX, timerY, 1);

        // Update underline based on current setting mode
        clearUnderline();
        switch (settingMode) {
            case SETTING_HOURS:
                underlineHours();
                drawSoundOption(false); // Show but don't highlight
                break;
            case SETTING_MINUTES:
                underlineMinutes();
                drawSoundOption(false);
                break;
            case SETTING_SECONDS:
                underlineSeconds();
                drawSoundOption(false);
                break;
            case SETTING_SOUND:
                drawSoundOption(true); // Highlight sound option
                break;
            default: break;
        }

        // Handle ESC/BACK button - exit without starting timer
        if (check(EscPress)) { return; }

        // Handle increment (NextPress)
        if (check(NextPress)) {
            switch (settingMode) {
                case SETTING_HOURS: hours = (hours >= MAX_HOURS) ? 0 : hours + 1; break;
                case SETTING_MINUTES: minutes = (minutes >= MAX_MINUTES) ? 0 : minutes + 1; break;
                case SETTING_SECONDS: seconds = (seconds >= MAX_SECONDS) ? 0 : seconds + 1; break;
                case SETTING_SOUND:
                    playSoundOnFinish = !playSoundOnFinish; // Toggle
                    break;
                default: break;
            }
        }

        // Handle decrement (PrevPress)
        if (check(PrevPress)) {
            switch (settingMode) {
                case SETTING_HOURS: hours = (hours <= 0) ? MAX_HOURS : hours - 1; break;
                case SETTING_MINUTES: minutes = (minutes <= 0) ? MAX_MINUTES : minutes - 1; break;
                case SETTING_SECONDS: seconds = (seconds <= 0) ? MAX_SECONDS : seconds - 1; break;
                case SETTING_SOUND:
                    playSoundOnFinish = !playSoundOnFinish; // Toggle
                    break;
                default: break;
            }
        }

        // Handle selection (SelPress) - move to next field or start timer
        if (check(SelPress)) {
            settingMode = static_cast<SettingMode>(settingMode + 1);

            // If completed all fields and timer is valid, start countdown
            if (settingMode >= SETTING_COMPLETE) {
                if (hours > 0 || minutes > 0 || seconds > 0) {
                    duration = (hours * 3600 + minutes * 60 + seconds) * 1000;
                    break; // Exit setup, proceed to loop()
                }
                // If timer is 0:0:0, reset to first field
                settingMode = SETTING_HOURS;
            }
        }

        delay(INPUT_POLL_DELAY); // CPU saving delay
    }

    // Start the countdown
    loop();
}

bool Timer::responsiveDelay(unsigned long ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        if (check(SelPress) || check(EscPress)) {
            return true; // Button Pressed
        }
        delay(10); // Check every 10ms
    }
    return false; // Timeout
}

void Timer::loop() {
    unsigned long startMillis = millis();
    unsigned long lastUpdateMillis = 0;

    int lastSeconds = -1; // Track last displayed value to avoid unnecessary redraws
    char timeString[12];

    tft.fillScreen(bruceConfig.bgColor);

    // Countdown loop
    while (true) {
        unsigned long currentMillis = millis();

        // Calculate elapsed time (handles millis() overflow correctly)
        unsigned long elapsedMillis = currentMillis - startMillis;

        // Check for ESC/BACK button - exit timer
        if (check(EscPress)) { break; }

        // Check if timer has completed
        if (elapsedMillis >= duration) {
            // Play alarm pattern only if enabled
            if (playSoundOnFinish) { playAlarmPattern(); }
            break;
        }

        // Update display only once per second to reduce flicker and CPU usage
        if (currentMillis - lastUpdateMillis >= DISPLAY_UPDATE_INTERVAL) {
            unsigned long remainingMillis = duration - elapsedMillis;

            int seconds = (remainingMillis / 1000) % 60;
            int minutes = (remainingMillis / 60000) % 60;
            int hours = (remainingMillis / 3600000) % 100;

            // Only redraw if the seconds value changed
            if (seconds != lastSeconds) {
                snprintf(timeString, sizeof(timeString), "%02d:%02d:%02d", hours, minutes, seconds);

                // Calculate optimal font size based on display width
                uint8_t f_size = 4;
                for (uint8_t i = 4; i > 0; i--) {
                    if (i * LW * 8 < (tftWidth - BORDER_PAD_X * 2)) {
                        f_size = i;
                        break;
                    }
                }

                drawMainBorder(false);
                tft.setTextSize(f_size);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                tft.drawCentreString(timeString, timerX, timerY, 1);

                lastSeconds = seconds;
            }

            lastUpdateMillis = currentMillis;
        }

        delay(INPUT_POLL_DELAY); // CPU saving delay
    }
}

void Timer::playAlarmPattern() {
    // Display "TIME'S UP!" message
    tft.fillScreen(bruceConfig.bgColor);
    drawMainBorderWithTitle("Timer finished!", false);

    tft.setTextSize(2);
    tft.setTextColor(TFT_RED, bruceConfig.bgColor);
    tft.drawCentreString("TIME'S UP!", timerX, timerY - LH, 1);

    tft.setTextSize(1);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawCentreString("Press SEL or BACK to stop", timerX, timerY + (2 * LH), 1);

    // Alarm pattern loop - continues until user stops it
    while (true) {
        // Check if user wants to stop the alarm
        if (check(SelPress) || check(EscPress)) { break; }

        // Pattern:
        _tone(2000, 1000);

        if (check(SelPress) || check(EscPress)) { break; }
        if (responsiveDelay(100)) { break; }

        _tone(6000, 1000);

        if (check(SelPress) || check(EscPress)) { break; }
        if (responsiveDelay(50)) { break; }

        _tone(6000, 1000);
        if (check(SelPress) || check(EscPress)) { break; }
        if (responsiveDelay(800)) { break; }
    }
}

void Timer::clearUnderline() {
    tft.drawLine(BORDER_PAD_X, underlineY, tftWidth - BORDER_PAD_X, underlineY, bruceConfig.bgColor);
}

void Timer::underlineHours() {
    tft.drawLine(
        timerX - (4 * LW * fontSize),
        underlineY,
        timerX - (2 * LW * fontSize),
        underlineY,
        bruceConfig.priColor
    );
}

void Timer::underlineMinutes() {
    tft.drawLine(
        timerX - (LW * fontSize), underlineY, timerX + (LW * fontSize), underlineY, bruceConfig.priColor
    );
}

void Timer::underlineSeconds() {
    tft.drawLine(
        timerX + (2 * LW * fontSize),
        underlineY,
        timerX + (4 * LW * fontSize),
        underlineY,
        bruceConfig.priColor
    );
}

void Timer::drawSoundOption(bool highlight) {
    int optionY = underlineY + (2 * LH); // Position below timer

    tft.setTextSize(1);

    // Choose colors based on highlight state
    uint16_t textColor = highlight ? bruceConfig.priColor : TFT_DARKGREY;
    uint16_t statusColor = playSoundOnFinish ? TFT_GREEN : TFT_RED;

    // Clear the line first
    tft.fillRect(BORDER_PAD_X, optionY, tftWidth - BORDER_PAD_X * 2, LH + 2, bruceConfig.bgColor);

    // Build the option text
    char optionText[32];
    snprintf(optionText, sizeof(optionText), "Play sound: %s", playSoundOnFinish ? "ON" : "OFF");

    tft.setTextColor(textColor, bruceConfig.bgColor);
    tft.drawCentreString(optionText, timerX, optionY, 1);

    // Optional: Draw a small indicator if highlighted
    if (highlight) {
        int indicatorY = optionY + LH + 2;
        int textWidth = strlen(optionText) * LW;
        int startX = timerX - (textWidth / 2);
        int endX = timerX + (textWidth / 2);

        tft.drawLine(startX, indicatorY, endX, indicatorY, bruceConfig.priColor);
    }
}
