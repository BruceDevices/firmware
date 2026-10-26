/* Validate both pulse halves, frame boundaries and repetitions. A compatible
 * waveform identifies the encoding family, not a manufacturer/device model.
 * GPL-3.0-or-later. */
#include "rtl_princeton.h"
#include <stdbool.h>

static bool near(uint64_t actual, uint64_t expected, unsigned percent) {
  uint64_t delta = actual > expected ? actual - expected : expected - actual;
  return delta * 100 <= expected * percent;
}
static bool frame(const pulse_data_t *p, unsigned first, unsigned end,
                  bool terminal, uint32_t *key, uint32_t *te_us) {
  if (end - first != 24) return false;
  uint64_t total = 0;
  for (unsigned n = first; n < end; n++) {
    if (p->pulse[n] <= 0 || p->gap[n] <= 0) return false;
    total += (uint32_t)p->pulse[n] + (uint64_t)p->gap[n];
  }
  uint64_t te = (total + 48) / 96;
  uint64_t us = te * 1000000 / p->sample_rate;
  if (us < 100 || us > 800 || !near(p->pulse[end], te, 30) ||
      p->gap[end] < (int64_t)(15 * te) ||
      (!terminal && p->gap[end] > (int64_t)(72 * te)))
    return false;
  uint32_t value = 0;
  for (unsigned n = first; n < end; n++) {
    bool zero = near(p->pulse[n], te, 30) && near(p->gap[n], te * 3, 30);
    bool one = near(p->pulse[n], te * 3, 30) && near(p->gap[n], te, 30);
    if (zero == one || !near((uint64_t)p->pulse[n] + p->gap[n], te * 4, 20))
      return false;
    value = (value << 1) | one;
  }
  *key = value; *te_us = (uint32_t)us;
  return true;
}
size_t rtl_princeton_collect(const pulse_data_t *p, RtlPrinceton *out, size_t cap) {
  if (!p || !out || !cap || !p->sample_rate || p->num_pulses > PD_MAX_PULSES)
    return 0;
  unsigned first = 0, run = 0;
  uint32_t previous = 0, previous_te = 0;
  size_t count = 0;
  for (unsigned n = 0; n < p->num_pulses; n++) {
    // A real sync ends a short high pulse with a long low gap. Capture
    // termination extends the final gap to at least 100 ms. That final
    // frame may confirm a prior complete frame, but cannot start a match.
    if (p->pulse[n] <= 0 || p->gap[n] <= (int64_t)p->pulse[n] * 8) continue;
    uint32_t key, te;
    bool terminal = n + 1 == p->num_pulses &&
        (uint64_t)p->gap[n] * 1000000 >= (uint64_t)p->sample_rate * 100000;
    if (frame(p, first, n, terminal, &key, &te)) {
      if (terminal && (!run || key != previous || !near(te, previous_te, 20))) break;
      run = run && key == previous && near(te, previous_te, 20) ? run + 1 : 1;
      previous = key; previous_te = te;
      if (run >= 2) {
        size_t at = 0;
        while (at < count && (out[at].key != key || !near(te, out[at].te_us, 20))) at++;
        if (at < count) { if (run > out[at].repeats) out[at].repeats = run; }
        else if (count < cap) out[count++] = (RtlPrinceton){key, te, run};
      }
    } else run = 0;
    first = n + 1;
  }
  return count;
}
