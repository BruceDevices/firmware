/* BMP reading for ESL image uploads.
 *
 * Consumes the BMPs produced by TagTinker's web image prep tool. Note that bpp
 * is a plane marker, not a real bit depth: 1 means one 1-bit plane, and 2 means
 * two stacked 1-bit planes (identical row stride) where the header height is
 * the per-plane height and the accent plane begins at row offset height.
 *
 * Pure logic with no filesystem or hardware dependency: the caller loads the
 * whole file into RAM first, which is also what the encode-then-transmit
 * ordering rule requires. */
#pragma once

/* Via esl_proto.h, never the vendored header directly: that keeps the vendor
 * symbols' linkage independent of include order in C++ callers. */
#include "esl_proto.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t data_offset;
    uint32_t row_stride;
    uint16_t width;
    uint16_t height; /* per-plane height */
    uint16_t bpp;    /* 1, 2, 24 or 32 (1/2 are plane-count markers) */
    bool top_down;
} EslBmpInfo;

/* Parses the 54-byte header. Accepts bpp 1, 2, 24 and 32 — the same set as
 * upstream's tx_bmp_open — and computes upstream's per-bpp row stride.
 * Refusing non-1/2 belongs to the Color 2.6 send path, not here. Returns false
 * only for a bad magic, an unsupported bpp, or a too-short buffer. */
bool esl_bmp_parse(const uint8_t *file, size_t len, EslBmpInfo *info);

/* Nearest-neighbour coordinate mapping, clamped to the source extent. */
uint16_t esl_bmp_map_x(uint16_t out_x, uint16_t tx_w, uint16_t src_w);
uint16_t esl_bmp_map_y(uint16_t out_y, uint16_t tx_h, uint16_t src_h);

typedef struct {
    const uint8_t *file;
    size_t file_len;
    const EslBmpInfo *info;
} EslColor26BmpCtx;

/* TagTinkerPixelAtFn over the Color 2.6 wire space (152x296, two planes).
 * Handles the wire->glass transpose, source orientation and rescaling.
 * ctx must be an EslColor26BmpCtx*. */
uint8_t esl_color26_bmp_pixel(size_t idx, void *ctx);

typedef struct {
    const uint8_t *file;
    size_t file_len;
    const EslBmpInfo *info;
    uint16_t out_w; /* target profile width */
    uint16_t out_h; /* target profile height */
    bool second_plane;
} EslGenericBmpCtx;

/* TagTinkerPixelAtFn for every non-Color-2.6 dot-matrix profile: plain
 * nearest-neighbour rescale to out_w x out_h, with the accent plane taken from
 * a stacked 2-plane source or left clear. ctx must be an EslGenericBmpCtx*.
 * Total pixel count is out_w * out_h * (second_plane ? 2 : 1). */
uint8_t esl_generic_bmp_pixel(size_t idx, void *ctx);

#ifdef __cplusplus
}
#endif
