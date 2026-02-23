/*
 * Micro QuickJS REPL library
 *
 * Copyright (c) 2017-2025 Fabrice Bellard
 * Copyright (c) 2017-2025 Charlie Gordon
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <mquickjs_build.h>

#include "user_classes_js.h"

static const JSPropDef js_object_proto[] = {
    JS_CFUNC_DEF("hasOwnProperty", 1, js_object_hasOwnProperty),
    JS_CFUNC_DEF("toString", 0, js_object_toString),
    JS_PROP_END,
};

static const JSPropDef js_object[] = {
    JS_CFUNC_DEF("defineProperty", 3, js_object_defineProperty),
    JS_CFUNC_DEF("getPrototypeOf", 1, js_object_getPrototypeOf),
    JS_CFUNC_DEF("setPrototypeOf", 2, js_object_setPrototypeOf),
    JS_CFUNC_DEF("create", 2, js_object_create),
    JS_CFUNC_DEF("keys", 1, js_object_keys),
    JS_CFUNC_DEF("getOwnPropertyNames", 1, js_object_keys),
    JS_PROP_END,
};

static const JSClassDef js_object_class =
    JS_CLASS_DEF("Object", 1, js_object_constructor, JS_CLASS_OBJECT,
                 js_object, js_object_proto, NULL, NULL);

static const JSPropDef js_function_proto[] = {
    JS_CGETSET_DEF("prototype", js_function_get_prototype, js_function_set_prototype ),
    JS_CFUNC_DEF("call", 1, js_function_call ),
    JS_CFUNC_DEF("apply", 2, js_function_apply ),
    JS_CFUNC_DEF("bind", 1, js_function_bind ),
    JS_CFUNC_DEF("toString", 0, js_function_toString ),
    JS_CGETSET_MAGIC_DEF("length", js_function_get_length_name, NULL, 0 ),
    JS_CGETSET_MAGIC_DEF("name", js_function_get_length_name, NULL, 1 ),
    JS_PROP_END,
};

static const JSClassDef js_function_class =
    JS_CLASS_DEF("Function", 1, js_function_constructor, JS_CLASS_CLOSURE, NULL, js_function_proto, NULL, NULL);

static const JSPropDef js_number_proto[] = {
    JS_CFUNC_DEF("toExponential", 1, js_number_toExponential ),
    JS_CFUNC_DEF("toFixed", 1, js_number_toFixed ),
    JS_CFUNC_DEF("toPrecision", 1, js_number_toPrecision ),
    JS_CFUNC_DEF("toString", 1, js_number_toString ),
    JS_PROP_END,
};

static const JSPropDef js_number[] = {
    JS_CFUNC_DEF("parseInt", 2, js_number_parseInt ),
    JS_CFUNC_DEF("parseFloat", 1, js_number_parseFloat ),
    JS_PROP_DOUBLE_DEF("MAX_VALUE", 1.7976931348623157e+308, 0 ),
    JS_PROP_DOUBLE_DEF("MIN_VALUE", 5e-324, 0 ),
    JS_PROP_DOUBLE_DEF("NaN", NAN, 0 ),
    JS_PROP_DOUBLE_DEF("NEGATIVE_INFINITY", -INFINITY, 0 ),
    JS_PROP_DOUBLE_DEF("POSITIVE_INFINITY", INFINITY, 0 ),
    JS_PROP_DOUBLE_DEF("EPSILON", 2.220446049250313e-16, 0 ), /* ES6 */
    JS_PROP_DOUBLE_DEF("MAX_SAFE_INTEGER", 9007199254740991.0, 0 ), /* ES6 */
    JS_PROP_DOUBLE_DEF("MIN_SAFE_INTEGER", -9007199254740991.0, 0 ), /* ES6 */
    JS_PROP_END,
};

static const JSClassDef js_number_class =
    JS_CLASS_DEF("Number", 1, js_number_constructor, JS_CLASS_NUMBER, js_number, js_number_proto, NULL, NULL);

static const JSClassDef js_boolean_class =
    JS_CLASS_DEF("Boolean", 1, js_boolean_constructor, JS_CLASS_BOOLEAN, NULL, NULL, NULL, NULL);

static const JSPropDef js_string_proto[] = {
    JS_CGETSET_DEF("length", js_string_get_length, js_string_set_length ),
    JS_CFUNC_MAGIC_DEF("charAt", 1, js_string_charAt, magic_charAt ),
    JS_CFUNC_MAGIC_DEF("charCodeAt", 1, js_string_charAt, magic_charCodeAt ),
    JS_CFUNC_MAGIC_DEF("codePointAt", 1, js_string_charAt, magic_codePointAt ),
    JS_CFUNC_DEF("slice", 2, js_string_slice ),
    JS_CFUNC_DEF("substr", 2, js_string_substring ),
    JS_CFUNC_DEF("substring", 2, js_string_substring ),
    JS_CFUNC_DEF("concat", 1, js_string_concat ),
    JS_CFUNC_MAGIC_DEF("indexOf", 1, js_string_indexOf, 0 ),
    JS_CFUNC_MAGIC_DEF("lastIndexOf", 1, js_string_indexOf, 1 ),
    JS_CFUNC_DEF("match", 1, js_string_match ),
    JS_CFUNC_MAGIC_DEF("replace", 2, js_string_replace, 0 ),
    JS_CFUNC_MAGIC_DEF("replaceAll", 2, js_string_replace, 1 ),
    JS_CFUNC_DEF("search", 1, js_string_search ),
    JS_CFUNC_DEF("split", 2, js_string_split ),
    JS_CFUNC_MAGIC_DEF("toLowerCase", 0, js_string_toLowerCase, 1 ),
    JS_CFUNC_MAGIC_DEF("toUpperCase", 0, js_string_toLowerCase, 0 ),
    JS_CFUNC_MAGIC_DEF("trim", 0, js_string_trim, 3 ),
    JS_CFUNC_MAGIC_DEF("trimEnd", 0, js_string_trim, 2 ),
    JS_CFUNC_MAGIC_DEF("trimStart", 0, js_string_trim, 1 ),
    JS_CFUNC_DEF("toString", 0, js_string_toString ),
    JS_PROP_END,
};

