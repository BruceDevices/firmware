#if !defined(RADIOLIB_LR11X0_TYPES_H)
#define RADIOLIB_LR11X0_TYPES_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_LR11X0

// MAC address length in bytes
#define RADIOLIB_LR11X0_WIFI_RESULT_MAC_LEN                     (6)

// SSID length in bytes
#define RADIOLIB_LR11X0_WIFI_RESULT_SSID_LEN                    (32)

/*!
  \struct LR11x0WifiResult_t
  \brief Structure to save result of passive WiFi scan.
  This result only saves the basic information.
*/
struct LR11x0WifiResult_t {
  /*! \brief WiFi (802.11) signal type, 'b', 'n' or 'g' */
  char type;

  /*! \brief Data rate ID holding information about modulation and coding rate. See LR11x0 user manual for details. */
  uint8_t dataRateId;

  /*! \brief Channel frequency in MHz */
  uint16_t channelFreq;

  /*! \brief MAC address origin: from gateway (1), phone (2) or undetermined (3) */
  uint8_t origin;

  /*! \brief Whether this signal was sent by an access point (true) or end device (false) */
  bool ap;

  /*! \brief RSSI in dBm */
  float rssi;

  /*! \brief MAC address */
  uint8_t mac[RADIOLIB_LR11X0_WIFI_RESULT_MAC_LEN];
};

/*!
  \struct LR11x0WifiResultFull_t
  \brief Structure to save result of passive WiFi scan.
  This result saves additional information alongside that in LR11x0WifiResult_t.
*/
struct LR11x0WifiResultFull_t: public LR11x0WifiResult_t {
  /*! \brief Frame type. See LR11x0 user manual for details. */
  uint8_t frameType;

  /*! \brief Frame sub type. See LR11x0 user manual for details. */
  uint8_t frameSubType;

  /*! \brief Frame sent from client station to distribution system. */
  bool toDistributionSystem;

  /*! \brief Frame sent from distribution system to client station. */
  bool fromDistributionSystem;

  /*! \brief See LR11x0 user manual for details. */
  uint16_t phiOffset;

  /*! \brief Number of microseconds the AP has been active. */
  uint64_t timestamp;

  /*! \brief Beacon period in microseconds. */
  uint32_t periodBeacon;
};

/*!
  \struct LR11x0WifiResultExtended_t
  \brief Structure to save result of passive WiFi scan.
  This result saves additional information alongside that in LR11x0WifiResultFull_t.
  Only scans performed with RADIOLIB_LR11X0_WIFI_ACQ_MODE_FULL_BEACON acquisition mode
  can yield this result!
*/
struct LR11x0WifiResultExtended_t: public LR11x0WifiResultFull_t {
  /*! \brief Data rate. See LR11x0 user manual for details. */
  uint8_t rate;

  /*! \brief Refer to IEEE Std 802.11, 2016, Part 11: Wireless LAN MAC and PHY Spec. */
  uint16_t service;

  /*! \brief Refer to IEEE Std 802.11, 2016, Part 11: Wireless LAN MAC and PHY Spec. */
  uint16_t length;

  /*! \brief MAC address 0 */
  uint8_t mac0[RADIOLIB_LR11X0_WIFI_RESULT_MAC_LEN];

  /*! \brief MAC address 2 */
  uint8_t mac2[RADIOLIB_LR11X0_WIFI_RESULT_MAC_LEN];

  /*! \brief Refer to IEEE Std 802.11, 2016, Part 11: Wireless LAN MAC and PHY Spec. */
  uint16_t seqCtrl;

  /*! \brief SSID */
  uint8_t ssid[RADIOLIB_LR11X0_WIFI_RESULT_SSID_LEN];

  /*! \brief WiFi channel number */
  uint8_t currentChannel;

  /*! \brief Two-letter country code (null-terminated string). */
  char countryCode[3];

