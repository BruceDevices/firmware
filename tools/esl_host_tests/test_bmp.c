#include "esl_bmp.h"
#include "test_util.h"
#include <stdlib.h>

/* Builds a minimal 54-byte-header BMP. planes=2 stacks a second plane. */
static uint8_t *make_bmp(uint16_t w, uint16_t h, uint16_t bpp, int top_down,
                         unsigned planes, size_t *out_len) {
    const uint32_t stride = ((uint32_t)(w + 31u) / 32u) * 4u;
    const uint32_t data = 54u;
    const size_t len = data + (size_t)stride * h * planes;
    uint8_t *f = (uint8_t *)calloc(len, 1);

    f[0] = 'B';
    f[1] = 'M';
    f[10] = (uint8_t)(data & 0xFF);
    f[18] = (uint8_t)(w & 0xFF);
    f[19] = (uint8_t)(w >> 8);
    int32_t hh = top_down ? -(int32_t)h : (int32_t)h;
    f[22] = (uint8_t)(hh & 0xFF);
    f[23] = (uint8_t)((hh >> 8) & 0xFF);
    f[24] = (uint8_t)((hh >> 16) & 0xFF);
    f[25] = (uint8_t)((hh >> 24) & 0xFF);
    f[28] = (uint8_t)(bpp & 0xFF);
    f[29] = (uint8_t)(bpp >> 8);

    *out_len = len;
    return f;
}

static void set_bit(uint8_t *f, const EslBmpInfo *i, unsigned plane,
                    uint16_t row, uint16_t x) {
    uint32_t off = i->data_offset +
                   ((uint32_t)row + (uint32_t)plane * i->height) * i->row_stride;
    f[off + x / 8u] |= (uint8_t)(1u << (7u - (x % 8u)));
}

static void test_parse(void) {
    size_t len = 0;
    uint8_t *f = make_bmp(296, 152, 1, 0, 1, &len);
    EslBmpInfo info;

    CHECK(esl_bmp_parse(f, len, &info));
    CHECK_EQ(info.width, 296);
    CHECK_EQ(info.height, 152);
    CHECK_EQ(info.bpp, 1);
    CHECK_EQ(info.data_offset, 54);
    CHECK_EQ(info.row_stride, 40); /* (296+31)/32*4 */
    CHECK(!info.top_down);
    free(f);

    /* Negative header height means top-down. */
    f = make_bmp(152, 296, 2, 1, 2, &len);
    CHECK(esl_bmp_parse(f, len, &info));
    CHECK(info.top_down);
    CHECK_EQ(info.height, 296);
    CHECK_EQ(info.bpp, 2);
    CHECK_EQ(info.row_stride, 20); /* (152+31)/32*4 */
    free(f);

    /* 24 and 32 bpp are accepted, with upstream's per-bpp strides. Rejecting
     * non-1/2 is the Color 2.6 send path's job, not the parser's. */
    f = make_bmp(100, 10, 24, 0, 1, &len);
    f[28] = 24;
    CHECK(esl_bmp_parse(f, len, &info));
    CHECK_EQ(info.bpp, 24);
    CHECK_EQ(info.row_stride, 300); /* (100*3 + 3) & ~3 */
    free(f);

    f = make_bmp(100, 10, 32, 0, 1, &len);
    f[28] = 32;
    CHECK(esl_bmp_parse(f, len, &info));
    CHECK_EQ(info.bpp, 32);
    CHECK_EQ(info.row_stride, 400); /* 100*4 */
    free(f);

    /* Rejections. Each case is isolated so it can only fail for its own
     * reason: the header is restored to valid between mutations. */
    f = make_bmp(8, 8, 1, 0, 1, &len);
    CHECK(esl_bmp_parse(f, len, &info)); /* valid baseline */

    f[0] = 'X';
    CHECK(!esl_bmp_parse(f, len, &info)); /* bad magic */
    f[0] = 'B';

    f[28] = 16;
    CHECK(!esl_bmp_parse(f, len, &info)); /* bpp outside the accepted set */
    f[28] = 1;

    CHECK(!esl_bmp_parse(f, 53, &info)); /* truncated header, bpp valid */
    CHECK(!esl_bmp_parse(NULL, len, &info));
    CHECK(!esl_bmp_parse(f, len, NULL));

    f[18] = 0;
    f[19] = 0;
    CHECK(!esl_bmp_parse(f, len, &info)); /* width == 0 */
    f[18] = 8;

    f[22] = 0;
    f[23] = 0;
    f[24] = 0;
    f[25] = 0;
    CHECK(!esl_bmp_parse(f, len, &info)); /* height == 0 */
    free(f);
}

