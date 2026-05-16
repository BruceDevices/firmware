#if !defined(_RADIOLIB_LR11X0_H)
#define _RADIOLIB_LR11X0_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_LR11X0

#include "../../Module.h"

#include "../LR11x0/LR_common.h"

#include "LR11x0_commands.h"
#include "LR11x0_registers.h"
#include "LR11x0_types.h"

// LR11X0 physical layer properties
#define RADIOLIB_LR11X0_FREQUENCY_STEP_SIZE                     1.0
#define RADIOLIB_LR11X0_MAX_PACKET_LENGTH                       255
#define RADIOLIB_LR11X0_CRYSTAL_FREQ                            32.0
#define RADIOLIB_LR11X0_DIV_EXPONENT                            25

/*!
  \class LR11x0
  \brief Base class for %LR11x0 series. All derived classes for %LR11x0 (e.g. LR1110 or LR1120) inherit from this base class.
  This class should not be instantiated directly from user code, only from its derived classes.
*/
class LR11x0: public LRxxxx {
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
    explicit LR11x0(Module* mod);

    /*!
      \brief Custom operation modes for LR11x0.
      Needed because LR11x0 has several modems (sub-GHz, 2.4 GHz etc.) in one package
    */
    enum OpMode_t {
        /*! End of table marker, use \ref END_OF_MODE_TABLE constant instead */
        MODE_END_OF_TABLE = Module::MODE_END_OF_TABLE,
        /*! Standby/idle mode */
        MODE_STBY = Module::MODE_IDLE,
        /*! Receive mode */
        MODE_RX = Module::MODE_RX,
        /*! Low power transmission mode */
        MODE_TX = Module::MODE_TX,
        /*! High power transmission mode */
        MODE_TX_HP,
        /*! High frequency transmission mode */
        MODE_TX_HF,
        /*! GNSS scanning mode */
        MODE_GNSS,
        /*! WiFi scanning mode */
        MODE_WIFI,
    };
    
    /*!
      \brief Initialization method for LoRa modem.
      \param bw LoRa bandwidth in kHz.
      \param sf LoRa spreading factor.
      \param cr LoRa coding rate denominator. Allowed values range from 4 to 8. Note that a value of 4 means no coding,
      is undocumented and not recommended without your own FEC.
      \param syncWord 1-byte LoRa sync word.
      \param preambleLength LoRa preamble length in symbols
      \param tcxoVoltage TCXO reference voltage to be set.
      \param high defaults to false for Sub-GHz band, true for frequencies above 1GHz
      \returns \ref status_codes
    */
    int16_t begin(float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, uint16_t preambleLength, float tcxoVoltage, bool high = false);

    /*!
      \brief Initialization method for FSK modem.
      \param br FSK bit rate in kbps.
      \param freqDev Frequency deviation from carrier frequency in kHz.
      \param rxBw Receiver bandwidth in kHz.
      \param preambleLength FSK preamble length in bits.
      \param tcxoVoltage TCXO reference voltage to be set.
      \returns \ref status_codes
    */
    int16_t beginGFSK(float br, float freqDev, float rxBw, uint16_t preambleLength, float tcxoVoltage);

    /*!
      \brief Initialization method for LR-FHSS modem.
      \param bw LR-FHSS bandwidth, one of RADIOLIB_LR11X0_LR_FHSS_BW_* values.
      \param cr LR-FHSS coding rate, one of RADIOLIB_LR11X0_LR_FHSS_CR_* values.
      \param narrowGrid Whether to use narrow (3.9 kHz) or wide (25.39 kHz) grid spacing.
      \param tcxoVoltage TCXO reference voltage to be set.
      \returns \ref status_codes
    */
    int16_t beginLRFHSS(uint8_t bw, uint8_t cr, bool narrowGrid, float tcxoVoltage);

    /*!
      \brief Initialization method for GNSS scanning.
      \param constellations GNSS constellations to use (GPS, BeiDou or both). Defaults to both.
      \param tcxoVoltage TCXO reference voltage to be set.
      \returns \ref status_codes
    */
    int16_t beginGNSS(uint8_t constellations = RADIOLIB_LR11X0_GNSS_CONSTELLATION_GPS | RADIOLIB_LR11X0_GNSS_CONSTELLATION_BEIDOU, float tcxoVoltage = 1.6);

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
      \brief Starts direct mode reception. Only implemented for PhysicalLayer compatibility, as %SX126x series does not support direct mode reception.
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
      \param mode Oscillator to be used in standby mode. Can be set to RADIOLIB_LR11X0_STANDBY_RC (13 MHz RC oscillator)
      or RADIOLIB_LR11X0_STANDBY_XOSC (32 MHz external crystal oscillator).
      \returns \ref status_codes
    */
    int16_t standby(uint8_t mode) override;

