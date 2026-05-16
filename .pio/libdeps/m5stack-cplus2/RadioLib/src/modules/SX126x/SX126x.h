#if !defined(_RADIOLIB_SX126X_H)
#define _RADIOLIB_SX126X_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_SX126X

#include "../../Module.h"

#include "../../protocols/PhysicalLayer/PhysicalLayer.h"
#include "../../utils/FEC.h"
#include "../../utils/CRC.h"

#include "SX126x_commands.h"
#include "SX126x_registers.h"

// SX126X physical layer properties
#define RADIOLIB_SX126X_FREQUENCY_STEP_SIZE                     0.9536743164
#define RADIOLIB_SX126X_MAX_PACKET_LENGTH                       255
#define RADIOLIB_SX126X_CRYSTAL_FREQ                            32.0f
#define RADIOLIB_SX126X_DIV_EXPONENT                            25

// LR-FHSS packet lengths
#define RADIOLIB_SX126X_LR_FHSS_MAX_ENC_SIZE                    (608)
#define RADIOLIB_SX126X_LR_FHSS_HEADER_BITS                     (114)
#define RADIOLIB_SX126X_LR_FHSS_HDR_BYTES                       (10)
#define RADIOLIB_SX126X_LR_FHSS_SYNC_WORD_BYTES                 (4)
#define RADIOLIB_SX126X_LR_FHSS_FRAG_BITS                       (48)
#define RADIOLIB_SX126X_LR_FHSS_BLOCK_PREAMBLE_BITS             (2)
#define RADIOLIB_SX126X_LR_FHSS_BLOCK_BITS                      (RADIOLIB_SX126X_LR_FHSS_FRAG_BITS + RADIOLIB_SX126X_LR_FHSS_BLOCK_PREAMBLE_BITS)

/*!
  \class SX126x
  \brief Base class for %SX126x series. All derived classes for %SX126x (e.g. SX1262 or SX1268) inherit from this base class.
  This class should not be instantiated directly from Arduino sketch, only from its derived classes.
*/
class SX126x: public PhysicalLayer {
  public:
    // introduce PhysicalLayer overloads
    using PhysicalLayer::transmit;
    using PhysicalLayer::receive;
    using PhysicalLayer::startTransmit;
    using PhysicalLayer::startReceive;
    using PhysicalLayer::readData;

    /*!
      \struct paTableEntry_t
      \brief This structure is used as entry in the PA lookup table, 
      to optimize PA configuration for minimum power consumption.
    */
    struct __attribute__((packed)) paTableEntry_t {
      uint8_t paDutyCycle: 4;
      uint8_t hpMax: 4;
      int8_t paVal;
    };

    /*!
      \brief Default constructor.
      \param mod Instance of Module that will be used to communicate with the radio.
    */
    explicit SX126x(Module* mod);

    /*!
      \brief Whether the module has an XTAL (true) or TCXO (false). Defaults to false.
    */
    bool XTAL;

    /*!
      \brief Whether to use XOSC (true) or RC (false) oscillator in standby mode. Defaults to false.
    */
    bool standbyXOSC;

    // basic methods

    /*!
      \brief Initialization method for LoRa modem.
      \param cr LoRa coding rate denominator. Allowed values range from 4 to 8. Note that a value of 4 means no coding,
      is undocumented and not recommended without your own FEC.
      \param syncWord 1-byte LoRa sync word.
      \param preambleLength LoRa preamble length in symbols. Allowed values range from 1 to 65535.
      \param tcxoVoltage TCXO reference voltage to be set on DIO3. Defaults to 1.6 V, set to 0 to skip.
      \param useRegulatorLDO Whether to use only LDO regulator (true) or DC-DC regulator (false). Defaults to false.
      \returns \ref status_codes
    */
    int16_t begin(uint8_t cr, uint8_t syncWord, uint16_t preambleLength, float tcxoVoltage, bool useRegulatorLDO = false);

    /*!
      \brief Initialization method for FSK modem.
      \param br FSK bit rate in kbps. Allowed values range from 0.6 to 300.0 kbps.
      \param freqDev Frequency deviation from carrier frequency in kHz. Allowed values range from 0.0 to 200.0 kHz.
      \param rxBw Receiver bandwidth in kHz. Allowed values are 4.8, 5.8, 7.3, 9.7, 11.7, 14.6, 19.5, 23.4, 29.3, 39.0,
      46.9, 58.6, 78.2, 93.8, 117.3, 156.2, 187.2, 234.3, 312.0, 373.6 and 467.0 kHz.
      \param preambleLength FSK preamble length in bits. Allowed values range from 0 to 65535.
      \param tcxoVoltage TCXO reference voltage to be set on DIO3. Defaults to 1.6 V, set to 0 to skip.
      \param useRegulatorLDO Whether to use only LDO regulator (true) or DC-DC regulator (false). Defaults to false.
      \returns \ref status_codes
    */
    int16_t beginFSK(float br, float freqDev, float rxBw, uint16_t preambleLength, float tcxoVoltage, bool useRegulatorLDO = false);

