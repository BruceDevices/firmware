#include "conditioners.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "custom_ir.h"
#include <globals.h>
#include <ir_Gree.h>

void sendGreeAC(bool power, int temp, int mode) {
    IRGreeAC ac(bruceConfigPins.irTx);
    ac.begin();
    ac.setPower(power);
    ac.setTemp(temp);
    ac.setMode(mode); // 0=Auto, 1=Cool, 2=Dry, 3=Fan, 4=Heat
    ac.setFan(0);     // 0=Auto
    ac.send();
    delay(200); 
}

void conditionersMenu() {
    options = {
        {"ON (Cool 22)",  [=]() { displayRedStripe("Sending ON..."); sendGreeAC(true, 22, 1); displaySuccess("Done!"); }},
        {"ON (Heat 24)",  [=]() { displayRedStripe("Sending HEAT..."); sendGreeAC(true, 24, 4); displaySuccess("Done!"); }},
        {"OFF",           [=]() { displayRedStripe("Sending OFF..."); sendGreeAC(false, 22, 1); displaySuccess("Done!"); }},
        {"Back",          yield}
    };
    loopOptions(options);
}
