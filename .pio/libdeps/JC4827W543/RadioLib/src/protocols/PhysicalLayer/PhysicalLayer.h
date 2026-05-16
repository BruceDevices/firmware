#if !defined(_RADIOLIB_PHYSICAL_LAYER_H)
#define _RADIOLIB_PHYSICAL_LAYER_H

#include "../../TypeDef.h"
#include "../../Module.h"

// common IRQ values - the IRQ flags in RadioLibIrqFlags_t arguments are offset by this value
enum RadioLibIrqType_t {
  RADIOLIB_IRQ_TX_DONE = 0x00,
  RADIOLIB_IRQ_RX_DONE = 0x01,
  RADIOLIB_IRQ_PREAMBLE_DETECTED = 0x02,
  RADIOLIB_IRQ_SYNC_WORD_VALID = 0x03,
  RADIOLIB_IRQ_HEADER_VALID = 0x04,
  RADIOLIB_IRQ_HEADER_ERR = 0x05,
  RADIOLIB_IRQ_CRC_ERR = 0x06,
  RADIOLIB_IRQ_CAD_DONE = 0x07,
  RADIOLIB_IRQ_CAD_DETECTED = 0x08,
  RADIOLIB_IRQ_TIMEOUT = 0x09,
  RADIOLIB_IRQ_NOT_SUPPORTED = 0x1F, // this must be the last value, intentionally set to 31
};

// some commonly used default values - defined here to ensure all modules have the same default behavior
#define RADIOLIB_IRQ_RX_DEFAULT_FLAGS       ((1UL << RADIOLIB_IRQ_RX_DONE) | (1UL << RADIOLIB_IRQ_TIMEOUT) | (1UL << RADIOLIB_IRQ_CRC_ERR) | (1UL << RADIOLIB_IRQ_HEADER_VALID) | (1UL << RADIOLIB_IRQ_HEADER_ERR))
#define RADIOLIB_IRQ_RX_DEFAULT_MASK        ((1UL << RADIOLIB_IRQ_RX_DONE))
#define RADIOLIB_IRQ_CAD_DEFAULT_FLAGS      ((1UL << RADIOLIB_IRQ_CAD_DETECTED) | (1UL << RADIOLIB_IRQ_CAD_DONE))
#define RADIOLIB_IRQ_CAD_DEFAULT_MASK       ((1UL << RADIOLIB_IRQ_CAD_DETECTED) | (1UL << RADIOLIB_IRQ_CAD_DONE))

/*!
  \struct LoRaRate_t
  \brief Data rate structure interpretation in case LoRa is used
*/
struct LoRaRate_t {
  /*! \brief LoRa spreading factor */
  uint8_t spreadingFactor;
  
  /*! \brief LoRa bandwidth in kHz */
  float bandwidth;

  /*! \brief LoRa coding rate denominator */
  uint8_t codingRate;
};

/*!
  \struct FSKRate_t
  \brief Data rate structure interpretation in case FSK is used
*/
struct FSKRate_t {
  /*! \brief FSK bit rate in kbps */
  float bitRate;
  
  /*! \brief FSK frequency deviation in kHz */
  float freqDev;
};

/*!
  \struct LrFhssRate_t
  \brief Data rate structure interpretation in case LR-FHSS is used
*/
struct LrFhssRate_t {
  /*! \brief Bandwidth */
  uint8_t bw;

  /*! \brief Coding rate */
  uint8_t cr;

  /*! \brief Grid spacing */
  bool narrowGrid;
};

/*!
  \union DataRate_t
  \brief Common data rate structure
*/
union DataRate_t {
  /*! \brief Interpretation for LoRa modems */
  LoRaRate_t lora;

  /*! \brief Interpretation for FSK modems */
  FSKRate_t fsk;

  /*! \brief Interpretation for LR-FHSS modems */
  LrFhssRate_t lrFhss;
};

struct LoRaPacketConfig_t {
  /*! \brief LoRa preamble length */
  uint16_t preambleLength;

