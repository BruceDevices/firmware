#ifndef __RAM_PROFILE_H__
#define __RAM_PROFILE_H__

// Usage: RAM_LOG("stage-name"); at each boot stage you want to measure.

#if defined(ENABLE_RAM_LOGGING)

// Logs, over Serial, the current heap/PSRAM state and elapsed time for a named
// boot stage. Focus is on INTERNAL DRAM (free + largest contiguous block),
// which is what Wi-Fi/BLE need and what gets exhausted on non-PSRAM boards.
void ramProfileLog(const char *stage);

#define RAM_LOG(stage) ramProfileLog(stage)

#else

#define RAM_LOG(stage) ((void)0)

#endif // ENABLE_RAM_LOGGING

#endif // __RAM_PROFILE_H__