    /*!
      \brief Sets the module to standby mode.
      \param mode Oscillator to be used in standby mode. Can be set to RADIOLIB_LR11X0_STANDBY_RC (13 MHz RC oscillator)
      or RADIOLIB_LR11X0_STANDBY_XOSC (32 MHz external crystal oscillator).
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

    // configuration methods

    /*!
      \brief Sets LoRa bandwidth. Allowed values are 62.5, 125.0, 250.0 and 500.0 kHz. (default, high = false)
      \param bw LoRa bandwidth to be set in kHz.
      \param high if set to true, allowed bandwidth is 203.125, 406.25 and 812.5 kHz, frequency must be above 1GHz
      \returns \ref status_codes
    */
    int16_t setBandwidth(float bw, bool high = false);

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
      \brief Sets GFSK bit rate. Allowed values range from 0.6 to 300.0 kbps.
      \param br FSK bit rate to be set in kbps.
      \returns \ref status_codes
    */
    int16_t setBitRate(float br) override;

    /*!
      \brief Sets GFSK frequency deviation. Allowed values range from 0.0 to 200.0 kHz.
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
    int16_t fixedPacketLengthMode(uint8_t len = RADIOLIB_LR11X0_MAX_PACKET_LENGTH);

    /*!
      \brief Set modem in variable packet length mode. Available in GFSK mode only.
      \param maxLen Maximum packet length.
      \returns \ref status_codes
    */
    int16_t variablePacketLengthMode(uint8_t maxLen = RADIOLIB_LR11X0_MAX_PACKET_LENGTH);

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
      \param delay TCXO timeout in us. Defaults to 5000 us.
      \returns \ref status_codes
    */
    int16_t setTCXO(float voltage, uint32_t delay = 5000);

    /*!
      \brief Sets CRC configuration.
      \param len CRC length in bytes, Allowed values are 1 or 2, set to 0 to disable CRC.
      \param initial Initial CRC value. GFSK only. Defaults to 0x1D0F (CCIT CRC).
      \param polynomial Polynomial for CRC calculation. GFSK only. Defaults to 0x1021 (CCIT CRC).
      \param inverted Invert CRC bytes. GFSK only. Defaults to true (CCIT CRC).
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
      \brief Gets RSSI (Recorded Signal Strength Indicator) of the last received packet. Only available for LoRa or GFSK modem.
      \returns RSSI of the last received packet in dBm.
    */
    float getRSSI() override;

    /*!
      \brief Gets SNR (Signal to Noise Ratio) of the last received packet. Only available for LoRa modem.
      \returns SNR of the last received packet in dB.
    */
    float getSNR() override;

    /*!
      \brief Gets frequency error of the latest received packet.
      \returns Frequency error in Hz.
    */
    float getFrequencyError();

    /*!
      \brief Query modem for the packet length of received payload.
      \param update Update received packet length. Will return cached value when set to false.
      \returns Length of last received packet in bytes.
    */
    size_t getPacketLength(bool update = true) override;

    /*!
      \brief Query modem for the packet length of received payload.
      \param update Update received packet length. Will return cached value when set to false.
      \param offset Pointer to a variable that will hold the receive packet's offset in the RX buffer
      \returns Length of last received packet in bytes.
    */
    size_t getPacketLength(bool update, uint8_t* offset);

    /*!
      \brief Get LoRa header information from last received packet. Only valid in explicit header mode.
      \param cr Pointer to variable to store the coding rate.
      \param hasCRC Pointer to variable to store the CRC status.
      \returns \ref status_codes
    */
    int16_t getLoRaRxHeaderInfo(uint8_t* cr, bool* hasCRC);

    /*!
      \brief Get expected time-on-air for a given size of payload
      \param len Payload length in bytes.
      \returns Expected time-on-air in microseconds.
    */
    RadioLibTime_t getTimeOnAir(size_t len) override;

    /*!
      \brief Read currently active IRQ flags.
      \returns IRQ flags.
    */
    uint32_t getIrqFlags() override;

    /*!
      \brief Set interrupt on IRQ pin to be sent on a specific IRQ bit (e.g. RxTimeout, CadDone).
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
      \brief Set regulator mode to LDO.
      \returns \ref status_codes
    */
    int16_t setRegulatorLDO();

