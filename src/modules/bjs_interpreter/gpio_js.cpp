#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "gpio_js.h"

#include "helpers_js.h"

duk_ret_t putPropGPIOFunctions(duk_context *ctx, duk_idx_t obj_idx, uint8_t magic) {
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "pinMode", native_pinMode, 3, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "digitalRead", native_digitalRead, 1, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "analogRead", native_analogRead, 1, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "touchRead", native_touchRead, 1, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "digitalWrite", native_digitalWrite, 2, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "analogWrite", native_analogWrite, 2, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "dacWrite", native_dacWrite, 2, magic); // only pins 25 and 26
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "ledcSetup", native_ledcSetup, 3, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "ledcAttachPin", native_ledcAttachPin, 2, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "ledcWrite", native_ledcWrite, 2, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "pins", native_pins, 0, magic);
    return 0;
}

duk_ret_t registerGPIO(duk_context *ctx) {
    bduk_register_c_lightfunc(ctx, "pinMode", native_pinMode, 2);
    bduk_register_c_lightfunc(ctx, "digitalWrite", native_digitalWrite, 2);
    bduk_register_c_lightfunc(ctx, "analogWrite", native_analogWrite, 2);
    bduk_register_c_lightfunc(ctx, "dacWrite", native_dacWrite, 2); // only pins 25 and 26
    bduk_register_c_lightfunc(ctx, "digitalRead", native_digitalRead, 1);
    bduk_register_c_lightfunc(ctx, "analogRead", native_analogRead, 1);
    bduk_register_c_lightfunc(ctx, "touchRead", native_touchRead, 1);
    bduk_register_c_lightfunc(ctx, "pins", native_pins, 0);
    bduk_register_int(ctx, "HIGH", HIGH);
    bduk_register_int(ctx, "LOW", LOW);
    bduk_register_int(ctx, "INPUT", INPUT);
    bduk_register_int(ctx, "OUTPUT", OUTPUT);
    bduk_register_int(ctx, "PULLUP", PULLUP);
    bduk_register_int(ctx, "INPUT_PULLUP", INPUT_PULLUP);
    bduk_register_int(ctx, "PULLDOWN", PULLDOWN);
    bduk_register_int(ctx, "INPUT_PULLDOWN", INPUT_PULLDOWN);
    return 0;
}

duk_ret_t native_digitalWrite(duk_context *ctx) {
    digitalWrite(duk_to_int(ctx, 0), duk_to_boolean(ctx, 1));
    return 0;
}

duk_ret_t native_analogWrite(duk_context *ctx) {
    analogWrite(duk_to_int(ctx, 0), duk_to_int(ctx, 1));
    return 0;
}

duk_ret_t native_digitalRead(duk_context *ctx) {
    int val = digitalRead(duk_to_int(ctx, 0));
    duk_push_int(ctx, val);
    return 1;
}

duk_ret_t native_analogRead(duk_context *ctx) {
    int val = analogRead(duk_to_int(ctx, 0));
    duk_push_int(ctx, val);
    return 1;
}

duk_ret_t native_touchRead(duk_context *ctx) {
#if SOC_TOUCH_SENSOR_SUPPORTED
    int val = touchRead(duk_to_int(ctx, 0));
    duk_push_int(ctx, val);
    return 1;
#else
    return duk_error(ctx, DUK_ERR_TYPE_ERROR, "%s function not supported on this device", "gpio.touchRead()");
#endif
}

duk_ret_t native_dacWrite(duk_context *ctx) {
#if defined(SOC_DAC_SUPPORTED)
    dacWrite(duk_to_int(ctx, 0), duk_to_int(ctx, 1));
#else
    return duk_error(ctx, DUK_ERR_TYPE_ERROR, "%s function not supported on this device", "gpio.dacWrite()");
#endif
    return 0;
}

duk_ret_t native_ledcSetup(duk_context *ctx) {
    int val = ledcAttach(duk_get_int(ctx, 0), 50, duk_get_int(ctx, 1));
    duk_push_int(ctx, val);

    return 1;
}

duk_ret_t native_ledcAttachPin(duk_context *ctx) {
    ledcAttach(duk_get_int(ctx, 0), 50, duk_get_int(ctx, 1));
    return 0;
}

duk_ret_t native_ledcWrite(duk_context *ctx) {
    ledcWrite(duk_get_int(ctx, 0), duk_get_int(ctx, 1));
    return 0;
}

duk_ret_t native_pinMode(duk_context *ctx) {
    uint8_t pin = 255;
    uint8_t mode = INPUT;

    duk_uint_t arg0Type = duk_get_type_mask(ctx, 0);
    if (arg0Type & DUK_TYPE_MASK_NUMBER) {
        pin = duk_to_int(ctx, 0);
    } else if (arg0Type & DUK_TYPE_MASK_STRING) {
        const char *pinString = duk_to_string(ctx, 0);
        if (pinString[0] == 'G') { pin = atoi(&pinString[1]); }
    }

    if (pin == 255) {
        return duk_error(
            ctx, DUK_ERR_TYPE_ERROR, "%s invalid %d argument: %s", "gpio.init()", 1, duk_to_string(ctx, 0)
        );
    }

    if (arg0Type & DUK_TYPE_MASK_NUMBER) {
        mode = duk_to_int(ctx, 0);
    } else if (arg0Type & DUK_TYPE_MASK_STRING) {
        String modeString = duk_to_string(ctx, 1);
        String pullModeString = duk_to_string(ctx, 2);

        if (modeString == "input" || modeString == "analog") {
            if (pullModeString == "up") {
                mode = INPUT_PULLUP;
            } else if (pullModeString == "down") {
                mode = INPUT_PULLDOWN;
            } else {
                mode = INPUT;
            }
        } else if (modeString.startsWith("output")) {
            mode = OUTPUT;
        }
    }

    pinMode(pin, mode);
    return 0;
}

duk_ret_t native_pins(duk_context *ctx) {
    duk_idx_t obj_idx = duk_push_object(ctx);
    bduk_put_prop(ctx, obj_idx, "grove_sda", duk_push_uint, bruceConfigPins.i2c_bus.sda);
    bduk_put_prop(ctx, obj_idx, "grove_scl", duk_push_uint, bruceConfigPins.i2c_bus.scl);
    bduk_put_prop(ctx, obj_idx, "serial_tx", duk_push_uint, bruceConfigPins.uart_bus.tx);
    bduk_put_prop(ctx, obj_idx, "serial_rx", duk_push_uint, bruceConfigPins.uart_bus.rx);
    bduk_put_prop(ctx, obj_idx, "spi_sck", duk_push_uint, SPI_SCK_PIN);
    bduk_put_prop(ctx, obj_idx, "spi_mosi", duk_push_uint, SPI_MOSI_PIN);
    bduk_put_prop(ctx, obj_idx, "spi_miso", duk_push_uint, SPI_MISO_PIN);
    bduk_put_prop(ctx, obj_idx, "spi_ss", duk_push_uint, SPI_SS_PIN);
    bduk_put_prop(ctx, obj_idx, "ir_tx", duk_push_uint, TXLED);
    bduk_put_prop(ctx, obj_idx, "ir_rx", duk_push_uint, RXLED);

    return 1;
}
#endif
