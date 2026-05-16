#if !defined(_RADIOLIB_SX1268_H)
#define _RADIOLIB_SX1268_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_SX126X

#include "../../Module.h"
#include "SX126x.h"

//RADIOLIB_SX126X_CMD_SET_PA_CONFIG
#define RADIOLIB_SX126X_PA_CONFIG_SX1268                        0x00

//RADIOLIB_SX126X_REG_VERSION_STRING
#define RADIOLIB_SX1268_CHIP_TYPE                               "SX1268"

/*!
  \class SX1268
  \brief Derived class for %SX1268 modules.
*/
class SX1268: public SX126x {
  public:
    /*!
      \brief Default constructor.
      \param mod Instance of Module that will be used to communicate with the radio.
    */
    SX1268(Module* mod); // cppcheck-suppress noExplicitConstructor

    // basic methods

    /*!
      \brief Initialization method for LoRa modem.
      \param freq Carrier frequency in MHz. Defaults to 434.0 MHz.
      \param bw LoRa bandwidth in kHz. Defaults to 125.0 kHz.
      \param sf LoRa spreading factor. Defaults to 9.
      \param cr LoRa coding rate denominator. Defaults to 7 (coding rate 4/7). Allowed values range from 4 to 8. 
      Note that a value of 4 means no coding, is undocumented and not recommended without your own FEC.
      \param syncWord 1-byte LoRa sync word. Defaults to RADIOLIB_SX126X_SYNC_WORD_PRIVATE (0x12).
      \param power Output power in dBm. Defaults to 10 dBm.
      \param preambleLength LoRa preamble length in symbols. Defaults to 8 symbols.
      \param tcxoVoltage TCXO reference voltage to be set on DIO3. Defaults to 1.6 V.
      If you are seeing -706/-707 error codes, it likely means you are using non-0 value for module with XTAL.
      To use XTAL, either set this value to 0, or set SX126x::XTAL to true.
      \param useRegulatorLDO Whether to use only LDO regulator (true) or DC-DC regulator (false). Defaults to false.
      \returns \ref status_codes
    */
    int16_t begin(float freq = 434.0, float bw = 125.0, uint8_t sf = 9, uint8_t cr = 7, uint8_t syncWord = RADIOLIB_SX126X_SYNC_WORD_PRIVATE, int8_t power = 10, uint16_t preambleLength = 8, float tcxoVoltage = 1.6, bool useRegulatorLDO = false);

    /*!
      \brief Initialization method for FSK modem.
      \param freq Carrier frequency in MHz. Defaults to 434.0 MHz.
      \param br FSK bit rate in kbps. Defaults to 4.8 kbps.
      \param freqDev Frequency deviation from carrier frequency in kHz. Defaults to 5.0 kHz.
      \param rxBw Receiver bandwidth in kHz. Defaults to 156.2 kHz.
      \param power Output power in dBm. Defaults to 10 dBm.
      \param preambleLength FSK preamble length in bits. Defaults to 16 bits.
      \param tcxoVoltage TCXO reference voltage to be set on DIO3. Defaults to 1.6 V.
      If you are seeing -706/-707 error codes, it likely means you are using non-0 value for module with XTAL.
      To use XTAL, either set this value to 0, or set SX126x::XTAL to true.
      \param useRegulatorLDO Whether to use only LDO regulator (true) or DC-DC regulator (false). Defaults to false.
      \returns \ref status_codes
    */
    int16_t beginFSK(float freq = 434.0, float br = 4.8, float freqDev = 5.0, float rxBw = 156.2, int8_t power = 10, uint16_t preambleLength = 16, float tcxoVoltage = 1.6, bool useRegulatorLDO = false);
    