  /*! \brief LoRa implicit header mode */
  bool implicitHeader;

  /*! \brief LoRa CRC mode */
  bool crcEnabled;

  /*! \brief LoRa low data rate optimization */
  bool ldrOptimize;
};

struct FSKPacketConfig_t {
  /*! \brief FSK preamble length in bits */
  uint16_t preambleLength;

  /*! \brief Length of the sync word in bits */
  uint8_t syncWordLength;

  /*! \brief FSK CRC length in bytes */
  uint8_t crcLength;
};

struct LrFhssPacketConfig_t {
  /*! \brief LR-FHSS header count (1 - 4) */
  uint8_t hdrCount;
};

/*!
  \union PacketConfig_t
  \brief Common packet configuration structure
*/
union PacketConfig_t {
  LoRaPacketConfig_t lora;
  FSKPacketConfig_t fsk;
  LrFhssPacketConfig_t lrFhss;
};

/*!
  \struct CADScanConfig_t
  \brief Channel scan configuration interpretation in case LoRa CAD is used
*/
struct CADScanConfig_t {
  /*! \brief Number of symbols to consider signal present */
  uint8_t symNum;
  
  /*! \brief Number of peak detection symbols */
  uint8_t detPeak;
  
  /*! \brief Number of minimum detection symbols */
  uint8_t detMin;
  
  /*! \brief Exit mode after signal detection is complete - module-specific value */
  uint8_t exitMode;
  
  /*! \brief Timeout in microseconds */
  RadioLibTime_t timeout;

  /*! \brief Optional IRQ flags to set, bits offset by the value of RADIOLIB_IRQ_ */
  RadioLibIrqFlags_t irqFlags;

  /*! \brief Optional IRQ mask to set, bits offset by the value of RADIOLIB_IRQ_ */
  RadioLibIrqFlags_t irqMask;
};

/*!
  \struct RSSIScanConfig_t
  \brief Channel scan configuration interpretation in case RSSI threshold is used
*/
struct RSSIScanConfig_t {
  /*! \brief RSSI limit in dBm */
  float limit;
};

/*!
  \union ChannelScanConfig_t
  \brief Common channel scan configuration structure
*/
union ChannelScanConfig_t {
  /*! \brief Interpretation for modems that use CAD (usually LoRa modems)*/
  CADScanConfig_t cad;

  /*! \brief Interpretation for modems that use RSSI threshold*/
  RSSIScanConfig_t rssi;
};

struct StandbyConfig_t {
  /*! \brief Module-specific standby mode configuration. */
  uint8_t mode;
};

struct ReceiveConfig_t {
  /*! \brief  Raw timeout value. Some modules use this argument to specify operation mode (single vs. continuous receive). */
  uint32_t timeout;
  
  /*! \brief Sets the IRQ flags. */
  RadioLibIrqFlags_t irqFlags;
  
  /*! \brief Sets the mask of IRQ flags that will trigger the radio interrupt pin. */
  RadioLibIrqFlags_t irqMask;
  
  /*! \brief Packet length, needed for some modules under special circumstances (e.g. LoRa implicit header mode). */
  size_t len;
};

struct TransmitConfig_t {
  /*! \brief Binary data that will be transmitted. */
  const uint8_t* data;

  /*! \brief Length of binary data to transmit (in bytes). */
  size_t len;

  /*! \brief Node address to transmit the packet to. Only used in FSK mode. */
  uint8_t addr;
};

struct SleepConfig_t {
  /*! \brief Module-specific sleep mode configuration. */
  uint8_t mode;
};

union RadioModeConfig_t {
  /*! \brief Interpretation for standby mode */
  StandbyConfig_t standby;

  /*! \brief Interpretation for Rx mode */
  ReceiveConfig_t receive;

  /*! \brief Interpretation for Tx mode */
  TransmitConfig_t transmit;

