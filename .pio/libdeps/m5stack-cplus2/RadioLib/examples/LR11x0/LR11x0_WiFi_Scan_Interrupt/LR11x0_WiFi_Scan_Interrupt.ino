/*
  RadioLib LR11x0 WiFi scan Interrupt Example

  This example performs a passive scan of WiFi networks.
  The scan shows basic information about the networks,
  such as the frequency, country code and SSID.

  Other modules from LR11x0 family can also be used.
  
  This example assumes Seeed Studio Wio WM1110 is used.
  For other LR11x0 modules, some configuration such as
  RF switch control may have to be adjusted.

  Using blocking scan is not recommended, as depending
  on the scan settings, the program may be blocked
  for several seconds! Instead, interrupt scan is recommended.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#lr11x0---wifi-scan

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// LR1110 has the following connections:
// NSS pin:   10
// DIO1 pin:  2
// NRST pin:  3
// BUSY pin:  9
LR1110 radio = new Module(10, 2, 3, 9);

// or detect the pinout automatically using RadioBoards
// https://github.com/radiolib-org/RadioBoards
/*
#define RADIO_BOARD_AUTO
#include <RadioBoards.h>
Radio radio = new RadioModule();
*/

// set RF switch configuration for Wio WM1110
// Wio WM1110 uses DIO5 and DIO6 for RF switching
// NOTE: other boards may be different!
static const uint32_t rfswitch_dio_pins[] = { 
  RADIOLIB_LR11X0_DIO5, RADIOLIB_LR11X0_DIO6,
  RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC
};

static const Module::RfSwitchMode_t rfswitch_table[] = {
  // mode                  DIO5  DIO6 
  { LR11x0::MODE_STBY,   { LOW,  LOW  } },
  { LR11x0::MODE_RX,     { HIGH, LOW  } },
  { LR11x0::MODE_TX,     { HIGH, HIGH } },
  { LR11x0::MODE_TX_HP,  { LOW,  HIGH } },
  { LR11x0::MODE_TX_HF,  { LOW,  LOW  } },
  { LR11x0::MODE_GNSS,   { LOW,  LOW  } },
  { LR11x0::MODE_WIFI,   { LOW,  LOW  } },
  END_OF_MODE_TABLE,
};

// flag to indicate that a scan was completed
volatile bool scanFlag = false;

// this function is called when a scan is completed
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // scan is complete, set the flag
  scanFlag = true;
}

void setup() {
  Serial.begin(9600);

  // initialize LR1110 with default settings
  Serial.print(F("[LR1110] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // set RF switch control configuration
  radio.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);

  // set the function that will be called
  // when WiFi scan is complete
  radio.setIrqAction(setFlag);

  // scan all WiFi signals with default scan configuration
  Serial.print(F("[LR1110] Starting passive WiFi scan ... "));
  state = radio.startWifiScan('*');
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }
}

void loop() {
  // check if the flag is set
  if(scanFlag) {
    // reset flag
    scanFlag = false;

    // get the number of scan results
    uint8_t count = 0;
    Serial.print(F("[LR1110] Reading WiFi scan results ... "));
    int state = radio.getWifiScanResultsCount(&count);
    if(state == RADIOLIB_ERR_NONE) {
      Serial.println(F("success!"));

      // print the table header
      Serial.print(F("[LR1110] Reading "));
      Serial.print(count);
      Serial.println(F(" scan results:"));
      Serial.println(F(" # | WiFi type\t| Frequency\t| MAC Address\t    | Country\t| RSSI [dBm]\t| SSID"));

      // read all results one by one
      // this result type contains the most information, including the SSID
      LR11x0WifiResultExtended_t result;
      for(int i = 0; i < count; i++) {
        if(i < 10) { Serial.print(" "); } Serial.print(i); Serial.print(" | ");
        state = radio.getWifiScanResult(&result, i);
        if(state != RADIOLIB_ERR_NONE) {
          Serial.print(F("Failed to read result, code "));
          Serial.println(state);
          continue;
        }

        // print the basic information
        Serial.print(F("802.11")); Serial.print(result.type); Serial.print("\t| ");
        Serial.print(result.channelFreq); Serial.print(" MHz\t| ");

        // print MAC address
        for(int j = 0; j < 6; j++) {
          if(result.mac[j] < 0x10) { Serial.print("0"); }
          Serial.print(result.mac[j], HEX);
          if(j < 5) { Serial.print(":"); }
        }
        Serial.print(" | ");

        // print the two-letter country code
        String country = result.countryCode;
        Serial.print(country);
        Serial.print("  \t| ");

        // print the RSSI
        Serial.print(result.rssi);
        Serial.print("\t| ");
        
        // print the network SSID
        Serial.println((char*)result.ssid);
      }
    
    } else {
      // some other error occurred
      Serial.print(F("failed, code "));
      Serial.println(state);
    }
    
    // start scanning again
    Serial.print(F("[LR1110] Starting passive WiFi scan ... "));
    state = radio.startWifiScan('*');
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("success!"));
    } else {
      Serial.print(F("failed, code "));
      Serial.println(state);
    }
  }
}