    /*!
      \brief Set regulator mode to DC-DC.
      \returns \ref status_codes
    */
    int16_t setRegulatorDCDC();

    /*!
      \brief Enables or disables Rx Boosted Gain mode (additional Rx gain for increased power consumption).
      \param en True for Rx Boosted Gain, false for Rx Power Saving Gain
      \returns \ref status_codes
    */
    int16_t setRxBoostedGainMode(bool en);

    /*!
      \brief Sets output power. Allowed values are in range from -9 to 22 dBm (high-power PA),
      -17 to 14 dBm (low-power PA) and -18 to 13 dBm (high-frequency PA).
      This method allows user full control over PA configuration parameters.
      If you set incorrect PA configuration values, you can fail to reach the
      desired output power level or damage your device.
      Unless you can verify the output power, it is strongly advised to use
      LR1110::setOutputPower(power) or LR1120::setOutputPower(power).
      \param power Output power to be set in dBm.
      \param paSel PA selection, low-power, high-power or high-frequency. One of RADIOLIB_LR11X0_PA_SEL_* macros.
      \param regPaSupply PA power source selection, internal regulator or VBAT. One of RADIOLIB_LR11X0_PA_SUPPLY_* macros.
      Must be set to RADIOLIB_LR11X0_PA_SUPPLY_VBAT when output power is more than 14 dBm.
      \param paDutyCycle PA duty cycle.
      \param paHpSel High-power PA size control.
      \param rampTime PA power ramping time raw value, one of RADIOLIB_LRXXXX_PA_RAMP_* macros.
      \returns \ref status_codes
    */
    int16_t setOutputPower(int8_t power, uint8_t paSel, uint8_t regPaSupply, uint8_t paDutyCycle, uint8_t paHpSel, uint8_t rampTime);

    /*! \copydoc Module::setRfSwitchTable */
    void setRfSwitchTable(const uint32_t (&pins)[Module::RFSWITCH_MAX_PINS], const Module::RfSwitchMode_t table[]);

    /*!
      \brief Forces LoRa low data rate optimization. Only available in LoRa mode. After calling this method, LDRO will always be set to
      the provided value, regardless of symbol length. To re-enable automatic LDRO configuration, call LR11x0::autoLDRO()
      \param enable Force LDRO to be always enabled (true) or disabled (false).
      \returns \ref status_codes
    */
    int16_t forceLDRO(bool enable);

    /*!
      \brief Re-enables automatic LDRO configuration. Only available in LoRa mode. After calling this method, LDRO will be enabled automatically
      when symbol length exceeds 16 ms.
      \returns \ref status_codes
    */
    int16_t autoLDRO();

    /*!
      \brief Sets LR-FHSS configuration.
      \param bw LR-FHSS bandwidth, one of RADIOLIB_LR11X0_LR_FHSS_BW_* values.
      \param cr LR-FHSS coding rate, one of RADIOLIB_LR11X0_LR_FHSS_CR_* values.
      \param hdrCount Header packet count, 1 - 4. Defaults to 3.
      \param hopSeed 9-bit seed number for PRNG generation of the hopping sequence. Defaults to 0x13A.
      \returns \ref status_codes
    */
    int16_t setLrFhssConfig(uint8_t bw, uint8_t cr, uint8_t hdrCount = 3, uint16_t hopSeed = 0x13A);
    
    /*!
      \brief Start passive WiFi scan. BUSY pin will be de-activated when the scan is finished.
      \param wifiType Type of WiFi (802.11) signals to scan, 'b', 'n', 'g' or '*' for all signals.
      \param mode Scan acquisition mode, one of RADIOLIB_LR11X0_WIFI_ACQ_MODE_*.
      The type of results available after the scan depends on this mode.
      Defaults to RADIOLIB_LR11X0_WIFI_ACQ_MODE_FULL_BEACON, which provides the most information.
      \param chanMask Bit mask of WiFi channels to scan, defaults to all channels.
      More channels leads to longer overall scan duration.
      \param numScans Number of scans to perform per each enabled channel. Defaults to 16 scans.
      More scans leads to longer overall scan duration.
      \param timeout Timeout of each scan in milliseconds. Defaults to 100 ms
      Longer timeout leads to longer overall scan duration.
      \returns \ref status_codes
    */
    int16_t startWifiScan(char wifiType, uint8_t mode = RADIOLIB_LR11X0_WIFI_ACQ_MODE_FULL_BEACON, uint16_t chanMask = RADIOLIB_LR11X0_WIFI_ALL_CHANNELS, uint8_t numScans = 16, uint16_t timeout = 100);

