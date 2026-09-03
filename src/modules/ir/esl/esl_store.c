#include "esl_store.h"

#include <stdio.h>
#include <string.h>

void esl_store_settings_defaults(EslSettings *s) {
    if (s == NULL) return;
    s->show_startup_warning = true;
    s->data_frame_repeats = 2u;
}

void esl_store_session_init(EslSession *s) {
    if (s == NULL) return;
    memset(s, 0, sizeof(*s));
    esl_store_settings_defaults(&s->settings);
    s->selected_target = -1;
    s->img_page = 1u;
    s->esl_width = 200u;
    s->esl_height = 80u;
}

void esl_store_set_default_name(EslSession *s, EslTarget *t) {
    if (s == NULL || t == NULL) return;
    snprintf(t->name, sizeof(t->name), "tag%d", (int)s->target_count + 1);
}

static void apply_repeats(EslSettings *s, int rep) {
    /* Flipper stores then clamps the unsigned field, so a negative parse
     * becomes a large value and lands on 10, not 1. */
    s->data_frame_repeats = (uint8_t)rep;
    if (s->data_frame_repeats < 1u) s->data_frame_repeats = 1u;
    if (s->data_frame_repeats > 10u) s->data_frame_repeats = 10u;
}

bool esl_store_settings_parse(const char *text, EslSettings *s) {
    if (text == NULL || s == NULL) return false;

    int warn = 0;
    int rep = 0;
    int sig = 0;
    if (sscanf(text, "%d|%d|%d", &warn, &rep, &sig) == 3) {
        s->show_startup_warning = (warn != 0);
        apply_repeats(s, rep);
    } else if (sscanf(text, "%d|%d", &warn, &rep) == 2) {
        s->show_startup_warning = (warn != 0);
        apply_repeats(s, rep);
    } else {
        return false;
    }
    (void)sig;
    return true;
}

bool esl_store_settings_format(const EslSettings *s, char *out, size_t cap) {
    if (s == NULL || out == NULL || cap == 0u) return false;
    int n = snprintf(out, cap, "%d|%u|0", s->show_startup_warning ? 1 : 0,
                     (unsigned)s->data_frame_repeats);
    return n > 0 && (size_t)n < cap;
}

static void refresh_profile(EslTarget *t) {
    memset(&t->profile, 0, sizeof(t->profile));
    tagtinker_barcode_to_profile(t->barcode, &t->profile);
}

bool esl_store_targets_parse(const char *text, EslSession *s) {
    if (text == NULL || s == NULL) return false;

    s->target_count = 0u;
    const char *p = text;
    while (*p != '\0' && s->target_count < ESL_STORE_MAX_TARGETS) {
        const char *eol = p;
        while (*eol != '\0' && *eol != '\n') eol++;

        size_t linelen = (size_t)(eol - p);
        if (linelen > 0u) {
            char line[128];
            if (linelen >= sizeof(line)) linelen = sizeof(line) - 1u;
            memcpy(line, p, linelen);
            line[linelen] = '\0';

            char *sep = strchr(line, '|');
            if (sep != NULL) *sep = '\0';

            EslTarget *t = &s->targets[s->target_count];
            if (tagtinker_barcode_to_plid(line, t->plid)) {
                strncpy(t->barcode, line, ESL_STORE_BC_LEN);
                t->barcode[ESL_STORE_BC_LEN] = '\0';
                memset(t->name, 0, sizeof(t->name));
                if (sep != NULL && *(sep + 1) != '\0') {
                    strncpy(t->name, sep + 1, ESL_STORE_NAME_LEN);
                    t->name[ESL_STORE_NAME_LEN] = '\0';
                } else {
                    esl_store_set_default_name(s, t);
                }
                refresh_profile(t);
                s->target_count++;
            }
        }

        p = (*eol == '\n') ? eol + 1 : eol;
    }
    return true;
}

bool esl_store_targets_format(const EslSession *s, char *out, size_t cap) {
    if (s == NULL || out == NULL || cap == 0u) return false;

    size_t used = 0u;
    out[0] = '\0';
    for (uint8_t i = 0u; i < s->target_count; i++) {
        int n = snprintf(out + used, cap - used, "%s|%s\n", s->targets[i].barcode,
                         s->targets[i].name);
        if (n < 0 || (size_t)n >= cap - used) return false;
        used += (size_t)n;
    }
    return true;
}

int esl_store_find_target(const EslSession *s, const char *barcode) {
    if (s == NULL || barcode == NULL || barcode[0] == '\0') return -1;
    for (uint8_t i = 0u; i < s->target_count; i++) {
        if (strcmp(s->targets[i].barcode, barcode) == 0) return (int)i;
    }
    return -1;
}

int esl_store_ensure_target(EslSession *s, const char *barcode) {
    if (s == NULL || barcode == NULL) return -1;

    int existing = esl_store_find_target(s, barcode);
    if (existing >= 0) return existing;
    if (s->target_count >= ESL_STORE_MAX_TARGETS) return -1;

    EslTarget *t = &s->targets[s->target_count];
    memset(t, 0, sizeof(*t));
    strncpy(t->barcode, barcode, ESL_STORE_BC_LEN);
    t->barcode[ESL_STORE_BC_LEN] = '\0';

    if (!tagtinker_barcode_to_plid(t->barcode, t->plid)) {
        memset(t, 0, sizeof(*t));
        return -1;
    }

    esl_store_set_default_name(s, t);
    refresh_profile(t);
    s->target_count++;
    return (int)(s->target_count - 1u);
}