static const JSPropDef js_string[] = {
    JS_CFUNC_MAGIC_DEF("fromCharCode", 1, js_string_fromCharCode, 0 ),
    JS_CFUNC_MAGIC_DEF("fromCodePoint", 1, js_string_fromCharCode, 1 ),
    JS_PROP_END,
};

static const JSClassDef js_string_class =
    JS_CLASS_DEF("String", 1, js_string_constructor, JS_CLASS_STRING, js_string, js_string_proto, NULL, NULL);

static const JSPropDef js_array_proto[] = {
    JS_CFUNC_DEF("concat", 1, js_array_concat ),
    JS_CGETSET_DEF("length", js_array_get_length, js_array_set_length ),
    JS_CFUNC_MAGIC_DEF("push", 1, js_array_push, 0 ),
    JS_CFUNC_DEF("pop", 0, js_array_pop ),
    JS_CFUNC_DEF("join", 1, js_array_join ),
    JS_CFUNC_DEF("toString", 0, js_array_toString ),
    JS_CFUNC_DEF("reverse", 0, js_array_reverse ),
    JS_CFUNC_DEF("shift", 0, js_array_shift ),
    JS_CFUNC_DEF("slice", 2, js_array_slice ),
    JS_CFUNC_DEF("splice", 2, js_array_splice ),
    JS_CFUNC_DEF("fill", 3, js_fill ),
    JS_CFUNC_MAGIC_DEF("unshift", 1, js_array_push, 1 ),
    JS_CFUNC_MAGIC_DEF("indexOf", 1, js_array_indexOf, 0 ),
    JS_CFUNC_MAGIC_DEF("lastIndexOf", 1, js_array_indexOf, 1 ),
    JS_CFUNC_MAGIC_DEF("every", 1, js_array_every, js_special_every ),
    JS_CFUNC_MAGIC_DEF("some", 1, js_array_every, js_special_some ),
    JS_CFUNC_MAGIC_DEF("forEach", 1, js_array_every, js_special_forEach ),
    JS_CFUNC_MAGIC_DEF("map", 1, js_array_every, js_special_map ),
    JS_CFUNC_MAGIC_DEF("filter", 1, js_array_every, js_special_filter ),
    JS_CFUNC_MAGIC_DEF("reduce", 1, js_array_reduce, js_special_reduce ),
    JS_CFUNC_MAGIC_DEF("reduceRight", 1, js_array_reduce, js_special_reduceRight ),
    JS_CFUNC_MAGIC_DEF("reduce", 1, js_array_reduce, js_special_reduce ),
    JS_CFUNC_DEF("sort", 1, js_array_sort ),
    JS_PROP_END,
};

static const JSPropDef js_array[] = {
    JS_CFUNC_DEF("isArray", 1, js_array_isArray ),
    JS_PROP_END,
};

static const JSClassDef js_array_class =
    JS_CLASS_DEF("Array", 1, js_array_constructor, JS_CLASS_ARRAY, js_array, js_array_proto, NULL, NULL);

static const JSPropDef js_error_proto[] = {
    JS_CFUNC_DEF("toString", 0, js_error_toString ),
    JS_PROP_STRING_DEF("name", "Error", 0 ),
    JS_CGETSET_MAGIC_DEF("message", js_error_get_message, NULL, 0 ),
    JS_CGETSET_MAGIC_DEF("stack", js_error_get_message, NULL, 1 ),
    JS_PROP_END,
};

static const JSClassDef js_error_class =
    JS_CLASS_MAGIC_DEF("Error", 1, js_error_constructor, JS_CLASS_ERROR, NULL, js_error_proto, NULL, NULL);