/* Generic profiles: plain nearest-neighbour rescale, no transpose. */
static void test_generic_pixel(void) {
    size_t len = 0;
    /* 8x8 source, top-down, scaled up to a 16x16 output. */
    uint8_t *f = make_bmp(8, 8, 1, 1, 1, &len);
    EslBmpInfo info;
    CHECK(esl_bmp_parse(f, len, &info));

    set_bit(f, &info, 0, 0, 0); /* source (0,0) is white */

    EslGenericBmpCtx ctx = {f, len, &info, 16, 16, false};

    /* Upscale x2: output (0,0), (1,0), (0,1), (1,1) all map to source (0,0). */
    CHECK_EQ(esl_generic_bmp_pixel(0, &ctx), 0);
    CHECK_EQ(esl_generic_bmp_pixel(1, &ctx), 0);
    CHECK_EQ(esl_generic_bmp_pixel(16, &ctx), 0);
    CHECK_EQ(esl_generic_bmp_pixel(17, &ctx), 0);
    /* Output (2,0) maps to source (1,0), which is untouched. */
    CHECK_EQ(esl_generic_bmp_pixel(2, &ctx), 1);

    /* With second_plane on a 1-plane source, the accent plane reads clear. */
    EslGenericBmpCtx accent_ctx = {f, len, &info, 16, 16, true};
    CHECK_EQ(esl_generic_bmp_pixel(256, &accent_ctx), 1);
    CHECK_EQ(esl_generic_bmp_pixel(257, &accent_ctx), 1);

    /* Guards. */
    CHECK_EQ(esl_generic_bmp_pixel(0, NULL), 1);
    EslGenericBmpCtx zero_ctx = {f, len, &info, 0, 0, false};
    CHECK_EQ(esl_generic_bmp_pixel(0, &zero_ctx), 1);
    free(f);
}

/* Bottom-up BMPs store file row 0 as the last logical row. Identity output
 * size so only the flip is under test. */
static void test_generic_bottom_up_row_flip(void) {
    size_t len = 0;
    uint8_t *f = make_bmp(8, 8, 1, 0, 1, &len);
    EslBmpInfo info;
    CHECK(esl_bmp_parse(f, len, &info));
    CHECK(!info.top_down);
    CHECK_EQ(info.height, 8);

    set_bit(f, &info, 0, 7, 0); /* file row height-1 → logical y = 0 */
    set_bit(f, &info, 0, 0, 1); /* file row 0 → logical y = height-1 */

    EslGenericBmpCtx ctx = {f, len, &info, 8, 8, false};
    CHECK_EQ(esl_generic_bmp_pixel(0, &ctx), 0);          /* (0, 0) */
    CHECK_EQ(esl_generic_bmp_pixel(1, &ctx), 1);          /* (1, 0) clear */
    CHECK_EQ(esl_generic_bmp_pixel(7u * 8u + 1u, &ctx), 0); /* (1, 7) */
    CHECK_EQ(esl_generic_bmp_pixel(7u * 8u, &ctx), 1);      /* (0, 7) clear */
    free(f);
}

/* Off-diagonal bit on a non-square source: swapping x/y would fail. */
static void test_generic_no_transpose(void) {
    size_t len = 0;
    uint8_t *f = make_bmp(8, 4, 1, 1, 1, &len);
    EslBmpInfo info;
    CHECK(esl_bmp_parse(f, len, &info));

    set_bit(f, &info, 0, 0, 1); /* source (1, 0) */

    EslGenericBmpCtx ctx = {f, len, &info, 8, 4, false};
    CHECK_EQ(esl_generic_bmp_pixel(1, &ctx), 0); /* (1, 0) set */
    CHECK_EQ(esl_generic_bmp_pixel(8, &ctx), 1); /* (0, 1) clear */
    free(f);
}

