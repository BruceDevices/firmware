#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESL_TEXT_FULL_JOB_PIXEL_LIMIT 49152u

bool esl_text_should_send_full(uint16_t w, uint16_t h, bool second_plane);
uint16_t esl_text_chunk_height(uint16_t w, uint16_t h, bool second_plane);
uint32_t esl_text_chunk_settle_ms(uint16_t w, uint16_t h, bool color_clear);

#ifdef __cplusplus
}
#endif
