#ifndef __ROLLING_CODE_DB_H__
#define __ROLLING_CODE_DB_H__

#include <stddef.h>
#include <stdint.h>
#include <FS.h>

// ---------------------------------------------------------------------------
// Cipher type constants — used by the manufacturer key table entries below.
// KeeLoq cipher variants (all use the KeeLoq NLFSR cipher core).
// ---------------------------------------------------------------------------
#define CIPHER_KEELOQ_SIMPLE          1
#define CIPHER_KEELOQ_NORMAL          2
#define CIPHER_KEELOQ_SECURE          3
#define CIPHER_KEELOQ_MAGIC_XOR_TYPE1 4
#define CIPHER_KEELOQ_MAGIC_SERIAL_T1 6
#define CIPHER_KEELOQ_MAGIC_SERIAL_T2 7
#define CIPHER_KEELOQ_MAGIC_SERIAL_T3 8
#define CIPHER_KEELOQ_JAROLIFT        10
#define CIPHER_KEELOQ_ERREKA          12
#define CIPHER_KEELOQ_PUJOL           13
#define CIPHER_KEELOQ_AERF            14
#define CIPHER_KEELOQ_FAAC_SLH        15 // FAAC SLH: KeeLoq cipher + its own learning function
// Non-KeeLoq ciphers
#define CIPHER_AES128_ECB             20 // Beninca ARC

// ---------------------------------------------------------------------------
// CC1101 bandwidth presets selected before TX.
// ---------------------------------------------------------------------------
#define BW_OOK_270 0
#define BW_OOK_650 1

// ---------------------------------------------------------------------------
// Manufacturer key table.
//
// Several rolling code protocols (KeeLoq variants, FAAC SLH, Jarolift, Beninca
// ARC) need a manufacturer-supplied secret key to encrypt/decrypt their hop
// word. These are compiled in here instead of being read from an SD `/mfcodes`
// file.
//
// NOTE ON KEY MATERIAL: the real secret keys live in proprietary/encrypted
// vendor assets and are NOT shipped in this source tree. Each entry below
// carries the correct *name* and *cipher type* (these are not secret), but the
// `key` is left as 0 where the real value is unknown. Drop the real 64-bit
// value into the matching row, or supply an SD `/mfcodes` file (semicolon
// format `name;key_hex;type`) which is merged on top of this table at runtime
// by KeeloqKeystore.
// ---------------------------------------------------------------------------
struct RollingMfKey {
    const char *name; // identifier string matched against RollingProtocol::mf_key_name
    uint64_t key;     // 64-bit key material (0 = supply your own / SD merge)
    uint8_t type;     // one of the CIPHER_* constants above
};

extern const RollingMfKey rolling_mf_keys[];
extern const size_t rolling_mf_keys_count;

// Look up a manufacturer key by name. Returns true and fills out_key/out_type
// on success. Consults the compiled-in table; the SD `/mfcodes` merge happens
// in KeeloqKeystore.
bool rolling_mf_key_lookup(const char *name, uint64_t *out_key, uint8_t *out_type);

// ---------------------------------------------------------------------------
// Protocol families.
// ---------------------------------------------------------------------------
enum RollingFamily {
    RF_FAMILY_ALUTECH_AT4N = 0,
    RF_FAMILY_BENINCA_ARC,
    RF_FAMILY_FAAC_SLH,
    RF_FAMILY_JAROLIFT,
    RF_FAMILY_KEELOQ,
    RF_FAMILY_NICE_FLOR_S,
    RF_FAMILY_NICE_ONE,
    RF_FAMILY_SECURITY_PLUS_1,
    RF_FAMILY_SECURITY_PLUS_2,
    RF_FAMILY_SOMFY_KEYTIS,
    RF_FAMILY_SOMFY_TELIS,
};

struct RollingProtocol {
    const char *display_name; // e.g. "FAAC SLH 868MHz"
    RollingFamily family;
    uint32_t frequency_hz;  // e.g. 868350000
    const char *mf_key_name; // name to look up in rolling_mf_keys (nullptr = self-contained / runtime)
    bool has_seed;          // true: form shows a Seed field (FAAC SLH, Jarolift)
    bool has_crc;           // true: .sub file carries a CRC field (Alutech AT-4N)
    uint8_t serial_bytes;   // byte widths for Create Signal form fields
    uint8_t button_bytes;
    uint8_t counter_bytes;
    uint8_t seed_bytes;     // 0 if not applicable
    uint16_t data_bit_count; // total bits in the OTA frame
    int te_short_us;        // base pulse timings for the encoder
    int te_long_us;
    uint8_t bw_preset;      // BW_OOK_270 or BW_OOK_650
};

extern const RollingProtocol rolling_protocols[];
extern const size_t rolling_protocols_count;

// Find a protocol descriptor by display name. Returns nullptr if not found.
const RollingProtocol *rolling_protocol_by_name(const char *display_name);
// Find the first protocol descriptor matching a family. Returns nullptr if not found.
const RollingProtocol *rolling_protocol_by_family(RollingFamily family);

// True if a CIPHER_* type is a plain KeeLoq manufacturer cipher (i.e. selectable
// as a manufacturer under the single flat "KeeLoq" protocol entry). Excludes the
// protocols that, although KeeLoq-based, are their own registry entries
// (FAAC SLH, Jarolift) and the non-KeeLoq AES cipher (Beninca ARC).
bool rolling_mf_is_keeloq_manufacturer(uint8_t type);

// True if a protocol family needs secret key material (a manufacturer key or a
// rainbow table) to produce/decode valid frames. Self-contained families
// (Somfy, Security+) return false.
bool rolling_family_needs_key(RollingFamily family);

// True if the required key material for this protocol is actually present (non
// zero). For KeeLoq, pass the chosen manufacturer name. Self-contained families
// always return true.
bool rolling_key_present(const RollingProtocol *proto, const char *keeloq_mf_name);

// ---------------------------------------------------------------------------
// Rainbow tables / AES key loaded from SD at boot.
// Place raw binary files on the SD card:
//   /nice_flors.bin   — 32 bytes, Nice FloR-S / Nice One table
//   /alutech.bin      — 32 bytes, Alutech AT-4N table
//   /beninca.bin      — 16 bytes, Beninca ARC AES-128 key
// Call rolling_code_db_load_sd(fs) once at startup. Without these files the
// protocols still work but encryption is a no-op (counters increment, output
// is garbage — same as the all-zero compiled placeholder).
// ---------------------------------------------------------------------------
extern uint8_t nice_flor_s_rainbow[32];
extern uint8_t alutech_at4n_rainbow[32];
extern uint8_t beninca_arc_aes_key[16];

// Load rainbow tables and Beninca key from SD. Safe to call multiple times.
void rolling_code_db_load_sd(FS *fs);

#endif
