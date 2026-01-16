#include "pcf85063_RTC.h"

#ifndef RTC_SDA
#define RTC_SDA 21
#endif
#ifndef RTC_SCL
#define RTC_SCL 22
#endif

void pcf85063_RTC::begin(void) {
    wr->begin(RTC_SDA, RTC_SCL);

    // Initialize PCF85063 with basic settings
    // Reset oscillator stop flag and enable normal operation
    uint8_t ctrl1 = ReadReg(PCF85063_REG_CTRL1);
    ctrl1 &= ~0x20; // Clear OSF (Oscillator Stop Flag)
    WriteReg(PCF85063_REG_CTRL1, ctrl1);
}

void pcf85063_RTC::GetPcf85063Time(void) {
    wr->beginTransmission(PCF85063_ADDR);
    wr->write(PCF85063_REG_SC);
    wr->endTransmission();
    wr->requestFrom(PCF85063_ADDR, 7);

    while (wr->available()) {
        trdata[0] = wr->read(); // Seconds
        trdata[1] = wr->read(); // Minutes
        trdata[2] = wr->read(); // Hours
        trdata[3] = wr->read(); // Day of month
        trdata[4] = wr->read(); // Day of week
        trdata[5] = wr->read(); // Month
        trdata[6] = wr->read(); // Year
    }

    DataMask();
    Bcd2asc();
    Str2Time();
}

void pcf85063_RTC::Str2Time(void) {
    Second = (asc[0] - 0x30) * 10 + asc[1] - 0x30;
    Minute = (asc[2] - 0x30) * 10 + asc[3] - 0x30;
    Hour = (asc[4] - 0x30) * 10 + asc[5] - 0x30;
}

void pcf85063_RTC::DataMask() {
    trdata[0] = trdata[0] & 0x7F; // Seconds (bit 7 is OS - Oscillator Stop)
    trdata[1] = trdata[1] & 0x7F; // Minutes (bit 7 unused)
    trdata[2] = trdata[2] & 0x3F; // Hours (bits 7-6 unused)
    trdata[3] = trdata[3] & 0x3F; // Day of month (bits 7-6 unused)
    trdata[4] = trdata[4] & 0x07; // Day of week (bits 7-3 unused)
    trdata[5] = trdata[5] & 0x1F; // Month (bits 7-5 unused)
    trdata[6] = trdata[6] & 0xFF; // Year (all bits used)
}

void pcf85063_RTC::Bcd2asc(void) {
    uint8_t i, j;
    for (j = 0, i = 0; i < 7; i++) {
        asc[j++] = (trdata[i] & 0xF0) >> 4 | 0x30;
        asc[j++] = (trdata[i] & 0x0F) | 0x30;
    }
}

uint8_t pcf85063_RTC::Bcd2ToByte(uint8_t Value) {
    uint8_t tmp = 0;
    tmp = ((uint8_t)(Value & (uint8_t)0xF0) >> (uint8_t)0x4) * 10;
    return (tmp + (Value & (uint8_t)0x0F));
}

uint8_t pcf85063_RTC::ByteToBcd2(uint8_t Value) {
    uint8_t bcdhigh = 0;

    while (Value >= 10) {
        bcdhigh++;
        Value -= 10;
    }

    return ((uint8_t)(bcdhigh << 4) | Value);
}

void pcf85063_RTC::GetTime(RTC_TimeTypeDef *RTC_TimeStruct) {
    uint8_t buf[3] = {0};

    wr->beginTransmission(PCF85063_ADDR);
    wr->write(PCF85063_REG_SC);
    wr->endTransmission();
    wr->requestFrom(PCF85063_ADDR, 3);

    while (wr->available()) {
        buf[0] = wr->read(); // Seconds
        buf[1] = wr->read(); // Minutes
        buf[2] = wr->read(); // Hours
    }

    RTC_TimeStruct->Seconds = Bcd2ToByte(buf[0] & 0x7F);
    RTC_TimeStruct->Minutes = Bcd2ToByte(buf[1] & 0x7F);
    RTC_TimeStruct->Hours = Bcd2ToByte(buf[2] & 0x3F);
}

void pcf85063_RTC::SetTime(RTC_TimeTypeDef *RTC_TimeStruct) {
    if (RTC_TimeStruct == NULL) return;

    // Stop the clock to ensure atomic update
    uint8_t ctrl1 = ReadReg(PCF85063_REG_CTRL1);
    WriteReg(PCF85063_REG_CTRL1, ctrl1 | 0x20); // Set STOP bit

    wr->beginTransmission(PCF85063_ADDR);
    wr->write(PCF85063_REG_SC);
    wr->write(ByteToBcd2(RTC_TimeStruct->Seconds) & 0x7F); // Clear OS bit
    wr->write(ByteToBcd2(RTC_TimeStruct->Minutes));
    wr->write(ByteToBcd2(RTC_TimeStruct->Hours));
    wr->endTransmission();

    // Restart the clock
    WriteReg(PCF85063_REG_CTRL1, ctrl1 & 0xDF); // Clear STOP bit
}