    /*!
      \brief Initialization method for BPSK modem.
      \param br FSK bit rate in kbps. Only 100 and 600 bps is supported.
      \param tcxoVoltage TCXO reference voltage to be set on DIO3. Defaults to 1.6 V, set to 0 to skip.
      \param useRegulatorLDO Whether to use only LDO regulator (true) or DC-DC regulator (false). Defaults to false.
      \returns \ref status_codes
    */
    int16_t beginBPSK(float br, float tcxoVoltage, bool useRegulatorLDO = false);

    /*!
      \brief Initialization method for LR-FHSS modem. This modem only supports transmission!
      \param bw LR-FHSS bandwidth, one of RADIOLIB_SX126X_LR_FHSS_BW_* values.
      \param cr LR-FHSS coding rate, one of RADIOLIB_SX126X_LR_FHSS_CR_* values.
      \param narrowGrid Whether to use narrow (3.9 kHz) or wide (25.39 kHz) grid spacing.
      \param tcxoVoltage TCXO reference voltage to be set on DIO3. Defaults to 1.6 V, set to 0 to skip.
      \param useRegulatorLDO Whether to use only LDO regulator (true) or DC-DC regulator (false). Defaults to false.
      \returns \ref status_codes
    */
    int16_t beginLRFHSS(uint8_t bw, uint8_t cr, bool narrowGrid, float tcxoVoltage, bool useRegulatorLDO = false);

    /*!
      \brief Sets LR-FHSS configuration.
      \param bw LR-FHSS bandwidth, one of RADIOLIB_SX126X_LR_FHSS_BW_* values.
      \param cr LR-FHSS coding rate, one of RADIOLIB_SX126X_LR_FHSS_CR_* values.
      \param hdrCount Header packet count, 1 - 4. Defaults to 3.
      \param hopSeqId 9-bit seed number for PRNG generation of the hopping sequence. Defaults to 0x13A.
      \returns \ref status_codes
    */
    int16_t setLrFhssConfig(uint8_t bw, uint8_t cr, uint8_t hdrCount = 3, uint16_t hopSeqId = 0x100);

    /*!
      \brief Reset method. Will reset the chip to the default state using RST pin.
      \param verify Whether correct module startup should be verified. When set to true, RadioLib will attempt to verify the module has started correctly
      by repeatedly issuing setStandby command. Enabled by default.
      \returns \ref status_codes
    */
    int16_t reset(bool verify = true);

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
      Configuration defaults to the values recommended by AN1200.48.
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
      \brief Sets the module to sleep mode. To wake the device up, call standby().
      Overload with warm start enabled for PhysicalLayer compatibility.
      \returns \ref status_codes
    */
    int16_t sleep() override; 
    
    /*!
      \brief Sets the module to sleep mode. To wake the device up, call standby().
      \param retainConfig Set to true to retain configuration of the currently active modem ("warm start")
      or to false to discard current configuration ("cold start"). Defaults to true.
      \returns \ref status_codes
    */
    int16_t sleep(bool retainConfig);

    /*!
      \brief Sets the module to standby mode (overload for PhysicalLayer compatibility, uses 13 MHz RC oscillator).
      \returns \ref status_codes
    */
    int16_t standby() override;

    /*!
      \brief Sets the module to standby mode.
      \param mode Oscillator to be used in standby mode. Can be set to RADIOLIB_SX126X_STANDBY_RC (13 MHz RC oscillator)
      or RADIOLIB_SX126X_STANDBY_XOSC (32 MHz external crystal oscillator).
      \returns \ref status_codes
    */
    int16_t standby(uint8_t mode) override;

    /*!
      \brief Sets the module to standby mode.
      \param mode Oscillator to be used in standby mode. Can be set to RADIOLIB_SX126X_STANDBY_RC (13 MHz RC oscillator)
      or RADIOLIB_SX126X_STANDBY_XOSC (32 MHz external crystal oscillator).
      \param wakeup Whether to force the module to wake up. Setting to true will immediately attempt to wake up the module.
      \returns \ref status_codes
    */
    int16_t standby(uint8_t mode, bool wakeup);

