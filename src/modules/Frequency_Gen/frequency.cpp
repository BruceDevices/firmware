#include "core/display.h"
#include "core/utils.h"
#include <stdlib.h>

#include "core/menu_items/BuzzerMenu.h"

#include "driver/ledc.h"
#include <Arduino.h>

const uint8_t buzzerPin = 2;
int buzzerFreq = 11600;
int buzzerVol = 64;
void SetBuzzerFreq11600() { buzzerFreq = 11600; }
void SetBuzzerFreq3070() { buzzerFreq = 3070; }
void SetBuzzerFreq9000() { buzzerFreq = 9000; }

void SetBuzzerVol100() { buzzerVol = 128; }
void SetBuzzerVol50() { buzzerVol = 64; }
void SetBuzzerVol20() { buzzerVol = 25.6; }
void SetBuzzerVol10() { buzzerVol = 12.8; }

void BuzzerConfig() {
    returnToMenu = false;
    options = {
        {"11600Hz",   [=]() { SetBuzzerFreq11600(); }},
        {"9000Hz",    [=]() { SetBuzzerFreq9000(); } },
        {"3070Hz",    [=]() { SetBuzzerFreq3070(); } },
        {"Main Menu", [=]() { return; }              },
    };
    // addOptionToMainMenu();

    loopOptions(options);
    // options.clear(); // needed?? idk, check custom_ir.cpp
};

void BuzzerVolume() {
    returnToMenu = false;
    options = {
        {"100%",      [=]() { SetBuzzerVol100(); }},
        {"50%",       [=]() { SetBuzzerVol50(); } },
        {"20%",       [=]() { SetBuzzerVol20(); } },
        {"10%",       [=]() { SetBuzzerVol10(); } },
        {"Main Menu", [=]() { return; }           },
    };
    // addOptionToMainMenu();

    loopOptions(options);
    // options.clear(); // needed?? idk, check custom_ir.cpp
};

void StartFreq() {
    ledcSetup(0, buzzerFreq, 8); // channel 0, freq, 8‑bit resolution
    ledcAttachPin(buzzerPin, 0);
    ledcWrite(0, buzzerVol);
    while (true) {
        if (check(PrevPress)) {
            ledcWrite(0, 0); // silence
            return;
        }
        delay(2);
    }
}

// PlayErika() - Plays "Erika (Auf der Heide blüht ein kleines Blümelein)" on the M5StickC Plus 2 buzzer
void PlayErika() {
    while (true) {
        if (check(PrevPress)) {
            ledcWrite(0, 0); // silence
            return;
        }
        ledcAttachPin(buzzerPin, 0); // Attach the buzzer pin once

        // Note frequencies (Hz), 0 = rest.
        const int NoteFrequencies[] = {494, 523, 587, 587, 587, 784, 784, 988, 880, 784, 0,
                                       0,   740, 784, 880, 0,   0,   988, 880, 784, 0,   0};

        // Durations in ms at 120 BPM (quarter=500ms, dotted=750ms, eighth=250ms, half=1000ms)
        const int NoteDurations[] = {750,  250, 500, 500, 500, 500,  500, 750, 250, 500, 500,
                                     1000, 500, 500, 500, 500, 1000, 750, 250, 500, 500, 1000};

        const int notesCount = sizeof(NoteFrequencies) / sizeof(NoteFrequencies[0]);

        for (int i = 0; i < notesCount; i++) {
            if (check(PrevPress)) {
                ledcWrite(0, 0); // Stop sound
                return;
            }

            int freq = NoteFrequencies[i];
            int dur = NoteDurations[i];

            if (freq > 0) {
                ledcSetup(0, freq, 8);   // Set frequency
                ledcWrite(0, buzzerVol); // Set volume
            } else {
                ledcWrite(0, 0); // Rest (silence)
            }

            delay(dur);

            // Small gap between notes
            ledcWrite(0, 0);
            delay(25);
        }
        if (check(PrevPress)) {
            ledcWrite(0, 0); // silence
            return;
        }
        // Ensure buzzer is off
        ledcWrite(0, 0);
    }
}

