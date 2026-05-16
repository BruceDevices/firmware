#if !defined(RADIOLIB_LR2021_H)
#define RADIOLIB_LR2021_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_LR2021

#include "../../Module.h"

#include "../../protocols/PhysicalLayer/PhysicalLayer.h"
#include "../LR11x0/LR_common.h"
#include "LR2021_commands.h"
#include "LR2021_types.h"

// LR2021 physical layer properties
#define RADIOLIB_LR2021_FREQUENCY_STEP_SIZE                     1.0
#define RADIOLIB_LR2021_MAX_PACKET_LENGTH                       255
#define RADIOLIB_LR2021_CRYSTAL_FREQ                            32.0
#define RADIOLIB_LR2021_DIV_EXPONENT                            25

/*!
  \class LR2021
  \brief 
*/
class LR2021: public LRxxxx {
  public:
    // introduce PhysicalLayer overloads
    using PhysicalLayer::transmit;
    using PhysicalLayer::receive;
    using PhysicalLayer::startTransmit;
    using PhysicalLayer::startReceive;
    using PhysicalLayer::readData;

    /*!
      \brief Default constructor.
      \param mod Instance of Module that will be used to communicate with the radio.
    */
    LR2021(Module* mod); // cppcheck-suppress noExplicitConstructor

    /*!
      \brief Which DIO pin is to be used as the interrupt pin.
    */
    uint32_t irqDioNum = 5;

    /*!
      \brief Custom operation modes for LR2021.
      Needed because LR2021 has several modems (sub-GHz, 2.4 GHz etc.) in one package
    */
    enum OpMode_t {
        /*! End of table marker, use \ref END_OF_MODE_TABLE constant instead */
        MODE_END_OF_TABLE = Module::MODE_END_OF_TABLE,
        /*! Standby/idle mode */
        MODE_STBY = Module::MODE_IDLE,
        /*! Receive mode */
        MODE_RX = Module::MODE_RX,
        /*! Transmission mode */
        MODE_TX = Module::MODE_TX,
        /*! High frequency receive mode */
        MODE_RX_HF,
        /*! High frequency transmission mode */
        MODE_TX_HF,
    };

    // basic methods

    /*!
      \brief Initialization method for LoRa modem.
      \param freq Carrier frequency in MHz. Defaults to 434.0 MHz.
      \param bw LoRa bandwidth in kHz. Defaults to 125.0 kHz.
      \param sf LoRa spreading factor. Defaults to 9.
      \param cr LoRa coding rate denominator. Defaults to 7 (coding rate 4/7). Allowed values range from 4 to 8. Note that a value of 4 means no coding,
      is undocumented and not recommended without your own FEC.
      \param syncWord 1-byte LoRa sync word. Defaults to RADIOLIB_LR2021_LORA_SYNC_WORD_PRIVATE (0x12).
      \param power Output power in dBm. Defaults to 10 dBm.
      \param preambleLength LoRa preamble length in symbols. Defaults to 8 symbols.
      \param tcxoVoltage TCXO reference voltage to be set. Defaults to 1.6 V.
      If you are seeing -706/-707 error codes, it likely means you are using non-0 value for module with XTAL.
      To use XTAL, either set this value to 0, or set LR2021::XTAL to true.
      \returns \ref status_codes
    */
    int16_t begin(float freq = 434.0, float bw = 125.0, uint8_t sf = 9, uint8_t cr = 7, uint8_t syncWord = RADIOLIB_LR2021_LORA_SYNC_WORD_PRIVATE, int8_t power = 10, uint16_t preambleLength = 8, float tcxoVoltage = 1.6);

    /*!
      \brief Initialization method for FSK modem.
      \param freq Carrier frequency in MHz. Defaults to 434.0 MHz.
      \param br FSK bit rate in kbps. Defaults to 4.8 kbps.
      \param freqDev Frequency deviation from carrier frequency in kHz. Defaults to 5.0 kHz.
      \param rxBw Receiver bandwidth in kHz. Defaults to 153.8 kHz.
      \param power Output power in dBm. Defaults to 10 dBm.
      \param preambleLength FSK preamble length in bits. Defaults to 16 bits.
      \param tcxoVoltage TCXO reference voltage to be set. Defaults to 1.6 V.
      If you are seeing -706/-707 error codes, it likely means you are using non-0 value for module with XTAL.
      To use XTAL, either set this value to 0, or set LR2021::XTAL to true.
      \returns \ref status_codes
    */
    int16_t beginGFSK(float freq = 434.0, float br = 4.8, float freqDev = 5.0, float rxBw = 153.8, int8_t power = 10, uint16_t preambleLength = 16, float tcxoVoltage = 1.6);
    
    /*!
      \brief Initialization method for OOK modem.
      \param freq Carrier frequency in MHz. Defaults to 434.0 MHz.
      \param br OOK bit rate in kbps. Defaults to 4.8 kbps.
      \param rxBw Receiver bandwidth in kHz. Defaults to 153.8 kHz.
      \param power Output power in dBm. Defaults to 10 dBm.
      \param preambleLength OOK preamble length in bits. Defaults to 16 bits.
      \param tcxoVoltage TCXO reference voltage to be set. Defaults to 1.6 V.
      If you are seeing -706/-707 error codes, it likely means you are using non-0 value for module with XTAL.
      To use XTAL, either set this value to 0, or set LR2021::XTAL to true.
      \returns \ref status_codes
    */
    int16_t beginOOK(float freq = 434.0, float br = 4.8, float rxBw = 153.8, int8_t power = 10, uint16_t preambleLength = 16, float tcxoVoltage = 1.6);
    
