#include "frank_vader.h"

#include "core/display.h"

namespace {

constexpr int FV_SPRITE_W = 80;
constexpr int FV_SPRITE_H = 80;
constexpr uint8_t FV_FRAME_DELAY_MS = 220;

void fv_draw_character(tft_sprite &fvSprite, uint8_t frame) {
    const uint16_t helmet = tft.color565(28, 30, 38);
    const uint16_t helmetShade = tft.color565(16, 18, 24);
    const uint16_t skin = tft.color565(126, 142, 122);
    const uint16_t skinShade = tft.color565(92, 108, 90);
    const uint16_t shadow = TFT_DARKGREY;
    const uint16_t outline = bruceConfig.priColor;
    const uint16_t eye = (frame & 1) ? TFT_ORANGE : TFT_RED;
    const uint16_t eyeGlow = (frame & 1) ? TFT_YELLOW : TFT_RED;

    fvSprite.fillScreen(TFT_TRANSPARENT);

    // Cape and shoulders.
    fvSprite.fillTriangle(10, 80, 27, 57, 40, 72, helmetShade);
    fvSprite.fillTriangle(70, 80, 53, 57, 40, 72, helmetShade);
    fvSprite.fillTriangle(10, 80, 40, 72, 40, 80, helmet);
    fvSprite.fillTriangle(70, 80, 40, 72, 40, 80, helmet);

    // Neck and the two Frankenstein bolts.
    fvSprite.fillRect(32, 52, 16, 12, skinShade);
    fvSprite.fillRect(25, 54, 7, 5, TFT_CYAN);
    fvSprite.fillRect(48, 54, 7, 5, TFT_CYAN);
    fvSprite.drawFastVLine(28, 59, 4, shadow);
    fvSprite.drawFastVLine(52, 59, 4, shadow);

    // Square Frankenstein jaw, then the Vader-style helmet over it.
    fvSprite.fillRoundRect(22, 34, 36, 26, 5, skin);
    fvSprite.fillRoundRect(24, 46, 32, 12, 3, skinShade);
    fvSprite.fillEllipse(40, 23, 22, 18, helmet);
    fvSprite.fillRect(18, 23, 44, 17, helmet);
    fvSprite.fillTriangle(18, 28, 28, 41, 18, 41, helmet);
    fvSprite.fillTriangle(62, 28, 52, 41, 62, 41, helmet);
    fvSprite.drawRoundRect(17, 22, 46, 25, 8, outline);
    fvSprite.drawLine(20, 25, 29, 39, shadow);
    fvSprite.drawLine(60, 25, 51, 39, shadow);

    // Helmet V, glowing eyes, nose, and square mouth grille.
    fvSprite.drawLine(31, 14, 40, 29, outline);
    fvSprite.drawLine(49, 14, 40, 29, outline);
    fvSprite.fillRect(29, 37, 9, 4, eye);
    fvSprite.fillRect(42, 37, 9, 4, eye);
    fvSprite.fillCircle(33, 39, 1, eyeGlow);
    fvSprite.fillCircle(47, 39, 1, eyeGlow);
    fvSprite.fillTriangle(37, 41, 43, 41, 40, 46, skinShade);
    fvSprite.fillRect(31, 49, 18, 2, shadow);
    fvSprite.drawFastVLine(34, 48, 4, skin);
    fvSprite.drawFastVLine(38, 48, 4, skin);
    fvSprite.drawFastVLine(42, 48, 4, skin);
    fvSprite.drawFastVLine(46, 48, 4, skin);

    // Chest light and a small breathing pulse.
    fvSprite.fillCircle(40, 68, (frame & 2) ? 6 : 4, eye);
    fvSprite.fillCircle(40, 68, 2, eyeGlow);
}

} // namespace

void fv_show_profile() {
    displayInfo("Profile: Frank Vader", true);
    displayInfo(String("Edition: ") + FV_EDITION, true);
    displayInfo(String("Build: ") + FV_VERSION, true);
    displayWarning("Authorized use only", true);
}

void fv_show_character() {
    while (check(AnyKeyPress)) vTaskDelay(10 / portTICK_PERIOD_MS);

    drawMainBorderWithTitle("FV Character");
    printCenterFootnote("Any key: back");

    tft_sprite fvSprite(&tft);
    if (fvSprite.createSprite(FV_SPRITE_W, FV_SPRITE_H) == nullptr) {
        displayWarning("FV sprite unavailable", false);
        return;
    }

    const int x = tftWidth > FV_SPRITE_W ? (tftWidth - FV_SPRITE_W) / 2 : 0;
    const int y = tftHeight > FV_SPRITE_H ? (tftHeight - FV_SPRITE_H) / 2 : 0;

    uint8_t frame = 0;
    do {
        fv_draw_character(fvSprite, frame);
        fvSprite.pushSprite(x, y, TFT_TRANSPARENT);
        frame = (frame + 1) & 0x03;
        delay(FV_FRAME_DELAY_MS);
    } while (!check(AnyKeyPress));

    fvSprite.deleteSprite();
}
