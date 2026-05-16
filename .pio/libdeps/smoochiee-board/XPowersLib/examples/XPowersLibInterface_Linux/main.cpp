/*
MIT License

Copyright (c) 2023 lewis he

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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <fcntl.h>    //define O_RDWR  
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/poll.h>
#include "XPowersLib.h"

#ifndef CONFIG_PMU_IRQ
#define CONFIG_PMU_IRQ 55
#endif

extern int linux_gpio_edge(int pin, int edge);
extern int linux_gpio_direction(int pin, int dir);
extern int linux_gpio_export(int pin);
extern int linux_gpio_unexport(int pin);

const uint8_t pmu_irq_pin = CONFIG_PMU_IRQ;

// Change to the hardware I2C device name you need to use
const char *i2c_device = "/dev/i2c-3";
int     hardware_i2c_fd = -1;


int linux_i2c_read_callback(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len)
{
    uint8_t tmp[1] = {regAddr};
    // Write reg address
    write(hardware_i2c_fd, tmp, 1);
    // Read reg data
    return read(hardware_i2c_fd, data, len);
}

int linux_i2c_write_callback(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len)
{
    uint8_t tmp[1] = {regAddr};
    // Write reg address
    write(hardware_i2c_fd, tmp, 1);
    // Write data buffer
    return write(hardware_i2c_fd, data, len);
}


// Use the XPowersLibInterface standard to use the xpowers API
XPowersLibInterface *PMU = NULL;

void printPMU()
{
    printf("isCharging:%s\n", PMU->isCharging() ? "YES" : "NO");
    printf("isDischarge:%s\n", PMU->isDischarge() ? "YES" : "NO");
    printf("isVbusIn:%s\n", PMU->isVbusIn() ? "YES" : "NO");
    printf("getBattVoltage:%u mV\n", PMU->getBattVoltage());
    printf("getVbusVoltage:%u mV\n", PMU->getVbusVoltage());
    printf("getSystemVoltage:%u mV\n", PMU->getSystemVoltage());

    // The battery percentage may be inaccurate at first use, the PMU will automatically
    // learn the battery curve and will automatically calibrate the battery percentage
    // after a charge and discharge cycle
    if (PMU->isBatteryConnect()) {
        printf("getBatteryPercent:%d%%", PMU->getBatteryPercent());
    }

    printf("\n");
}

int main()
{

    // Open I2C device
    if ((hardware_i2c_fd = open(i2c_device, O_RDWR)) < 0)  {
        perror("Failed to open i2c device.\n");
        exit(1);
    } else {
        printf("open : %s\r\n", i2c_device);
    }

    if (!PMU) {
        if (ioctl(hardware_i2c_fd, I2C_SLAVE, AXP2101_SLAVE_ADDRESS) < 0)  {
            printf("Failed to access bus.\n");
            exit(1);
        }
        PMU = new XPowersAXP2101(AXP2101_SLAVE_ADDRESS, linux_i2c_read_callback, linux_i2c_write_callback);
        if (!PMU->init()) {
            printf("Warning: Failed to find AXP2101 power management\n");
            delete PMU;
            PMU = NULL;
        } else {
            printf("AXP2101 PMU init succeeded, using AXP2101 PMU\n");
        }
    }


    if (!PMU) {
        if (ioctl(hardware_i2c_fd, I2C_SLAVE, AXP192_SLAVE_ADDRESS) < 0)  {
            printf("Failed to access bus.\n");
            exit(1);
        }
        PMU = new XPowersAXP192(AXP192_SLAVE_ADDRESS, linux_i2c_read_callback, linux_i2c_write_callback);
        if (!PMU->init()) {
            printf("Warning: Failed to find AXP192 power management\n");
            delete PMU;
            PMU = NULL;
        } else {
            printf("AXP192 PMU init succeeded, using AXP192 PMU\n");
        }
    }

    if (!PMU) {
        if (ioctl(hardware_i2c_fd, I2C_SLAVE, AXP202_SLAVE_ADDRESS) < 0)  {
            printf("Failed to access bus.\n");
            exit(1);
        }
        PMU = new XPowersAXP202(AXP202_SLAVE_ADDRESS, linux_i2c_read_callback, linux_i2c_write_callback);
        printf("Warning: Failed to find AXP202 power management\n");
        delete PMU;
        PMU = NULL;
    } else {
        printf("AXP202 PMU init succeeded, using AXP202 PMU\n");
    }

    if (!PMU) {
        printf("PMU not detected, please check..\n");
        exit(1);
    }

    //The following AXP192 power supply setting voltage is based on esp32 T-beam
    if (PMU->getChipModel() == XPOWERS_AXP192) {

        // lora radio power channel
        PMU->setPowerChannelVoltage(XPOWERS_LDO2, 3300);
        PMU->enablePowerOutput(XPOWERS_LDO2);


        // oled module power channel,
        // disable it will cause abnormal communication between boot and AXP power supply,
        // do not turn it off
        PMU->setPowerChannelVoltage(XPOWERS_DCDC1, 3300);
        // enable oled power
        PMU->enablePowerOutput(XPOWERS_DCDC1);

        // gnss module power channel
        PMU->setPowerChannelVoltage(XPOWERS_LDO3, 3300);
        // PMU->enablePowerOutput(XPOWERS_LDO3);


        //protected oled power source
        PMU->setProtectedChannel(XPOWERS_DCDC1);
        //protected esp32 power source
        PMU->setProtectedChannel(XPOWERS_DCDC3);

        //disable not use channel
        PMU->disablePowerOutput(XPOWERS_DCDC2);

        //disable all axp chip interrupt
        PMU->disableIRQ(XPOWERS_AXP192_ALL_IRQ);


        //
        /*  Set the constant current charging current of AXP192
            opt:
            XPOWERS_AXP192_CHG_CUR_100MA,
            XPOWERS_AXP192_CHG_CUR_190MA,
            XPOWERS_AXP192_CHG_CUR_280MA,
            XPOWERS_AXP192_CHG_CUR_360MA,
            XPOWERS_AXP192_CHG_CUR_450MA,
            XPOWERS_AXP192_CHG_CUR_550MA,
            XPOWERS_AXP192_CHG_CUR_630MA,
            XPOWERS_AXP192_CHG_CUR_700MA,
            XPOWERS_AXP192_CHG_CUR_780MA,
            XPOWERS_AXP192_CHG_CUR_880MA,
            XPOWERS_AXP192_CHG_CUR_960MA,
            XPOWERS_AXP192_CHG_CUR_1000MA,
            XPOWERS_AXP192_CHG_CUR_1080MA,
            XPOWERS_AXP192_CHG_CUR_1160MA,
            XPOWERS_AXP192_CHG_CUR_1240MA,
            XPOWERS_AXP192_CHG_CUR_1320MA,
        */
        PMU->setChargerConstantCurr(XPOWERS_AXP192_CHG_CUR_550MA);


    }
    // The following AXP202 power supply voltage setting is based on esp32 T-Watch
    else if (PMU->getChipModel() == XPOWERS_AXP202) {

        PMU->disablePowerOutput(XPOWERS_DCDC2); //not elicited

        //Display backlight
        PMU->setPowerChannelVoltage(XPOWERS_LDO2, 3300);
        PMU->enablePowerOutput(XPOWERS_LDO2);

        // Shiled Vdd
        PMU->setPowerChannelVoltage(XPOWERS_LDO3, 3300);
        PMU->enablePowerOutput(XPOWERS_LDO3);

        // S7xG GNSS Vdd
        PMU->setPowerChannelVoltage(XPOWERS_LDO4, 1800);
        PMU->enablePowerOutput(XPOWERS_LDO4);


        //
        /*  Set the constant current charging current of AXP202
            opt:
            XPOWERS_AXP202_CHG_CUR_100MA,
            XPOWERS_AXP202_CHG_CUR_190MA,
            XPOWERS_AXP202_CHG_CUR_280MA,
            XPOWERS_AXP202_CHG_CUR_360MA,
            XPOWERS_AXP202_CHG_CUR_450MA,
            XPOWERS_AXP202_CHG_CUR_550MA,
            XPOWERS_AXP202_CHG_CUR_630MA,
            XPOWERS_AXP202_CHG_CUR_700MA,
            XPOWERS_AXP202_CHG_CUR_780MA,
            XPOWERS_AXP202_CHG_CUR_880MA,
            XPOWERS_AXP202_CHG_CUR_960MA,
            XPOWERS_AXP202_CHG_CUR_1000MA,
            XPOWERS_AXP202_CHG_CUR_1080MA,
            XPOWERS_AXP202_CHG_CUR_1160MA,
            XPOWERS_AXP202_CHG_CUR_1240MA,
            XPOWERS_AXP202_CHG_CUR_1320MA,
        */
        PMU->setChargerConstantCurr(XPOWERS_AXP202_CHG_CUR_550MA);

    }
    // The following AXP192 power supply voltage setting is based on esp32s3 T-beam
    else if (PMU->getChipModel() == XPOWERS_AXP2101) {

        // gnss module power channel
        PMU->setPowerChannelVoltage(XPOWERS_ALDO4, 3300);
        PMU->enablePowerOutput(XPOWERS_ALDO4);

        // lora radio power channel
        PMU->setPowerChannelVoltage(XPOWERS_ALDO3, 3300);
        PMU->enablePowerOutput(XPOWERS_ALDO3);

        // m.2 interface
        PMU->setPowerChannelVoltage(XPOWERS_DCDC3, 3300);
        PMU->enablePowerOutput(XPOWERS_DCDC3);

        // PMU->setPowerChannelVoltage(XPOWERS_DCDC4, 3300);
        // PMU->enablePowerOutput(XPOWERS_DCDC4);

        //not use channel
        PMU->disablePowerOutput(XPOWERS_DCDC2); //not elicited
        PMU->disablePowerOutput(XPOWERS_DCDC5); //not elicited
        PMU->disablePowerOutput(XPOWERS_DLDO1); //Invalid power channel, it does not exist
        PMU->disablePowerOutput(XPOWERS_DLDO2); //Invalid power channel, it does not exist
        PMU->disablePowerOutput(XPOWERS_VBACKUP);

        //disable all axp chip interrupt
        PMU->disableIRQ(XPOWERS_AXP2101_ALL_IRQ);

        /*  Set the constant current charging current of AXP2101
            opt:
            XPOWERS_AXP2101_CHG_CUR_100MA,
            XPOWERS_AXP2101_CHG_CUR_125MA,
            XPOWERS_AXP2101_CHG_CUR_150MA,
            XPOWERS_AXP2101_CHG_CUR_175MA,
            XPOWERS_AXP2101_CHG_CUR_200MA,
            XPOWERS_AXP2101_CHG_CUR_300MA,
            XPOWERS_AXP2101_CHG_CUR_400MA,
            XPOWERS_AXP2101_CHG_CUR_500MA,
            XPOWERS_AXP2101_CHG_CUR_600MA,
            XPOWERS_AXP2101_CHG_CUR_700MA,
            XPOWERS_AXP2101_CHG_CUR_800MA,
            XPOWERS_AXP2101_CHG_CUR_900MA,
            XPOWERS_AXP2101_CHG_CUR_1000MA,
        */
        PMU->setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_500MA);

    }

    printf("=======================================================================\n");
    if (PMU->isChannelAvailable(XPOWERS_DCDC1)) {
        printf("DC1  : %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_DCDC1)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_DCDC1));
    }
    if (PMU->isChannelAvailable(XPOWERS_DCDC2)) {
        printf("DC2  : %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_DCDC2)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_DCDC2));
    }
    if (PMU->isChannelAvailable(XPOWERS_DCDC3)) {
        printf("DC3  : %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_DCDC3)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_DCDC3));
    }
    if (PMU->isChannelAvailable(XPOWERS_DCDC4)) {
        printf("DC4  : %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_DCDC4)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_DCDC4));
    }
    if (PMU->isChannelAvailable(XPOWERS_LDO2)) {
        printf("LDO2 : %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_LDO2)   ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_LDO2));
    }
    if (PMU->isChannelAvailable(XPOWERS_LDO3)) {
        printf("LDO3 : %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_LDO3)   ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_LDO3));
    }
    if (PMU->isChannelAvailable(XPOWERS_LDO4)) {
        printf("LDO4 : %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_LDO4)   ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_LDO4));
    }
    if (PMU->isChannelAvailable(XPOWERS_LDO5)) {
        printf("LDO5 : %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_LDO5)   ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_LDO5));
    }
    if (PMU->isChannelAvailable(XPOWERS_ALDO1)) {
        printf("ALDO1: %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_ALDO1)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_ALDO1));
    }
    if (PMU->isChannelAvailable(XPOWERS_ALDO2)) {
        printf("ALDO2: %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_ALDO2)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_ALDO2));
    }
    if (PMU->isChannelAvailable(XPOWERS_ALDO3)) {
        printf("ALDO3: %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_ALDO3)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_ALDO3));
    }
    if (PMU->isChannelAvailable(XPOWERS_ALDO4)) {
        printf("ALDO4: %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_ALDO4)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_ALDO4));
    }
    if (PMU->isChannelAvailable(XPOWERS_BLDO1)) {
        printf("BLDO1: %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_BLDO1)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_BLDO1));
    }
    if (PMU->isChannelAvailable(XPOWERS_BLDO2)) {
        printf("BLDO2: %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_BLDO2)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_BLDO2));
    }
    printf("=======================================================================\n");


    //Set up the charging voltage, AXP2101/AXP192 4.2V gear is the same
    // XPOWERS_AXP192_CHG_VOL_4V2 = XPOWERS_AXP2101_CHG_VOL_4V2
    PMU->setChargeTargetVoltage(XPOWERS_AXP192_CHG_VOL_4V2);

    // Set VSY off voltage as 2600mV , Adjustment range 2600mV ~ 3300mV
    PMU->setSysPowerDownVoltage(2600);

    // Get the VSYS shutdown voltage
    uint16_t vol = PMU->getSysPowerDownVoltage();
    printf("->  getSysPowerDownVoltage:%u\n", vol);



    // Set the time of pressing the button to turn off
    PMU->setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
    uint8_t opt = PMU->getPowerKeyPressOffTime();
    printf("PowerKeyPressOffTime:");
    switch (opt) {
    case XPOWERS_POWEROFF_4S: printf("4 Second\n");
        break;
    case XPOWERS_POWEROFF_6S: printf("6 Second\n");
        break;
    case XPOWERS_POWEROFF_8S: printf("8 Second\n");
        break;
    case XPOWERS_POWEROFF_10S: printf("10 Second\n");
        break;
    default:
        break;
    }

    // Set the button power-on press time
    PMU->setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);
    opt = PMU->getPowerKeyPressOnTime();
    printf("PowerKeyPressOnTime:");
    switch (opt) {
    case XPOWERS_POWERON_128MS: printf("128 Ms\n");
        break;
    case XPOWERS_POWERON_512MS: printf("512 Ms\n");
        break;
    case XPOWERS_POWERON_1S: printf("1 Second\n");
        break;
    case XPOWERS_POWERON_2S: printf("2 Second\n");
        break;
    default:
        break;
    }

    printf("===========================================================================");

    // It is necessary to disable the detection function of the TS pin on the board
    // without the battery temperature detection function, otherwise it will cause abnormal charging
    PMU->disableTSPinMeasure();

    // Enable internal ADC detection
    PMU->enableBattDetection();
    PMU->enableVbusVoltageMeasure();
    PMU->enableBattVoltageMeasure();
    PMU->enableSystemVoltageMeasure();


    /*
      The default setting is CHGLED is automatically controlled by the PMU.
    - XPOWERS_CHG_LED_OFF,
    - XPOWERS_CHG_LED_BLINK_1HZ,
    - XPOWERS_CHG_LED_BLINK_4HZ,
    - XPOWERS_CHG_LED_ON,
    - XPOWERS_CHG_LED_CTRL_CHG,
    * */
    PMU->setChargingLedMode(XPOWERS_CHG_LED_OFF);


    // Clear all interrupt flags
    PMU->clearIrqStatus();


    /*
    // call specific interrupt request

    uint64_t pmuIrqMask = 0;

    if (PMU->getChipModel() == XPOWERS_AXP192) {

        pmuIrqMask = XPOWERS_AXP192_VBUS_INSERT_IRQ     | XPOWERS_AXP192_VBUS_REMOVE_IRQ |      //BATTERY
                     XPOWERS_AXP192_BAT_INSERT_IRQ      | XPOWERS_AXP192_BAT_REMOVE_IRQ  |      //VBUS
                     XPOWERS_AXP192_PKEY_SHORT_IRQ      | XPOWERS_AXP192_PKEY_LONG_IRQ   |      //POWER KEY
                     XPOWERS_AXP192_BAT_CHG_START_IRQ   | XPOWERS_AXP192_BAT_CHG_DONE_IRQ ;     //CHARGE
    } else if (PMU->getChipModel() == XPOWERS_AXP2101) {

        pmuIrqMask = XPOWERS_AXP2101_BAT_INSERT_IRQ     | XPOWERS_AXP2101_BAT_REMOVE_IRQ      |   //BATTERY
                     XPOWERS_AXP2101_VBUS_INSERT_IRQ    | XPOWERS_AXP2101_VBUS_REMOVE_IRQ     |   //VBUS
                     XPOWERS_AXP2101_PKEY_SHORT_IRQ     | XPOWERS_AXP2101_PKEY_LONG_IRQ       |   //POWER KEY
                     XPOWERS_AXP2101_BAT_CHG_DONE_IRQ   | XPOWERS_AXP2101_BAT_CHG_START_IRQ;      //CHARGE
    }
    // Enable the required interrupt function
    PMU->enableIRQ(pmuIrqMask);

    */

    // Call the interrupt request through the interface class
    PMU->disableInterrupt(XPOWERS_ALL_INT);

    PMU->enableInterrupt(XPOWERS_USB_INSERT_INT |
                         XPOWERS_USB_REMOVE_INT |
                         XPOWERS_BATTERY_INSERT_INT |
                         XPOWERS_BATTERY_REMOVE_INT |
                         XPOWERS_PWR_BTN_CLICK_INT |
                         XPOWERS_CHARGE_START_INT |
                         XPOWERS_CHARGE_DONE_INT);

    // Attach PMU irq to gpio interrupt
    linux_gpio_unexport(pmu_irq_pin);
    linux_gpio_export(pmu_irq_pin);
    linux_gpio_direction(pmu_irq_pin, INPUT);
    linux_gpio_edge(pmu_irq_pin, FALLING);

    struct pollfd fdset[1];
    char buf[64];
    int fd, ret;

    snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", pmu_irq_pin);
    fd = open(buf, O_RDONLY);
    if (fd < 0) {
        printf("Failed to open gpio value for reading!\n");
        return -1;
    }
    fdset[0].fd = fd;
    fdset[0].events = POLLPRI;

    while (1) {
        ret = read(fd, buf, 10);
        if (ret == -1) {
            printf("read failed\n");
        }

        ret = poll(fdset, 1, 0);
        if (ret == -1) {
            printf("poll failed\n");
        }

        if (fdset[0].revents & POLLPRI) {
            ret = lseek(fd, 0, SEEK_SET);
            if (ret == -1) {
                printf("lseek failed\n");
            }

            // Get PMU Interrupt Status Register
            uint32_t status = PMU->getIrqStatus();
            printf("STATUS => HEX:0x%X\n", status);

            if (PMU->isVbusInsertIrq()) {
                printf("isVbusInsert\n");
            }
            if (PMU->isVbusRemoveIrq()) {
                printf("isVbusRemove\n");
            }
            if (PMU->isBatInsertIrq()) {
                printf("isBatInsert\n");
            }
            if (PMU->isBatRemoveIrq()) {
                printf("isBatRemove\n");
            }
            if (PMU->isPekeyShortPressIrq()) {
                printf("isPekeyShortPress\n");
            }
            if (PMU->isPekeyLongPressIrq()) {
                printf("isPekeyLongPress\n");
            }
            if (PMU->isBatChargeDoneIrq()) {
                printf("isBatChargeDone\n");
            }
            if (PMU->isBatChargeStartIrq()) {
                printf("isBatChargeStart\n");
            }
            // Clear PMU Interrupt Status Register
            PMU->clearIrqStatus();
        }
    }
    return 0;
}
