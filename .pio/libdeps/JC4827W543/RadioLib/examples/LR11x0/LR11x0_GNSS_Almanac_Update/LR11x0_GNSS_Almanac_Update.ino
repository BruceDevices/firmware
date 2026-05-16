/*
  RadioLib LR11x0 GNSS Almanac Update Example

  This example updates the LR11x0 GNSS almanac.
  Almanac is a database of orbital predictions of 
  GNSS satellites, which allows the module to predict
  when different satellites will appear in the sky,
  and frequency of their signal.
  
  Up-to-date almanac is necessary for operation!
  After an update, data will remain valid for 30 days.
  All GNSS examples require at least limited
  visibility of the sky!

  NOTE: This example will only work for LR11x0 devices
        with sufficiently recent firmware!
        LR1110: 4.1
        LR1120: 2.1
        If your device firmware reports older firmware,
        update it using the LR11x0_Firmware_Update example.

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

// structure to save information about the GNSS almanac
LR11x0GnssAlmanacStatus_t almStatus;

void setup() {
  Serial.begin(9600);

  // initialize LR1110 with default settings
  Serial.print(F("[LR1110] Initializing ... "));
  int state = radio.beginGNSS(RADIOLIB_LR11X0_GNSS_CONSTELLATION_GPS);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // set RF switch control configuration
  radio.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);

  // check the firmware version
  Serial.print(F("[LR1110] Checking firmware version ... "));
  state = radio.isGnssScanCapable();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("check passed!"));
  } else {
    Serial.println(F("check failed, firmware update needed."));
    while (true) { delay(10); }
  }

  // run GNSS scans until we get at least the time
  // NOTE: Depending on visibility of satellites,
  //       this may take multiple attempts!
  while(true) {
    // run GNSS scan
    Serial.print(F("[LR1110] Running GNSS scan ... "));
    state = radio.gnssScan(NULL);
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("success!"));
    } else {
      Serial.print(F("failed, code "));
      Serial.println(state);
      while (true) { delay(10); }
    }

    // check almanac status
    Serial.print(F("[LR1110] Checking GNSS almanac ... "));
    state = radio.getGnssAlmanacStatus(&almStatus);
    if (state != RADIOLIB_ERR_NONE) {
      Serial.print(F("failed, code "));
      Serial.println(state);
      while (true) { delay(10); }
    }

    // we have the status, check if we have demodulated time
    if(almStatus.gps.status < RADIOLIB_LR11X0_GNSS_ALMANAC_STATUS_UP_TO_DATE) {
      Serial.println(F("time unknown, another scan needed."));
    
    } else if(almStatus.gps.numUpdateNeeded > 0) {
      Serial.print(almStatus.gps.numUpdateNeeded);
      Serial.println(F(" satellites out-of-date."));
      break;
    
    } else {
      Serial.println(F("no update needed!"));
      while (true) { delay(10); }
      
    }
  }
}

void loop() {
  // wait until almanac data is available in the signal
  // multiple attempts are needed for this
  Serial.print(F("[LR1110] Waiting for subframe ... "));
  int state = radio.gnssDelayUntilSubframe(&almStatus, RADIOLIB_LR11X0_GNSS_CONSTELLATION_GPS);
  if(state == RADIOLIB_ERR_GNSS_SUBFRAME_NOT_AVAILABLE) {
    Serial.println(F("not enough time left."));

    // wait until the next update window
    delay(2000);
    
  } else {
    Serial.println(F("done!"));

    // we have enough time to start the update
    Serial.print(F("[LR1110] Starting update ... "));
    state = radio.updateGnssAlmanac(RADIOLIB_LR11X0_GNSS_CONSTELLATION_GPS);
    if(state != RADIOLIB_ERR_NONE) {
      Serial.print(F("failed, code "));
      Serial.println(state);
    } else {
      Serial.println(F("done!"));
    }
  
  }

  // check whether another update is needed
  Serial.print(F("[LR1110] Checking GNSS almanac ... "));
  state = radio.getGnssAlmanacStatus(&almStatus);
  if(state != RADIOLIB_ERR_NONE) {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // check if we have completed the update
  if(almStatus.gps.numUpdateNeeded == 0) {
    Serial.println(F("all satellites up-to-date!"));
    while (true) { delay(10); }
  } else {
    Serial.print(almStatus.gps.numUpdateNeeded);
    Serial.println(F(" satellites out-of-date."));
  }
  
  // wait a bit before the next update attempt
  delay(1000);
  
}
