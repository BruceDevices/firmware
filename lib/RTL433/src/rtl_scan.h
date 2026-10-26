/* Fair, burst-aware scheduling for one CC1101. GPL-3.0-or-later. */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define RTL_SCAN_PROBE_US 250000u
#define RTL_AUTO_DWELL_US 2000000u
#define RTL_SCAN_MAX_US 3000000u
#define RTL_PROFILE_AUTO 6u
#define RTL_PROFILE_ALL 7u
typedef struct {
  uint32_t entered_us, probe_us, visit;
  unsigned step, mode;
} RtlScan;

unsigned rtl_scan_count(unsigned mode);
unsigned rtl_scan_profile(unsigned mode, unsigned step);
void rtl_scan_init(RtlScan *s, uint32_t now, unsigned mode);
/* Returns true at a complete mode cycle; caller may hop bands. */
bool rtl_scan_advance(RtlScan *s);
/* Call after tuning, so setup/calibration time is not counted as reception. */
void rtl_scan_enter(RtlScan *s, uint32_t now);
bool rtl_scan_holding(const RtlScan *s, uint32_t now, bool burst);
bool rtl_scan_due(const RtlScan *s, uint32_t now, bool burst, bool drained);
#ifdef __cplusplus
}
#endif