    /*!
      \brief Initialization method for LR-FHSS modem.
      \param freq Carrier frequency in MHz. Defaults to 434.0 MHz.
      \param bw LR-FHSS bandwidth, one of RADIOLIB_LRXXXX_LR_FHSS_BW_* values. Defaults to 722.66 kHz.
      \param cr LR-FHSS coding rate, one of RADIOLIB_LRXXXX_LR_FHSS_CR_* values. Defaults to 2/3 coding rate.
      \param narrowGrid Whether to use narrow (3.9 kHz) or wide (25.39 kHz) grid spacing. Defaults to true (narrow/non-FCC) grid.
      \param power Output power in dBm. Defaults to 10 dBm.
      \param tcxoVoltage TCXO reference voltage to be set. Defaults to 1.6 V.
      If you are seeing -706/-707 error codes, it likely means you are using non-0 value for module with XTAL.
      To use XTAL, either set this value to 0, or set LR2021::XTAL to true.
      \returns \ref status_codes
    */
    int16_t beginLRFHSS(float freq = 434.0, uint8_t bw = RADIOLIB_LRXXXX_LR_FHSS_BW_722_66, uint8_t cr = RADIOLIB_LRXXXX_LR_FHSS_CR_2_3, bool narrowGrid = true, int8_t power = 10, float tcxoVoltage = 1.6);

    /*!
      \brief Initialization method for FLRC modem.
      \param freq Carrier frequency in MHz. Defaults to 434.0 MHz.
      \param br FLRC bit rate in kbps. Defaults to 650 kbps.
      \param cr FLRC coding rate. Defaults to RADIOLIB_LR2021_FLRC_CR_2_3 (coding rate 2/3).
      \param pwr Output power in dBm. Defaults to 10 dBm.
      \param preambleLength FLRC preamble length in bits. Defaults to 16 bits.
      \param dataShaping Time-bandwidth product of the Gaussian filter to be used for shaping. Defaults to 0.5.
      \returns \ref status_codes
    */
    int16_t beginFLRC(float freq = 434.0, uint16_t br = 650, uint8_t cr = RADIOLIB_LR2021_FLRC_CR_2_3, int8_t pwr = 10, uint16_t preambleLength = 16, uint8_t dataShaping = RADIOLIB_SHAPING_0_5, float tcxoVoltage = 1.6);

    /*!
      \brief Blocking binary transmit method.
      Overloads for string-based transmissions are implemented in PhysicalLayer.
      \param data Binary data to be sent.
      \param len Number of bytes to send.
      \param addr Address to send the data to. Will only be added if address filtering was enabled.
      \returns \ref status_codes
    */
    int16_t transmit(const uint8_t* data, size_t len, uint8_t addr = 0) override;

    /*!
      \brief Blocking binary receive method.
      Overloads for string-based transmissions are implemented in PhysicalLayer.
      \param data Pointer to array to save the received binary data.
      \param len Number of bytes that will be received. Must be known in advance for binary transmissions.
      \param timeout Reception timeout in milliseconds. If set to 0,
      timeout period will be calculated automatically based on the radio configuration.
      \returns \ref status_codes
    */
    int16_t receive(uint8_t* data, size_t len, RadioLibTime_t timeout = 0) override;

    /*!
      \brief Starts direct mode transmission.
      \param frf Raw RF frequency value. Defaults to 0, required for quick frequency shifts in RTTY.
      \returns \ref status_codes
    */
    int16_t transmitDirect(uint32_t frf = 0) override;

    /*!
      \brief Starts direct mode reception. Only implemented for PhysicalLayer compatibility, as LR2021 does not support direct mode reception.
      Will always return RADIOLIB_ERR_UNKNOWN.
      \returns \ref status_codes
    */
    int16_t receiveDirect() override;

    /*!
      \brief Performs scan for LoRa transmission in the current channel. Detects both preamble and payload.
      \returns \ref status_codes
    */
    int16_t scanChannel() override;

    /*!
      \brief Performs scan for LoRa transmission in the current channel. Detects both preamble and payload.
      \param config CAD configuration structure.
      \returns \ref status_codes
    */
    int16_t scanChannel(const ChannelScanConfig_t &config) override;

    /*!
      \brief Sets the module to standby mode (overload for PhysicalLayer compatibility, uses 13 MHz RC oscillator).
      \returns \ref status_codes
    */
    int16_t standby() override;

    /*!
      \brief Sets the module to standby mode.
      \param mode Oscillator to be used in standby mode. Can be set to RADIOLIB_LR2021_STANDBY_RC (13 MHz RC oscillator)
      or RADIOLIB_LR2021_STANDBY_XOSC (32 MHz external crystal oscillator).
      \returns \ref status_codes
    */
    int16_t standby(uint8_t mode) override;

    /*!
      \brief Sets the module to standby mode.
      \param mode Oscillator to be used in standby mode. Can be set to RADIOLIB_LR2021_STANDBY_RC (13 MHz RC oscillator)
      or RADIOLIB_LR2021_STANDBY_XOSC (32 MHz external crystal oscillator).
      \param wakeup Whether to force the module to wake up. Setting to true will immediately attempt to wake up the module.
      \returns \ref status_codes
    */
    int16_t standby(uint8_t mode, bool wakeup);

