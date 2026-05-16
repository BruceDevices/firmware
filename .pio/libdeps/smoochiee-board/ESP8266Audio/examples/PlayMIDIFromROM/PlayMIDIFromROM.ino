// PlayMIDIFromROM
// Released to the public domain (2025) by Earle F. Philhower, III <earlephilhower@yahoo.com>
#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <ESP8266Audio.h>

// Define the SF2 (SoundFont) to use. Will be stored in PROGMEM as global _tsf
#ifdef ESP8266
// The ESP8266 has a 1MB ROM limit so use a smaller SF2 file
#include <libtinysoundfont/scratch2010.h>
#else
// Pico and ESP32 can have up to 16MB ROMs, use the 1M General MIDI (1mgm) font or your own
// For ESP32 be sure to use HUGE APP or another partition mode which allows you to have sketches larger than 1-2MB

#include <libtinysoundfont/1mgm.h>
#endif

// Select one of the following in order of CPU limitations.  You can also use your own.
//#include "furelise_mid.h"
//#include "venture_mid.h"
#include "jm_mozdi_mid.h"

// Note that the .MID file does NOT need to be in PROGMEM, it just makes the examples easier to use (no FS upload required).
AudioFileSourcePROGMEM *midfile;

// Any AudioOutput* would work here (AudioOutputPWM, AudioOutputI2SNoDAT, etc.)
AudioOutputI2S *dac;

// The MIDI wavetable synthesizer
AudioGeneratorMIDI *midi;


void setup() {
  WiFi.mode(WIFI_OFF);

  Serial.begin(115200);
  Serial.println("Starting up...\n");

  audioLogger = &Serial;
  midfile = new AudioFileSourcePROGMEM(_mid, sizeof(_mid));

  dac = new AudioOutputI2S();
  dac->SetPinout(0, 1, 2); // Adjust to your I2S DAC pinout
  dac->SetGain(0.5);
  midi = new AudioGeneratorMIDI();
  midi->SetSoundFont(&_tsf);
  // If you get audio clicks/break up then you can decrease the sample rate.  You can also try increasing this if your MIDs are simple enough and the chip fast enough
  midi->SetSampleRate(22050);

  Serial.printf("BEGIN...\n");
  midi->begin(midfile, dac);
}

void loop() {
  static int cnt, lcnt;
  if (midi->isRunning()) {
    if (!midi->loop()) {
      midi->stop();
    } else {
      if (++cnt == 10000) {
        Serial.printf("Loop %d\n", ++lcnt * 1000);
        cnt = 0;
      }
    }
  } else {
    Serial.printf("MIDI done\n");
    delay(1000);
  }
}
