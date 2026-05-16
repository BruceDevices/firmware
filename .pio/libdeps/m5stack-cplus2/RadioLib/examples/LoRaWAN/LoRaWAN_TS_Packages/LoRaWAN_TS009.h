#include <Arduino.h>
#include <RadioLib.h>
// #include <RadioBoards.h>

#warning "The variables below must match your main code. Please check your radio type!"
extern SX1278 radio;          // this can be any LoRaWAN-compatible type (e.g. SX1262)
extern LoRaWANNode node;
extern uint32_t periodicity;
extern bool isConfirmed;
extern bool reply;
extern uint8_t dataUp[255];
extern size_t lenUp;
extern uint8_t fPort;

#define RADIOLIB_LORAWAN_TS009_PACKAGE_VERSION        (0x00)
#define RADIOLIB_LORAWAN_TS009_DUT_RESET              (0x01)
#define RADIOLIB_LORAWAN_TS009_DUT_JOIN               (0x02)
#define RADIOLIB_LORAWAN_TS009_SWITCH_CLASS           (0x03)
#define RADIOLIB_LORAWAN_TS009_ADR_BIT_CHANGE         (0x04)
#define RADIOLIB_LORAWAN_TS009_REGIONAL_DUTY_CYCLE    (0x05)
#define RADIOLIB_LORAWAN_TS009_TX_PERIODICITY_CHANGE  (0x06)
#define RADIOLIB_LORAWAN_TS009_TX_FRAMES_CTRL         (0x07)
#define RADIOLIB_LORAWAN_TS009_ECHO_PAYLOAD           (0x08)
#define RADIOLIB_LORAWAN_TS009_RX_APP_CNT             (0x09)
#define RADIOLIB_LORAWAN_TS009_RX_APP_CNT_RESET       (0x0A)
#define RADIOLIB_LORAWAN_TS009_LINK_CHECK             (0x20)
#define RADIOLIB_LORAWAN_TS009_DEVICE_TIME            (0x21)
#define RADIOLIB_LORAWAN_TS009_PING_SLOT_INFO         (0x22)
#define RADIOLIB_LORAWAN_TS009_TX_CW                  (0x7D)
#define RADIOLIB_LORAWAN_TS009_DUT_FPORT224_DISABLE   (0x7E)
#define RADIOLIB_LORAWAN_TS009_DUT_VERSIONS           (0x7F)

