#if !defined(_RADIOLIB_LR1120_H)
#define _RADIOLIB_LR1120_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_LR11X0

#include "../../Module.h"
#include "LR11x0.h"

/*!
  \class LR1120
  \brief Derived class for %LR1120 modules.
*/
class LR1120: public LR11x0 {
  public:
    /*!
      \brief Default constructor.
      \param mod Instance of Module that will be used to communicate with the radio.
    */
    LR1120(Module* mod); // cppcheck-suppress noExplicitConstructor

    // basic methods

    /*!
      \brief Initialization method for LoRa modem.
      \param freq Carrier frequency in MHz. Defaults to 434.0 MHz.
      \param bw LoRa bandwidth in kHz. Defaults to 125.0 kHz.
      \param sf LoRa spreading factor. Defaults to 9.
      \param cr LoRa coding rate denominator. Defaults to 7 (coding rate 4/7). Allowed values range from 4 to 8. Note that a value of 4 means no coding,
      is undocumented and not recommended without your own FEC.
      \param syncWord 1-byte LoRa sync word. Defaults to RADIOLIB_LR11X0_LORA_SYNC_WORD_PRIVATE (0x12).
      \param power Output power in dBm. Defaults to 10 dBm.
      \param preambleLength LoRa preamble length in symbols. Defaults to 8 symbols.
      \param tcxoVoltage TCXO reference voltage to be set. Defaults to 1.6 V.
      If you are seeing -706/-707 error codes, it likely means you are using non-0 value for module with XTAL.
      To use XTAL, either set this value to 0, or set LR11x0::XTAL to true.
      \returns \ref status_codes
    */
    int16_t begin(float freq = 434.0, float bw = 125.0, uint8_t sf = 9, uint8_t cr = 7, uint8_t syncWord = RADIOLIB_LR11X0_LORA_SYNC_WORD_PRIVATE, int8_t power = 10, uint16_t preambleLength = 8, float tcxoVoltage = 1.6);

    /*!
      \brief Initialization method for FSK modem.
      \param freq Carrier frequency in MHz. Defaults to 434.0 MHz.
      \param br FSK bit rate in kbps. Defaults to 4.8 kbps.
      \param freqDev Frequency deviation from carrier frequency in kHz. Defaults to 5.0 kHz.
      \param rxBw Receiver bandwidth in kHz. Defaults to 156.2 kHz.
      \param power Output power in dBm. Defaults to 10 dBm.
      \param preambleLength FSK preamble length in bits. Defaults to 16 bits.
      \param tcxoVoltage TCXO reference voltage to be set. Defaults to 1.6 V.
      If you are seeing -706/-707 error codes, it likely means you are using non-0 value for module with XTAL.
      To use XTAL, either set this value to 0, or set LR11x0::XTAL to true.
      \returns \ref status_codes
    */
    int16_t beginGFSK(float freq = 434.0, float br = 4.8, float freqDev = 5.0, float rxBw = 156.2, int8_t power = 10, uint16_t preambleLength = 16, float tcxoVoltage = 1.6);
    
    /*!
      \brief Initialization method for LR-FHSS modem.
      \param freq Carrier frequency in MHz. Defaults to 434.0 MHz.
      \param bw LR-FHSS bandwidth, one of RADIOLIB_LRXXXX_LR_FHSS_BW_* values. Defaults to 722.66 kHz.
      \param cr LR-FHSS coding rate, one of RADIOLIB_LRXXXX_LR_FHSS_CR_* values. Defaults to 2/3 coding rate.
      \param narrowGrid Whether to use narrow (3.9 kHz) or wide (25.39 kHz) grid spacing. Defaults to true (narrow/non-FCC) grid.
      \param power Output power in dBm. Defaults to 10 dBm.
      \param tcxoVoltage TCXO reference voltage to be set. Defaults to 1.6 V.
      If you are seeing -706/-707 error codes, it likely means you are using non-0 value for module with XTAL.
      To use XTAL, either set this value to 0, or set LR11x0::XTAL to true.
      \returns \ref status_codes
    */
    int16_t beginLRFHSS(float freq = 434.0, uint8_t bw = RADIOLIB_LRXXXX_LR_FHSS_BW_722_66, uint8_t cr = RADIOLIB_LRXXXX_LR_FHSS_CR_2_3, bool narrowGrid = true, int8_t power = 10, float tcxoVoltage = 1.6);