void playNeverGonnaGiveYouUp() {
#define NOTE_B0 31
#define NOTE_C1 33
#define NOTE_CS1 35
#define NOTE_D1 37
#define NOTE_DS1 39
#define NOTE_E1 41
#define NOTE_F1 44
#define NOTE_FS1 46
#define NOTE_G1 49
#define NOTE_GS1 52
#define NOTE_A1 55
#define NOTE_AS1 58
#define NOTE_B1 62
#define NOTE_C2 65
#define NOTE_CS2 69
#define NOTE_D2 73
#define NOTE_DS2 78
#define NOTE_E2 82
#define NOTE_F2 87
#define NOTE_FS2 93
#define NOTE_G2 98
#define NOTE_GS2 104
#define NOTE_A2 110
#define NOTE_AS2 117
#define NOTE_B2 123
#define NOTE_C3 131
#define NOTE_CS3 139
#define NOTE_D3 147
#define NOTE_DS3 156
#define NOTE_E3 165
#define NOTE_F3 175
#define NOTE_FS3 185
#define NOTE_G3 196
#define NOTE_GS3 208
#define NOTE_A3 220
#define NOTE_AS3 233
#define NOTE_B3 247
#define NOTE_C4 262
#define NOTE_CS4 277
#define NOTE_D4 294
#define NOTE_DS4 311
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_FS4 370
#define NOTE_G4 392
#define NOTE_GS4 415
#define NOTE_A4 440
#define NOTE_AS4 466
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_CS5 554
#define NOTE_D5 587
#define NOTE_DS5 622
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_FS5 740
#define NOTE_G5 784
#define NOTE_GS5 831
#define NOTE_A5 880
#define NOTE_AS5 932
#define NOTE_B5 988
#define NOTE_C6 1047
#define NOTE_CS6 1109
#define NOTE_D6 1175
#define NOTE_DS6 1245
#define NOTE_E6 1319
#define NOTE_F6 1397
#define NOTE_FS6 1480
#define NOTE_G6 1568
#define NOTE_GS6 1661
#define NOTE_A6 1760
#define NOTE_AS6 1865
#define NOTE_B6 1976
#define NOTE_C7 2093
#define NOTE_CS7 2217
#define NOTE_D7 2349
#define NOTE_DS7 2489
#define NOTE_E7 2637
#define NOTE_F7 2794
#define NOTE_FS7 2960
#define NOTE_G7 3136
#define NOTE_GS7 3322
#define NOTE_A7 3520
#define NOTE_AS7 3729
#define NOTE_B7 3951
#define NOTE_C8 4186
#define NOTE_CS8 4435
#define NOTE_D8 4699
#define NOTE_DS8 4978
#define REST 0

    int tempo = 114;

    int melody[] = {NOTE_D5,  -4, NOTE_E5,  -4, NOTE_A4,  4,  NOTE_E5, -4, NOTE_FS5, -4, NOTE_A5, 16,
                    NOTE_G5,  16, NOTE_FS5, 8,  NOTE_D5,  -4, NOTE_E5, -4, NOTE_A4,  2,  NOTE_A4, 16,
                    NOTE_A4,  16, NOTE_B4,  16, NOTE_D5,  8,  NOTE_D5, 16,

                    NOTE_D5,  -4, NOTE_E5,  -4, NOTE_A4,  4,  NOTE_E5, -4, NOTE_FS5, -4, NOTE_A5, 16,
                    NOTE_G5,  16, NOTE_FS5, 8,  NOTE_D5,  -4, NOTE_E5, -4, NOTE_A4,  2,  NOTE_A4, 16,
                    NOTE_A4,  16, NOTE_B4,  16, NOTE_D5,  8,  NOTE_D5, 16,

                    REST,     4,  NOTE_B4,  8,  NOTE_CS5, 8,  NOTE_D5, 8,  NOTE_D5,  8,  NOTE_E5, 8,
                    NOTE_CS5, -8, NOTE_B4,  16, NOTE_A4,  2,  REST,    4};

    int notes = sizeof(melody) / sizeof(melody[0]);

    for (int thisNote = 0; thisNote < notes; thisNote += 2) {
        int note = melody[thisNote];
        int duration = melody[thisNote + 1];

        int noteDuration = 0;
        if (duration > 0) {
            noteDuration = 60000 / tempo / duration;
        } else {
            noteDuration = (60000 / tempo / abs(duration)) * 1.5;
        }

        if (note == REST) {
            delay(noteDuration);
        } else {
            tone(buzzerPin, note, noteDuration * 0.9);
            // Optionally apply volume if using analogWrite or DAC
            // analogWrite(buzzerPin, buzzerVol); // only if using PWM/digital volume
            delay(noteDuration);
            noTone(buzzerPin);
        }
    }
}

