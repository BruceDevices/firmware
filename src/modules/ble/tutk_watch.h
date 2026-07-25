#ifndef __TUTK_WATCH_H__
#define __TUTK_WATCH_H__

// Tier-2 TUTK detection: passive traffic classifier.
//
// Modern TUTK/Kalay cameras don't answer a clean LAN broadcast (see Tier-1),
// but they continuously phone home to TUTK master servers (UDP 32100) and
// resolve Kalay/ThroughTek DNS names. Those are unicast, so on WPA2 we can only
// see them by becoming the path: this module ARP-poisons the gateway identity
// (half-duplex), hooks the lwIP netif input to inspect each redirected frame,
// classifies TUTK talkers, and FORWARDS the frame on to the real gateway so the
// network keeps working (detection-only, non-destructive).
//
// EXPERIMENTAL: this is a MITM technique and has not yet been validated on
// hardware. Only run it on networks you are authorised to test.

void tutkWatch();

#endif
