/**
 * @file pomodoro.cpp
 * @author lordbuffcloud (https://github.com/lordbuffcloud/bruce-pomodoro)
 * @brief Pomodoro - work/break interval timer
 * @version 1.0
 * @date 2026-05-24
 */

#include "pomodoro.h"
#include "core/display.h"
#include "core/led_control.h"
#include "core/utils.h"
#include "modules/others/audio.h"

#include <math.h>

#define INPUT_POLL_DELAY 50
#define DISPLAY_UPDATE_INTERVAL 1000
#define LED_BREATH_INTERVAL 80     // ms between LED brightness updates
#define LED_BREATH_PERIOD_MS 4000  // full sin cycle = 4s

#define MIN_WORK 1
#define MAX_WORK 90
#define MIN_BREAK 1
#define MAX_BREAK 60
#define MIN_ROUNDS 2
#define MAX_ROUNDS 8

Pomodoro::Pomodoro() {
    playStartupChime();
    if (configure()) {
        ledBegin();
        runSession();
        ledEnd();
    }
}

void Pomodoro::playStartupChime() {
    // 3-note rising arpeggio: C5, E5, G5
    _tone(523, 90);
    delay(40);
    _tone(659, 90);
    delay(40);
    _tone(784, 140);
}

void Pomodoro::playLongBreakFanfare() {
    // Longer, more triumphant entry to the long break: 5 notes ending high
    _tone(523, 90);  // C5
    delay(60);
    _tone(659, 90);  // E5
    delay(60);
    _tone(784, 90);  // G5
    delay(60);
    _tone(988, 90);  // B5
    delay(60);
    _tone(1047, 240); // C6 (held)
}

Pomodoro::~Pomodoro() {
    tft.fillScreen(bruceConfig.bgColor);
    backToMenu();
}

void Pomodoro::ledBegin() {
#ifdef HAS_RGB_LED
    ledEffects(false);
    delay(20);
#endif
}

void Pomodoro::ledEnd() {
#ifdef HAS_RGB_LED
    setLedColor(bruceConfig.ledColor);
    if (bruceConfig.ledEffect != LED_EFFECT_SOLID) { ledEffects(true); }
#endif
}

void Pomodoro::ledSetPhase(Phase p, bool dim) {
#ifdef HAS_RGB_LED
    CRGB c;
    switch (p) {
        case PHASE_WORK: c = CRGB(dim ? 40 : 180, 0, 0); break;
        case PHASE_SHORT_BREAK: c = CRGB(0, dim ? 40 : 160, 0); break;
        case PHASE_LONG_BREAK: c = CRGB(0, dim ? 30 : 80, dim ? 40 : 160); break;
        default: c = CRGB::Black;
    }
    setLedColor(c);
#else
    (void)p;
    (void)dim;
#endif
}

void Pomodoro::ledBreathTick(Phase p, unsigned long t, bool paused) {
#ifdef HAS_RGB_LED
    if (paused) return; // static dim color handled by ledSetPhase(p, true)

    // sin-wave 0..1, 4s period
    float phase01 = (t % LED_BREATH_PERIOD_MS) / (float)LED_BREATH_PERIOD_MS;
    float s = 0.5f + 0.5f * sinf(phase01 * 2.0f * (float)M_PI);
    // Map to brightness 0.35..1.0 so it stays visibly lit at the trough
    float scale = 0.35f + 0.65f * s;

    uint8_t r = 0, g = 0, b = 0;
    switch (p) {
        case PHASE_WORK:        r = (uint8_t)(200 * scale); break;
        case PHASE_SHORT_BREAK: g = (uint8_t)(180 * scale); break;
        case PHASE_LONG_BREAK:  g = (uint8_t)(90 * scale); b = (uint8_t)(180 * scale); break;
    }
    setLedColor(CRGB(r, g, b));
#else
    (void)p; (void)t; (void)paused;
#endif
}

void Pomodoro::drawRoundDots(Phase p) {
    int dotR = 3;
    int gap = 4;
    int total = roundsBeforeLong;
    if (total < 1) total = 1;
    int width = total * (dotR * 2) + (total - 1) * gap;
    int startX = centerX - width / 2 + dotR;
    int y = tftHeight - BORDER_PAD_X - 2 * LH - 4;

    // Position in cycle: completed rounds = ((round-1) % roundsBeforeLong)
    int cyclePos = (round - 1) % roundsBeforeLong;

    uint16_t fillColor = phaseColor(p);
    uint16_t pendingColor = TFT_DARKGREY;

    for (int i = 0; i < total; i++) {
        int cx = startX + i * (dotR * 2 + gap);
        if (i < cyclePos) {
            tft.fillCircle(cx, y, dotR, fillColor);
        } else if (i == cyclePos) {
            tft.fillCircle(cx, y, dotR, fillColor);
            tft.drawCircle(cx, y, dotR + 2, bruceConfig.priColor);
        } else {
            tft.drawCircle(cx, y, dotR, pendingColor);
        }
    }
}