    /*!
      \brief Handle LR-FHSS hop. 
      When using LR-FHSS in interrupt-driven mode, this method MUST be called each time an interrupt is triggered!
      \returns \ref status_codes
    */
    int16_t hopLRFHSS();

    // interrupt methods

    /*!
      \brief Sets interrupt service routine to call when DIO1 activates.
      \param func ISR to call.
    */
    virtual void setDio1Action(void (*func)(void));

    /*!
      \brief Clears interrupt service routine to call when DIO1 activates.
    */
    virtual void clearDio1Action();

    /*!
      \brief Sets interrupt service routine to call when a packet is received.
      \param func ISR to call.
    */
    void setPacketReceivedAction(void (*func)(void)) override;

    /*!
      \brief Clears interrupt service routine to call when a packet is received.
    */
    void clearPacketReceivedAction() override;

    /*!
      \brief Sets interrupt service routine to call when a packet is sent.
      \param func ISR to call.
    */
    void setPacketSentAction(void (*func)(void)) override;

    /*!
      \brief Clears interrupt service routine to call when a packet is sent.
    */
    void clearPacketSentAction() override;

    /*!
      \brief Sets interrupt service routine to call when a channel scan is finished.
      \param func ISR to call.
    */
    void setChannelScanAction(void (*func)(void)) override;

    /*!
      \brief Clears interrupt service routine to call when a channel scan is finished.
    */
    void clearChannelScanAction() override;

    /*!
      \brief Clean up after transmission is done.
      \returns \ref status_codes
    */
    int16_t finishTransmit() override;

    /*!
      \brief Clean up after reception is done.
      \returns \ref status_codes
    */
    int16_t finishReceive() override;
    
    /*!
      \brief Interrupt-driven receive method with default parameters.
      Implemented for compatibility with PhysicalLayer.

      \returns \ref status_codes
    */
    int16_t startReceive() override;

    /*!
      \brief Interrupt-driven receive method where the device mostly sleeps and periodically wakes to listen.
      Note that this function assumes the unit will take 500us + TCXO_delay to change state.
      See datasheet section 13.1.7, version 1.2.

      \param rxPeriod The duration the receiver will be in Rx mode, in microseconds.
      \param sleepPeriod The duration the receiver will not be in Rx mode, in microseconds.

      \param irqFlags Sets the IRQ flags, defaults to RX done, RX timeout, CRC error and header error. 
      \param irqMask Sets the mask of IRQ flags that will trigger DIO1, defaults to RX done.
      \returns \ref status_codes
    */
    int16_t startReceiveDutyCycle(uint32_t rxPeriod, uint32_t sleepPeriod, RadioLibIrqFlags_t irqFlags = RADIOLIB_IRQ_RX_DEFAULT_FLAGS, RadioLibIrqFlags_t irqMask = RADIOLIB_IRQ_RX_DEFAULT_MASK);

    /*!
      \brief Calls \ref startReceiveDutyCycle with rxPeriod and sleepPeriod set so the unit shouldn't miss any messages.
      \param senderPreambleLength Expected preamble length of the messages to receive.
      If set to zero, the currently configured preamble length will be used. Defaults to zero.
      This value cannot exceed the configured preamble length. If the sender preamble length is variable, set the
      maximum expected length by calling setPreambleLength(maximumExpectedLength) prior to this method, and use the
      minimum expected length here.

      \param minSymbols Ensure that the unit will catch at least this many symbols of any preamble of the specified senderPreambleLength.
      To reliably latch a preamble, the receiver requires 8 symbols for SF7-12 and 12 symbols for SF5-6 (see datasheet section 6.1.1.1, version 1.2).
      If set to zero, the minimum required symbols will be used. Defaults to 0.

      If senderPreambleLength is less than 2*minSymbols + 1, this method is equivalent to startReceive().

      \param irqFlags Sets the IRQ flags, defaults to RX done, RX timeout, CRC error and header error.
      \param irqMask Sets the mask of IRQ flags that will trigger DIO1, defaults to RX done.
      \returns \ref status_codes
    */
    int16_t startReceiveDutyCycleAuto(uint16_t senderPreambleLength = 0, uint16_t minSymbols = 0, RadioLibIrqFlags_t irqFlags = RADIOLIB_IRQ_RX_DEFAULT_FLAGS, RadioLibIrqFlags_t irqMask = RADIOLIB_IRQ_RX_DEFAULT_MASK);

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
      \brief Interrupt-driven channel activity detection method. DIO1 will be activated
      when LoRa preamble is detected, or upon timeout. Defaults to CAD parameter values recommended by AN1200.48.
      \returns \ref status_codes
    */
    int16_t startChannelScan() override;

