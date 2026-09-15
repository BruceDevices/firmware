#ifndef __MUSICPLAYER_H
#define __MUSICPLAYER_H

#include "core/display.h"
#include <FS.h>

// Senape3000
// Need better extension parsing, total duration functions in audio.cpp, better UI...
// Need better UI managements and so much more...

/**
 * @brief Music Player UI
 * @param fs Filesystem pointer (SD or LittleFS)
 * @param filepath Path to audio file
 * @note Supports: MP3, WAV, FLAC, AAC, OPUS, MOD, RTTTL
 */
void musicPlayerUI(FS *fs, const String &filepath);

#endif // __MUSICPLAYER_H