const char *Pomodoro::phaseName(Phase p) {
    switch (p) {
        case PHASE_WORK: return "WORK";
        case PHASE_SHORT_BREAK: return "SHORT BREAK";
        case PHASE_LONG_BREAK: return "LONG BREAK";
    }
    return "";
}

uint16_t Pomodoro::phaseColor(Phase p) {
    switch (p) {
        case PHASE_WORK: return TFT_RED;
        case PHASE_SHORT_BREAK: return TFT_GREEN;
        case PHASE_LONG_BREAK: return TFT_CYAN;
    }
    return bruceConfig.priColor;
}

bool Pomodoro::configure() {
    SetupField field = FIELD_WORK;

    tft.fillScreen(bruceConfig.bgColor);
    delay(150);

    while (true) {
        drawSetup(field);

        if (check(EscPress)) { return false; }

        if (check(NextPress)) {
            switch (field) {
                case FIELD_WORK: workMin = (workMin >= MAX_WORK) ? MIN_WORK : workMin + 1; break;
                case FIELD_SHORT: shortMin = (shortMin >= MAX_BREAK) ? MIN_BREAK : shortMin + 1; break;
                case FIELD_LONG: longMin = (longMin >= MAX_BREAK) ? MIN_BREAK : longMin + 1; break;
                case FIELD_ROUNDS:
                    roundsBeforeLong = (roundsBeforeLong >= MAX_ROUNDS) ? MIN_ROUNDS : roundsBeforeLong + 1;
                    break;
                case FIELD_SOUND: playSoundOnPhaseEnd = !playSoundOnPhaseEnd; break;
                case FIELD_START: break;
            }
        }

        if (check(PrevPress)) {
            switch (field) {
                case FIELD_WORK: workMin = (workMin <= MIN_WORK) ? MAX_WORK : workMin - 1; break;
                case FIELD_SHORT: shortMin = (shortMin <= MIN_BREAK) ? MAX_BREAK : shortMin - 1; break;
                case FIELD_LONG: longMin = (longMin <= MIN_BREAK) ? MAX_BREAK : longMin - 1; break;
                case FIELD_ROUNDS:
                    roundsBeforeLong = (roundsBeforeLong <= MIN_ROUNDS) ? MAX_ROUNDS : roundsBeforeLong - 1;
                    break;
                case FIELD_SOUND: playSoundOnPhaseEnd = !playSoundOnPhaseEnd; break;
                case FIELD_START: break;
            }
        }

        if (check(SelPress)) {
            if (field == FIELD_START) { return true; }
            field = static_cast<SetupField>(field + 1);
        }

        delay(INPUT_POLL_DELAY);
    }
}

void Pomodoro::drawSetup(SetupField field) {
    drawMainBorderWithTitle("Pomodoro", false);

    int rowH = LH + 4;
    int firstRowY = BORDER_PAD_X + 4 * LH;
    int labelX = BORDER_PAD_X + 6;
    int valueX = tftWidth - BORDER_PAD_X - 6;

    auto rowY = [&](int i) { return firstRowY + i * rowH; };

    tft.setTextSize(1);

    struct Row {
        const char *label;
        char value[16];
        bool highlight;
    };

    Row rows[6];
    rows[0] = {"Work", "", field == FIELD_WORK};
    snprintf(rows[0].value, sizeof(rows[0].value), "%d min", workMin);

    rows[1] = {"Short break", "", field == FIELD_SHORT};
    snprintf(rows[1].value, sizeof(rows[1].value), "%d min", shortMin);

    rows[2] = {"Long break", "", field == FIELD_LONG};
    snprintf(rows[2].value, sizeof(rows[2].value), "%d min", longMin);

    rows[3] = {"Rounds", "", field == FIELD_ROUNDS};
    snprintf(rows[3].value, sizeof(rows[3].value), "%d", roundsBeforeLong);

    rows[4] = {"Sound", "", field == FIELD_SOUND};
    snprintf(rows[4].value, sizeof(rows[4].value), "%s", playSoundOnPhaseEnd ? "ON" : "OFF");

    rows[5] = {"", "", field == FIELD_START};
    snprintf(rows[5].value, sizeof(rows[5].value), "[ START ]");

    for (int i = 0; i < 6; i++) {
        int y = rowY(i);
        tft.fillRect(BORDER_PAD_X, y - 1, tftWidth - BORDER_PAD_X * 2, rowH, bruceConfig.bgColor);

        uint16_t fg = rows[i].highlight ? bruceConfig.priColor : TFT_DARKGREY;
        tft.setTextColor(fg, bruceConfig.bgColor);

        if (i == 5) {
            tft.setTextDatum(MC_DATUM);
            tft.drawString(rows[i].value, centerX, y + LH / 2, 1);
            tft.setTextDatum(TL_DATUM);
            if (rows[i].highlight) {
                int w = strlen(rows[i].value) * LW;
                tft.drawLine(centerX - w / 2, y + rowH - 2, centerX + w / 2, y + rowH - 2, fg);
            }
        } else {
            tft.setTextDatum(TL_DATUM);
            tft.drawString(rows[i].label, labelX, y, 1);
            tft.setTextDatum(TR_DATUM);
            tft.drawString(rows[i].value, valueX, y, 1);
            tft.setTextDatum(TL_DATUM);
        }
    }
}