  /*! \brief Interpretation for scanning */
  ChannelScanConfig_t scan;

  /*! \brief Interpretation for sleep mode */
  SleepConfig_t sleep;
};

/*!
  \enum ModemType_t
  \brief Type of modem, used by setModem.
*/
enum ModemType_t {
  RADIOLIB_MODEM_FSK = 0,
  RADIOLIB_MODEM_LORA,
  RADIOLIB_MODEM_LRFHSS,
  RADIOLIB_MODEM_NONE,  // last entry
};

/*!
  \enum RadioModeType_t
  \brief Basic radio operating modes, used by stageMode.
*/
enum RadioModeType_t {
  RADIOLIB_RADIO_MODE_NONE = 0,
  RADIOLIB_RADIO_MODE_STANDBY,
  RADIOLIB_RADIO_MODE_RX,
  RADIOLIB_RADIO_MODE_TX,
  RADIOLIB_RADIO_MODE_SCAN,
  RADIOLIB_RADIO_MODE_SLEEP,
};

/*!
  \class PhysicalLayer

  \brief Provides common interface for protocols that run on %LoRa/FSK modules, such as RTTY or LoRaWAN.
  Also extracts some common module-independent methods. Using this interface class allows to use the protocols
  on various modules without much code duplicity. Because this class is used mainly as interface,
  all of its virtual members must be implemented in the module class.
*/
class PhysicalLayer {
  public:

    /*! \brief  Frequency step of the synthesizer in Hz. */
    float freqStep;

    /*! \brief  Maximum length of packet that can be received by the module. */
    size_t maxPacketLength;

    // constructor

    /*!
      \brief Default constructor.
    */
    PhysicalLayer();

    /*!
      \brief Default destructor.
    */
    virtual ~PhysicalLayer() = default;

    // basic methods

    #if defined(RADIOLIB_BUILD_ARDUINO)
    /*!
      \brief Arduino Flash String transmit method.
      \param str Pointer to Arduino Flash String that will be transmitted.
      \param addr Node address to transmit the packet to. Only used in FSK mode.
      \returns \ref status_codes
    */
    int16_t transmit(__FlashStringHelper* fstr, uint8_t addr = 0);

    /*!
      \brief Arduino String transmit method.
      \param str Address of Arduino string that will be transmitted.
      \param addr Node address to transmit the packet to. Only used in FSK mode.
      \returns \ref status_codes
    */
    int16_t transmit(String& str, uint8_t addr = 0);
    #endif

    /*!
      \brief C-string transmit method.
      \param str C-string that will be transmitted.
      \param addr Node address to transmit the packet to. Only used in FSK mode.
      \returns \ref status_codes
    */
    int16_t transmit(const char* str, uint8_t addr = 0);

    /*!
      \brief Binary transmit method. Must be implemented in module class.
      \param data Binary data that will be transmitted.
      \param len Length of binary data to transmit (in bytes).
      \param addr Node address to transmit the packet to. Only used in FSK mode.
      \returns \ref status_codes
    */
    virtual int16_t transmit(const uint8_t* data, size_t len, uint8_t addr = 0);

    #if defined(RADIOLIB_BUILD_ARDUINO)
    /*!
      \brief Arduino String receive method.
      \param str Address of Arduino String to save the received data.
      \param len Expected number of characters in the message. Leave as 0 if expecting a unknown size packet.
      \param timeout Reception timeout in milliseconds. If set to 0,
      timeout period will be calculated automatically based on the radio configuration.
      \returns \ref status_codes
    */
    int16_t receive(String& str, size_t len = 0, RadioLibTime_t timeout = 0);
    #endif

    /*!
      \brief Sets module to sleep.
      \returns \ref status_codes
    */
    virtual int16_t sleep();

    /*!
      \brief Sets module to standby.
      \returns \ref status_codes
    */
    virtual int16_t standby();

    /*!
      \brief Sets module to a specific standby mode.
      \returns \ref status_codes
    */
    virtual int16_t standby(uint8_t mode);

