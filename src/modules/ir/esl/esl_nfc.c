#include "esl_nfc.h"

#include <stdio.h>
#include <string.h>

/* Direct ASCII-to-index lookup table, -1 = not in alphabet */
static const int8_t CHAR_LUT[128] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,35,-1,-1,
    28,24,30,38,58, 5,23, 6, 3,40,-1,-1,-1,-1,-1,-1,
    -1,20,15,54,16,44,46,63, 4,48,34,19,37, 0,26, 1,
     8,41,31, 2,45,55,60,12,11,57,33,-1,-1,-1,-1,50,
    -1,13,39, 9,43,18,29,52,59, 7,61,62,14,25,32,56,
    42,47,53,22,36,49,10,21,17,27,51,-1,-1,-1,-1,-1,
};

static int alphabet_index(char c) {
    uint8_t idx = (uint8_t)c;
    if(idx >= 128) return -1;
    return CHAR_LUT[idx];
}

static uint32_t decode_b64(const char* s, int len) {
    uint32_t r = 0;
    for(int i = 0; i < len; i++) {
        int idx = alphabet_index(s[(len - 1) - i]);
        if(idx < 0) return 0;
        r = (r * 64) + (uint32_t)idx;
    }
    return r;
}

bool esl_nfc_decode_tag10(const char *tag10, char barcode[18]) {
    if(strlen(tag10) != 10) return false;

    for(int i = 0; i < 10; i++) {
        if(alphabet_index(tag10[i]) < 0) return false;
    }

    uint32_t val1 = decode_b64(tag10 + 5, 5);
    uint32_t val2 = decode_b64(tag10, 5);

    char raw[20];
    snprintf(raw, sizeof(raw), "%09lu%09lu", (unsigned long)val1, (unsigned long)val2);

    int lc = (raw[0] - '0') * 10 + (raw[1] - '0');
    if(lc > 25) return false;
    char letter = (char)(lc + 65);

    barcode[0] = letter;
    memcpy(barcode + 1, raw + 2, 16);
    barcode[17] = '\0';

    if(barcode[1] != '4') return false;

    int cs = 0;
    for(int i = 0; i < 16; i++) {
        char c = barcode[i];
        cs += (c >= 'a' && c <= 'z') ? (c - 32) : c;
    }
    return (cs % 10) == (barcode[16] - '0');
}

bool esl_nfc_decode_ul_pages(const uint8_t pages[][4], unsigned pages_read,
                             char barcode[18]) {
    if(pages_read < 11) return false;

    const uint8_t* p3 = pages[3];
    if(p3[0] != 0xE1) return false;

    const uint8_t* p4 = pages[4];
    if(p4[0] != 0x03) return false;
    uint8_t ndef_len = p4[1];
    if(ndef_len < 5) return false;

    uint8_t flat[28];
    for(int i = 0; i < 7; i++) {
        memcpy(flat + i * 4, pages[4 + i], 4);
    }

    int payload_end = 6 + (ndef_len - 4);
    if(payload_end > 28) payload_end = 28;

    char url_body[40] = {0};
    int j = 0;
    for(int i = 6; i < payload_end && j < 39; i++) {
        if(flat[i] == 0xFE) break;
        url_body[j++] = (char)flat[i];
    }

    const char* last_slash = strrchr(url_body, '/');
    if(!last_slash) return false;

    return esl_nfc_decode_tag10(last_slash + 1, barcode);
}

unsigned esl_nfc_parse_ul_dump(const char *dump, uint8_t pages[][4],
                               unsigned cap) {
    unsigned highest = 0;
    bool any = false;
    const char *p;

    if (dump == NULL || pages == NULL || cap == 0u) return 0;

    p = dump;
    while (*p != '\0') {
        const char *eol = p;
        char line[96];
        size_t linelen;
        char *s;
        char *e;
        unsigned n = 0, b0 = 0, b1 = 0, b2 = 0, b3 = 0;
        int consumed = 0;

        while (*eol != '\0' && *eol != '\n' && *eol != '\r') eol++;
        linelen = (size_t)(eol - p);
        if (linelen >= sizeof(line)) linelen = sizeof(line) - 1u;
        memcpy(line, p, linelen);
        line[linelen] = '\0';

        s = line;
        while (*s == ' ' || *s == '\t') s++;
        e = s + strlen(s);
        while (e > s && (e[-1] == ' ' || e[-1] == '\t')) {
            e--;
            *e = '\0';
        }

        if (sscanf(s, "Page %u: %x %x %x %x%n", &n, &b0, &b1, &b2, &b3,
                   &consumed) == 5) {
            const char *rest = s + consumed;
            while (*rest == ' ' || *rest == '\t') rest++;
            if (*rest == '\0' && n < cap) {
                pages[n][0] = (uint8_t)b0;
                pages[n][1] = (uint8_t)b1;
                pages[n][2] = (uint8_t)b2;
                pages[n][3] = (uint8_t)b3;
                if (!any || n + 1u > highest) highest = n + 1u;
                any = true;
            }
        }

        p = eol;
        while (*p == '\n' || *p == '\r') p++;
    }

    return any ? highest : 0;
}
