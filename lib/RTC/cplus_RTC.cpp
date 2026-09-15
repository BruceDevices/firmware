#include "cplus_RTC.h"

// RTC::RTC() {
// }
#ifndef RTC_SDA
#define RTC_SDA 21
#endif
#ifndef RTC_SCL
#define RTC_SCL 22
#endif
void cplus_RTC::begin(void) { wr->begin(RTC_SDA, RTC_SCL); }

void cplus_RTC::GetBm8563Time(void) {
    wr->beginTransmission(0x51);
    wr->write(0x02);
    wr->endTransmission();
    wr->requestFrom(0x51, 7);
    while (wr->available()) {
        trdata[0] = wr->read();
        trdata[1] = wr->read();
        trdata[2] = wr->read();
        trdata[3] = wr->read();
        trdata[4] = wr->read();
        trdata[5] = wr->read();
        trdata[6] = wr->read();
    }

    DataMask();
    Bcd2asc();
    Str2Time();
}

void cplus_RTC::Str2Time(void) {
    Second = (asc[0] - 0x30) * 10 + asc[1] - 0x30;
    Minute = (asc[2] - 0x30) * 10 + asc[3] - 0x30;
    Hour = (asc[4] - 0x30) * 10 + asc[5] - 0x30;
    /*
    uint8_t Hour;
    uint8_t Week;
    uint8_t Day;
    uint8_t Month;
    uint8_t  Year;
    */
}

void cplus_RTC::DataMask() {
    trdata[0] = trdata[0] & 0x7f; // 秒
    trdata[1] = trdata[1] & 0x7f; // 分
    trdata[2] = trdata[2] & 0x3f; // 时

    trdata[3] = trdata[3] & 0x3f; // 日
    trdata[4] = trdata[4] & 0x07; // 星期
    trdata[5] = trdata[5] & 0x1f; // 月

    trdata[6] = trdata[6] & 0xff; // 年
}
/********************************************************************
函 数 名： void Bcd2asc(void)
功 能： bcd 码转换成 asc 码，供Lcd显示用
说 明：
调 用：
入口参数：
返 回 值：无
***********************************************************************/
void cplus_RTC::Bcd2asc(void) {
    uint8_t i, j;
    for (j = 0, i = 0; i < 7; i++) {
        asc[j++] = (trdata[i] & 0xf0) >> 4 | 0x30; /*格式为: 秒 分 时 日 月 星期 年 */
        asc[j++] = (trdata[i] & 0x0f) | 0x30;
    }
}

uint8_t cplus_RTC::Bcd2ToByte(uint8_t Value) {
    uint8_t tmp = 0;
    tmp = ((uint8_t)(Value & (uint8_t)0xF0) >> (uint8_t)0x4) * 10;
    return (tmp + (Value & (uint8_t)0x0F));
}

uint8_t cplus_RTC::ByteToBcd2(uint8_t Value) {
    uint8_t bcdhigh = 0;

    while (Value >= 10) {
        bcdhigh++;
        Value -= 10;
    }

    return ((uint8_t)(bcdhigh << 4) | Value);
}

void cplus_RTC::GetTime(RTC_TimeTypeDef *RTC_TimeStruct) {
    // if()
    uint8_t buf[3] = {0};

    wr->beginTransmission(0x51);
    wr->write(0x02);
    wr->endTransmission();
    wr->requestFrom(0x51, 3);

    while (wr->available()) {
        buf[0] = wr->read();
        buf[1] = wr->read();
        buf[2] = wr->read();
    }

    RTC_TimeStruct->Seconds = Bcd2ToByte(buf[0] & 0x7f); // 秒
    RTC_TimeStruct->Minutes = Bcd2ToByte(buf[1] & 0x7f); // 分
    RTC_TimeStruct->Hours = Bcd2ToByte(buf[2] & 0x3f);   // 时
}

void cplus_RTC::SetTime(RTC_TimeTypeDef *RTC_TimeStruct) {
    if (RTC_TimeStruct == NULL) return;

    wr->beginTransmission(0x51);
    wr->write(0x02);
    wr->write(ByteToBcd2(RTC_TimeStruct->Seconds));
    wr->write(ByteToBcd2(RTC_TimeStruct->Minutes));
    wr->write(ByteToBcd2(RTC_TimeStruct->Hours));
    wr->endTransmission();
}

