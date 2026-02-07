#include "audio.h"
#include "core/mykeyboard.h"

#if defined(HAS_NS4168_SPKR)
#include "AudioFileSourceFunction.h"
#include "AudioGeneratorAAC.h"
#include "AudioGeneratorFLAC.h"
#include "AudioGeneratorMIDI.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2SNoDAC.h"
#include <ESP8266Audio.h>
#include <ESP8266SAM.h>

void _setup_codec_speaker(bool enable) __attribute__((weak));
void _setup_codec_speaker(bool enable) {}

// Volume control constants
static const float AUDIO_VOLUME_SCALE = 0.1f;
static const float AUDIO_VOLUME_MAX = 100.0f;

// Task configuration
static const uint32_t AUDIO_TASK_STACK_SIZE = 16384; // 16KB - increased for complex MP3 files
static const UBaseType_t AUDIO_TASK_PRIORITY = 1;
static const BaseType_t AUDIO_TASK_CORE = 1; // Core 1

// ===== ASYNC PLAYBACK STATE =====
struct AudioPlayerState {
    // Playback objects
    AudioGenerator *generator;
    AudioFileSource *source;
    AudioOutputI2S *output;

    // State management
    PlaybackState state;
    PlaybackMode mode;
    String currentFile;

    // Control flags (volatile for thread safety)
    volatile bool stopRequested;
    volatile bool pauseRequested;
    volatile bool volumeChanged;
    volatile uint8_t newVolume;
    float currentGain; // Store current gain for pause fade

    // Timing info
    unsigned long startTime;
    unsigned long pausedTime;
    unsigned long totalPausedDuration;

    // Task handle
    TaskHandle_t taskHandle;

    // Mutex for thread safety
    SemaphoreHandle_t mutex;

    AudioPlayerState()
        : generator(nullptr), source(nullptr), output(nullptr), state(PLAYBACK_IDLE), mode(PLAYBACK_BLOCKING),
          currentFile(""), stopRequested(false), pauseRequested(false), volumeChanged(false), newVolume(0),
          currentGain(1.0f), startTime(0), pausedTime(0), totalPausedDuration(0), taskHandle(nullptr),
          mutex(nullptr) {
        mutex = xSemaphoreCreateMutex();
    }

