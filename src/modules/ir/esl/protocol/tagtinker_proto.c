/*
 * ESL protocol helpers.
 */

#include "tagtinker_proto.h"
#include "../tagtinker_app.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    uint16_t type_code;
    uint16_t width;
    uint16_t height;
    TagTinkerTagKind kind;
    TagTinkerTagColor color;
    const char* model_name;
    uint8_t pl_bit_def;
} TagTinkerProfileEntry;

static const TagTinkerProfileEntry profile_table[] = {
    {1206, 0,   0,   TagTinkerTagKindSegment,   TagTinkerTagColorMono,   "Continuum E2 HCS", 0},
    {1207, 0,   0,   TagTinkerTagKindSegment,   TagTinkerTagColorMono,   "Continuum E2 HCN", 4},
    {1217, 0,   0,   TagTinkerTagKindSegment,   TagTinkerTagColorMono,   "Continuum E5 HCS", 2},
    {1219, 0,   0,   TagTinkerTagKindSegment,   TagTinkerTagColorMono,   "Continuum E5 HCN", 1},
    {1240, 0,   0,   TagTinkerTagKindSegment,   TagTinkerTagColorMono,   "Continuum E4 HCS", 3},
    {1241, 0,   0,   TagTinkerTagKindSegment,   TagTinkerTagColorMono,   "Continuum E4 HCN", 0},
    {1242, 0,   0,   TagTinkerTagKindSegment,   TagTinkerTagColorMono,   "Continuum E4 HCN FZ", 0},
    {1243, 0,   0,   TagTinkerTagKindSegment,   TagTinkerTagColorMono,   "Continuum E4 HCW", 0},
    {1265, 0,   0,   TagTinkerTagKindSegment,   TagTinkerTagColorMono,   "Continuum E5 HCS", 2},
    {1275, 320, 192, TagTinkerTagKindDotMatrix, TagTinkerTagColorMono,   "DM110", 0},
    {1276, 320, 140, TagTinkerTagKindDotMatrix, TagTinkerTagColorMono,   "DM90", 0},
    {1291, 0,   0,   TagTinkerTagKindSegment,   TagTinkerTagColorMono,   "FVL Promoline 3-16", 0},
    {1300, 172, 72,  TagTinkerTagKindDotMatrix, TagTinkerTagColorMono,   "DM3370", 0},
    {1314, 400, 300, TagTinkerTagKindDotMatrix, TagTinkerTagColorMono,   "SmartTag HD110", 0},
    {1315, 296, 128, TagTinkerTagKindDotMatrix, TagTinkerTagColorMono,   "SmartTag HD L", 0},
    {1317, 152, 152, TagTinkerTagKindDotMatrix, TagTinkerTagColorMono,   "SmartTag HD S", 0},
    {1318, 208, 112, TagTinkerTagKindDotMatrix, TagTinkerTagColorMono,   "SmartTag HD M", 0},
    {1319, 800, 480, TagTinkerTagKindDotMatrix, TagTinkerTagColorMono,   "SmartTag HD200", 0},
    {1322, 152, 152, TagTinkerTagKindDotMatrix, TagTinkerTagColorMono,   "SmartTag HD S", 0},
    {1324, 208, 112, TagTinkerTagKindDotMatrix, TagTinkerTagColorMono,   "SmartTag HD M FZ", 0},
    {1327, 208, 112, TagTinkerTagKindDotMatrix, TagTinkerTagColorRed,    "SmartTag HD M Red", 0},
    {1328, 296, 128, TagTinkerTagKindDotMatrix, TagTinkerTagColorRed,    "SmartTag HD L Red", 0},
    {1336, 400, 300, TagTinkerTagKindDotMatrix, TagTinkerTagColorRed,    "SmartTag HD110 Red", 0},
    {1339, 152, 152, TagTinkerTagKindDotMatrix, TagTinkerTagColorRed,    "SmartTag HD S Red", 0},
    {1340, 800, 480, TagTinkerTagKindDotMatrix, TagTinkerTagColorRed,    "SmartTag HD200 Red", 0},
    {1344, 296, 128, TagTinkerTagKindDotMatrix, TagTinkerTagColorYellow, "SmartTag HD L Yellow", 0},
    {1346, 800, 480, TagTinkerTagKindDotMatrix, TagTinkerTagColorYellow, "SmartTag HD200 Yellow", 0},
    {1348, 264, 176, TagTinkerTagKindDotMatrix, TagTinkerTagColorRed,    "SmartTag HD T Red", 0},
    {1349, 264, 176, TagTinkerTagKindDotMatrix, TagTinkerTagColorYellow, "SmartTag HD T Yellow", 0},
    {1351, 648, 480, TagTinkerTagKindDotMatrix, TagTinkerTagColorMono,   "SmartTag HD150", 0},
    {1353, 648, 480, TagTinkerTagKindDotMatrix, TagTinkerTagColorRed,    "SmartTag HD150 Red", 0},
    {1354, 648, 480, TagTinkerTagKindDotMatrix, TagTinkerTagColorRed,    "SmartTag HD150 Red", 0},
    {1370, 296, 128, TagTinkerTagKindDotMatrix, TagTinkerTagColorRed,    "SmartTag HD L Red (2021)", 0},
    {1371, 648, 480, TagTinkerTagKindDotMatrix, TagTinkerTagColorRed,    "SmartTag HD150 Red (2021)", 0},
    {1510, 0,   0,   TagTinkerTagKindSegment,   TagTinkerTagColorMono,   "SmartTag E5 M", 1},
    {TAGTINKER_TYPE_SMARTAG_COLOR_26, TAGTINKER_COLOR26_WIRE_W, TAGTINKER_COLOR26_WIRE_H,
     TagTinkerTagKindDotMatrix, TagTinkerTagColorRed, "SmartTAG Color 2.6", 0},
    {1627, 296, 128, TagTinkerTagKindDotMatrix, TagTinkerTagColorRed,    "SmartTag HD L Red", 0},
    {1628, 296, 128, TagTinkerTagKindDotMatrix, TagTinkerTagColorRed,    "SmartTag HD L Red", 0},
    {1639, 152, 152, TagTinkerTagKindDotMatrix, TagTinkerTagColorRed,    "SmartTag HD S Red", 0},
};

