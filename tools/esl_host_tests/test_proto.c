#include "protocol/tagtinker_proto.h"
#include "test_util.h"

static const char *BARCODE = "A4165420155216265";

static uint8_t zero_pixel(size_t idx, void *ctx) {
    (void)idx; (void)ctx;
    return 0U;
}

static uint8_t alt_pixel(size_t idx, void *ctx) {
    (void)ctx;
    return (uint8_t)(idx & 1U);
}

static void test_barcode(void) {
    uint8_t plid[4] = {0};
    const uint8_t want[4] = {0x10, 0x06, 0x9E, 0x40};
    CHECK(tagtinker_barcode_to_plid(BARCODE, plid));
    CHECK_MEM(plid, want, 4);

    uint16_t type = 0;
    CHECK(tagtinker_barcode_to_type(BARCODE, &type));
    CHECK_EQ(type, 1626);

    /* Wrong length must be rejected. */
    CHECK(!tagtinker_barcode_to_plid("A416542015521626", plid));
    CHECK(!tagtinker_is_barcode_valid("too-short"));
    CHECK(tagtinker_is_barcode_valid(BARCODE));
}

static void test_profile(void) {
    TagTinkerTagProfile p;
    CHECK(tagtinker_barcode_to_profile(BARCODE, &p));
    CHECK_STR(p.model_name, "SmartTAG Color 2.6");
    CHECK_EQ(p.type_code, 1626);
    CHECK_EQ(p.width, 152);   /* wire dims live in the profile */
    CHECK_EQ(p.height, 296);
    CHECK_EQ(p.kind, TagTinkerTagKindDotMatrix);
    CHECK_EQ(p.color, TagTinkerTagColorRed);
    CHECK(p.known);

    uint16_t gw = 0, gh = 0;
    tagtinker_profile_glass_size(&p, &gw, &gh);
    CHECK_EQ(gw, 296);
    CHECK_EQ(gh, 152);
    CHECK(tagtinker_profile_needs_wh_swap(&p));
    CHECK(tagtinker_profile_uses_ui_page(&p));

    /* An unknown type code must fail rather than silently guess. */
    TagTinkerTagProfile unknown;
    CHECK(!tagtinker_barcode_to_profile("A4165420155299995", &unknown));
}

static void test_page_and_transpose(void) {
    CHECK_EQ(tagtinker_color26_resolve_page(0), 2);
    CHECK_EQ(tagtinker_color26_resolve_page(1), 2);
    CHECK_EQ(tagtinker_color26_resolve_page(2), 2);
    CHECK_EQ(tagtinker_color26_resolve_page(5), 5);
    CHECK_EQ(tagtinker_color26_resolve_page(9), 7);

    uint16_t bx = 0, by = 0;
    tagtinker_color26_proto_to_glass(TAGTINKER_COLOR26_WIRE_W, 0, 0, &bx, &by);
    CHECK_EQ(bx, 0);   CHECK_EQ(by, 151);
    tagtinker_color26_proto_to_glass(TAGTINKER_COLOR26_WIRE_W, 0, 1, &bx, &by);
    CHECK_EQ(bx, 1);   CHECK_EQ(by, 151);
    tagtinker_color26_proto_to_glass(TAGTINKER_COLOR26_WIRE_W, 1, 0, &bx, &by);
    CHECK_EQ(bx, 0);   CHECK_EQ(by, 150);
    tagtinker_color26_proto_to_glass(TAGTINKER_COLOR26_WIRE_W, 151, 0, &bx, &by);
    CHECK_EQ(bx, 0);   CHECK_EQ(by, 0);
    tagtinker_color26_proto_to_glass(TAGTINKER_COLOR26_WIRE_W, 151, 295, &bx, &by);
    CHECK_EQ(bx, 295); CHECK_EQ(by, 0);
}

static void test_crc(void) {
    const uint8_t vec[] = {0x85, 0x10, 0x06, 0x9E, 0x40, 0x17};
    CHECK_EQ(tagtinker_crc16(vec, sizeof(vec)), 0xD3D8);
}

