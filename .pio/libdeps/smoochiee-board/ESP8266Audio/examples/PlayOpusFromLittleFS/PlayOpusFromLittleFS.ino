#include <Arduino.h>

#ifdef ESP8266

void setup() {
  Serial.begin(115200);
}

void loop() {
  Serial.println("OPUS not supported on the ESP8266, sorry!");
  delay(1000);
}

#else

#include <WiFi.h>
#include <LittleFS.h>

#include "AudioFileSourceLittleFS.h"
#include "AudioGeneratorOpus.h"
#include "AudioOutputI2S.h"

// The includes OPUS file is from Kevin MacLeod (incompetech.com), Licensed under Creative Commons: By Attribution 3.0, http://creativecommons.org/licenses/by/3.0/

AudioGeneratorOpus *opus;
AudioFileSourceLittleFS *file;
AudioOutputI2S *out;

#ifdef ESP32
// We need more than the 8K default with other things running, so just double to avoid issues.
// Opus codes uses *lots* of stack variables in the internal decoder instead of a global working chunk
SET_LOOP_TASK_STACK_SIZE(16 * 1024);  // 16KB
// On the Pico this is already taken care of using the built-in NONTHREADSAFE_PSEUDOSTACK in the config file.
#endif

void setup() {
  WiFi.mode(WIFI_OFF);
  Serial.begin(115200);
  delay(1000);
  LittleFS.begin();
  Serial.printf("Sample Opus playback begins...\n");

  audioLogger = &Serial;
  file = new AudioFileSourceLittleFS("/gs-16b-2c-44100hz.opus");
  out = new AudioOutputI2S();
  // out->SetPinout(0, 1, 2); // Set the pinout if needed
  opus = new AudioGeneratorOpus();
  opus->begin(file, out);
}

void loop() {
  if (opus->isRunning()) {
    if (!opus->loop()) {
      opus->stop();
    }
  } else {
    Serial.printf("Opus done\n");
    delay(1000);
  }
}

#endif