void pcf85063_RTC::GetDate(RTC_DateTypeDef *RTC_DateStruct) {
    uint8_t buf[4] = {0};

    wr->beginTransmission(PCF85063_ADDR);
    wr->write(PCF85063_REG_DM);
    wr->endTransmission();
    wr->requestFrom(PCF85063_ADDR, 4);

    while (wr->available()) {
        buf[0] = wr->read(); // Day of month
        buf[1] = wr->read(); // Day of week
        buf[2] = wr->read(); // Month
        buf[3] = wr->read(); // Year
    }

    RTC_DateStruct->Date = Bcd2ToByte(buf[0] & 0x3F);
    RTC_DateStruct->WeekDay = Bcd2ToByte(buf[1] & 0x07);
    RTC_DateStruct->Month = Bcd2ToByte(buf[2] & 0x1F);
    RTC_DateStruct->Year = 2000 + Bcd2ToByte(buf[3] & 0xFF); // PCF85063 only supports 2000-2099
}

void pcf85063_RTC::SetDate(RTC_DateTypeDef *RTC_DateStruct) {
    if (RTC_DateStruct == NULL) return;

    // Stop the clock to ensure atomic update
    uint8_t ctrl1 = ReadReg(PCF85063_REG_CTRL1);
    WriteReg(PCF85063_REG_CTRL1, ctrl1 | 0x20); // Set STOP bit

    wr->beginTransmission(PCF85063_ADDR);
    wr->write(PCF85063_REG_DM);
    wr->write(ByteToBcd2(RTC_DateStruct->Date));
    wr->write(ByteToBcd2(RTC_DateStruct->WeekDay));
    wr->write(ByteToBcd2(RTC_DateStruct->Month));
    wr->write(ByteToBcd2((uint8_t)(RTC_DateStruct->Year % 100))); // PCF85063 year is 00-99
    wr->endTransmission();

    // Restart the clock
    WriteReg(PCF85063_REG_CTRL1, ctrl1 & 0xDF); // Clear STOP bit
}

void pcf85063_RTC::WriteReg(uint8_t reg, uint8_t data) {
    wr->beginTransmission(PCF85063_ADDR);
    wr->write(reg);
    wr->write(data);
    wr->endTransmission();
}

uint8_t pcf85063_RTC::ReadReg(uint8_t reg) {
    wr->beginTransmission(PCF85063_ADDR);
    wr->write(reg);
    wr->endTransmission(false);
    wr->requestFrom(PCF85063_ADDR, 1);
    return wr->read();
}

int pcf85063_RTC::SetAlarmIRQ(int afterSeconds) {
    // Clear any existing alarm interrupt
    uint8_t ctrl2 = ReadReg(PCF85063_REG_CTRL2);

    if (afterSeconds < 0) {
        // Disable alarm
        ctrl2 &= ~0x80; // Clear AIE (Alarm Interrupt Enable)
        WriteReg(PCF85063_REG_CTRL2, ctrl2);
        return -1;
    }

    // PCF85063 timer setup - use countdown timer for alarm functionality
    // Timer can count seconds (0-255) or minutes (0-255)
    uint8_t timer_mode;
    uint8_t timer_value;

    if (afterSeconds > 255) {
        // Use minute timer
        timer_value = (afterSeconds / 60) & 0xFF;
        timer_mode = 0x92; // Timer enabled, source clock = 1/60 Hz, interrupt enabled
        afterSeconds = timer_value * 60;
    } else {
        // Use second timer
        timer_value = afterSeconds & 0xFF;
        timer_mode = 0x91; // Timer enabled, source clock = 1 Hz, interrupt enabled
    }

    WriteReg(PCF85063_REG_TIMER_VALUE, timer_value);
    WriteReg(PCF85063_REG_TIMER_MODE, timer_mode);

    // Enable timer interrupt
    ctrl2 |= 0x01; // Set TIE (Timer Interrupt Enable)
    WriteReg(PCF85063_REG_CTRL2, ctrl2);

    return afterSeconds;
}

void pcf85063_RTC::clearIRQ() {
    uint8_t ctrl2 = ReadReg(PCF85063_REG_CTRL2);
    ctrl2 &= 0x7D; // Clear TF and AF flags
    WriteReg(PCF85063_REG_CTRL2, ctrl2);
}

void pcf85063_RTC::disableIRQ() {
    clearIRQ();
    uint8_t ctrl2 = ReadReg(PCF85063_REG_CTRL2);
    ctrl2 &= 0x7E; // Clear TIE and AIE
    WriteReg(PCF85063_REG_CTRL2, ctrl2);
}

// PCF85063 specific functions
void pcf85063_RTC::setClockOutput(uint8_t frequency) {
    uint8_t ctrl2 = ReadReg(PCF85063_REG_CTRL2);
    ctrl2 &= 0xF8; // Clear COF bits
    ctrl2 |= (frequency & 0x07);
    WriteReg(PCF85063_REG_CTRL2, ctrl2);
}

void pcf85063_RTC::setBatteryLowDetection(bool enable) {
    uint8_t ctrl1 = ReadReg(PCF85063_REG_CTRL1);
    if (enable) {
        ctrl1 |= 0x04; // Set BLF bit
    } else {
        ctrl1 &= 0xFB; // Clear BLF bit
    }
    WriteReg(PCF85063_REG_CTRL1, ctrl1);
}

void pcf85063_RTC::reset() {
    // Software reset - clear all registers to default values
    WriteReg(PCF85063_REG_CTRL1, 0x58); // Default value with software reset
    delay(10);                          // Give time for reset
}

struct tm pcf85063_RTC::getTimeStruct() {
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