    /*!
      \brief Initialization method for BPSK modem.
      NOTE: Proceed with caution! BPSK support in SX126x is epxerimental and poorly documented!
      \param freq Carrier frequency in MHz. Defaults to 434.0 MHz.
      \param br FSK bit rate in kbps. Defaults to 600 bps, only 100 and 600 bps is supported.
      \param power Output power in dBm. Defaults to 10 dBm.
      \param tcxoVoltage TCXO reference voltage to be set on DIO3. Defaults to 1.6 V.
      If you are seeing -706/-707 error codes, it likely means you are using non-0 value for module with XTAL.
      To use XTAL, either set this value to 0, or set SX126x::XTAL to true.
      \param useRegulatorLDO Whether to use only LDO regulator (true) or DC-DC regulator (false). Defaults to false.
      \returns \ref status_codes
    */
    virtual int16_t beginBPSK(float freq = 434.0, float br = 0.6, int8_t power = 10, float tcxoVoltage = 1.6, bool useRegulatorLDO = false);
    
    /*!
      \brief Initialization method for LR-FHSS modem. This modem only supports transmission!
      \param freq Carrier frequency in MHz. Defaults to 434.0 MHz.
      \param bw LR-FHSS bandwidth, one of RADIOLIB_SX126X_LR_FHSS_BW_* values. Defaults to 722.66 kHz.
      \param cr LR-FHSS coding rate, one of RADIOLIB_SX126X_LR_FHSS_CR_* values. Defaults to 2/3 coding rate.
      \param narrowGrid Whether to use narrow (3.9 kHz) or wide (25.39 kHz) grid spacing. Defaults to true (narrow/non-FCC) grid.
      \param power Output power in dBm. Defaults to 10 dBm.
      \param tcxoVoltage TCXO reference voltage to be set. Defaults to 1.6 V.
      If you are seeing -706/-707 error codes, it likely means you are using non-0 value for module with XTAL.
      To use XTAL, either set this value to 0, or set SX126x::XTAL to true.
      \param useRegulatorLDO Whether to use only LDO regulator (true) or DC-DC regulator (false). Defaults to false.
      \returns \ref status_codes
    */
    int16_t beginLRFHSS(float freq = 434.0, uint8_t bw = RADIOLIB_SX126X_LR_FHSS_BW_722_66, uint8_t cr = RADIOLIB_SX126X_LR_FHSS_CR_2_3, bool narrowGrid = true, int8_t power = 10, float tcxoVoltage = 1.6, bool useRegulatorLDO = false);
    
    // configuration methods

    /*!
      \brief Sets carrier frequency. Allowed values are in range from 410.0 to 810.0 MHz.
      Will automatically perform image calibration if the frequency changes by
      more than RADIOLIB_SX126X_CAL_IMG_FREQ_TRIG MHz.
      \param freq Carrier frequency to be set in MHz.
      \returns \ref status_codes
    */
    int16_t setFrequency(float freq) override;

    /*!
      \brief Sets carrier frequency. Allowed values are in range from 150.0 to 960.0 MHz.
      Will automatically perform image calibration if the frequency changes by
      more than RADIOLIB_SX126X_CAL_IMG_FREQ_TRIG_MHZ.
      \param freq Carrier frequency to be set in MHz.
      \param skipCalibration Skip automated image calibration.
      \returns \ref status_codes
    */
    int16_t setFrequency(float freq, bool skipCalibration);

    /*!
      \brief Sets output power. Allowed values are in range from -9 to 22 dBm.
      \param power Output power to be set in dBm.
      \returns \ref status_codes
    */
    int16_t setOutputPower(int8_t power) override;

    /*!
      \brief Sets output power. Allowed values are in range from -9 to 22 dBm.
      \param power Output power to be set in dBm.
      \param optimize Whether to use power-optimized PA configuration (true) or datasheet default (false).
      \returns \ref status_codes
    */
    int16_t setOutputPower(int8_t power, bool optimize);

    /*!
      \brief Check if output power is configurable.
      \param power Output power in dBm.
      \param clipped Clipped output power value to what is possible within the module's range.
      \returns \ref status_codes
    */
    int16_t checkOutputPower(int8_t power, int8_t* clipped) override;
    
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
