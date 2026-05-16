#include "ADSB.h"

#if !RADIOLIB_EXCLUDE_ADSB

#include <math.h>

ADSBClient::ADSBClient(PhysicalLayer* phy) {
  phyLayer = phy;
}

int16_t ADSBClient::begin() {
  // set what is supported by PHY
  int16_t state = phyLayer->setFrequency(RADIOLIB_ADSB_CARRIER_FREQUENCY);
  RADIOLIB_ASSERT(state);

  state = phyLayer->setPreambleLength(16);
  RADIOLIB_ASSERT(state);
  
  state = phyLayer->setSyncWord(NULL, 0);
  RADIOLIB_ASSERT(state);

  state = phyLayer->setEncoding(RADIOLIB_ENCODING_MANCHESTER_INV);
  return(state);
}

int16_t ADSBClient::decode(const uint8_t in[RADIOLIB_ADSB_FRAME_LEN_BYTES], ADSBFrame* out) {
  RADIOLIB_ASSERT_PTR(out);

  // get the basic information
  out->downlinkFormat = (in[0] & 0xF8) >> 3;
  out->capability = in[0] & 0x07;
  for(int i = 0; i < RADIOLIB_ADSB_FRAME_ICAO_LEN_BYTES; i++) {
    out->icao[i] = in[RADIOLIB_ADSB_FRAME_ICAO_POS + i];
  }

  // lookup table to avoid a whole bunch of if-else statements
  const ADSBMessageType msgTypeLut[] = {
    ADSBMessageType::RESERVED,
    ADSBMessageType::AIRCRAFT_ID, ADSBMessageType::AIRCRAFT_ID,
    ADSBMessageType::AIRCRAFT_ID, ADSBMessageType::AIRCRAFT_ID, 
    ADSBMessageType::SURFACE_POS, ADSBMessageType::SURFACE_POS, 
    ADSBMessageType::SURFACE_POS, ADSBMessageType::SURFACE_POS, 
    ADSBMessageType::AIRBORNE_POS_ALT_BARO, ADSBMessageType::AIRBORNE_POS_ALT_BARO, 
    ADSBMessageType::AIRBORNE_POS_ALT_BARO, ADSBMessageType::AIRBORNE_POS_ALT_BARO, 
    ADSBMessageType::AIRBORNE_POS_ALT_BARO, ADSBMessageType::AIRBORNE_POS_ALT_BARO, 
    ADSBMessageType::AIRBORNE_POS_ALT_BARO, ADSBMessageType::AIRBORNE_POS_ALT_BARO, 
    ADSBMessageType::AIRBORNE_POS_ALT_BARO, ADSBMessageType::AIRBORNE_POS_ALT_BARO, 
    ADSBMessageType::AIRBORNE_VEL, 
    ADSBMessageType::AIRBORNE_POS_ALT_GNSS,
    ADSBMessageType::AIRBORNE_POS_ALT_GNSS,
    ADSBMessageType::AIRBORNE_POS_ALT_GNSS,
    ADSBMessageType::RESERVED, ADSBMessageType::RESERVED,
    ADSBMessageType::RESERVED, ADSBMessageType::RESERVED, ADSBMessageType::RESERVED,
    ADSBMessageType::AIRCRAFT_STATUS,
    ADSBMessageType::TARGET_STATE,
    ADSBMessageType::RESERVED,
    ADSBMessageType::AIRCRAFT_OPS_STATUS,
  };

  // get the message type and then the message itself
  uint8_t typeCode = (in[RADIOLIB_ADSB_FRAME_MESSAGE_POS] & (0x1F << 3)) >> 3;
  RADIOLIB_DEBUG_PROTOCOL_PRINTLN("ADS-B Type Code = %d", typeCode);
  out->messageType = msgTypeLut[typeCode];
  for(int i = 0; i < RADIOLIB_ADSB_FRAME_MESSAGE_LEN_BYTES; i++) {
    out->message[i] = in[RADIOLIB_ADSB_FRAME_MESSAGE_POS + i];
  }

  return(RADIOLIB_ERR_NONE);
}

int16_t ADSBClient::parseHexId(const ADSBFrame* in, char id[RADIOLIB_ADSB_HEX_ID_LEN]) {
  RADIOLIB_ASSERT_PTR(in);

  for(int i = 0; i < RADIOLIB_ADSB_HEX_ID_LEN / 2; i++) {
    snprintf(&id[2*i], 3, "%02X", in->icao[i]);
  }

  id[RADIOLIB_ADSB_HEX_ID_LEN - 1] = '\0';
  return(RADIOLIB_ERR_NONE);
}