    /*!
      \brief Interrupt-driven channel activity detection method. DIO1 will be activated
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
      \brief Sets LoRa bandwidth. Allowed values are 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0 and 500.0 kHz.
      \param bw LoRa bandwidth to be set in kHz.
      \returns \ref status_codes
    */
    virtual int16_t setBandwidth(float bw);

    /*!
      \brief Sets LoRa spreading factor. Allowed values range from 5 to 12.
      \param sf LoRa spreading factor to be set.
      \returns \ref status_codes
    */
    virtual int16_t setSpreadingFactor(uint8_t sf);

    /*!
      \brief Sets LoRa coding rate denominator. Allowed values range from 4 to 8. Note that a value of 4 means no coding, 
      is undocumented and not recommended without your own FEC.
      \param cr LoRa coding rate denominator to be set.
      \param longInterleave Enable long interleaver when set to true.
      Note that with long interleaver enabled, CR 4/7 is not possible, there are packet length restrictions,
      and it is not compatible with SX127x radios.
      \returns \ref status_codes
    */
    int16_t setCodingRate(uint8_t cr, bool longInterleave = false);

    /*!
      \brief Sets LoRa sync word.
      \param syncWord LoRa sync word to be set.
      \param controlBits Undocumented control bits, required for compatibility purposes.
      \returns \ref status_codes
    */
    int16_t setSyncWord(uint8_t syncWord, uint8_t controlBits = 0x44);

    /*!
      \brief Sets current protection limit. Can be set in 2.5 mA steps.
      \param currentLimit current protection limit to be set in mA. Allowed values range from 0 to 140.
      \returns \ref status_codes
    */
    int16_t setCurrentLimit(float currentLimit);

    /*!
      \brief Reads current protection limit.
      \returns Currently configured overcurrent protection limit in mA.
    */
    float getCurrentLimit();

    /*!
      \brief Sets preamble length for LoRa or FSK modem. Allowed values range from 1 to 65535.
      \param preambleLength Preamble length to be set in symbols (LoRa) or bits (FSK).
      NOTE: In FSK mode, sync word length limits the preamble detector length
      (the number of preamble bits that must be detected to start receiving packet).
      For details, see the note in SX1261 datasheet, Rev 2.1, section 6.2.2.1, page 45.
      Preamble detector length is adjusted automatically each time this method is called.
      \returns \ref status_codes
    */
    int16_t setPreambleLength(size_t preambleLength) override;

    /*!
      \brief Sets FSK frequency deviation. Allowed values range from 0.0 to 200.0 kHz.
      \param freqDev FSK frequency deviation to be set in kHz.
      \returns \ref status_codes
    */
    int16_t setFrequencyDeviation(float freqDev) override;

    /*!
      \brief Sets FSK bit rate. Allowed values range from 0.6 to 300.0 kbps.
      \param br FSK bit rate to be set in kbps.
      \returns \ref status_codes
    */
    int16_t setBitRate(float br) override;

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
      \brief Sets FSK receiver bandwidth. Allowed values are 4.8, 5.8, 7.3, 9.7, 11.7, 14.6, 19.5,
      23.4, 29.3, 39.0, 46.9, 58.6, 78.2, 93.8, 117.3, 156.2, 187.2, 234.3, 312.0, 373.6 and 467.0 kHz.
      \param rxBw FSK receiver bandwidth to be set in kHz.
      \returns \ref status_codes
    */
    int16_t setRxBandwidth(float rxBw);

    /*!
      \brief Enables or disables Rx Boosted Gain mode as described in SX126x datasheet
      section 9.6 (SX1261/2 v2.1, SX1268 v1.1)
      \param rxbgm True for Rx Boosted Gain, false for Rx Power Saving Gain
      \param persist True to persist Rx gain setting when waking up from warm-start mode
      (e.g. when using Rx duty cycle mode).
      \returns \ref status_codes
    */
    int16_t setRxBoostedGainMode(bool rxbgm, bool persist = true);

    /*!
      \brief Sets time-bandwidth product of Gaussian filter applied for shaping.
      Allowed values are RADIOLIB_SHAPING_0_3, RADIOLIB_SHAPING_0_5, RADIOLIB_SHAPING_0_7 or RADIOLIB_SHAPING_1_0.
      Set to RADIOLIB_SHAPING_NONE to disable data shaping.
      \param sh Time-bandwidth product of Gaussian filter to be set.
      \returns \ref status_codes
    */
    int16_t setDataShaping(uint8_t sh) override;

