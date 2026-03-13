/*
Thanks to thoses developers for their projects:
* @7h30th3r0n3 : https://github.com/7h30th3r0n3/Evil-M5Core2 and https://github.com/7h30th3r0n3/PwnGridSpam
* @viniciusbo : https://github.com/viniciusbo/m5-palnagotchi
* @sduenasg : https://github.com/sduenasg/pio_palnagotchi

Thanks to @bmorcelli for his help doing a better code.
*/
#if !defined(LITE_VERSION)
#include "ui.h"
#include "../wifi/sniffer.h"

#define ROW_SIZE 40
#define PADDING 10

int32_t display_w;
int32_t display_h;
int32_t canvas_h;
int32_t canvas_center_x;
int32_t canvas_top_h;
int32_t canvas_bot_h;
int32_t canvas_peers_menu_h;
int32_t canvas_peers_menu_w;

uint8_t menu_current_cmd = 0;
uint8_t menu_current_opt = 0;

void initUi() {
    tft.setTextSize(1);
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextColor(bruceConfig.priColor);

    display_w = tftWidth;
    display_h = tftHeight;
    canvas_h = display_h * .8;
    canvas_center_x = display_w / 2;
    canvas_top_h = display_h * .1;
    canvas_bot_h = display_h * .9;
    canvas_peers_menu_h = display_h * .8;
    canvas_peers_menu_w = display_w * .8;
}

String getRssiBars(signed int rssi) {
    String rssi_bars = "";

    if (rssi != -1000) {
        if (rssi >= -67) {
            rssi_bars = "[####]";
        } else if (rssi >= -70) {
            rssi_bars = "[### ]";
        } else if (rssi >= -80) {
            rssi_bars = "[##  ]";
        } else {
            rssi_bars = "[#   ]";
        }
    }

    return rssi_bars;
}

void drawTime() {
    tft.drawPixel(0, 0, 0);
    tft.fillRect(80, 0, display_w, canvas_top_h - 3, bruceConfig.bgColor);
    tft.setTextDatum(TR_DATUM);
    unsigned long ellapsed = millis() / 1000;
    int8_t h = ellapsed / 3600;
    int sr = ellapsed % 3600;
    int8_t m = sr / 60;
    int8_t s = sr % 60;
    char right_str[50] = "UPS 0%  UP 00:00:00";
    sprintf(right_str, "UPS %i%% UP %02d:%02d:%02d", getBattery(), h, m, s);
    tft.drawString(right_str, display_w, 3);
}

void drawFooterData(uint8_t friends_run, uint8_t friends_tot, String last_friend_name, signed int rssi) {
    tft.drawPixel(0, 0, 0);
    tft.fillRect(0, canvas_bot_h + 1, display_w - 50, canvas_bot_h + 10, bruceConfig.bgColor);
    tft.setTextSize(1);
    tft.setTextColor(bruceConfig.priColor);
    tft.setTextDatum(TL_DATUM);

    String rssi_bars = getRssiBars(rssi);
    String stats = "FRND 0 (0)";
    if (friends_run > 0) {
        stats = "FRND " + String(friends_run) + " (" + String(friends_tot) + ")" + " [" +
                last_friend_name.substring(0, 13) + "] " + rssi_bars;
    }

    tft.drawString(stats, 0, canvas_bot_h + 5);
}

void updateUi(bool show_toolbars) {
    uint8_t mood_id = getCurrentMoodId();
    String mood_face = getCurrentMoodFace();
    String mood_phrase = getCurrentMoodPhrase();
    bool mood_broken = isCurrentMoodBroken();

    // Draw header and footer
    if (show_toolbars) {
        drawTopCanvas();
        if (tftHeight > 150) drawTime();
        drawFooterData(
            getPwngridRunTotalPeers(),
            getPwngridTotalPeers(),
            getPwngridLastFriendName(),
            getPwngridClosestRssi()
        );
    }

    // Draw mood
    drawMood(mood_face, mood_phrase, mood_broken);

#if defined(HAS_TOUCH)
    TouchFooter();
#endif
}

void drawTopCanvas() {
    // prepare canvas
    tft.setTextSize(1);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextDatum(TL_DATUM);
    char buffer[32];
    sprintf(buffer, "CH %02d, HS %d", ch, num_HS);
    // draw screen
    tft.drawPixel(0, 0, 0);
    tft.fillRect(0, 0, display_w, canvas_top_h, bruceConfig.bgColor);
    tft.drawString(buffer, 0, 3);
    tft.drawLine(0, canvas_top_h - 1, display_w, canvas_top_h - 1, bruceConfig.priColor);
}

void drawBottomCanvas() {
    // prepare canvas
    tft.setTextSize(1);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextDatum(TR_DATUM);
    // draw screen
    tft.drawPixel(0, 0, 0);
    tft.fillRect(0, canvas_bot_h, display_w, canvas_bot_h + 10, bruceConfig.bgColor);
    tft.drawString("NOT AI", display_w, canvas_bot_h + 5);
    tft.drawLine(0, canvas_bot_h, display_w, canvas_bot_h, bruceConfig.priColor);
}

void drawMood(String face, String phrase, bool broken) {
    // prepare canvas
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FG + 1);
    tft.setTextDatum(MC_DATUM);
    // draw screen
    tft.drawPixel(0, 0, 0);
    tft.fillRect(0, canvas_top_h + 10, display_w, canvas_bot_h - 40, bruceConfig.bgColor);
    tft.drawCentreString(face, canvas_center_x, canvas_h / 3, SMOOTH_FONT);
    // prepare canvas
    tft.setTextDatum(BC_DATUM);
    tft.setTextSize(1);
    // draw screen
    tft.drawPixel(0, 0, 0);
    tft.drawCentreString(phrase, canvas_center_x, canvas_h - 30, SMOOTH_FONT);
}
#endif
