#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "device_js.h"
#include <globals.h>

#include "helpers_js.h"

JSValue native_getDeviceName(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    const char *deviceName = bruceConfig.wifiAp.ssid != NULL ? bruceConfig.wifiAp.ssid.c_str() : "Bruce";
    return JS_NewString(ctx, deviceName);
}

JSValue native_getBoard(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if defined(DEVICE_NAME)
    String board = DEVICE_NAME;
#else
    String board = "Undefined";
#endif
    return JS_NewString(ctx, board.c_str());
}

JSValue native_getBruceVersion(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return JS_NewString(ctx, BRUCE_VERSION);
}

JSValue native_getBattery(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int bat = getBattery();
    return JS_NewInt32(ctx, bat);
}

JSValue native_getBatteryDetailed(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "battery_percent", JS_NewInt32(ctx, getBattery()));
#ifdef USE_BQ27220_VIA_I2C
    JS_SetPropertyStr(ctx, obj, "remaining_capacity", JS_NewInt32(ctx, bq.getRemainCap()));
    JS_SetPropertyStr(ctx, obj, "full_capacity", JS_NewInt32(ctx, bq.getFullChargeCap()));
    JS_SetPropertyStr(ctx, obj, "design_capacity", JS_NewInt32(ctx, bq.getDesignCap()));
    JS_SetPropertyStr(ctx, obj, "is_charging", JS_NewBool(bq.getIsCharging()));
    JS_SetPropertyStr(
        ctx,
        obj,
        "charging_voltage",
        JS_NewFloat64(ctx, ((double)bq.getVolt(VOLT_MODE::VOLT_CHARGING) / 1000.0))
    );
    JS_SetPropertyStr(ctx, obj, "charging_current", JS_NewInt32(ctx, bq.getCurr(CURR_MODE::CURR_CHARGING)));
    JS_SetPropertyStr(ctx, obj, "time_to_empty", JS_NewInt32(ctx, bq.getTimeToEmpty()));
    JS_SetPropertyStr(ctx, obj, "average_power_use", JS_NewInt32(ctx, bq.getAvgPower()));
    JS_SetPropertyStr(
        ctx, obj, "voltage", JS_NewFloat64(ctx, ((double)bq.getVolt(VOLT_MODE::VOLT) / 1000.0))
    );
    JS_SetPropertyStr(ctx, obj, "voltage_raw", JS_NewInt32(ctx, bq.getVolt(VOLT_MODE::VOLT_RWA)));
    JS_SetPropertyStr(ctx, obj, "current_instant", JS_NewInt32(ctx, bq.getCurr(CURR_INSTANT)));
    JS_SetPropertyStr(ctx, obj, "current_average", JS_NewInt32(ctx, (bq.getCurr(CURR_MODE::CURR_AVERAGE))));
    JS_SetPropertyStr(ctx, obj, "current_raw", JS_NewInt32(ctx, bq.getVolt(VOLT_MODE::VOLT_RWA)));
#endif
    return obj;
}

JSValue native_getFreeHeapSize(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "ram_free", JS_NewInt32(ctx, info.total_free_bytes));
    JS_SetPropertyStr(ctx, obj, "ram_min_free", JS_NewInt32(ctx, info.minimum_free_bytes));
    JS_SetPropertyStr(ctx, obj, "ram_largest_free_block", JS_NewInt32(ctx, info.largest_free_block));
    JS_SetPropertyStr(ctx, obj, "ram_size", JS_NewInt32(ctx, ESP.getHeapSize()));
    JS_SetPropertyStr(ctx, obj, "psram_free", JS_NewInt32(ctx, ESP.getFreePsram()));
    JS_SetPropertyStr(ctx, obj, "psram_size", JS_NewInt32(ctx, ESP.getPsramSize()));
    JS_SetPropertyStr(ctx, obj, "psram_largest_free_block", JS_NewInt32(ctx, ESP.getMaxAllocPsram()));

    return obj;
}

JSValue native_getEEPROMSize(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return JS_NewInt32(ctx, EEPROMSIZE);
}

#endif