bool esl_store_delete_target(EslSession *s, uint8_t index) {
    if (s == NULL || index >= s->target_count) return false;

    if (index + 1u < s->target_count) {
        memmove(&s->targets[index], &s->targets[index + 1u],
                sizeof(EslTarget) * (size_t)(s->target_count - index - 1u));
    }
    memset(&s->targets[s->target_count - 1u], 0, sizeof(EslTarget));
    s->target_count--;
    s->selected_target = -1;
    return true;
}

bool esl_store_recents_parse(const char *text, EslSession *s) {
    if (text == NULL || s == NULL) return false;

    s->recent_count = 0u;
    const char *p = text;
    while (*p != '\0' && s->recent_count < ESL_STORE_MAX_RECENTS) {
        const char *eol = p;
        while (*eol != '\0' && *eol != '\n') eol++;

        size_t linelen = (size_t)(eol - p);
        if (linelen > 0u) {
            char line[128];
            if (linelen >= sizeof(line)) linelen = sizeof(line) - 1u;
            memcpy(line, p, linelen);
            line[linelen] = '\0';

            unsigned w = 0u, h = 0u, pg = 0u, inv = 0u, clr = 0u, pad = 0u,
                     sig = 0u;
            int parsed =
                sscanf(line, "%u|%u|%u|%u|%u|%u|%u|", &w, &h, &pg, &inv, &clr,
                       &pad, &sig);
            if (parsed >= 6) {
                char *q = line;
                int pipes = 0;
                int wanted = (parsed >= 7) ? 7 : 6;
                while (*q != '\0' && pipes < wanted) {
                    if (*q == '|') pipes++;
                    q++;
                }
                if (pipes == wanted) {
                    uint8_t idx = s->recent_count++;
                    s->recents[idx].width = (uint16_t)w;
                    s->recents[idx].height = (uint16_t)h;
                    s->recents[idx].page = (uint8_t)pg;
                    s->recents[idx].invert = (inv != 0u);
                    s->recents[idx].color_clear = (clr != 0u);
                    s->recents[idx].padding = (uint8_t)pad;
                    memset(s->recents[idx].text, 0, sizeof(s->recents[idx].text));
                    strncpy(s->recents[idx].text, q, ESL_STORE_TEXT_LEN - 1u);
                    s->recents[idx].text[ESL_STORE_TEXT_LEN - 1u] = '\0';
                }
            }
            (void)sig;
        }

        p = (*eol == '\n') ? eol + 1 : eol;
    }
    return true;
}

bool esl_store_recents_format(const EslSession *s, char *out, size_t cap) {
    if (s == NULL || out == NULL || cap == 0u) return false;

    size_t used = 0u;
    out[0] = '\0';
    for (uint8_t i = 0u; i < s->recent_count; i++) {
        int n = snprintf(out + used, cap - used, "%u|%u|%u|%d|%d|%u|%u|%s\n",
                         (unsigned)s->recents[i].width,
                         (unsigned)s->recents[i].height,
                         (unsigned)s->recents[i].page,
                         s->recents[i].invert ? 1 : 0,
                         s->recents[i].color_clear ? 1 : 0,
                         (unsigned)s->recents[i].padding, 0u,
                         s->recents[i].text);
        if (n < 0 || (size_t)n >= cap - used) return false;
        used += (size_t)n;
    }
    return true;
}

void esl_store_recents_add(EslSession *s, const char *text) {
    if (s == NULL || text == NULL || text[0] == '\0') return;

    int existing = -1;
    for (uint8_t i = 0u; i < s->recent_count; i++) {
        if (strcmp(s->recents[i].text, text) == 0 &&
            s->recents[i].width == s->esl_width &&
            s->recents[i].height == s->esl_height) {
            existing = (int)i;
            break;
        }
    }

    if (existing >= 0) {
        if (existing > 0) {
            EslRecent tmp = s->recents[existing];
            memmove(&s->recents[1], &s->recents[0],
                    sizeof(EslRecent) * (size_t)existing);
            s->recents[0] = tmp;
        }
        return;
    }

    if (s->recent_count < ESL_STORE_MAX_RECENTS) {
        s->recent_count++;
    }
    memmove(&s->recents[1], &s->recents[0],
            sizeof(EslRecent) * (size_t)(s->recent_count - 1u));
    s->recents[0].width = s->esl_width;
    s->recents[0].height = s->esl_height;
    s->recents[0].page = s->img_page;
    s->recents[0].invert = s->invert_text;
    s->recents[0].color_clear = s->color_clear;
    s->recents[0].padding = s->text_padding_pct;
    memset(s->recents[0].text, 0, sizeof(s->recents[0].text));
    strncpy(s->recents[0].text, text, ESL_STORE_TEXT_LEN - 1u);
    s->recents[0].text[ESL_STORE_TEXT_LEN - 1u] = '\0';
}
