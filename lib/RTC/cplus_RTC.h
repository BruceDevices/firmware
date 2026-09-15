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

class cplus_RTC {
public:
    // cplus_RTC();

    void begin(void);
    void setWire(TwoWire *obj) { wr = obj; }
    void GetBm8563Time(void);

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
    TwoWire *wr = &Wire1; // Default I2C bus for StickCPluses
    void Bcd2asc(void);
    void DataMask();
    void Str2Time(void);

    uint8_t Bcd2ToByte(uint8_t Value);
    uint8_t ByteToBcd2(uint8_t Value);

private:
    /*定义数组用来存储读取的时间数据 */
    uint8_t trdata[7];
    /*定义数组用来存储转换的 asc 码时间数据*/
    // uint8_t asc[14];
};
