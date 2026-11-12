#ifndef RF_WEATHER_STATION_H
#define RF_WEATHER_STATION_H

// Native (non-JS) live weather-sensor monitor: decodes AcuRite-606TX, Nexus
// and Prologue OOK/PPM frames directly from CC1101 captures and shows a
// Flipper-Zero-style station list (model/id/channel/temp/humidity/battery).
void rf_weather_station();

#endif
