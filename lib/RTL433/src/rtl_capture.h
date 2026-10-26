/* Pulse assembly shared by the on-device receiver and host tests.
 * GPL-3.0-or-later. */
#pragma once
#include "rtl_engine.h"
#ifdef __cplusplus
extern "C" {
#endif
#define RTL_GAP_US 100000u
typedef struct {
  pulse_data_t pulses;
  RtlMeta meta;
  /* Real final low interval, before the decoder's synthetic end-of-burst gap.
   * Zero means the last observed interval was high. */
  uint32_t tail_gap_us;
  bool tail_known;
  uint32_t queued_us; /* transient worker timing; not part of the pulse file */
  bool window; /* transient: unfinished snapshot, not a complete burst */
} RtlFrame;
typedef struct {
  uint32_t high;
  unsigned fresh_pairs;
  bool invalid;
} RtlCapture;
void rtl_capture_reset(RtlCapture *c, RtlFrame *f);
/* Returns true when a complete burst is ready. The caller then resets the
 * buffer. */
bool rtl_capture_feed(RtlCapture *c, RtlFrame *f, bool level, uint32_t us);
bool rtl_capture_finish(RtlCapture *c, RtlFrame *f, uint32_t trailing_gap);
/* Snapshot complete, measured pairs without fabricating a quiet interval.
 * A running noisy channel must not require 100 ms of silence to decode. */
bool rtl_capture_window(RtlCapture *c, RtlFrame *f);
/* Keep half a full window so repeat-dependent packets can cross its boundary.
 * Call after publishing the full window, before feeding another edge. */
void rtl_capture_rollover(RtlCapture *c, RtlFrame *f);
#ifdef __cplusplus
}
#endif
