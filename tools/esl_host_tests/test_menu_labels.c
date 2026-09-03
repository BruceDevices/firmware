#include "esl_menu_labels.h"
#include "test_util.h"

#define ESL_N(a) (sizeof(a) / sizeof((a)[0]))

static void assert_not_legacy_name(const char *s) {
    CHECK(s != NULL);
    CHECK(strcmp(s, "ESL Image") != 0);
    CHECK(strcmp(s, "TagTinker") != 0);
    CHECK(strcmp(s, "TagTinker v2.1") != 0);
}

static void assert_table(const char *const *items, size_t n,
                         const char *const *want, size_t want_n) {
    CHECK_EQ(n, want_n);
    for (size_t i = 0; i < want_n; i++) {
        CHECK_STR(items[i], want[i]);
        assert_not_legacy_name(items[i]);
    }
}

static void test_app_name_and_singletons(void) {
    CHECK_STR(ESL_UI_APP_NAME, "Tag Tinker");
    assert_not_legacy_name(ESL_UI_APP_NAME);

    CHECK_STR(ESL_RECENT_NEW, "[+] New Text");
    CHECK_STR(ESL_SEND_BMP, ">> Send BMP <<");
    CHECK_STR(ESL_TRANSMIT, ">> Transmit <<");
    assert_not_legacy_name(ESL_RECENT_NEW);
    assert_not_legacy_name(ESL_SEND_BMP);
    assert_not_legacy_name(ESL_TRANSMIT);
}

static void test_menu_tables_count_and_order(void) {
    static const char *const want_main[] = {
        "Broadcast Payloads", "Targeted Payloads", "Settings", "About"};
    static const char *const want_broadcast[] = {
        "Change Page", "Diagnostic Page"};
    static const char *const want_target_prefix[] = {
        "+ Scan NFC", "+ Type Barcode"};
    static const char *const want_actions_always[] = {
        "Show Tag Info", "Rename Tag"};
    static const char *const want_actions_graphics[] = {
        "Set Text", "Set Image", "WiFi Plugins"};
    static const char *const want_actions_tail[] = {"LED Test", "Delete Tag"};
    static const char *const want_settings[] = {
        "Startup Warning", "Frame Repeat", "Clear Recents"};
    static const char *const want_about[] = {
        "Tag Tinker", "Ported by I12BP8", "Research by furrtek",
        "NFC by 7h30th3r0n3"};
    static const char *const want_warning[] = {
        "RESEARCH TOOL:", "PERMISSION:", "CAUTION:", "RESPONSIBILITY:"};
    static const char *const want_empty[] = {
        "No matching BMPs", "Drop into apps_data/", "  tagtinker/dropped/",
        "Use Image Prep page"};

    assert_table(ESL_MAIN_ITEMS, ESL_N(ESL_MAIN_ITEMS), want_main,
                 ESL_N(want_main));
    assert_table(ESL_BROADCAST_ITEMS, ESL_N(ESL_BROADCAST_ITEMS),
                 want_broadcast, ESL_N(want_broadcast));
    assert_table(ESL_TARGET_MENU_PREFIX, ESL_N(ESL_TARGET_MENU_PREFIX),
                 want_target_prefix, ESL_N(want_target_prefix));
    assert_table(ESL_TARGET_ACTIONS_ALWAYS, ESL_N(ESL_TARGET_ACTIONS_ALWAYS),
                 want_actions_always, ESL_N(want_actions_always));
    assert_table(ESL_TARGET_ACTIONS_GRAPHICS, ESL_N(ESL_TARGET_ACTIONS_GRAPHICS),
                 want_actions_graphics, ESL_N(want_actions_graphics));
    assert_table(ESL_TARGET_ACTIONS_TAIL, ESL_N(ESL_TARGET_ACTIONS_TAIL),
                 want_actions_tail, ESL_N(want_actions_tail));
    assert_table(ESL_SETTINGS_ITEMS, ESL_N(ESL_SETTINGS_ITEMS), want_settings,
                 ESL_N(want_settings));
    assert_table(ESL_ABOUT_LINES, ESL_N(ESL_ABOUT_LINES), want_about,
                 ESL_N(want_about));
    assert_table(ESL_WARNING_TITLES, ESL_N(ESL_WARNING_TITLES), want_warning,
                 ESL_N(want_warning));
    assert_table(ESL_SET_IMAGE_EMPTY, ESL_N(ESL_SET_IMAGE_EMPTY), want_empty,
                 ESL_N(want_empty));
}

int main(void) {
    test_app_name_and_singletons();
    test_menu_tables_count_and_order();
    TEST_REPORT("test_menu_labels");
}
