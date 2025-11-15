#include "audio.h"
#include "AudioFileSourceFunction.h"
#include "AudioGeneratorMIDI.h"
#include "AudioGeneratorWAV.h"
#include "AudioGeneratorAAC.h"
#include "AudioGeneratorFLAC.h"
#include "AudioOutputI2SNoDAC.h"
#include "core/mykeyboard.h"
#include <ESP8266Audio.h>
#include <ESP8266SAM.h>

#ifdef ESP32
#include <esp_idf_version.h>
#endif

static bool configureI2SPinout(AudioOutputI2S *output) {
#if defined(ESP_IDF_VERSION_MAJOR) && ESP_IDF_VERSION_MAJOR >= 5
    return output->SetPinout(BCLK, WCLK, DOUT, MCLK);
#else
    return output->SetPinout(BCLK, WCLK, DOUT);
#endif
}

#if defined(HAS_NS4168_SPKR)

bool playAudioFile(FS *fs, String filepath) {
    if (!bruceConfig.soundEnabled) return false;

    AudioFileSource *source = new AudioFileSourceFS(*fs, filepath.c_str());
    if (!source) return false;

    AudioOutputI2S *audioout = new AudioOutputI2S(
    ); // https://github.com/earlephilhower/ESP8266Audio/blob/master/src/AudioOutputI2S.cpp#L32
    configureI2SPinout(audioout);
    
    // set volume, derived from https://github.com/earlephilhower/ESP8266Audio/blob/master/examples/WebRadio/WebRadio.ino
    audioout->SetGain(((float)bruceConfig.soundVolume) / 100.0);

    AudioGenerator *generator = NULL;

    // switch on extension
    filepath.toLowerCase(); // case-insensitive match
    if (filepath.endsWith(".txt") || filepath.endsWith(".rtttl")) generator = new AudioGeneratorRTTTL();
    if (filepath.endsWith(".wav")) generator = new AudioGeneratorWAV();
    if (filepath.endsWith(".mod")) generator = new AudioGeneratorMOD();
    if (filepath.endsWith(".opus")) generator = new AudioGeneratorOpus();
    if (filepath.endsWith(".aac")) generator = new AudioGeneratorAAC();
    if (filepath.endsWith(".flac")) generator = new AudioGeneratorFLAC();
    // OGG Vorbis is not supported https://github.com/earlephilhower/ESP8266Audio/issues/84
    if (filepath.endsWith(".mp3")) {
        generator = new AudioGeneratorMP3();
        source = new AudioFileSourceID3(source);
    }
    /* 2FIX: compilation issues
    if(filepath.endsWith(".mid"))  {
      // need to load a soundfont
      AudioFileSource* sf2 = NULL;
      if(setupSdCard()) sf2 = new AudioFileSourceFS(SD, "1mgm.sf2");  // TODO: make configurable
      if(!sf2) sf2 = new AudioFileSourceLittleFS(LittleFS, "1mgm.sf2");  // TODO: make configurable
      if(!sf2) return false;  // a soundfount was not found
      AudioGeneratorMIDI* midi = new AudioGeneratorMIDI();
      midi->SetSoundfont(sf2);
      generator = midi;
    } */
    if (generator && source && audioout) {
        Serial.println("Start audio");
        generator->begin(source, audioout);
        while (generator->isRunning()) {
            if (!generator->loop() || check(AnyKeyPress)) generator->stop();
        }
        audioout->stop();
        source->close();
        Serial.println("Stop audio");

        delete generator;
        delete source;
        delete audioout;

        return true;
    }
    // else
    return false; // init error
}

