/* WIFITE native WPS research backend. MIT, 2026. */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define WF_MSG_MAX 1400
#define WF_TARGET_MAX 64
typedef struct {
  uint8_t bssid[6], channel, ssid_len;
  int8_t rssi;
  uint16_t capability, methods, clients;
  int8_t locked; /* -1 = attribute absent, never infer unlocked */
  uint8_t selected, version, wps; /* WPS IE observed; zero != proven disabled */
  char ssid[33], manufacturer[33], model[33];
} WfTarget;
typedef struct {
  uint8_t pke[192], pkr[192], auth[32], wrap[16];
  uint8_t enonce[16], rnonce[16], mac[6], uuid[16];
  uint8_t ehash1[32], ehash2[32], rs1[16], rs2[16];
  uint8_t last[WF_MSG_MAX];
  size_t last_len;
  uint16_t auth_flags, encr_flags;
  char pin[9], key[65];
  unsigned key_len;
  int stage;
} WfSession;
typedef bool (*WfYield)(void *ctx, unsigned progress, unsigned total);
typedef bool (*WfProgress)(void *ctx, const char *phase, uint64_t done,
                           uint64_t total);
/* Ported Pixiewps PRNG families 1-5; full=true expands RTL and enables Knuth.
 * Full scans can take many hours on ESP32. Cancellation is cooperative. */
int wf_pixie_all(WfSession *s, bool full, uint32_t epoch, WfProgress cb,
                 void *ctx);
int wf_crack_first(WfSession *s, const uint8_t nonce[16], WfYield cb,
                   void *ctx);
int wf_crack_second(WfSession *s, const uint8_t nonce[16], WfYield cb,
                    void *ctx);
uint16_t wf_be16(const uint8_t *p);
bool wf_tlv_valid(const uint8_t *p, size_t n);
const uint8_t *wf_attr(const uint8_t *p, size_t n, uint16_t tag, size_t *len);
bool wf_beacon(const uint8_t *p, size_t n, WfTarget *out);
bool wf_scan_beacon(const uint8_t *p, size_t n, WfTarget *out);
/* Passive confirmation only. An absent flag never clears an observed lock. */
typedef enum { WfLockPending, WfLockUnknown, WfLockClear, WfLockSet } WfLockState;
typedef struct {
  uint32_t first_at, last_at;
  uint16_t sequence;
  unsigned samples;
  int8_t value;
  bool have, saw_lock;
} WfLockWatch;
WfLockState wf_lock_observe(WfLockWatch *watch, int8_t value,
                            uint16_t sequence, uint32_t captured_at);
/* Returns EAP start, identity, WSC, failure or unrelated. FCS already removed.
 */
typedef enum {
  WfUnrelated,
  WfIdentity,
  WfWsc,
  WfEapFailure,
  WfFragment
} WfEapKind;
typedef struct {
  WfEapKind kind;
  uint8_t id, opcode, flags;
  uint16_t total;
  const uint8_t *data;
  size_t len;
} WfEap;
WfEap wf_eap(const uint8_t *p, size_t n, const uint8_t *ap, const uint8_t *sta);
/* Driver-delivered EAPOL (starts at the 802.1X header, no Ethernet header). */
WfEap wf_eapol(const uint8_t *p, size_t n);
/* kind: 0 Start, 1 registrar Identity Response, 2 expanded WSC Response. */
size_t wf_eapol_response(uint8_t *out, size_t cap, int kind, uint8_t id,
                         uint8_t op, const uint8_t *message, size_t n);
typedef struct {
  uint8_t data[WF_MSG_MAX];
  size_t used, total, last_offset, last_len;
  uint8_t id, flags;
  uint16_t declared;
  bool active, have;
} WfFragments;
/* -1 invalid, 0 send fragment ACK, 1 complete, 2 repeat last response.
 * Complete messages may refer to the reassembly buffer until the next call. */
int wf_reassemble(WfFragments *f, WfEap *e);
/* All crypto routines return false on error and validate input authenticator.
 */
bool wf_m2(WfSession *s, const uint8_t *m1, size_t n,
           const uint8_t random_private[32], const uint8_t random_nonce[16],
           uint8_t *out, size_t *out_n);
bool wf_m3(WfSession *s, const uint8_t *m, size_t n);
/* Two bounded hypotheses: all-zero secret nonces, then E-Nonce reused as both.
 */
int wf_pixie(WfSession *s, WfYield yield, void *ctx);
bool wf_m4(WfSession *s, const uint8_t random[48], uint8_t *out, size_t *n);
bool wf_m6(WfSession *s, const uint8_t *m5, size_t in_n, const uint8_t iv[16],
           uint8_t *out, size_t *out_n);
bool wf_m7(WfSession *s, const uint8_t *m7, size_t n);
unsigned wf_checksum(unsigned pin7);
typedef struct {
  unsigned first, second, phase;
} WfBrute;
bool wf_brute_pin(const WfBrute *b, char pin[9]);
bool wf_brute_next(WfBrute *b, unsigned evidence);
void wf_wipe(void *p, size_t n);