    /*!
      \brief Sets FSK sync word in the form of array of up to 8 bytes.
      Can also set LR-FHSS sync word, but its length must be 4 bytes.
      \param syncWord FSK sync word to be set.
      \param len FSK sync word length in bytes.
      NOTE: In FSK mode, sync word length limits the preamble detector length
      (the number of preamble bits that must be detected to start receiving packet).
      For details, see the note in SX1261 datasheet, Rev 2.1, section 6.2.2.1, page 45.
      Preamble detector length is adjusted automatically each time this method is called.
      \returns \ref status_codes
    */
    int16_t setSyncWord(uint8_t* syncWord, size_t len) override;

    /*!
      \brief Sets CRC configuration.
      \param len CRC length in bytes, Allowed values are 1 or 2, set to 0 to disable CRC.
      \param initial Initial CRC value. FSK only. Defaults to 0x1D0F (CCIT CRC).
      \param polynomial Polynomial for CRC calculation. FSK only. Defaults to 0x1021 (CCIT CRC).
      \param inverted Invert CRC bytes. FSK only. Defaults to true (CCIT CRC).
      \returns \ref status_codes
    */
    int16_t setCRC(uint8_t len, uint16_t initial = 0x1D0F, uint16_t polynomial = 0x1021, bool inverted = true);

    /*!
      \brief Sets FSK whitening parameters.
      \param enabled True = Whitening enabled
      \param initial Initial value used for the whitening LFSR in FSK mode.
      By default set to 0x01FF for compatibility with SX127x and LoRaWAN.
      \returns \ref status_codes
    */
    int16_t setWhitening(bool enabled, uint16_t initial = 0x01FF);

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
      \brief Set DIO2 to function as RF switch (default in Semtech example designs).
      \returns \ref status_codes
    */
    int16_t setDio2AsRfSwitch(bool enable = true);

    /*!
      \brief Gets recorded signal strength indicator.
      Overload with packet mode enabled for PhysicalLayer compatibility.
      \returns RSSI value in dBm.
    */
    float getRSSI() override;

    /*!
      \brief Gets RSSI (Recorded Signal Strength Indicator).
      \param packet Whether to read last packet RSSI, or the current value.
      \returns RSSI value in dBm.
    */
    float getRSSI(bool packet);

    /*!
      \brief Gets SNR (Signal to Noise Ratio) of the last received packet. Only available for LoRa modem.
      \returns SNR of the last received packet in dB.
    */
    float getSNR() override;

    /*!
      \brief Gets frequency error of the latest received packet.
      WARNING: This functionality is based on SX128x implementation and not documented on SX126x.
      While it seems to be working, it should be used with caution!

      \returns Frequency error in Hz.
    */
    float getFrequencyError();

    /*!
      \brief Query modem for the packet length of received payload.
      \param update Not used for SX126x modules.
      \returns Length of last received packet in bytes.
    */
    size_t getPacketLength(bool update = true) override;

    /*!
      \brief Query modem for the packet length of received payload and Rx buffer offset.
      \param update Not used for SX126x modules.
      \param offset Pointer to variable to store the Rx buffer offset.
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
      \brief Set modem in fixed packet length mode. Available in FSK mode only.
      \param len Packet length.
      \returns \ref status_codes
    */
    int16_t fixedPacketLengthMode(uint8_t len = RADIOLIB_SX126X_MAX_PACKET_LENGTH);

    /*!
      \brief Set modem in variable packet length mode. Available in FSK mode only.
      \param maxLen Maximum packet length.
      \returns \ref status_codes
    */
    int16_t variablePacketLengthMode(uint8_t maxLen = RADIOLIB_SX126X_MAX_PACKET_LENGTH);

    /*!
      \brief Calculate the expected time-on-air for a given modem, data rate, packet configuration and payload size.
      \param modem Modem type.
      \param dr Data rate.
      \param pc Packet config.
      \param len Payload length in bytes.
      \returns Expected time-on-air in microseconds.
    */
    RadioLibTime_t calculateTimeOnAir(ModemType_t modem, DataRate_t dr, PacketConfig_t pc, size_t len) override;

    /*!
      \brief Get expected time-on-air for a given size of payload
      \param len Payload length in bytes.
      \returns Expected time-on-air in microseconds.
    */
    RadioLibTime_t getTimeOnAir(size_t len) override;

