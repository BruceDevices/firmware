#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "globals_js.h"
#include "user_classes_js.h"

#include "mbedtls/base64.h"

JSValue js_gc(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    JS_GC(ctx);
    return JS_UNDEFINED;
}

JSValue js_load(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    JSCStringBuf buf_str;
    const char *filename = JS_ToCString(ctx, argv[0], &buf_str);
    if (!filename) return JS_EXCEPTION;

    FileParamsJS fileParams = js_get_path_from_params(ctx, argv);

    size_t script_len = 0;
    char *script = readBigFile(fileParams.fs, fileParams.path, false, &script_len);

    JSValue ret = JS_Eval(ctx, (const char *)script, script_len, filename, 0);
    free(script);
    return ret;
}

/* timers */
typedef struct {
    bool allocated;
    JSGCRef func;
    int64_t timeout;     /* next due time in ms */
    int32_t interval_ms; /* period for intervals */
    bool repeat;
    bool main;
} JSTimer;

#define MAX_TIMERS 16

typedef struct {
    JSTimer timers[MAX_TIMERS];
} JSTimerContextState;

static const char *kTimersStateProp = "__bruce_timers_state";

static JSTimerContextState *get_timer_state(JSContext *ctx, bool create) {
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue holder = JS_GetPropertyStr(ctx, global, kTimersStateProp);

    if (JS_IsObject(ctx, holder)) {
        void *opaque = JS_GetOpaque(ctx, holder);
        if (opaque) return (JSTimerContextState *)opaque;
    }

    if (!create) return NULL;

    JSTimerContextState *state = (JSTimerContextState *)calloc(1, sizeof(JSTimerContextState));
    if (!state) return NULL;

    if (!JS_IsObject(ctx, holder)) { holder = JS_NewObjectClassUser(ctx, JS_CLASS_TIMERS_STATE); }
    JS_SetOpaque(ctx, holder, state);
    JS_SetPropertyStr(ctx, global, kTimersStateProp, holder);
    return state;
}

void native_timers_state_finalizer(JSContext *ctx, void *opaque) {
    JSTimerContextState *state = (JSTimerContextState *)opaque;
    if (!state) return;
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (state->timers[i].allocated) {
            JS_DeleteGCRef(ctx, &state->timers[i].func);
            state->timers[i].allocated = false;
        }
    }
    free(state);
}

void js_timers_init(JSContext *ctx) { (void)get_timer_state(ctx, true); }

void js_timers_deinit(JSContext *ctx) {
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue holder = JS_GetPropertyStr(ctx, global, kTimersStateProp);
    if (!JS_IsObject(ctx, holder)) return;

    JSTimerContextState *state = (JSTimerContextState *)JS_GetOpaque(ctx, holder);
    if (!state) return;

    for (int i = 0; i < MAX_TIMERS; i++) {
        if (state->timers[i].allocated) {
            JS_DeleteGCRef(ctx, &state->timers[i].func);
            state->timers[i].allocated = false;
        }
    }

    JS_SetOpaque(ctx, holder, NULL);
    free(state);

    JS_SetPropertyStr(ctx, global, kTimersStateProp, JS_UNDEFINED);
}

int js_add_main_timer(JSContext *ctx, JSValue func) {
    JSTimer *th;
    int i;
    JSValue *pfunc;

    if (!JS_IsFunction(ctx, func)) return -1;

    JSTimerContextState *state = get_timer_state(ctx, true);
    if (!state) return -1;

    for (i = 0; i < MAX_TIMERS; i++) {
        th = &state->timers[i];
        if (!th->allocated) {
            pfunc = JS_AddGCRef(ctx, &th->func);
            *pfunc = func;
            th->timeout = millis();
            th->interval_ms = 0;
            th->repeat = false;
            th->main = true;
            th->allocated = true;
            return i;
        }
    }

    return -1;
}

JSValue js_setTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    JSTimer *th;
    int delay, i;
    JSValue *pfunc;

    if (!JS_IsFunction(ctx, argv[0])) return JS_ThrowTypeError(ctx, "not a function");
    if (JS_ToInt32(ctx, &delay, argv[1])) return JS_EXCEPTION;

    JSTimerContextState *state = get_timer_state(ctx, true);
    if (!state) return JS_ThrowInternalError(ctx, "out of memory");

    for (i = 0; i < MAX_TIMERS; i++) {
        th = &state->timers[i];
        if (!th->allocated) {
            pfunc = JS_AddGCRef(ctx, &th->func);
            *pfunc = argv[0];
            th->timeout = millis() + delay;
            th->interval_ms = 0;
            th->repeat = false;
            th->main = false;
            th->allocated = true;
            return JS_NewInt32(ctx, i);
        }
    }
    return JS_ThrowInternalError(ctx, "too many timers");
}

