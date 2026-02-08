// Senape3000 (Really Ugly) Audio Player UI

#include "audio_player.h"
#if defined(HAS_NS4168_SPKR)
#include "core/mykeyboard.h"
#include "core/settings.h"
#include "modules/others/audio.h"
#include <Arduino.h>

// ===== ENUMS & STRUCTS =====

enum IconType { ICON_PREV, ICON_PLAY, ICON_PAUSE, ICON_NEXT, ICON_VOL, ICON_LOOP_ON, ICON_LOOP_OFF };

struct PlayerState {
    bool isPlaying;
    bool isPaused;
    bool loopEnabled;
    String filename;
    unsigned long lastUpdate;
    int scrollOffset;
    unsigned long lastPauseAction;
};

struct UILayout {
    int MARGIN_X;
    int MARGIN_Y;
    int HEADER_HEIGHT;
    int DISPLAY_HEIGHT;
    int PROGRESS_HEIGHT;
    int CONTROLS_HEIGHT;
    int BUTTON_SIZE;
    int BUTTON_SPACING;
    int TEXT_SIZE_LARGE;
    int TEXT_SIZE_SMALL;

    void calculate() {
        bool isLarge = (tftWidth > 200 && tftHeight > 200);
        MARGIN_X = isLarge ? 10 : 5;
        MARGIN_Y = isLarge ? 10 : 5;
        HEADER_HEIGHT = isLarge ? 30 : 22;
        DISPLAY_HEIGHT = isLarge ? 40 : 30;
        PROGRESS_HEIGHT = isLarge ? 12 : 8;
        CONTROLS_HEIGHT = isLarge ? 70 : 50;
        BUTTON_SIZE = isLarge ? 50 : 36;
        TEXT_SIZE_LARGE = isLarge ? 2 : 1;
        TEXT_SIZE_SMALL = 1;
    }
};

// ===== HELPER FUNCTIONS =====

String formatTime(unsigned long ms) {
    unsigned long totalSeconds = ms / 1000;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, seconds);
    return String(buffer);
}

// Delete file extension in filename display
String extractFilename(const String &filepath) {
    int lastSlash = filepath.lastIndexOf('/');
    int lastDot = filepath.lastIndexOf('.');
    String name = filepath.substring(lastSlash + 1);
    // Remove extension
    if (lastDot > lastSlash) { name = name.substring(0, lastDot - lastSlash - 1); }
    return name;
}

// ===== GRAPHICS ENGINE =====

// Draw vector icons with better visibility
void drawVectorIcon(int x, int y, int size, IconType icon, uint16_t color) {
    int cx = x + size / 2;
    int cy = y + size / 2;
    int r = size / 2 - 2; // Larger icon radius for better visibility

    switch (icon) {
        case ICON_PLAY: {
            // Triangle play icon - shifted right for better centering
            int offset = -r / 4; // 3 More left
            tft.fillTriangle(
                cx - r / 2 + offset, cy - r, cx - r / 2 + offset, cy + r, cx + r + offset, cy, color
            );
            break;
        }

        case ICON_PAUSE: {
            // Two vertical bars - centered
            int barWidth = r / 1.5;
            int gap = r / 4;
            tft.fillRoundRect(cx - barWidth - gap / 2, cy - r, barWidth, r * 2, 2, color);
            tft.fillRoundRect(cx + gap / 2, cy - r, barWidth, r * 2, 2, color);
            break;
        }

        case ICON_PREV: {
            // Two triangles pointing left - shifted
            int offset = -r / 6;
            tft.fillTriangle(cx + offset, cy - r, cx + offset, cy + r, cx - r + offset, cy, color);
            tft.fillTriangle(cx + r + offset, cy - r, cx + r + offset, cy + r, cx + offset, cy, color);
            break;
        }

        case ICON_VOL: {
            // Speaker cone - better centered
            int offset = -r / 4;
            tft.fillTriangle(cx - r + offset, cy, cx + offset, cy - r, cx + offset, cy + r, color);
            tft.fillRect(cx - r + offset, cy - r / 2, r / 2, r, color);
            // Sound waves
            tft.drawCircle(cx + r / 3, cy, r / 2, color);
            tft.drawCircle(cx + r / 3, cy, r, color);
            // Redraw cone on top
            tft.fillTriangle(cx - r + offset, cy, cx + offset, cy - r, cx + offset, cy + r, color);
            break;
        }

        case ICON_LOOP_ON: {
            // Main circle
            tft.drawCircle(cx, cy, r, color);
            // Arrow tip to indicate loop
            tft.fillTriangle(cx + r - 3, cy - 3, cx + r + 3, cy - 3, cx + r, cy + 3, color);
            tft.setTextColor(color);
            tft.setTextSize(1);
            tft.setCursor(cx - 2, cy - 3);
            tft.print("1");
            break;
        }

        case ICON_LOOP_OFF: {
            // Same as LOOP_ON but in grey
            tft.drawCircle(cx, cy, r, TFT_DARKGREY);
            // Arrow tip to indicate loop direction
            tft.fillTriangle(cx + r - 3, cy - 3, cx + r + 3, cy - 3, cx + r, cy + 3, TFT_DARKGREY);
            tft.setTextColor(TFT_DARKGREY);
            tft.setTextSize(1);
            tft.setCursor(cx - 2, cy - 3);
            tft.print("1");
            break;
        }

        default: break;
    }
}