static const TagTinkerProfileEntry* find_profile_entry(uint16_t type_code) {
    for(size_t i = 0; i < COUNT_OF(profile_table); i++) {
        if(profile_table[i].type_code == type_code) return &profile_table[i];
    }
    return NULL;
}

uint16_t tagtinker_crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0x8408;
    for(size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for(int b = 0; b < 8; b++)
            crc = (crc & 1) ? (crc >> 1) ^ 0x8408 : crc >> 1;
    }
    return crc;
}

static size_t terminate(uint8_t* buf, size_t len) {
    uint16_t crc = tagtinker_crc16(buf, len);
    buf[len]     = crc & 0xFF;
    buf[len + 1] = (crc >> 8) & 0xFF;
    return len + 2;
}

static size_t raw_frame(uint8_t* buf, uint8_t proto, const uint8_t plid[4], uint8_t cmd) {
    buf[0] = proto;
    memcpy(&buf[1], plid, 4);
    buf[5] = cmd;
    return 6;
}

static size_t mcu_frame(uint8_t* buf, const uint8_t plid[4], uint8_t cmd) {
    size_t p = raw_frame(buf, TAGTINKER_PROTO_DM, plid, 0x34);
    buf[p++] = 0x00;
    buf[p++] = 0x00;
    buf[p++] = 0x00;
    buf[p++] = cmd;
    return p;
}

static void append_word(uint8_t* buf, size_t* p, uint16_t value) {
    buf[(*p)++] = (value >> 8) & 0xFF;
    buf[(*p)++] = value & 0xFF;
}

