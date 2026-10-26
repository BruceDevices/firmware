/* Adapter and JSON output for rtl_433. GPL-3.0-or-later. */
#include "rtl_engine.h"
#include "rtl433/logger.h"
#include "rtl433/pulse_slicer.h"
#include "rtl433/rtl_433_devices.h"
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DECL(name) &name,
static const r_device *const catalog[] = {DEVICES};
#undef DECL
size_t rtl_catalog_count(void) { return sizeof(catalog) / sizeof(catalog[0]); }
const r_device *rtl_catalog_get(size_t i) {
  return i < rtl_catalog_count() ? catalog[i] : NULL;
}
const char *rtl_modulation_name(unsigned m) {
  switch (m) {
  case OOK_PULSE_PCM: return "OOK PCM/RZ";
  case OOK_PULSE_RZI: return "OOK RZI";
  case OOK_PULSE_PPM: return "OOK PPM";
  case OOK_PULSE_PWM: return "OOK PWM";
  case OOK_PULSE_MANCHESTER_ZEROBIT: return "OOK Manchester";
  case OOK_PULSE_DMC: return "OOK DMC";
  case OOK_PULSE_PIWM_RAW: return "OOK PIWM raw";
  case OOK_PULSE_PIWM_DC: return "OOK PIWM DC";
  case OOK_PULSE_PWM_OSV1: return "OOK Oregon v1";
  case OOK_PULSE_NRZS: return "OOK NRZS";
  case FSK_PULSE_PCM: return "FSK PCM";
  case FSK_PULSE_PWM: return "FSK PWM";
  case FSK_PULSE_MANCHESTER_ZEROBIT: return "FSK Manchester";
  default: return NULL;
  }
}
bool rtl_decoder_runnable(const r_device *d) {
  return d && d->disabled != 3 && d->decode_fn && rtl_modulation_name(d->modulation);
}
bool rtl_encoding_family(const r_device *d, const data_t *data) {
  const data_t *model = rtl_field(data, "model");
  return d && d->decode_fn == generic_remote.decode_fn && model &&
         model->type == DATA_STRING && !strcmp(model->value.v_ptr, "Princeton");
}
const char *rtl_evidence(const r_device *d, const data_t *data) {
  if (rtl_encoding_family(d, data)) return "FRAMING";
  const data_t *mic = rtl_field(data, "mic");
  if (mic && mic->type == DATA_STRING) {
    if (!strcmp(mic->value.v_ptr, "CRC")) return "CRC";
    if (!strcmp(mic->value.v_ptr, "CHECKSUM")) return "CHECKSUM";
    if (!strcmp(mic->value.v_ptr, "PARITY")) return "PARITY";
  }
  return "CANDIDATE";
}
static void output_data(r_device *d, data_t *data) {
  RtlEngine *e = d->output_ctx;
  if (data) {
    // Akhan uses the same 24-bit EV1527 encoding. Do not create a second,
    // model-specific record for the identical independently validated key.
    if (d->decode_fn == akhan_100F14.decode_fn && e->princeton_count) {
      const data_t *id = rtl_field(data, "id"), *cmd = rtl_field(data, "data");
      if (id && id->type == DATA_INT && cmd && cmd->type == DATA_STRING) {
        unsigned nibble = 0;
        if (sscanf(cmd->value.v_ptr, "0x%x", &nibble) == 1 && nibble < 16) {
          uint32_t key = ((uint32_t)id->value.v_int << 4) | nibble;
          for (size_t i = 0; i < e->princeton_count; i++)
            if (e->princeton[i].key == key) { data_free(data); return; }
        }
      }
    }
    e->messages++;
    e->output(e->ctx, d, data, e->meta);
    data_free(data);
  }
}
static void output_log(r_device *d, int level, data_t *data) {
  RtlEngine *e = d->output_ctx;
  (void)level;
  e->warnings++;
  data_free(data);
}
bool rtl_engine_init(RtlEngine *e, RtlOutput output, RtlYield yield,
                     void *ctx) {
  memset(e, 0, sizeof(*e));
  e->count = rtl_catalog_count();
  e->output = output;
  e->yield = yield;
  e->ctx = ctx;
  e->devices = calloc(e->count, sizeof(*e->devices));
  e->enabled = calloc(e->count, sizeof(*e->enabled));
  if (!e->devices || !e->enabled) {
    rtl_engine_free(e);
    return false;
  }
  for (size_t i = 0; i < e->count; i++) {
    const r_device *t = catalog[i];
    r_device *d = t->create_fn ? t->create_fn(NULL) : malloc(sizeof(*d));
    if (!d) {
      rtl_engine_free(e);
      return false;
    }
    if (!t->create_fn)
      *d = *t;
    e->devices[i] = d;
    d->protocol_num = i + 1;
    d->verbose = 0;
    d->verbose_bits = 0;
    d->output_fn = output_data;
    d->log_fn = output_log;
    d->output_ctx = e;
    e->enabled[i] = t->disabled == 0 && rtl_decoder_runnable(d);
  }
  return true;
}
void rtl_engine_free(RtlEngine *e) {
  if (e->devices)
    for (size_t i = 0; i < e->count; i++)
      if (e->devices[i]) {
        free(e->devices[i]->decode_ctx);
        free(e->devices[i]);
      }
  free(e->devices);
  free(e->enabled);
  memset(e, 0, sizeof(*e));
}
static int slice(const pulse_data_t *p, r_device *d) {
  if (d->decode_fn == generic_remote.decode_fn) {
    RtlEngine *e = d->output_ctx;
    for (size_t i = 0; i < e->princeton_count; i++) {
      const RtlPrinceton *v = &e->princeton[i];
      char code[7], tristate[13];
      snprintf(code, sizeof code, "%06lX", (unsigned long)(v->key & 0xffffff));
      for (unsigned b = 0; b < 12; b++) tristate[b] = "0ZX1"[(v->key >> (22 - b*2)) & 3];
      tristate[12] = 0;
      data_t *data = data_make(
          "model", "", DATA_STRING, "Princeton",
          "id", "Address (20 bit)", DATA_INT, v->key >> 4,
          "button", "Data (4 bit)", DATA_INT, v->key & 15,
          "code", "Raw 24-bit code", DATA_STRING, code,
          "bits", "", DATA_INT, 24,
          "te_us", "Short pulse", DATA_INT, v->te_us,
          "repeats", "Matching complete frames", DATA_INT, v->repeats,
          "tristate", "", DATA_STRING, tristate,
          "family", "", DATA_STRING, "Princeton / EV1527 / PT226x compatible",
          "identification", "", DATA_STRING, "Encoding family; device model unverified",
          NULL);
      output_data(d, data);
    }
    return (int)e->princeton_count;
  }
  switch (d->modulation) {
  case OOK_PULSE_PCM:
  case FSK_PULSE_PCM:
    return pulse_slicer_pcm(p, d);
  case OOK_PULSE_PPM:
    return pulse_slicer_ppm(p, d);
  case OOK_PULSE_RZI:
    return pulse_slicer_rzi(p, d);
  case OOK_PULSE_PWM:
  case FSK_PULSE_PWM:
    return pulse_slicer_pwm(p, d);
  case OOK_PULSE_MANCHESTER_ZEROBIT:
  case FSK_PULSE_MANCHESTER_ZEROBIT:
    return pulse_slicer_manchester_zerobit(p, d);
  case OOK_PULSE_DMC:
    return pulse_slicer_dmc(p, d);
  case OOK_PULSE_PIWM_RAW:
    return pulse_slicer_piwm_raw(p, d);
  case OOK_PULSE_PIWM_DC:
    return pulse_slicer_piwm_dc(p, d);
  case OOK_PULSE_PWM_OSV1:
    return pulse_slicer_osv1(p, d);
  case OOK_PULSE_NRZS:
    return pulse_slicer_nrzs(p, d);
  default:
    return 0;
  }
}
unsigned rtl_engine_decode(RtlEngine *e, const pulse_data_t *p,
                           const RtlMeta *meta) {
  if (!e || !meta || !p || !p->num_pulses || p->num_pulses > PD_MAX_PULSES || !p->sample_rate)
    return 0;
  e->meta = meta;
  e->princeton_count = 0;
  if (!meta->fsk)
    for (size_t i = 0; i < e->count; i++)
      if (e->enabled[i] && e->devices[i]->decode_fn == generic_remote.decode_fn) {
        e->princeton_count = rtl_princeton_collect(p, e->princeton, RTL_PRINCETON_MAX);
        break;
      }
  size_t hot[4];
  memcpy(hot, e->recent[meta->fsk], sizeof hot);
  unsigned before = e->messages, next = 0;
  for (unsigned priority = 0; e->messages == before && priority < UINT_MAX;
       priority = next) {
    next = UINT_MAX;
    /* Try recent matches first within their original priority. Still run all
     * remaining eligible decoders, so a cache hit cannot hide other records. */
    for (size_t slot = 0; slot < e->count + 4; slot++) {
      size_t i;
      if (slot < 4) {
        if (!hot[slot] || hot[slot] > e->count) continue;
        i = hot[slot] - 1;
      } else {
        i = slot - 4;
        bool tried = false;
        for (unsigned h = 0; h < 4; h++) tried |= hot[h] == i + 1;
        if (tried) continue;
      }
      r_device *d = e->devices[i];
      if (!e->enabled[i] || !d->decode_fn ||
          (d->modulation >= FSK_DEMOD_MIN_VAL) != meta->fsk)
        continue;
      if (d->priority > priority && d->priority < next)
        next = d->priority;
      if (d->priority == priority) {
        unsigned messages = e->messages;
        slice(p, d);
        if (e->messages > messages) {
          size_t *recent = e->recent[meta->fsk];
          unsigned at = 0;
          while (at < 3 && recent[at] != i + 1) at++;
          memmove(recent + 1, recent, at * sizeof(*recent));
          recent[0] = i + 1;
        }
        /* Cooperate only after real work, never for disabled entries. The
         * platform callback budgets blocking sleeps by elapsed time. */
        if (e->yield) e->yield(e->ctx);
      }
    }
  }
  e->meta = NULL;
  return e->messages - before;
}

