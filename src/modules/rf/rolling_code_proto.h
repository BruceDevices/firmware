#ifndef __ROLLING_CODE_PROTO_H__
#define __ROLLING_CODE_PROTO_H__

#include "rolling_code_db.h"
#include "structs.h"
#include <stdint.h>
#include <vector>

// ---------------------------------------------------------------------------
// Per-protocol pulse builders. Each returns a signed-int timing vector for
// rolling_code_tx(): positive = HIGH µs, negative = LOW µs. Ported from
// Momentum's *_get_upload() functions (index arithmetic only, no HAL).
// ---------------------------------------------------------------------------
std::vector<int> encode_keeloq(uint64_t data64, int te_short, int te_long); // generic KeeLoq PWM
std::vector<int> encode_faac_slh(uint64_t data64);                          // te 255/595
std::vector<int> encode_jarolift(uint64_t data64, uint8_t tail8);          // te 400/800, 72-bit
std::vector<int> encode_nice_flor_s(uint64_t data52);                       // te 500/1000, 52-bit
std::vector<int> encode_nice_one(uint64_t data72);                          // te 500/1000, 72-bit
std::vector<int> encode_alutech_at4n(uint64_t payload64);                   // te 400/800, encrypts + CRC
std::vector<int> encode_security_plus_1(uint32_t fixed, uint32_t rolling);  // tri-state, keyless rolling
std::vector<int> encode_security_plus_2(uint64_t data);                     // 64-bit
std::vector<int> encode_somfy(const uint8_t frame[7]);                      // Manchester, 56-bit
std::vector<int> encode_somfy_keytis(const uint8_t frame[10]);             // Manchester, 80-bit

// ---------------------------------------------------------------------------
// High-level dispatcher. Re-encrypts `data` at its current counter using the
// descriptor `proto`, stores the resulting OTA payload back into `data` (key /
// crc_field as applicable) and returns the timing vector ready for TX.
//
// Returns an empty vector if the protocol cannot be encoded (e.g. missing key
// material). Pure computation — does not touch the radio.
// ---------------------------------------------------------------------------
std::vector<int> rolling_encode(RfCodes &data, const RollingProtocol *proto);

// Generate random parameters for a Create Signal form, masked to the field
// widths in `proto`. Fills serial/btn/cnt/seed/somfy_key as applicable.
void rolling_randomize(RfCodes &data, const RollingProtocol *proto);

// Decode a captured rolling signal. `pulses` is the raw signed-µs pulse train
// (+HIGH/-LOW). data.rolling_protocol must already be set (by the framing
// detector). Demodulates the frame and fills serial/btn/cnt/seed/somfy_key/key
// for the identified family — fully for the keyless families (Somfy, Security+),
// and the cleartext fixed fields (serial/button) plus a key-gated counter for
// the encrypted families. Returns true if anything was decoded.
bool rolling_decode(RfCodes &data, const std::vector<int> &pulses);

#endif
