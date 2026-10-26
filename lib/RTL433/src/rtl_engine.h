/* Native WirelessPi adapter. GPL-3.0-or-later. */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "rtl433/pulse_data.h"
#include "rtl433/r_device.h"
#include "rtl_princeton.h"

typedef struct {
  uint32_t frequency, time_ms;
  int rssi;
  bool fsk, offline;
  unsigned profile;
  bool profile_known;
} RtlMeta;
typedef void (*RtlOutput)(void *ctx, const r_device *decoder,
                          const data_t *data, const RtlMeta *meta);
typedef void (*RtlYield)(void *ctx);
typedef struct {
  r_device **devices;
  bool *enabled;
  size_t count;
  RtlOutput output;
  RtlYield yield;
  void *ctx;
  const RtlMeta *meta;
  unsigned messages, warnings;
  /* One-based catalog indices, most recent first, separated by modulation. */
  size_t recent[2][4];
  RtlPrinceton princeton[RTL_PRINCETON_MAX];
  size_t princeton_count;
} RtlEngine;
size_t rtl_catalog_count(void);
const r_device *rtl_catalog_get(size_t index);
const char *rtl_modulation_name(unsigned modulation);
bool rtl_decoder_runnable(const r_device *decoder);
/* Integrity evidence is not proof of a manufacturer's identity. */
bool rtl_encoding_family(const r_device *decoder, const data_t *data);
const char *rtl_evidence(const r_device *decoder, const data_t *data);
bool rtl_engine_init(RtlEngine *e, RtlOutput output, RtlYield yield, void *ctx);
void rtl_engine_free(RtlEngine *e);
unsigned rtl_engine_decode(RtlEngine *e, const pulse_data_t *pulses,
                           const RtlMeta *meta);
/* Allocated JSON retains every decoded field, with an outer capture metadata
 * object. */
char *rtl_json(const r_device *decoder, const data_t *data,
               const RtlMeta *meta);
char *rtl_identity(const r_device *decoder, const data_t *data);
char *rtl_pretty_json(const char *json);
char *rtl_csv_record(const char *model, const char *id, const char *channel,
                      unsigned hits, const RtlMeta *meta, const char *json);
const data_t *rtl_field(const data_t *data, const char *key);
void rtl_value_text(const data_t *field, char *dst, size_t n);
#ifdef __cplusplus
}
#endif
