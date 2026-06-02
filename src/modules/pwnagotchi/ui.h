/*
Thanks to thoses developers for their projects:
* @7h30th3r0n3 : https://github.com/7h30th3r0n3/Evil-M5Core2 and https://github.com/7h30th3r0n3/PwnGridSpam
* @viniciusbo : https://github.com/viniciusbo/m5-palnagotchi
* @sduenasg : https://github.com/sduenasg/pio_palnagotchi

Thanks to @bmorcelli for his help doing a better code.
*/
#if !defined(LITE_VERSION)
#include "core/display.h"
#include "mood.h"
#include "pwngrid.h"

void initUi();
void wakeUp();
void drawMood(String face, String phrase, bool broken = false);
void drawTime();
void drawFooterData(
    uint8_t friends_run = 0, uint8_t friends_tot = 0, String last_friend_name = "", signed int rssi = -1000
);
void drawTopCanvas();
void drawBottomCanvas();
void updateUi(bool show_toolbars = false);
#endif
