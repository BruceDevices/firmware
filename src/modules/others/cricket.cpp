#include "core/display.h"
#include <Arduino.h>

#define BUZZER_PIN 2
#define BUZZER_CHANNEL 0
#define BUZZER_RESOLUTION 8

void setupCircket() {
    ledcSetup(0, 500, 8);
    ledcAttachPin(BUZZER_PIN, 0);
}

// void loopCricket() {
//     setupCircket();
//     while (true) {
//         for (int freq = 2000; freq <= 6000; freq += 50) {
//             ledcWriteTone(0, freq);
//             delay(10);
//         }
//         ledcWriteTone(0, 0);
//         delay(1000); // random here
//     }
// }

void playTone(int freq, float duty) {
    ledcSetup(BUZZER_CHANNEL, freq, BUZZER_RESOLUTION);
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
    duty = constrain(duty, 0, 255);
    ledcWrite(BUZZER_CHANNEL, (int)duty);
}

void stopTone() { ledcWrite(BUZZER_CHANNEL, 0); }

void chirpLoud(byte times, int freq, float volume) {
    int maxDuty = volume * 255;
    while (times-- > 0) {
        int v = 0;
        while (v < maxDuty) {
            playTone(freq, v);
            v += 4;
        }
        v = maxDuty;
        while (v > 0) {
            playTone(freq, v);
            v -= 4;
        }
        delay(10);
    }
    stopTone();
}

void chirpFade(byte times, int freq, float volume) {
    int maxDuty = volume * 255;
    float fadeOut = 1.0;
    while (times-- > 0) {
        float mv = 0.0;
        while (mv < 1.0) {
            int mvDuty = maxDuty * mv * fadeOut;
            int v = 0;
            while (v < mvDuty) {
                playTone(freq, v);
                v += 4;
            }
            v = mvDuty;
            while (v > 0) {
                playTone(freq, v);
                v -= 4;
            }
            delay(20);
            mv += 0.2;
        }
        mv = 1.0;
        while (mv > 0.0) {
            int mvDuty = maxDuty * mv * fadeOut;
            int v = 0;
            while (v < mvDuty) {
                playTone(freq, v);
                v += 4;
            }
            v = mvDuty;
            while (v > 0) {
                playTone(freq, v);
                v -= 4;
            }
            delay(20);
            mv -= 0.2;
        }
        fadeOut -= 0.75;
    }
    stopTone();
}

void chirpNoise(byte times, float volume) {
    int maxDuty = volume * 255;
    while (times-- > 0) {
        int v = 0;
        while (v < maxDuty) {
            playTone(random(6000, 10000), v);
            v += 4;
        }
        v = maxDuty;
        while (v > 0) {
            playTone(random(6000, 10000), v);
            v -= 4;
        }
        delay(10);
    }
    stopTone();
}

void chirp() {
    int freq = random(3700, 4000);
    float volume = random(10, 101) / 100.0;

    int chirpType = random(0, 3);
    if (chirpType == 0) {
        int chirpCount = random(1, 3);
        int count = random(1, 6);
        while (chirpCount-- > 0) {
            chirpLoud(count, freq, volume);
            delay(100);
        }
    } else if (chirpType == 1) {
        int chirpCount = random(1, 3);
        chirpFade(chirpCount, freq, volume);
    } else if (chirpType == 2) {
        int chirpCount = random(1, 3);
        int count = 3;
        while (chirpCount-- > 0) {
            chirpNoise(count, volume);
            delay(10);
        }
    }
}

int delayTime;

void startChirp() {
    while (true) {
        chirp();
        int delayTime = random(900000, 14400000); // 15min, 4h
        for (int i = 0; i < delayTime; ++i) {
            delay(1);
            if (check(PrevPress)) return;
        }
    }
}
