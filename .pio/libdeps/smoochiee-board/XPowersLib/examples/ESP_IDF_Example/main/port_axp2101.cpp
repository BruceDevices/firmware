#include <stdio.h>
#include <cstring>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"

#ifdef CONFIG_XPOWERS_CHIP_AXP2101

#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"
static const char *TAG = "AXP2101";

static XPowersPMU power;

extern int pmu_register_read(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len);
extern int pmu_register_write_byte(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len);


esp_err_t pmu_init()
{
    //* Implemented using read and write callback methods, applicable to other platforms
#if CONFIG_I2C_COMMUNICATION_METHOD_CALLBACK_RW
    ESP_LOGI(TAG, "Implemented using read and write callback methods");
    if (power.begin(AXP2101_SLAVE_ADDRESS, pmu_register_read, pmu_register_write_byte)) {
        ESP_LOGI(TAG, "Init PMU SUCCESS!");
    } else {
        ESP_LOGE(TAG, "Init PMU FAILED!");
        return ESP_FAIL;
    }
#endif

    //* Use the built-in esp-idf communication method
#if CONFIG_I2C_COMMUNICATION_METHOD_BUILTIN_RW
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)) && defined(CONFIG_XPOWERS_ESP_IDF_NEW_API)

    ESP_LOGI(TAG, "Implemented using built-in read and write methods (Use higher version >= 5.0 API)");

    // * Using the new API of esp-idf 5.x, you need to pass the I2C BUS handle,
    // * which is useful when the bus shares multiple devices.
    extern i2c_master_bus_handle_t bus_handle;

    if (power.begin(bus_handle, AXP2101_SLAVE_ADDRESS)) {
        ESP_LOGI(TAG, "Init PMU SUCCESS!");
    } else {
        ESP_LOGE(TAG, "Init PMU FAILED!");
        return false;
    }
#else

    ESP_LOGI(TAG, "Implemented using built-in read and write methods (Use lower version < 5.0 API)");

    if (power.begin((i2c_port_t)CONFIG_I2C_MASTER_PORT_NUM, AXP2101_SLAVE_ADDRESS, CONFIG_PMU_I2C_SDA, CONFIG_PMU_I2C_SCL)) {
        ESP_LOGI(TAG, "Init PMU SUCCESS!");
    } else {
        ESP_LOGE(TAG, "Init PMU FAILED!");
        return false;
    }
