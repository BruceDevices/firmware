#include "esl_bmp.h"

#define ESL_BMP_HEADER_SIZE 54u

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

bool esl_bmp_parse(const uint8_t *file, size_t len, EslBmpInfo *info) {
    if (file == NULL || info == NULL || len < ESL_BMP_HEADER_SIZE) return false;
    if (file[0] != 'B' || file[1] != 'M') return false;

    /* Same accepted set as upstream tx_bmp_open. */
    const uint16_t bpp = rd16(&file[28]);
    if (bpp != 1u && bpp != 2u && bpp != 24u && bpp != 32u) return false;

    int32_t h = (int32_t)rd32(&file[22]);
    info->top_down = false;
    if (h < 0) {
        info->top_down = true;
        h = -h;
    }

    info->bpp = bpp;
    info->width = (uint16_t)rd32(&file[18]);
    info->height = (uint16_t)h;
    info->data_offset = rd32(&file[10]);

    /* Upstream's per-bpp strides. Markers 1 and 2 both store one bit per
     * pixel, so they share a stride. */
    if (bpp == 1u || bpp == 2u) {
        info->row_stride = (((uint32_t)info->width + 31u) / 32u) * 4u;
    } else if (bpp == 24u) {
        info->row_stride = (((uint32_t)info->width * 3u) + 3u) & ~3u;
    } else {
        info->row_stride = (uint32_t)info->width * 4u;
    }

    if (info->width == 0u || info->height == 0u) return false;
    if (info->data_offset >= len) return false;
    return true;
}

uint16_t esl_bmp_map_x(uint16_t out_x, uint16_t tx_w, uint16_t src_w) {
    if (tx_w == 0u || src_w == 0u) return 0u;
    uint32_t x = (uint32_t)out_x * (uint32_t)src_w / (uint32_t)tx_w;
    if (x >= src_w) x = (uint32_t)src_w - 1u;
    return (uint16_t)x;
}

uint16_t esl_bmp_map_y(uint16_t out_y, uint16_t tx_h, uint16_t src_h) {
    if (tx_h == 0u || src_h == 0u) return 0u;
    uint32_t y = (uint32_t)out_y * (uint32_t)src_h / (uint32_t)tx_h;
    if (y >= src_h) y = (uint32_t)src_h - 1u;
    return (uint16_t)y;
}

/* Maps a wire pixel to a source pixel, honouring the source's orientation:
 * already-wire BMPs pass straight through, glass BMPs are transposed, and any
 * other size is transposed then nearest-neighbour rescaled. */
static void color26_src_xy(const EslBmpInfo *info, uint16_t px, uint16_t py,
                           uint16_t *sx, uint16_t *sy) {
    if (info->width == TAGTINKER_COLOR26_WIRE_W &&
        info->height == TAGTINKER_COLOR26_WIRE_H) {
        *sx = px;
        *sy = py;
        return;
    }

    uint16_t gx = 0u, gy = 0u;
    tagtinker_color26_proto_to_glass(TAGTINKER_COLOR26_WIRE_W, px, py, &gx, &gy);

    if (info->width == TAGTINKER_COLOR26_GLASS_W &&
        info->height == TAGTINKER_COLOR26_GLASS_H) {
        *sx = gx;
        *sy = gy;
        return;
    }

    *sx = esl_bmp_map_x(gx, TAGTINKER_COLOR26_GLASS_W, info->width);
    *sy = esl_bmp_map_y(gy, TAGTINKER_COLOR26_GLASS_H, info->height);
}

/* Returns the ESL bit at source (bx, by) in the given plane. Shared by both
 * pixel callbacks. Out-of-range reads return 1 (clear) rather than touching
 * memory. One bit per pixel, matching upstream's bmp_read_pixel. */
static uint8_t esl_bmp_bit(const uint8_t *file, size_t file_len,
                           const EslBmpInfo *info, uint16_t bx, uint16_t by,
                           uint8_t plane) {
    if (info->width == 0u || info->height == 0u) return 1u;
    if (bx >= info->width) bx = (uint16_t)(info->width - 1u);
    if (by >= info->height) by = (uint16_t)(info->height - 1u);

    const uint16_t row =
        info->top_down ? by : (uint16_t)(info->height - 1u - by);
    uint32_t off = info->data_offset +
                   ((uint32_t)row + (uint32_t)plane * (uint32_t)info->height) *
                       info->row_stride;
    off += (uint32_t)bx / 8u;
    if (off >= file_len) return 1u;

    /* BMP bit 1 is white, which is ESL 0. */
    const uint8_t bit = (uint8_t)((file[off] >> (7u - (bx % 8u))) & 1u);
    return bit ? 0u : 1u;
}

uint8_t esl_color26_bmp_pixel(size_t idx, void *ctx) {
    const EslColor26BmpCtx *c = (const EslColor26BmpCtx *)ctx;
    if (c == NULL || c->file == NULL || c->info == NULL) return 1u;

    const size_t plane_count = (size_t)TAGTINKER_COLOR26_WIRE_W *
                               (size_t)TAGTINKER_COLOR26_WIRE_H;
    uint8_t plane = 0u;
    if (idx >= plane_count) {
        plane = 1u;
        idx -= plane_count;
    }
    if (idx >= plane_count) return 1u;

    /* A 1-plane source has no accent data. */
    if (plane == 1u && c->info->bpp != 2u) return 1u;

    const uint16_t px = (uint16_t)(idx % TAGTINKER_COLOR26_WIRE_W);
    const uint16_t py = (uint16_t)(idx / TAGTINKER_COLOR26_WIRE_W);
    uint16_t bx = 0u, by = 0u;
    color26_src_xy(c->info, px, py, &bx, &by);
    return esl_bmp_bit(c->file, c->file_len, c->info, bx, by, plane);
}

uint8_t esl_generic_bmp_pixel(size_t idx, void *ctx) {
    const EslGenericBmpCtx *c = (const EslGenericBmpCtx *)ctx;
    if (c == NULL || c->file == NULL || c->info == NULL) return 1u;
    if (c->out_w == 0u || c->out_h == 0u) return 1u;

    const size_t plane_count = (size_t)c->out_w * (size_t)c->out_h;
    uint8_t plane = 0u;
    if (idx >= plane_count) {
        plane = 1u;
        idx -= plane_count;
    }
    if (idx >= plane_count) return 1u;

    /* A 1-plane source has no accent data, so the accent plane reads clear. */
    if (plane == 1u && c->info->bpp != 2u) return 1u;

    const uint16_t ox = (uint16_t)(idx % c->out_w);
    const uint16_t oy = (uint16_t)(idx / c->out_w);
    const uint16_t sx = esl_bmp_map_x(ox, c->out_w, c->info->width);
    const uint16_t sy = esl_bmp_map_y(oy, c->out_h, c->info->height);
    return esl_bmp_bit(c->file, c->file_len, c->info, sx, sy, plane);
}
