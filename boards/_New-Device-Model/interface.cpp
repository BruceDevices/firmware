#include "core/powerSave.h"
#include <interface.h>
#include <Arduino.h>

// =====================================================================
// 1. กำหนดขาขยาย (GPIO Mapping) สำหรับ DevKitC V1 30 pin
// =====================================================================
#define BTN_UP     32  // ปุ่มเลื่อนขึ้น / เมนูก่อนหน้า (Prev)
#define BTN_DOWN   33  // ปุ่มเลื่อนลง / เมนูถัดไป (Next)
#define BTN_SELECT 26  // ปุ่มตกลง (Select)
#define BTN_ESC    22  // ปุ่มย้อนกลับ (Exit / Escape)

#define TFT_BL     21  // ขาไฟ Backlight ของหน้าจอ ST7789

// ตั้งค่าแนลสำหรับสร้างสัญญาณ PWM ควบคุมความสว่างจอ
#define PWM_CH     0
#define PWM_FREQ   5000
#define PWM_RES    8

// ฟังก์ชันภายในสแกนสถานะว่า "มีปุ่มใดปุ่มหนึ่งถูกกดอยู่หรือไม่"
bool anyBtnPressed() {
    return (digitalRead(BTN_UP) == LOW || 
            digitalRead(BTN_DOWN) == LOW || 
            digitalRead(BTN_SELECT) == LOW || 
            digitalRead(BTN_ESC) == LOW);
}

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    // ตั้งค่าขาปุ่มกดทั้งหมดเป็น INPUT_PULLUP (Active LOW)
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_SELECT, INPUT_PULLUP);
    pinMode(BTN_ESC, INPUT_PULLUP);

    // กำหนดค่าสัญญาณ PWM ควบคุม Backlight ผ่าน LEDC ของ ESP32
    ledcSetup(PWM_CH, PWM_FREQ, PWM_RES);
    ledcAttachPin(TFT_BL, PWM_CH);
    
    // เปิดความสว่างสูงสุด (255) ไว้ก่อนเมื่อเริ่มบูตเครื่อง
    ledcWrite(PWM_CH, 255); 
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {
    // ปล่อยว่างไว้ได้ (ใช้กรณีต้องการเปิดวงจรส่วนอื่นหลังจากระบบหลักโหลดเสร็จแล้ว)
}

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() { 
    // คืนค่าเป็น 100% เสมอ เนื่องจาก DevKitC แบบเพียวๆ ไม่มีวงจรไอซี Fuel Gauge วัดแบตเตอรี่
    // เพื่อป้องกันระบบขึ้นเตือน Low Battery วนลูป
    return 100; 
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    // สั่งปรับระดับความสว่างหน้าจอผ่านค่า PWM (รับค่าแปรผันจากเมนูการตั้งค่าของ Bruce ตั้งแต่ 0 - 255)
    ledcWrite(PWM_CH, brightval);
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    checkPowerSaveTime();
    PrevPress = false;
    NextPress = false;
    SelPress = false;
    AnyKeyPress = false;
    EscPress = false;

    // 1. ตรวจสอบว่ามีการกดปุ่มใดๆ เกิดขึ้นหรือไม่
    if (anyBtnPressed()) { 
        if (!wakeUpScreen()) {
            AnyKeyPress = true;
        } else {
            goto END; // ถ้าเป็นกรณีหน้าจอดับแล้วตื่นขึ้นมา ให้ข้ามการกดฟังก์ชันอื่นไปก่อน
        }
    }
    
    // 2. แยกแยะเงื่อนไขการกดสวิตช์รายตัว
    if (digitalRead(BTN_UP) == LOW)     { PrevPress = true; }
    if (digitalRead(BTN_DOWN) == LOW)   { NextPress = true; }
    if (digitalRead(BTN_ESC) == LOW)    { EscPress = true; }
    if (digitalRead(BTN_SELECT) == LOW) { SelPress = true; }

END:
    // 3. ดีเด้นซ์ปุ่มกด (Debounce) ป้องกันสัญญาณรบกวนหรือสถานะการกดเบิ้ล
    if (AnyKeyPress || PrevPress || NextPress || EscPress || SelPress) {
        long tmp = millis();
        // รอจนกว่าจะปล่อยปุ่มทุกปุ่ม หรือจำกัดเวลาหน่วงไว้ไม่เกิน 200ms
        while ((millis() - tmp) < 200 && anyBtnPressed());
    }
}

/*********************************************************************
** Function: keyboard
** location: mykeyboard.cpp
** Starts keyboard to type data
**********************************************************************/
String keyboard(String mytext, int maxSize, String msg) {
    // คืนค่าข้อความเดิมกลับออกไป 
    // โครงสร้างซอฟต์แวร์คีย์บอร์ดแบบเสมือน (Virtual Screen Keyboard) จะถูกควบคุมผ่านหน้าจอหลักโดยอัตโนมัติ
    return mytext;
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {
    // ดับไฟหน้าจอ ST7789
    ledcWrite(PWM_CH, 0);
    
    // สั่งให้ ESP32 สแตนด์บายรอสัญญาณตื่น (Wakeup) จากปุ่ม SELECT (GPIO 26) เมื่อถูกกดลงกราวด์ (LOW)
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_SELECT, 0);
    
    // เข้าสู่โหมด Deep Sleep เพื่อจำลองการปิดเครื่อง
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to turn off the device (name is odd btw)
**********************************************************************/
void checkReboot() {
    // สามารถใส่เงื่อนไขเพิ่มเติมได้ เช่น ถ้ากดปุ่ม ESC ค้างไว้เกิน 3 วินาที ให้สั่ง ESP.restart();
}
