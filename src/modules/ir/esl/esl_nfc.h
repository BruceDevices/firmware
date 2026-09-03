/* ESL NFC barcode decode.
 *
 * ESL tags carry an NDEF URI whose last 10 characters encode the ESL ID
 * with a custom base64 alphabet. This module is a Flipper-types-free port
 * of TagTinker/nfc/tagtinker_nfc.c. */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool esl_nfc_decode_tag10(const char *tag10, char barcode[18]);
bool esl_nfc_decode_ul_pages(const uint8_t pages[][4], unsigned pages_read,
                             char barcode[18]);
/* Parse Bruce RFID `strAllPages` text ("Page N: AA BB CC DD"). Returns the
 * highest page index + 1, or 0 if no UL-sized lines were found. Classic
 * 16-byte "Page N:" lines and T4T dumps do not count. */
unsigned esl_nfc_parse_ul_dump(const char *dump, uint8_t pages[][4],
                               unsigned cap);

#ifdef __cplusplus
}
#endif