void cplus_RTC::GetDate(RTC_DateTypeDef *RTC_DateStruct) {
    uint8_t buf[4] = {0};

    wr->beginTransmission(0x51);
    wr->write(0x05);
    wr->endTransmission();
    wr->requestFrom(0x51, 4);

    while (wr->available()) {
        buf[0] = wr->read();
        buf[1] = wr->read();
        buf[2] = wr->read();
        buf[3] = wr->read();
    }

    RTC_DateStruct->Date = Bcd2ToByte(buf[0] & 0x3f);
    RTC_DateStruct->WeekDay = Bcd2ToByte(buf[1] & 0x07);
    RTC_DateStruct->Month = Bcd2ToByte(buf[2] & 0x1f);

    if (buf[2] & 0x80) {
        RTC_DateStruct->Year = 1900 + Bcd2ToByte(buf[3] & 0xff);
    } else {
        RTC_DateStruct->Year = 2000 + Bcd2ToByte(buf[3] & 0xff);
    }
}

void cplus_RTC::SetDate(RTC_DateTypeDef *RTC_DateStruct) {
    if (RTC_DateStruct == NULL) return;
    wr->beginTransmission(0x51);
    wr->write(0x05);
    wr->write(ByteToBcd2(RTC_DateStruct->Date));
    wr->write(ByteToBcd2(RTC_DateStruct->WeekDay));

    if (RTC_DateStruct->Year < 2000) {
        wr->write(ByteToBcd2(RTC_DateStruct->Month) | 0x80);
        wr->write(ByteToBcd2((uint8_t)(RTC_DateStruct->Year % 100)));

    } else {
        /* code */
        wr->write(ByteToBcd2(RTC_DateStruct->Month) | 0x00);
        wr->write(ByteToBcd2((uint8_t)(RTC_DateStruct->Year % 100)));
    }

    wr->endTransmission();
}

void cplus_RTC::WriteReg(uint8_t reg, uint8_t data) {
    wr->beginTransmission(0x51);
    wr->write(reg);
    wr->write(data);
    wr->endTransmission();
}

uint8_t cplus_RTC::ReadReg(uint8_t reg) {
    wr->beginTransmission(0x51);
    wr->write(reg);
    wr->endTransmission(false);
    wr->requestFrom(0x51, 1);
    return wr->read();
}

int cplus_RTC::SetAlarmIRQ(int afterSeconds) {
    uint8_t reg_value = 0;
    reg_value = ReadReg(0x01);
    printf("read 0x%X\n", reg_value);

    if (afterSeconds < 0) {
        reg_value &= ~(1 << 0);
        WriteReg(0x01, reg_value);
        reg_value = 0x03;
        WriteReg(0x0E, reg_value);
        return -1;
    }

    uint8_t type_value = 2;
    uint8_t div = 1;
    if (afterSeconds > 255) {
        div = 60;
        type_value = 0x83;
    } else {
        type_value = 0x82;
    }

    afterSeconds = (afterSeconds / div) & 0xFF;
    WriteReg(0x0F, afterSeconds);
    WriteReg(0x0E, type_value);

    reg_value |= (1 << 0);
    reg_value &= ~(1 << 7);
    WriteReg(0x01, reg_value);
    return afterSeconds * div;
}

void cplus_RTC::clearIRQ() {
    uint8_t data = ReadReg(0x01);
    WriteReg(0x01, data & 0xf3);
}
void cplus_RTC::disableIRQ() {
    clearIRQ();
    uint8_t data = ReadReg(0x01);
    WriteReg(0x01, data & 0xfC);
}

struct tm cplus_RTC::getTimeStruct() {
    RTC_TimeTypeDef timeStruct;
    RTC_DateTypeDef dateStruct;
    struct tm timeinfo = {0};

    // Get current time and date from RTC
    GetTime(&timeStruct);
    GetDate(&dateStruct);

    // Populate tm structure
    timeinfo.tm_sec = timeStruct.Seconds;
    timeinfo.tm_min = timeStruct.Minutes;
    timeinfo.tm_hour = timeStruct.Hours;
    timeinfo.tm_mday = dateStruct.Date;
    timeinfo.tm_mon = dateStruct.Month - 1;    // tm_mon is 0-11, RTC month is 1-12
    timeinfo.tm_year = dateStruct.Year - 1900; // tm_year is years since 1900
    timeinfo.tm_wday = dateStruct.WeekDay;

    // Calculate day of year
    mktime(&timeinfo); // This normalizes the struct and calculates tm_yday

    return timeinfo;
}