JSValue js_setInterval(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    JSTimer *th;
    int delay, i;
    JSValue *pfunc;

    if (!JS_IsFunction(ctx, argv[0])) return JS_ThrowTypeError(ctx, "not a function");
    if (JS_ToInt32(ctx, &delay, argv[1])) return JS_EXCEPTION;

    JSTimerContextState *state = get_timer_state(ctx, true);
    if (!state) return JS_ThrowInternalError(ctx, "out of memory");

    for (i = 0; i < MAX_TIMERS; i++) {
        th = &state->timers[i];
        if (!th->allocated) {
            pfunc = JS_AddGCRef(ctx, &th->func);
            *pfunc = argv[0];
            th->timeout = millis() + delay;
            th->interval_ms = delay;
            th->repeat = true;
            th->main = false;
            th->allocated = true;
            return JS_NewInt32(ctx, i);
        }
    }
    return JS_ThrowInternalError(ctx, "too many timers");
}

JSValue js_clearTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int timer_id;
    JSTimer *th;

    if (JS_ToInt32(ctx, &timer_id, argv[0])) return JS_EXCEPTION;
    if (timer_id >= 0 && timer_id < MAX_TIMERS) {
        JSTimerContextState *state = get_timer_state(ctx, false);
        if (!state) return JS_UNDEFINED;

        th = &state->timers[timer_id];
        if (th->allocated) {
            JS_DeleteGCRef(ctx, &th->func);
            th->allocated = false;
        }
    }
    return JS_UNDEFINED;
}

JSValue js_clearInterval(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return js_clearTimeout(ctx, this_val, argc, argv);
}

void run_timers(JSContext *ctx) {
    int64_t min_delay;
    int64_t delayMs;
    int64_t cur_time;
    bool has_timer;
    int i;
    JSTimer *th;
    struct timespec ts;

    JSTimerContextState *state = get_timer_state(ctx, false);
    if (!state) return;

    while (interpreter_state >= 0) {
        min_delay = 1000;
        cur_time = millis();
        has_timer = false;
        for (i = 0; i < MAX_TIMERS; i++) {
            th = &state->timers[i];
            if (th->allocated) {
                if (th->main && interpreter_state != 2) { continue; }
                if (th->main && interpreter_state == 2) { interpreter_state = 3; }
                has_timer = true;
                delayMs = th->timeout - cur_time;
                if (delayMs <= 0) {
                    JSValue ret;
                    /* the timer expired */
                    if (JS_StackCheck(ctx, 2)) goto fail;
                    JS_PushArg(ctx, th->func.val); /* func name */
                    JS_PushArg(ctx, JS_NULL);      /* this */

                    if (!th->repeat && !th->main) {
                        JS_DeleteGCRef(ctx, &th->func);
                        th->allocated = false;
                    } else {
                        // Reschedule before calling so callbacks can clearInterval safely.
                        // Keep cadence by advancing from the previous due time.
                        th->timeout = th->timeout + (int64_t)th->interval_ms;
                    }

                    ret = JS_Call(ctx, 0);
                    if (JS_IsException(ret)) {
                    fail:
                        log_e("Error in run_timers");
                        JSValue obj = JS_GetException(ctx);
                        JS_PrintValueF(ctx, obj, JS_DUMP_LONG);
                        return;
                    }

                    if (th->main) { interpreter_state = 0; }

                    // If interval callback threw, state may still be allocated; keep going.
                    min_delay = 0;
                    break;
                } else if (delayMs < min_delay) {
                    min_delay = delayMs;
                }
            }
        }
        if (!has_timer) break;
        if (min_delay > 0) { delay(min_delay); }
    }
}

JSValue js_print(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int i;
    JSValue v;

    for (i = 0; i < argc; i++) {
        if (i != 0) putchar(' ');
        v = argv[i];
        if (JS_IsString(ctx, v)) {
            JSCStringBuf buf;
            const char *str;
            size_t len;
            str = JS_ToCStringLen(ctx, &len, v, &buf);
            fwrite(str, 1, (size_t)len, stdout);
        } else {
            JS_PrintValueF(ctx, argv[i], JS_DUMP_LONG);
        }
    }
    putchar('\n');
    return JS_UNDEFINED;
}

JSValue js_date_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return JS_NewInt64(ctx, (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL)));
}

JSValue js_performance_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return JS_NewInt64(ctx, (int64_t)millis());
}