    /*!
      \brief Sets interrupt service routine to call when a WiFi scan is completed.
      \param func ISR to call.
    */
    void setWiFiScanAction(void (*func)(void));

    /*!
      \brief Clears interrupt service routine to call when a WiFi scan is completed.
    */
    void clearWiFiScanAction();

    /*!
      \brief Get number of WiFi scan results after the scan is finished.
      \param count Pointer to a variable that will hold the number of scan results.
      \returns \ref status_codes
    */
    int16_t getWifiScanResultsCount(uint8_t* count);

    /*!
      \brief Retrieve passive WiFi scan result.
      \param result Pointer to structure to hold the result data.
      \param index Result index, starting from 0. The number of scan results can be retrieved by calling getWifiScanResultsCount.
      \param brief Whether to only retrieve the results in brief format. If set to false, only information in LR11x0WifiResult_t
      will be retrieved. If set to true, information in LR11x0WifiResultFull_t will be retrieved. In addition, if WiFi scan mode
      was set to RADIOLIB_LR11X0_WIFI_ACQ_MODE_FULL_BEACON, all information in LR11x0WifiResultExtended_t will be retrieved.
      \returns \ref status_codes
    */
    int16_t getWifiScanResult(LR11x0WifiResult_t* result, uint8_t index, bool brief = false);
    
    /*!
      \brief Blocking WiFi scan method. Performs a full passive WiFi scan.
      This method may block for several seconds!
      \param wifiType Type of WiFi (802.11) signals to scan, 'b', 'n', 'g' or '*' for all signals.
      \param count Pointer to a variable that will hold the number of scan results.
      \param mode Scan acquisition mode, one of RADIOLIB_LR11X0_WIFI_ACQ_MODE_*.
      The type of results available after the scan depends on this mode.
      Defaults to RADIOLIB_LR11X0_WIFI_ACQ_MODE_FULL_BEACON, which provides the most information.
      \param chanMask Bit mask of WiFi channels to scan, defaults to all channels.
      More channels leads to longer overall scan duration.
      \param numScans Number of scans to perform per each enabled channel. Defaults to 16 scans.
      More scans leads to longer overall scan duration.
      \param timeout Timeout of each scan in milliseconds. Defaults to 100 ms
      Longer timeout leads to longer overall scan duration.
      \returns \ref status_codes
    */
    int16_t wifiScan(uint8_t wifiType, uint8_t* count, uint8_t mode = RADIOLIB_LR11X0_WIFI_ACQ_MODE_FULL_BEACON, uint16_t chanMask = RADIOLIB_LR11X0_WIFI_ALL_CHANNELS, uint8_t numScans = 16, uint16_t timeout = 100);
   
    /*!
      \brief Retrieve LR11x0 hardware, device and firmware version information.
      \param info Pointer to LR11x0VersionInfo_t structure to populate.
      \returns \ref status_codes
    */
    int16_t getVersionInfo(LR11x0VersionInfo_t* info);
    
    /*!
      \brief Method to upload new firmware image to the device.
      The device will be automatically erased, a new firmware will be uploaded,
      written to flash and executed.
      \param image Pointer to the image to upload.
      \param size Size of the image in 32-bit words.
      \param nonvolatile Set to true when the image is saved in non-volatile memory of the host processor,
      or to false when the patch is in its RAM. Defaults to true.
      \returns \ref status_codes
    */
    int16_t updateFirmware(const uint32_t* image, size_t size, bool nonvolatile = true);
    
    /*!
      \brief Method to check whether the device is capable of performing a GNSS scan.
      \returns \ref status_codes
    */
    int16_t isGnssScanCapable();

    /*!
      \brief Performs GNSS scan.
      \param res Pointer to LR11x0GnssPosition_t structure to populate.
      Will not be saved if set to NULL, defaults to NULL.
      \returns \ref status_codes
    */
    int16_t gnssScan(LR11x0GnssResult_t* res = NULL);

    /*!
      \brief Read information about the almanac.
      \param stat Pointer to structure to save the almanac status into.
      This is not the actual almanac, just a structure providing information about it.
      \returns \ref status_codes
    */
    int16_t getGnssAlmanacStatus(LR11x0GnssAlmanacStatus_t *stat);

    /*!
      \brief Blocking wait until the next subframe with almanac data is available.
      Used to control timing during almanac update from satellite.
      \param stat Pointer to structure containing the almanac status read by getGnssAlmanacStatus.
      This is not the actual almanac, just a structure providing information about it.
      \param constellation Constellation to wait for, one of RADIOLIB_LR11X0_GNSS_CONSTELLATION_*.
      Constellations cannot be updated at the same time, but rather must be updated sequentially!
      \returns \ref status_codes
    */
    int16_t gnssDelayUntilSubframe(LR11x0GnssAlmanacStatus_t *stat, uint8_t constellation);