// Draw Button container + Icon with padding
void drawButton(int x, int y, int size, IconType icon, bool selected, bool active) {
    uint16_t bgColor = TFT_BLACK;
    uint16_t iconColor = active ? TFT_WHITE : TFT_DARKGREY;
    uint16_t borderColor = selected ? bruceConfig.priColor : TFT_DARKGREY;

    // Highlighted button has colored background
    if (selected) {
        bgColor = TFT_DARKGREY;
        iconColor = TFT_WHITE;
        tft.fillRoundRect(x, y, size, size, 6, bgColor);
        tft.drawRoundRect(x, y, size, size, 6, TFT_YELLOW);
    } else {
        tft.fillRoundRect(x, y, size, size, 6, bgColor);
        tft.drawRoundRect(x, y, size, size, 6, borderColor);
    }

    // Add padding between icon and button border for better visual spacing
    int iconPadding = 4;
    drawVectorIcon(x + iconPadding, y + iconPadding, size - 2 * iconPadding, icon, iconColor);
}

// Draw Styled Progress Bar
void drawProgressBar(int x, int y, int width, int height, unsigned long position, unsigned long duration) {
    uint16_t barBg = TFT_DARKGREY;
    uint16_t barFill = bruceConfig.priColor;

    // Background bar
    int barY = y + (height / 2) - 2;
    int barH = 4;
    tft.fillRect(x, barY, width, barH, barBg);

    if (duration > 0) {
        int fillWidth = (position * width) / duration;
        if (fillWidth > width) fillWidth = width;

        // Fill bar
        tft.fillRect(x, barY, fillWidth, barH, barFill);

        // Knob at the end
        if (fillWidth > 2) { tft.fillCircle(x + fillWidth, y + (height / 2), 4, TFT_WHITE); }
    }
}
// Volume control popup
bool showVolumeControl(uint8_t &currentVolume) {
    const int BAR_WIDTH = (tftWidth > 200) ? 60 : 40;
    const int BAR_HEIGHT = (tftHeight > 200) ? 140 : 100;
    const int BAR_X = (tftWidth - BAR_WIDTH) / 2;
    const int BAR_Y = (tftHeight - BAR_HEIGHT) / 2;

    uint8_t tempVolume = currentVolume;
    bool changed = false;

    auto drawVolumeBar = [&]() {
        // Dark overlay
        tft.fillRect(BAR_X - 2, BAR_Y - 2, BAR_WIDTH + 4, BAR_HEIGHT + 4, TFT_BLACK);

        // Container
        tft.drawRoundRect(BAR_X, BAR_Y, BAR_WIDTH, BAR_HEIGHT, 8, TFT_WHITE);

        // Speaker icon at top
        drawVectorIcon(BAR_X + (BAR_WIDTH - 20) / 2, BAR_Y + 10, 20, ICON_VOL, TFT_WHITE);

        // Vertical bar
        int innerBarH = BAR_HEIGHT - 60;
        int innerBarW = 10;
        int innerBarX = BAR_X + (BAR_WIDTH - innerBarW) / 2;
        int innerBarY = BAR_Y + 35;

        tft.fillRect(innerBarX, innerBarY, innerBarW, innerBarH, TFT_DARKGREY);

        int fillH = (tempVolume * innerBarH) / 100;
        uint16_t volColor = (tempVolume > 80) ? TFT_RED : bruceConfig.priColor;
        tft.fillRect(innerBarX, innerBarY + (innerBarH - fillH), innerBarW, fillH, volColor);

        // Volume percentage text
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextSize(1);
        tft.setCursor(BAR_X + 10, BAR_Y + BAR_HEIGHT - 15);
        tft.printf("%3d%%", tempVolume);
    };

    drawVolumeBar();

    while (true) {
        InputHandler();
        wakeUpScreen();

        bool needsRedraw = false;

        if (check(NextPress) || check(UpPress)) {
            if (tempVolume < 100) {
                tempVolume += 10;
                if (tempVolume > 100) tempVolume = 100;
                needsRedraw = true;
                changed = true;
            }
        }

        if (check(PrevPress) || check(DownPress)) {
            if (tempVolume >= 5) {
                tempVolume -= 5;
            } else {
                tempVolume = 0;
            }
            needsRedraw = true;
            changed = true;
        }

        if (needsRedraw) {
            setAudioPlaybackVolume(tempVolume);
            drawVolumeBar();
            delay(50);
        }

        if (check(SelPress)) {
            currentVolume = tempVolume;
            delay(200);
            return changed;
        }

        if (check(EscPress)) {
            if (changed) setAudioPlaybackVolume(currentVolume);
            delay(200);
            return false;
        }
    }
}
// ===== MAIN PLAYER UI =====

