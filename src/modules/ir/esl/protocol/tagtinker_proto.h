/*
 * ESL protocol helpers.
 *
 * This layer turns barcodes, pixels, and payload bytes into the frames
 * that the Flipper sends over IR to the tag.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define TAGTINKER_PROTO_DM  0x85
#define TAGTINKER_PROTO_SEG 0x84
#define TAGTINKER_MAX_FRAME_SIZE 96
#define TAGTINKER_IMAGE_DATA_BYTES_PER_FRAME 20U

/* Color 2.6 glass is landscape 296x152. Image header is 152x296 (short x long). */
#define TAGTINKER_TYPE_SMARTAG_COLOR_26 1626
#define TAGTINKER_COLOR26_WIRE_W 152U
#define TAGTINKER_COLOR26_WIRE_H 296U
#define TAGTINKER_COLOR26_GLASS_W 296U
#define TAGTINKER_COLOR26_GLASS_H 152U

static inline bool tagtinker_type_needs_wh_swap(uint16_t type_code) {
    return type_code == TAGTINKER_TYPE_SMARTAG_COLOR_26;
}

static inline bool tagtinker_type_uses_ui_page(uint16_t type_code) {
    return tagtinker_type_needs_wh_swap(type_code);
}

/* Store-used Color 2.6 tags keep the barcode on page 1. Page 2 is the image
 * slot. Unspecified pages (0 or 1) map there. Explicit 2-7 are left alone. */
static inline uint8_t tagtinker_color26_resolve_page(uint8_t page) {
    if(page <= 1U) return 2U;
    if(page > 7U) return 7U;
    return page;
}

/* Wire (px, py) on 152x296 -> glass (bx, by) 296x152. */
static inline void tagtinker_color26_proto_to_glass(
    uint16_t proto_w,
    uint16_t px,
    uint16_t py,
    uint16_t* bx,
    uint16_t* by) {
    *bx = py;
    *by = (uint16_t)(proto_w - 1U - px);
}

typedef struct TagTinkerApp TagTinkerApp;

/* CRC used by the ESL wire format. */
uint16_t tagtinker_crc16(const uint8_t* data, size_t len);

typedef enum {
    TagTinkerTagKindUnknown = 0,
    TagTinkerTagKindDotMatrix,
    TagTinkerTagKindSegment,
} TagTinkerTagKind;

typedef enum {
    TagTinkerTagColorMono = 0,
    TagTinkerTagColorRed,
    TagTinkerTagColorYellow,
} TagTinkerTagColor;

typedef struct {
    uint16_t type_code;
    uint16_t width;
    uint16_t height;
    TagTinkerTagKind kind;
    TagTinkerTagColor color;
    const char* model_name;
    uint8_t pl_bit_def;
    bool known;
} TagTinkerTagProfile;

static inline bool tagtinker_profile_needs_wh_swap(const TagTinkerTagProfile* profile) {
    return profile && tagtinker_type_needs_wh_swap(profile->type_code);
}

static inline bool tagtinker_profile_uses_ui_page(const TagTinkerTagProfile* profile) {
    return profile && tagtinker_type_uses_ui_page(profile->type_code);
}

static inline void tagtinker_profile_glass_size(
    const TagTinkerTagProfile* profile,
    uint16_t* width,
    uint16_t* height) {
    if(!profile || !width || !height) return;
    if(tagtinker_profile_needs_wh_swap(profile)) {
        *width = TAGTINKER_COLOR26_GLASS_W;
        *height = TAGTINKER_COLOR26_GLASS_H;
        return;
    }
    *width = profile->width;
    *height = profile->height;
}

bool tagtinker_is_barcode_valid(const char* barcode);
bool tagtinker_barcode_to_plid(const char* barcode, uint8_t plid[4]);
bool tagtinker_barcode_to_type(const char* barcode, uint16_t* type_code);
bool tagtinker_barcode_to_profile(const char* barcode, TagTinkerTagProfile* profile);

typedef struct {
    uint8_t* data;
    size_t byte_count;
    uint8_t comp_type;
} TagTinkerImagePayload;

typedef enum {
    TagTinkerCompressionAuto = 0,
    TagTinkerCompressionRaw,
    TagTinkerCompressionRle,
} TagTinkerCompressionMode;

bool tagtinker_encode_image_payload(
    const uint8_t* pixels,
    uint16_t width,
    uint16_t height,
    bool color_clear,
    TagTinkerCompressionMode mode,
    TagTinkerImagePayload* payload);

typedef uint8_t (*TagTinkerPixelAtFn)(size_t idx, void* ctx);

bool tagtinker_encode_fn_payload(
    TagTinkerPixelAtFn pixel_at,
    void* ctx,
    size_t total_pixels,
    TagTinkerCompressionMode mode,
    TagTinkerImagePayload* payload);

bool tagtinker_encode_planes_payload(
    const uint8_t* primary_pixels,
    const uint8_t* secondary_pixels,
    size_t pixel_count,
    TagTinkerCompressionMode mode,
    TagTinkerImagePayload* payload);
void tagtinker_free_image_payload(TagTinkerImagePayload* payload);
size_t tagtinker_make_image_param_frame(
    uint8_t* buf,
    const uint8_t plid[4],
    uint16_t byte_count,
    uint8_t comp_type,
    uint8_t page,
    uint16_t width,
    uint16_t height,
    uint16_t pos_x,
    uint16_t pos_y);
size_t tagtinker_make_image_data_frame(
    uint8_t* buf,
    const uint8_t plid[4],
    uint16_t frame_index,
    const uint8_t data_bytes[TAGTINKER_IMAGE_DATA_BYTES_PER_FRAME]);

/* Broadcast frames address every listening tag. */
size_t tagtinker_build_broadcast_page_frame(
    uint8_t* buf, uint8_t page, bool forever, uint16_t duration);

size_t tagtinker_build_broadcast_debug_frame(uint8_t* buf);

/* Addressed frames use the PLID decoded from the barcode. */
size_t tagtinker_make_addressed_frame(
    uint8_t* buf, const uint8_t plid[4],
    const uint8_t* payload, size_t payload_len);

/* Tags need a wake ping before most addressed commands. */
size_t tagtinker_make_ping_frame(uint8_t* buf, const uint8_t plid[4]);

size_t tagtinker_make_wake_frame(uint8_t* buf, const uint8_t plid[4]);

size_t tagtinker_make_refresh_frame(uint8_t* buf, const uint8_t plid[4]);

/* Image upload uses MCU frames: one parameter frame followed by data frames. */
size_t tagtinker_make_mcu_frame(
    uint8_t* buf, const uint8_t plid[4], uint8_t cmd);

/* RLE is the tag's compact bitmap format. Raw mode keeps one bit per pixel. */
size_t tagtinker_rle_compress(
    const uint8_t* pixels, size_t count,
    uint8_t* out, size_t out_cap, uint8_t* comp_type);

/* Builds the full IR sequence: wake, image params, data chunks, refresh. */
void tagtinker_build_image_sequence(
    TagTinkerApp* app,
    const uint8_t plid[4],
    const uint8_t* pixels,
    uint16_t width, uint16_t height,
    uint8_t page,
    uint16_t pos_x, uint16_t pos_y,
    uint16_t wake_repeats);