    /*!
      \brief Perform almanac update. Must be called immediately after gnssDelayUntilSubframe.
      \param constellation Constellation to update, one of RADIOLIB_LR11X0_GNSS_CONSTELLATION_*.
      Constellations cannot be updated at the same time, but rather must be updated sequentially!
      \returns \ref status_codes
    */
    int16_t updateGnssAlmanac(uint8_t constellation);
    
    /*!
      \brief Get GNSS position. Called after gnssScan to retrieve the position calculated by the internal solver.
      \param pos Pointer to LR11x0GnssPosition_t structure to populate.
      \param filtered Whether to save the filtered, or unfiltered values. Defaults to true (filtered).
      \returns \ref status_codes
    */
    int16_t getGnssPosition(LR11x0GnssPosition_t* pos, bool filtered = true);

    /*!
      \brief Get GNSS satellites found during the last scan.
      \param sats Pointer to array of LR11x0GnssSatellite_t structures to populate.
      \param numSats Number of satellites to read. Can be retrieved from LR11x0GnssResult_t passed to gnssScan.
      \returns \ref status_codes
    */
    int16_t getGnssSatellites(LR11x0GnssSatellite_t* sats, uint8_t numSats);

    /*!
      \brief Get modem currently in use by the radio.
      \param modem Pointer to a variable to save the retrieved configuration into.
      \returns \ref status_codes
    */
    int16_t getModem(ModemType_t* modem) override;

    /*!
      \brief Perform image rejection calibration for the specified frequency band.
      WARNING: Use at your own risk! Setting incorrect values may lead to decreased performance
      \param freqMin Frequency band lower bound.
      \param freqMax Frequency band upper bound.
      \returns \ref status_codes
    */
    int16_t calibrateImageRejection(float freqMin, float freqMax);
    
    /*! \copydoc PhysicalLayer::stageMode */
    int16_t stageMode(RadioModeType_t mode, RadioModeConfig_t* cfg) override;

    /*! \copydoc PhysicalLayer::launchMode */
    int16_t launchMode() override;
    
#if !RADIOLIB_GODMODE && !RADIOLIB_LOW_LEVEL
  protected:
#endif
    Module* getMod() override;

    // method that applies some magic workaround for specific bitrate, frequency deviation,
    // receiver bandwidth and carrier frequencies for GFSK (and resets it in all other cases)
    int16_t workaroundGFSK();

    // LR11x0 SPI command implementations
    int16_t writeRegMem32(uint32_t addr, const uint32_t* data, size_t len);
    int16_t readRegMem32(uint32_t addr, uint32_t* data, size_t len);
    int16_t writeBuffer8(const uint8_t* data, size_t len);
    int16_t readBuffer8(uint8_t* data, size_t len, size_t offset);
    int16_t clearRxBuffer(void);
    int16_t writeRegMemMask32(uint32_t addr, uint32_t mask, uint32_t data);

    int16_t getVersion(uint8_t* hw, uint8_t* device, uint8_t* major, uint8_t* minor);
    int16_t getErrors(uint16_t* err);
    int16_t clearErrors(void);
    int16_t calibrate(uint8_t params);
    int16_t setRegMode(uint8_t mode);
    int16_t setDioAsRfSwitch(uint8_t en, uint8_t stbyCfg, uint8_t rxCfg, uint8_t txCfg, uint8_t txHpCfg, uint8_t txHfCfg, uint8_t gnssCfg, uint8_t wifiCfg);
    int16_t setDioIrqParams(uint32_t irq1, uint32_t irq2);
    int16_t setDioIrqParams(uint32_t irq);
    int16_t clearIrqState(uint32_t irq);
    int16_t configLfClock(uint8_t setup);
    int16_t setTcxoMode(uint8_t tune, uint32_t delay);
    int16_t reboot(bool stay);
    int16_t getVbat(float* vbat);
    int16_t getTemp(float* temp);
    int16_t setFs(void);
    int16_t getRandomNumber(uint32_t* rnd);
    int16_t eraseInfoPage(void);
    int16_t writeInfoPage(uint16_t addr, const uint32_t* data, size_t len);
    int16_t readInfoPage(uint16_t addr, uint32_t* data, size_t len);
    int16_t getChipEui(uint8_t* eui);
    int16_t getSemtechJoinEui(uint8_t* eui);
    int16_t deriveRootKeysAndGetPin(uint8_t* pin);
    int16_t enableSpiCrc(bool en);
    int16_t driveDiosInSleepMode(bool en);