#define ERROR_DEF(cname, name, class_id)                       \
    static const JSPropDef js_ ## cname ## _proto[] = { \
        JS_PROP_STRING_DEF("name", name, 0 ),                  \
        JS_PROP_END,                                         \
    };                                                                 \
    static const JSClassDef js_ ## cname ## _class =                    \
        JS_CLASS_MAGIC_DEF(name, 1, js_error_constructor, class_id, NULL, js_ ## cname ## _proto, &js_error_class, NULL);

ERROR_DEF(eval_error, "EvalError", JS_CLASS_EVAL_ERROR)
ERROR_DEF(range_error, "RangeError", JS_CLASS_RANGE_ERROR)
ERROR_DEF(reference_error, "ReferenceError", JS_CLASS_REFERENCE_ERROR)
ERROR_DEF(syntax_error, "SyntaxError", JS_CLASS_SYNTAX_ERROR)
ERROR_DEF(type_error, "TypeError", JS_CLASS_TYPE_ERROR)
ERROR_DEF(uri_error, "URIError", JS_CLASS_URI_ERROR)
ERROR_DEF(internal_error, "InternalError", JS_CLASS_INTERNAL_ERROR)


static const JSPropDef js_math[] = {
    JS_CFUNC_MAGIC_DEF("min", 2, js_math_min_max, 0 ),
    JS_CFUNC_MAGIC_DEF("max", 2, js_math_min_max, 1 ),
    JS_CFUNC_SPECIAL_DEF("sign", 1, f_f, js_math_sign ),
    JS_CFUNC_SPECIAL_DEF("abs", 1, f_f, js_fabs ),
    JS_CFUNC_SPECIAL_DEF("floor", 1, f_f, js_floor ),
    JS_CFUNC_SPECIAL_DEF("ceil", 1, f_f, js_ceil ),
    JS_CFUNC_SPECIAL_DEF("round", 1, f_f, js_round_inf ),
    JS_CFUNC_SPECIAL_DEF("sqrt", 1, f_f, js_sqrt ),

    JS_PROP_DOUBLE_DEF("E", 2.718281828459045, 0 ),
    JS_PROP_DOUBLE_DEF("LN10", 2.302585092994046, 0 ),
    JS_PROP_DOUBLE_DEF("LN2", 0.6931471805599453, 0 ),
    JS_PROP_DOUBLE_DEF("LOG2E", 1.4426950408889634, 0 ),
    JS_PROP_DOUBLE_DEF("LOG10E", 0.4342944819032518, 0 ),
    JS_PROP_DOUBLE_DEF("PI", 3.141592653589793, 0 ),
    JS_PROP_DOUBLE_DEF("SQRT1_2", 0.7071067811865476, 0 ),
    JS_PROP_DOUBLE_DEF("SQRT2", 1.4142135623730951, 0 ),

    JS_CFUNC_SPECIAL_DEF("sin", 1, f_f, js_sin ),
    JS_CFUNC_SPECIAL_DEF("cos", 1, f_f, js_cos ),
    JS_CFUNC_SPECIAL_DEF("tan", 1, f_f, js_tan ),
    JS_CFUNC_SPECIAL_DEF("asin", 1, f_f, js_asin ),
    JS_CFUNC_SPECIAL_DEF("acos", 1, f_f, js_acos ),
    JS_CFUNC_SPECIAL_DEF("atan", 1, f_f, js_atan ),
    JS_CFUNC_DEF("atan2", 2, js_math_atan2 ),
    JS_CFUNC_SPECIAL_DEF("exp", 1, f_f, js_exp ),
    JS_CFUNC_SPECIAL_DEF("log", 1, f_f, js_log ),
    JS_CFUNC_DEF("pow", 2, js_math_pow ),
    JS_CFUNC_DEF("random", 0, js_math_random ),

    /* some ES6 functions */
    JS_CFUNC_DEF("imul", 2, js_math_imul ),
    JS_CFUNC_DEF("clz32", 1, js_math_clz32 ),
    JS_CFUNC_SPECIAL_DEF("fround", 1, f_f, js_math_fround ),
    JS_CFUNC_SPECIAL_DEF("trunc", 1, f_f, js_trunc ),
    JS_CFUNC_SPECIAL_DEF("log2", 1, f_f, js_log2 ),
    JS_CFUNC_SPECIAL_DEF("log10", 1, f_f, js_log10 ),

    /* Bruce: additional math helpers */
    JS_CFUNC_DEF("acosh", 1, native_math_acosh),
    JS_CFUNC_DEF("asinh", 1, native_math_asinh),
    JS_CFUNC_DEF("atanh", 1, native_math_atanh),
    JS_CFUNC_DEF("is_equal", 3, native_math_is_equal),

    JS_PROP_END,
};

static const JSClassDef js_math_obj =
    JS_OBJECT_DEF("Math", js_math);

static const JSPropDef js_json[] = {
    JS_CFUNC_DEF("parse", 2, js_json_parse ),
    JS_CFUNC_DEF("stringify", 3, js_json_stringify ),
    JS_PROP_END,
};

static const JSClassDef js_json_obj =
    JS_OBJECT_DEF("JSON", js_json);

/* typed arrays */
static const JSPropDef js_array_buffer_proto[] = {
    JS_CGETSET_DEF("byteLength", js_array_buffer_get_byteLength, NULL ),
    JS_PROP_END,
};

static const JSClassDef js_array_buffer_class =
    JS_CLASS_DEF("ArrayBuffer", 1, js_array_buffer_constructor, JS_CLASS_ARRAY_BUFFER, NULL, js_array_buffer_proto, NULL, NULL);

static const JSPropDef js_typed_array_base_proto[] = {
    JS_CGETSET_MAGIC_DEF("length", js_typed_array_get_length, NULL, 0 ),
    JS_CGETSET_MAGIC_DEF("byteLength", js_typed_array_get_length, NULL, 1 ),
    JS_CGETSET_MAGIC_DEF("byteOffset", js_typed_array_get_length, NULL, 2 ),
    JS_CGETSET_MAGIC_DEF("buffer", js_typed_array_get_length, NULL, 3 ),
    JS_CFUNC_DEF("join", 1, js_array_join ),
    JS_CFUNC_DEF("toString", 0, js_array_toString ),
    JS_CFUNC_DEF("subarray", 2, js_typed_array_subarray ),
    JS_CFUNC_DEF("set", 1, js_typed_array_set ),
    JS_CFUNC_DEF("fill", 3, js_fill ),
    JS_PROP_END,
};

static const JSClassDef js_typed_array_base_class =
    JS_CLASS_DEF("TypedArray", 0, js_typed_array_base_constructor, JS_CLASS_TYPED_ARRAY, NULL, js_typed_array_base_proto, NULL, NULL);

#define TA_DEF(name, class_name, bpe)\
static const JSPropDef js_ ## name [] = {\
    JS_PROP_DOUBLE_DEF("BYTES_PER_ELEMENT", bpe, 0),\
    JS_PROP_END,\
};\
static const JSPropDef js_ ## name ## _proto[] = {\
    JS_PROP_DOUBLE_DEF("BYTES_PER_ELEMENT", bpe, 0),\
    JS_PROP_END,\
};\
static const JSClassDef js_ ## name ## _class =\
    JS_CLASS_MAGIC_DEF(#name, 3, js_typed_array_constructor, class_name, js_ ## name, js_ ## name ## _proto, &js_typed_array_base_class, NULL);

TA_DEF(Uint8ClampedArray, JS_CLASS_UINT8C_ARRAY, 1)
TA_DEF(Int8Array, JS_CLASS_INT8_ARRAY, 1)
TA_DEF(Uint8Array, JS_CLASS_UINT8_ARRAY, 1)
TA_DEF(Int16Array, JS_CLASS_INT16_ARRAY, 2)
TA_DEF(Uint16Array, JS_CLASS_UINT16_ARRAY, 2)
TA_DEF(Int32Array, JS_CLASS_INT32_ARRAY, 4)
TA_DEF(Uint32Array, JS_CLASS_UINT32_ARRAY, 4)
TA_DEF(Float32Array, JS_CLASS_FLOAT32_ARRAY, 4)
TA_DEF(Float64Array, JS_CLASS_FLOAT64_ARRAY, 8)

/* regexp */

static const JSPropDef js_regexp_proto[] = {
    JS_CGETSET_DEF("lastIndex", js_regexp_get_lastIndex, js_regexp_set_lastIndex ),
    JS_CGETSET_DEF("source", js_regexp_get_source, NULL ),
    JS_CGETSET_DEF("flags", js_regexp_get_flags, NULL ),
    JS_CFUNC_MAGIC_DEF("exec", 1, js_regexp_exec, 0 ),
    JS_CFUNC_MAGIC_DEF("test", 1, js_regexp_exec, 1 ),
    JS_PROP_END,
};

static const JSClassDef js_regexp_class =
    JS_CLASS_DEF("RegExp", 2, js_regexp_constructor, JS_CLASS_REGEXP, NULL, js_regexp_proto, NULL, NULL);

/* other objects */

static const JSPropDef js_date[] = {
    JS_CFUNC_DEF("now", 0, js_date_now),
    JS_PROP_END,
};

static const JSClassDef js_date_class =
    JS_CLASS_DEF("Date", 7, js_date_constructor, JS_CLASS_DATE, js_date, NULL, NULL, NULL);

static const JSPropDef js_console[] = {
    JS_CFUNC_DEF("log", 1, js_print),
    JS_PROP_END,
};

static const JSClassDef js_console_obj =
    JS_OBJECT_DEF("Console", js_console);

static const JSPropDef js_performance[] = {
    JS_CFUNC_DEF("now", 0, js_performance_now),
    JS_PROP_END,
};
static const JSClassDef js_performance_obj =
    JS_OBJECT_DEF("Performance", js_performance);


static const JSPropDef js_exports[] = {
    JS_PROP_END,
};
static const JSClassDef js_exports_obj =
    JS_OBJECT_DEF("Exports", js_exports);

const JSPropDef js_audio[] = {
    JS_CFUNC_DEF("playFile", 1, native_playAudioFile),
    JS_CFUNC_DEF("tone", 3, native_tone),
    JS_PROP_END,
};

const JSClassDef js_audio_obj = JS_OBJECT_DEF("Audio", js_audio);

const JSPropDef js_keyboard[] = {
    JS_CFUNC_DEF("keyboard", 4, native_keyboard),
    JS_CFUNC_DEF("numKeyboard", 4, native_num_keyboard),
    JS_CFUNC_DEF("hexKeyboard", 4, native_hex_keyboard),
    JS_CFUNC_DEF("getKeysPressed", 0, native_getKeysPressed),
    JS_CFUNC_DEF("getPrevPress", 1, native_getPrevPress),
    JS_CFUNC_DEF("getSelPress", 1, native_getSelPress),
    JS_CFUNC_DEF("getEscPress", 1, native_getEscPress),
    JS_CFUNC_DEF("getNextPress", 1, native_getNextPress),
    JS_CFUNC_DEF("getAnyPress", 1, native_getAnyPress),
    JS_CFUNC_DEF("setLongPress", 1, native_setLongPress),
    JS_PROP_END,
};

const JSClassDef js_keyboard_obj = JS_OBJECT_DEF("Keyboard", js_keyboard);

const JSPropDef js_notification[] = {
    JS_CFUNC_DEF("blink", 1, native_notifyBlink),
    JS_PROP_END,
};

const JSClassDef js_notification_obj = JS_OBJECT_DEF("Notification", js_notification);

/* BadUSB module */
static const JSPropDef js_badusb[] = {
    JS_CFUNC_DEF("setup", 0, native_badusbSetup),
    JS_CFUNC_DEF("print", 1, native_badusbPrint),
    JS_CFUNC_DEF("println", 1, native_badusbPrintln),
    JS_CFUNC_DEF("press", 1, native_badusbPress),
    JS_CFUNC_DEF("hold", 1, native_badusbHold),
    JS_CFUNC_DEF("release", 1, native_badusbRelease),
    JS_CFUNC_DEF("releaseAll", 0, native_badusbReleaseAll),
    JS_CFUNC_DEF("pressRaw", 1, native_badusbPressRaw),
    JS_CFUNC_DEF("runFile", 1, native_badusbRunFile),
    JS_PROP_END,
};

const JSClassDef js_badusb_obj = JS_OBJECT_DEF("BadUSB", js_badusb);

/* IR module */
static const JSPropDef js_ir[] = {
    JS_CFUNC_DEF("read", 1, native_irRead),
    JS_CFUNC_DEF("readRaw", 1, native_irReadRaw),
    JS_CFUNC_DEF("transmitFile", 1, native_irTransmitFile),
    JS_CFUNC_DEF("transmit", 3, native_irTransmit),
    JS_PROP_END,
};

const JSClassDef js_ir_obj = JS_OBJECT_DEF("IR", js_ir);

/* Dialog module */
static const JSPropDef js_dialog[] = {
    JS_CFUNC_DEF("message", 2, native_dialogMessage),
    JS_CFUNC_DEF("info", 2, native_dialogInfo),
    JS_CFUNC_DEF("success", 2, native_dialogSuccess),
    JS_CFUNC_DEF("warning", 2, native_dialogWarning),
    JS_CFUNC_DEF("error", 2, native_dialogError),
    JS_CFUNC_DEF("choice", 1, native_dialogChoice),
    JS_CFUNC_DEF("prompt", 3, native_keyboard),
    JS_CFUNC_DEF("pickFile", 2, native_dialogPickFile),
    JS_CFUNC_DEF("viewFile", 1, native_dialogViewFile),
    JS_CFUNC_DEF("viewText", 2, native_dialogViewText),
    JS_CFUNC_DEF("createTextViewer", 2, native_dialogCreateTextViewer),
    JS_CFUNC_DEF("drawStatusBar", 0, native_drawStatusBar),
    JS_PROP_END,
};

const JSClassDef js_dialog_obj = JS_OBJECT_DEF("Dialog", js_dialog);

const JSPropDef js_subghz[] = {
    JS_CFUNC_DEF("transmitFile", 1, native_subghzTransmitFile),
    JS_CFUNC_DEF("transmit", 4, native_subghzTransmit),
    JS_CFUNC_DEF("read", 1, native_subghzRead),
    JS_CFUNC_DEF("readRaw", 1, native_subghzReadRaw),
    JS_CFUNC_DEF("setFrequency", 1, native_subghzSetFrequency),
    JS_PROP_END,
};

const JSClassDef js_subghz_obj = JS_OBJECT_DEF("SubGHz", js_subghz);

const JSPropDef js_serial[] = {
    JS_CFUNC_DEF("print", 1, native_serialPrint),
    JS_CFUNC_DEF("println", 1, native_serialPrintln),
    JS_CFUNC_DEF("readln", 1, native_serialReadln),
    JS_CFUNC_DEF("cmd", 1, native_serialCmd),
    JS_CFUNC_DEF("write", 1, native_serialPrint),
    JS_PROP_END,
};

const JSClassDef js_serial_obj = JS_OBJECT_DEF("Serial", js_serial);

const JSPropDef js_storage[] = {
    JS_CFUNC_DEF("readdir", 2, native_storageReaddir),
    JS_CFUNC_DEF("read", 2, native_storageRead),
    JS_CFUNC_DEF("write", 4, native_storageWrite),
    JS_CFUNC_DEF("rename", 2, native_storageRename),
    JS_CFUNC_DEF("remove", 1, native_storageRemove),
    JS_CFUNC_DEF("mkdir", 1, native_storageMkdir),
    JS_CFUNC_DEF("rmdir", 1, native_storageRmdir),
    JS_CFUNC_DEF("spaceLittleFS", 0, native_storageSpaceLittleFS),
    JS_CFUNC_DEF("spaceSDCard", 0, native_storageSpaceSDCard),
    JS_PROP_END,
};

const JSClassDef js_storage_obj = JS_OBJECT_DEF("Storage", js_storage);

/* Device module */
static const JSPropDef js_device[] = {
    JS_CFUNC_DEF("getName", 0, native_getDeviceName),
    JS_CFUNC_DEF("getBoard", 0, native_getBoard),
    JS_CFUNC_DEF("getModel", 0, native_getBoard),
    JS_CFUNC_DEF("getBruceVersion", 0, native_getBruceVersion),
    JS_CFUNC_DEF("getBatteryCharge", 0, native_getBattery),
    JS_CFUNC_DEF("getBatteryDetailed", 0, native_getBatteryDetailed),
    JS_CFUNC_DEF("getFreeHeapSize", 0, native_getFreeHeapSize),
    JS_CFUNC_DEF("getEEPROMSize", 0, native_getEEPROMSize),
    JS_PROP_END,
};

const JSClassDef js_device_obj = JS_OBJECT_DEF("Device", js_device);

/* GPIO module */
static const JSPropDef js_gpio[] = {
    JS_CFUNC_DEF("pinMode", 3, native_pinMode),
    JS_CFUNC_DEF("digitalRead", 1, native_digitalRead),
    JS_CFUNC_DEF("analogRead", 1, native_analogRead),
    JS_CFUNC_DEF("touchRead", 1, native_touchRead),
    JS_CFUNC_DEF("digitalWrite", 2, native_digitalWrite),
    JS_CFUNC_DEF("dacWrite", 2, native_dacWrite),

    JS_CFUNC_DEF("analogWrite", 2, native_analogWrite),
    JS_CFUNC_DEF("analogWriteResolution", 2, native_analogWriteResolution),
    JS_CFUNC_DEF("analogWriteFrequency", 2, native_analogWriteFrequency),

    JS_CFUNC_DEF("ledcAttach", 3, native_ledcAttach),
    JS_CFUNC_DEF("ledcWrite", 2, native_ledcWrite),
    JS_CFUNC_DEF("ledcWriteTone", 3, native_ledcWriteTone),
    JS_CFUNC_DEF("ledcFade", 3, native_ledcFade),
    JS_CFUNC_DEF("ledcChangeFrequency", 3, native_ledcChangeFrequency),
    JS_CFUNC_DEF("ledcDetach", 3, native_ledcDetach),

    JS_CFUNC_DEF("pins", 0, native_pins),
    JS_PROP_END,
};

const JSClassDef js_gpio_obj = JS_OBJECT_DEF("GPIO", js_gpio);

/* I2C module */
static const JSPropDef js_i2c[] = {
    JS_CFUNC_DEF("begin", 3, native_i2c_begin),
    JS_CFUNC_DEF("scan", 0, native_i2c_scan),
    JS_CFUNC_DEF("write", 3, native_i2c_write),
    JS_CFUNC_DEF("read", 2, native_i2c_read),
    JS_CFUNC_DEF("writeRead", 4, native_i2c_write_read),
    JS_PROP_END,
};

const JSClassDef js_i2c_obj = JS_OBJECT_DEF("I2C", js_i2c);

/* WiFi module */
static const JSPropDef js_wifi[] = {
    JS_CFUNC_DEF("connected", 0, native_wifiConnected),
    JS_CFUNC_DEF("connectDialog", 0, native_wifiConnectDialog),
    JS_CFUNC_DEF("connect", 3, native_wifiConnect),
    JS_CFUNC_DEF("scan", 0, native_wifiScan),
    JS_CFUNC_DEF("disconnect", 0, native_wifiDisconnect),
    JS_CFUNC_DEF("httpFetch", 2, native_httpFetch),
    JS_CFUNC_DEF("getMACAddress", 0, native_wifiMACAddress),
    JS_CFUNC_DEF("getIPAddress", 0, native_ipAddress),
    JS_PROP_END,
};

const JSClassDef js_wifi_obj = JS_OBJECT_DEF("WiFi", js_wifi);

/* Mic module */
static const JSPropDef js_mic[] = {
    JS_CFUNC_DEF("recordWav", 2, native_micRecordWav),
    JS_CFUNC_DEF("captureSamples", 1, native_micCaptureSamples),
    JS_PROP_END,
};

const JSClassDef js_mic_obj = JS_OBJECT_DEF("Mic", js_mic);

/* Rfid module */
static const JSPropDef js_rfid[] = {
    JS_CFUNC_DEF("read", 1, native_rfidRead),
    JS_CFUNC_DEF("readUID", 1, native_rfidReadUID),
    JS_CFUNC_DEF("write", 1, native_rfidWrite),
    JS_CFUNC_DEF("save", 1, native_rfidSave),
    JS_CFUNC_DEF("load", 1, native_rfidLoad),
    JS_CFUNC_DEF("clear", 0, native_rfidClear),
    JS_CFUNC_DEF("addMifareKey", 1, native_rfid_AddMifareKey),

    // SRIX functions
    JS_CFUNC_DEF("srixRead", 1, native_srixRead),
    JS_CFUNC_DEF("srixWrite", 1, native_srixWrite),
    JS_CFUNC_DEF("srixSave", 1, native_srixSave),
    JS_CFUNC_DEF("srixLoad", 1, native_srixLoad),
    JS_CFUNC_DEF("srixClear", 0, native_srixClear),
    JS_CFUNC_DEF("srixWriteBlock", 2, native_srixWriteBlock),
    JS_PROP_END,
};

const JSClassDef js_rfid_obj = JS_OBJECT_DEF("Rfid", js_rfid);

/* Runtime module */
static const JSPropDef js_runtime[] = {
    JS_CFUNC_DEF("toBackground", 0, native_runtimeToBackground),
    JS_CFUNC_DEF("toForeground", 0, native_runtimeToForeground),
    JS_CFUNC_DEF("isForeground", 0, native_runtimeIsForeground),
    JS_CFUNC_DEF("main", 1, native_runtimeMain),
    JS_PROP_END,
};

const JSClassDef js_runtime_obj = JS_OBJECT_DEF("Runtime", js_runtime);

/* Display module */
static const JSPropDef js_display[] = {
    JS_CFUNC_DEF("color", 4, native_color),
    JS_CFUNC_DEF("fill", 1, native_fillScreen),
    JS_CFUNC_DEF("setCursor", 2, native_setCursor),
    JS_CFUNC_DEF("print", 1, native_print),
    JS_CFUNC_DEF("println", 1, native_println),
    JS_CFUNC_DEF("setTextColor", 1, native_setTextColor),
    JS_CFUNC_DEF("setTextSize", 1, native_setTextSize),
    JS_CFUNC_DEF("setTextAlign", 2, native_setTextAlign),
    JS_CFUNC_DEF("drawText", 3, native_drawString),
    JS_CFUNC_DEF("drawString", 3, native_drawString),
    JS_CFUNC_DEF("drawPixel", 3, native_drawPixel),
    JS_CFUNC_DEF("drawLine", 5, native_drawLine),
    JS_CFUNC_DEF("drawWideLine", 6, native_drawWideLine),
    JS_CFUNC_DEF("drawFastVLine", 4, native_drawFastVLine),
    JS_CFUNC_DEF("drawFastHLine", 4, native_drawFastHLine),
    JS_CFUNC_DEF("drawRect", 5, native_drawRect),
    JS_CFUNC_DEF("drawFillRect", 5, native_drawFillRect),
    JS_CFUNC_DEF("drawFillRectGradient", 7, native_drawFillRectGradient),
    JS_CFUNC_DEF("drawRoundRect", 6, native_drawRoundRect),
    JS_CFUNC_DEF("drawFillRoundRect", 6, native_drawFillRoundRect),
    JS_CFUNC_DEF("drawTriangle", 7, native_drawTriangle),
    JS_CFUNC_DEF("drawFillTriangle", 7, native_drawFillTriangle),
    JS_CFUNC_DEF("drawCircle", 4, native_drawCircle),
    JS_CFUNC_DEF("drawFillCircle", 4, native_drawFillCircle),
    JS_CFUNC_DEF("drawBitmap", 7, native_drawBitmap),
    JS_CFUNC_DEF("drawXBitmap", 7, native_drawXBitmap),
    JS_CFUNC_DEF("drawArc", 6, native_drawArc),
    JS_CFUNC_DEF("drawJpg", 4, native_drawJpg),
#if !defined(LITE_VERSION)
    JS_CFUNC_DEF("drawGif", 6, native_drawGif),
    JS_CFUNC_DEF("gifOpen", 2, native_gifOpen),
#endif
    JS_CFUNC_DEF("width", 0, native_width),
    JS_CFUNC_DEF("height", 0, native_height),
    JS_CFUNC_DEF("createSprite", 2, native_createSprite),
    JS_CFUNC_DEF("getRotation", 0, native_getRotation),
    JS_CFUNC_DEF("getBrightness", 0, native_getBrightness),
    JS_CFUNC_DEF("setBrightness", 2, native_setBrightness),
    JS_CFUNC_DEF("restoreBrightness", 0, native_restoreBrightness),
    JS_PROP_END,
};

const JSClassDef js_display_obj = JS_OBJECT_DEF("Display", js_display);
/* TextViewer (dialog.createTextViewer) */
static const JSPropDef js_textviewer_proto[] = {
    JS_CFUNC_DEF("draw", 0, native_dialogCreateTextViewerDraw),
    JS_CFUNC_DEF("scrollUp", 0, native_dialogCreateTextViewerScrollUp),
    JS_CFUNC_DEF("scrollDown", 0, native_dialogCreateTextViewerScrollDown),
    JS_CFUNC_DEF("scrollToLine", 1, native_dialogCreateTextViewerScrollToLine),
    JS_CFUNC_DEF("getLine", 1, native_dialogCreateTextViewerGetLine),
    JS_CFUNC_DEF("getMaxLines", 0, native_dialogCreateTextViewerGetMaxLines),
    JS_CFUNC_DEF("getVisibleText", 0, native_dialogCreateTextViewerGetVisibleText),
    JS_CFUNC_DEF("clear", 0, native_dialogCreateTextViewerClear),
    JS_CFUNC_DEF("setText", 1, native_dialogCreateTextViewerFromString),
    JS_CFUNC_DEF("close", 0, native_dialogCreateTextViewerClose),
    JS_PROP_END,
};

static const JSPropDef js_textviewer[] = { JS_PROP_END };

static const JSClassDef js_textviewer_class = JS_CLASS_DEF(
    "TextViewer", 0, native_dialogCreateTextViewer, JS_CLASS_TEXTVIEWER, js_textviewer, js_textviewer_proto, NULL, native_textviewer_finalizer
);

static const JSPropDef js_sprite_proto[] = {
    JS_CFUNC_DEF("setTextColor", 1, native_setTextColor),
    JS_CFUNC_DEF("setTextSize", 1, native_setTextSize),
    JS_CFUNC_DEF("setTextAlign", 2, native_setTextAlign),
    JS_CFUNC_DEF("drawText", 3, native_drawString),
    JS_CFUNC_DEF("drawString", 3, native_drawString),
    JS_CFUNC_DEF("drawPixel", 3, native_drawPixel),
    JS_CFUNC_DEF("drawLine", 5, native_drawLine),
    JS_CFUNC_DEF("drawRect", 5, native_drawRect),
    JS_CFUNC_DEF("drawFillRect", 5, native_drawFillRect),
    JS_CFUNC_DEF("drawFillRectGradient", 7, native_drawFillRectGradient),
    JS_CFUNC_DEF("drawRoundRect", 6, native_drawRoundRect),
    JS_CFUNC_DEF("drawFillRoundRect", 6, native_drawFillRoundRect),
    JS_CFUNC_DEF("drawCircle", 4, native_drawCircle),
    JS_CFUNC_DEF("drawFillCircle", 4, native_drawFillCircle),
    JS_CFUNC_DEF("drawBitmap", 7, native_drawBitmap),
    JS_CFUNC_DEF("drawXBitmap", 7, native_drawXBitmap),
    JS_CFUNC_DEF("drawJpg", 4, native_drawJpg),
    JS_CFUNC_DEF("width", 0, native_width),
    JS_CFUNC_DEF("height", 0, native_height),
    JS_CFUNC_DEF("setCursor", 2, native_setCursor),
    JS_CFUNC_DEF("print", 1, native_print),
    JS_CFUNC_DEF("println", 1, native_println),
    JS_CFUNC_DEF("fill", 1, native_fillScreen),
    JS_CFUNC_DEF("color", 4, native_color),
    JS_CFUNC_DEF("getRotation", 0, native_getRotation),
    JS_CFUNC_DEF("getBrightness", 0, native_getBrightness),
    JS_CFUNC_DEF("setBrightness", 2, native_setBrightness),
    JS_CFUNC_DEF("restoreBrightness", 0, native_restoreBrightness),
    JS_CFUNC_DEF("pushSprite", 0, native_pushSprite),
    JS_CFUNC_DEF("deleteSprite", 0, native_deleteSprite),
    JS_PROP_END,
};

static const JSPropDef js_sprite[] = {
    JS_PROP_END,
};

static const JSClassDef js_sprite_class =
    JS_CLASS_DEF("Sprite", 0, native_createSprite, JS_CLASS_SPRITE, js_sprite, js_sprite_proto, NULL, native_sprite_finalizer);

static const JSPropDef js_gif_proto[] = {
    JS_CFUNC_DEF("gifPlayFrame", 3, native_gifPlayFrame),
    JS_CFUNC_DEF("gifDimensions", 0, native_gifDimensions),
    JS_CFUNC_DEF("gifReset", 0, native_gifReset),
    JS_CFUNC_DEF("gifClose", 1, native_gifClose),
    JS_PROP_END,
};

static const JSPropDef js_gif[] = {
    JS_PROP_END,
};

static const JSClassDef js_gif_class =
    JS_CLASS_DEF("Gif", 0, NULL, JS_CLASS_GIF, js_gif, js_gif_proto, NULL, native_gif_finalizer);

static const JSClassDef js_timers_state_class =
    JS_CLASS_DEF("TimersState", 0, NULL, JS_CLASS_TIMERS_STATE, NULL, NULL, NULL, native_timers_state_finalizer);

/* Buffer */
static const JSPropDef js_buffer_proto[] = {
    JS_CFUNC_DEF("toString", 1, native_buffer_toString),
    JS_PROP_END,
};

static const JSPropDef js_buffer[] = {
    JS_CFUNC_DEF("from", 2, native_buffer_from),
    JS_PROP_END,
};

static const JSClassDef js_buffer_class =
    JS_CLASS_DEF("Buffer", 0, NULL, JS_CLASS_BUFFER, js_buffer, js_buffer_proto, NULL, NULL);

static const JSPropDef js_internal_functions[] = {
    JS_PROP_CLASS_DEF("TimersState", &js_timers_state_class),
    JS_PROP_END,
};

const JSClassDef js_internal_functions_obj = JS_OBJECT_DEF("InternalFunctions", js_internal_functions);

static const JSPropDef js_global_object[] = {
    JS_PROP_CLASS_DEF("Object", &js_object_class),
    JS_PROP_CLASS_DEF("Function", &js_function_class),
    JS_PROP_CLASS_DEF("Number", &js_number_class),
    JS_PROP_CLASS_DEF("Boolean", &js_boolean_class),
    JS_PROP_CLASS_DEF("String", &js_string_class),
    JS_PROP_CLASS_DEF("Array", &js_array_class),
    JS_PROP_CLASS_DEF("Math", &js_math_obj),
    JS_PROP_CLASS_DEF("Date", &js_date_class),
    JS_PROP_CLASS_DEF("JSON", &js_json_obj),
    JS_PROP_CLASS_DEF("RegExp", &js_regexp_class),

    JS_PROP_CLASS_DEF("Error", &js_error_class),
    JS_PROP_CLASS_DEF("EvalError", &js_eval_error_class),
    JS_PROP_CLASS_DEF("RangeError", &js_range_error_class),
    JS_PROP_CLASS_DEF("ReferenceError", &js_reference_error_class),
    JS_PROP_CLASS_DEF("SyntaxError", &js_syntax_error_class),
    JS_PROP_CLASS_DEF("TypeError", &js_type_error_class),
    JS_PROP_CLASS_DEF("URIError", &js_uri_error_class),
    JS_PROP_CLASS_DEF("InternalError", &js_internal_error_class),

    JS_PROP_CLASS_DEF("ArrayBuffer", &js_array_buffer_class),
    JS_PROP_CLASS_DEF("Uint8ClampedArray", &js_Uint8ClampedArray_class),
    JS_PROP_CLASS_DEF("Int8Array", &js_Int8Array_class),
    JS_PROP_CLASS_DEF("Uint8Array", &js_Uint8Array_class),
    JS_PROP_CLASS_DEF("Int16Array", &js_Int16Array_class),
    JS_PROP_CLASS_DEF("Uint16Array", &js_Uint16Array_class),
    JS_PROP_CLASS_DEF("Int32Array", &js_Int32Array_class),
    JS_PROP_CLASS_DEF("Uint32Array", &js_Uint32Array_class),
    JS_PROP_CLASS_DEF("Float32Array", &js_Float32Array_class),
    JS_PROP_CLASS_DEF("Float64Array", &js_Float64Array_class),

    JS_CFUNC_DEF("parseInt", 2, js_number_parseInt ),
    JS_CFUNC_DEF("parseFloat", 1, js_number_parseFloat ),
    JS_CFUNC_DEF("eval", 1, js_global_eval),
    JS_CFUNC_DEF("isNaN", 1, js_global_isNaN ),
    JS_CFUNC_DEF("isFinite", 1, js_global_isFinite ),

    JS_PROP_DOUBLE_DEF("Infinity", 1.0 / 0.0, 0 ),
    JS_PROP_DOUBLE_DEF("NaN", NAN, 0 ),
    JS_PROP_UNDEFINED_DEF("undefined", 0 ),
    /* Note: null is expanded as the global object in js_global_object[] */
    JS_PROP_NULL_DEF("globalThis", 0 ),

    JS_PROP_CLASS_DEF("console", &js_console_obj),
    JS_PROP_CLASS_DEF("performance", &js_performance_obj),

    JS_CFUNC_DEF("gc", 0, js_gc),
    JS_CFUNC_DEF("load", 1, js_load),
    JS_CFUNC_DEF("setTimeout", 2, js_setTimeout),
    JS_CFUNC_DEF("clearTimeout", 1, js_clearTimeout),
    JS_CFUNC_DEF("setInterval", 2, js_setInterval),
    JS_CFUNC_DEF("clearInterval", 1, js_clearInterval),

    /* Bruce functions */
    /* Global functions */
    JS_PROP_CLASS_DEF("exports", &js_exports_obj),

    JS_CFUNC_DEF("assert", 2, native_assert ),
    JS_CFUNC_DEF("require", 1, native_require ),
    JS_CFUNC_DEF("now", 0, native_now ),
    JS_CFUNC_DEF("delay", 1, native_delay ),
    JS_CFUNC_DEF("random", 2, native_random ),
    JS_CFUNC_DEF("parse_int", 1, native_parse_int ),
    JS_CFUNC_DEF("to_string", 1, native_to_string ),
    JS_CFUNC_DEF("to_hex_string", 1, native_to_hex_string ),
    JS_CFUNC_DEF("to_lower_case", 1, native_to_lower_case ),
    JS_CFUNC_DEF("to_upper_case", 1, native_to_upper_case ),
    JS_CFUNC_DEF("atob", 1, native_atob ),
    JS_CFUNC_DEF("btoa", 1, native_btoa ),
    JS_CFUNC_DEF("atob_bin", 1, native_atob_bin ),
    JS_CFUNC_DEF("btoa_bin", 1, native_btoa_bin ),
    JS_CFUNC_DEF("exit", 0, native_exit ),

    /* Modules */
    JS_PROP_CLASS_DEF("audio", &js_audio_obj),
    JS_PROP_CLASS_DEF("badusb", &js_badusb_obj),
    JS_PROP_CLASS_DEF("device", &js_device_obj),
    JS_PROP_CLASS_DEF("display", &js_display_obj),
    JS_PROP_CLASS_DEF("dialog", &js_dialog_obj),
    JS_PROP_CLASS_DEF("gpio", &js_gpio_obj),
    JS_PROP_CLASS_DEF("i2c", &js_i2c_obj),
    JS_PROP_CLASS_DEF("ir", &js_ir_obj),
    JS_PROP_CLASS_DEF("keyboard", &js_keyboard_obj),
    JS_PROP_CLASS_DEF("notification", &js_notification_obj),
    JS_PROP_CLASS_DEF("mic", &js_mic_obj),
    JS_PROP_CLASS_DEF("rfid", &js_rfid_obj),
    JS_PROP_CLASS_DEF("runtime", &js_runtime_obj),
    JS_PROP_CLASS_DEF("serial", &js_serial_obj),
    JS_PROP_CLASS_DEF("storage", &js_storage_obj),
    JS_PROP_CLASS_DEF("subghz", &js_subghz_obj),
    JS_PROP_CLASS_DEF("wifi", &js_wifi_obj),

    // MUST BE IN THE SAME ORDER AS IN THE user_classes_js FILE they cannot be guarded by #ifdef LITE_VERSION
    JS_PROP_CLASS_DEF("TimersState", &js_timers_state_class),
    JS_PROP_CLASS_DEF("Sprite", &js_sprite_class),
    JS_PROP_CLASS_DEF("TextViewer", &js_textviewer_class),
    JS_PROP_CLASS_DEF("Gif", &js_gif_class),
    JS_PROP_CLASS_DEF("Buffer", &js_buffer_class),

    JS_PROP_CLASS_DEF("__internal_functions", &js_internal_functions_obj),

    JS_PROP_END,
};

/* Additional C function declarations (only useful for C
   closures). They are always defined first. */
static const JSPropDef js_c_function_decl[] = {
    /* must come first if "bind" is defined */
    JS_CFUNC_SPECIAL_DEF("bound", 0, generic_params, js_function_bound ),

    JS_PROP_END,
};

int main(int argc, char **argv)
{
    return build_atoms("js_stdlib", js_global_object, js_c_function_decl, argc, argv);
}
