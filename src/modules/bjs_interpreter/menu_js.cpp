#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "menu_js.h"
#include "core/display.h"

JSValue native_menuShow(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: menu.show(title_string, options_array_of_strings)
    // returns: selected index (0-based), or -1 if user pressed back
    //
    // Example:
    //   let choice = menu.show("Pick One", ["Scan RF", "Replay", "Settings"]);
    //   if (choice == 0) { /* Scan RF */ }

    if (argc < 2) return JS_NewInt32(ctx, -1);

    // Get title
    const char *title = "Menu";
    JSCStringBuf title_buf;
    if (JS_IsString(ctx, argv[0])) {
        title = JS_ToCString(ctx, argv[0], &title_buf);
    }

    // Get options array (arrays are objects with a length property)
    if (!JS_IsObject(ctx, argv[1])) return JS_NewInt32(ctx, -1);

    JSValue lengthVal = JS_GetPropertyStr(ctx, argv[1], "length");
    int length = 0;
    JS_ToInt32(ctx, &length, lengthVal);

    if (length <= 0 || length > 100) return JS_NewInt32(ctx, -1);

    // Build Bruce options vector from JS array
    options.clear();
    std::vector<String> labels; // Keep strings alive during menu display
    labels.reserve(length);

    for (int i = 0; i < length; i++) {
        JSValue item = JS_GetPropertyUint32(ctx, argv[1], i);
        if (JS_IsString(ctx, item)) {
            JSCStringBuf item_buf;
            const char *str = JS_ToCString(ctx, item, &item_buf);
            labels.push_back(String(str));
        } else {
            labels.push_back("Option " + String(i));
        }
        // The lambda is a no-op; we just need the index from loopOptions
        options.push_back({labels.back().c_str(), []() {}});
    }

    // Show the native Bruce menu and get the selected index
    int selected = loopOptions(options, MENU_TYPE_SUBMENU, title);

    options.clear();

    // loopOptions returns the last index; if returnToMenu is true, user pressed back
    if (returnToMenu) {
        returnToMenu = false;
        return JS_NewInt32(ctx, -1);
    }

    return JS_NewInt32(ctx, selected);
}

JSValue native_menuShowMainBorder(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    bool clear = true;
    if (argc > 0 && JS_IsBool(argv[0])) {
        clear = JS_ToBool(ctx, argv[0]);
    }
    drawMainBorder(clear);
    return JS_UNDEFINED;
}

JSValue native_menuShowMainBorderWithTitle(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    const char *title = "";
    JSCStringBuf title_buf;
    if (argc > 0 && JS_IsString(ctx, argv[0])) {
        title = JS_ToCString(ctx, argv[0], &title_buf);
    }
    drawMainBorderWithTitle(String(title));
    return JS_UNDEFINED;
}

JSValue native_menuPrintTitle(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    const char *title = "";
    JSCStringBuf title_buf;
    if (argc > 0 && JS_IsString(ctx, argv[0])) {
        title = JS_ToCString(ctx, argv[0], &title_buf);
    }
    printTitle(String(title));
    return JS_UNDEFINED;
}

JSValue native_menuPrintSubtitle(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    const char *text = "";
    JSCStringBuf text_buf;
    if (argc > 0 && JS_IsString(ctx, argv[0])) {
        text = JS_ToCString(ctx, argv[0], &text_buf);
    }
    printSubtitle(String(text));
    return JS_UNDEFINED;
}

JSValue native_menuDisplayMessage(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    const char *msg = "";
    JSCStringBuf msg_buf;
    if (argc > 0 && JS_IsString(ctx, argv[0])) {
        msg = JS_ToCString(ctx, argv[0], &msg_buf);
    }
    displayTextLine(String(msg));
    return JS_UNDEFINED;
}

#endif
