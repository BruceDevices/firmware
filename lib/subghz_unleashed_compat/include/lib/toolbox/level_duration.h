#ifndef __SUBGHZ_COMPAT_LEVEL_DURATION_H__
#define __SUBGHZ_COMPAT_LEVEL_DURATION_H__

#include <stdbool.h>
#include <stdint.h>

#define LEVEL_DURATION_RESET 0xFFFFFFFFu
#define LEVEL_DURATION_WAIT 0xFFFFFFFEu
#define LEVEL_DURATION_LEVEL_LOW false
#define LEVEL_DURATION_LEVEL_HIGH true

typedef struct {
    bool level;
    uint32_t duration;
} LevelDuration;

static inline LevelDuration level_duration_make(bool level, uint32_t duration) {
    LevelDuration ld = {.level = level, .duration = duration};
    return ld;
}

static inline LevelDuration level_duration_reset(void) {
    LevelDuration ld = {.level = false, .duration = LEVEL_DURATION_RESET};
    return ld;
}

static inline LevelDuration level_duration_wait(void) {
    LevelDuration ld = {.level = false, .duration = LEVEL_DURATION_WAIT};
    return ld;
}

static inline bool level_duration_get_level(LevelDuration ld) { return ld.level; }

#endif