int16_t ADSBClient::parseCallsign(const ADSBFrame* in, char callsign[RADIOLIB_ADSB_CALLSIGN_LEN], ADSBAircraftCategory* cat) {
  RADIOLIB_ASSERT_PTR(in);

  if(in->messageType != ADSBMessageType::AIRCRAFT_ID) {
    return(RADIOLIB_ERR_ADSB_INVALID_MSG_TYPE);
  }

  // precomputed bitshifts
  callsign[0] = (in->message[1] & (0x3F << 2)) >> 2;
  callsign[1] = ((in->message[1] & (0x03 >> 0)) << 4) | ((in->message[2] & (0x0F << 4)) >> 4);
  callsign[2] = ((in->message[2] & (0x0F << 0)) << 2) | ((in->message[3] & (0x03 << 6)) >> 6);
  callsign[3] = in->message[3] & 0x3F;
  callsign[4] = (in->message[4] & (0x3F << 2)) >> 2;
  callsign[5] = ((in->message[4] & (0x03 >> 0)) << 4) | ((in->message[5] & (0x0F << 4)) >> 4);
  callsign[6] = ((in->message[5] & (0x0F << 0)) << 2) | ((in->message[6] & (0x03 << 6)) >> 6);
  callsign[7] = in->message[6] & 0x3F;
  callsign[RADIOLIB_ADSB_CALLSIGN_LEN - 1] = '\0';

  // convert to ASCII
  for(int i = 0; i < RADIOLIB_ADSB_CALLSIGN_LEN - 1; i++) {
    if((callsign[i] >= 1) && (callsign[i] <= 26)) {
      callsign[i] += '@';
    } else if(callsign[i] == 32) {
      callsign[i] = ' ';
    } else if(!((callsign[i] >= '0') && (callsign[i] <= '9'))) {
      callsign[i] = '\0';
    }
  }

  // only continue processing if category was requested by user
  if(!cat) { return(RADIOLIB_ERR_NONE);}
  
  uint8_t type = (in->message[0] & 0xF8) >> 3;
  uint8_t category = in->message[0] & 0x03;
  if(type < 2) {
    *cat = ADSBAircraftCategory::RESERVED;
    return(RADIOLIB_ERR_NONE);
  } else if(category == 0) {
    *cat = ADSBAircraftCategory::NONE;
    return(RADIOLIB_ERR_NONE);
  }

  // lookup table to avoid a whole bunch of if-else statements
  const ADSBAircraftCategory catLut[] = {
    ADSBAircraftCategory::SURFACE_EMERGENCY,
    ADSBAircraftCategory::RESERVED,
    ADSBAircraftCategory::SURFACE_SERVICE,
    ADSBAircraftCategory::GROUND_OBSTRUCTION,
    ADSBAircraftCategory::GROUND_OBSTRUCTION,
    ADSBAircraftCategory::GROUND_OBSTRUCTION,
    ADSBAircraftCategory::GROUND_OBSTRUCTION,
    ADSBAircraftCategory::GLIDER,
    ADSBAircraftCategory::LIGHTER_THAN_AIR,
    ADSBAircraftCategory::PARACHUTIST,
    ADSBAircraftCategory::ULTRALIGHT_PARAGLIDER,
    ADSBAircraftCategory::RESERVED,
    ADSBAircraftCategory::UAV,
    ADSBAircraftCategory::SPACE_VEHICLE,
    ADSBAircraftCategory::LIGHT,
    ADSBAircraftCategory::MEDIUM_1,
    ADSBAircraftCategory::MEDIUM_2,
    ADSBAircraftCategory::HIGH_VORTEX,
    ADSBAircraftCategory::HEAVY,
    ADSBAircraftCategory::HIGH_PERFORMANCE,
    ADSBAircraftCategory::ROTORCRAFT,
  };

  // the index is parsed from the type and category
  // in real world values overflowing the lookup table should not appear,
  // but better to check anyway
  size_t index = (type - 2)*7 + (category - 1);
  if(index >= sizeof(catLut)/sizeof(catLut[0])) {
    return(RADIOLIB_ERR_ADSB_INVALID_CATEGORY);
  }

  *cat = catLut[index];
  return(RADIOLIB_ERR_NONE);
}

void ADSBClient::setReferencePosition(float lat, float lon) {
  this->refPos[0] = lat;
  this->refPos[1] = lon;
}

// lookup table for longitude zones
// having this as lookup avoids a whole bunch of floating-point calculations
static const float lonZoneLut[] = {
  87.0000000000000f, 86.5353699751210f, 85.7554162094442f, 84.8916619070209f, 83.9917356298057f, 
  83.0719944471981f, 82.1395698051061f, 81.1980134927195f, 80.2492321328051f, 79.2942822545693f, 
  78.3337408292275f, 77.3678946132819f, 76.3968439079447f, 75.4205625665336f, 74.4389341572514f, 
  73.4517744166787f, 72.4588454472895f, 71.4598647302898f, 70.4545107498760f, 69.4424263114402f, 
  68.4232202208333f, 67.3964677408467f, 66.3617100838262f, 65.3184530968209f, 64.2661652256744f, 
  63.2042747938193f, 62.1321665921033f, 61.0491777424635f, 59.9545927669403f, 58.8476377614846f, 
  57.7274735386611f, 56.5931875620592f, 55.4437844449504f, 54.2781747227290f, 53.0951615279600f, 
  51.8934246916877f, 50.6715016555384f, 49.4277643925569f, 48.1603912809662f, 46.8673325249875f, 
  45.5462672266023f, 44.1945495141927f, 42.8091401224356f, 41.3865183226024f, 39.9225668433386f, 
  38.4124189241226f, 36.8502510759353f, 35.2289959779639f, 33.5399343629848f, 31.7720970768108f, 
  29.9113568573181f, 27.9389871012190f, 25.8292470705878f, 23.5450448655707f, 21.0293949260285f, 
  18.1862635707134f, 14.8281743686868f, 10.4704712999685f, 
};