    /*!
      \brief Sets the module to sleep mode. To wake the device up, call standby().
      Overload with warm start enabled for PhysicalLayer compatibility.
      \returns \ref status_codes
    */
    int16_t sleep() override;

    /*!
      \brief Sets the module to sleep mode. To wake the device up, call standby().
      \param retainConfig Set to true to retain configuration of the currently active modem ("warm start")
      or to false to discard current configuration ("cold start"). Defaults to true.
      \param sleepTime Sleep duration (enables automatic wakeup), in multiples of 30.52 us. Ignored if set to 0.
      \returns \ref status_codes
    */
    int16_t sleep(bool retainConfig, uint32_t sleepTime);

    /*!
      \brief Query modem for the packet length of received payload.
      \param update Update received packet length. Will return cached value when set to false.
      \returns Length of last received packet in bytes.
    */
    size_t getPacketLength(bool update = true) override;

    // interrupt methods

    /*!
      \brief Clean up after transmission is done.
      \returns \ref status_codes
    */
    int16_t finishTransmit() override;

    /*!
      \brief Interrupt-driven receive method with default parameters.
      Implemented for compatibility with PhysicalLayer.

      \returns \ref status_codes
    */
    int16_t startReceive() override;

    /*!
      \brief Reads data received after calling startReceive method. When the packet length is not known in advance,
      getPacketLength method must be called BEFORE calling readData!
      \param data Pointer to array to save the received binary data.
      \param len Number of bytes that will be read. When set to 0, the packet length will be retrieved automatically.
      When more bytes than received are requested, only the number of bytes requested will be returned.
      \returns \ref status_codes
    */
    int16_t readData(uint8_t* data, size_t len) override;

    /*!
      \brief Clean up after reception is done.
      \returns \ref status_codes
    */
    int16_t finishReceive() override;
    
    /*!
      \brief Interrupt-driven channel activity detection method. IRQ1 will be activated
      when LoRa preamble is detected, or upon timeout. Defaults to CAD parameter values recommended by AN1200.48.
      \returns \ref status_codes
    */
    int16_t startChannelScan() override;

    /*!
      \brief Interrupt-driven channel activity detection method. IRQ pin will be activated
      when LoRa preamble is detected, or upon timeout.
      \param config CAD configuration structure.
      \returns \ref status_codes
    */
    int16_t startChannelScan(const ChannelScanConfig_t &config) override;

    /*!
      \brief Read the channel scan result
      \returns \ref status_codes
    */
    int16_t getChannelScanResult() override;

    /*!
      \brief Read currently active IRQ flags.
      \returns IRQ flags.
    */
    uint32_t getIrqFlags() override;

    /*!
      \brief Set interrupt on DIO1 to be sent on a specific IRQ bit (e.g. RxTimeout, CadDone).
      \param irq Module-specific IRQ flags.
      \returns \ref status_codes
    */
    int16_t setIrqFlags(uint32_t irq) override;

    /*!
      \brief Clear interrupt on a specific IRQ bit (e.g. RxTimeout, CadDone).
      \param irq Module-specific IRQ flags.
      \returns \ref status_codes
    */
    int16_t clearIrqFlags(uint32_t irq) override;
    
    /*!
      \brief Set modem for the radio to use. Will perform full reset and reconfigure the radio
      using its default parameters.
      \param modem Modem type to set - FSK, LoRa or LR-FHSS.
      \returns \ref status_codes
    */
    int16_t setModem(ModemType_t modem) override;
    
    // configuration methods

    /*!
      \brief Sets carrier frequency. Allowed values are in range from 150.0 to 960.0 MHz,
      1900 - 2200 MHz and 2400 - 2500 MHz.
      Will automatically perform image calibration if the frequency changes by
      more than RADIOLIB_LR2021_CAL_IMG_FREQ_TRIG MHz.
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
      more than RADIOLIB_LR2021_CAL_IMG_FREQ_TRIG MHz.
      NOTE: When switching between sub-GHz and high-frequency bands, after changing the frequency,
      setOutputPower() must be called in order to set the correct power amplifier!
      \param freq Carrier frequency to be set in MHz.
      \param skipCalibration Skip automated image calibration.
      \returns \ref status_codes
    */
    int16_t setFrequency(float freq, bool skipCalibration);

    /*!
      \brief Sets output power. Allowed values are in range from -9 to 22 dBm (sub-GHz PA) or -19 to 12 dBm (high-frequency PA).
      \param power Output power to be set in dBm.
      \returns \ref status_codes
    */
    int16_t setOutputPower(int8_t power) override;

    /*!
      \brief Sets output power. Allowed values are in range from -9 to 22 dBm (sub-GHz PA) or -19 to 12 dBm (high-frequency PA).
      \param power Output power to be set in dBm.
      \param rampTimeUs PA power ramping time in microseconds. Provided value is rounded up to the
      nearest discrete ramp time supported by the PA.
      \returns \ref status_codes
    */
    int16_t setOutputPower(int8_t power, uint32_t rampTimeUs);

    /*!
      \brief Check if output power is configurable.
      This method is needed for compatibility with PhysicalLayer::checkOutputPower.
      \param power Output power in dBm, PA will be determined automatically.
      \param clipped Clipped output power value to what is possible within the module's range.
      \returns \ref status_codes
    */
    int16_t checkOutputPower(int8_t power, int8_t* clipped) override;
    