/*!
  \brief This function implements the TS009 specification.
  To enable this package, add this to your setup:
  `node.addAppPackage(RADIOLIB_LORAWAN_PACKAGE_TS009, handleTS009)`
  Make sure that all `extern` variables are handled in your user code!
*/
void handleTS009(uint8_t* dataDown, size_t lenDown) {
  if(lenDown == 0 || dataDown == NULL) {
    return;
  }
  RADIOLIB_DEBUG_PRINTLN("CID = %02x, len = %d", dataDown[0], lenDown - 1);

  switch(dataDown[0]) {
    case(RADIOLIB_LORAWAN_TS009_PACKAGE_VERSION): {
      lenUp = 3;
      dataUp[1] = 5;  // PackageIdentifier
      dataUp[2] = 1;  // PackageVersion
      fPort = RADIOLIB_LORAWAN_FPORT_TS009;
      RADIOLIB_DEBUG_PRINTLN("PackageIdentifier: %d, PackageVersion: %d", dataUp[1], dataUp[2]);

      reply = true;
    } break;

    case(RADIOLIB_LORAWAN_TS009_DUT_RESET): {
      RADIOLIB_DEBUG_PRINTLN("Restarting...");

      #warning "Please implement this reset function yourself!"

      // the function to reset the MCU is platform-dependent
      // for ESP32 for example, this would be:
      // ESP.restart();

      reply = false;
    } break;

    case(RADIOLIB_LORAWAN_TS009_DUT_JOIN): {
      RADIOLIB_DEBUG_PRINTLN("Reverting to Join state");
      node.clearSession();
      
      reply = false;
    } break;

    case(RADIOLIB_LORAWAN_TS009_SWITCH_CLASS): {
      uint8_t classType = dataDown[1];
      node.setClass(classType);
      RADIOLIB_DEBUG_PRINTLN("Switching to class: %s", classType == 0 ? "A" : (classType == 1 ? "B" : "C"));

      reply = false;
    } break;

    case(RADIOLIB_LORAWAN_TS009_ADR_BIT_CHANGE): {
      bool adr = (bool)dataDown[1];
      node.setADR(adr);
      RADIOLIB_DEBUG_PRINTLN("ADR: %d", adr);

      reply = false;
    } break;

    case(RADIOLIB_LORAWAN_TS009_REGIONAL_DUTY_CYCLE): {
      bool dutycycle = (bool)dataDown[1];
      node.setDutyCycle(dutycycle, 36000);
      RADIOLIB_DEBUG_PRINTLN("Dutycycle: %d", dutycycle);

      reply = false;
    } break;

    case(RADIOLIB_LORAWAN_TS009_TX_PERIODICITY_CHANGE): {
      uint32_t defaultIntervalSecs = 30;
      uint32_t intervals[11] = {defaultIntervalSecs, 5, 10, 20, 30, 40, 50, 60, 120, 240, 480};
      periodicity = intervals[dataDown[1]];
      
      RADIOLIB_DEBUG_PRINTLN("Tx Periodicity: %d", periodicity);

      reply = false;
    } break;

    case(RADIOLIB_LORAWAN_TS009_TX_FRAMES_CTRL): {
      switch(dataDown[1]) {
        case(0):
          // no change
          // isConfirmed = isConfirmed;
          break;
        case(1):
          isConfirmed = false;
          break;
        case(2):
          isConfirmed = true;
          break;
      }
      RADIOLIB_DEBUG_PRINTLN("Confirmed: %d", isConfirmed);

      reply = false;
    } break;

    case(RADIOLIB_LORAWAN_TS009_ECHO_PAYLOAD): {
      lenUp = lenDown;
      for (size_t i = 1; i < lenDown; i++) {
        dataUp[i] = dataDown[i] + 1;
      }
      fPort = RADIOLIB_LORAWAN_FPORT_TS009;
      RADIOLIB_DEBUG_PRINTLN("Echoing payload");

      reply = true;
    } break;

    case(RADIOLIB_LORAWAN_TS009_RX_APP_CNT): {
      lenUp = 3;
      uint16_t aFcntDown16 = (uint16_t)node.getAFCntDown();
      dataUp[1] = aFcntDown16 & 0xFF;
      dataUp[2] = aFcntDown16 >> 8;
      fPort = RADIOLIB_LORAWAN_FPORT_TS009;
      RADIOLIB_DEBUG_PRINTLN("aFCntDown16: %d", aFcntDown16);

      reply = true;
    } break;

    case(RADIOLIB_LORAWAN_TS009_RX_APP_CNT_RESET): {
      RADIOLIB_DEBUG_PRINTLN("Resetting Application Frame count");
      RADIOLIB_DEBUG_PRINTLN("WARNING: not implemented - never used in tests!");

      reply = false;
    } break;

    case(RADIOLIB_LORAWAN_TS009_LINK_CHECK): {
      lenUp = 0;
      fPort = RADIOLIB_LORAWAN_FPORT_MAC_COMMAND;
      node.sendMacCommandReq(RADIOLIB_LORAWAN_MAC_LINK_CHECK);
      RADIOLIB_DEBUG_PRINTLN("Requesting LinkCheck");

      reply = true;
    } break;

    case(RADIOLIB_LORAWAN_TS009_DEVICE_TIME): {
      lenUp = 0;
      fPort = RADIOLIB_LORAWAN_FPORT_MAC_COMMAND;
      node.sendMacCommandReq(RADIOLIB_LORAWAN_MAC_DEVICE_TIME);
      RADIOLIB_DEBUG_PRINTLN("Requesting DeviceTime");

      reply = true;
    } break;

    case(RADIOLIB_LORAWAN_TS009_PING_SLOT_INFO): {
      lenUp = 0;
      RADIOLIB_DEBUG_PRINTLN("Requesting PingSlotInfo not implemented");
      // send PingSlotInfo MAC command which is not implemented
      reply = false;
    } break;

    case(RADIOLIB_LORAWAN_TS009_TX_CW): {
      uint16_t timeout = ((uint16_t)dataDown[2] <<  8) |  (uint16_t)dataDown[1];
      uint32_t freqRaw = ((uint32_t)dataDown[5] << 16) | ((uint32_t)dataDown[4] << 8) | ((uint32_t)dataDown[3]);
      float freq = (float)freqRaw/10000.0;
      uint8_t txPower = dataDown[6];
      RADIOLIB_DEBUG_PRINTLN("Continuous wave: %7.3f MHz, %d dBm, %d s", freq, txPower, timeout);
      radio.setFrequency(freq);
      radio.setOutputPower(txPower);
      radio.transmitDirect();
      delay(timeout * 1000);
      radio.standby();

      reply = false;
    } break;

    case(RADIOLIB_LORAWAN_TS009_DUT_FPORT224_DISABLE): {
      RADIOLIB_DEBUG_PRINTLN("Disabling FPort 224");
      node.removePackage(RADIOLIB_LORAWAN_PACKAGE_TS009);

      reply = false;
    } break;

    case(RADIOLIB_LORAWAN_TS009_DUT_VERSIONS): {
      lenUp = 13;
      // firmware version - this is RadioLib's version as an example
      dataUp[1] = RADIOLIB_VERSION_MAJOR;
      dataUp[2] = RADIOLIB_VERSION_MINOR;
      dataUp[3] = RADIOLIB_VERSION_PATCH;
      dataUp[4] = RADIOLIB_VERSION_EXTRA;

      // lorawan version
      dataUp[5] = 1;
#if (LORAWAN_VERSION == 1)
      dataUp[6] = 1;
      dataUp[7] = 0;
#else
      dataUp[6] = 0;
      dataUp[7] = 4;
#endif
      dataUp[8] = 0;

      // regional parameters version
      dataUp[9] = 1;
      dataUp[10] = 0;
      dataUp[11] = 4;
      dataUp[12] = 0;
      fPort = RADIOLIB_LORAWAN_FPORT_TS009;
      RADIOLIB_DEBUG_PRINTLN("Requested DUT versions");
      
      reply = true;
    } break;
  }

  // if we must reply, copy the command ID into the uplink buffer
  if(reply) {
    dataUp[0] = dataDown[0];
  }
}