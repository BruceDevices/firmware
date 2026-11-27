#pragma once

// Frank Vader custom edition of Bruce.
// Enabled by the fv_* PlatformIO environments in boards/frank_vader.ini.

#ifndef FV_EDITION
#define FV_EDITION "Bruce Force"
#endif

#ifndef FV_VERSION
#define FV_VERSION "FV-BF-1.0"
#endif

void fv_show_profile();
void fv_show_character();