    /*! \copydoc Module::setRfSwitchTable */
    void setRfSwitchTable(const uint32_t (&pins)[Module::RFSWITCH_MAX_PINS], const Module::RfSwitchMode_t table[]);

    /*!
      \brief Sets LoRa bandwidth. Allowed values are 31.25, 41.67, 62.5, 83.34, 125.0, 
      101.56, 203.13, 250.0, 406.25, 500.0 kHz, 812.5 kHz and 1000.0 kHz.
      \param bw LoRa bandwidth to be set in kHz.
      \returns \ref status_codes
    */
    int16_t setBandwidth(float bw);

    /*!
      \brief Sets LoRa spreading factor. Allowed values range from 5 to 12.
      \param sf LoRa spreading factor to be set.
      \param legacy Enable legacy mode for SF6 - this allows to communicate with SX127x at SF6.
      \returns \ref status_codes
    */
    int16_t setSpreadingFactor(uint8_t sf, bool legacy = false);

    /*!
      \brief Sets LoRa coding rate denominator. Allowed values range from 4 to 8. Note that a value of 4 means no coding, 
      is undocumented and not recommended without your own FEC.
      \param cr LoRa coding rate denominator to be set.
      \param longInterleave Enable long interleaver when set to true.
      Note that with long interleaver enabled, CR 4/7 is not possible, there are packet length restrictions,
      and it is not compatible with SX127x radios!
      \returns \ref status_codes
    */
    int16_t setCodingRate(uint8_t cr, bool longInterleave = false);

    /*!
      \brief Sets LoRa sync word.
      \param syncWord LoRa sync word to be set.
      \returns \ref status_codes
    */
    int16_t setSyncWord(uint8_t syncWord);

    /*!
      \brief Sets preamble length for LoRa or GFSK modem. Allowed values range from 1 to 65535.
      \param preambleLength Preamble length to be set in symbols (LoRa) or bits (GFSK).
      \returns \ref status_codes
    */
    int16_t setPreambleLength(size_t preambleLength) override;

    /*!
      \brief Sets TCXO (Temperature Compensated Crystal Oscillator) configuration.
      \param voltage TCXO reference voltage in volts. Allowed values are 1.6, 1.7, 1.8, 2.2. 2.4, 2.7, 3.0 and 3.3 V.
      Set to 0 to disable TCXO.
      NOTE: After setting this parameter to 0, the module will be reset (since there's no other way to disable TCXO).
      \param delay TCXO timeout in us. Defaults to 1000000 (1 second), because especially on the first startup,
      this delay may be measured very inaccurately.
      \returns \ref status_codes
    */
    int16_t setTCXO(float voltage, uint32_t delay = 1000000);

    /*!
      \brief Sets CRC configuration.
      \param len CRC length in bytes, Allowed values are 1 or 2, set to 0 to disable CRC.
      \param initial Initial CRC value. GFSK only. Defaults to 0x1D0F (CCITT CRC).
      \param polynomial Polynomial for CRC calculation. GFSK only. Defaults to 0x1021 (CCITT CRC).
      \param inverted Invert CRC bytes. GFSK only. Defaults to true (CCITT CRC).
      \returns \ref status_codes
    */
    int16_t setCRC(uint8_t len, uint32_t initial = 0x00001D0FUL, uint32_t polynomial = 0x00001021UL, bool inverted = true);

    /*!
      \brief Enable/disable inversion of the I and Q signals
      \param enable IQ inversion enabled (true) or disabled (false);
      \returns \ref status_codes
    */
    int16_t invertIQ(bool enable) override;

    /*!
      \brief Sets GFSK bit rate. Allowed values range from 0.5 to 2000.0 kbps.
      \param br FSK bit rate to be set in kbps.
      \returns \ref status_codes
    */
    int16_t setBitRate(float br) override;

    /*!
      \brief Sets GFSK frequency deviation. Allowed values range from 0.6 to 500.0 kHz.
      \param freqDev GFSK frequency deviation to be set in kHz.
      \returns \ref status_codes
    */
    int16_t setFrequencyDeviation(float freqDev) override;

    /*!
      \brief Sets GFSK receiver bandwidth. Allowed values are 4.8, 5.8, 7.3, 9.7, 11.7, 14.6, 19.5,
      23.4, 29.3, 39.0, 46.9, 58.6, 78.2, 93.8, 117.3, 156.2, 187.2, 234.3, 312.0, 373.6 and 467.0 kHz.
      \param rxBw GFSK receiver bandwidth to be set in kHz.
      \returns \ref status_codes
    */
    int16_t setRxBandwidth(float rxBw);
    
    /*!
      \brief Sets GFSK sync word in the form of array of up to 8 bytes.
      \param syncWord GFSK sync word to be set.
      \param len GFSK sync word length in bytes.
      \returns \ref status_codes
    */
    int16_t setSyncWord(uint8_t* syncWord, size_t len) override;

    /*!
      \brief Sets node address. Calling this method will also enable address filtering for node address only.
      \param nodeAddr Node address to be set.
      \returns \ref status_codes
    */
    int16_t setNodeAddress(uint8_t nodeAddr);

    /*!
      \brief Sets broadcast address. Calling this method will also enable address
      filtering for node and broadcast address.
      \param broadAddr Node address to be set.
      \returns \ref status_codes
    */
    int16_t setBroadcastAddress(uint8_t broadAddr);