#endif //ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)
#endif //CONFIG_I2C_COMMUNICATION_METHOD_BUILTIN_RW

    //Turn off not use power channel
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


    //ESP32s3 Core VDD
    power.setDC3Voltage(3300);
    power.enableDC3();

    //Extern 3.3V VDD
    power.setDC1Voltage(3300);
    power.enableDC1();

    // CAM DVDD  1500~1800
    power.setALDO1Voltage(1800);
    // power.setALDO1Voltage(1500);
    power.enableALDO1();

    // CAM DVDD 2500~2800
    power.setALDO2Voltage(2800);
    power.enableALDO2();

    // CAM AVDD 2800~3000
    power.setALDO4Voltage(3000);
    power.enableALDO4();

    // PIR VDD 3300
    power.setALDO3Voltage(3300);
    power.enableALDO3();

    // OLED VDD 3300
    power.setBLDO1Voltage(3300);
    power.enableBLDO1();

    // MIC VDD 33000
    power.setBLDO2Voltage(3300);
    power.enableBLDO2();

    ESP_LOGI(TAG, "DCDC=======================================================================\n");
    ESP_LOGI(TAG, "DC1  : %s   Voltage:%u mV \n",  power.isEnableDC1()  ? "+" : "-", power.getDC1Voltage());
    ESP_LOGI(TAG, "DC2  : %s   Voltage:%u mV \n",  power.isEnableDC2()  ? "+" : "-", power.getDC2Voltage());
    ESP_LOGI(TAG, "DC3  : %s   Voltage:%u mV \n",  power.isEnableDC3()  ? "+" : "-", power.getDC3Voltage());
    ESP_LOGI(TAG, "DC4  : %s   Voltage:%u mV \n",  power.isEnableDC4()  ? "+" : "-", power.getDC4Voltage());
    ESP_LOGI(TAG, "DC5  : %s   Voltage:%u mV \n",  power.isEnableDC5()  ? "+" : "-", power.getDC5Voltage());
    ESP_LOGI(TAG, "ALDO=======================================================================\n");
    ESP_LOGI(TAG, "ALDO1: %s   Voltage:%u mV\n",  power.isEnableALDO1()  ? "+" : "-", power.getALDO1Voltage());
    ESP_LOGI(TAG, "ALDO2: %s   Voltage:%u mV\n",  power.isEnableALDO2()  ? "+" : "-", power.getALDO2Voltage());
    ESP_LOGI(TAG, "ALDO3: %s   Voltage:%u mV\n",  power.isEnableALDO3()  ? "+" : "-", power.getALDO3Voltage());
    ESP_LOGI(TAG, "ALDO4: %s   Voltage:%u mV\n",  power.isEnableALDO4()  ? "+" : "-", power.getALDO4Voltage());
    ESP_LOGI(TAG, "BLDO=======================================================================\n");
    ESP_LOGI(TAG, "BLDO1: %s   Voltage:%u mV\n",  power.isEnableBLDO1()  ? "+" : "-", power.getBLDO1Voltage());
    ESP_LOGI(TAG, "BLDO2: %s   Voltage:%u mV\n",  power.isEnableBLDO2()  ? "+" : "-", power.getBLDO2Voltage());
    ESP_LOGI(TAG, "CPUSLDO====================================================================\n");
    ESP_LOGI(TAG, "CPUSLDO: %s Voltage:%u mV\n",  power.isEnableCPUSLDO() ? "+" : "-", power.getCPUSLDOVoltage());
    ESP_LOGI(TAG, "DLDO=======================================================================\n");
    ESP_LOGI(TAG, "DLDO1: %s   Voltage:%u mV\n",  power.isEnableDLDO1()  ? "+" : "-", power.getDLDO1Voltage());
    ESP_LOGI(TAG, "DLDO2: %s   Voltage:%u mV\n",  power.isEnableDLDO2()  ? "+" : "-", power.getDLDO2Voltage());
    ESP_LOGI(TAG, "===========================================================================\n");

    power.clearIrqStatus();

    power.enableVbusVoltageMeasure();
    power.enableBattVoltageMeasure();
    power.enableSystemVoltageMeasure();
    power.enableTemperatureMeasure();

    // It is necessary to disable the detection function of the TS pin on the board
    // without the battery temperature detection function, otherwise it will cause abnormal charging
    power.disableTSPinMeasure();

    // Disable all interrupts
    power.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    // Clear all interrupt flags
    power.clearIrqStatus();
    // Enable the required interrupt function
    power.enableIRQ(
        XPOWERS_AXP2101_BAT_INSERT_IRQ    | XPOWERS_AXP2101_BAT_REMOVE_IRQ      |   //BATTERY
        XPOWERS_AXP2101_VBUS_INSERT_IRQ   | XPOWERS_AXP2101_VBUS_REMOVE_IRQ     |   //VBUS
        XPOWERS_AXP2101_PKEY_SHORT_IRQ    | XPOWERS_AXP2101_PKEY_LONG_IRQ       |   //POWER KEY
        XPOWERS_AXP2101_BAT_CHG_DONE_IRQ  | XPOWERS_AXP2101_BAT_CHG_START_IRQ       //CHARGE
        // XPOWERS_AXP2101_PKEY_NEGATIVE_IRQ | XPOWERS_AXP2101_PKEY_POSITIVE_IRQ   |   //POWER KEY
    );

    /*
      The default setting is CHGLED is automatically controlled by the power.
    - XPOWERS_CHG_LED_OFF,
    - XPOWERS_CHG_LED_BLINK_1HZ,
    - XPOWERS_CHG_LED_BLINK_4HZ,
    - XPOWERS_CHG_LED_ON,
    - XPOWERS_CHG_LED_CTRL_CHG,
    * */
    power.setChargingLedMode(XPOWERS_CHG_LED_BLINK_1HZ);

    // Set the precharge charging current
    power.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_50MA);
    // Set constant current charge current limit
    power.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_200MA);
    // Set stop charging termination current
    power.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);

    // Set charge cut-off voltage
    power.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V1);

    // Set the watchdog trigger event type
    // power.setWatchdogConfig(XPOWERS_AXP2101_WDT_IRQ_TO_PIN);
    // Set watchdog timeout
    power.setWatchdogTimeout(XPOWERS_AXP2101_WDT_TIMEOUT_4S);
    // Enable watchdog to trigger interrupt event
    power.enableWatchdog();
    return ESP_OK;
}


