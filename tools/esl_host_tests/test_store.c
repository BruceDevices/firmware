#include "esl_store.h"
#include "test_util.h"

static void test_settings_defaults_and_roundtrip(void) {
    EslSettings s;
    esl_store_settings_defaults(&s);
    CHECK(s.show_startup_warning);
    CHECK_EQ(s.data_frame_repeats, 2u);

    CHECK(esl_store_settings_parse("0|7|0", &s));
    CHECK(!s.show_startup_warning);
    CHECK_EQ(s.data_frame_repeats, 7u);

    CHECK(esl_store_settings_parse("1|2", &s)); /* older 2-field */
    CHECK(s.show_startup_warning);
    CHECK_EQ(s.data_frame_repeats, 2u);

    CHECK(esl_store_settings_parse("1|0|0", &s));
    CHECK_EQ(s.data_frame_repeats, 1u); /* clamp low */
    CHECK(esl_store_settings_parse("1|99|0", &s));
    CHECK_EQ(s.data_frame_repeats, 10u); /* clamp high */

    char buf[32];
    s.show_startup_warning = true;
    s.data_frame_repeats = 4;
    CHECK(esl_store_settings_format(&s, buf, sizeof(buf)));
    CHECK_STR(buf, "1|4|0");
}

static void test_targets_parse_save_crud(void) {
    EslSession sess;
    esl_store_session_init(&sess);
    /* Flipper's loader does not dedup: two valid lines both load. The
     * invalid middle line is skipped. Empty name → default tagN. */
    const char *src =
        "A4165420155216265|cat\n"
        "not-a-barcode|x\n"
        "A4165420155216265|\n";
    CHECK(esl_store_targets_parse(src, &sess));
    CHECK_EQ(sess.target_count, 2u);
    CHECK_STR(sess.targets[0].barcode, "A4165420155216265");
    CHECK_STR(sess.targets[0].name, "cat");
    CHECK_EQ(sess.targets[0].plid[0], 0x10u);
    CHECK_EQ(sess.targets[0].plid[1], 0x06u);
    CHECK_EQ(sess.targets[0].plid[2], 0x9Eu);
    CHECK_EQ(sess.targets[0].plid[3], 0x40u);
    CHECK_EQ(sess.targets[0].profile.type_code, 1626u);
    CHECK_STR(sess.targets[1].name, "tag2");

    CHECK_EQ(esl_store_find_target(&sess, "A4165420155216265"), 0);
    CHECK_EQ(esl_store_find_target(&sess, "missing"), -1);

    EslSession empty;
    esl_store_session_init(&empty);
    CHECK_EQ(esl_store_ensure_target(&empty, "A4165420155216265"), 0);
    CHECK_STR(empty.targets[0].name, "tag1");
    CHECK_EQ(esl_store_ensure_target(&empty, "A4165420155216265"), 0); /* dedup */
    CHECK_EQ(empty.target_count, 1u);
    CHECK_EQ(esl_store_ensure_target(&empty, "bad"), -1);

    CHECK(esl_store_delete_target(&empty, 0));
    CHECK_EQ(empty.target_count, 0u);

    char out[256];
    CHECK(esl_store_targets_format(&sess, out, sizeof(out)));
    CHECK_STR(out, "A4165420155216265|cat\nA4165420155216265|tag2\n");
}

static void test_target_cap(void) {
    EslSession sess;
    esl_store_session_init(&sess);
    sess.target_count = ESL_STORE_MAX_TARGETS;
    CHECK_EQ(esl_store_ensure_target(&sess, "A4165420155216265"), -1);
}

static void test_recents_parse_add(void) {
    EslSession sess;
    esl_store_session_init(&sess);
    CHECK(esl_store_recents_parse("296|152|2|0|0|10|0|hello\n", &sess));
    CHECK_EQ(sess.recent_count, 1u);
    CHECK_EQ(sess.recents[0].width, 296u);
    CHECK_EQ(sess.recents[0].height, 152u);
    CHECK_EQ(sess.recents[0].page, 2u);
    CHECK(!sess.recents[0].invert);
    CHECK_EQ(sess.recents[0].padding, 10u);
    CHECK_STR(sess.recents[0].text, "hello");

    /* older 6-field (no sig). Own session so this does not wipe the
     * 8-field hello row that recents_add uses for same-text+size. */
    EslSession older;
    esl_store_session_init(&older);
    CHECK(esl_store_recents_parse("152|152|1|1|0|5|hi\n", &older));
    CHECK_EQ(older.recent_count, 1u);
    CHECK(older.recents[0].invert);
    CHECK_STR(older.recents[0].text, "hi");

    sess.esl_width = 296;
    sess.esl_height = 152;
    sess.img_page = 3;
    sess.invert_text = false;
    sess.color_clear = false;
    sess.text_padding_pct = 0;
    esl_store_recents_add(&sess, "hello"); /* same text+size → move to front */
    CHECK_EQ(sess.recent_count, 1u);
    esl_store_recents_add(&sess, "world");
    CHECK_EQ(sess.recent_count, 2u);
    CHECK_STR(sess.recents[0].text, "world");
    CHECK_STR(sess.recents[1].text, "hello");
    esl_store_recents_add(&sess, "hello"); /* non-front match → index 0 */
    CHECK_EQ(sess.recent_count, 2u);
    CHECK_STR(sess.recents[0].text, "hello");
    CHECK_STR(sess.recents[1].text, "world");

    char out[256];
    CHECK(esl_store_recents_format(&sess, out, sizeof(out)));
    CHECK(strstr(out, "world") != NULL);
}

int main(void) {
    test_settings_defaults_and_roundtrip();
    test_targets_parse_save_crud();
    test_target_cap();
    test_recents_parse_add();
    TEST_REPORT("test_store");
}