    /*!
      \brief Disables address filtering. Calling this method will also erase previously set addresses.
      \returns \ref status_codes
    */
    int16_t disableAddressFiltering();

    /*!
      \brief Sets time-bandwidth product of Gaussian filter applied for shaping.
      Allowed values are RADIOLIB_SHAPING_0_3, RADIOLIB_SHAPING_0_5, RADIOLIB_SHAPING_0_7 or RADIOLIB_SHAPING_1_0.
      Set to RADIOLIB_SHAPING_NONE to disable data shaping.
      \param sh Time-bandwidth product of Gaussian filter to be set.
      \returns \ref status_codes
    */
    int16_t setDataShaping(uint8_t sh) override;

    /*!
      \brief Sets transmission encoding. Available in GFSK mode only. Serves only as alias for PhysicalLayer compatibility.
      \param encoding Encoding to be used. Set to 0 for NRZ, and 2 for whitening.
      \returns \ref status_codes
    */
    int16_t setEncoding(uint8_t encoding) override;

    /*!
      \brief Set modem in fixed packet length mode. Available in GFSK mode only.
      \param len Packet length.
      \returns \ref status_codes
    */
    int16_t fixedPacketLengthMode(uint8_t len = RADIOLIB_LR2021_MAX_PACKET_LENGTH);

    /*!
      \brief Set modem in variable packet length mode. Available in GFSK mode only.
      \param maxLen Maximum packet length.
      \returns \ref status_codes
    */
    int16_t variablePacketLengthMode(uint8_t maxLen = RADIOLIB_LR2021_MAX_PACKET_LENGTH);

    /*!
      \brief Sets GFSK whitening parameters.
      \param enabled True = Whitening enabled
      \param initial Initial value used for the whitening LFSR in GFSK mode.
      By default set to 0x01FF for compatibility with SX127x and LoRaWAN.
      \returns \ref status_codes
    */
    int16_t setWhitening(bool enabled, uint16_t initial = 0x01FF);

    /*!
      \brief Set data rate.
      \param dr Data rate struct.
      \param modem The modem corresponding to the requested datarate (FSK, LoRa or LR-FHSS). 
      Defaults to currently active modem if not supplied.
      \returns \ref status_codes
    */
    int16_t setDataRate(DataRate_t dr, ModemType_t modem = RADIOLIB_MODEM_NONE) override;

    /*!
      \brief Check the data rate can be configured by this module.
      \param dr Data rate struct.
      \param modem The modem corresponding to the requested datarate (FSK, LoRa or LR-FHSS). 
      Defaults to currently active modem if not supplied.
      \returns \ref status_codes
    */
    int16_t checkDataRate(DataRate_t dr, ModemType_t modem = RADIOLIB_MODEM_NONE) override;

    /*!
      \brief Sets LR-FHSS configuration.
      \param bw LR-FHSS bandwidth, one of RADIOLIB_LRXXXX_LR_FHSS_BW_* values.
      \param cr LR-FHSS coding rate, one of RADIOLIB_LRXXXX_LR_FHSS_CR_* values.
      \param hdrCount Header packet count, 1 - 4. Defaults to 3.
      \param hopSeed 9-bit seed number for PRNG generation of the hopping sequence. Defaults to 0x13A.
      \returns \ref status_codes
    */
    int16_t setLrFhssConfig(uint8_t bw, uint8_t cr, uint8_t hdrCount = 3, uint16_t hopSeed = 0x13A);

    /*!
      \brief Enables or disables Rx Boosted Gain mode (additional Rx gain for increased power consumption).
      \param level Rx gain boost level. 0 (disabled) to 7 (maximum boost).
      \returns \ref status_codes
    */
    int16_t setRxBoostedGainMode(uint8_t level);

    /*!
      \brief Get expected time-on-air for a given size of payload
      \param len Payload length in bytes.
      \returns Expected time-on-air in microseconds.
    */
    RadioLibTime_t getTimeOnAir(size_t len) override;

    /*!
      \brief Get modem currently in use by the radio.
      \param modem Pointer to a variable to save the retrieved configuration into.
      \returns \ref status_codes
    */
    int16_t getModem(ModemType_t* modem) override;

    /*! \copydoc PhysicalLayer::stageMode */
    int16_t stageMode(RadioModeType_t mode, RadioModeConfig_t* cfg) override;

    /*! \copydoc PhysicalLayer::launchMode */
    int16_t launchMode() override;

    /*!
      \brief Read the supply voltage on the Vbat pin.
      \param bits Measurement resolution in bits, 8 to 13.
      \returns \ref Supply voltage in volts.
    */
    float getVoltage(uint8_t bits = 13);
    
    /*!
      \brief Read the temperature.
      \param source Measurement source, one of RADIOLIB_LR2021_TEMP_SOURCE_* macros.
      \param bits Measurement resolution in bits, 8 to 13.
      \returns \ref Temperature in degrees Celsius.
    */
    float getTemperature(uint8_t source, uint8_t bits = 13);

    /*!
      \brief Gets recorded signal strength indicator.
      Overload with packet mode enabled for PhysicalLayer compatibility.
      \returns RSSI value in dBm.
    */
    float getRSSI() override;