void pmu_isr_handler()
{
// Get PMU Interrupt Status Register
    power.getIrqStatus();

    if (power.isDropWarningLevel2Irq()) {
        ESP_LOGI(TAG, "isDropWarningLevel2");
    }
    if (power.isDropWarningLevel1Irq()) {
        ESP_LOGI(TAG, "isDropWarningLevel1");
    }
    if (power.isGaugeWdtTimeoutIrq()) {
        ESP_LOGI(TAG, "isWdtTimeout");
    }
    if (power.isBatChargerOverTemperatureIrq()) {
        ESP_LOGI(TAG, "isBatChargeOverTemperature");
    }
    if (power.isBatWorkOverTemperatureIrq()) {
        ESP_LOGI(TAG, "isBatWorkOverTemperature");
    }
    if (power.isBatWorkUnderTemperatureIrq()) {
        ESP_LOGI(TAG, "isBatWorkUnderTemperature");
    }
    if (power.isVbusInsertIrq()) {
        ESP_LOGI(TAG, "isVbusInsert");
    }
    if (power.isVbusRemoveIrq()) {
        ESP_LOGI(TAG, "isVbusRemove");
    }
    if (power.isBatInsertIrq()) {
        ESP_LOGI(TAG, "isBatInsert");
    }
    if (power.isBatRemoveIrq()) {
        ESP_LOGI(TAG, "isBatRemove");
    }
    if (power.isPekeyShortPressIrq()) {
        ESP_LOGI(TAG, "isPekeyShortPress");
    }
    if (power.isPekeyLongPressIrq()) {
        ESP_LOGI(TAG, "isPekeyLongPress");
    }
    if (power.isPekeyNegativeIrq()) {
        ESP_LOGI(TAG, "isPekeyNegative");
    }
    if (power.isPekeyPositiveIrq()) {
        ESP_LOGI(TAG, "isPekeyPositive");
    }
    if (power.isWdtExpireIrq()) {
        ESP_LOGI(TAG, "isWdtExpire");
    }
    if (power.isLdoOverCurrentIrq()) {
        ESP_LOGI(TAG, "isLdoOverCurrentIrq");
    }
    if (power.isBatfetOverCurrentIrq()) {
        ESP_LOGI(TAG, "isBatfetOverCurrentIrq");
    }
    if (power.isBatChargeDoneIrq()) {
        ESP_LOGI(TAG, "isBatChargeDone");
    }
    if (power.isBatChargeStartIrq()) {
        ESP_LOGI(TAG, "isBatChargeStart");
    }
    if (power.isBatDieOverTemperatureIrq()) {
        ESP_LOGI(TAG, "isBatDieOverTemperature");
    }
    if (power.isChargeOverTimeoutIrq()) {
        ESP_LOGI(TAG, "isChargeOverTimeout");
    }
    if (power.isBatOverVoltageIrq()) {
        ESP_LOGI(TAG, "isBatOverVoltage");
    }
    // Clear PMU Interrupt Status Register
    power.clearIrqStatus();
}

#endif  /*CONFIG_XPOWERS_AXP2101_CHIP_AXP2102*/

