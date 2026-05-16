/*
  RadioLib LR11x0 Firmware Update Example

  This example updates the internal LR1110 firmware.
  Newer versions of the firmware introduce fixes
  and possibly even new features, so it is recommended
  to use the latest available firmware version
  when possible.

  Other modules from LR11x0 family can also be used.

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// select the firmware image you want to upload
// WARNING: Make sure you select the correct firmware
//          for your device! Uploading incorrect firmware
//          (e.g. LR1110 firmware to LR1120 device)
//          may damage your hardware!
//#define RADIOLIB_LR1110_FIRMWARE_0303
//#define RADIOLIB_LR1110_FIRMWARE_0304
//#define RADIOLIB_LR1110_FIRMWARE_0305
//#define RADIOLIB_LR1110_FIRMWARE_0306
//#define RADIOLIB_LR1110_FIRMWARE_0307
#define RADIOLIB_LR1110_FIRMWARE_0401
//#define RADIOLIB_LR1120_FIRMWARE_0101
//#define RADIOLIB_LR1120_FIRMWARE_0102
//#define RADIOLIB_LR1120_FIRMWARE_0201
//#define RADIOLIB_LR1121_FIRMWARE_0102
//#define RADIOLIB_LR1121_FIRMWARE_0103

// enable this macro if you want to store the image in host
// MCU RAM instead of Flash.
// NOTE: the firmware images are very large, up to 240 kB!
//#define RADIOLIB_LR1110_FIRMWARE_IN_RAM

// include the firmware image
#include <modules/LR11x0/LR11x0_firmware.h>

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

  // print the firmware versions before the update
  printVersions();

  // prompt the user
  Serial.println(F("[LR1110] Send any character to start the update"));
  while(!Serial.available()) { delay(1); }

  // upload update into LR11x0 non-volatile memory
  Serial.print(F("[LR1110] Updating firmware, this may take several seconds ... "));
  state = radio.updateFirmware(lr11xx_firmware_image, RADIOLIB_LR11X0_FIRMWARE_IMAGE_SIZE);
  /*
    use the following if you enabled RADIOLIB_LR1110_FIRMWARE_IN_RAM
    state = radio.updateFirmware(lr11xx_firmware_image, RADIOLIB_LR11X0_FIRMWARE_IMAGE_SIZE, false);
  */
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // print the firmware versions after the update
  printVersions();
  
}

void printVersions() {
  LR11x0VersionInfo_t version;
  Serial.print(F("[LR1110] Reading firmware versions ... "));
  int16_t state = radio.getVersionInfo(&version);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));

    Serial.print(F("[LR1110] Device: "));
    Serial.println(version.device);

    Serial.print(F("[LR1110] Base firmware: "));
    Serial.print(version.fwMajor);
    Serial.print('.');
    Serial.println(version.fwMinor);

    Serial.print(F("[LR1110] WiFi firmware: "));
    Serial.print(version.fwMajorWiFi);
    Serial.print('.');
    Serial.println(version.fwMinorWiFi);

    Serial.print(F("[LR1110] GNSS firmware: "));
    Serial.print(version.fwGNSS);
    Serial.print('.');
    Serial.println(version.almanacGNSS);

  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  
  }

}

void loop() {
  
}
