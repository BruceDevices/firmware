#include "rtl_capture.h"
#include <limits.h>
#include <string.h>
void rtl_capture_reset(RtlCapture *c, RtlFrame *f) {
  memset(c, 0, sizeof(*c));
  memset(f, 0, sizeof(*f));
  f->pulses.sample_rate = 1000000;
}
bool rtl_capture_finish(RtlCapture *c, RtlFrame *f, uint32_t gap) {
  if (c->invalid)
    return false;
  f->tail_known = true;
  f->tail_gap_us = c->high || !f->pulses.num_pulses ? 0 :
      (uint32_t)f->pulses.gap[f->pulses.num_pulses - 1];
  if (c->high) {
    if (f->pulses.num_pulses >= PD_MAX_PULSES) {
      c->invalid = true;
      return false;
    }
    unsigned i = f->pulses.num_pulses++;
    f->pulses.pulse[i] = c->high;
    f->pulses.gap[i] = gap < RTL_GAP_US ? RTL_GAP_US : gap;
    c->high = 0;
  } else if (f->pulses.num_pulses) {
    unsigned i = f->pulses.num_pulses - 1;
    if (f->pulses.gap[i] < (int)RTL_GAP_US)
      f->pulses.gap[i] = RTL_GAP_US;
  }
  return f->pulses.num_pulses >= 8;
}
bool rtl_capture_feed(RtlCapture *c, RtlFrame *f, bool level, uint32_t us) {
  if (!us || us > INT_MAX) {
    c->invalid = true;
    return false;
  }
  if (c->invalid)
    return false;
  if (level) {
    if (us >= RTL_GAP_US || c->high > RTL_GAP_US - us) {
      c->invalid = true;
      return false;
    }
    c->high += us;
  } else if (us >= RTL_GAP_US) {
    return rtl_capture_finish(c, f, us);
  } else if (c->high) {
    if (f->pulses.num_pulses >= PD_MAX_PULSES) {
      c->invalid = true;
      return false;
    }
    unsigned i = f->pulses.num_pulses++;
    f->pulses.pulse[i] = c->high;
    f->pulses.gap[i] = us;
    c->high = 0;
    c->fresh_pairs++;
  } else if (f->pulses.num_pulses) {
    int *gap = &f->pulses.gap[f->pulses.num_pulses - 1];
    if (*gap > INT_MAX - (int)us)
      c->invalid = true;
    else
      *gap += us;
  }
  return false;
}
bool rtl_capture_window(RtlCapture *c, RtlFrame *f) {
  if (c->invalid || c->fresh_pairs < 8 || f->pulses.num_pulses < 8)
    return false;
  f->tail_known = true;
  f->tail_gap_us = f->pulses.gap[f->pulses.num_pulses - 1];
  c->fresh_pairs = 0;
  return true;
}
void rtl_capture_rollover(RtlCapture *c, RtlFrame *f) {
  if (c->invalid || f->pulses.num_pulses < PD_MAX_PULSES) return;
  unsigned keep = PD_MAX_PULSES / 2;
  unsigned remove = f->pulses.num_pulses - keep;
  uint64_t elapsed = 0;
  for (unsigned i = 0; i < remove; i++)
    elapsed += (uint32_t)f->pulses.pulse[i] + (uint32_t)f->pulses.gap[i];
  memmove(f->pulses.pulse, f->pulses.pulse + remove, keep * sizeof(int));
  memmove(f->pulses.gap, f->pulses.gap + remove, keep * sizeof(int));
  f->pulses.num_pulses = keep;
  f->meta.time_ms += elapsed / 1000;
  /* Pending high duration belongs to the next pair and must survive. */
  f->tail_known = false;
}