    /*!
      \brief Gets RSSI (Recorded Signal Strength Indicator).
      \param packet Whether to read last packet RSSI, or the current value.
      \param skipReceive Set to true to skip putting radio in receive mode for the RSSI measurement in FSK/OOK mode.
      \returns RSSI value in dBm.
    */
    float getRSSI(bool packet, bool skipReceive = false);

    /*!
      \brief Gets SNR (Signal to Noise Ratio) of the last received packet. Only available for LoRa modem.
      \returns SNR of the last received packet in dB.
    */
    float getSNR() override;

    /*!
      \brief Get one truly random byte from RSSI noise.
      \returns TRNG byte.
    */
    uint8_t randomByte() override;

    /*!
      \brief Set implicit header mode for future reception/transmission.
      \param len Payload length in bytes.
      \returns \ref status_codes
    */
    int16_t implicitHeader(size_t len);

    /*!
      \brief Set explicit header mode for future reception/transmission.
      \returns \ref status_codes
    */
    int16_t explicitHeader();

    /*!
      \brief Set OOK detector properties. The default values are set to allow ADS-B reception.
      \param pattern Preamble pattern, should end with 01 or 10 (binary).
      \param len Preamble pattern length in bits.
      \param repeats Number of preamble repeats, maximum of 31.
      \param syncRaw Whether the sync word is send raw (unencoded) or encoded. Set to false for encoded sync word.
      \param rising Whether the start of frame delimiter edge is rising (true) or falling (false).
      \param sofLen Start-of-frame length in bits.
      \returns \ref status_codes
    */
    int16_t ookDetector(uint16_t pattern = 0x0285, uint8_t len = 16, uint8_t repeats = 0, bool syncRaw = false, bool rising = false, uint8_t sofLen = 0);
    
    /*!
      \brief Set OOK detection threshold.
      \param level Threshold level in dB
      \returns \ref status_codes
    */
    int16_t setOokDetectionThreshold(int16_t level);

    /*!
      \brief Configure LoRa side detector, which enables to detect mutiple spreading factors and receive one of them.
      The following limitations apply:
        * In Rx mode, all side-detector spreading factors must be higher than the primary one (configured via begin or setSpreadingFactor)
        * For CAD mode, the above condition is inverted - all side-detector spreading factors must be smaller
        * All packets to be detected must have the same header type (implicit or explicit)
        * If bandwidth is higher than 500 kHz, at most 2 side detectors are allowed.
        * If the primary spreading factor is 10, 11 or 12, at most 2 side detectors are allowed.
        * All spreading factors must be different.
        * The difference between maximum and minimum spreading factor used must be less than or equal to 4.
      \param cfg Pointer to an array of side detector configuration structures. Set to null to disable all side detectors.
      \param numDetectors Number of side detectors to configure. Maximum of 3, set to 0 to to disable all side detectors.
      \returns \ref status_codes
    */
    int16_t setSideDetector(const LR2021LoRaSideDetector_t* cfg, size_t numDetectors);

    /*!
      \brief Sets gain of receiver LNA (low-noise amplifier). Can be set to any integer in range 1 to 13,
      where 13 is the highest gain. Set to 0 to enable automatic gain control (recommended).
      \param gain Gain of receiver LNA (low-noise amplifier) to be set.
      \returns \ref status_codes
    */
    int16_t setGain(uint8_t gain);

#if !RADIOLIB_GODMODE && !RADIOLIB_LOW_LEVEL
  protected:
#endif
    Module* getMod() override;
    
#if !RADIOLIB_GODMODE
  protected:
#endif

#if !RADIOLIB_GODMODE
  private:
#endif
    // flag to determine whether we are in the sub-GHz or 2.4 GHz range
    // this is needed to automatically detect which PA to use
    bool highFreq = false;
    uint8_t gainModeLf = RADIOLIB_LR2021_RX_BOOST_LF;
    uint8_t gainModeHf = RADIOLIB_LR2021_RX_BOOST_HF;

    // cached FLRC parameters
    uint16_t bitRateFlrc = 0;
    uint8_t codingRateFlrc = 0;

    int16_t modSetup(float freq, float tcxoVoltage, uint8_t modem);
    bool findChip(void);
    int16_t config(uint8_t modem);
    int16_t setPacketMode(uint8_t mode, uint8_t len);
    int16_t startCad(uint8_t symbolNum, uint8_t detPeak, uint8_t detMin, uint8_t exitMode, RadioLibTime_t timeout);