// TODO: Implement user module
JSValue native_require(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc < 1) { return JS_ThrowTypeError(ctx, "require() expects 1 argument"); }

    JSCStringBuf name_buf;
    const char *name = JS_ToCString(ctx, argv[0], &name_buf);
    if (!name) { return JS_EXCEPTION; }

    JSValue global = JS_GetGlobalObject(ctx);
    JSValue val = JS_GetPropertyStr(ctx, global, name);

    return val;
}

JSValue native_assert(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    bool ok = false;
    if (argc > 0) ok = JS_ToBool(ctx, argv[0]);
    if (ok) { return JS_NewBool(1); }
    const char *msg = "assert";
    if (argc > 1 && JS_IsString(ctx, argv[1])) {
        JSCStringBuf sb;
        const char *s = JS_ToCString(ctx, argv[1], &sb);
        if (s) msg = s;
    }
    char buf[256];
    snprintf(buf, sizeof(buf), "Assertion failed: %s", msg);
    return JS_ThrowTypeError(ctx, "%s", buf);
}

JSValue native_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return JS_NewInt64(ctx, (int64_t)millis());
}

JSValue native_delay(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (interpreter_state < 0) { return JS_ThrowInternalError(ctx, "Script exited"); }
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) {
        int ms;
        JS_ToInt32(ctx, &ms, argv[0]);
        vTaskDelay(pdMS_TO_TICKS(ms));
    }
    if (interpreter_state < 0) { return JS_ThrowInternalError(ctx, "Script exited"); }
    return JS_UNDEFINED;
}

JSValue native_random(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int val = 0;
    if (argc > 1 && JS_IsNumber(ctx, argv[0]) && JS_IsNumber(ctx, argv[1])) {
        int a, b;
        JS_ToInt32(ctx, &a, argv[0]);
        JS_ToInt32(ctx, &b, argv[1]);
        val = random(a, b);
    } else if (argc > 0 && JS_IsNumber(ctx, argv[0])) {
        int a;
        JS_ToInt32(ctx, &a, argv[0]);
        val = random(a);
    } else {
        val = random();
    }
    return JS_NewInt32(ctx, val);
}

JSValue native_parse_int(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc > 0 && (JS_IsString(ctx, argv[0]) || JS_IsNumber(ctx, argv[0]) || JS_IsBool(argv[0]))) {
        double d;
        JS_ToNumber(ctx, &d, argv[0]);
        return JS_NewFloat64(ctx, d);
    }
    return JS_NewFloat64(ctx, NAN);
}

JSValue native_to_string(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc > 0) { return JS_ToString(ctx, argv[0]); }
    return JS_NewString(ctx, "");
}

JSValue native_to_hex_string(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc > 0 && (JS_IsString(ctx, argv[0]) || JS_IsNumber(ctx, argv[0]) || JS_IsBool(argv[0]))) {
        int value;
        JS_ToInt32(ctx, &value, argv[0]);
        return JS_NewString(ctx, String(value, HEX).c_str());
    }
    return JS_NewString(ctx, "");
}

JSValue native_to_lower_case(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc > 0 && JS_IsString(ctx, argv[0])) {
        JSCStringBuf sb;
        const char *s = JS_ToCString(ctx, argv[0], &sb);
        if (s) {
            String text = String(s);
            text.toLowerCase();
            return JS_NewString(ctx, text.c_str());
        }
    }
    return JS_NewString(ctx, "");
}

JSValue native_to_upper_case(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc > 0 && JS_IsString(ctx, argv[0])) {
        JSCStringBuf sb;
        const char *s = JS_ToCString(ctx, argv[0], &sb);
        if (s) {
            String text = String(s);
            text.toUpperCase();
            return JS_NewString(ctx, text.c_str());
        }
    }
    return JS_NewString(ctx, "");
}

JSValue native_atob(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc < 1 || !JS_IsString(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "atob: arg0 must be string");
    }

    size_t in_len = 0;
    JSCStringBuf in_sb;
    const char *in = JS_ToCStringLen(ctx, &in_len, argv[0], &in_sb);
    if (!in) return JS_EXCEPTION;

    // Strip ASCII whitespace to be a bit more forgiving.
    uint8_t *clean = (uint8_t *)malloc(in_len + 1);
    if (!clean) return JS_ThrowOutOfMemory(ctx);
    size_t clean_len = 0;
    for (size_t i = 0; i < in_len; ++i) {
        char ch = in[i];
        if (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t') continue;
        clean[clean_len++] = (uint8_t)ch;
    }

    size_t out_max = (clean_len / 4) * 3 + 3;
    uint8_t *out = (uint8_t *)malloc(out_max);
    if (!out) {
        free(clean);
        return JS_ThrowOutOfMemory(ctx);
    }

    size_t out_len = 0;
    int rc = mbedtls_base64_decode(out, out_max, &out_len, clean, clean_len);
    free(clean);

    if (rc != 0) {
        free(out);
        return JS_ThrowTypeError(ctx, "atob: invalid base64");
    }

    JSValue s = buffer_latin1_to_string(ctx, out, out_len);
    free(out);
    return s;
}

