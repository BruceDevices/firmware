#if !defined(RADIOLIB_ADSB_H)
#define RADIOLIB_ADSB_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_ADSB

#include "../PhysicalLayer/PhysicalLayer.h"

#define RADIOLIB_ADSB_CARRIER_FREQUENCY                         (1090.0f)

// basic ADS-B frame structure
#define RADIOLIB_ADSB_FRAME_LEN_BYTES                           (14)
#define RADIOLIB_ADSB_FRAME_DF_CA_POS                           (0)
#define RADIOLIB_ADSB_FRAME_ICAO_LEN_BYTES                      (3)
#define RADIOLIB_ADSB_FRAME_ICAO_POS                            (1)
#define RADIOLIB_ADSB_FRAME_MESSAGE_LEN_BYTES                   (7)
#define RADIOLIB_ADSB_FRAME_MESSAGE_POS                         (4)
#define RADIOLIB_ADSB_FRAME_PARITY_INTERROGATOR_LEN_BYTES       (3)
#define RADIOLIB_ADSB_FRAME_PARITY_INTERROGATOR_POS             (11)

// length of the ICAO address, including a terminating null
#define RADIOLIB_ADSB_HEX_ID_LEN                                (6 + 1)

// length of the callsign, including a terminating null
#define RADIOLIB_ADSB_CALLSIGN_LEN                              (8 + 1)

/*!
  \enum ADSBMessageType
  \brief ADS-B Message Type
*/
enum class ADSBMessageType {
  AIRCRAFT_ID = 0,
  SURFACE_POS,
  AIRBORNE_POS_ALT_BARO,
  AIRBORNE_VEL,
  AIRBORNE_POS_ALT_GNSS,
  AIRCRAFT_STATUS,
  TARGET_STATE,
  AIRCRAFT_OPS_STATUS,
  RESERVED,
};

/*!
  \enum ADSBAircraftCategory
  \brief ADS-B Aircraft category
*/
enum class ADSBAircraftCategory {
  NONE = 0,
  SURFACE_EMERGENCY,
  SURFACE_SERVICE,
  GROUND_OBSTRUCTION,
  GLIDER,
  LIGHTER_THAN_AIR,
  PARACHUTIST,
  ULTRALIGHT_PARAGLIDER,
  UAV,
  SPACE_VEHICLE,
  LIGHT,
  MEDIUM_1,
  MEDIUM_2,
  HIGH_VORTEX,
  HEAVY,
  HIGH_PERFORMANCE,
  ROTORCRAFT,
  RESERVED,
};

/*!
  \struct ADSBFrame
  \brief ADS-B Frame structure
*/
struct ADSBFrame {
  public:
    /*! \brief Downlink format (DF). Received frames should always have DF = 17. */
    uint8_t downlinkFormat;

    /*! \brief Transponder capability. */
    uint8_t capability;

    /*! \brief Transponder ICAO address. */
    uint8_t icao[RADIOLIB_ADSB_FRAME_ICAO_LEN_BYTES];

    /*! \brief Message type. */
    ADSBMessageType messageType;

    /*! \brief Message buffer, interpretation is dependent on the value of messageType. */
    uint8_t message[RADIOLIB_ADSB_FRAME_MESSAGE_LEN_BYTES];
};

/*!
  \class ADSBClient
  \brief Client for receiving and decoding ADS-B broadcast.
*/
class ADSBClient {
  public:
    /*! 
      \brief Default constructor.
      \param phy Pointer to the wireless module providing PhysicalLayer communication.
    */
    explicit ADSBClient(PhysicalLayer* phy);

    /*!
      \brief Initialization method.
      \returns \ref status_codes
    */
    int16_t begin();

    /*!
      \brief Frame decoding method, turns raw received binary data into ADS-B frame.
      \param in Received raw data.
      \param out Pointer to ADSBFrame where the decoded frame will be saved.
      \returns \ref status_codes
    */
    int16_t decode(const uint8_t in[RADIOLIB_ADSB_FRAME_LEN_BYTES], ADSBFrame* out);

    /*!
      \brief Method to parse the transponder ICAO address (hex ID).
      \param in Pointer to ADSBFrame where decoded frame was saved.
      \param callsign Buffer where the parsed ID will be saved as null-terminated string.
      \returns \ref status_codes
    */
    int16_t parseHexId(const ADSBFrame* in, char id[RADIOLIB_ADSB_HEX_ID_LEN]);

    /*!
      \brief Method to parse callsign from a received frame.
      \param in Pointer to ADSBFrame where decoded frame was saved.
      \param callsign Buffer where the parsed callsign will be saved as null-terminated string.
      \param cat If set, parsed aircraft category will be saved here.
      \returns \ref status_codes
    */
    int16_t parseCallsign(const ADSBFrame* in, char callsign[RADIOLIB_ADSB_CALLSIGN_LEN], ADSBAircraftCategory* cat = NULL);

    /*!
      \brief Method to set reference position to be used during airborne position calculation.
      \param lat Reference latitude in degrees (north positive, south negative).
      \param lat Reference longitude in degrees (east positive, west negative).
    */
    void setReferencePosition(float lat, float lon);

    /*!
      \brief Parse aircraft position from incoming frame, uasing reference position set by setReferencePosition.
      The reference position is the "rough area" where the aircraft is located, typically it is the receiver location.
      Aircraft position will be calcualted accurately only within 180 nautical miles (about 333 km) from the reference position!
      \param in Pointer to ADSBFrame where decoded frame was saved.
      \param alt Pointer to variable where the parsed altitude will be saved. Can be set to null to skip altitude calculation.
      Units are feet for barometric altitude, meters for GNSS altitude.
      \param lat Pointer to variable where the parsed latitude in degrees will be saved. Can be set to null to skip latitude calculation.
      \param lon Pointer to variable where the parsed longitude in degrees will be saved. Can be set to null to skip longitude calculation.
      \param altGnss If set, this variable will be set to true if the altitude source is GNSS, or false if the altitude is barometric.
    */
    int16_t parseAirbornePosition(const ADSBFrame* in, int* alt, float* lat, float* lon, bool* altGnss = NULL);
  
#if !RADIOLIB_GODMODE
  private:
#endif
    PhysicalLayer* phyLayer;
    
    // reference position
    float refPos[2] = { 0, 0 };
};

#endif

#endif
