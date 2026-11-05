#include "esl_wifi.h"

#include <stdio.h>
#include <string.h>

bool esl_wifi_parse_render_header(const uint8_t hdr[8], EslWifiRenderHeader *out) {
    if (hdr == NULL || out == NULL) return false;
    uint16_t w = (uint16_t)hdr[0] | ((uint16_t)hdr[1] << 8);
    uint16_t h = (uint16_t)hdr[2] | ((uint16_t)hdr[3] << 8);
    uint8_t planes = hdr[4];
    uint16_t stride = (uint16_t)hdr[6] | ((uint16_t)hdr[7] << 8);
    if (planes != 1u && planes != 2u) return false;
    if (w == 0u || h == 0u || stride == 0u) return false;
    out->width = w;
    out->height = h;
    out->planes = planes;
    out->row_stride = stride;
    return true;
}

uint32_t esl_wifi_plane_bytes(const EslWifiRenderHeader *hdr) {
    if (hdr == NULL) return 0;
    return (uint32_t)hdr->row_stride * (uint32_t)hdr->height * (uint32_t)hdr->planes;
}

static int hexv(uint8_t b) { return b < 10 ? '0' + b : 'a' + (b - 10); }

void esl_wifi_url_append_escaped(char *dst, size_t cap, size_t *pos,
                                 const char *s) {
    if (dst == NULL || pos == NULL || s == NULL || cap == 0) return;
    while (*s && *pos + 4 < cap) {
        unsigned char c = (unsigned char)*s++;
        bool safe = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
                    (c >= 'a' && c <= 'z') || c == '-' || c == '_' || c == '.';
        if (safe) {
            dst[(*pos)++] = (char)c;
        } else {
            dst[(*pos)++] = '%';
            dst[(*pos)++] = (char)hexv((uint8_t)(c >> 4));
            dst[(*pos)++] = (char)hexv((uint8_t)(c & 0xFu));
        }
    }
    dst[*pos] = 0;
}

bool esl_wifi_build_plugins_url(char *dst, size_t cap, const char *base) {
    if (dst == NULL || cap == 0 || base == NULL || base[0] == '\0') return false;
    int n = snprintf(dst, cap, "%s/plugins", base);
    return n > 0 && (size_t)n < cap;
}

bool esl_wifi_build_render_url(char *dst, size_t cap, const char *base,
                               const char *plugin_id, uint16_t w, uint16_t h,
                               uint8_t accent, const char *const *keys,
                               const char *const *values, uint8_t n_params) {
    if (dst == NULL || cap == 0 || base == NULL || plugin_id == NULL) return false;

    int wrote = snprintf(dst, cap, "%s/render/", base);
    if (wrote < 0 || (size_t)wrote >= cap) return false;
    size_t pos = (size_t)wrote;
    esl_wifi_url_append_escaped(dst, cap, &pos, plugin_id);

    int wrote2 = snprintf(
        dst + pos, cap - pos, "?w=%u&h=%u&accent=%s", (unsigned)w, (unsigned)h,
        accent == 1 ? "red" : accent == 2 ? "yellow" : "none");
    if (wrote2 < 0 || pos + (size_t)wrote2 >= cap) return false;
    pos += (size_t)wrote2;

    if (keys != NULL && values != NULL) {
        for (uint8_t i = 0; i < n_params; i++) {
            if (keys[i] == NULL || values[i] == NULL) continue;
            if (pos + 4 >= cap) break;
            dst[pos++] = '&';
            esl_wifi_url_append_escaped(dst, cap, &pos, keys[i]);
            if (pos + 1 >= cap) break;
            dst[pos++] = '=';
            esl_wifi_url_append_escaped(dst, cap, &pos, values[i]);
        }
    }
    dst[pos] = 0;
    return true;
}

/* ---- Tiny JSON subset (worker /plugins only) ------------------------- */

typedef struct {
    const char *p;
    bool ok;
} Jp;

static void jp_skip_ws(Jp *j) {
    while (j->p[0] == ' ' || j->p[0] == '\t' || j->p[0] == '\n' ||
           j->p[0] == '\r') {
        j->p++;
    }
}

static bool jp_eat(Jp *j, char c) {
    jp_skip_ws(j);
    if (j->p[0] != c) {
        j->ok = false;
        return false;
    }
    j->p++;
    return true;
}

static bool jp_peek(Jp *j, char c) {
    jp_skip_ws(j);
    return j->p[0] == c;
}

