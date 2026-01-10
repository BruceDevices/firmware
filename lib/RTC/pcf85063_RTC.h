#pragma once

#include <Wire.h>
#include <time.h>

typedef struct {
    uint8_t Hours;
    uint8_t Minutes;
    uint8_t Seconds;
} RTC_TimeTypeDef;

typedef struct {
    uint8_t WeekDay;
    uint8_t Month;
    uint8_t Date;
    uint16_t Year;
} RTC_DateTypeDef;

class pcf85063_RTC {
public:
    void begin(void);
    void setWire(TwoWire *obj) { wr = obj; }
    void GetPcf85063Time(void);

    void SetTime(RTC_TimeTypeDef *RTC_TimeStruct);
    void SetDate(RTC_DateTypeDef *RTC_DateStruct);

    void GetTime(RTC_TimeTypeDef *RTC_TimeStruct);
    void GetDate(RTC_DateTypeDef *RTC_DateStruct);

    struct tm getTimeStruct();

    void WriteReg(uint8_t reg, uint8_t data);
    uint8_t ReadReg(uint8_t reg);

    int SetAlarmIRQ(int afterSeconds);
    void clearIRQ();
    void disableIRQ();

    // PCF85063 specific functions
    void setClockOutput(
        uint8_t frequency
    ); // 0=32.768kHz, 1=16.384kHz, 2=8.192kHz, 3=4.096kHz, 4=2.048kHz, 5=1.024kHz, 6=1Hz, 7=disabled
    void setBatteryLowDetection(bool enable);
    void reset();

public:
    uint8_t Second;
    uint8_t Minute;
    uint8_t Hour;
    uint8_t Week;
    uint8_t Day;
    uint8_t Month;
    uint8_t Year;
    uint8_t DateString[9];
    uint8_t TimeString[9];

    uint8_t asc[14];

private:
    TwoWire *wr = &Wire;
    void Bcd2asc(void);
    void DataMask();
    void Str2Time(void);

    uint8_t Bcd2ToByte(uint8_t Value);
    uint8_t ByteToBcd2(uint8_t Value);

private:
    // PCF85063 I2C address
    static constexpr uint8_t PCF85063_ADDR = 0x51;

    // PCF85063 Register addresses
    static constexpr uint8_t PCF85063_REG_CTRL1 = 0x00;
    static constexpr uint8_t PCF85063_REG_CTRL2 = 0x01;
    static constexpr uint8_t PCF85063_REG_OFFSET = 0x02;
    static constexpr uint8_t PCF85063_REG_RAM_BYTE = 0x03;
    static constexpr uint8_t PCF85063_REG_SC = 0x04; // Seconds
    static constexpr uint8_t PCF85063_REG_MN = 0x05; // Minutes
    static constexpr uint8_t PCF85063_REG_HR = 0x06; // Hours
    static constexpr uint8_t PCF85063_REG_DM = 0x07; // Day of month
    static constexpr uint8_t PCF85063_REG_DW = 0x08; // Day of week
    static constexpr uint8_t PCF85063_REG_MO = 0x09; // Month
    static constexpr uint8_t PCF85063_REG_YR = 0x0A; // Year
    static constexpr uint8_t PCF85063_REG_SECOND_ALARM = 0x0B;
    static constexpr uint8_t PCF85063_REG_MINUTE_ALARM = 0x0C;
    static constexpr uint8_t PCF85063_REG_HOUR_ALARM = 0x0D;
    static constexpr uint8_t PCF85063_REG_DAY_ALARM = 0x0E;
    static constexpr uint8_t PCF85063_REG_WEEKDAY_ALARM = 0x0F;
    static constexpr uint8_t PCF85063_REG_TIMER_VALUE = 0x10;
    static constexpr uint8_t PCF85063_REG_TIMER_MODE = 0x11;

    /*定义数组用来存储读取的时间数据 */
    uint8_t trdata[7];
};
