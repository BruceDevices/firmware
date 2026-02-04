#include <SPIFFS.h>
// Keep SPIFFS first

// Playback modes
enum PlaybackMode {
    PLAYBACK_BLOCKING = 0, // Original behavior
    PLAYBACK_ASYNC = 1     // Non-blocking (runs in task)
};

// Playback state
enum PlaybackState {
    PLAYBACK_IDLE = 0,
    PLAYBACK_PLAYING,
    PLAYBACK_PAUSED,
    PLAYBACK_STOPPING,
    PLAYBACK_ERROR
};

// Playback info structure (for GUI)
struct AudioPlaybackInfo {
    PlaybackState state;
    String currentFile;
    unsigned long duration; // Total duration in ms (0 if unknown) ACTUALLY NOT IMPLEMENTED
    unsigned long position; // Current position in ms
    uint8_t volume;
    bool isAsyncMode;
};

// Existing functions
bool playAudioFile(FS *fs, String filepath, PlaybackMode mode = PLAYBACK_BLOCKING);
bool playAudioRTTTLString(String song, PlaybackMode mode = PLAYBACK_BLOCKING);
bool tts(String text, PlaybackMode mode = PLAYBACK_BLOCKING);
bool isAudioFile(String filePath);
void playTone(unsigned int frequency, unsigned long duration = 0UL, short waveType = 0);
void _tone(unsigned int frequency, unsigned long duration = 0UL);

// NEW: Async playback control API
bool stopAudioPlayback();                    // Stop current playback
bool pauseAudioPlayback();                   // Pause/resume toggle
bool isAudioPlaying();                       // Check if audio is playing
AudioPlaybackInfo getAudioPlaybackInfo();    // Get current state for GUI
void setAudioPlaybackVolume(uint8_t volume); // Change volume during playback