    // chip control commands
    int16_t readRadioRxFifo(uint8_t* data, size_t len);
    int16_t writeRadioTxFifo(const uint8_t* data, size_t len);
    int16_t writeRegMem32(uint32_t addr, const uint32_t* data, size_t len);
    int16_t writeRegMemMask32(uint32_t addr, uint32_t mask, uint32_t data);
    int16_t readRegMem32(uint32_t addr, uint32_t* data, size_t len);
    int16_t setFs(void);
    int16_t setAdditionalRegToRetain(uint8_t slot, uint32_t addr);
    int16_t setRx(uint32_t timeout);
    int16_t setTx(uint32_t timeout);
    int16_t setRxTxFallbackMode(uint8_t mode);
    int16_t setRxDutyCycle(uint32_t rxMaxTime, uint32_t cycleTime, uint8_t cfg);
    int16_t autoTxRx(uint32_t delay, uint8_t mode, uint32_t timeout);
    int16_t getRxPktLength(uint16_t* len);
    int16_t resetRxStats(void);
    int16_t setDefaultRxTxTimeout(uint32_t rxTimeout, uint32_t txTimeout);
    int16_t setRegMode(uint8_t simoUsage, const uint8_t rampTimes[4]);
    int16_t calibrate(uint8_t blocks);
    int16_t calibrateFrontEnd(const uint16_t freq[3]);
    int16_t getVbat(uint8_t resolution, uint16_t* vbat);
    int16_t getTemp(uint8_t source, uint8_t resolution, float* temp);
    int16_t setEolConfig(bool enable, uint8_t trim);
    int16_t getRandomNumber(uint32_t* rnd);
    int16_t getVersion(uint8_t* major, uint8_t* minor);
    int16_t clearErrors(void);
    int16_t getErrors(uint16_t* err);
    int16_t setDioFunction(uint8_t dio, uint8_t func, uint8_t pullDrive);
    int16_t setDioRfSwitchConfig(uint8_t dio, uint8_t func);
    int16_t setDioIrqConfig(uint8_t dio, uint32_t irq);
    int16_t clearIrqState(uint32_t irq);
    int16_t getAndClearIrqStatus(uint32_t* irq);
    int16_t configFifoIrq(uint8_t rxFifoIrq, uint8_t txFifoIrq, uint8_t rxHighThreshold, uint8_t txHighThreshold);
    int16_t getFifoIrqFlags(uint8_t* rxFifoFlags, uint8_t* txFifoFlags);
    int16_t clearFifoIrqFlags(uint8_t rxFifoFlags, uint8_t txFifoFlags);
    int16_t getAndClearFifoIrqFlags(uint8_t* rxFifoFlags, uint8_t* txFifoFlags);
    int16_t getRxFifoLevel(uint16_t* level);
    int16_t getTxFifoLevel(uint16_t* level);
    int16_t clearRxFifo(void);
    int16_t clearTxFifo(void);
    int16_t configLfClock(uint8_t cfg);
    int16_t configClkOutputs(uint8_t scaling);
    int16_t setTcxoMode(uint8_t tune, uint32_t startTime);
    int16_t setXoscCpTrim(uint8_t xta, uint8_t xtb, uint8_t startTime);

    // radio frequency front end commands
    int16_t setRfFrequency(uint32_t rfFreq);
    int16_t setRxPath(uint8_t rxPath, uint8_t rxBoost);
    int16_t getRssiInst(float* rssi);
    int16_t setRssiCalibration(uint8_t rxPath, const uint16_t gain[RADIOLIB_LR2021_GAIN_TABLE_LENGTH], const uint8_t noiseFloor[RADIOLIB_LR2021_GAIN_TABLE_LENGTH]);
    int16_t setTimestampSource(uint8_t index, uint8_t source);
    int16_t getTimestampValue(uint8_t index, uint32_t* timestamp);
    int16_t setCca(uint32_t duration, uint8_t gain);
    int16_t getCcaResult(float* rssiMin, float* rssiMax, float* rssiAvg);
    int16_t setCadParams(uint32_t cadTimeout, uint8_t threshold, uint8_t exitMode, uint32_t trxTimeout);
    int16_t setCad(void);
    int16_t selPa(uint8_t pa);
    int16_t setPaConfig(uint8_t pa, uint8_t paLfMode, uint8_t paLfDutyCycle, uint8_t paLfSlices, uint8_t paHfDutyCycle);
    int16_t setTxParams(int8_t txPower, uint8_t rampTime);

    // modem configuration commands
    int16_t setPacketType(uint8_t packetType);
    int16_t getPacketType(uint8_t* packetType);

    // LoRa commands
    int16_t setLoRaModulationParams(uint8_t sf, uint8_t bw, uint8_t cr, uint8_t ldro);
    int16_t setLoRaPacketParams(uint16_t preambleLen, uint8_t hdrType, uint8_t payloadLen, uint8_t crcType, uint8_t invertIQ);
    int16_t setLoRaSynchTimeout(uint8_t numSymbols, bool format);
    int16_t setLoRaSyncword(uint8_t syncword);
    int16_t setLoRaSideDetConfig(uint8_t* configs, size_t numSideDets);
    int16_t setLoRaSideDetSyncword(uint8_t* syncwords, size_t numSideDets);
    int16_t setLoRaCadParams(uint8_t numSymbols, bool preambleOnly, uint8_t pnrDelta, uint8_t cadExitMode, uint32_t timeout, uint8_t detPeak);
    int16_t setLoRaCad(void);
    int16_t getLoRaRxStats(uint16_t* pktRxTotal, uint16_t* pktCrcError, uint16_t* headerCrcError, uint16_t* falseSynch);
    int16_t getLoRaPacketStatus(uint8_t* crc, uint8_t* cr, uint8_t* packetLen, float* snrPacket, float* rssiPacket, float* rssiSignalPacket);
    int16_t setLoRaAddress(uint8_t addrLen, uint8_t addrPos, const uint8_t* addr);
    int16_t setLoRaHopping(uint8_t hopCtrl, uint16_t hopPeriod, const uint32_t* freqHops, size_t numFreqHops);
    int16_t setLoRaTxSync(uint8_t function, uint8_t dioNum);
    int16_t setLoRaSideDetCad(const uint8_t* pnrDelta, const uint8_t* detPeak, size_t numSideDets);
    int16_t setLoRaHeaderType(uint8_t hdrType, size_t len = RADIOLIB_LR2021_MAX_PACKET_LENGTH);