bool tagtinker_barcode_to_plid(const char* barcode, uint8_t plid[4]) {
    if(!barcode || strlen(barcode) != 17) return false;
    uint64_t a = 0, b = 0;
    for(int i = 2; i < 7; i++)  a = a * 10 + (barcode[i] - '0');
    for(int i = 7; i < 12; i++) b = b * 10 + (barcode[i] - '0');
    
    uint64_t id = (a << 16) | b;
    plid[0] = id & 0xFF; // LSB first
    plid[1] = (id >> 8)  & 0xFF;
    plid[2] = (id >> 16) & 0xFF;
    plid[3] = (id >> 24) & 0xFF;
    return true;
}

bool tagtinker_barcode_to_type(const char* barcode, uint16_t* type_code) {
    if(!barcode || strlen(barcode) != 17 || !type_code) return false;
    uint16_t type = 0;
    for(int i = 12; i < 16; i++) type = type * 10 + (barcode[i] - '0');
    *type_code = type;
    return true;
}

bool tagtinker_barcode_to_profile(const char* barcode, TagTinkerTagProfile* profile) {
    if(!profile) return false;
    memset(profile, 0, sizeof(*profile));
    uint16_t type_code = 0;
    if(!tagtinker_barcode_to_type(barcode, &type_code)) return false;
    profile->type_code = type_code;
    const TagTinkerProfileEntry* entry = find_profile_entry(type_code);
    if(!entry) return false;
    profile->width = entry->width; profile->height = entry->height;
    profile->kind = entry->kind; profile->color = entry->color;
    profile->model_name = entry->model_name; profile->pl_bit_def = entry->pl_bit_def;
    profile->known = true;
    return true;
}

size_t tagtinker_make_ping_frame(uint8_t* buf, const uint8_t plid[4]) {
    size_t p = raw_frame(buf, TAGTINKER_PROTO_DM, plid, 0x97);
    buf[p++] = 0x01;
    buf[p++] = 0x00;
    buf[p++] = 0x00;
    buf[p++] = 0x00;
    for(int i = 0; i < 20; i++) buf[p++] = 0x01;
    return terminate(buf, p);
}

size_t tagtinker_make_wake_frame(uint8_t* buf, const uint8_t plid[4]) {
    /* PrecIR / PriceHax wake: cmd 0x17 and 22 trailing 0x01 bytes. */
    size_t p = raw_frame(buf, TAGTINKER_PROTO_DM, plid, 0x17);
    buf[p++] = 0x01;
    buf[p++] = 0x00;
    buf[p++] = 0x00;
    buf[p++] = 0x00;
    for(int i = 0; i < 22; i++) buf[p++] = 0x01;
    return terminate(buf, p);
}

size_t tagtinker_make_refresh_frame(uint8_t* buf, const uint8_t plid[4]) {
    size_t p = mcu_frame(buf, plid, 0x01);
    for(int i = 0; i < 18; i++) buf[p++] = 0x00;
    return terminate(buf, p);
}

size_t tagtinker_make_image_param_frame(
    uint8_t* buf, const uint8_t plid[4], uint16_t byte_count, uint8_t comp_type,
    uint8_t page, uint16_t width, uint16_t height, uint16_t pos_x, uint16_t pos_y) {
    size_t p = mcu_frame(buf, plid, 0x05);
    append_word(buf, &p, byte_count);
    buf[p++] = 0x00;
    buf[p++] = comp_type;
    buf[p++] = page;
    append_word(buf, &p, width);
    append_word(buf, &p, height);
    append_word(buf, &p, pos_x);
    append_word(buf, &p, pos_y);
    append_word(buf, &p, 0x0000);
    buf[p++] = 0x88;
    append_word(buf, &p, 0x0000);
    for(int i = 0; i < 4; i++) buf[p++] = 0x00;
    return terminate(buf, p);
}