static int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static bool jp_parse_string(Jp *j, char *out, size_t cap) {
    jp_skip_ws(j);
    if (j->p[0] != '"') {
        j->ok = false;
        return false;
    }
    j->p++;
    size_t n = 0;
    while (j->p[0] != '\0' && j->p[0] != '"') {
        unsigned char c = (unsigned char)j->p[0];
        j->p++;
        if (c == '\\') {
            char e = j->p[0];
            if (e == '\0') {
                j->ok = false;
                return false;
            }
            j->p++;
            switch (e) {
            case '"':
            case '\\':
            case '/':
                c = (unsigned char)e;
                break;
            case 'b':
                c = 8;
                break;
            case 'f':
                c = 12;
                break;
            case 'n':
                c = '\n';
                break;
            case 'r':
                c = '\r';
                break;
            case 't':
                c = '\t';
                break;
            case 'u': {
                int h0 = hex_digit(j->p[0]);
                int h1 = hex_digit(j->p[1]);
                int h2 = hex_digit(j->p[2]);
                int h3 = hex_digit(j->p[3]);
                if (h0 < 0 || h1 < 0 || h2 < 0 || h3 < 0) {
                    j->ok = false;
                    return false;
                }
                j->p += 4;
                unsigned cp = ((unsigned)h0 << 12) | ((unsigned)h1 << 8) |
                              ((unsigned)h2 << 4) | (unsigned)h3;
                if (cp < 0x80u) {
                    c = (unsigned char)cp;
                } else if (cp < 0x800u) {
                    if (out != NULL && n + 1 < cap) out[n++] = (char)(0xC0u | (cp >> 6));
                    c = (unsigned char)(0x80u | (cp & 0x3Fu));
                } else {
                    if (out != NULL && n + 2 < cap) {
                        out[n++] = (char)(0xE0u | (cp >> 12));
                        out[n++] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
                    }
                    c = (unsigned char)(0x80u | (cp & 0x3Fu));
                }
                break;
            }
            default:
                c = (unsigned char)e;
                break;
            }
        }
        if (out != NULL && n + 1 < cap) out[n++] = (char)c;
    }
    if (j->p[0] != '"') {
        j->ok = false;
        return false;
    }
    j->p++;
    if (out != NULL && cap > 0) out[n] = '\0';
    return true;
}

static bool jp_parse_int(Jp *j, int32_t *out) {
    jp_skip_ws(j);
    const char *s = j->p;
    int sign = 1;
    if (s[0] == '-') {
        sign = -1;
        s++;
    }
    if (s[0] < '0' || s[0] > '9') {
        j->ok = false;
        return false;
    }
    int32_t v = 0;
    while (s[0] >= '0' && s[0] <= '9') {
        v = v * 10 + (s[0] - '0');
        s++;
    }
    j->p = s;
    if (out) *out = sign < 0 ? -v : v;
    return true;
}

static bool jp_skip_number(Jp *j) {
    jp_skip_ws(j);
    if (j->p[0] == '-') j->p++;
    if (j->p[0] < '0' || j->p[0] > '9') {
        j->ok = false;
        return false;
    }
    while (j->p[0] >= '0' && j->p[0] <= '9') j->p++;
    if (j->p[0] == '.') {
        j->p++;
        while (j->p[0] >= '0' && j->p[0] <= '9') j->p++;
    }
    if (j->p[0] == 'e' || j->p[0] == 'E') {
        j->p++;
        if (j->p[0] == '+' || j->p[0] == '-') j->p++;
        while (j->p[0] >= '0' && j->p[0] <= '9') j->p++;
    }
    return true;
}

static bool jp_skip_literal(Jp *j, const char *lit) {
    jp_skip_ws(j);
    size_t n = strlen(lit);
    if (strncmp(j->p, lit, n) != 0) {
        j->ok = false;
        return false;
    }
    j->p += n;
    return true;
}

static bool jp_skip_value(Jp *j);

static bool jp_skip_object(Jp *j) {
    if (!jp_eat(j, '{')) return false;
    jp_skip_ws(j);
    if (jp_peek(j, '}')) {
        j->p++;
        return true;
    }
    while (j->ok) {
        if (!jp_parse_string(j, NULL, 0)) return false;
        if (!jp_eat(j, ':')) return false;
        if (!jp_skip_value(j)) return false;
        jp_skip_ws(j);
        if (jp_peek(j, ',')) {
            j->p++;
            continue;
        }
        return jp_eat(j, '}');
    }
    return false;
}

