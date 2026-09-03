#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESL_TEXT_FULL_JOB_PIXEL_LIMIT 49152u

bool esl_text_should_send_full(uint16_t w, uint16_t h, bool second_plane);
uint16_t esl_text_chunk_height(uint16_t w, uint16_t h, bool second_plane);
uint32_t esl_text_chunk_settle_ms(uint16_t w, uint16_t h, bool color_clear);

typedef struct {
    const uint8_t *primary;
    const uint8_t *secondary; /* NULL → plane 1 is encoder-clear (0) */
} EslTextColor26Ctx;

/* Wire idx → glass via proto_to_glass. Font 1 (clear) encodes 0. */
uint8_t esl_text_color26_pixel(size_t idx, void *ctx);

#ifdef __cplusplus
}
#endif
