#include "pablo_mode.h"
#include "display.h"
#include "globals.h"
#include "mykeyboard.h"
#include <Arduino.h>

static const char* mango_words[] = {
    "mango", "MANGO", "Mango", "PABLO", "pablo", "Pablo",
    "MANGO!", "mango?", "PABLO MODE",
    "mango mango", "PABLO!", "i love mango",
    "MANGO MANGO", "mangoooo", "MANGOOOO",
    "mango is life", "MANGO TIME", "mango mango mango",
    "PABLO WAS HERE", "mango?!?!", "OH NO MANGO",
    "HELP PABLO", "mango invasion", "MANGO SZN",
    "pablo please", "PABLO HELP", "mango attack",
    "pablo pablo", "MANGO OR DIE", "call pablo"
};
static const int NUM_WORDS = sizeof(mango_words) / sizeof(mango_words[0]);

static bool pabloRunning = false;

static void pabloTone(int freq, int duration_ms) {
#if defined(BUZZ_PIN)
    tone(BUZZ_PIN, freq, duration_ms);
    delay(duration_ms);
#elif defined(DOUT)
    ledcAttach(DOUT, freq, 8);
    ledcWriteTone(DOUT, freq);
    delay(duration_ms);
    ledcWriteTone(DOUT, 0);
    ledcDetach(DOUT);
#else
    delay(duration_ms);
#endif
}

static uint16_t mangoColor() {
    uint8_t r = 200 + random(0, 56);
    uint8_t g = 120 + random(0, 90);
    return tft.color565(r, g, 0);
}

static void drawMango(int cx, int cy, int size, uint16_t color) {
    int rx = size;
    int ry = (size * 3) / 4;
    tft.fillEllipse(cx, cy, rx, ry, color);
    uint8_t r = ((color >> 11) & 0x1F) * 8;
    uint8_t g = ((color >> 5) & 0x3F) * 4;
    uint16_t dark = tft.color565((r * 2) / 3, (g * 2) / 3, 0);
    tft.fillEllipse(cx + rx / 4, cy + ry / 5, rx / 3, ry / 4, dark);
    uint16_t stem = tft.color565(80, 50, 10);
    tft.drawWideLine(cx, cy - ry, cx + 2, cy - ry - 5, 2, stem);
    uint16_t leaf = tft.color565(30, 160, 30);
    tft.fillEllipse(cx + 4, cy - ry - 3, 5, 3, leaf);
}

static void mangoChant(int phase) {
    if (!pabloRunning) return;
    static const int notes[] = {
        523, 659, 784, 1047, 784, 659, 523, 392, 523, 659
    };
    static const int LEN = 10;
    int noteLen = 200;
    for (int i = 0; i < LEN; i++) {
        if (!pabloRunning) break;
        pabloTone(notes[i], noteLen);
    }
}

static void pabloTask(void* pv) {
    pabloRunning = true;
    unsigned long start = millis();

    mangoChant(0);

    while (pabloRunning) {
        unsigned long elapsed = millis() - start;

        int phase = 0;
        if (elapsed > 35000) phase = 3;
        else if (elapsed > 22000) phase = 2;
        else if (elapsed > 10000) phase = 1;

        int spawns = 2 + phase * 3;
        for (int s = 0; s < spawns; s++) {
            int sz = random(20 + phase * 10, 35 + phase * 15);
            drawMango(random(sz, tft.width() - sz), random(sz, tft.height() - sz), sz, mangoColor());
        }

        int textCount = 1 + phase * 2;
        for (int t = 0; t < textCount; t++) {
            const char* w = mango_words[random(0, NUM_WORDS)];
            int x = random(0, max(1, tft.width() - 80));
            int y = random(0, max(1, tft.height() - 14));
            tft.setTextSize(1 + phase);
            tft.setTextColor(mangoColor());
            tft.drawString(w, x, y);
        }

        if (phase >= 2) {
            for (int p = 0; p < phase; p++) {
                int sz = random(25, 50);
                drawMango(random(sz, tft.width() - sz), random(sz, tft.height() - sz),
                          sz, tft.color565(random(200, 255), random(80, 180), 0));
            }
        }

        if (phase == 3) {
            for (int b = 0; b < 3; b++) {
                int sz = random(30, 55);
                drawMango(random(sz, tft.width() - sz), random(sz, tft.height() - sz),
                          sz, tft.color565(255, random(100, 200), 0));
            }
            tft.setTextSize(2);
            for (int k = 0; k < 4; k++) {
                const char* big = (k % 2 == 0) ? "PABLO" : "MANGO";
                tft.setTextColor(mangoColor());
                tft.drawString(big, random(0, max(1, tft.width() - 100)), random(0, max(1, tft.height() - 20)));
            }
        }

        if (elapsed > 42000) {
            for (int f = 400; f < 2000; f += 200) {
                pabloTone(f, 80);
            }
            tft.fillScreen(tft.color565(255, 180, 0));
            tft.setTextSize(2);
            tft.setTextColor(tft.color565(255, 0, 0), tft.color565(255, 180, 0));
            tft.drawCentreString("Hacked by", tft.width() / 2, tft.height() / 2 - 20);
            tft.drawCentreString("Pablo Escrow", tft.width() / 2, tft.height() / 2 + 5);
            tft.drawCentreString("Bar", tft.width() / 2, tft.height() / 2 + 30);
            vTaskDelay(pdMS_TO_TICKS(3000));
            esp_restart();
        }

        int delay_ms = (phase == 0) ? 1200 : (phase == 1) ? 800 : (phase == 2) ? 400 : 200;
        vTaskDelay(pdMS_TO_TICKS(delay_ms));

        if (elapsed > 2000 && (elapsed % 5000) < 1200) {
            mangoChant(phase);
        }
    }

    pabloRunning = false;
    vTaskDelete(NULL);
}

void pabloMode() {
    if (pabloRunning) {
        pabloRunning = false;
        vTaskDelay(pdMS_TO_TICKS(500));
        return;
    }

    tft.fillScreen(0x0000);
    tft.setTextSize(2);
    tft.setTextColor(0xFFFF, 0x0000);
    tft.drawCentreString("PABLO MODE", tft.width() / 2, tft.height() / 2 - 20);
    tft.setTextSize(1);
    tft.setTextColor(0xFFE0, 0x0000);
    tft.drawCentreString("Mango infection incoming...", tft.width() / 2, tft.height() / 2 + 5);
    tft.drawCentreString("Press any key to abort", tft.width() / 2, tft.height() / 2 + 20);

    unsigned long warnStart = millis();
    while (millis() - warnStart < 3000) {
        if (check(EscPress) || check(SelPress)) return;
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    xTaskCreate(pabloTask, "pablo", 8192, NULL, 1, NULL);
}