JSValue native_btoa(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc < 1) return JS_ThrowTypeError(ctx, "btoa: missing arg0");

    // btoa(string) where string is latin1/binary
    if (JS_IsString(ctx, argv[0])) {
        size_t in_len = 0;
        JSCStringBuf in_sb;
        const char *in = JS_ToCStringLen(ctx, &in_len, argv[0], &in_sb);
        if (!in) return JS_EXCEPTION;

        uint8_t *bytes = NULL;
        size_t bytes_len = 0;
        if (!buffer_latin1_string_to_bytes((const uint8_t *)in, in_len, &bytes, &bytes_len)) {
            return JS_ThrowTypeError(ctx, "btoa: input must be latin1/binary string");
        }

        size_t out_max = ((bytes_len + 2) / 3) * 4 + 1;
        uint8_t *out = (uint8_t *)malloc(out_max);
        if (!out) {
            free(bytes);
            return JS_ThrowOutOfMemory(ctx);
        }

        size_t out_len = 0;
        int rc = mbedtls_base64_encode(out, out_max, &out_len, bytes, bytes_len);
        free(bytes);

        if (rc != 0) {
            free(out);
            return JS_ThrowTypeError(ctx, "btoa: base64 encode failed");
        }

        JSValue s = JS_NewStringLen(ctx, (const char *)out, out_len);
        free(out);
        return s;
    }

    // btoa(Uint8Array|TypedArray)
    if (JS_IsTypedArray(ctx, argv[0])) {
        size_t bytes_len = 0;
        const uint8_t *bytes = (const uint8_t *)JS_GetTypedArrayBuffer(ctx, &bytes_len, argv[0]);
        if (!bytes) return JS_ThrowTypeError(ctx, "btoa: invalid typed array backing store");

        size_t out_max = ((bytes_len + 2) / 3) * 4 + 1;
        uint8_t *out = (uint8_t *)malloc(out_max);
        if (!out) return JS_ThrowOutOfMemory(ctx);

        size_t out_len = 0;
        int rc = mbedtls_base64_encode(out, out_max, &out_len, bytes, bytes_len);
        if (rc != 0) {
            free(out);
            return JS_ThrowTypeError(ctx, "btoa: base64 encode failed");
        }

        JSValue s = JS_NewStringLen(ctx, (const char *)out, out_len);
        free(out);
        return s;
    }

    return JS_ThrowTypeError(ctx, "btoa: arg0 must be string or Uint8Array");
}

JSValue native_btoa_bin(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc < 1 || !JS_IsTypedArray(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "btoa_bin: arg0 must be Uint8Array");
    }
    // Delegate to btoa() typed-array path.
    return native_btoa(ctx, this_val, 1, argv);
}

JSValue native_atob_bin(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc < 1 || !JS_IsString(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "atob_bin: arg0 must be string");
    }

    size_t in_len = 0;
    JSCStringBuf in_sb;
    const char *in = JS_ToCStringLen(ctx, &in_len, argv[0], &in_sb);
    if (!in) return JS_EXCEPTION;

    // Strip ASCII whitespace
    uint8_t *clean = (uint8_t *)malloc(in_len + 1);
    if (!clean) return JS_ThrowOutOfMemory(ctx);
    size_t clean_len = 0;
    for (size_t i = 0; i < in_len; ++i) {
        char ch = in[i];
        if (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t') continue;
        clean[clean_len++] = (uint8_t)ch;
    }

    size_t out_max = (clean_len / 4) * 3 + 3;
    uint8_t *out = (uint8_t *)malloc(out_max);
    if (!out) {
        free(clean);
        return JS_ThrowOutOfMemory(ctx);
    }

    size_t out_len = 0;
    int rc = mbedtls_base64_decode(out, out_max, &out_len, clean, clean_len);
    free(clean);
    if (rc != 0) {
        free(out);
        return JS_ThrowTypeError(ctx, "atob_u8: invalid base64");
    }

    JSValue bytes = JS_NewUint8ArrayCopy(ctx, out, out_len);
    free(out);
    return bytes;
}

JSValue native_exit(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return JS_ThrowInternalError(ctx, "Script exited");
}

#endif
