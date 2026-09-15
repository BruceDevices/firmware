var display = require('display');
var keyboardApi = require('keyboard');
var dialog = require('dialog');
var ir = require('ir');

var fillScreen = display.fill;
var drawString = display.drawString;
var getAnyPress = keyboardApi.getAnyPress;
var keyboardPrompt = keyboardApi.keyboard;
var irTransmit = ir.transmit;
var dialogChoice = dialog.choice;
var dialogError = dialog.error;
var color = display.color;
// bruteforce IR signals
// use at your own risk, some devices may lock you out as a security mechanism when spammed with this!

var value_prefix = 0x20DF0000; // enter a fixed value to use a prefix, if you know it
var no_bits = 16;  // 2-bytes range -> 2^16=65536 values to try
var delay_ms = 200;  // delay after each try
var protocol = "NEC";  // one of the most common protocols


function brute_force() {
    var max_val = value_prefix + (1 << no_bits);

    for (var brute_val = value_prefix; brute_val < max_val; brute_val++) {
        fillScreen(0);
        var curr_val = "0x" + brute_val.toString(16).toUpperCase();

        drawString("sending", 3, 0);
        drawString(curr_val, 3, 16);
        drawString("hold any key to stop", 3, 32);

        if (getAnyPress()) break;

        // example full cmd: IRSend {"Protocol":"NEC","Bits":32,"Data":"0x20DF10EF"}
        //serialCmd("IRSend {'Protocol':'" + protocol + "','Bits':32,'Data':'" + curr_val + "'}");


        var r = irTransmit(curr_val, protocol, 32);
        if (!r) drawString("ERROR: check serial log", 3, 64);

        delay(delay_ms);
    }
}


while (true) {
    var menu_options = {};
    menu_options["Init value:" + value_prefix] = "value_prefix";
    menu_options["Bits to iterate:" + no_bits] = "no_bits";
    menu_options["Delay (ms):" + delay_ms] = "delay_ms";
    menu_options["Protocol:" + protocol] = "protocol";
    menu_options["Start attack"] = "attack";

    var choice = dialogChoice(menu_options);

    if (choice == "") break;  // quit
    else if (choice == "value_prefix") value_prefix = parseInt(keyboardPrompt(String(value_prefix), 32, "starting value"));
    else if (choice == "no_bits") no_bits = parseInt(keyboardPrompt(String(no_bits), 32, "bits to iterate"));
    else if (choice == "delay_ms") delay_ms = parseInt(keyboardPrompt(String(delay_ms), 32, "delay afear each try (in ms)"));
    else if (choice == "protocol") protocol = keyboardPrompt(protocol, 32, "Protocol");
    else if (choice == "attack") {
        if (!value_prefix || !no_bits || !delay_ms || !protocol) {
            dialogError("invalid params");
            continue;
        }
        brute_force();
    }

    fillScreen(0); // clear screen
}