    /*!
      \brief Calculate the timeout value for this specific module / series (in number of symbols or units of time)
      \param timeoutUs Timeout in microseconds to listen for
      \returns Timeout value in a unit that is specific for the used module
    */
    RadioLibTime_t calculateRxTimeout(RadioLibTime_t timeoutUs) override;

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
      \brief Sets transmission encoding. Available in FSK mode only. Serves only as alias for PhysicalLayer compatibility.
      \param encoding Encoding to be used. Set to 0 for NRZ, and 2 for whitening.
      \returns \ref status_codes
    */
    int16_t setEncoding(uint8_t encoding) override;

    /*! \copydoc Module::setRfSwitchPins */
    void setRfSwitchPins(uint32_t rxEn, uint32_t txEn);

    /*! \copydoc Module::setRfSwitchTable */
    void setRfSwitchTable(const uint32_t (&pins)[Module::RFSWITCH_MAX_PINS], const Module::RfSwitchMode_t table[]);

    /*!
      \brief Forces LoRa low data rate optimization. Only available in LoRa mode. After calling this method,
      LDRO will always be set to the provided value, regardless of symbol length.
      To re-enable automatic LDRO configuration, call SX126x::autoLDRO()

      \param enable Force LDRO to be always enabled (true) or disabled (false).
      \returns \ref status_codes
    */
    int16_t forceLDRO(bool enable);

    /*!
      \brief Re-enables automatic LDRO configuration. Only available in LoRa mode.
      After calling this method, LDRO will be enabled automatically when symbol length exceeds 16 ms.

      \returns \ref status_codes
    */
    int16_t autoLDRO();

    /*!
      \brief Get one truly random byte from RSSI noise.
      \returns TRNG byte.
    */
    uint8_t randomByte() override;

    /*!
      \brief Enable/disable inversion of the I and Q signals
      \param enable IQ inversion enabled (true) or disabled (false);
      \returns \ref status_codes
    */
    int16_t invertIQ(bool enable) override;

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

    #if !RADIOLIB_EXCLUDE_DIRECT_RECEIVE
    /*!
      \brief Set interrupt service routine function to call when data bit is received in direct mode.
      \param func Pointer to interrupt service routine.
    */
    void setDirectAction(void (*func)(void)) override;

    /*!
      \brief Function to read and process data bit in direct reception mode.
      \param pin Pin on which to read.
    */
    void readBit(uint32_t pin) override;
    #endif

    /*!
      \brief Upload binary patch into the SX126x device RAM.
      Patch is needed to e.g., enable spectral scan and must be uploaded again on every power cycle.
      \param patch Binary patch to upload.
      \param len Length of the binary patch in 4-byte words.
      \param nonvolatile Set to true when the patch is saved in non-volatile memory of the host processor,
      or to false when the patch is in its RAM.
      \returns \ref status_codes
    */
    int16_t uploadPatch(const uint32_t* patch, size_t len, bool nonvolatile = true);

    /*!
      \brief Start spectral scan. Requires binary path to be uploaded.
      \param numSamples Number of samples for each scan. Fewer samples = better temporal resolution.
      \param window RSSI averaging window size.
      \param interval Scan interval length, one of RADIOLIB_SX126X_SCAN_INTERVAL_* macros.
      \returns \ref status_codes
    */
    int16_t spectralScanStart(uint16_t numSamples, uint8_t window = RADIOLIB_SX126X_SPECTRAL_SCAN_WINDOW_DEFAULT, uint8_t interval = RADIOLIB_SX126X_SCAN_INTERVAL_8_20_US);
    
    /*!
      \brief Abort an ongoing spectral scan.
    */
    void spectralScanAbort();

    /*!
      \brief Read the status of spectral scan.
      \returns \ref status_codes
    */
    int16_t spectralScanGetStatus();

    /*!
      \brief Read the result of spectral scan.
      \param results Array to which the results will be saved, must be RADIOLIB_SX126X_SPECTRAL_SCAN_RES_SIZE long.
      \returns \ref status_codes
    */
    int16_t spectralScanGetResult(uint16_t* results);