void musicPlayerUI(FS *fs, const String &filepath) {
    if (!fs || filepath.isEmpty() || !fs->exists(filepath)) {
        displayError("File Invalid", true);
        return;
    }

    UILayout ui;
    ui.calculate();

    PlayerState player;
    player.isPlaying = false;
    player.isPaused = false;
    player.loopEnabled = false;
    player.filename = extractFilename(filepath);
    player.lastUpdate = 0;
    player.scrollOffset = 0;
    player.lastPauseAction = 0;

    enum ControlButton { BTN_PREV = 0, BTN_PLAY = 1, BTN_VOLUME = 2, BTN_LOOP = 3, BTN_COUNT = 4 };
    int selectedButton = BTN_PLAY;

    unsigned long currentPosition = 0;
    unsigned long duration = 0;
    uint8_t currentVolume = bruceConfig.soundVolume;

    // --- Draw functions ---

    auto drawHeader = [&]() {
        tft.fillRect(0, 0, tftWidth, ui.HEADER_HEIGHT, bruceConfig.priColor);
        tft.setTextColor(bruceConfig.bgColor, bruceConfig.priColor);
        tft.setTextSize(ui.TEXT_SIZE_LARGE);

        // Center title
        String title = "AUDIO PLAYER (BETA)";
        int w = title.length() * 6 * ui.TEXT_SIZE_LARGE;
        tft.setCursor((tftWidth - w) / 2, (ui.HEADER_HEIGHT - (8 * ui.TEXT_SIZE_LARGE)) / 2);
        tft.print(title);
    };
    auto drawTrackInfo = [&]() {
        int y = ui.HEADER_HEIGHT + ui.MARGIN_Y;

        // Background area info
        tft.fillRect(0, y, tftWidth, ui.DISPLAY_HEIGHT, bruceConfig.bgColor);

        // Music icon box - larger for better visibility
        int iconSize = ui.DISPLAY_HEIGHT - 4;
        int boxSize = iconSize + 45; // Wider box towards right
        tft.drawRoundRect(ui.MARGIN_X, y + 2, boxSize, iconSize, 4, TFT_DARKGREY);
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(2);

        // Center "AUDIO" text in box
        String AudioFormatLabel = "AUDIO";              // TODO: Use file extension
        int textWidth = AudioFormatLabel.length() * 12; // textSize(2): 6 pixel base * 2 = 12 pixel per char
        int textHeight = 16;                            // textSize(2): 8 pixel base * 2 = 16 pixel
        int centerX = ui.MARGIN_X + (boxSize - textWidth) / 2 + 2; // Offset 5
        int centerY = y + 2 + (iconSize - textHeight) / 2;
        tft.setCursor(centerX, centerY);
        tft.print(AudioFormatLabel);

        // Scrolling Title
        int gap = 20; // Fixed gap from MP3 box
        int textX = ui.MARGIN_X + boxSize + gap;
        int maxChars = (tftWidth - textX - 5) / (6 * ui.TEXT_SIZE_LARGE);

        tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
        tft.setTextSize(ui.TEXT_SIZE_LARGE);

        String displayText = player.filename;
        if (displayText.length() > maxChars) {
            String scrolled = displayText + "   " + displayText;
            displayText = scrolled.substring(player.scrollOffset, player.scrollOffset + maxChars);

            // Scroll every 300ms
            if (millis() - player.lastUpdate > 300) {
                player.scrollOffset++;
                if (player.scrollOffset >= player.filename.length() + 3) player.scrollOffset = 0;
                player.lastUpdate = millis();
            }
        }

        tft.setCursor(textX, y + (ui.DISPLAY_HEIGHT / 2) - 8);
        tft.print(displayText);
    };

    auto drawTimers = [&]() {
        int y = ui.HEADER_HEIGHT + ui.MARGIN_Y + ui.DISPLAY_HEIGHT + 2;
        tft.setTextSize(ui.TEXT_SIZE_SMALL);
        tft.setTextColor(TFT_LIGHTGREY, bruceConfig.bgColor);

        String curr = formatTime(currentPosition);
        String tot = duration > 0 ? formatTime(duration) : "--:--";

        tft.setCursor(ui.MARGIN_X, y);
        tft.print(curr);

        String totalStr = tot;
        int w = totalStr.length() * 6;
        tft.setCursor(tftWidth - ui.MARGIN_X - w, y);
        tft.print(totalStr);
    };
    auto drawControls = [&]() {
        int startY = tftHeight - ui.CONTROLS_HEIGHT - ui.MARGIN_Y;
        int totalWidth = tftWidth - (2 * ui.MARGIN_X);
        int gap = (totalWidth - (BTN_COUNT * ui.BUTTON_SIZE)) / (BTN_COUNT - 1);
        int x = ui.MARGIN_X;

        // PREV button
        drawButton(x, startY, ui.BUTTON_SIZE, ICON_PREV, selectedButton == BTN_PREV, true);
        x += ui.BUTTON_SIZE + gap;

        // PLAY/PAUSE button
        IconType ppIcon = (player.isPlaying && !player.isPaused) ? ICON_PAUSE : ICON_PLAY;
        drawButton(x, startY, ui.BUTTON_SIZE, ppIcon, selectedButton == BTN_PLAY, true);
        x += ui.BUTTON_SIZE + gap;

        // VOLUME button
        drawButton(x, startY, ui.BUTTON_SIZE, ICON_VOL, selectedButton == BTN_VOLUME, true);
        x += ui.BUTTON_SIZE + gap;

        // LOOP button
        IconType loopIcon = player.loopEnabled ? ICON_LOOP_ON : ICON_LOOP_OFF;
        drawButton(x, startY, ui.BUTTON_SIZE, loopIcon, selectedButton == BTN_LOOP, true);
    };

    // Initial Draw
    tft.fillScreen(bruceConfig.bgColor);
    drawHeader();
    drawTrackInfo();
    drawTimers();
    drawProgressBar(
        ui.MARGIN_X,
        ui.HEADER_HEIGHT + ui.DISPLAY_HEIGHT + 20,
        tftWidth - 2 * ui.MARGIN_X,
        ui.PROGRESS_HEIGHT,
        0,
        1
    );
    drawControls();

    unsigned long lastPosUpdate = 0;
    // MAIN LOOP
    while (true) {
        InputHandler();
        wakeUpScreen();

        bool controlsNeedRedraw = false;
        bool progressNeedsRedraw = false;
        bool infoNeedsRedraw = false;

        AudioPlaybackInfo info = getAudioPlaybackInfo();

        // Playback state check
        bool actualPlaying = (info.state == PLAYBACK_PLAYING);
        if (player.isPlaying != actualPlaying && !player.isPaused) {
            // State change detected externally (e.g. end of track)
        }

        // Update position more smoothly (every 200ms)
        if (millis() - lastPosUpdate > 200) {
            if (currentPosition != info.position || info.duration != duration) {
                currentPosition = info.position;
                duration = info.duration;
                progressNeedsRedraw = true;
            }

            // Scroll text logic
            if (player.filename.length() > 15) infoNeedsRedraw = true;
            lastPosUpdate = millis();
        }

        // Loop check
        if (player.loopEnabled && info.state == PLAYBACK_IDLE && player.isPlaying && !player.isPaused) {
            playAudioFile(fs, filepath, PLAYBACK_ASYNC);
            controlsNeedRedraw = true;
        }

        // INPUT HANDLING
        if (check(PrevPress)) {
            selectedButton = (selectedButton - 1 + BTN_COUNT) % BTN_COUNT;
            controlsNeedRedraw = true;
        }

        if (check(NextPress)) {
            selectedButton = (selectedButton + 1) % BTN_COUNT;
            controlsNeedRedraw = true;
        }
        if (check(SelPress)) {
            // Anti-bounce for play button
            if (selectedButton == BTN_PLAY && millis() - player.lastPauseAction < 300) {
                // Ignore rapid presses
            } else {
                switch (selectedButton) {
                    case BTN_PREV:
                        stopAudioPlayback();
                        delay(50);
                        playAudioFile(fs, filepath, PLAYBACK_ASYNC);
                        player.isPlaying = true;
                        player.isPaused = false;
                        controlsNeedRedraw = true;
                        break;

                    case BTN_PLAY:
                        if (player.isPlaying && !player.isPaused) {
                            // Pause
                            pauseAudioPlayback();
                            player.isPaused = true;
                        } else if (player.isPaused) {
                            // Restart from pause pauseAudioPlayback (toggle)
                            pauseAudioPlayback();
                            player.isPaused = false;
                        } else {
                            // Play New File
                            playAudioFile(fs, filepath, PLAYBACK_ASYNC);
                            player.isPlaying = true;
                            player.isPaused = false;
                        }
                        player.lastPauseAction = millis();
                        controlsNeedRedraw = true;
                        break;

                    case BTN_VOLUME:
                        showVolumeControl(currentVolume);
                        tft.fillScreen(bruceConfig.bgColor); // Full redraw after popup
                        drawHeader();
                        infoNeedsRedraw = true;
                        progressNeedsRedraw = true;
                        controlsNeedRedraw = true;
                        break;

                    case BTN_LOOP:
                        player.loopEnabled = !player.loopEnabled;
                        controlsNeedRedraw = true;
                        break;
                }
            }
        }
        if (check(EscPress)) {
            stopAudioPlayback();
            break;
        }

        // REDRAWS
        if (infoNeedsRedraw) drawTrackInfo();

        if (progressNeedsRedraw) {
            drawTimers();
            drawProgressBar(
                ui.MARGIN_X,
                ui.HEADER_HEIGHT + ui.DISPLAY_HEIGHT + 20,
                tftWidth - 2 * ui.MARGIN_X,
                ui.PROGRESS_HEIGHT,
                currentPosition,
                duration
            );
        }

        if (controlsNeedRedraw) drawControls();

        delay(30); // Loop for responsive input
    }

    tft.fillScreen(bruceConfig.bgColor);
}
#endif