    /*!
      \brief Sets module to received mode using its default configuration.
      \returns \ref status_codes
    */
    virtual int16_t startReceive();

    /*!
      \brief Interrupt-driven receive method. A DIO pin will be activated when full packet is received. 
      Must be implemented in module class.
      \param timeout Raw timeout value. Some modules use this argument to specify operation mode
      (single vs. continuous receive).
      \param irqFlags Sets the IRQ flags.
      \param irqMask Sets the mask of IRQ flags that will trigger the radio interrupt pin.
      \param len Packet length, needed for some modules under special circumstances (e.g. LoRa implicit header mode).
      \returns \ref status_codes
    */
    virtual int16_t startReceive(uint32_t timeout, RadioLibIrqFlags_t irqFlags = RADIOLIB_IRQ_RX_DEFAULT_FLAGS, RadioLibIrqFlags_t irqMask = RADIOLIB_IRQ_RX_DEFAULT_MASK, size_t len = 0);

    /*!
      \brief Binary receive method. Must be implemented in module class.
      \param data Pointer to array to save the received binary data.
      \param len Packet length, needed for some modules under special circumstances (e.g. LoRa implicit header mode).
      \param timeout Reception timeout in milliseconds. If set to 0,
      timeout period will be calculated automatically based on the radio configuration.
      \returns \ref status_codes
    */
    virtual int16_t receive(uint8_t* data, size_t len, RadioLibTime_t timeout = 0);

    #if defined(RADIOLIB_BUILD_ARDUINO)
    /*!
      \brief Interrupt-driven Arduino String transmit method. Unlike the standard transmit method, this one is non-blocking.
      Interrupt pin will be activated when transmission finishes.
      \param str Address of Arduino String that will be transmitted.
      \param addr Node address to transmit the packet to. Only used in FSK mode.
      \returns \ref status_codes
    */
    int16_t startTransmit(String& str, uint8_t addr = 0);
    #endif

    /*!
      \brief Interrupt-driven Arduino String transmit method. Unlike the standard transmit method, this one is non-blocking.
      Interrupt pin will be activated when transmission finishes.
      \param str C-string that will be transmitted.
      \param addr Node address to transmit the packet to. Only used in FSK mode.
      \returns \ref status_codes
    */
    int16_t startTransmit(const char* str, uint8_t addr = 0);

    /*!
      \brief Interrupt-driven binary transmit method.
      \param data Binary data that will be transmitted.
      \param len Length of binary data to transmit (in bytes).
      \param addr Node address to transmit the packet to. Only used in FSK mode.
      \returns \ref status_codes
    */
    virtual int16_t startTransmit(const uint8_t* data, size_t len, uint8_t addr = 0);

    /*!
      \brief Clean up after transmission is done.
      \returns \ref status_codes
    */
    virtual int16_t finishTransmit();

    /*!
      \brief Clean up after reception is done.
      \returns \ref status_codes
    */
    virtual int16_t finishReceive();

    #if defined(RADIOLIB_BUILD_ARDUINO)
    /*!
      \brief Reads data that was received after calling startReceive method.
      \param str Address of Arduino String to save the received data.
      \param len Expected number of characters in the message. When set to 0, the packet length will be retrieved 
      automatically. When more bytes than received are requested, only the number of bytes requested will be returned.
      \returns \ref status_codes
    */
    int16_t readData(String& str, size_t len = 0);
    #endif

    /*!
      \brief Reads data that was received after calling startReceive method.
      \param data Pointer to array to save the received binary data.
      \param len Number of bytes that will be read. When set to 0, the packet length will be retrieved automatically.
      When more bytes than received are requested, only the number of bytes requested will be returned.
      \returns \ref status_codes
    */
    virtual int16_t readData(uint8_t* data, size_t len);