/* A stacked 2-plane generic source feeds the accent plane. */
static void test_generic_accent_plane(void) {
    size_t len = 0;
    uint8_t *f = make_bmp(8, 8, 2, 1, 2, &len);
    EslBmpInfo info;
    CHECK(esl_bmp_parse(f, len, &info));

    set_bit(f, &info, 1, 0, 0); /* accent plane, source (0,0) */

    EslGenericBmpCtx ctx = {f, len, &info, 8, 8, true};
    CHECK_EQ(esl_generic_bmp_pixel(0, &ctx), 1);  /* primary untouched */
    CHECK_EQ(esl_generic_bmp_pixel(64, &ctx), 0); /* accent set */
    CHECK_EQ(esl_generic_bmp_pixel(65, &ctx), 1);
    free(f);
}

static void test_map(void) {
    /* Identity when sizes match. */
    CHECK_EQ(esl_bmp_map_x(0, 100, 100), 0);
    CHECK_EQ(esl_bmp_map_x(99, 100, 100), 99);
    /* Downscale by 2. */
    CHECK_EQ(esl_bmp_map_x(0, 50, 100), 0);
    CHECK_EQ(esl_bmp_map_x(49, 50, 100), 98);
    /* Upscale by 2 repeats source columns. */
    CHECK_EQ(esl_bmp_map_x(0, 100, 50), 0);
    CHECK_EQ(esl_bmp_map_x(1, 100, 50), 0);
    CHECK_EQ(esl_bmp_map_x(2, 100, 50), 1);
    /* Clamped, never out of range. */
    CHECK_EQ(esl_bmp_map_x(200, 100, 50), 49);
    CHECK_EQ(esl_bmp_map_y(200, 100, 50), 49);
    /* Degenerate inputs return 0 rather than dividing by zero. */
    CHECK_EQ(esl_bmp_map_x(5, 0, 50), 0);
    CHECK_EQ(esl_bmp_map_y(5, 50, 0), 0);
}

/* A glass-oriented (296x152) top-down 1bpp BMP with a single white pixel at
 * glass (0,151). The wire index that maps there is wire(0,0) -> glass(0,151),
 * which is wire index 0. */
static void test_color26_glass_orientation(void) {
    size_t len = 0;
    uint8_t *f = make_bmp(296, 152, 1, 1, 1, &len);
    EslBmpInfo info;
    CHECK(esl_bmp_parse(f, len, &info));

    set_bit(f, &info, 0, 151, 0); /* top_down: row index == glass y */

    EslColor26BmpCtx ctx = {f, len, &info};

    /* BMP bit 1 (white) -> ESL 0. */
    CHECK_EQ(esl_color26_bmp_pixel(0, &ctx), 0);
    /* Neighbouring wire pixels are untouched -> ESL 1. */
    CHECK_EQ(esl_color26_bmp_pixel(1, &ctx), 1);
    CHECK_EQ(esl_color26_bmp_pixel(TAGTINKER_COLOR26_WIRE_W, &ctx), 1);

    /* A 1bpp source has no accent plane, so plane 2 reads all clear. */
    const size_t plane = (size_t)TAGTINKER_COLOR26_WIRE_W *
                         TAGTINKER_COLOR26_WIRE_H;
    CHECK_EQ(esl_color26_bmp_pixel(plane, &ctx), 1);
    free(f);
}

/* A wire-oriented (152x296) BMP is consumed without any transpose. */
static void test_color26_wire_orientation(void) {
    size_t len = 0;
    uint8_t *f = make_bmp(152, 296, 1, 1, 1, &len);
    EslBmpInfo info;
    CHECK(esl_bmp_parse(f, len, &info));

    set_bit(f, &info, 0, 0, 0); /* wire (0,0) */

    EslColor26BmpCtx ctx = {f, len, &info};
    CHECK_EQ(esl_color26_bmp_pixel(0, &ctx), 0);
    CHECK_EQ(esl_color26_bmp_pixel(1, &ctx), 1);
    free(f);
}

