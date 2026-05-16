# NTPClient

[![Check Arduino status](https://github.com/arduino-libraries/NTPClient/actions/workflows/check-arduino.yml/badge.svg)](https://github.com/arduino-libraries/NTPClient/actions/workflows/check-arduino.yml)
[![Compile Examples status](https://github.com/arduino-libraries/NTPClient/actions/workflows/compile-examples.yml/badge.svg)](https://github.com/arduino-libraries/NTPClient/actions/workflows/compile-examples.yml)
[![Spell Check status](https://github.com/arduino-libraries/NTPClient/actions/workflows/spell-check.yml/badge.svg)](https://github.com/arduino-libraries/NTPClient/actions/workflows/spell-check.yml)

Connect to a NTP server, here is how:

```cpp
#include <NTPClient.h>
// change next line to use with another board/shield
#include <ESP8266WiFi.h>
//#include <WiFi.h> // for WiFi shield
//#include <WiFi101.h> // for WiFi 101 shield or MKR1000
#include <WiFiUdp.h>

const char *ssid     = "<SSID>";
const char *password = "<PASSWORD>";

WiFiUDP ntpUDP;

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP);

// You can specify the time server pool and the offset, (in seconds)
// additionally you can specify the update interval (in milliseconds).
// NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

void setup(){
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  timeClient.begin();
}

void loop() {
  timeClient.update();

  Serial.println(timeClient.getFormattedTime());

  delay(1000);
}
```

## Function documentation
`getEpochTime` returns the Unix epoch, which are the seconds elapsed since 00:00:00 UTC on 1 January 1970 (leap seconds are ignored, every day is treated as having 86400 seconds). **Attention**: If you have set a time offset this time offset will be added to your epoch timestamp.
