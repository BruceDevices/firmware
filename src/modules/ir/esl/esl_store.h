#pragma once

#include "esl_proto.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESL_STORE_MAX_TARGETS 16
#define ESL_STORE_NAME_LEN 16
#define ESL_STORE_MAX_RECENTS 6
#define ESL_STORE_TEXT_LEN 32
#define ESL_STORE_BC_LEN 17

typedef struct {
    bool show_startup_warning;
    uint8_t data_frame_repeats; /* 1–10 */
} EslSettings;

typedef struct {
    char barcode[ESL_STORE_BC_LEN + 1];
    char name[ESL_STORE_NAME_LEN + 1];
    uint8_t plid[4];
    TagTinkerTagProfile profile;
} EslTarget;

typedef struct {
    uint16_t width, height;
    uint8_t page, padding;
    bool invert, color_clear;
    char text[ESL_STORE_TEXT_LEN];
} EslRecent;

typedef struct {
    EslSettings settings;
    EslTarget targets[ESL_STORE_MAX_TARGETS];
    uint8_t target_count;
    int8_t selected_target; /* -1 none */
    EslRecent recents[ESL_STORE_MAX_RECENTS];
    uint8_t recent_count;
    /* last text-compose fields used when adding a recent */
    uint16_t esl_width, esl_height;
    uint8_t img_page, text_padding_pct;
    bool invert_text, color_clear;
} EslSession;

void esl_store_session_init(EslSession *s);
void esl_store_settings_defaults(EslSettings *s);
bool esl_store_settings_parse(const char *text, EslSettings *s);
bool esl_store_settings_format(const EslSettings *s, char *out, size_t cap);
bool esl_store_targets_parse(const char *text, EslSession *s);
bool esl_store_targets_format(const EslSession *s, char *out, size_t cap);
int esl_store_find_target(const EslSession *s, const char *barcode); /* -1 miss */
int esl_store_ensure_target(EslSession *s, const char *barcode); /* index or -1 */
bool esl_store_delete_target(EslSession *s, uint8_t index);
void esl_store_set_default_name(EslSession *s, EslTarget *t);
bool esl_store_recents_parse(const char *text, EslSession *s);
bool esl_store_recents_format(const EslSession *s, char *out, size_t cap);
void esl_store_recents_add(EslSession *s, const char *text);

#ifdef __cplusplus
}
#endif
