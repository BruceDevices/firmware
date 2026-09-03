#include "esl_nfc.h"
#include "test_util.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Pinned vector: Color 2.6 barcode from test_store, 10-char computed by the
 * inverse below (test-only; production has no encoder). */
#define ESL_NFC_TAG10 "c37DcEVoBM"
#define ESL_NFC_BARCODE "A4165420155216265"

/* Inverse of CHAR_LUT — test file only. */
static const char ESL_NFC_INV[64] = {
    'M', 'O', 'S', '8', 'H', '5', '7', 'i', 'P', 'c', 'v', 'X', 'W', 'a',
    'l', 'B', 'D', 'x', 'e', 'K', 'A', 'w', 's', '6', '1', 'm', 'N', 'y',
    '0', 'f', '2', 'R', 'n', 'Z', 'J', '-', 't', 'L', '3', 'b', '9', 'Q',
    'p', 'd', 'E', 'T', 'F', 'q', 'I', 'u', '_', 'z', 'g', 'r', 'C', 'U',
    'o', 'Y', '4', 'h', 'V', 'j', 'k', 'G',
};

static void encode_b64_5(uint32_t val, char out[6]) {
    int i;
    for (i = 0; i < 5; i++) {
        out[i] = ESL_NFC_INV[val % 64u];
        val /= 64u;
    }
    out[5] = '\0';
}

/* Tiny inverse of esl_nfc_decode_tag10. Used to pin ESL_NFC_TAG10. */
static int encode_tag10(const char *barcode, char tag10[11]) {
    char raw[19];
    unsigned long val1;
    unsigned long val2;
    char a[6];
    char b[6];

    if (barcode == NULL || barcode[0] < 'A' || barcode[0] > 'Z' ||
        strlen(barcode) != 17) {
        return 0;
    }
    snprintf(raw, sizeof(raw), "%02d%.16s", (int)(barcode[0] - 65), barcode + 1);
    if (sscanf(raw, "%09lu%09lu", &val1, &val2) != 2) return 0;
    encode_b64_5((uint32_t)val2, a);
    encode_b64_5((uint32_t)val1, b);
    memcpy(tag10, a, 5);
    memcpy(tag10 + 5, b, 5);
    tag10[10] = '\0';
    return 1;
}

static void test_decode_tag10_rejects(void) {
    char barcode[18];

    memset(barcode, 0x5A, sizeof(barcode));
    CHECK(!esl_nfc_decode_tag10("", barcode));
    CHECK(!esl_nfc_decode_tag10("short", barcode));
    CHECK(!esl_nfc_decode_tag10("12345678901", barcode));

    /* '!' is outside the custom 64-char alphabet. */
    CHECK(!esl_nfc_decode_tag10("ABCDEFGHI!", barcode));
    CHECK(!esl_nfc_decode_tag10("!!!!!!!!!!", barcode));

    /* All-'M' (index 0) decodes to A0000000000000000 — barcode[1] != '4'. */
    CHECK(!esl_nfc_decode_tag10("MMMMMMMMMM", barcode));

    /* val1=4000000, val2=0 → A4000000000000000; checksum wants 9, got 0. */
    CHECK(!esl_nfc_decode_tag10("MMMMMMtDBM", barcode));
}

static void test_decode_ul_pages_rejects(void) {
    uint8_t pages[11][4];
    char barcode[18];

    memset(pages, 0, sizeof(pages));
    pages[3][0] = 0xE1;
    pages[4][0] = 0x03;
    pages[4][1] = 15;

    CHECK(!esl_nfc_decode_ul_pages(pages, 10, barcode));
    CHECK(!esl_nfc_decode_ul_pages(pages, 0, barcode));

    pages[3][0] = 0x00;
    CHECK(!esl_nfc_decode_ul_pages(pages, 11, barcode));

    pages[3][0] = 0xE1;
    pages[4][0] = 0x00;
    CHECK(!esl_nfc_decode_ul_pages(pages, 11, barcode));

    pages[4][0] = 0x03;
    pages[4][1] = 4; /* ndef_len < 5 */
    CHECK(!esl_nfc_decode_ul_pages(pages, 11, barcode));
}

static void test_decode_tag10_accepts_pinned(void) {
    char got[11];
    char barcode[18];

    CHECK(encode_tag10(ESL_NFC_BARCODE, got));
    CHECK_STR(got, ESL_NFC_TAG10);

    memset(barcode, 0, sizeof(barcode));
    CHECK(esl_nfc_decode_tag10(ESL_NFC_TAG10, barcode));
    CHECK_STR(barcode, ESL_NFC_BARCODE);
}

static void test_decode_ul_pages_accepts_synthetic(void) {
    uint8_t pages[11][4];
    char barcode[18];
    const char *tag = ESL_NFC_TAG10;

    memset(pages, 0, sizeof(pages));
    pages[3][0] = 0xE1;
    pages[4][0] = 0x03;
    pages[4][1] = 15; /* payload_end = 17 → '/' + 10-char URI tail */
    pages[5][2] = '/';
    pages[5][3] = (uint8_t)tag[0];
    pages[6][0] = (uint8_t)tag[1];
    pages[6][1] = (uint8_t)tag[2];
    pages[6][2] = (uint8_t)tag[3];
    pages[6][3] = (uint8_t)tag[4];
    pages[7][0] = (uint8_t)tag[5];
    pages[7][1] = (uint8_t)tag[6];
    pages[7][2] = (uint8_t)tag[7];
    pages[7][3] = (uint8_t)tag[8];
    pages[8][0] = (uint8_t)tag[9];

    memset(barcode, 0, sizeof(barcode));
    CHECK(esl_nfc_decode_ul_pages(pages, 11, barcode));
    CHECK_STR(barcode, ESL_NFC_BARCODE);
}

int main(void) {
    test_decode_tag10_rejects();
    test_decode_ul_pages_rejects();
    test_decode_tag10_accepts_pinned();
    test_decode_ul_pages_accepts_synthetic();
    TEST_REPORT("test_nfc");
}
