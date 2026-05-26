/**
 * @file pomodoro.h
 * @author lordbuffcloud (https://github.com/lordbuffcloud/bruce-pomodoro)
 * @brief Pomodoro - work/break interval timer
 * @version 1.0
 * @date 2026-05-24
 */

#ifndef __POMODORO_H__
#define __POMODORO_H__

#include <globals.h>

class Pomodoro {
private:
    enum Phase {
        PHASE_WORK = 0,
        PHASE_SHORT_BREAK = 1,
        PHASE_LONG_BREAK = 2,
    };

    enum SetupField {
        FIELD_WORK = 0,
        FIELD_SHORT = 1,
        FIELD_LONG = 2,
        FIELD_ROUNDS = 3,
        FIELD_SOUND = 4,
        FIELD_START = 5,
    };

    int workMin = 25;
    int shortMin = 5;
    int longMin = 15;
    int roundsBeforeLong = 4;
    bool playSoundOnPhaseEnd = true;

    int round = 1;
    Phase phase = PHASE_WORK;

    int centerX = tftWidth / 2;
    int centerY = tftHeight / 2;

    bool configure();
    void runSession();
    bool runPhase(Phase p, int durationSec);
    void drawSetup(SetupField field);
    void drawRunFrame(Phase p, int remainingSec, int phaseDurationSec);
    bool playPhaseEndAlarm(Phase justEnded);
    const char *phaseName(Phase p);
    uint16_t phaseColor(Phase p);
    void ledBegin();
    void ledEnd();
    void ledSetPhase(Phase p, bool dim);
    void ledBreathTick(Phase p, unsigned long t, bool paused);
    void drawRoundDots(Phase p);
    void playStartupChime();
    void playLongBreakFanfare();

public:
    Pomodoro();
    ~Pomodoro();
};

#endif