/* Bounded dynamically sized serializer: no silently truncated JSON records. */
typedef struct {
  char *s;
  size_t n, cap;
  size_t limit;
  bool failed;
} Json;
static void put(Json *j, const char *s, size_t n) {
  if (j->failed)
    return;
  size_t limit = j->limit ? j->limit : 65536;
  if (n > limit || j->n > limit - n) {
    j->failed = true;
    return;
  }
  if (j->n + n + 1 > j->cap) {
    size_t cap = j->cap ? j->cap : 512;
    while (cap < j->n + n + 1)
      cap *= 2;
    char *p = realloc(j->s, cap);
    if (!p) {
      j->failed = true;
      return;
    }
    j->s = p;
    j->cap = cap;
  }
  memcpy(j->s + j->n, s, n);
  j->n += n;
  j->s[j->n] = 0;
}
static void literal(Json *j, const char *s) { put(j, s, strlen(s)); }
static void quoted(Json *j, const char *s) {
  literal(j, "\"");
  for (const unsigned char *p = (const unsigned char *)(s ? s : ""); *p; p++) {
    if (*p == '"' || *p == '\\') {
      literal(j, "\\");
      put(j, (const char *)p, 1);
    } else if (*p < 32) {
      char b[7];
      snprintf(b, sizeof b, "\\u%04x", *p);
      literal(j, b);
    } else
      put(j, (const char *)p, 1);
  }
  literal(j, "\"");
}
static void object(Json *, const data_t *, unsigned);
static void value(Json *j, data_type_t type, data_value_t v, unsigned depth) {
  if (depth > 16) {
    j->failed = true;
    return;
  }
  char b[64];
  switch (type) {
  case DATA_INT:
    snprintf(b, sizeof b, "%d", v.v_int);
    literal(j, b);
    break;
  case DATA_DOUBLE:
    if (isfinite(v.v_dbl)) {
      /* Shortest significant-digit representation that round-trips. Sensor
       * values stay readable without discarding precision in other decoders. */
      for (int precision = 1; precision <= 17; precision++) {
        snprintf(b, sizeof b, "%.*g", precision, v.v_dbl);
        if (strtod(b, NULL) == v.v_dbl)
          break;
      }
      literal(j, b);
    } else
      literal(j, "null");
    break;
  case DATA_STRING:
    quoted(j, v.v_ptr);
    break;
  case DATA_DATA:
    object(j, v.v_ptr, depth + 1);
    break;
  case DATA_ARRAY: {
    const data_array_t *a = v.v_ptr;
    literal(j, "[");
    if (a)
      for (int i = 0; i < a->num_values; i++) {
        if (i)
          literal(j, ",");
        data_value_t x = {0};
        if (a->type == DATA_INT)
          x.v_int = ((int *)a->values)[i];
        else if (a->type == DATA_DOUBLE)
          x.v_dbl = ((double *)a->values)[i];
        else
          x.v_ptr = ((void **)a->values)[i];
        value(j, a->type, x, depth + 1);
      }
    literal(j, "]");
    break;
  }
  default:
    literal(j, "null");
    break;
  }
}
static void object(Json *j, const data_t *d, unsigned depth) {
  literal(j, "{");
  bool first = true;
  for (; d; d = d->next) {
    if (!first)
      literal(j, ",");
    first = false;
    quoted(j, d->key);
    literal(j, ":");
    value(j, d->type, d->value, depth);
  }
  literal(j, "}");
}
char *rtl_json(const r_device *d, const data_t *data, const RtlMeta *m) {
  Json j = {0};
  /* Sensor readings first, so pressure/temperature fit on the first details
   * page. Metadata follows and remains available by scrolling. */
  literal(&j, "{\"data\":");
  object(&j, data, 0);
  char b[256];
  snprintf(
      b, sizeof b,
      ",\"time_ms\":%lu,\"frequency_hz\":%lu,\"rssi_dbm\":%d,\"modulation\":\"%"
      "s\",\"source\":\"%s\",\"decoder_id\":%u,\"protocol\":",
      (unsigned long)m->time_ms, (unsigned long)m->frequency, m->rssi,
      m->fsk ? "FSK" : "OOK", m->offline ? "file" : "radio", d->protocol_num);
  literal(&j, b);
  quoted(&j, d->name);
  literal(&j, ",\"identification_status\":");
  quoted(&j, rtl_encoding_family(d, data) ? "encoding_family" : "model_candidate");
  literal(&j, ",\"validation_reported\":");
  quoted(&j, rtl_evidence(d, data));
  literal(&j, ",\"optional_decoder\":");
  literal(&j, d->disabled ? "true" : "false");
  literal(&j, ",\"radio_profile\":");
  if (m->profile_known) {
    snprintf(b, sizeof b, "%u", m->profile);
    literal(&j, b);
  } else literal(&j, "null");
  literal(&j, "}");
  if (j.failed) {
    free(j.s);
    return NULL;
  }
  return j.s;
}
char *rtl_identity(const r_device *d, const data_t *data) {
  Json j = {0};
  char b[24];
  snprintf(b, sizeof b, "[%u", d->protocol_num);
  literal(&j, b);
  const char *keys[] = {"model", "id", "channel"};
  for (unsigned i = 0; i < 3; i++) {
    literal(&j, ",");
    const data_t *f = rtl_field(data, keys[i]);
    if (f) value(&j, f->type, f->value, 0);
    else literal(&j, "null");
  }
  literal(&j, "]");
  if (j.failed) { free(j.s); return NULL; }
  return j.s;
}
static void csv_quote(Json *j, const char *s) {
  literal(j, "\"");
  for (; *s; s++) {
    if (*s == '"') literal(j, "\"");
    put(j, s, 1);
  }
  literal(j, "\"");
}
char *rtl_csv_record(const char *model, const char *id, const char *channel,
                      unsigned hits, const RtlMeta *m, const char *json) {
  Json j = {.limit = 262144};
  csv_quote(&j, model); literal(&j, ",");
  csv_quote(&j, id); literal(&j, ",");
  csv_quote(&j, channel);
  char b[80];
  snprintf(b, sizeof b, ",%u,%lu,%d,", hits, (unsigned long)m->frequency, m->rssi);
  literal(&j, b); csv_quote(&j, json); literal(&j, "\n");
  if (j.failed) { free(j.s); return NULL; }
  return j.s;
}
/* Format our valid JSON without parsing/retyping its values or interpreting
 * punctuation inside strings. ASCII escapes make Unicode visible on the
 * compact ASCII font; JSON exports retain the original UTF-8. */
