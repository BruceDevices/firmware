#include <NTPClient.h>
// change next line to use with another board/shield
#include <ESP8266WiFi.h>
//#include <WiFi.h> // for WiFi shield
//#include <WiFi101.h> // for WiFi 101 shield or MKR1000
#include <WiFiUdp.h>

const char *ssid     = "<SSID>";
const char *password = "<PASSWORD>";

WiFiUDP ntpUDP;
// initialized to a time offset of 10 hours
NTPClient timeClient(ntpUDP,"pool.ntp.org", 36000, 60000);
//                           HH:MM:SS
// timeClient initializes to 10:00:00 if it does not receive an NTP packet
// before the 100ms timeout.
// without isTimeSet() the LED would be switched on, although the time
// was not yet set correctly.

// blue LED on ESP-12F
const int led = 2;
const int hour = 10;
const int minute = 0;

void setup(){
  Serial.begin(115200);

  pinMode(led, OUTPUT);
  // led is off when pin is high
  digitalWrite(led, 1);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay (500);
    Serial.print (".");
  }

  timeClient.begin();
}

void loop() {
  timeClient.update();

  Serial.println(timeClient.getFormattedTime());
  if(timeClient.isTimeSet()) {
    if (hour == timeClient.getHours() && minute == timeClient.getMinutes()) {
      digitalWrite(led, 0);
    }
  }

  delay(1000);
}