    // configuration methods

    /*!
      \brief Sets carrier frequency. Allowed values are in range from 150.0 to 960.0 MHz,
      1900 - 2200 MHz and 2400 - 2500 MHz.
      Will automatically perform image calibration if the frequency changes by
      more than RADIOLIB_LR11X0_CAL_IMG_FREQ_TRIG MHz.
      NOTE: When switching between sub-GHz and high-frequency bands, after changing the frequency,
      setOutputPower() must be called in order to set the correct power amplifier!
      \param freq Carrier frequency to be set in MHz.
      \returns \ref status_codes
    */
    int16_t setFrequency(float freq) override;

    /*!
      \brief Sets carrier frequency. Allowed values are in range from 150.0 to 960.0 MHz,
      1900 - 2200 MHz and 2400 - 2500 MHz.
      Will automatically perform image calibration if the frequency changes by
      more than RADIOLIB_LR11X0_CAL_IMG_FREQ_TRIG MHz.
      NOTE: When switching between sub-GHz and high-frequency bands, after changing the frequency,
      setOutputPower() must be called in order to set the correct power amplifier!
      \param freq Carrier frequency to be set in MHz.
      \param skipCalibration Skip automated image calibration.
      \param band Half bandwidth for image calibration. For example,
      if carrier is 434 MHz and band is set to 4 MHz, then the image will be calibrate
      for band 430 - 438 MHz. Unused if calibrate is set to false, defaults to 4 MHz
      \returns \ref status_codes
    */
    int16_t setFrequency(float freq, bool skipCalibration, float band = 4);

    /*!
      \brief Sets output power. Allowed values are in range from -9 to 22 dBm (high-power PA) or -17 to 14 dBm (low-power PA).
      \param power Output power to be set in dBm, output PA is determined automatically preferring the low-power PA.
      \returns \ref status_codes
    */
    int16_t setOutputPower(int8_t power) override;

    /*!
      \brief Sets output power. Allowed values are in range from -9 to 22 dBm (high-power PA), -17 to 14 dBm (low-power PA)
      or -18 to 13 dBm (high-frequency PA).
      \param power Output power to be set in dBm.
      \param forceHighPower Force using the high-power PA in sub-GHz bands, or high-frequency PA in 2.4 GHz band.
      If set to false, PA will be determined automatically based on configured output power and frequency,
      preferring the low-power PA but always using high-frequency PA in 2.4 GHz band.
      Ignored when operating in 2.4 GHz band.
      \param rampTimeUs PA power ramping time in microseconds. Provided value is rounded up to the
      nearest discrete ramp time supported by the PA. Defaults to 48 us.
      \returns \ref status_codes
    */
    int16_t setOutputPower(int8_t power, bool forceHighPower, uint32_t rampTimeUs = 48);

    /*!
      \brief Check if output power is configurable.
      This method is needed for compatibility with PhysicalLayer::checkOutputPower.
      \param power Output power in dBm, PA will be determined automatically.
      \param clipped Clipped output power value to what is possible within the module's range.
      \returns \ref status_codes
    */
    int16_t checkOutputPower(int8_t power, int8_t* clipped) override;

    /*!
      \brief Check if output power is configurable.
      \param power Output power in dBm.
      \param clipped Clipped output power value to what is possible within the module's range.
      \param forceHighPower Force using the high-power PA. If set to false, PA will be determined automatically
      based on configured output power, preferring the low-power PA. If set to true, only high-power PA will be used.
      Ignored when operating in 2.4 GHz band.
      \returns \ref status_codes
    */
    int16_t checkOutputPower(int8_t power, int8_t* clipped, bool forceHighPower);

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
    // flag to determine whether we are in the sub-GHz or 2.4 GHz range
    // this is needed to automatically detect which PA to use
    bool highFreq = false;

};

#endif

#endif