static bool jp_skip_array(Jp *j) {
    if (!jp_eat(j, '[')) return false;
    jp_skip_ws(j);
    if (jp_peek(j, ']')) {
        j->p++;
        return true;
    }
    while (j->ok) {
        if (!jp_skip_value(j)) return false;
        jp_skip_ws(j);
        if (jp_peek(j, ',')) {
            j->p++;
            continue;
        }
        return jp_eat(j, ']');
    }
    return false;
}

static bool jp_skip_value(Jp *j) {
    jp_skip_ws(j);
    char c = j->p[0];
    if (c == '"') return jp_parse_string(j, NULL, 0);
    if (c == '{') return jp_skip_object(j);
    if (c == '[') return jp_skip_array(j);
    if (c == '-' || (c >= '0' && c <= '9')) return jp_skip_number(j);
    if (c == 't') return jp_skip_literal(j, "true");
    if (c == 'f') return jp_skip_literal(j, "false");
    if (c == 'n') return jp_skip_literal(j, "null");
    j->ok = false;
    return false;
}

static uint8_t param_type_from(const char *t) {
    if (t == NULL) return ESL_WIFI_PARAM_STRING;
    if (strcmp(t, "string") == 0) return ESL_WIFI_PARAM_STRING;
    if (strcmp(t, "int") == 0) return ESL_WIFI_PARAM_INT;
    if (strcmp(t, "enum") == 0) return ESL_WIFI_PARAM_ENUM;
    if (strcmp(t, "bool") == 0) return ESL_WIFI_PARAM_BOOL;
    return ESL_WIFI_PARAM_STRING;
}

static bool parse_options_array(Jp *j, EslWifiParam *sp) {
    if (!jp_eat(j, '[')) return false;
    jp_skip_ws(j);
    if (jp_peek(j, ']')) {
        j->p++;
        return true;
    }
    while (j->ok) {
        char opt[24];
        opt[0] = '\0';
        if (jp_peek(j, '"')) {
            if (!jp_parse_string(j, opt, sizeof opt)) return false;
            if (sp->option_count < ESL_WIFI_MAX_OPTIONS) {
                memcpy(sp->options[sp->option_count], opt, sizeof opt);
                sp->option_count++;
            }
        } else {
            if (!jp_skip_value(j)) return false;
        }
        jp_skip_ws(j);
        if (jp_peek(j, ',')) {
            j->p++;
            continue;
        }
        return jp_eat(j, ']');
    }
    return false;
}

static bool parse_param_object(Jp *j, EslWifiParam *sp) {
    memset(sp, 0, sizeof *sp);
    sp->int_min = 0;
    sp->int_max = 100;
    if (!jp_eat(j, '{')) return false;
    jp_skip_ws(j);
    if (jp_peek(j, '}')) {
        j->p++;
        return true;
    }
    while (j->ok) {
        char key[24];
        key[0] = '\0';
        if (!jp_parse_string(j, key, sizeof key)) return false;
        if (!jp_eat(j, ':')) return false;
        jp_skip_ws(j);
        if (strcmp(key, "key") == 0) {
            if (!jp_parse_string(j, sp->key, sizeof sp->key)) return false;
        } else if (strcmp(key, "label") == 0) {
            if (!jp_parse_string(j, sp->label, sizeof sp->label)) return false;
        } else if (strcmp(key, "type") == 0) {
            char t[16];
            t[0] = '\0';
            if (!jp_parse_string(j, t, sizeof t)) return false;
            sp->type = param_type_from(t);
        } else if (strcmp(key, "default") == 0) {
            if (jp_peek(j, '"')) {
                if (!jp_parse_string(j, sp->default_value, sizeof sp->default_value))
                    return false;
            } else {
                if (!jp_skip_value(j)) return false;
            }
        } else if (strcmp(key, "options") == 0) {
            if (jp_peek(j, '[')) {
                if (!parse_options_array(j, sp)) return false;
            } else {
                if (!jp_skip_value(j)) return false;
            }
        } else if (strcmp(key, "min") == 0) {
            if (!jp_parse_int(j, &sp->int_min)) return false;
        } else if (strcmp(key, "max") == 0) {
            if (!jp_parse_int(j, &sp->int_max)) return false;
        } else {
            if (!jp_skip_value(j)) return false;
        }
        jp_skip_ws(j);
        if (jp_peek(j, ',')) {
            j->p++;
            continue;
        }
        return jp_eat(j, '}');
    }
    return false;
}

