#include <stdio.h>
#include <cstring>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"

#ifdef CONFIG_XPOWERS_CHIP_AXP192

#define XPOWERS_CHIP_AXP192
#include "XPowersLib.h"
static const char *TAG = "AXP192";

XPowersPMU power;

extern int pmu_register_read(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len);
extern int pmu_register_write_byte(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len);


esp_err_t pmu_init()
{
    //* Implemented using read and write callback methods, applicable to other platforms
#if CONFIG_I2C_COMMUNICATION_METHOD_CALLBACK_RW
    ESP_LOGI(TAG, "Implemented using read and write callback methods");
    if (power.begin(AXP192_SLAVE_ADDRESS, pmu_register_read, pmu_register_write_byte)) {
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

    if (power.begin(bus_handle, AXP192_SLAVE_ADDRESS)) {
        ESP_LOGI(TAG, "Init PMU SUCCESS!");
    } else {
        ESP_LOGE(TAG, "Init PMU FAILED!");
        return false;
    }
#else

    ESP_LOGI(TAG, "Implemented using built-in read and write methods (Use lower version < 5.0 API)");

    if (power.begin((i2c_port_t)CONFIG_I2C_MASTER_PORT_NUM, AXP192_SLAVE_ADDRESS, CONFIG_PMU_I2C_SDA, CONFIG_PMU_I2C_SCL)) {
        ESP_LOGI(TAG, "Init PMU SUCCESS!");
    } else {
        ESP_LOGE(TAG, "Init PMU FAILED!");
        return false;
    }
#endif //ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)
#endif //CONFIG_I2C_COMMUNICATION_METHOD_BUILTIN_RW

    ESP_LOGI(TAG, "getID:0x%x", power.getChipID());

    // Set the minimum system operating voltage inside the PMU,
    // below this value will shut down the PMU
    // Range: 2600~3300mV
    power.setSysPowerDownVoltage(2700);

    // Set the minimum common working voltage of the PMU VBUS input,
    // below this value will turn off the PMU
    power.setVbusVoltageLimit(XPOWERS_AXP192_VBUS_VOL_LIM_4V5);

    // Turn off USB input current limit
    power.setVbusCurrentLimit(XPOWERS_AXP192_VBUS_CUR_LIM_OFF);

    // DC1 700~3500mV, IMAX=1.2A
    power.setDC1Voltage(3300);
    ESP_LOGI(TAG, "DC1  :%s   Voltage:%u mV ",  power.isEnableDC1()  ? "ENABLE" : "DISABLE", power.getDC1Voltage());

    // DC2 700~2750mV, IMAX=1.6A;
    power.setDC2Voltage(700);
    ESP_LOGI(TAG, "DC2  :%s   Voltage:%u mV ",  power.isEnableDC2()  ? "ENABLE" : "DISABLE", power.getDC2Voltage());

    // DC3 700~3500mV,IMAX=0.7A;
    power.setDC3Voltage(3300);
    ESP_LOGI(TAG, "DC3  :%s   Voltage:%u mV ",  power.isEnableDC3()  ? "ENABLE" : "DISABLE", power.getDC3Voltage());


    //LDO2 1800~3300V, 100mV/step, IMAX=200mA
    power.setLDO2Voltage(1800);

    //LDO3 1800~3300V, 100mV/step, IMAX=200mA
    power.setLDO3Voltage(1800);

    //LDOio 1800~3300V, 100mV/step, IMAX=50mA
    power.setLDOioVoltage(3300);


    // Enable power output channel
    // power.enableDC1();
    power.enableDC2();
    power.enableDC3();
    power.enableLDO2();
    power.enableLDO3();
    power.enableLDOio();

    ESP_LOGI(TAG, "DCDC=======================================================================\n");
    ESP_LOGI(TAG, "DC1  :%s   Voltage:%u mV \n",  power.isEnableDC1()  ? "ENABLE" : "DISABLE", power.getDC1Voltage());
    ESP_LOGI(TAG, "DC2  :%s   Voltage:%u mV \n",  power.isEnableDC2()  ? "ENABLE" : "DISABLE", power.getDC2Voltage());
    ESP_LOGI(TAG, "DC3  :%s   Voltage:%u mV \n",  power.isEnableDC3()  ? "ENABLE" : "DISABLE", power.getDC3Voltage());
    ESP_LOGI(TAG, "LDO=======================================================================\n");
    ESP_LOGI(TAG, "LDO2: %s   Voltage:%u mV\n",  power.isEnableLDO2()  ? "ENABLE" : "DISABLE", power.getLDO2Voltage());
    ESP_LOGI(TAG, "LDO3: %s   Voltage:%u mV\n",  power.isEnableLDO3()  ? "ENABLE" : "DISABLE", power.getLDO3Voltage());
    ESP_LOGI(TAG, "LDOio: %s   Voltage:%u mV\n",  power.isEnableLDOio()  ? "ENABLE" : "DISABLE", power.getLDOioVoltage());
    ESP_LOGI(TAG, "==========================================================================\n");

    // Set the time of pressing the button to turn off
    power.setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
    uint8_t opt = power.getPowerKeyPressOffTime();
    ESP_LOGI(TAG, "PowerKeyPressOffTime:");
    switch (opt) {
    case XPOWERS_POWEROFF_4S: ESP_LOGI(TAG, "4 Second");
        break;
    case XPOWERS_POWEROFF_6S: ESP_LOGI(TAG, "6 Second");
        break;
    case XPOWERS_POWEROFF_8S: ESP_LOGI(TAG, "8 Second");
        break;
    case XPOWERS_POWEROFF_10S: ESP_LOGI(TAG, "10 Second");
        break;
    default:
        break;
    }
    // Set the button power-on press time
    power.setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);
    opt = power.getPowerKeyPressOnTime();
    ESP_LOGI(TAG, "PowerKeyPressOnTime:");
    switch (opt) {
    case XPOWERS_POWERON_128MS: ESP_LOGI(TAG, "128 Ms");
        break;
    case XPOWERS_POWERON_512MS: ESP_LOGI(TAG, "512 Ms");
        break;
    case XPOWERS_POWERON_1S: ESP_LOGI(TAG, "1 Second");
        break;
    case XPOWERS_POWERON_2S: ESP_LOGI(TAG, "2 Second");
        break;
    default:
        break;
    }

    ESP_LOGI(TAG, "===========================================================================");

    // It is necessary to disable the detection function of the TS pin on the board
    // without the battery temperature detection function, otherwise it will cause abnormal charging
    power.disableTSPinMeasure();

    // power.enableTemperatureMeasure();
    // power.disableTemperatureMeasure();

    // Enable internal ADC detection
    power.enableBattDetection();
    power.enableVbusVoltageMeasure();
    power.enableBattVoltageMeasure();
    power.enableSystemVoltageMeasure();

    /*
      The default setting is CHGLED is automatically controlled by the PMU.
    - XPOWERS_CHG_LED_OFF,
    - XPOWERS_CHG_LED_BLINK_1HZ,
    - XPOWERS_CHG_LED_BLINK_4HZ,
    - XPOWERS_CHG_LED_ON,
    - XPOWERS_CHG_LED_CTRL_CHG,
    * */
    power.setChargingLedMode(XPOWERS_CHG_LED_OFF);

    // Disable all interrupts
    power.disableIRQ(XPOWERS_AXP192_ALL_IRQ);
    // Clear all interrupt flags
    power.clearIrqStatus();
    // Enable the required interrupt function
    power.enableIRQ(
        XPOWERS_AXP192_BAT_INSERT_IRQ    | XPOWERS_AXP192_BAT_REMOVE_IRQ      |   //BATTERY
        XPOWERS_AXP192_VBUS_INSERT_IRQ   | XPOWERS_AXP192_VBUS_REMOVE_IRQ     |   //VBUS
        XPOWERS_AXP192_PKEY_SHORT_IRQ    | XPOWERS_AXP192_PKEY_LONG_IRQ       |   //POWER KEY
        XPOWERS_AXP192_BAT_CHG_DONE_IRQ  | XPOWERS_AXP192_BAT_CHG_START_IRQ   |    //CHARGE
        // XPOWERS_AXP192_PKEY_NEGATIVE_IRQ | XPOWERS_AXP192_PKEY_POSITIVE_IRQ   |   //POWER KEY
        XPOWERS_AXP192_TIMER_TIMEOUT_IRQ               //Timer
    );

    // Set constant current charge current limit
    power.setChargerConstantCurr(XPOWERS_AXP192_CHG_CUR_280MA);
    // Set stop charging termination current
    power.setChargerTerminationCurr(XPOWERS_AXP192_CHG_ITERM_LESS_10_PERCENT);

    // Set charge cut-off voltage
    power.setChargeTargetVoltage(XPOWERS_AXP192_CHG_VOL_4V2);

    // Cache writes and reads, as long as the PMU remains powered, the data will always be stored inside the PMU
    ESP_LOGI(TAG, "Write pmu data buffer .");
    uint8_t data[XPOWERS_AXP192_DATA_BUFFER_SIZE] = {1, 2, 3, 4, 5, 6};
    power.writeDataBuffer(data, XPOWERS_AXP192_DATA_BUFFER_SIZE);
    memset(data, 0, XPOWERS_AXP192_DATA_BUFFER_SIZE);

    ESP_LOGI(TAG, "Read pmu data buffer :");
    power.readDataBuffer(data, XPOWERS_AXP192_DATA_BUFFER_SIZE);
    ESP_LOG_BUFFER_HEX(TAG, data, XPOWERS_AXP192_DATA_BUFFER_SIZE);

    // Set the timing after one minute, the isWdtExpireIrq will be triggered in the loop interrupt function
    power.setTimerout(1);

    return ESP_OK;
}