void PlayNokia() {
    while (true) {
// Note definitions
#define NOTE_B0 31
#define NOTE_C1 33
#define NOTE_CS1 35
#define NOTE_D1 37
#define NOTE_DS1 39
#define NOTE_E1 41
#define NOTE_F1 44
#define NOTE_FS1 46
#define NOTE_G1 49
#define NOTE_GS1 52
#define NOTE_A1 55
#define NOTE_AS1 58
#define NOTE_B1 62
#define NOTE_C2 65
#define NOTE_CS2 69
#define NOTE_D2 73
#define NOTE_DS2 78
#define NOTE_E2 82
#define NOTE_F2 87
#define NOTE_FS2 93
#define NOTE_G2 98
#define NOTE_GS2 104
#define NOTE_A2 110
#define NOTE_AS2 117
#define NOTE_B2 123
#define NOTE_C3 131
#define NOTE_CS3 139
#define NOTE_D3 147
#define NOTE_DS3 156
#define NOTE_E3 165
#define NOTE_F3 175
#define NOTE_FS3 185
#define NOTE_G3 196
#define NOTE_GS3 208
#define NOTE_A3 220
#define NOTE_AS3 233
#define NOTE_B3 247
#define NOTE_C4 262
#define NOTE_CS4 277
#define NOTE_D4 294
#define NOTE_DS4 311
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_FS4 370
#define NOTE_G4 392
#define NOTE_GS4 415
#define NOTE_A4 440
#define NOTE_AS4 466
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_CS5 554
#define NOTE_D5 587
#define NOTE_DS5 622
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_FS5 740
#define NOTE_G5 784
#define NOTE_GS5 831
#define NOTE_A5 880
#define NOTE_AS5 932
#define NOTE_B5 988
#define NOTE_C6 1047
#define NOTE_CS6 1109
#define NOTE_D6 1175
#define NOTE_DS6 1245
#define NOTE_E6 1319
#define NOTE_F6 1397
#define NOTE_FS6 1480
#define NOTE_G6 1568
#define NOTE_GS6 1661
#define NOTE_A6 1760
#define NOTE_AS6 1865
#define NOTE_B6 1976
#define NOTE_C7 2093
#define NOTE_CS7 2217
#define NOTE_D7 2349
#define NOTE_DS7 2489
#define NOTE_E7 2637
#define NOTE_F7 2794
#define NOTE_FS7 2960
#define NOTE_G7 3136
#define NOTE_GS7 3322
#define NOTE_A7 3520
#define NOTE_AS7 3729
#define NOTE_B7 3951
#define NOTE_C8 4186
#define NOTE_CS8 4435
#define NOTE_D8 4699
#define NOTE_DS8 4978
#define REST 0

        int tempo = 180;

        int melody[] = {
            NOTE_E5, 8, NOTE_D5, 8, NOTE_FS4, 4, NOTE_GS4, 4, NOTE_CS5, 8, NOTE_B4, 8, NOTE_D4, 4,
            NOTE_E4, 4, NOTE_B4, 8, NOTE_A4,  8, NOTE_CS4, 4, NOTE_E4,  4, NOTE_A4, 2,
        };

        int notes = sizeof(melody) / sizeof(melody[0]) / 2;
        int wholenote = (60000 * 4) / tempo;

        for (int thisNote = 0; thisNote < notes * 2; thisNote += 2) {
            if (check(PrevPress)) {
                ledcWrite(0, 0); // silence
                return;          // exit PlayNokia function immediately
            }

            int divider = melody[thisNote + 1];
            int noteDuration;

            if (divider > 0) {
                noteDuration = wholenote / divider;
            } else {
                noteDuration = (wholenote / abs(divider)) * 1.5;
            }

            tone(buzzerPin, melody[thisNote], noteDuration * 0.9);
            delay(noteDuration);
            noTone(buzzerPin);
        }
        delay(1000);
    }
}