int16_t ADSBClient::parseAirbornePosition(const ADSBFrame* in, int* alt, float* lat, float* lon, bool* altGnss) {
  RADIOLIB_ASSERT_PTR(in);

  if((in->messageType != ADSBMessageType::AIRBORNE_POS_ALT_BARO) &&
     (in->messageType != ADSBMessageType::AIRBORNE_POS_ALT_GNSS)) {
    return(RADIOLIB_ERR_ADSB_INVALID_MSG_TYPE);
  }

  // parse altitude
  if(alt) {
    if(in->messageType == ADSBMessageType::AIRBORNE_POS_ALT_BARO) {
      // barometric altitude, get the raw value without the Q bit
      uint16_t altRaw = ((in->message[1] & 0xFE) << 3) | ((in->message[2] & 0xF0) >> 4);

      // now get the step size based on the Q bit and calculate
      int step = (in->message[1] & 0x01) ? 25 : 100;
      *alt = step*(int)altRaw - 1000;

    } else {
      // GNSS altitude, which is just meters
      *alt = (in->message[1] << 4) | ((in->message[2] & 0xF0) >> 4);

    }

    // pass the source flag, if requested
    if(altGnss) { *altGnss = (in->messageType == ADSBMessageType::AIRBORNE_POS_ALT_GNSS); }
  }

  // check if this is an even or odd frame
  bool odd = in->message[2] & 0x04;

  // always calculate the latitude - it is needed to also calculate longitude
  uint32_t latRaw = (((uint32_t)(in->message[2] & 0x03)) << 15) | ((uint32_t)in->message[3] << 7) | (uint32_t)((in->message[4] & 0xFE) >> 1);
  float latCpr = (float)latRaw / (float)(1UL << 17);
  float latZoneSize = odd ? 360.0f/59.0f : 6.0f;
  int latZoneIdx = floor(this->refPos[0] / latZoneSize) + floor((fmod(this->refPos[0], latZoneSize) / latZoneSize) - latCpr + 0.5f);
  float tmpLat = latZoneSize * (latZoneIdx + (float)latCpr);
  if(lat) { *lat = tmpLat; }
  RADIOLIB_DEBUG_PROTOCOL_PRINT("latRaw=%d\n", latRaw);
  RADIOLIB_DEBUG_PROTOCOL_PRINT("latCpr=%f\n", (double)latCpr);
  RADIOLIB_DEBUG_PROTOCOL_PRINT("latZoneSize=%f\n", (double)latZoneSize);
  RADIOLIB_DEBUG_PROTOCOL_PRINT("latZoneIdx=%d\n", latZoneIdx);

  // only calculate longitude if the user requested it
  if(lon) {
    int lonZone = 1;
    if(abs(tmpLat) < 87.0f) {
      for(size_t i = 0; i < sizeof(lonZoneLut)/sizeof(lonZoneLut[0]); i++) {
        if(abs(tmpLat) >= lonZoneLut[i]) {
          lonZone = i + 1;
          break;
        }
      }
      if(lonZone == 1) {
        lonZone = 59;
      }
    }
    uint32_t lonRaw = (((uint32_t)(in->message[4] & 0x01)) << 16) | ((uint32_t)in->message[5] << 8) | (uint32_t)in->message[6];
    float lonCpr = (float)lonRaw / (float)(1UL << 17);
    float lonZoneSize = 360.0f / RADIOLIB_MAX(lonZone - (int)odd, 1);
    int lonZoneIdx = floor(this->refPos[1] / lonZoneSize) + floor((fmod(this->refPos[1], lonZoneSize) / lonZoneSize) - lonCpr + 0.5f);
    *lon = lonZoneSize * (lonZoneIdx + (float)lonCpr);

    RADIOLIB_DEBUG_PROTOCOL_PRINT("lonRaw=%d\n", lonRaw);
    RADIOLIB_DEBUG_PROTOCOL_PRINT("lonCpr=%f\n", (double)lonCpr);
    RADIOLIB_DEBUG_PROTOCOL_PRINT("lonZone=%d\n", lonZone);
    RADIOLIB_DEBUG_PROTOCOL_PRINT("lonZoneSize=%f\n", (double)lonZoneSize);
    RADIOLIB_DEBUG_PROTOCOL_PRINT("lonZoneIdx=%d\n", lonZoneIdx);
  }

  return(RADIOLIB_ERR_NONE);
}

#endif