void pmu_isr_handler()
{
    // Get PMU Interrupt Status Register
    power.getIrqStatus();

    if (power.isAcinOverVoltageIrq()) {
        ESP_LOGI(TAG, "isAcinOverVoltageIrq");
    }
    if (power.isAcinInserIrq()) {
        ESP_LOGI(TAG, "isAcinInserIrq");
    }
    if (power.isAcinRemoveIrq()) {
        ESP_LOGI(TAG, "isAcinRemoveIrq");
    }
    if (power.isVbusOverVoltageIrq()) {
        ESP_LOGI(TAG, "isVbusOverVoltageIrq");
    }
    if (power.isVbusInsertIrq()) {
        ESP_LOGI(TAG, "isVbusInsertIrq");
    }
    if (power.isVbusRemoveIrq()) {
        ESP_LOGI(TAG, "isVbusRemoveIrq");
    }
    if (power.isVbusLowVholdIrq()) {
        ESP_LOGI(TAG, "isVbusLowVholdIrq");
    }
    if (power.isBatInsertIrq()) {
        ESP_LOGI(TAG, "isBatInsertIrq");
    }
    if (power.isBatRemoveIrq()) {
        ESP_LOGI(TAG, "isBatRemoveIrq");
    }
    if (power.isBattEnterActivateIrq()) {
        ESP_LOGI(TAG, "isBattEnterActivateIrq");
    }
    if (power.isBattExitActivateIrq()) {
        ESP_LOGI(TAG, "isBattExitActivateIrq");
    }
    if (power.isBatChargeStartIrq()) {
        ESP_LOGI(TAG, "isBatChargeStartIrq");
    }
    if (power.isBatChargeDoneIrq()) {
        ESP_LOGI(TAG, "isBatChargeDoneIrq");
    }
    if (power.isBattTempHighIrq()) {
        ESP_LOGI(TAG, "isBattTempHighIrq");
    }
    if (power.isBattTempLowIrq()) {
        ESP_LOGI(TAG, "isBattTempLowIrq");
    }
    if (power.isChipOverTemperatureIrq()) {
        ESP_LOGI(TAG, "isChipOverTemperatureIrq");
    }
    if (power.isChargingCurrentLessIrq()) {
        ESP_LOGI(TAG, "isChargingCurrentLessIrq");
    }
    if (power.isDC1VoltageLessIrq()) {
        ESP_LOGI(TAG, "isDC1VoltageLessIrq");
    }
    if (power.isDC2VoltageLessIrq()) {
        ESP_LOGI(TAG, "isDC2VoltageLessIrq");
    }
    if (power.isDC3VoltageLessIrq()) {
        ESP_LOGI(TAG, "isDC3VoltageLessIrq");
    }
    if (power.isPekeyShortPressIrq()) {
        ESP_LOGI(TAG, "isPekeyShortPress");
        // enterPmuSleep();
    }
    if (power.isPekeyLongPressIrq()) {
        ESP_LOGI(TAG, "isPekeyLongPress");

    }
    if (power.isNOEPowerOnIrq()) {
        ESP_LOGI(TAG, "isNOEPowerOnIrq");
    }
    if (power.isNOEPowerDownIrq()) {
        ESP_LOGI(TAG, "isNOEPowerDownIrq");
    }
    if (power.isVbusEffectiveIrq()) {
        ESP_LOGI(TAG, "isVbusEffectiveIrq");
    }
    if (power.isVbusInvalidIrq()) {
        ESP_LOGI(TAG, "isVbusInvalidIrq");
    }
    if (power.isVbusSessionIrq()) {
        ESP_LOGI(TAG, "isVbusSessionIrq");
    }
    if (power.isVbusSessionEndIrq()) {
        ESP_LOGI(TAG, "isVbusSessionEndIrq");
    }
    if (power.isLowVoltageLevel2Irq()) {
        ESP_LOGI(TAG, "isLowVoltageLevel2Irq");
    }
    if (power.isWdtExpireIrq()) {
        ESP_LOGI(TAG, "isWdtExpire");
        // Clear the timer state and continue to the next timer
        power.clearTimerFlag();
    }
    if (power.isGpio2EdgeTriggerIrq()) {
        ESP_LOGI(TAG, "isGpio2EdgeTriggerIrq");
    }
    if (power.isGpio1EdgeTriggerIrq()) {
        ESP_LOGI(TAG, "isGpio1EdgeTriggerIrq");
    }
    if (power.isGpio0EdgeTriggerIrq()) {
        ESP_LOGI(TAG, "isGpio0EdgeTriggerIrq");
    }
    // Clear PMU Interrupt Status Register
    power.clearIrqStatus();
}
#endif /*CONFIG_XPOWERS_AXP192_CHIP_AXP192*/


