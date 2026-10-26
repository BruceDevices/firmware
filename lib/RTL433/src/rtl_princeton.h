/* Princeton-compatible OOK waveform validation. GPL-3.0-or-later. */
#pragma once
#include "rtl433/pulse_data.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
  uint32_t key, te_us;
  unsigned repeats;
} RtlPrinceton;
#define RTL_PRINCETON_MAX (PD_MAX_PULSES / 25)
size_t rtl_princeton_collect(const pulse_data_t *p, RtlPrinceton *out, size_t cap);
#ifdef __cplusplus
}
#endif
