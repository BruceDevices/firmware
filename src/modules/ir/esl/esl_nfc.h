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

#ifdef __cplusplus
}
#endif