    // ranging commands
    int16_t setRangingAddr(uint32_t addr, uint8_t checkLen);
    int16_t setRangingReqAddr(uint32_t addr);
    int16_t getRangingResult(uint8_t type, uint32_t* rng1, uint8_t* rssi1, uint32_t* rng2);
    int16_t getRangingStats(uint16_t* exchangeValid, uint16_t* requestValid, uint16_t* responseDone, uint16_t* timeout, uint16_t* requestDiscarded);
    int16_t setRangingTxRxDelay(uint32_t delay);
    int16_t setRangingParams(bool spyMode, uint8_t nbSymbols);

    // GFSK commands
    int16_t setGfskModulationParams(uint32_t bitRate, uint8_t pulseShape, uint8_t rxBw, uint32_t freqDev);
    int16_t setGfskPacketParams(uint16_t preambleLen, uint8_t preambleDetect, bool longPreamble, bool pldLenBits, uint8_t addrComp, uint8_t packetFormat, uint16_t payloadLen, uint8_t crc, uint8_t dcFree);
    int16_t setGfskWhiteningParams(uint8_t whitenType, uint16_t init);
    int16_t setGfskCrcParams(uint32_t poly, uint32_t init);
    int16_t setGfskSyncword(const uint8_t* syncWord, size_t syncWordLen, bool msbFirst);
    int16_t setGfskAddress(uint8_t addrNode, uint8_t addrBroadcast);
    int16_t getGfskRxStats(uint16_t* packetRx, uint16_t* packetCrcError, uint16_t* lenError, uint16_t* preambleDet, uint16_t* syncOk, uint16_t* syncFail, uint16_t* timeout);
    int16_t getGfskPacketStatus(uint16_t* packetLen, float* rssiAvg, float* rssiSync, bool* addrMatchNode, bool* addrMatchBroadcast, float* lqi);

    // OQPSK commands
    int16_t setOqpskParams(uint8_t mode, uint8_t rxBw, uint8_t payloadLen, uint16_t preambleLen, bool addrFilt, bool fcsManual);
    int16_t getOqpskRxStats(uint16_t* packetRx, uint16_t* crcError, uint16_t* lenError);
    int16_t getOqpskPacketStatus(uint8_t* rxHeader, uint16_t* payloadLen, float* rssiAvg, float* rssiSync, float* lqi);
    int16_t setOqpskPacketLen(uint8_t len);
    int16_t setOqpskAddress(const uint8_t longDestAddr[8], uint16_t shortDestAddr, uint16_t panId, uint8_t transId);

    // BPSK commands
    int16_t setBpskModulationParams(uint32_t bitRate, uint8_t pulseShape, bool diff, uint8_t diffInit);
    int16_t setBpskPacketParams(uint8_t payloadLen, uint8_t mode, bool sigFoxControlMsg, uint8_t sigFoxRank);

    // FLRC commands
    int16_t setFlrcModulationParams(uint8_t brBw, uint8_t cr, uint8_t pulseShape);
    int16_t setFlrcPacketParams(uint8_t agcPreambleLen, uint8_t syncWordLen, uint8_t syncWordTx, uint8_t syncMatch, bool fixedLength, uint8_t crc, uint16_t payloadLen);
    int16_t getFlrcRxStats(uint16_t* packetRx, uint16_t* packetCrcError, uint16_t* lenError);
    int16_t getFlrcPacketStatus(uint16_t* packetLen, float* rssiAvg, float* rssiSync, uint8_t* syncWordNum);
    int16_t setFlrcSyncWord(uint8_t syncWordNum, uint32_t syncWord);

    // LR-FHSS commands
    int16_t lrFhssSetSyncword(uint32_t syncWord);
    //! \TODO: [LR2021] Implement reading/writing LR-FHSS hopping table 
    //int16_t readLrFhssHoppingTable(LR2021LrFhssHopTableEntry_t* hopTable[40], size_t* hopTableLen);
    //int16_t writeLrFhssHoppingTable(LR2021LrFhssHopTableEntry_t* hopTable[40], size_t hopTableLen);

    // OOK commands
    int16_t setOokModulationParams(uint32_t bitRate, uint8_t pulseShape, uint8_t rxBw, uint8_t depth);
    int16_t setOokPacketParams(uint16_t preambleLen, uint8_t addrComp, uint8_t packetFormat, uint16_t payloadLen, uint8_t crc, uint8_t manchester);
    int16_t setOokCrcParams(uint32_t poly, uint32_t init);
    int16_t setOokSyncword(const uint8_t* syncWord, size_t syncWordLen, bool msbFirst);
    int16_t setOokAddress(uint8_t addrNode, uint8_t addrBroadcast);
    int16_t getOokRxStats(uint16_t* packetRx, uint16_t* crcError, uint16_t* lenError);
    int16_t getOokPacketStatus(uint16_t* packetLen, float* rssiAvg, float* rssiSync, bool* addrMatchNode, bool* addrMatchBroadcast, float* lqi);
    int16_t setOokDetector(uint16_t preamblePattern, uint8_t patternLen, uint8_t patternNumRepeaters, bool syncWordRaw, bool sofDelimiterRising, uint8_t sofDelimiterLen);
    int16_t setOokWhiteningParams(uint8_t bitIdx, uint16_t poly, uint16_t init);

    // test commands
    int16_t setTxTestMode(uint8_t mode);
};

#endif

#endif