    /*!
      \brief Enables direct transmission mode on pins DIO1 (clock) and DIO2 (data). Must be implemented in module class.
      While in direct mode, the module will not be able to transmit or receive packets. Can only be activated in FSK mode.
      \param frf 24-bit raw frequency value to start transmitting at. Required for quick frequency shifts in RTTY.
      \returns \ref status_codes
    */
    virtual int16_t transmitDirect(uint32_t frf = 0);

    /*!
      \brief Enables direct reception mode on pins DIO1 (clock) and DIO2 (data). Must be implemented in module class.
      While in direct mode, the module will not be able to transmit or receive packets. Can only be activated in FSK mode.
      \returns \ref status_codes
    */
    virtual int16_t receiveDirect();

    // configuration methods

    /*!
      \brief Sets carrier frequency. Must be implemented in module class.
      \param freq Carrier frequency to be set in MHz.
      \returns \ref status_codes
    */
    virtual int16_t setFrequency(float freq);

    /*!
      \brief Sets FSK bit rate. Only available in FSK mode. Must be implemented in module class.
      \param br Bit rate to be set (in kbps).
      \returns \ref status_codes
    */
    virtual int16_t setBitRate(float br);

    /*!
      \brief Sets FSK frequency deviation from carrier frequency. Only available in FSK mode.
      Must be implemented in module class.
      \param freqDev Frequency deviation to be set (in kHz).
      \returns \ref status_codes
    */
    virtual int16_t setFrequencyDeviation(float freqDev);

    /*!
      \brief Sets GFSK data shaping. Only available in FSK mode. Must be implemented in module class.
      \param sh Shaping to be set. See \ref config_shaping for possible values.
      \returns \ref status_codes
    */
    virtual int16_t setDataShaping(uint8_t sh);

    /*!
      \brief Sets FSK data encoding. Only available in FSK mode. Must be implemented in module class.
      \param encoding Encoding to be used. See \ref config_encoding for possible values.
      \returns \ref status_codes
    */
    virtual int16_t setEncoding(uint8_t encoding);

    /*!
      \brief Set IQ inversion. Must be implemented in module class if the module supports it.
      \param enable True to use inverted IQ, false for non-inverted.
      \returns \ref status_codes
    */
    virtual int16_t invertIQ(bool enable);

    /*!
      \brief Set output power. Must be implemented in module class if the module supports it.
      \param power Output power in dBm. The allowed range depends on the module used.
      \returns \ref status_codes
    */
    virtual int16_t setOutputPower(int8_t power);

    /*!
      \brief Check if output power is configurable. Must be implemented in module class if the module supports it.
      \param power Output power in dBm. The allowed range depends on the module used.
      \param clipped Clipped output power value to what is possible within the module's range.
      \returns \ref status_codes
    */
    virtual int16_t checkOutputPower(int8_t power, int8_t* clipped);

    /*!
      \brief Set sync word. Must be implemented in module class if the module supports it.
      \param sync Pointer to the sync word.
      \param len Sync word length in bytes. Maximum length depends on the module used.
      \returns \ref status_codes
    */
    virtual int16_t setSyncWord(uint8_t* sync, size_t len);

    /*!
      \brief Set preamble length. Must be implemented in module class if the module supports it.
      \param len Preamble length in bytes. Maximum length depends on the module used.
      \returns \ref status_codes
    */
    virtual int16_t setPreambleLength(size_t len);
    
    /*!
      \brief Set data rate. Must be implemented in module class if the module supports it.
      \param dr Data rate struct.
      \param modem The modem corresponding to the requested datarate (FSK, LoRa or LR-FHSS). 
      Defaults to currently active modem if not supplied.
      \returns \ref status_codes
    */
    virtual int16_t setDataRate(DataRate_t dr, ModemType_t modem = RADIOLIB_MODEM_NONE);

