#include "clicker.h"
#include "core/display.h"
#include "globals.h"
#include <USB.h>
// TODO: fix BLE Mouse for cplus1 on launcher, uses all FLASH
// #include <BleMouse.h>
#include "core/mykeyboard.h"

#ifdef USB_as_HID
#include <USBHIDMouse.h>
USBHIDMouse Mouse;
// BleMouse bleMouse;

unsigned long prevMillisec = 0;
unsigned long currMillisec = 0;
int clickCount = 0, delayValueInt;

void clicker_setup() {
    String delayValue = num_keyboard("100", 4, "Delay between click(ms)");
    delayValueInt = atoi(delayValue.c_str());

    usbClickerSetup();
    /*
    options = {
      {"USB", [=]() { usbClickerSetup();}},
      {"BLE", [=]() { bleClickerSetup();}},
    };
    delay(200);
    loopOptions(options, MENU_TYPE_SUBMENU, "");
    delay(200);
    */
}

void usbClickerSetup() {
    USB.begin();
    Mouse.begin();

    tft.fillScreen(bruceConfig.bgColor);
    tft.setCursor(0, 0);
    tft.print("CPS");

    for (;;) {
        Mouse.click(MOUSE_LEFT);
        clickCount++;
        currMillisec = millis();
        if (currMillisec - prevMillisec >= 1000) {
            Serial.println(clickCount);
            displayTextLine(String(clickCount));
            clickCount = 0;
            prevMillisec = currMillisec;
        }
        delay(delayValueInt);
        if (check(EscPress) || returnToMenu) break;
    }
}
#endif

/*
void bleClickerSetup(){
  bleMouse.begin();
  tft.fillScreen(bruceConfig.bgColor);
  tft.setCursor(0,0);
  displayTextLine("Waiting to pair");

  for(;;){
     if(bleMouse.isConnected()) {
       bleMouse.click(MOUSE_LEFT);
       clickCount++;
       currMillisec = millis();
       if (currMillisec - prevMillisec >= 1000) {
         Serial.println(clickCount);
         displayTextLine(String(clickCount));
         clickCount = 0;
         prevMillisec = currMillisec;
       }
     }
     delay(delayValueInt);
     if (check(EscPress) || returnToMenu) break ;
  }
}
*/