size_t tagtinker_make_image_data_frame(
    uint8_t* buf,
    const uint8_t plid[4],
    uint16_t index,
    const uint8_t data[TAGTINKER_IMAGE_DATA_BYTES_PER_FRAME]) {
    size_t p = mcu_frame(buf, plid, 0x20);
    append_word(buf, &p, index);
    memcpy(&buf[p], data, TAGTINKER_IMAGE_DATA_BYTES_PER_FRAME);
    p += TAGTINKER_IMAGE_DATA_BYTES_PER_FRAME;
    return terminate(buf, p);
}

typedef struct { uint8_t* data; size_t bit_pos; } TagTinkerBitWriter;
static inline void bit_writer_append(TagTinkerBitWriter* writer, uint8_t bit) {
    size_t byte_idx = writer->bit_pos / 8U;
    size_t bit_idx = 7U - (writer->bit_pos % 8U);
    if(bit) writer->data[byte_idx] |= (uint8_t)(1U << bit_idx);
    writer->bit_pos++;
}

static size_t record_run_bit_length(uint32_t count) {
    size_t bits = 0; do { bits++; count >>= 1; } while(count);
    return (bits * 2U) - 1U;
}

static void bit_writer_append_run(TagTinkerBitWriter* writer, uint32_t count) {
    uint8_t bits[32]; int n = 0; uint32_t v = count;
    while(v) { bits[n++] = v & 1U; v >>= 1; }
    for(int i = 0; i < n / 2; i++) { uint8_t t = bits[i]; bits[i] = bits[n-1-i]; bits[n-1-i] = t; }
    for(int i = 1; i < n; i++) bit_writer_append(writer, 0U);
    for(int i = 0; i < n; i++) bit_writer_append(writer, bits[i]);
}

static inline uint8_t plane_pixel_at(const uint8_t* p1, const uint8_t* p2, size_t count, size_t idx) {
    return (idx < count) ? p1[idx] : p2[idx - count];
}

#define DATA_BITS_PER_FRAME (TAGTINKER_IMAGE_DATA_BYTES_PER_FRAME * 8U)

static size_t tagtinker_rle_fn_bit_length(TagTinkerPixelAtFn pixel_at, void* ctx, size_t total) {
    if(!pixel_at || total == 0) return 0;
    size_t bit_len = 1U;
    uint8_t run_pixel = pixel_at(0, ctx);
    uint32_t run_count = 1;
    for(size_t i = 1; i < total; i++) {
        uint8_t pix = pixel_at(i, ctx);
        if(pix == run_pixel) run_count++;
        else { bit_len += record_run_bit_length(run_count); run_pixel = pix; run_count = 1; }
    }
    if(run_count > 0U) bit_len += record_run_bit_length(run_count);
    return bit_len;
}

static void tagtinker_pack_fn_raw(TagTinkerPixelAtFn pixel_at, void* ctx, size_t total, uint8_t* out) {
    TagTinkerBitWriter writer = {.data = out, .bit_pos = 0};
    for(size_t i = 0; i < total; i++) bit_writer_append(&writer, pixel_at(i, ctx));
}

static void tagtinker_pack_fn_rle(TagTinkerPixelAtFn pixel_at, void* ctx, size_t total, uint8_t* out) {
    if(total == 0) return;
    TagTinkerBitWriter writer = {.data = out, .bit_pos = 0};
    uint8_t run_pixel = pixel_at(0, ctx);
    uint32_t run_count = 1;
    bit_writer_append(&writer, run_pixel);
    for(size_t i = 1; i < total; i++) {
        uint8_t pix = pixel_at(i, ctx);
        if(pix == run_pixel) run_count++;
        else { bit_writer_append_run(&writer, run_count); run_pixel = pix; run_count = 1; }
    }
    if(run_count > 0U) bit_writer_append_run(&writer, run_count);
}