    /*!
      \brief Check the data rate can be configured by this module. Must be implemented in module class if the module supports it.
      \param dr Data rate struct.
      \param modem The modem corresponding to the requested datarate (FSK, LoRa or LR-FHSS). 
      Defaults to currently active modem if not supplied.
      \returns \ref status_codes
    */
    virtual int16_t checkDataRate(DataRate_t dr, ModemType_t modem = RADIOLIB_MODEM_NONE);

    /*!
      \brief Query modem for the packet length of received payload. Must be implemented in module class.
      \param update Update received packet length. Will return cached value when set to false.
      \returns Length of last received packet in bytes.
    */
    virtual size_t getPacketLength(bool update = true);

    /*!
      \brief Gets RSSI (Recorded Signal Strength Indicator) of the last received packet.
      \returns RSSI of the last received packet in dBm.
    */
    virtual float getRSSI();

    /*!
      \brief Gets SNR (Signal to Noise Ratio) of the last received packet. Only available for LoRa modem.
      \returns SNR of the last received packet in dB.
    */
    virtual float getSNR();
    
    /*!
      \brief Calculate the expected time-on-air for a given modem, data rate, packet configuration and payload size.
      \param modem Modem type.
      \param dr Data rate.
      \param pc Packet config.
      \param len Payload length in bytes.
      \returns Expected time-on-air in microseconds.
    */
    virtual RadioLibTime_t calculateTimeOnAir(ModemType_t modem, DataRate_t dr, PacketConfig_t pc, size_t len);

    /*!
      \brief Get expected time-on-air for a given size of payload
      \param len Payload length in bytes.
      \returns Expected time-on-air in microseconds.
    */
    virtual RadioLibTime_t getTimeOnAir(size_t len);

    /*!
      \brief Calculate the timeout value for this specific module / series 
      (in number of symbols or units of time).
      \param timeoutUs Timeout in microseconds to listen for.
      \returns Timeout value in a unit that is specific for the used module.
    */
    virtual RadioLibTime_t calculateRxTimeout(RadioLibTime_t timeoutUs);

    /*!
      \brief Convert from radio-agnostic IRQ flags to radio-specific flags.
      \param irq Radio-agnostic IRQ flags.
      \returns Flags for a specific radio module.
    */
    uint32_t getIrqMapped(RadioLibIrqFlags_t irq);

    /*!
      \brief Check whether a specific IRQ bit is set (e.g. RxTimeout, CadDone).
      \param irq IRQ type to check, one of RADIOLIB_IRQ_*.
      \returns 1 when requested IRQ is set, 0 when it is not or RADIOLIB_ERR_UNSUPPORTED if the IRQ is not supported.
    */
    int16_t checkIrq(RadioLibIrqType_t irq);

    /*!
      \brief Set interrupt on specific IRQ bit(s) (e.g. RxTimeout, CadDone).
      Keep in mind that not all radio modules support all RADIOLIB_IRQ_ flags!
      \param irq Flags to set, multiple bits may be enabled. IRQ to enable corresponds to the bit index (RadioLibIrq_t).
      For example, if bit 0 is enabled, the module will enable its RADIOLIB_IRQ_TX_DONE (if it is supported).
      \returns \ref status_codes
    */
    int16_t setIrq(RadioLibIrqFlags_t irq);

    /*!
      \brief Clear interrupt on a specific IRQ bit (e.g. RxTimeout, CadDone).
      Keep in mind that not all radio modules support all RADIOLIB_IRQ_ flags!
      \param irq Flags to set, multiple bits may be enabled. IRQ to enable corresponds to the bit index (RadioLibIrq_t).
      For example, if bit 0 is enabled, the module will enable its RADIOLIB_IRQ_TX_DONE (if it is supported).
      \returns \ref status_codes
    */
    int16_t clearIrq(RadioLibIrqFlags_t irq);

    /*!
      \brief Read currently active IRQ flags.
      Must be implemented in module class.
      \returns IRQ flags.
    */
    virtual uint32_t getIrqFlags();