bool playAudioRTTTLString(String song) {
    if (!bruceConfig.soundEnabled) return false;

    // derived from
    // https://github.com/earlephilhower/ESP8266Audio/blob/master/examples/PlayRTTTLToI2SDAC/PlayRTTTLToI2SDAC.ino

    song.trim();
    if (song == "") return false;

    AudioOutputI2S *audioout = new AudioOutputI2S();
    configureI2SPinout(audioout);

    AudioGenerator *generator = new AudioGeneratorRTTTL();

    AudioFileSource *source = new AudioFileSourcePROGMEM(song.c_str(), song.length());

    if (generator && source && audioout) {
        Serial.println("Start audio");
        generator->begin(source, audioout);
        while (generator->isRunning()) {
            if (!generator->loop() || check(AnyKeyPress)) generator->stop();
        }
        audioout->stop();
        source->close();
        Serial.println("Stop audio");

        delete generator;
        delete source;
        delete audioout;

        return true;
    }
    // else
    return false; // init error
}

bool tts(String text) {
    if (!bruceConfig.soundEnabled) return false;

    text.trim();
    if (text == "") return false;

    AudioOutputI2S *audioout = new AudioOutputI2S();
    configureI2SPinout(audioout);

    // https://github.com/earlephilhower/ESP8266SAM/blob/master/examples/Speak/Speak.ino
    audioout->begin();
    ESP8266SAM *sam = new ESP8266SAM;
    sam->Say(audioout, text.c_str());
    delete sam;
    return true;
}

bool isAudioFile(String filepath) {

    return filepath.endsWith(".opus") || filepath.endsWith(".rtttl") || filepath.endsWith(".wav") ||
           filepath.endsWith(".mod") || filepath.endsWith(".mp3");
}

void playTone(unsigned int frequency, unsigned long duration, short waveType) {
    if (!bruceConfig.soundEnabled) return;

    // derived from
    // https://github.com/earlephilhower/ESP8266Audio/blob/master/examples/PlayWAVFromFunction/PlayWAVFromFunction.ino

    if (frequency == 0 || duration == 0) return;

    float hz = frequency;

    AudioGeneratorWAV *wav;
    AudioFileSourceFunction *file;
    AudioOutputI2S *out = new AudioOutputI2S();
    configureI2SPinout(out);

    file = new AudioFileSourceFunction(duration / 1000.0); // , 1, 44100
    //
    // you can set (sec, channels, hz, bit/sample) but you should care about
    // the trade-off between performance and the audio quality
    //
    // file = new AudioFileSourceFunction(sec, channels, hz, bit/sample);
    // channels   : default = 1
    // hz         : default = 8000 (8000, 11025, 22050, 44100, 48000, etc.)
    // bit/sample : default = 16 (8, 16, 32)

    // ===== set your sound function =====

    if (waveType == 0) { // square
        file->addAudioGenerators([&](const float time) {
            float v = (sin(hz * time) >= 0) ? 1.0f : -1.0f;
            ;         // generate square wave
            v *= 0.1; // scale
            return v;
        });
    } else if (waveType == 1) { // sine
        file->addAudioGenerators([&](const float time) {
            float v = sin(TWO_PI * hz * time); // generate sine wave
            v *= fmod(time, 1.f);              // change linear
            v *= 0.1;                          // scale
            return v;
        });
    }
    // TODO: more wave types: triangle, sawtooth
    //
    // sound function should have one argument(float) and one return(float)
    // param  : float (current time [sec] of the song)
    // return : float (the amplitude of sound which varies from -1.f to +1.f)

    wav = new AudioGeneratorWAV();
    wav->begin(file, out);

    while (wav->isRunning()) {
        if (!wav->loop() || check(AnyKeyPress)) wav->stop();
    }

    delete file;
    delete wav;
    delete out;
}

#endif

void _tone(unsigned int frequency, unsigned long duration) {
    if (!bruceConfig.soundEnabled) return;

#if defined(BUZZ_PIN)
    tone(BUZZ_PIN, frequency, duration);
#elif defined(HAS_NS4168_SPKR)
    //  alt. implementation using the speaker
    playTone(frequency, duration, 0);
#endif
}