    /*!
      \brief Set the PA configuration. Allows user to optimize PA for a specific output power
      and matching network. Any calls to this method must be done after calling begin/beginFSK and/or setOutputPower.
      WARNING: Use at your own risk! Setting invalid values can and will lead to permanent damage!
      \param paDutyCycle PA duty cycle raw value.
      \param deviceSel Device select, usually RADIOLIB_SX126X_PA_CONFIG_SX1261,
      RADIOLIB_SX126X_PA_CONFIG_SX1262 or RADIOLIB_SX126X_PA_CONFIG_SX1268.
      \param hpMax hpMax raw value.
      \param paLut paLut PA lookup table raw value.
      \returns \ref status_codes
    */
    int16_t setPaConfig(uint8_t paDutyCycle, uint8_t deviceSel, uint8_t hpMax = RADIOLIB_SX126X_PA_CONFIG_HP_MAX, uint8_t paLut = RADIOLIB_SX126X_PA_CONFIG_PA_LUT);

     /*!
      \brief Perform image rejection calibration for the specified frequency.
      Will try to use Semtech-defined presets first, and if none of them matches,
      custom iamge calibration will be attempted using calibrateImageRejection.
      \param freq Frequency to perform the calibration for.
      \returns \ref status_codes
    */
    int16_t calibrateImage(float freq);

    /*!
      \brief Perform image rejection calibration for the specified frequency band.
      WARNING: Use at your own risk! Setting incorrect values may lead to decreased performance
      \param freqMin Frequency band lower bound.
      \param freqMax Frequency band upper bound.
      \returns \ref status_codes
    */
    int16_t calibrateImageRejection(float freqMin, float freqMax);

    /*!
      \brief Set PA ramp-up time. Set to 200us by default.
      \returns \ref status_codes
    */
    int16_t setPaRampTime(uint8_t rampTime);

    /*!
      \brief Sets output power. Allowed values are in range from -9 to 22 dBm.
      This method allows user full control over PA configuration parameters.
      If you set incorrect PA configuration values, you can fail to reach the
      desired output power level or damage your device.
      Unless you can verify the output power, it is strongly advised to use
      SX1262::setOutputPower(power) or SX1268::setOutputPower(power).
      \param power Output power to be set in dBm.
      \param paDutyCycle Raw PA duty cycle value.
      \param hpMax Raw hpMax value.
      \param deviceSel Device select. Use either RADIOLIB_SX126X_PA_CONFIG_SX1262 or RADIOLIB_SX126X_PA_CONFIG_SX1268.
      \returns \ref status_codes
    */
    int16_t setOutputPower(int8_t power, uint8_t paDutyCycle, uint8_t hpMax, uint8_t deviceSel);

#if !RADIOLIB_GODMODE && !RADIOLIB_LOW_LEVEL
  protected:
#endif
    Module* getMod() override;
    
    // SX126x SPI command implementations
    int16_t setFs();
    int16_t setTx(uint32_t timeout = 0);
    int16_t setRx(uint32_t timeout);
    int16_t setCad(uint8_t symbolNum, uint8_t detPeak, uint8_t detMin, uint8_t exitMode, RadioLibTime_t timeout);
    int16_t writeRegister(uint16_t addr, const uint8_t* data, uint8_t numBytes);
    int16_t readRegister(uint16_t addr, uint8_t* data, uint8_t numBytes);
    int16_t writeBuffer(const uint8_t* data, uint8_t numBytes, uint8_t offset = 0x00);
    int16_t readBuffer(uint8_t* data, uint8_t numBytes, uint8_t offset = 0x00);
    int16_t setDioIrqParams(uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask = RADIOLIB_SX126X_IRQ_NONE, uint16_t dio3Mask = RADIOLIB_SX126X_IRQ_NONE);
    virtual int16_t clearIrqStatus(uint16_t clearIrqParams = RADIOLIB_SX126X_IRQ_ALL);
    int16_t setRfFrequency(uint32_t frf);
    int16_t calibrateImage(const uint8_t* data);
    uint8_t getPacketType();
    int16_t setTxParams(uint8_t power, uint8_t rampTime);
    int16_t setModulationParams(uint8_t sf, uint8_t bw, uint8_t cr, uint8_t ldro);
    int16_t setModulationParamsFSK(uint32_t br, uint8_t sh, uint8_t rxBw, uint32_t freqDev);
    int16_t setModulationParamsBPSK(uint32_t br, uint8_t sh = RADIOLIB_SX126X_BPSK_PULSE_SHAPE);
    int16_t setPacketParams(uint16_t preambleLen, uint8_t crcType, uint8_t payloadLen, uint8_t hdrType, uint8_t invertIQ);
    int16_t setPacketParamsFSK(uint16_t preambleLen, uint8_t preambleDetectorLen, uint8_t crcType, uint8_t syncWordLen, uint8_t addrCmp, uint8_t whiten, uint8_t packType = RADIOLIB_SX126X_GFSK_PACKET_VARIABLE, uint8_t payloadLen = 0xFF);
    int16_t setPacketParamsBPSK(uint8_t payloadLen, uint16_t rampUpDelay, uint16_t rampDownDelay, uint16_t payloadLenBits);
    int16_t setBufferBaseAddress(uint8_t txBaseAddress = 0x00, uint8_t rxBaseAddress = 0x00);
    int16_t setRegulatorMode(uint8_t mode);
    uint8_t getStatus();
    uint32_t getPacketStatus();
    uint16_t getDeviceErrors();
    int16_t clearDeviceErrors();

#if !RADIOLIB_GODMODE
  protected:
#endif
    const char* chipType = NULL;
    uint8_t bandwidth = 0;
    float freqMHz = 0;
    