    /*!
      \brief Set interrupt on DIO1 to be sent on a specific IRQ bit (e.g. RxTimeout, CadDone).
      Must be implemented in module class.
      \param irq Module-specific IRQ flags.
      \returns \ref status_codes
    */
    virtual int16_t setIrqFlags(uint32_t irq);

    /*!
      \brief Clear interrupt on a specific IRQ bit (e.g. RxTimeout, CadDone).
      Must be implemented in module class.
      \param irq Module-specific IRQ flags.
      \returns \ref status_codes
    */
    virtual int16_t clearIrqFlags(uint32_t irq);

    /*!
      \brief Interrupt-driven channel activity detection method. Interrupt will be activated
      when packet is detected. Must be implemented in module class.
      \returns \ref status_codes
    */
    virtual int16_t startChannelScan();

    /*!
      \brief Interrupt-driven channel activity detection method. interrupt will be activated
      when packet is detected. Must be implemented in module class.
      \param config Scan configuration structure. Interpretation depends on currently active modem.
      \returns \ref status_codes
    */
    virtual int16_t startChannelScan(const ChannelScanConfig_t &config);

    /*!
      \brief Read the channel scan result
      \returns \ref status_codes
    */
    virtual int16_t getChannelScanResult();

    /*!
      \brief Check whether the current communication channel is free or occupied. Performs CAD for LoRa modules,
      or RSSI measurement for FSK modules.
      \returns RADIOLIB_CHANNEL_FREE when channel is free,
      RADIOLIB_PREAMBLE_DETECTEDwhen occupied or other \ref status_codes.
    */
    virtual int16_t scanChannel();

    /*!
      \brief Check whether the current communication channel is free or occupied. Performs CAD for LoRa modules,
      or RSSI measurement for FSK modules.
      \param config Scan configuration structure. Interpretation depends on currently active modem.
      \returns RADIOLIB_CHANNEL_FREE when channel is free,
      RADIOLIB_PREAMBLE_DETECTEDwhen occupied or other \ref status_codes.
    */
    virtual int16_t scanChannel(const ChannelScanConfig_t &config);

    /*!
      \brief Get truly random number in range 0 - max.
      \param max The maximum value of the random number (non-inclusive).
      \returns Random number.
    */
    int32_t random(int32_t max);

    /*!
      \brief Get truly random number in range min - max.
      \param min The minimum value of the random number (inclusive).
      \param max The maximum value of the random number (non-inclusive).
      \returns Random number.
    */
    int32_t random(int32_t min, int32_t max);

    /*!
      \brief Get one truly random byte from RSSI noise. Must be implemented in module class.
      \returns TRNG byte.
    */
    virtual uint8_t randomByte();

    /*!
      \brief Configure module parameters for direct modes. Must be called prior to "ham" modes like RTTY or AX.25.
      Only available in FSK mode.
      \returns \ref status_codes
    */
    int16_t startDirect();

    #if !RADIOLIB_EXCLUDE_DIRECT_RECEIVE
    /*!
      \brief Set sync word to be used to determine start of packet in direct reception mode.
      \param syncWord Sync word bits.
      \param len Sync word length in bits. Set to zero to disable sync word matching.
      \returns \ref status_codes
    */
    int16_t setDirectSyncWord(uint32_t syncWord, uint8_t len);

    /*!
      \brief Set interrupt service routine function to call when data bit is received in direct mode.
      Must be implemented in module class.
      \param func Pointer to interrupt service routine.
    */
    virtual void setDirectAction(void (*func)(void));

    /*!
      \brief Function to read and process data bit in direct reception mode. Must be implemented in module class.
      \param pin Pin on which to read.
    */
    virtual void readBit(uint32_t pin);

    /*!
      \brief Get the number of direct mode bytes currently available in buffer.
      \returns Number of available bytes.
    */
    int16_t available();

    /*!
      \brief Forcefully drop synchronization.
    */
    void dropSync();

    /*!
      \brief Get data from direct mode buffer.
      \param drop Drop synchronization on read - next reading will require waiting for the sync word again.
      Defaults to true.
      \returns Byte from direct mode buffer.
    */
    uint8_t read(bool drop = true);
    #endif

