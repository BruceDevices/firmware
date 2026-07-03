#include "core/powerSave.h"
#include <interface.h>
#include <Arduino.h>

// กำหนดแชนเนลสำหรับควบคุมไฟ Backlight จอด้วย PWM
#define PWM_CH     0
#define PWM_FREQ   5000
#define PWM_RES    8

// ฟังก์ชันภายใน: เช็กว่ามีปุ่มใดปุ่มหนึ่งถูกกดอยู่หรือไม่ (Active LOW)
bool anyBtnPressed() {
    return (!digitalRead(UP_BTN) || !digitalRead(DW_BTN) || 
            !digitalRead(L_BTN)  || !digitalRead(R_BTN)  || !digitalRead(SEL_BTN));
}

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    // 1. เปลี่ยนเป็น INPUT_PULLUP เพื่อเปิดแรงดันอ้างอิงภายในชิป
    pinMode(UP_BTN, INPUT_PULLUP);
    pinMode(SEL_BTN, INPUT_PULLUP);
    pinMode(DW_BTN, INPUT_PULLUP);
    pinMode(R_BTN, INPUT_PULLUP);
    pinMode(L_BTN, INPUT_PULLUP);

    // 2. แก้เป็น 1 เพื่อเปิดคำสั่งกลับสีจอ ST7789 ไม่ให้สีเพี้ยน
    bruceConfig.colorInverted = 1; 
    
    // ทิศทางหน้าจอแนวตั้ง (หากต้องการแนวนอนให้แก้เป็น 1 หรือ 3 ใน platformio.ini)
    bruceConfigPins.rotation = 0; 
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() { 
    // ตั้งค่าขาไฟจอและเปิดใช้งานระบบ PWM (LEDC) ของ ESP32
    ledcSetup(PWM_CH, PWM_FREQ, PWM_RES);
    ledcAttachPin(TFT_BL, PWM_CH);
    ledcWrite(PWM_CH, 255); // เปิดไฟสว่างสุดตอนเริ่มทำงาน
}

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() { 
    return 100; // บอร์ด DevKitC ทั่วไปไม่มีไอซีวัดไฟ ให้ส่ง 100% ไว้เสมอ
}

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    // ปรับระดับไฟหน้าจอจริงตามค่าแปรผัน (0 - 255) จากเมนูตั้งค่าของเฟิร์มแวร์
    ledcWrite(PWM_CH, brightval);
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = millis();
    static unsigned long esc_tm = millis();
    static bool esc_armed = false;
    
    if (!(millis() - tm > 200 || LongPress)) return;

    bool u = digitalRead(UP_BTN);
    bool d = digitalRead(DW_BTN);
    bool r = digitalRead(R_BTN);
    bool l = digitalRead(L_BTN);
    bool s = digitalRead(SEL_BTN);
    
    // ตรวจสอบว่ามีการกดปุ่มใดๆ
    if (!s || !u || !d || !r || !l) {
        tm = millis();
        if (!wakeUpScreen()) AnyKeyPress = true;
        else return;
    }
    
    // คอมโบกดปุ่ม Left + Select พร้อมกันเพื่อทำเป็นปุ่มลัด Esc (ย้อนกลับ)
    if (!l && !s) {
        EscPress = true;
        return;
    }
    
    // กดปุ่ม Left ปกติ (ทำหน้าที่เป็นเมนูก่อนหน้า และจับเวลากดค้างเพื่อเป็น Esc ได้ด้วย)
    if (!l) {
        PrevPress = true;
        if (esc_armed == false) {
            esc_tm = millis();
            esc_armed = true;
        }
    }
    
    if (esc_armed && millis() - esc_tm > 1000) {
        esc_armed = false;
        esc_tm = millis();
        PrevPress = false;
        EscPress = true; // หากกด Left ค้างไว้เกิน 1 วินาทีจะกลายเป็นปุ่ม Esc
    }
    
    if (!r) NextPress = true;
    if (!u) UpPress = true;
    if (!d) DownPress = true;
    if (!s) SelPress = true;

END:
    // แก้ไขระบบ Debounce ป้องกันปุ่มเบิ้ล โดยให้รอจนปล่อยปุ่มจริงหรือหมดเวลา 200ms
    if (AnyKeyPress || PrevPress || NextPress || EscPress || SelPress || UpPress || DownPress) {
        long tmp = millis();
        while ((millis() - tmp) < 200 && anyBtnPressed());
    }
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {
    ledcWrite(PWM_CH, 0); // ดับไฟหน้าจอ
    
    // ตั้งค่าให้บอร์ดตื่นจากการหลับลึก (Deep Sleep) เมื่อกดปุ่ม SEL_BTN (ขาลอจิกเป็น 0)
    esp_sleep_enable_ext0_wakeup((gpio_num_t)SEL_BTN, 0);
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to turn off the device
**********************************************************************/
void checkReboot() {
    // ปล่อยว่างไว้หรือใส่เงื่อนไขรีบูตระบบตามต้องการ
}
