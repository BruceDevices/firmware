var display = require('display');
var keyboard = require('keyboard');
var ir = require('ir');
var serial = require('serial');
var badusb = require('badusb');

var fillScreen = display.fill;
var drawString = display.drawString;
var print = display.print;
var getAnyPress = keyboard.getAnyPress;
var serialReadln = serial.readln;
var irRead = ir.read;
var badusbSetup = badusb.setup;
var badusbPress = badusb.press;
var badusbHold = badusb.hold;
var badusbRelease = badusb.release;
var badusbReleaseAll = badusb.releaseAll;
var badusbPrint = badusb.print;
var badusbPrintln = badusb.println;
var badusbPressRaw = badusb.pressRaw;
var badusbRunFile = badusb.runFile;

// flirc-like IR2Keyboard script
// use any IR remote to send commands to your pc
// requires: IR receiver + USBHID-capable board

var running=true;
badusbSetup();

while(true) {    
    fillScreen(0);
    drawString("waiting for IR signals...", 3 , 0);
    print("waiting for IR signals...", 0 , 0);
    
    if(getAnyPress()) break;
    var cmd = serialReadln(1);
    if(cmd.trim()!="") break;
    
    var curr_ir_signal = irRead(1); // 1s timeout
    
    drawString(curr_ir_signal, 3 , 16);
    drawString("hold any key to stop", 3 , 32);
    
    print("received:");
    print(curr_ir_signal);
    
    if(curr_ir_signal) {

        delay(delay_ms);
        fillScreen(0);
        
        // switch on curr_ir_signal
        //  https://github.com/espressif/arduino-esp32/blob/master/libraries/USB/src/USBHIDConsumerControl.h
        if(curr_ir_signal.includes("address: 04 00 00 00\ncommand: B0 00 00 00")) {  // CUSTOMIZE FOR YOUR REMOTE
            print("pressed play");
            //badusbPress(0x20);  // KEY_SPACE
            badusbPress(0x2c);  // KEY_SPACE
            //badusbPressSpecial(0x00CD);  // HID_USAGE_CONSUMER_PLAY_PAUSE
        }
        else if(curr_ir_signal.includes("address: 04 00 00 00\ncommand: 8E 00 00 00")) {  // CUSTOMIZE FOR YOUR REMOTE
            print("pressed next");
            badusbPrint("n");  // vlc next shortcut
            badusbPressRaw(0x11);  // HID_KEY_N
            //badusbPressSpecial(0x00B5); // HID_USAGE_CONSUMER_SCAN_NEXT
        }
        else if(curr_ir_signal.includes("address: 04 00 00 00\ncommand: 8F 00 00 00")) {  // CUSTOMIZE FOR YOUR REMOTE
            print("pressed prev");
            badusbPrint("p");  // vlc next shortcut
            badusbPressRaw(0x13);  // HID_KEY_P
            //badusbPressSpecial(0x00B6);  // HID_USAGE_CONSUMER_SCAN_PREVIOUS
        }
        else if(curr_ir_signal.includes("address: 04 00 00 00\ncommand: 8F 00 00 00")) {  // CUSTOMIZE FOR YOUR REMOTE
            print("pressed stop");
            //badusbPressSpecial(0x00B7);  // HID_USAGE_CONSUMER_STOP
        }
        else
            print("unknown button pressed");
    }
}
