#include "rtl_scan.h"

unsigned rtl_scan_count(unsigned mode) { return mode == RTL_PROFILE_ALL ? 6 : 2; }
unsigned rtl_scan_profile(unsigned mode, unsigned step) {
  static const unsigned profiles[] = {1, 5, 0, 4, 3, 2};
  return profiles[step % rtl_scan_count(mode)];
}
void rtl_scan_enter(RtlScan *s, uint32_t now) {
  s->entered_us = now;
  /* Vary the revisit phase instead of staying phase-locked to a periodic
   * transmitter. This is deterministic timing variation, not randomness. */
  s->probe_us = s->mode == RTL_PROFILE_ALL ?
      RTL_SCAN_PROBE_US + ((s->visit % 61u) * 37u % 61u) * 1000u : RTL_AUTO_DWELL_US;
}
void rtl_scan_init(RtlScan *s, uint32_t now, unsigned mode) {
  *s = (RtlScan){.mode = mode};
  rtl_scan_enter(s, now);
}
bool rtl_scan_advance(RtlScan *s) {
  s->visit++;
  s->step = (s->step + 1) % rtl_scan_count(s->mode);
  return s->step == 0;
}
bool rtl_scan_holding(const RtlScan *s, uint32_t now, bool burst) {
  uint32_t age = now - s->entered_us;
  return burst && age >= s->probe_us && age < RTL_SCAN_MAX_US;
}
bool rtl_scan_due(const RtlScan *s, uint32_t now, bool burst, bool drained) {
  uint32_t age = now - s->entered_us;
  if (age >= RTL_SCAN_MAX_US) return true;
  return age >= s->probe_us && !burst && drained;
}
