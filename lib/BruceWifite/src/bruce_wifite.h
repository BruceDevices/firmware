/* WIFITE API v1: all callbacks/driver work remain in resident firmware.
 * Caller polls events; cancel is nonblocking. Free ONLY after finished. */
#pragma once
#include "core.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
  WifiteScan,
  WifitePixie,
  WifiteVerifyPin,
  WifiteAuto,
  WifiteBruteforce,
  WifitePixieFull
} WifiteMode;
typedef enum { WifiteLog, WifiteTarget, WifiteResult } WifiteEventType;
typedef struct {
  WifiteEventType type;
  char text[128];
  WfTarget target;
} WifiteEvent;
typedef struct WifiteJob WifiteJob;
WifiteJob *wifite_start(WifiteMode mode, const WfTarget *target,
                        const char *pin);
bool wifite_poll(WifiteJob *job, WifiteEvent *event);
void wifite_cancel(WifiteJob *job);
bool wifite_finished(WifiteJob *job);
void wifite_free(WifiteJob *job);

#ifdef __cplusplus
}
#endif
