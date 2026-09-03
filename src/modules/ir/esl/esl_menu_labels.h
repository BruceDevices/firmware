#pragma once
#define ESL_UI_APP_NAME "Tag Tinker"
static const char *const ESL_MAIN_ITEMS[] = {
    "Broadcast Payloads", "Targeted Payloads", "Settings", "About"};
static const char *const ESL_BROADCAST_ITEMS[] = {
    "Change Page", "Diagnostic Page"};
static const char *const ESL_TARGET_MENU_PREFIX[] = {
    "+ Scan NFC", "+ Type Barcode"};
static const char *const ESL_TARGET_ACTIONS_ALWAYS[] = {
    "Show Tag Info", "Rename Tag"};
static const char *const ESL_TARGET_ACTIONS_GRAPHICS[] = {
    "Set Text", "Set Image", "WiFi Plugins"};
static const char *const ESL_TARGET_ACTIONS_TAIL[] = {
    "LED Test", "Delete Tag"};
static const char *const ESL_SETTINGS_ITEMS[] = {
    "Startup Warning", "Frame Repeat", "Clear Recents"};
static const char *const ESL_ABOUT_LINES[] = {
    "Tag Tinker", "Ported by I12BP8", "Research by furrtek", "NFC by 7h30th3r0n3"};
static const char *const ESL_WARNING_TITLES[] = {
    "RESEARCH TOOL:", "PERMISSION:", "CAUTION:", "RESPONSIBILITY:"};
static const char *const ESL_SET_IMAGE_EMPTY[] = {
    "No matching BMPs", "Drop into apps_data/", "  tagtinker/dropped/",
    "Use Image Prep page"};
static const char *const ESL_RECENT_NEW = "[+] New Text";
static const char *const ESL_SEND_BMP = ">> Send BMP <<";
static const char *const ESL_TRANSMIT = ">> Transmit <<";