    ~AudioPlayerState() {
        // Ensure task is stopped before destruction
        if (taskHandle != nullptr) {
            stopRequested = true;
            // Wait with timeout
            for (int i = 0; i < 200; i++) { // 2 seconds max
                if (taskHandle == nullptr) break;
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }

        cleanup();

        if (mutex) {
            vSemaphoreDelete(mutex);
            mutex = nullptr;
        }
    }

    void cleanup() {
        if (generator) {
            if (generator->isRunning()) { generator->stop(); }
            delete generator;
            generator = nullptr;
        }
        if (source) {
            source->close();
            delete source;
            source = nullptr;
        }
        if (output) {
            output->stop();
            delete output;
            output = nullptr;
        }
    }

    bool lock(TickType_t timeout = portMAX_DELAY) {
        if (mutex) { return xSemaphoreTake(mutex, timeout) == pdTRUE; }
        return false;
    }

    void unlock() {
        if (mutex) { xSemaphoreGive(mutex); }
    }
};

// Global player state
static AudioPlayerState *g_audioPlayer = nullptr;

// Initialize player state
static void initAudioPlayer() {
    if (!g_audioPlayer) { g_audioPlayer = new AudioPlayerState(); }
}

// Internal helper - unified audio output creation
static AudioOutputI2S *createConfiguredAudioOutput() {
    AudioOutputI2S *audioout = new AudioOutputI2S();

    if (!audioout) {
        Serial.println("ERROR: AudioOutputI2S allocation failed");
        return nullptr;
    }

    audioout->SetPinout(BCLK, WCLK, DOUT, MCLK);
    audioout->SetGain(bruceConfig.soundVolume / AUDIO_VOLUME_MAX);

    return audioout;
}

// Helper to validate file existence
static bool validateAudioFile(FS *fs, const String &filepath) {
    if (!fs) {
        Serial.println("ERROR: Invalid filesystem pointer");
        return false;
    }

    if (!fs->exists(filepath.c_str())) {
        Serial.print("ERROR: Audio file not found: ");
        Serial.println(filepath);
        return false;
    }

    File file = fs->open(filepath.c_str(), "r");
    if (!file) {
        Serial.println("ERROR: Cannot open audio file");
        return false;
    }

    bool isFile = !file.isDirectory();
    size_t fileSize = file.size();
    file.close();

    if (!isFile) {
        Serial.println("ERROR: Path is a directory, not a file");
        return false;
    }

    if (fileSize == 0) {
        Serial.println("ERROR: Audio file is empty");
        return false;
    }

    return true;
}

// ===== ASYNC PLAYBACK TASK =====
static void audioPlaybackTask(void *parameter) {
    if (!g_audioPlayer) {
        vTaskDelete(NULL);
        return;
    }

    AudioPlayerState *player = g_audioPlayer;

    Serial.println("Audio playback task started");

    // Main playback loop
    while (player->generator && player->generator->isRunning()) {
        // Check for stop request (prioritized check)
        if (player->stopRequested) {
            Serial.println("Stop requested");
            break;
        }

        // Check for pause request
        if (player->pauseRequested) {
            if (player->lock(pdMS_TO_TICKS(100))) {
                if (player->state == PLAYBACK_PLAYING) {
                    // Fade out volume to prevent pop/crackling
                    for (int i = 10; i >= 0; i--) {
                        player->output->SetGain(player->currentGain * i / 10.0f);
                        vTaskDelay(pdMS_TO_TICKS(8));
                    }

                    // Stop I2S output to prevent buffer loop
                    player->output->stop();

                    player->state = PLAYBACK_PAUSED;
                    player->pausedTime = millis();
                    Serial.println("Playback paused - I2S stopped");
                } else if (player->state == PLAYBACK_PAUSED) {
                    // Restart I2S output
                    if (!player->output->begin()) {
                        Serial.println("ERROR: Failed to restart I2S output");
                        player->pauseRequested = false;
                        player->unlock();
                        break; // Exit task if can't restart
                    }

                    // Restore volume with fade in
                    for (int i = 0; i <= 10; i++) {
                        player->output->SetGain(player->currentGain * i / 10.0f);
                        vTaskDelay(pdMS_TO_TICKS(8));
                    }

                    player->state = PLAYBACK_PLAYING;
                    player->totalPausedDuration += millis() - player->pausedTime;
                    Serial.println("Playback resumed - I2S restarted");
                }
                player->pauseRequested = false;
                player->unlock();
            }
        }

        // Check for volume change request
        if (player->volumeChanged && player->output) {
            if (player->lock(pdMS_TO_TICKS(50))) {
                player->currentGain = player->newVolume / AUDIO_VOLUME_MAX;
                player->output->SetGain(player->currentGain);
                player->volumeChanged = false;
                player->unlock();
                Serial.print("Volume changed to: ");
                Serial.println(player->newVolume);
            }
        }

        // Only loop generator when not paused
        if (player->state == PLAYBACK_PLAYING) {
            if (!player->generator->loop()) {
                Serial.println("Generator loop ended");
                break;
            }
        } else {
            // Paused - yield to prevent busy waiting
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        // Yield to prevent watchdog timeout
        taskYIELD();
    }

    // Cleanup after playback
    Serial.println("Audio playback task ending");

    // Use timeout on lock to prevent deadlock
    if (player->lock(pdMS_TO_TICKS(1000))) {
        player->state = PLAYBACK_STOPPING;

        if (player->output) player->output->stop();
        if (player->source) player->source->close();

        player->cleanup();
        player->state = PLAYBACK_IDLE;
        player->currentFile = "";

        // Clear task handle BEFORE unlocking
        player->taskHandle = nullptr;

        player->unlock();
    } else {
        // Couldn't acquire lock - force cleanup anyway
        Serial.println("WARNING: Force cleanup without lock");
        player->taskHandle = nullptr;
    }

    _setup_codec_speaker(false);

    // Delete task (must be last operation)
    vTaskDelete(NULL);
}
// ===== PUBLIC API IMPLEMENTATION =====

bool stopAudioPlayback() {
    initAudioPlayer();

    if (!g_audioPlayer) return false;

    // Quick check without lock
    if (g_audioPlayer->state == PLAYBACK_IDLE) { return false; }

    // Signal stop
    g_audioPlayer->stopRequested = true;

    // Wait for task to finish with timeout (2 seconds)
    unsigned long startWait = millis();
    while (g_audioPlayer->taskHandle != nullptr && (millis() - startWait) < 2000) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Force cleanup if timeout
    if (g_audioPlayer->taskHandle != nullptr) {
        Serial.println("WARNING: Stop timeout - forcing cleanup");
        if (g_audioPlayer->lock(pdMS_TO_TICKS(500))) {
            g_audioPlayer->cleanup();
            g_audioPlayer->state = PLAYBACK_IDLE;
            g_audioPlayer->taskHandle = nullptr;
            g_audioPlayer->unlock();
        }
    }

    return true;
}

bool pauseAudioPlayback() {
    initAudioPlayer();

    if (!g_audioPlayer) return false;

    PlaybackState currentState = g_audioPlayer->state;
    if (currentState != PLAYBACK_PLAYING && currentState != PLAYBACK_PAUSED) { return false; }

    g_audioPlayer->pauseRequested = true;

    return true;
}

bool isAudioPlaying() {
    initAudioPlayer();

    if (!g_audioPlayer) return false;

    PlaybackState state = g_audioPlayer->state;
    return (state == PLAYBACK_PLAYING || state == PLAYBACK_PAUSED);
}

AudioPlaybackInfo getAudioPlaybackInfo() {
    initAudioPlayer();

    AudioPlaybackInfo info;
    info.state = PLAYBACK_IDLE;
    info.currentFile = "";
    info.duration = 0;
    info.position = 0;
    info.volume = bruceConfig.soundVolume;
    info.isAsyncMode = false;

    if (!g_audioPlayer) return info;

    if (g_audioPlayer->lock(pdMS_TO_TICKS(100))) {
        info.state = g_audioPlayer->state;
        info.currentFile = g_audioPlayer->currentFile;
        info.volume = bruceConfig.soundVolume;
        info.isAsyncMode = (g_audioPlayer->mode == PLAYBACK_ASYNC);

        // Calculate position
        if (g_audioPlayer->state == PLAYBACK_PLAYING) {
            info.position = millis() - g_audioPlayer->startTime - g_audioPlayer->totalPausedDuration;
        } else if (g_audioPlayer->state == PLAYBACK_PAUSED) {
            info.position =
                g_audioPlayer->pausedTime - g_audioPlayer->startTime - g_audioPlayer->totalPausedDuration;
        }

        g_audioPlayer->unlock();
    }

    return info;
}

void setAudioPlaybackVolume(uint8_t volume) {
    if (volume > 100) volume = 100;

    // Update config
    bruceConfig.setSoundVolume(volume);

    initAudioPlayer();

    // If playing, signal volume change to task
    if (g_audioPlayer && g_audioPlayer->state != PLAYBACK_IDLE) {
        g_audioPlayer->newVolume = volume;
        g_audioPlayer->volumeChanged = true;
    }
}

// ===== HELPER: Start async playback task =====
static bool startAsyncPlayback(
    AudioGenerator *generator, AudioFileSource *source, AudioOutputI2S *output, const String &filename
) {
    initAudioPlayer();

    if (!g_audioPlayer->lock(pdMS_TO_TICKS(1000))) {
        Serial.println("ERROR: Could not acquire lock for async playback");
        delete generator;
        delete source;
        delete output;
        _setup_codec_speaker(false);
        return false;
    }

    // Set up state
    g_audioPlayer->generator = generator;
    g_audioPlayer->source = source;
    g_audioPlayer->output = output;
    g_audioPlayer->state = PLAYBACK_PLAYING;
    g_audioPlayer->mode = PLAYBACK_ASYNC;
    g_audioPlayer->currentFile = filename;
    g_audioPlayer->stopRequested = false;
    g_audioPlayer->pauseRequested = false;
    g_audioPlayer->volumeChanged = false;
    g_audioPlayer->currentGain = bruceConfig.soundVolume / AUDIO_VOLUME_MAX; // Store initial gain
    g_audioPlayer->startTime = millis();
    g_audioPlayer->pausedTime = 0;
    g_audioPlayer->totalPausedDuration = 0;

    g_audioPlayer->unlock();

    // Create task
    BaseType_t result = xTaskCreatePinnedToCore(
        audioPlaybackTask,
        "AudioPlayback",
        AUDIO_TASK_STACK_SIZE,
        NULL,
        AUDIO_TASK_PRIORITY,
        &g_audioPlayer->taskHandle,
        AUDIO_TASK_CORE
    );

    if (result != pdPASS || !g_audioPlayer->taskHandle) {
        Serial.println("ERROR: Failed to create playback task");

        // Cleanup on failure
        if (g_audioPlayer->lock(pdMS_TO_TICKS(500))) {
            g_audioPlayer->cleanup();
            g_audioPlayer->state = PLAYBACK_IDLE;
            g_audioPlayer->taskHandle = nullptr;
            g_audioPlayer->unlock();
        }

        _setup_codec_speaker(false);
        return false;
    }

    return true;
}

// ===== CORE PLAYBACK FUNCTIONS =====

bool playAudioFile(FS *fs, String filepath, PlaybackMode mode) {
    if (!bruceConfig.soundEnabled) return false;

    if (!validateAudioFile(fs, filepath)) { return false; }

    // Stop any current playback
    if (isAudioPlaying()) { stopAudioPlayback(); }

    _setup_codec_speaker(true);

    AudioFileSource *source = new AudioFileSourceFS(*fs, filepath.c_str());
    if (!source) {
        Serial.println("ERROR: Failed to create audio source");
        _setup_codec_speaker(false);
        return false;
    }

    AudioOutputI2S *audioout = createConfiguredAudioOutput();
    if (!audioout) {
        delete source;
        _setup_codec_speaker(false);
        return false;
    }

    AudioGenerator *generator = NULL;

    filepath.toLowerCase();
    if (filepath.endsWith(".txt") || filepath.endsWith(".rtttl")) generator = new AudioGeneratorRTTTL();
    else if (filepath.endsWith(".wav")) generator = new AudioGeneratorWAV();
    else if (filepath.endsWith(".mod")) generator = new AudioGeneratorMOD();
    else if (filepath.endsWith(".opus")) generator = new AudioGeneratorOpus();
    else if (filepath.endsWith(".aac")) generator = new AudioGeneratorAAC();
    else if (filepath.endsWith(".flac")) generator = new AudioGeneratorFLAC();
    else if (filepath.endsWith(".mp3")) {
        generator = new AudioGeneratorMP3();
        source = new AudioFileSourceID3(source);
    }

    if (!generator) {
        Serial.println("ERROR: Unsupported audio format");
        delete source;
        delete audioout;
        _setup_codec_speaker(false);
        return false;
    }

    if (!generator->begin(source, audioout)) {
        Serial.println("ERROR: Failed to begin audio playback");
        delete generator;
        delete source;
        delete audioout;
        _setup_codec_speaker(false);
        return false;
    }

    // === BLOCKING MODE ===
    if (mode == PLAYBACK_BLOCKING) {
        Serial.println("Start audio (blocking)");

        while (generator->isRunning()) {
            if (!generator->loop() || check(AnyKeyPress)) { generator->stop(); }
        }

        audioout->stop();
        source->close();
        Serial.println("Stop audio");

        delete generator;
        delete source;
        delete audioout;

        _setup_codec_speaker(false);
        return true;
    }

    // === ASYNC MODE ===
    Serial.println("Start audio (async)");
    return startAsyncPlayback(generator, source, audioout, filepath);
}

bool playAudioRTTTLString(String song, PlaybackMode mode) {
    if (!bruceConfig.soundEnabled) return false;

    song.trim();
    if (song == "") {
        Serial.println("ERROR: Empty RTTTL string");
        return false;
    }

    if (isAudioPlaying()) { stopAudioPlayback(); }

    _setup_codec_speaker(true);

    AudioOutputI2S *audioout = createConfiguredAudioOutput();
    if (!audioout) {
        _setup_codec_speaker(false);
        return false;
    }

    AudioGenerator *generator = new AudioGeneratorRTTTL();
    if (!generator) {
        delete audioout;
        _setup_codec_speaker(false);
        return false;
    }

    AudioFileSource *source = new AudioFileSourcePROGMEM(song.c_str(), song.length());
    if (!source) {
        delete generator;
        delete audioout;
        _setup_codec_speaker(false);
        return false;
    }

    if (!generator->begin(source, audioout)) {
        Serial.println("ERROR: Failed to begin RTTTL playback");
        delete generator;
        delete source;
        delete audioout;
        _setup_codec_speaker(false);
        return false;
    }

    // === BLOCKING MODE ===
    if (mode == PLAYBACK_BLOCKING) {
        Serial.println("Start RTTTL (blocking)");

        while (generator->isRunning()) {
            if (!generator->loop() || check(AnyKeyPress)) { generator->stop(); }
        }

        audioout->stop();
        source->close();
        Serial.println("Stop audio");

        delete generator;
        delete source;
        delete audioout;

        _setup_codec_speaker(false);
        return true;
    }

    // === ASYNC MODE ===
    Serial.println("Start RTTTL (async)");
    return startAsyncPlayback(generator, source, audioout, "RTTTL");
}
bool tts(String text, PlaybackMode mode) {
    if (!bruceConfig.soundEnabled) return false;

    text.trim();
    if (text == "") {
        Serial.println("ERROR: Empty TTS text");
        return false;
    }

    // TTS is always blocking - ESP8266SAM doesn't support async playback
    if (mode == PLAYBACK_ASYNC) { Serial.println("WARNING: TTS doesn't support async mode, using blocking"); }

    // Stop any current playback
    if (isAudioPlaying()) { stopAudioPlayback(); }

    _setup_codec_speaker(true);

    AudioOutputI2S *audioout = createConfiguredAudioOutput();
    if (!audioout) {
        _setup_codec_speaker(false);
        return false;
    }

    if (!audioout->begin()) {
        Serial.println("ERROR: Failed to initialize audio output for TTS");
        delete audioout;
        _setup_codec_speaker(false);
        return false;
    }

    ESP8266SAM *sam = new ESP8266SAM;
    if (!sam) {
        delete audioout;
        _setup_codec_speaker(false);
        return false;
    }

    // TTS synthesis (always blocking)
    sam->Say(audioout, text.c_str());

    delete sam;
    delete audioout;

    _setup_codec_speaker(false);
    return true;
}

bool isAudioFile(String filepath) {
    return filepath.endsWith(".opus") || filepath.endsWith(".rtttl") || filepath.endsWith(".txt") ||
           filepath.endsWith(".wav") || filepath.endsWith(".mod") || filepath.endsWith(".mp3") ||
           filepath.endsWith(".aac") || filepath.endsWith(".flac");
}

void playTone(unsigned int frequency, unsigned long duration, short waveType) {
    if (!bruceConfig.soundEnabled) return;

    _setup_codec_speaker(true);

    if (frequency == 0 || duration == 0) {
        _setup_codec_speaker(false);
        return;
    }

    float hz = frequency;

    AudioOutputI2S *out = createConfiguredAudioOutput();
    if (!out) {
        _setup_codec_speaker(false);
        return;
    }

    AudioFileSourceFunction *file = new AudioFileSourceFunction(duration / 1000.0);
    if (!file) {
        delete out;
        _setup_codec_speaker(false);
        return;
    }

    float volumeScale = (bruceConfig.soundVolume / AUDIO_VOLUME_MAX) * AUDIO_VOLUME_SCALE;

    if (waveType == 0) {
        file->addAudioGenerators([volumeScale, hz](const float time) {
            float v = (sin(hz * time) >= 0) ? 1.0f : -1.0f;
            v *= volumeScale;
            return v;
        });
    } else if (waveType == 1) {
        file->addAudioGenerators([volumeScale, hz](const float time) {
            float v = sin(TWO_PI * hz * time);
            v *= fmod(time, 1.f);
            v *= volumeScale;
            return v;
        });
    }

    AudioGeneratorWAV *wav = new AudioGeneratorWAV();
    if (!wav) {
        delete file;
        delete out;
        _setup_codec_speaker(false);
        return;
    }

    if (!wav->begin(file, out)) {
        Serial.println("ERROR: Failed to begin tone playback");
        delete wav;
        delete file;
        delete out;
        _setup_codec_speaker(false);
        return;
    }

    while (wav->isRunning()) {
        if (!wav->loop() || check(AnyKeyPress)) { wav->stop(); }
    }

    delete file;
    delete wav;
    delete out;

    _setup_codec_speaker(false);
}

#endif

void _tone(unsigned int frequency, unsigned long duration) {
    if (!bruceConfig.soundEnabled) return;

#if defined(BUZZ_PIN)
    tone(BUZZ_PIN, frequency, duration);
#elif defined(HAS_NS4168_SPKR)
    playTone(frequency, duration, 0);
#endif
}