char *rtl_pretty_json(const char *s) {
  Json j = {.limit = 262144};
  bool string = false, escape = false;
  unsigned depth = 0;
  for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
    unsigned char c = *p;
    if (string) {
      if (c >= 128) {
        unsigned cp = 0, extra = 0;
        if (c >= 0xc2 && c <= 0xdf) { cp = c & 31; extra = 1; }
        else if (c >= 0xe0 && c <= 0xef) { cp = c & 15; extra = 2; }
        else if (c >= 0xf0 && c <= 0xf4) { cp = c & 7; extra = 3; }
        bool valid = extra != 0;
        for (unsigned i = 1; i <= extra && valid; i++) {
          if ((p[i] & 0xc0) != 0x80) valid = false;
          else cp = (cp << 6) | (p[i] & 63);
        }
        char b[20];
        if (valid && cp <= 0x10ffff) {
          if (cp <= 0xffff) snprintf(b, sizeof b, "\\u%04x", cp);
          else { cp -= 0x10000; snprintf(b, sizeof b, "\\u%04x\\u%04x", 0xd800 + (cp >> 10), 0xdc00 + (cp & 1023)); }
          p += extra;
        } else snprintf(b, sizeof b, "\\x%02x", c);
        literal(&j, b);
      } else put(&j, (const char *)&c, 1);
      if (escape) escape = false;
      else if (c == '\\') escape = true;
      else if (c == '"') string = false;
      continue;
    }
    if (c == '"') string = true;
    if (c == '}' || c == ']') {
      if (depth) depth--;
      literal(&j, "\n");
      for (unsigned i = 0; i < depth; i++) literal(&j, "  ");
    }
    put(&j, (const char *)&c, 1);
    if (c == ':') literal(&j, " ");
    if (c == '{' || c == '[') depth++;
    if (c == '{' || c == '[' || c == ',') {
      literal(&j, "\n");
      for (unsigned i = 0; i < depth; i++) literal(&j, "  ");
    }
  }
  if (j.failed) { free(j.s); return NULL; }
  return j.s;
}
const data_t *rtl_field(const data_t *d, const char *key) {
  for (; d; d = d->next)
    if (!strcmp(d->key, key))
      return d;
  return NULL;
}
void rtl_value_text(const data_t *d, char *dst, size_t n) {
  if (!d) {
    snprintf(dst, n, "--");
    return;
  }
  if (d->type == DATA_STRING)
    snprintf(dst, n, "%s", (char *)d->value.v_ptr);
  else if (d->type == DATA_INT)
    snprintf(dst, n, "%d", d->value.v_int);
  else if (d->type == DATA_DOUBLE)
    snprintf(dst, n, "%.6g", d->value.v_dbl);
  else if (d->type == DATA_ARRAY)
    snprintf(dst, n, "[%d values; see JSON]",
             ((data_array_t *)d->value.v_ptr)->num_values);
  else
    snprintf(dst, n, "{see JSON}");
}
/* Decoder-library diagnostics go to the device serial console. */
static r_logger_handler logger;
static void *logger_ctx;
void r_logger_set_log_handler(r_logger_handler h, void *ctx) {
  logger = h;
  logger_ctx = ctx;
}
void print_log(log_level_t level, const char *src, const char *msg) {
  if (logger)
    logger(level, src, msg, logger_ctx);
  else if (level <= LOG_WARNING)
    printf("RTL433 %s: %s\n", src, msg);
}
void print_logf(log_level_t level, const char *src, const char *fmt, ...) {
  if (level > LOG_WARNING && !logger)
    return;
  char b[192];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(b, sizeof b, fmt, ap);
  va_end(ap);
  print_log(level, src, b);
}