/* A stacked 2-plane BMP feeds the accent plane from the second stack. */
static void test_color26_accent_plane(void) {
    size_t len = 0;
    uint8_t *f = make_bmp(152, 296, 2, 1, 2, &len);
    EslBmpInfo info;
    CHECK(esl_bmp_parse(f, len, &info));

    set_bit(f, &info, 1, 0, 0); /* accent plane, wire (0,0) */

    EslColor26BmpCtx ctx = {f, len, &info};
    const size_t plane = (size_t)TAGTINKER_COLOR26_WIRE_W *
                         TAGTINKER_COLOR26_WIRE_H;

    CHECK_EQ(esl_color26_bmp_pixel(0, &ctx), 1);         /* primary untouched */
    CHECK_EQ(esl_color26_bmp_pixel(plane, &ctx), 0);     /* accent set */
    CHECK_EQ(esl_color26_bmp_pixel(plane + 1, &ctx), 1);
    free(f);
}

/* Untrusted file_len must clamp pixel reads; data_offset past EOF is rejected. */
static void test_color26_bounds(void) {
    size_t len = 0;
    uint8_t *f = make_bmp(152, 296, 1, 1, 1, &len);
    EslBmpInfo info;
    CHECK(esl_bmp_parse(f, len, &info));

    set_bit(f, &info, 0, 0, 0); /* wire (0,0) is white → ESL 0 */

    EslColor26BmpCtx full = {f, len, &info};
    CHECK_EQ(esl_color26_bmp_pixel(0, &full), 0);

    /* file_len stops at the header: the payload offset is not in-range. */
    EslColor26BmpCtx trunc = {f, 54, &info};
    CHECK_EQ(esl_color26_bmp_pixel(0, &trunc), 1);
    free(f);

    f = make_bmp(8, 8, 1, 1, 1, &len);
    const uint32_t past = (uint32_t)len + 1u;
    f[10] = (uint8_t)(past);
    f[11] = (uint8_t)(past >> 8);
    f[12] = (uint8_t)(past >> 16);
    f[13] = (uint8_t)(past >> 24);
    CHECK(!esl_bmp_parse(f, len, &info));
    free(f);
}

/* Neither wire (152x296) nor glass (296x152): transpose then nearest-neighbour. */
static void test_color26_other_size_rescale(void) {
    size_t len = 0;
    const uint16_t src_w = 148;
    const uint16_t src_h = 76; /* exactly half the glass dimensions */
    uint8_t *f = make_bmp(src_w, src_h, 1, 1, 1, &len);
    EslBmpInfo info;
    CHECK(esl_bmp_parse(f, len, &info));

    const uint16_t sx = 10;
    const uint16_t sy = 20;
    set_bit(f, &info, 0, sy, sx);

    /* wire (px,py) → glass (bx,by) = (py, 151-px); glass → source via map. */
    const uint16_t set_px = 111;
    const uint16_t set_py = 20;
    uint16_t gx = 0, gy = 0;
    tagtinker_color26_proto_to_glass(TAGTINKER_COLOR26_WIRE_W, set_px, set_py,
                                     &gx, &gy);
    CHECK_EQ(esl_bmp_map_x(gx, TAGTINKER_COLOR26_GLASS_W, src_w), sx);
    CHECK_EQ(esl_bmp_map_y(gy, TAGTINKER_COLOR26_GLASS_H, src_h), sy);

    EslColor26BmpCtx ctx = {f, len, &info};
    const size_t set_idx =
        (size_t)set_py * TAGTINKER_COLOR26_WIRE_W + set_px;
    CHECK_EQ(esl_color26_bmp_pixel(set_idx, &ctx), 0);

    const uint16_t clear_px = 112;
    const uint16_t clear_py = 20;
    tagtinker_color26_proto_to_glass(TAGTINKER_COLOR26_WIRE_W, clear_px,
                                     clear_py, &gx, &gy);
    CHECK(esl_bmp_map_x(gx, TAGTINKER_COLOR26_GLASS_W, src_w) != sx ||
          esl_bmp_map_y(gy, TAGTINKER_COLOR26_GLASS_H, src_h) != sy);
    const size_t clear_idx =
        (size_t)clear_py * TAGTINKER_COLOR26_WIRE_W + clear_px;
    CHECK_EQ(esl_color26_bmp_pixel(clear_idx, &ctx), 1);
    free(f);
}

int main(void) {
    test_parse();
    test_map();
    test_color26_glass_orientation();
    test_color26_wire_orientation();
    test_color26_accent_plane();
    test_color26_bounds();
    test_color26_other_size_rescale();
    test_generic_pixel();
    test_generic_accent_plane();
    test_generic_bottom_up_row_flip();
    test_generic_no_transpose();
    TEST_REPORT("test_bmp");
}