static void test_frames(void) {
    uint8_t plid[4];
    tagtinker_barcode_to_plid(BARCODE, plid);
    uint8_t buf[TAGTINKER_MAX_FRAME_SIZE];

    const uint8_t want_wake[34] = {
        0x85, 0x10, 0x06, 0x9E, 0x40, 0x17, 0x01, 0x00, 0x00, 0x00,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x85, 0x7A};
    CHECK_EQ(tagtinker_make_wake_frame(buf, plid), 34);
    CHECK_MEM(buf, want_wake, 34);

    const uint8_t want_ping[32] = {
        0x85, 0x10, 0x06, 0x9E, 0x40, 0x97, 0x01, 0x00, 0x00, 0x00,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x6B, 0x81};
    CHECK_EQ(tagtinker_make_ping_frame(buf, plid), 32);
    CHECK_MEM(buf, want_ping, 32);

    const uint8_t want_refresh[30] = {
        0x85, 0x10, 0x06, 0x9E, 0x40, 0x34, 0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB9, 0xEB};
    CHECK_EQ(tagtinker_make_refresh_frame(buf, plid), 30);
    CHECK_MEM(buf, want_refresh, 30);

    /* byte_count=20, comp_type=2 (RLE), page=2, 152x296, pos 0,0 */
    const uint8_t want_param[34] = {
        0x85, 0x10, 0x06, 0x9E, 0x40, 0x34, 0x00, 0x00, 0x00, 0x05,
        0x00, 0x14, 0x00, 0x02, 0x02, 0x00, 0x98, 0x01, 0x28, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xBF, 0x64};
    CHECK_EQ(tagtinker_make_image_param_frame(
                 buf, plid, 20, 2, 2, TAGTINKER_COLOR26_WIRE_W,
                 TAGTINKER_COLOR26_WIRE_H, 0, 0),
             34);
    CHECK_MEM(buf, want_param, 34);

    uint8_t data20[TAGTINKER_IMAGE_DATA_BYTES_PER_FRAME];
    for (int i = 0; i < 20; i++) data20[i] = (uint8_t)i;

    const uint8_t want_d0[34] = {
        0x85, 0x10, 0x06, 0x9E, 0x40, 0x34, 0x00, 0x00, 0x00, 0x20,
        0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11,
        0x12, 0x13, 0x95, 0x20};
    CHECK_EQ(tagtinker_make_image_data_frame(buf, plid, 0, data20), 34);
    CHECK_MEM(buf, want_d0, 34);

    const uint8_t want_d7[34] = {
        0x85, 0x10, 0x06, 0x9E, 0x40, 0x34, 0x00, 0x00, 0x00, 0x20,
        0x00, 0x07, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11,
        0x12, 0x13, 0x91, 0xA4};
    CHECK_EQ(tagtinker_make_image_data_frame(buf, plid, 7, data20), 34);
    CHECK_MEM(buf, want_d7, 34);

    /* Every frame must fit the protocol's own buffer bound. */
    CHECK(34 <= TAGTINKER_MAX_FRAME_SIZE);
}

static void test_encode(void) {
    TagTinkerImagePayload p;

    /* One long run of zeros: RLE wins, so auto picks comp_type 2.
     * Bits: [0] + 7 zeros + 8 count bits of 160 (10100000) = 00 A0. */
    const uint8_t want_zeros[20] = {0x00, 0xA0};
    CHECK(tagtinker_encode_fn_payload(zero_pixel, NULL, 160,
                                      TagTinkerCompressionAuto, &p));
    CHECK_EQ(p.comp_type, 2);
    CHECK_EQ(p.byte_count, 20);
    CHECK_MEM(p.data, want_zeros, 20);
    tagtinker_free_image_payload(&p);
    CHECK(p.data == NULL);

    /* Alternating pixels: RLE would be worse, so auto must fall back to raw. */
    uint8_t want_alt[20];
    memset(want_alt, 0x55, sizeof(want_alt));
    CHECK(tagtinker_encode_fn_payload(alt_pixel, NULL, 160,
                                      TagTinkerCompressionAuto, &p));
    CHECK_EQ(p.comp_type, 0);
    CHECK_EQ(p.byte_count, 20);
    CHECK_MEM(p.data, want_alt, 20);
    tagtinker_free_image_payload(&p);

    /* Forcing RLE on 8 alternating pixels: [0] then eight 1-bit runs. */
    const uint8_t want_forced[20] = {0x7F, 0x80};
    CHECK(tagtinker_encode_fn_payload(alt_pixel, NULL, 8,
                                      TagTinkerCompressionRle, &p));
    CHECK_EQ(p.comp_type, 2);
    CHECK_EQ(p.byte_count, 20);
    CHECK_MEM(p.data, want_forced, 20);
    tagtinker_free_image_payload(&p);

    /* Payloads are always a whole number of 20-byte data frames. */
    size_t total = (size_t)TAGTINKER_COLOR26_WIRE_W *
                   TAGTINKER_COLOR26_WIRE_H * 2U;
    CHECK_EQ(total, 89984);
    CHECK(tagtinker_encode_fn_payload(zero_pixel, NULL, total,
                                      TagTinkerCompressionAuto, &p));
    CHECK_EQ(p.byte_count % TAGTINKER_IMAGE_DATA_BYTES_PER_FRAME, 0);
    tagtinker_free_image_payload(&p);

    /* Degenerate inputs must be rejected, not crash. */
    CHECK(!tagtinker_encode_fn_payload(NULL, NULL, 16,
                                       TagTinkerCompressionAuto, &p));
    CHECK(!tagtinker_encode_fn_payload(zero_pixel, NULL, 0,
                                       TagTinkerCompressionAuto, &p));
}

int main(void) {
    test_barcode();
    test_profile();
    test_page_and_transpose();
    test_crc();
    test_frames();
    test_encode();
    TEST_REPORT("test_proto");
}