    int16_t resetStats(void);
    int16_t getStats(uint16_t* nbPktReceived, uint16_t* nbPktCrcError, uint16_t* data1, uint16_t* data2);
    int16_t getPacketType(uint8_t* type);
    int16_t getRxBufferStatus(uint8_t* len, uint8_t* startOffset);
    int16_t getPacketStatusLoRa(float* rssiPkt, float* snrPkt, float* signalRssiPkt);
    int16_t getPacketStatusGFSK(float* rssiSync, float* rssiAvg, uint8_t* rxLen, uint8_t* stat);
    int16_t getRssiInst(float* rssi);
    int16_t setGfskSyncWord(uint8_t* sync);
    int16_t setLoRaPublicNetwork(bool pub);
    int16_t setRx(uint32_t timeout);
    int16_t setTx(uint32_t timeout);
    int16_t setRfFrequency(uint32_t rfFreq);
    int16_t autoTxRx(uint32_t delay, uint8_t intMode, uint32_t timeout);
    int16_t setCadParams(uint8_t symNum, uint8_t detPeak, uint8_t detMin, uint8_t cadExitMode, uint32_t timeout);
    int16_t setPacketType(uint8_t type);
    int16_t setModulationParamsLoRa(uint8_t sf, uint8_t bw, uint8_t cr, uint8_t ldro);
    int16_t setModulationParamsGFSK(uint32_t br, uint8_t sh, uint8_t rxBw, uint32_t freqDev);
    int16_t setModulationParamsLrFhss(uint32_t br, uint8_t sh);
    int16_t setModulationParamsSigfox(uint32_t br, uint8_t sh);
    int16_t setPacketParamsLoRa(uint16_t preambleLen, uint8_t hdrType, uint8_t payloadLen, uint8_t crcType, uint8_t invertIQ);
    int16_t setPacketParamsGFSK(uint16_t preambleLen, uint8_t preambleDetectorLen, uint8_t syncWordLen, uint8_t addrCmp, uint8_t packType, uint8_t payloadLen, uint8_t crcType, uint8_t whiten);
    int16_t setPacketParamsSigfox(uint8_t payloadLen, uint16_t rampUpDelay, uint16_t rampDownDelay, uint16_t bitNum);
    int16_t setTxParams(int8_t pwr, uint8_t ramp);
    int16_t setPacketAdrs(uint8_t node, uint8_t broadcast);
    int16_t setRxTxFallbackMode(uint8_t mode);
    int16_t setRxDutyCycle(uint32_t rxPeriod, uint32_t sleepPeriod, uint8_t mode);
    int16_t setPaConfig(uint8_t paSel, uint8_t regPaSupply, uint8_t paDutyCycle, uint8_t paHpSel);
    int16_t stopTimeoutOnPreamble(bool stop);
    int16_t setCad(void);
    int16_t setTxCw(void);
    int16_t setTxInfinitePreamble(void);
    int16_t setLoRaSynchTimeout(uint8_t symbolNum);
    int16_t setRangingAddr(uint32_t addr, uint8_t checkLen);
    int16_t setRangingReqAddr(uint32_t addr);
    int16_t getRangingResult(uint8_t type, float* res);
    int16_t setRangingTxRxDelay(uint32_t delay);
    int16_t setGfskCrcParams(uint32_t init, uint32_t poly);
    int16_t setGfskWhitParams(uint16_t seed);
    int16_t setRangingParameter(uint8_t symbolNum);
    int16_t setRssiCalibration(const int8_t* tune, int16_t gainOffset);
    int16_t setLoRaSyncWord(uint8_t sync);
    int16_t lrFhssSetSyncWord(uint32_t sync);
    int16_t configBleBeacon(uint8_t chan, const uint8_t* payload, size_t len);
    int16_t bleBeaconSend(uint8_t chan, const uint8_t* payload, size_t len);

