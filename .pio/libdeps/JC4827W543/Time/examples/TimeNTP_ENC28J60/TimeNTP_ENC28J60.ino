/*
 * Time_NTP.pde
 * Example showing time sync to NTP time source
 *
 * Also shows how to handle DST automatically.
 *
 * This sketch uses the EtherCard library:
 * http://jeelabs.org/pub/docs/ethercard/
 */
 
#include <TimeLib.h>
#include <EtherCard.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; 

// NTP Server
const char timeServer[] PROGMEM = "pool.ntp.org";

const int utcOffset = 1;     // Central European Time
//const int utcOffset = -5;  // Eastern Standard Time (USA)
//const int utcOffset = -4;  // Eastern Daylight Time (USA)
//const int utcOffset = -8;  // Pacific Standard Time (USA)
//const int utcOffset = -7;  // Pacific Daylight Time (USA)

// Packet buffer, must be big enough to packet and payload
#define BUFFER_SIZE 550
byte Ethernet::buffer[BUFFER_SIZE];

const unsigned int remotePort = 123;

void setup() 
{
  Serial.begin(9600);
  
  while (!Serial)    // Needed for Leonardo only
    ;
  delay(250);
  
  Serial.println("TimeNTP_ENC28J60 Example");
  
  if (ether.begin(BUFFER_SIZE, mac) == 0) {
     // no point in carrying on, so do nothing forevermore:
    while (1) {
      Serial.println("Failed to access Ethernet controller");
      delay(10000);
    }
  }
  
  if (!ether.dhcpSetup()) {
    // no point in carrying on, so do nothing forevermore:
    while (1) {
      Serial.println("Failed to configure Ethernet using DHCP");
      delay(10000);
    }
  }

  ether.printIp("IP number assigned by DHCP is ", ether.myip);

  Serial.println("waiting for sync");
  //setSyncProvider(getNtpTime);            // Use this for GMT time
  setSyncProvider(getDstCorrectedTime);     // Use this for local, DST-corrected time
}

time_t prevDisplay = 0; // when the digital clock was displayed

void loop()
{
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      digitalClockDisplay();  
    }
  }
}

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
}

void printDigits(int digits){
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

/*-------- NTP code ----------*/

// SyncProvider that returns UTC time
time_t getNtpTime()
{
  // Send request
  Serial.println("Transmit NTP Request");
  if (!ether.dnsLookup(timeServer)) {
    Serial.println("DNS failed");
    return 0; // return 0 if unable to get the time
  } else {
    //ether.printIp("SRV: ", ether.hisip);
    ether.ntpRequest(ether.hisip, remotePort);
  
    // Wait for reply
    uint32_t beginWait = millis();
    while (millis() - beginWait < 1500) {
      word len = ether.packetReceive();
      ether.packetLoop(len);

      unsigned long secsSince1900 = 0L;
      if (len > 0 && ether.ntpProcessAnswer(&secsSince1900, remotePort)) {
        Serial.println("Receive NTP Response");
        return secsSince1900 - 2208988800UL;
      }
    }
    
    Serial.println("No NTP Response :-(");
    return 0;
  }
}

/* Alternative SyncProvider that automatically handles Daylight Saving Time (DST) periods,
 * at least in Europe, see below.
 */
time_t getDstCorrectedTime (void) {
  time_t t = getNtpTime ();

  if (t > 0) {
    TimeElements tm;
    breakTime (t, tm);
    t += (utcOffset + dstOffset (tm.Day, tm.Month, tm.Year + 1970, tm.Hour)) * SECS_PER_HOUR;
  }

  return t;
}

/* This function returns the DST offset for the current UTC time.
 * This is valid for the EU, for other places see
 * http://www.webexhibits.org/daylightsaving/i.html
 * 
 * Results have been checked for 2012-2030 (but should work since
 * 1996 to 2099) against the following references:
 * - http://www.uniquevisitor.it/magazine/ora-legale-italia.php
 * - http://www.calendario-365.it/ora-legale-orario-invernale.html
 */
byte dstOffset (byte d, byte m, unsigned int y, byte h) {
  // Day in March that DST starts on, at 1 am
  byte dstOn = (31 - (5 * y / 4 + 4) % 7);

  // Day in October that DST ends  on, at 2 am
  byte dstOff = (31 - (5 * y / 4 + 1) % 7);

  if ((m > 3 && m < 10) ||
      (m == 3 && (d > dstOn || (d == dstOn && h >= 1))) ||
      (m == 10 && (d < dstOff || (d == dstOff && h <= 1))))
    return 1;
  else
    return 0;
}