    // Allow subclasses to define different TX modes
    uint8_t txMode = Module::MODE_TX;

    int16_t setFrequencyRaw(float freq);
    int16_t fixPaClamping(bool enable = true);

    // common low-level SPI interface
    static int16_t SPIparseStatus(uint8_t in);

#if !RADIOLIB_GODMODE
  private:
#endif
    Module* mod;

    uint8_t spreadingFactor = 0, codingRate = 0, ldrOptimize = 0, crcTypeLoRa = 0, headerType = 0;
    uint16_t preambleLengthLoRa = 0;
    float bandwidthKhz = 0;
    bool ldroAuto = true;

    uint32_t bitRate = 0, frequencyDev = 0;
    uint8_t preambleDetLength = 0, rxBandwidth = 0, pulseShape = 0, crcTypeFSK = 0, syncWordLength = 0, whitening = 0, packetType = 0;
    uint16_t preambleLengthFSK = 0;
    float rxBandwidthKhz = 0;

    uint32_t tcxoDelay = 0;
    uint8_t pwr = 0;

    size_t implicitLen = 0;
    uint8_t invertIQEnabled = RADIOLIB_SX126X_LORA_IQ_STANDARD;
    uint32_t rxTimeout = 0;

    // LR-FHSS stuff - there's a lot of it because all the encoding happens in software
    uint8_t lrFhssCr = RADIOLIB_SX126X_LR_FHSS_CR_2_3;
    uint8_t lrFhssBw = RADIOLIB_SX126X_LR_FHSS_BW_722_66;
    uint8_t lrFhssHdrCount = 3;
    uint8_t lrFhssSyncWord[RADIOLIB_SX126X_LR_FHSS_SYNC_WORD_BYTES] = { 0x12, 0xAD, 0x10, 0x1B };
    bool lrFhssGridNonFcc = false;
    uint16_t lrFhssNgrid = 0;
    uint16_t lrFhssLfsrState = 0;
    uint16_t lrFhssPoly = 0;
    uint16_t lrFhssSeed = 0;
    uint16_t lrFhssHopSeqId = 0;
    size_t lrFhssFrameBitsRem = 0;
    size_t lrFhssFrameHopsRem = 0;
    size_t lrFhssHopNum = 0;

    int16_t modSetup(float tcxoVoltage, bool useRegulatorLDO, uint8_t modem);
    int16_t config(uint8_t modem);
    bool findChip(const char* verStr);
    int16_t startReceiveCommon(uint32_t timeout = RADIOLIB_SX126X_RX_TIMEOUT_INF, RadioLibIrqFlags_t irqFlags = RADIOLIB_IRQ_RX_DEFAULT_FLAGS, RadioLibIrqFlags_t irqMask = RADIOLIB_IRQ_RX_DEFAULT_MASK);
    int16_t setPacketMode(uint8_t mode, uint8_t len);
    int16_t setHeaderType(uint8_t hdrType, size_t len = 0xFF);
    int16_t directMode();
    int16_t packetMode();

    // fixes to errata
    int16_t fixSensitivity();
    int16_t fixImplicitTimeout();
    int16_t fixInvertedIQ(uint8_t iqConfig);
    int16_t fixGFSK();

    // LR-FHSS utilities
    int16_t buildLRFHSSPacket(const uint8_t* in, size_t in_len, uint8_t* out, size_t* out_len, size_t* out_bits, size_t* out_hops);
    int16_t resetLRFHSS();
    uint16_t stepLRFHSS();
    int16_t setLRFHSSHop(uint8_t index);

    void regdump();
    void effectEvalPre(uint8_t* buff, uint32_t start);
    void effectEvalPost(uint8_t* buff, uint32_t start);
    void effectEval();
};

#endif

#endif