    int16_t wifiScan(uint8_t type, uint16_t mask, uint8_t acqMode, uint8_t nbMaxRes, uint8_t nbScanPerChan, uint16_t timeout, uint8_t abortOnTimeout);
    int16_t wifiScanTimeLimit(uint8_t type, uint16_t mask, uint8_t acqMode, uint8_t nbMaxRes, uint16_t timePerChan, uint16_t timeout);
    int16_t wifiCountryCode(uint16_t mask, uint8_t nbMaxRes, uint8_t nbScanPerChan, uint16_t timeout, uint8_t abortOnTimeout);
    int16_t wifiCountryCodeTimeLimit(uint16_t mask, uint8_t nbMaxRes, uint16_t timePerChan, uint16_t timeout);
    int16_t wifiGetNbResults(uint8_t* nbResults);
    int16_t wifiReadResults(uint8_t index, uint8_t nbResults, uint8_t format, uint8_t* results);
    int16_t wifiResetCumulTimings(void);
    int16_t wifiReadCumulTimings(uint32_t* detection, uint32_t* capture, uint32_t* demodulation);
    int16_t wifiGetNbCountryCodeResults(uint8_t* nbResults);
    int16_t wifiReadCountryCodeResults(uint8_t index, uint8_t nbResults, uint8_t* results);
    int16_t wifiCfgTimestampAPphone(uint32_t timestamp);
    int16_t wifiReadVersion(uint8_t* major, uint8_t* minor);

    int16_t gnssReadRssi(int8_t* rssi);
    int16_t gnssSetConstellationToUse(uint8_t mask);
    int16_t gnssReadConstellationToUse(uint8_t* mask);
    int16_t gnssSetAlmanacUpdate(uint8_t mask);
    int16_t gnssReadAlmanacUpdate(uint8_t* mask);
    int16_t gnssSetFreqSearchSpace(uint8_t freq);
    int16_t gnssReadFreqSearchSpace(uint8_t* freq);
    int16_t gnssReadVersion(uint8_t* fw, uint8_t* almanac);
    int16_t gnssReadSupportedConstellations(uint8_t* mask);
    int16_t gnssSetMode(uint8_t mode);
    int16_t gnssAutonomous(uint32_t gpsTime, uint8_t resMask, uint8_t nbSvMask);
    int16_t gnssAssisted(uint32_t gpsTime, uint8_t effort, uint8_t resMask, uint8_t nbSvMask);
    int16_t gnssSetAssistancePosition(float lat, float lon);
    int16_t gnssReadAssistancePosition(float* lat, float* lon);
    int16_t gnssPushSolverMsg(uint8_t* payload, size_t len);
    int16_t gnssPushDmMsg(uint8_t* payload, size_t len);
    int16_t gnssGetContextStatus(uint8_t* fwVersion, uint32_t* almanacCrc, uint8_t* errCode, uint8_t* almUpdMask, uint8_t* freqSpace);
    int16_t gnssGetNbSvDetected(uint8_t* nbSv);
    int16_t gnssGetSvDetected(uint8_t* svId, uint8_t* snr, int16_t* doppler, size_t nbSv);
    int16_t gnssGetConsumption(uint32_t* cpu, uint32_t* radio);
    int16_t gnssGetResultSize(uint16_t* size);
    int16_t gnssReadResults(uint8_t* result, uint16_t size);
    int16_t gnssAlmanacFullUpdateHeader(uint16_t date, uint32_t globalCrc);
    int16_t gnssAlmanacFullUpdateSV(uint8_t svn, const uint8_t* svnAlmanac);
    int16_t gnssAlmanacReadAddrSize(uint32_t* addr, uint16_t* size);
    int16_t gnssAlmanacReadSV(uint8_t svId, uint8_t* almanac);
    int16_t gnssGetNbSvVisible(uint32_t time, float lat, float lon, uint8_t constellation, uint8_t* nbSv);
    int16_t gnssGetSvVisible(uint8_t nbSv, uint8_t** svId, int16_t** doppler, int16_t** dopplerErr);
    int16_t gnssPerformScan(uint8_t effort, uint8_t resMask, uint8_t nbSvMax);
    int16_t gnssReadLastScanModeLaunched(uint8_t* lastScanMode);
    int16_t gnssFetchTime(uint8_t effort, uint8_t opt);
    int16_t gnssReadTime(uint8_t* err, uint32_t* time, uint32_t* nbUs, uint32_t* timeAccuracy);
    int16_t gnssResetTime(void);
    int16_t gnssResetPosition(void);
    int16_t gnssReadWeekNumberRollover(uint8_t* status, uint8_t* rollover);
    int16_t gnssReadDemodStatus(int8_t* status, uint8_t* info);
    int16_t gnssReadCumulTiming(uint32_t* timing, uint8_t* constDemod);
    int16_t gnssSetTime(uint32_t time, uint16_t accuracy);
    int16_t gnssReadDopplerSolverRes(uint8_t* error, uint8_t* nbSvUsed, float* lat, float* lon, uint16_t* accuracy, uint16_t* xtal, float* latFilt, float* lonFilt, uint16_t* accuracyFilt, uint16_t* xtalFilt);
    int16_t gnssReadDelayResetAP(uint32_t* delay);
    int16_t gnssAlmanacUpdateFromSat(uint8_t effort, uint8_t bitMask);
    int16_t gnssReadAlmanacStatus(uint8_t* status);
    int16_t gnssReadKeepSyncStatus(uint8_t mask, uint8_t* nbSvVisible, uint32_t* elapsed);
    int16_t gnssConfigAlmanacUpdatePeriod(uint8_t bitMask, uint8_t svType, uint16_t period);
    int16_t gnssReadAlmanacUpdatePeriod(uint8_t bitMask, uint8_t svType, uint16_t* period);
    int16_t gnssConfigDelayResetAP(uint32_t delay);
    int16_t gnssGetSvWarmStart(uint8_t bitMask, uint8_t* sv, uint8_t nbVisSat);
    int16_t gnssReadWarmStartStatus(uint8_t bitMask, uint8_t* nbVisSat, uint32_t* timeElapsed);
    int16_t gnssGetSvSync(uint8_t mask, uint8_t nbSv, uint8_t* syncList);
    int16_t gnssWriteBitMaskSatActivated(uint8_t bitMask, uint32_t* bitMaskActivated0, uint32_t* bitMaskActivated1);
    void gnssAbort();