    /*!
      \brief Sets interrupt service routine to call when a packet is received.
      \param func ISR to call.
    */
    virtual void setPacketReceivedAction(void (*func)(void));

    /*!
      \brief Clears interrupt service routine to call when a packet is received.
    */
    virtual void clearPacketReceivedAction();

    /*!
      \brief Sets interrupt service routine to call when a packet is sent.
      \param func ISR to call.
    */
    virtual void setPacketSentAction(void (*func)(void));

    /*!
      \brief Clears interrupt service routine to call when a packet is sent.
    */
    virtual void clearPacketSentAction();
    
    /*!
      \brief Sets interrupt service routine to call when a channel scan is finished.
      \param func ISR to call.
    */
    virtual void setChannelScanAction(void (*func)(void));

    /*!
      \brief Clears interrupt service routine to call when a channel scan is finished.
    */
    virtual void clearChannelScanAction();

    /*!
      \brief Set modem for the radio to use. Will perform full reset and reconfigure the radio
      using its default parameters.
      \param modem Modem type to set. Not all modems are implemented by all radio modules!
      \returns \ref status_codes
    */
    virtual int16_t setModem(ModemType_t modem);

    /*!
      \brief Get modem currently in use by the radio.
      \param modem Pointer to a variable to save the retrieved configuration into.
      \returns \ref status_codes
    */
    virtual int16_t getModem(ModemType_t* modem);

    /*!
      \brief Stage mode of the radio to be launched later using launchMode.
      \param mode Radio mode to prepare.
      \param cfg Configuration of this mode (mode-dependent).
      \returns \ref status_codes
    */
    virtual int16_t stageMode(RadioModeType_t mode, RadioModeConfig_t* cfg);

    /*!
      \brief Launch previously staged mode.
      \returns \ref status_codes
    */
    virtual int16_t launchMode();

    #if RADIOLIB_INTERRUPT_TIMING

    /*!
      \brief Set function to be called to set up the timing interrupt.
      For details, see https://github.com/jgromes/RadioLib/wiki/Interrupt-Based-Timing
      \param func Setup function to be called, with one argument (pulse length in microseconds).
    */
    void setInterruptSetup(void (*func)(uint32_t));

    /*!
      \brief Set timing interrupt flag.
      For details, see https://github.com/jgromes/RadioLib/wiki/Interrupt-Based-Timing
    */
    void setTimerFlag();

    #endif

#if !RADIOLIB_GODMODE
  protected:
#endif
    uint32_t irqMap[10] = { 0 };
    RadioModeType_t stagedMode = RADIOLIB_RADIO_MODE_NONE;

#if !RADIOLIB_EXCLUDE_DIRECT_RECEIVE
    void updateDirectBuffer(uint8_t bit);
#endif

#if !RADIOLIB_GODMODE
  private:
#endif

    #if !RADIOLIB_EXCLUDE_DIRECT_RECEIVE
    uint8_t bufferBitPos = 0;
    uint8_t bufferWritePos = 0;
    uint8_t bufferReadPos = 0;
    uint8_t buffer[RADIOLIB_STATIC_ARRAY_SIZE] = { 0 };
    uint32_t syncBuffer = 0;
    uint32_t directSyncWord = 0;
    uint8_t directSyncWordLen = 0;
    uint32_t directSyncWordMask = 0;
    bool gotSync = false;
    #endif

    virtual Module* getMod() = 0;

    // allow specific classes access the private getMod method
    friend class AFSKClient;
    friend class RTTYClient;
    friend class MorseClient;
    friend class HellClient;
    friend class SSTVClient;
    friend class AX25Client;
    friend class FSK4Client;
    friend class PagerClient;
    friend class BellClient;
    friend class FT8Client;
    friend class LoRaWANNode;
    friend class M17Client;
};

#endif