void Pomodoro::runSession() {
    round = 1;
    while (true) {
        bool isLong = (round % roundsBeforeLong == 0);

        if (!runPhase(PHASE_WORK, workMin * 60)) return;
        if (!playPhaseEndAlarm(PHASE_WORK)) return;

        Phase brk = isLong ? PHASE_LONG_BREAK : PHASE_SHORT_BREAK;
        int brkSec = (isLong ? longMin : shortMin) * 60;
        if (!runPhase(brk, brkSec)) return;
        if (!playPhaseEndAlarm(brk)) return;

        round++;
        if (round > 99) round = 1;
    }
}

bool Pomodoro::runPhase(Phase p, int durationSec) {
    phase = p;
    unsigned long startMs = millis();
    unsigned long lastUpdate = 0;
    unsigned long lastLedUpdate = 0;
    int lastShownSec = -1;
    bool paused = false;
    unsigned long pausedAt = 0;
    unsigned long pausedTotal = 0;

    tft.fillScreen(bruceConfig.bgColor);
    drawRunFrame(p, durationSec, durationSec);
    ledSetPhase(p, false);

    while (true) {
        unsigned long now = millis();

        if (check(EscPress)) { return false; }

        if (check(SelPress)) {
            if (!paused) {
                paused = true;
                pausedAt = now;
                ledSetPhase(p, true);
            } else {
                paused = false;
                pausedTotal += (now - pausedAt);
                ledSetPhase(p, false);
            }
            lastShownSec = -1;
            delay(150);
        }

        if (check(NextPress)) {
            return true;
        }

        unsigned long effectiveMs = paused ? (pausedAt - startMs - pausedTotal)
                                           : (now - startMs - pausedTotal);
        int elapsedSec = effectiveMs / 1000;
        int remaining = durationSec - elapsedSec;
        if (remaining < 0) remaining = 0;

        if (remaining == 0 && !paused) { return true; }

        if (now - lastUpdate >= DISPLAY_UPDATE_INTERVAL || lastShownSec == -1) {
            if (remaining != lastShownSec) {
                drawRunFrame(p, remaining, durationSec);
                if (paused) {
                    tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
                    tft.setTextSize(1);
                    tft.setTextDatum(MC_DATUM);
                    tft.drawString("-- PAUSED --", centerX, tftHeight - BORDER_PAD_X - LH, 1);
                    tft.setTextDatum(TL_DATUM);
                }
                lastShownSec = remaining;
            }
            lastUpdate = now;
        }

        if (now - lastLedUpdate >= LED_BREATH_INTERVAL) {
            ledBreathTick(p, now, paused);
            lastLedUpdate = now;
        }

        delay(INPUT_POLL_DELAY);
    }
}