    int16_t cryptoSetKey(uint8_t keyId, const uint8_t* key);
    int16_t cryptoDeriveKey(uint8_t srcKeyId, uint8_t dstKeyId, const uint8_t* key);
    int16_t cryptoProcessJoinAccept(uint8_t decKeyId, uint8_t verKeyId, uint8_t lwVer, const uint8_t* header, const uint8_t* dataIn, size_t len, uint8_t* dataOut);
    int16_t cryptoComputeAesCmac(uint8_t keyId, const uint8_t* data, size_t len, uint32_t* mic);
    int16_t cryptoVerifyAesCmac(uint8_t keyId, uint32_t micExp, const uint8_t* data, size_t len, bool* result);
    int16_t cryptoAesEncrypt01(uint8_t keyId, const uint8_t* dataIn, size_t len, uint8_t* dataOut);
    int16_t cryptoAesEncrypt(uint8_t keyId, const uint8_t* dataIn, size_t len, uint8_t* dataOut);
    int16_t cryptoAesDecrypt(uint8_t keyId, const uint8_t* dataIn, size_t len, uint8_t* dataOut);
    int16_t cryptoStoreToFlash(void);
    int16_t cryptoRestoreFromFlash(void);
    int16_t cryptoSetParam(uint8_t id, uint32_t value);
    int16_t cryptoGetParam(uint8_t id, uint32_t* value);
    int16_t cryptoCheckEncryptedFirmwareImage(uint32_t offset, const uint32_t* data, size_t len, bool nonvolatile);
    int16_t cryptoCheckEncryptedFirmwareImageResult(bool* result);

    int16_t bootEraseFlash(void);
    int16_t bootWriteFlashEncrypted(uint32_t offset, const uint32_t* data, size_t len, bool nonvolatile);
    int16_t bootGetHash(uint8_t hash[RADIOLIB_LR11X0_HASH_LEN]);
    int16_t bootReboot(bool stay);
    int16_t bootGetPin(uint8_t* pin);
    int16_t bootGetChipEui(uint8_t* eui);
    int16_t bootGetJoinEui(uint8_t* eui);
    
#if !RADIOLIB_GODMODE
  protected:
#endif
    uint8_t chipType = 0;

#if !RADIOLIB_GODMODE
  private:
#endif
    uint8_t wifiScanMode = 0;
    bool gnss = false;

    int16_t modSetup(float tcxoVoltage, uint8_t modem);
    bool findChip(uint8_t ver);
    int16_t config(uint8_t modem);
    int16_t setPacketMode(uint8_t mode, uint8_t len);
    int16_t startCad(uint8_t symbolNum, uint8_t detPeak, uint8_t detMin, uint8_t exitMode, RadioLibTime_t timeout);
    int16_t setHeaderType(uint8_t hdrType, size_t len = 0xFF);

    // common methods to avoid some copy-paste
    int16_t bleBeaconCommon(uint16_t cmd, uint8_t chan, const uint8_t* payload, size_t len);
    int16_t cryptoCommon(uint16_t cmd, uint8_t keyId, const uint8_t* dataIn, size_t len, uint8_t* dataOut);
};

#endif

#endif