  /*! \brief Refer to IEEE Std 802.11, 2016, Part 11: Wireless LAN MAC and PHY Spec. */
  uint8_t ioReg;

  /*! \brief True if frame check sequences is valid, false otherwise. */
  bool fcsCheckOk;
};

/*!
  \struct LR11x0VersionInfo_t
  \brief Structure to report information about versions of the LR11x0 hardware and firmware.
*/
struct LR11x0VersionInfo_t {
  /*! \brief Hardware revision. */
  uint8_t hardware;

  /*! \brief Which device this is - one of RADIOLIB_LR11X0_DEVICE_* macros. */
  uint8_t device;
  
  /*! \brief Major revision of the base firmware. */
  uint8_t fwMajor;
  
  /*! \brief Minor revision of the base firmware. */
  uint8_t fwMinor;

  /*! \brief Major revision of the WiFi firmware. */
  uint8_t fwMajorWiFi;
  
  /*! \brief Minor revision of the WiFi firmware. */
  uint8_t fwMinorWiFi;

  /*! \brief Revision of the GNSS firmware. */
  uint8_t fwGNSS;
  
  /*! \brief Almanac revision of the GNSS firmware. */
  uint8_t almanacGNSS;
};

/*!
  \struct LR11x0GnssResult_t
  \brief Structure to report information results of a GNSS scan.
*/
struct LR11x0GnssResult_t {
  /*! \brief Demodulator status. One of RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_* */
  int8_t demodStat;
  
  /*! \brief Number of satellites detected during the scan. */
  uint8_t numSatsDet;

  /*! \brief Result size, used when passing data to LoRa cloud. */
  uint16_t resSize;
};

/*!
  \struct LR11x0GnssPosition_t
  \brief Structure to report position from LR11x0 internal solver.
*/
struct LR11x0GnssPosition_t {
  /*! \brief Latitude in degrees. */
  float latitude;

  /*! \brief Longitude in degrees. */
  float longitude;

  /*! \brief Accuracy of this result. */
  uint16_t accuracy;

  /*! \brief Number of satellites used to solve this position. */
  uint8_t numSatsUsed;
};

/*!
  \struct LR11x0GnssSatellite_t
  \brief Structure to save information about a satellite found during GNSS scan.
*/
struct LR11x0GnssSatellite_t {
  /*! \brief Satellite vehicle (SV) identifier. */
  uint8_t svId;

  /*! \brief C/N0 in dB. */
  uint8_t c_n0;

  /*! \brief Doppler shift of the signal in Hz. */
  int16_t doppler;
};

/*!
  \struct LR11x0GnssAlmanacStatusPart_t
  \brief Structure to save information about one constellation of the GNSS almanac.
*/
struct LR11x0GnssAlmanacStatusPart_t {
  int8_t status;
  uint32_t timeUntilSubframe;
  uint8_t numSubframes;
  uint8_t nextSubframe4SvId;
  uint8_t nextSubframe5SvId;
  uint8_t nextSubframeStart;
  uint8_t numUpdateNeeded;
  uint32_t flagsUpdateNeeded[2];
  uint32_t flagsActive[2];
};

/*!
  \struct LR11x0GnssAlmanacStatus_t
  \brief Structure to save information about the GNSS almanac.
  This is not the actual almanac, just some context information about it.
*/
struct LR11x0GnssAlmanacStatus_t {
  /*! \brief GPS part of the almanac */
  LR11x0GnssAlmanacStatusPart_t gps;

  /*! \brief BeiDou part of the almanac */
  LR11x0GnssAlmanacStatusPart_t beidou;

  /*! \brief Extra flags present for BeiDou only */
  uint32_t beidouSvNoAlmanacFlags[2];

  /*! \brief Next almanac ID */
  uint8_t nextAlmanacId;

  /*! \brief Timestamp of when almanac status was retrieved - timeUntilSubframe is relative to this value. */
  RadioLibTime_t start;
};

#endif

#endif