bool tagtinker_encode_fn_payload(
    TagTinkerPixelAtFn pixel_at,
    void* ctx,
    size_t total,
    TagTinkerCompressionMode mode,
    TagTinkerImagePayload* payload) {
    if(!pixel_at || !payload || total == 0) return false;
    memset(payload, 0, sizeof(*payload));
    size_t comp_len = tagtinker_rle_fn_bit_length(pixel_at, ctx, total);
    bool use_compressed = (mode == TagTinkerCompressionRle) ||
                         (mode == TagTinkerCompressionAuto && comp_len > 0U && comp_len < total);
    size_t src_len = use_compressed ? comp_len : total;
    size_t padded_bits = src_len + ((DATA_BITS_PER_FRAME - (src_len % DATA_BITS_PER_FRAME)) % DATA_BITS_PER_FRAME);
    uint8_t* data = calloc(padded_bits / 8U, 1);
    if(!data) return false;
    if(use_compressed) tagtinker_pack_fn_rle(pixel_at, ctx, total, data);
    else tagtinker_pack_fn_raw(pixel_at, ctx, total, data);
    payload->data = data;
    payload->byte_count = padded_bits / 8U;
    payload->comp_type = use_compressed ? 2U : 0U;
    return true;
}

typedef struct {
    const uint8_t* p1;
    const uint8_t* p2;
    size_t count;
} TagTinkerPlaneCtx;

static uint8_t tagtinker_plane_pixel_cb(size_t idx, void* ctx) {
    const TagTinkerPlaneCtx* c = ctx;
    return plane_pixel_at(c->p1, c->p2, c->count, idx);
}

bool tagtinker_encode_planes_payload(
    const uint8_t* p1, const uint8_t* p2, size_t count, TagTinkerCompressionMode mode, TagTinkerImagePayload* payload) {
    if(!p1 || !payload) return false;
    TagTinkerPlaneCtx ctx = {.p1 = p1, .p2 = p2, .count = count};
    size_t total = p2 ? (count * 2U) : count;
    return tagtinker_encode_fn_payload(tagtinker_plane_pixel_cb, &ctx, total, mode, payload);
}

bool tagtinker_encode_image_payload(
    const uint8_t* pixels, uint16_t width, uint16_t height, bool color_clear,
    TagTinkerCompressionMode mode, TagTinkerImagePayload* payload) {
    size_t count = (size_t)width * height;
    uint8_t* second = NULL;
    if(color_clear) { second = malloc(count); if(!second) return false; memset(second, 1, count); }
    bool ok = tagtinker_encode_planes_payload(pixels, second, count, mode, payload);
    free(second); return ok;
}

void tagtinker_free_image_payload(TagTinkerImagePayload* payload) {
    if(payload && payload->data) { free(payload->data); payload->data = NULL; }
}

bool tagtinker_is_barcode_valid(const char* barcode) {
    if(!barcode || strlen(barcode) != 17) return false;
    return true;
}

size_t tagtinker_make_addressed_frame(uint8_t* buf, const uint8_t plid[4], const uint8_t* payload, size_t len) {
    size_t p = raw_frame(buf, TAGTINKER_PROTO_DM, plid, payload[0]);
    memcpy(&buf[p], payload + 1, len - 1); p += len - 1;
    return terminate(buf, p);
}

size_t tagtinker_build_broadcast_page_frame(uint8_t* buf, uint8_t page, bool forever, uint16_t duration) {
    const uint8_t plid[4] = {0};
    size_t p = raw_frame(buf, TAGTINKER_PROTO_DM, plid, 0x06);
    buf[p++] = ((page & 7) << 3) | 0x01 | (forever ? 0x80 : 0x00);
    buf[p++] = 0x00; buf[p++] = 0x00;
    buf[p++] = (duration >> 8) & 0xFF; buf[p++] = duration & 0xFF;
    return terminate(buf, p);
}

size_t tagtinker_build_broadcast_debug_frame(uint8_t* buf) {
    const uint8_t plid[4] = {0};
    size_t p = raw_frame(buf, TAGTINKER_PROTO_DM, plid, 0x06);
    buf[p++] = 0xF1; buf[p++] = 0x00; buf[p++] = 0x00; buf[p++] = 0x00; buf[p++] = 0x0A;
    return terminate(buf, p);
}