void Pomodoro::drawRunFrame(Phase p, int remainingSec, int phaseDurationSec) {
    if (remainingSec < 0) remainingSec = 0;

    char title[32];
    bool isLong = (round % roundsBeforeLong == 0);
    if (p == PHASE_WORK) {
        snprintf(title, sizeof(title), "Pomodoro #%d", round);
    } else {
        snprintf(title, sizeof(title), "Break (%s)", isLong ? "long" : "short");
    }
    drawMainBorderWithTitle(title, false);

    tft.setTextSize(1);
    tft.setTextColor(phaseColor(p), bruceConfig.bgColor);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(phaseName(p), centerX, BORDER_PAD_X + 3 * LH, 1);

    const int mins = remainingSec / 60;
    const int secs = remainingSec % 60;
    char timeStr[8];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", mins, secs);

    uint8_t f_size = 4;
    for (uint8_t i = 4; i > 0; i--) {
        if (i * LW * 6 < (tftWidth - BORDER_PAD_X * 2)) {
            f_size = i;
            break;
        }
    }

    tft.setTextSize(f_size);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    int timeY = centerY - (f_size * LH) / 2;
    tft.fillRect(BORDER_PAD_X + 2, timeY - 2, tftWidth - BORDER_PAD_X * 2 - 4, f_size * LH + 4, bruceConfig.bgColor);
    tft.drawString(timeStr, centerX, timeY + (f_size * LH) / 2, 1);

    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);

    int barX = BORDER_PAD_X + 6;
    int barW = tftWidth - BORDER_PAD_X * 2 - 12;
    int barY = centerY + (f_size * LH) / 2 + LH;
    int barH = 6;
    tft.drawRect(barX, barY, barW, barH, bruceConfig.priColor);
    int filledW = (phaseDurationSec > 0)
                      ? (barW - 2) * (phaseDurationSec - remainingSec) / phaseDurationSec
                      : 0;
    if (filledW < 0) filledW = 0;
    if (filledW > barW - 2) filledW = barW - 2;
    tft.fillRect(barX + 1, barY + 1, filledW, barH - 2, phaseColor(p));
    tft.fillRect(barX + 1 + filledW, barY + 1, (barW - 2) - filledW, barH - 2, bruceConfig.bgColor);

    drawRoundDots(p);

    char foot[40];
    snprintf(foot, sizeof(foot), "Round %d  -  SEL pause  NEXT skip", round);
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(foot, centerX, tftHeight - BORDER_PAD_X - LH, 1);
    tft.setTextDatum(TL_DATUM);
}

bool Pomodoro::playPhaseEndAlarm(Phase justEnded) {
    const char *banner = (justEnded == PHASE_WORK) ? "BREAK TIME" : "BACK TO WORK";
    bool workEnded = (justEnded == PHASE_WORK);

    Phase nextPhase;
    if (workEnded) {
        nextPhase = (round % roundsBeforeLong == 0) ? PHASE_LONG_BREAK : PHASE_SHORT_BREAK;
    } else {
        nextPhase = PHASE_WORK;
    }

    tft.fillScreen(bruceConfig.bgColor);
    drawMainBorderWithTitle("Pomodoro", false);

    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(phaseColor(nextPhase), bruceConfig.bgColor);
    tft.drawString(banner, centerX, centerY - LH, 1);

    tft.setTextSize(1);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawString("SEL: continue   ESC: stop", centerX, centerY + 2 * LH, 1);
    tft.setTextDatum(TL_DATUM);

    int beeps = playSoundOnPhaseEnd ? 6 : 0;

#ifdef HAS_RGB_LED
    CRGB flashColor;
    switch (nextPhase) {
        case PHASE_WORK: flashColor = CRGB(220, 30, 0); break;
        case PHASE_SHORT_BREAK: flashColor = CRGB(0, 220, 30); break;
        case PHASE_LONG_BREAK: flashColor = CRGB(0, 120, 220); break;
        default: flashColor = CRGB::White;
    }
#endif

    // Long-break entry gets a distinct fanfare; fewer alarm beeps follow it
    const bool playedFanfare = playSoundOnPhaseEnd && nextPhase == PHASE_LONG_BREAK;
    if (playedFanfare) {
#ifdef HAS_RGB_LED
        setLedColor(flashColor);
#endif
        playLongBreakFanfare();
        delay(150);
        beeps = 3;
    }

    for (int i = 0; i < beeps; i++) {
        if (check(SelPress)) { delay(150); return true; }
        if (check(EscPress)) { return false; }

#ifdef HAS_RGB_LED
        setLedColor(flashColor);
#endif

        if (workEnded) {
            _tone(880, 120);
            unsigned long s = millis();
            while (millis() - s < 60) {
                if (check(SelPress)) { delay(150); return true; }
                if (check(EscPress)) { return false; }
                delay(10);
            }
            _tone(1320, 200);
        } else {
            _tone(660, 200);
            unsigned long s = millis();
            while (millis() - s < 60) {
                if (check(SelPress)) { delay(150); return true; }
                if (check(EscPress)) { return false; }
                delay(10);
            }
            _tone(440, 250);
        }

#ifdef HAS_RGB_LED
        setLedColor(CRGB::Black);
#endif

        unsigned long s = millis();
        while (millis() - s < 400) {
            if (check(SelPress)) { delay(150); return true; }
            if (check(EscPress)) { return false; }
            delay(10);
        }
    }

#ifdef HAS_RGB_LED
    setLedColor(flashColor);
#endif

    while (true) {
        if (check(SelPress)) { delay(150); return true; }
        if (check(EscPress)) { return false; }
        delay(INPUT_POLL_DELAY);
    }
}
