/*
MIT License

Copyright (c) 2022 lewis he

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


/*
! WARN:
Please do not run the example without knowing the external load voltage of the PMU,
it may burn your external load, please check the voltage setting before running the example,
if there is any loss, please bear it by yourself
*/
#ifndef XPOWERS_NO_ERROR
#error "Running this example is known to not damage the device! Please go and uncomment this!"
#endif
// Defined using AXP2102
#define XPOWERS_CHIP_AXP2101

#include <Wire.h>
#include <Arduino.h>
#include "XPowersLib.h"

XPowersPMU power;

#ifndef CONFIG_PMU_SDA
#define CONFIG_PMU_SDA 21
#endif

#ifndef CONFIG_PMU_SCL
#define CONFIG_PMU_SCL 22
#endif

#ifndef CONFIG_PMU_IRQ
#define CONFIG_PMU_IRQ 35
#endif

const uint8_t i2c_sda = CONFIG_PMU_SDA;
const uint8_t i2c_scl = CONFIG_PMU_SCL;
const uint8_t pmu_irq_pin = CONFIG_PMU_IRQ;

uint16_t targetVol;
uint16_t vol = 0;


void setup()
{
    Serial.begin(115200);

    bool result = power.begin(Wire, AXP2101_SLAVE_ADDRESS, i2c_sda, i2c_scl);

    if (result == false) {
        Serial.println("power is not online..."); while (1)delay(50);
    }

    Serial.println("AXP2101 Power Output Test.");



    power.disableDC2();
    power.disableDC3();
    power.disableDC4();
    power.disableDC5();
    power.disableALDO1();
    power.disableALDO2();
    power.disableALDO3();
    power.disableALDO4();
    power.disableBLDO1();
    power.disableBLDO2();
    power.disableCPUSLDO();
    power.disableDLDO1();
    power.disableDLDO2();


    // DC1 IMAX=2A
    // 1500~3400mV,100mV/step,20steps
    vol = 1500;
    for (int i = 0; i < 20; ++i) {
        power.setDC1Voltage(vol);
        vol += 100;
        Serial.printf("DC1  :%s   Voltage:%u mV \n",  power.isEnableDC1()  ? "ENABLE" : "DISABLE", power.getDC1Voltage());
    }


    // DC2 IMAX=2A
    // 500~1200mV  10mV/step,71steps
    vol = 500;
    for (int i = 0; i < 71; ++i) {
        power.setDC2Voltage(vol);
        delay(1);
        targetVol = power.getDC2Voltage();
        Serial.printf("[%d]DC2  :%s   Voltage:%u mV \n", i,  power.isEnableDC2()  ? "ENABLE" : "DISABLE", targetVol );
        if (targetVol != vol)Serial.println(">>> FAILED!");
        vol += 10;
    }

    // DC2 IMAX=2A
    // 1220~1540mV 20mV/step,17steps
    vol = 1220;
    for (int i = 0; i < 17; ++i) {
        power.setDC2Voltage(vol);
        delay(1);
        targetVol = power.getDC2Voltage();
        Serial.printf("[%u]DC2  :%s   Voltage:%u mV \n", i,  power.isEnableDC2()  ? "ENABLE" : "DISABLE", targetVol );
        if (targetVol != vol)Serial.println(">>> FAILED!");
        vol += 20;
    }

    // DC3 IMAX = 2A
    // 500~1200mV,10mV/step,71steps
    vol = 500;
    for (int i = 0; i < 71; ++i) {
        power.setDC3Voltage(vol);
        delay(1);
        targetVol = power.getDC3Voltage();
        Serial.printf("[%u]DC3  :%s   Voltage:%u mV \n", i,  power.isEnableDC3()  ? "ENABLE" : "DISABLE", targetVol );
        if (targetVol != vol)Serial.println(">>> FAILED!");
        vol += 10;
    }

    // DC3 IMAX = 2A
    // 1220~1540mV,20mV/step,17steps
    vol = 1220;
    for (int i = 0; i < 17; ++i) {
        power.setDC3Voltage(vol);
        delay(1);
        targetVol = power.getDC3Voltage();
        Serial.printf("[%u]DC3  :%s   Voltage:%u mV \n", i,  power.isEnableDC3()  ? "ENABLE" : "DISABLE", targetVol );
        if (targetVol != vol)Serial.println(">>> FAILED!");
        vol += 20;
    }


    // DC3 IMAX = 2A
    // 1600~3400mV,100mV/step,19steps
    vol = 1600;
    for (int i = 0; i < 19; ++i) {
        power.setDC3Voltage(vol);
        delay(1);
        targetVol = power.getDC3Voltage();
        Serial.printf("[%u]DC3  :%s   Voltage:%u mV \n", i,  power.isEnableDC3()  ? "ENABLE" : "DISABLE", targetVol );
        if (targetVol != vol)Serial.println(">>> FAILED!");
        vol += 100;
    }


    // DCDC4 IMAX=1.5A
    // 500~1200mV,10mV/step,71steps
    vol = 500;
    for (int i = 0; i < 71; ++i) {
        power.setDC4Voltage(vol);
        delay(1);
        targetVol = power.getDC4Voltage();
        Serial.printf("[%u]DC4  :%s   Voltage:%u mV \n", i,  power.isEnableDC4()  ? "ENABLE" : "DISABLE", targetVol );
        if (targetVol != vol)Serial.println(">>> FAILED!");
        vol += 10;
    }

    // DCDC4 IMAX=1.5A
    // 1220~1840mV,20mV/step,32steps
    vol = 1220;
    for (int i = 0; i < 32; ++i) {
        power.setDC4Voltage(vol);
        delay(1);
        targetVol = power.getDC4Voltage();
        Serial.printf("[%u]DC4  :%s   Voltage:%u mV \n", i,  power.isEnableDC4()  ? "ENABLE" : "DISABLE", targetVol );
        if (targetVol != vol)Serial.println(">>> FAILED!");
        vol += 20;
    }

    // DC5 IMAX=2A
    // 1200mV
    power.setDC5Voltage(1200);
    targetVol = power.getDC5Voltage();
    Serial.printf("[0]DC5  :%s   Voltage:%u mV \n",   power.isEnableDC5()  ? "ENABLE" : "DISABLE", targetVol );


    // DC5 IMAX=2A
    // 1400~3700mV,100mV/step,24steps
    vol = 1400;
    for (int i = 0; i < 24; ++i) {
        power.setDC5Voltage(vol);
        delay(1);
        targetVol = power.getDC5Voltage();
        Serial.printf("[%u]DC5  :%s   Voltage:%u mV \n", i,  power.isEnableDC5()  ? "ENABLE" : "DISABLE", targetVol );
        if (targetVol != vol)Serial.println(">>> FAILED!");
        vol += 100;
    }



    //ALDO1 IMAX=300mA
    //500~3500mV, 100mV/step,31steps
    vol = 500;
    for (int i = 0; i < 31; ++i) {
        power.setALDO1Voltage(vol);
        delay(1);
        targetVol = power.getALDO1Voltage();
        Serial.printf("[%u]ALDO1  :%s   Voltage:%u mV \n", i,  power.isEnableALDO1()  ? "ENABLE" : "DISABLE", targetVol );
        if (targetVol != vol)Serial.println(">>> FAILED!");
        vol += 100;
    }

    //ALDO2 IMAX=300mA
    //500~3500mV, 100mV/step,31steps
    vol = 500;
    for (int i = 0; i < 31; ++i) {
        power.setALDO2Voltage(vol);
        delay(1);
        targetVol = power.getALDO2Voltage();
        Serial.printf("[%u]ALDO2  :%s   Voltage:%u mV \n", i,  power.isEnableALDO2()  ? "ENABLE" : "DISABLE", targetVol );
        if (targetVol != vol)Serial.println(">>> FAILED!");
        vol += 100;
    }

    //ALDO3 IMAX=300mA
    //500~3500mV, 100mV/step,31steps
    vol = 500;
    for (int i = 0; i < 31; ++i) {
        power.setALDO3Voltage(vol);
        delay(1);
        targetVol = power.getALDO3Voltage();
        Serial.printf("[%u]ALDO3  :%s   Voltage:%u mV \n", i,  power.isEnableALDO3()  ? "ENABLE" : "DISABLE", targetVol );
        if (targetVol != vol)Serial.println(">>> FAILED!");
        vol += 100;
    }

    //ALDO4 IMAX=300mA
    //500~3500mV, 100mV/step,31steps
    vol = 500;
    for (int i = 0; i < 31; ++i) {
        power.setALDO4Voltage(vol);
        delay(1);
        targetVol = power.getALDO4Voltage();
        Serial.printf("[%u]ALDO4  :%s   Voltage:%u mV \n", i,  power.isEnableALDO4()  ? "ENABLE" : "DISABLE", targetVol );
        if (targetVol != vol)Serial.println(">>> FAILED!");
        vol += 100;
    }

    //BLDO1 IMAX=300mA
    //500~3500mV, 100mV/step,31steps
    vol = 500;
    for (int i = 0; i < 31; ++i) {
        power.setBLDO1Voltage(vol);
        delay(1);
        targetVol = power.getBLDO1Voltage();
        Serial.printf("[%u]BLDO1  :%s   Voltage:%u mV \n", i,  power.isEnableBLDO1()  ? "ENABLE" : "DISABLE", targetVol );
        if (targetVol != vol)Serial.println(">>> FAILED!");
        vol += 100;
    }

    //BLDO2 IMAX=300mA
    //500~3500mV, 100mV/step,31steps
    vol = 500;
    for (int i = 0; i < 31; ++i) {
        power.setBLDO2Voltage(vol);
        delay(1);
        targetVol = power.getBLDO2Voltage();
        Serial.printf("[%u]BLDO2  :%s   Voltage:%u mV \n", i,  power.isEnableBLDO2()  ? "ENABLE" : "DISABLE", targetVol );
        if (targetVol != vol)Serial.println(">>> FAILED!");
        vol += 100;
    }


    //CPUSLDO IMAX=30mA
    //500~1400mV,50mV/step,19steps
    vol = 500;
    for (int i = 0; i < 19; ++i) {
        power.setCPUSLDOVoltage(vol);
        delay(1);
        targetVol = power.getCPUSLDOVoltage();
        Serial.printf("[%u]CPUSLDO  :%s   Voltage:%u mV \n", i,  power.isEnableCPUSLDO()  ? "ENABLE" : "DISABLE", targetVol );
        if (targetVol != vol)Serial.println(">>> FAILED!");
        vol += 50;
    }

    //DLDO1 IMAX=300mA
    //500~3400mV, 100mV/step,29steps
    vol = 500;
    for (int i = 0; i < 29; ++i) {
        power.setDLDO1Voltage(vol);
        delay(1);
        targetVol = power.getDLDO1Voltage();
        Serial.printf("[%u]DLDO1  :%s   Voltage:%u mV \n", i,  power.isEnableDLDO1()  ? "ENABLE" : "DISABLE", targetVol );
        if (targetVol != vol)Serial.println(">>> FAILED!");
        vol += 100;
    }

    //DLDO2 IMAX=300mA
    //500~1400mV, 50mV/step,2steps
    vol = 500;
    for (int i = 0; i < 29; ++i) {
        power.setDLDO2Voltage(vol);
        delay(1);
        targetVol = power.getDLDO2Voltage();
        Serial.printf("[%u]DLDO2  :%s   Voltage:%u mV \n", i,  power.isEnableDLDO2()  ? "ENABLE" : "DISABLE", targetVol );
        if (targetVol != vol)Serial.println(">>> FAILED!");
        vol += 100;
    }


    /*
    ! WARN:
    Please do not run the example without knowing the external load voltage of the PMU,
    it may burn your external load, please check the voltage setting before running the example,
    if there is any loss, please bear it by yourself
    */

    // power.enableDC1();
    // power.enableDC2();
    // power.enableDC3();
    // power.enableDC4();
    // power.enableDC5();

    // power.enableALDO1();
    // power.enableALDO2();
    // power.enableALDO3();
    // power.enableALDO4();


    // power.enableBLDO1();
    // power.enableBLDO2();

    // power.enableCPUSLDO();

    // power.enableDLDO1();
    // power.enableDLDO2();


}

void loop()
{
    delay(10);
}

