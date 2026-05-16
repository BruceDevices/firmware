#if !defined(_RADIOLIB_LLCC68_H)
#define _RADIOLIB_LLCC68_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_SX126X

#include "../../Module.h"
#include "../SX126x/SX1262.h"
#include "../SX126x/SX1261.h"

//RADIOLIB_SX126X_REG_VERSION_STRING
#define RADIOLIB_LLCC68_CHIP_TYPE                               "LLCC68"

/*!
  \class LLCC68
  \brief Derived class for %LLCC68 modules.
*/
class LLCC68: public SX1262 {
  public:
    /*!
      \brief Default constructor.
      \param mod Instance of Module that will be used to communicate with the radio.
    */
    LLCC68(Module* mod); // cppcheck-suppress noExplicitConstructor

    /*!
      \brief Initialization method for LoRa modem.
      \param freq Carrier frequency in MHz. Defaults to 434.0 MHz.
      \param bw LoRa bandwidth in kHz. Defaults to 125.0 kHz.
      \param sf LoRa spreading factor. Defaults to 9.
      \param cr LoRa coding rate denominator. Defaults to 7 (coding rate 4/7). Allowed values range from 4 to 8. Note that a value of 4 means no coding,
      is undocumented and not recommended without your own FEC.
      \param syncWord 1-byte LoRa sync word. Defaults to RADIOLIB_SX126X_SYNC_WORD_PRIVATE (0x12).
      \param pwr Output power in dBm. Defaults to 10 dBm.
      \param preambleLength LoRa preamble length in symbols. Defaults to 8 symbols.
      \param tcxoVoltage TCXO reference voltage to be set on DIO3. Defaults to 0 V (XTAL).
      If you are seeing -706/-707 error codes, it likely means you are using a module with TCXO.
      To use TCXO, either set this value to its reference voltage, or set SX126x::XTAL to false.
      \param useRegulatorLDO Whether to use only LDO regulator (true) or DC-DC regulator (false). Defaults to false.
      \returns \ref status_codes
    */
    int16_t begin(float freq = 434.0, float bw = 125.0, uint8_t sf = 9, uint8_t cr = 7, uint8_t syncWord = RADIOLIB_SX126X_SYNC_WORD_PRIVATE, int8_t power = 10, uint16_t preambleLength = 8, float tcxoVoltage = 0, bool useRegulatorLDO = false) override;
    
    /*!
      \brief Initialization method for FSK modem.
      \param freq Carrier frequency in MHz. Defaults to 434.0 MHz.
      \param br FSK bit rate in kbps. Defaults to 4.8 kbps.
      \param freqDev Frequency deviation from carrier frequency in kHz. Defaults to 5.0 kHz.
      \param rxBw Receiver bandwidth in kHz. Defaults to 156.2 kHz.
      \param power Output power in dBm. Defaults to 10 dBm.
      \param preambleLength FSK preamble length in bits. Defaults to 16 bits.
      \param tcxoVoltage TCXO reference voltage to be set on DIO3. Defaults to 0 V (XTAL).
      If you are seeing -706/-707 error codes, it likely means you are using non-0 value for module with XTAL.
      To use XTAL, either set this value to 0, or set SX126x::XTAL to true.
      \param useRegulatorLDO Whether to use only LDO regulator (true) or DC-DC regulator (false). Defaults to false.
      \returns \ref status_codes
    */
    int16_t beginFSK(float freq = 434.0, float br = 4.8, float freqDev = 5.0, float rxBw = 156.2, int8_t power = 10, uint16_t preambleLength = 16, float tcxoVoltage = 0, bool useRegulatorLDO = false) override;
    
    /*!
      \brief Initialization method for LR-FHSS modem. This modem only supports transmission!
      \param freq Carrier frequency in MHz. Defaults to 434.0 MHz.
      \param bw LR-FHSS bandwidth, one of RADIOLIB_SX126X_LR_FHSS_BW_* values. Defaults to 722.66 kHz.
      \param cr LR-FHSS coding rate, one of RADIOLIB_SX126X_LR_FHSS_CR_* values. Defaults to 2/3 coding rate.
      \param narrowGrid Whether to use narrow (3.9 kHz) or wide (25.39 kHz) grid spacing. Defaults to true (narrow/non-FCC) grid.
      \param power Output power in dBm. Defaults to 10 dBm.
      \param tcxoVoltage TCXO reference voltage to be set. Defaults to 0 V (XTAL).
      If you are seeing -706/-707 error codes, it likely means you are using non-0 value for module with XTAL.
      To use XTAL, either set this value to 0, or set SX126x::XTAL to true.
      \param useRegulatorLDO Whether to use only LDO regulator (true) or DC-DC regulator (false). Defaults to false.
      \returns \ref status_codes
    */
    int16_t beginLRFHSS(float freq = 434.0, uint8_t bw = RADIOLIB_SX126X_LR_FHSS_BW_722_66, uint8_t cr = RADIOLIB_SX126X_LR_FHSS_CR_2_3, bool narrowGrid = true, int8_t power = 10, float tcxoVoltage = 0, bool useRegulatorLDO = false) override;
    
    // configuration methods

    /*!
      \brief Sets LoRa bandwidth. Allowed values are 125.0, 250.0 and 500.0 kHz.
      \param bw LoRa bandwidth to be set in kHz.
      \returns \ref status_codes
    */
    int16_t setBandwidth(float bw) override;

    /*!
      \brief Sets LoRa spreading factor. Allowed values range from 5 to 11, depending on currently set spreading factor.
      \param sf LoRa spreading factor to be set.
      \returns \ref status_codes
    */
    int16_t setSpreadingFactor(uint8_t sf) override;

    /*!
      \brief Set data rate.
      \param dr Data rate struct.
      \param modem The modem corresponding to the requested datarate (FSK or LoRa). 
      Defaults to currently active modem if not supplied.
      \returns \ref status_codes
    */
    int16_t setDataRate(DataRate_t dr, ModemType_t modem = RADIOLIB_MODEM_NONE) override;
    
    /*!
      \brief Check the data rate can be configured by this module.
      \param dr Data rate struct.
      \param modem The modem corresponding to the requested datarate (FSK or LoRa). 
      Defaults to currently active modem if not supplied.
      \returns \ref status_codes
    */
    int16_t checkDataRate(DataRate_t dr, ModemType_t modem = RADIOLIB_MODEM_NONE) override;
    
    /*!
      \brief Set modem for the radio to use. Will perform full reset and reconfigure the radio
      using its default parameters.
      \param modem Modem type to set - FSK, LoRa or LR-FHSS.
      \returns \ref status_codes
    */
    int16_t setModem(ModemType_t modem) override;

#if !RADIOLIB_GODMODE
  private:
#endif

};

#endif

#endif