static bool parse_params_array(Jp *j, EslWifiPlugin *pl) {
    if (!jp_eat(j, '[')) return false;
    jp_skip_ws(j);
    if (jp_peek(j, ']')) {
        j->p++;
        return true;
    }
    while (j->ok) {
        jp_skip_ws(j);
        if (jp_peek(j, '{')) {
            EslWifiParam tmp;
            if (!parse_param_object(j, &tmp)) return false;
            if (pl->param_count < ESL_WIFI_MAX_PARAMS) {
                pl->params[pl->param_count] = tmp;
                pl->param_count++;
            }
        } else {
            if (!jp_skip_value(j)) return false;
        }
        jp_skip_ws(j);
        if (jp_peek(j, ',')) {
            j->p++;
            continue;
        }
        return jp_eat(j, ']');
    }
    return false;
}

static bool parse_plugin_object(Jp *j, EslWifiPlugin *pl) {
    memset(pl, 0, sizeof *pl);
    pl->accent_modes = 1;
    if (!jp_eat(j, '{')) return false;
    jp_skip_ws(j);
    if (jp_peek(j, '}')) {
        j->p++;
        return true;
    }
    while (j->ok) {
        char key[24];
        key[0] = '\0';
        if (!jp_parse_string(j, key, sizeof key)) return false;
        if (!jp_eat(j, ':')) return false;
        jp_skip_ws(j);
        if (strcmp(key, "id") == 0) {
            if (!jp_parse_string(j, pl->id, sizeof pl->id)) return false;
        } else if (strcmp(key, "name") == 0) {
            if (!jp_parse_string(j, pl->name, sizeof pl->name)) return false;
        } else if (strcmp(key, "description") == 0) {
            if (!jp_parse_string(j, pl->description, sizeof pl->description))
                return false;
        } else if (strcmp(key, "accent_modes") == 0) {
            int32_t v = 1;
            if (!jp_parse_int(j, &v)) return false;
            pl->accent_modes = (uint8_t)v;
        } else if (strcmp(key, "params") == 0) {
            if (jp_peek(j, '[')) {
                if (!parse_params_array(j, pl)) return false;
            } else {
                if (!jp_skip_value(j)) return false;
            }
        } else {
            if (!jp_skip_value(j)) return false;
        }
        jp_skip_ws(j);
        if (jp_peek(j, ',')) {
            j->p++;
            continue;
        }
        return jp_eat(j, '}');
    }
    return false;
}

static bool parse_plugins_array(Jp *j, EslWifiManifest *out) {
    if (!jp_eat(j, '[')) return false;
    jp_skip_ws(j);
    if (jp_peek(j, ']')) {
        j->p++;
        return true;
    }
    while (j->ok) {
        jp_skip_ws(j);
        if (jp_peek(j, '{')) {
            EslWifiPlugin tmp;
            if (!parse_plugin_object(j, &tmp)) return false;
            if (out->count < ESL_WIFI_MAX_PLUGINS) {
                out->plugins[out->count] = tmp;
                out->count++;
            }
        } else {
            if (!jp_skip_value(j)) return false;
        }
        jp_skip_ws(j);
        if (jp_peek(j, ',')) {
            j->p++;
            continue;
        }
        return jp_eat(j, ']');
    }
    return false;
}

bool esl_wifi_parse_manifest(const char *json, EslWifiManifest *out) {
    if (json == NULL || out == NULL) return false;
    memset(out, 0, sizeof *out);
    Jp j = {.p = json, .ok = true};
    jp_skip_ws(&j);
    if (j.p[0] == '\0') return false;
    if (!jp_eat(&j, '{')) return false;
    jp_skip_ws(&j);
    if (jp_peek(&j, '}')) return true;

    while (j.ok) {
        char key[24];
        key[0] = '\0';
        if (!jp_parse_string(&j, key, sizeof key)) return false;
        if (!jp_eat(&j, ':')) return false;
        jp_skip_ws(&j);
        if (strcmp(key, "plugins") == 0) {
            if (jp_peek(&j, '[')) {
                if (!parse_plugins_array(&j, out)) return false;
            } else {
                if (!jp_skip_value(&j)) return false;
            }
        } else {
            if (!jp_skip_value(&j)) return false;
        }
        jp_skip_ws(&j);
        if (jp_peek(&j, ',')) {
            j.p++;
            continue;
        }
        return jp_eat(&j, '}');
    }
    return false;
}
