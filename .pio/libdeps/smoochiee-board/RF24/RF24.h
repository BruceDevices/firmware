/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

/**
 * @file RF24.h
 *
 * Class declaration for RF24 and helper enums
 */

#ifndef RF24_H_
#define RF24_H_

#include "RF24_config.h"

#if defined(RF24_LINUX) || defined(LITTLEWIRE)
    #include "utility/includes.h"
#elif defined SOFTSPI
    #include <DigitalIO.h>
#endif

/**
 * @defgroup PALevel Power Amplifier level
 * Power Amplifier level. The units dBm (decibel-milliwatts or dB<sub>mW</sub>)
 * represents a logarithmic signal loss.
 * @see
 * - RF24::setPALevel()
 * - RF24::getPALevel()
 * @{
 */
typedef enum
{
    /**
     * (0) represents:
     * nRF24L01 | Si24R1 with<br>lnaEnabled = 1 | Si24R1 with<br>lnaEnabled = 0
     * :-------:|:-----------------------------:|:----------------------------:
     *  -18 dBm | -6 dBm | -12 dBm
     */
    RF24_PA_MIN = 0,
    /**
     * (1) represents:
     * nRF24L01 | Si24R1 with<br>lnaEnabled = 1 | Si24R1 with<br>lnaEnabled = 0
     * :-------:|:-----------------------------:|:----------------------------:
     *  -12 dBm | 0 dBm | -4 dBm
     */
    RF24_PA_LOW,
    /**
     * (2) represents:
     * nRF24L01 | Si24R1 with<br>lnaEnabled = 1 | Si24R1 with<br>lnaEnabled = 0
     * :-------:|:-----------------------------:|:----------------------------:
     *  -6 dBm | 3 dBm | 1 dBm
     */
    RF24_PA_HIGH,
    /**
     * (3) represents:
     * nRF24L01 | Si24R1 with<br>lnaEnabled = 1 | Si24R1 with<br>lnaEnabled = 0
     * :-------:|:-----------------------------:|:----------------------------:
     *  0 dBm | 7 dBm | 4 dBm
     */
    RF24_PA_MAX,
    /**
     * (4) This should not be used and remains for backward compatibility.
     */
    RF24_PA_ERROR
} rf24_pa_dbm_e;

/**
 * @}
 * @defgroup Datarate datarate
 * How fast data moves through the air. Units are in bits per second (bps).
 * @see
 * - RF24::setDataRate()
 * - RF24::getDataRate()
 * @{
 */
typedef enum
{
    /** (0) represents 1 Mbps */
    RF24_1MBPS = 0,
    /** (1) represents 2 Mbps */
    RF24_2MBPS,
    /** (2) represents 250 kbps */
    RF24_250KBPS
} rf24_datarate_e;

/**
 * @}
 * @defgroup CRCLength CRC length
 * The length of a CRC checksum that is used (if any). Cyclical Redundancy
 * Checking (CRC) is commonly used to ensure data integrity.
 * @see
 * - RF24::setCRCLength()
 * - RF24::getCRCLength()
 * - RF24::disableCRC()
 * @{
 */
typedef enum
{
    /** (0) represents no CRC checksum is used */
    RF24_CRC_DISABLED = 0,
    /** (1) represents CRC 8 bit checksum is used */
    RF24_CRC_8,
    /** (2) represents CRC 16 bit checksum is used */
    RF24_CRC_16
} rf24_crclength_e;

/**
 * @}
 * @defgroup fifoState FIFO state
 * The state of a single FIFO (RX or TX).
 * Remember, each FIFO has a maximum occupancy of 3 payloads.
 * @see RF24::isFifo()
 * @{
 */
typedef enum
{
    /// @brief The FIFO is not full nor empty, but it is occupied with 1 or 2 payloads.
    RF24_FIFO_OCCUPIED,
    /// @brief The FIFO is empty.
    RF24_FIFO_EMPTY,
    /// @brief The FIFO is full.
    RF24_FIFO_FULL,
    /// @brief Represents corruption of data over SPI (when observed).
    RF24_FIFO_INVALID,
} rf24_fifo_state_e;

/**
 * @}
 * @brief Driver class for nRF24L01(+) 2.4GHz Wireless Transceiver
 */
class RF24
{
private:
#ifdef SOFTSPI
    SoftSPI<SOFT_SPI_MISO_PIN, SOFT_SPI_MOSI_PIN, SOFT_SPI_SCK_PIN, SPI_MODE> spi;
#elif defined(SPI_UART)
    SPIUARTClass uspi;
#endif

#if defined(RF24_LINUX) || defined(XMEGA_D3) /* XMEGA can use SPI class */
    SPI spi;
#endif // defined (RF24_LINUX) || defined (XMEGA_D3)
#if defined(RF24_SPI_PTR)
    _SPI* _spi;
#endif // defined (RF24_SPI_PTR)

    rf24_gpio_pin_t ce_pin;  /* "Chip Enable" pin, activates the RX or TX role */
    rf24_gpio_pin_t csn_pin; /* SPI Chip select */
    uint32_t spi_speed;      /* SPI Bus Speed */
#if defined(RF24_LINUX) || defined(XMEGA_D3) || defined(RF24_RP2)
    uint8_t spi_rxbuff[32 + 1]; //SPI receive buffer (payload max 32 bytes)
    uint8_t spi_txbuff[32 + 1]; //SPI transmit buffer (payload max 32 bytes + 1 byte for the command)
#endif
    uint8_t status;                   /* The status byte returned from every SPI transaction */
    uint8_t payload_size;             /* Fixed size of payloads */
    uint8_t pipe0_reading_address[5]; /* Last address set on pipe 0 for reading. */
    uint8_t config_reg;               /* For storing the value of the NRF_CONFIG register */
    bool _is_p_variant;               /* For storing the result of testing the toggleFeatures() affect */
    bool _is_p0_rx;                   /* For keeping track of pipe 0's usage in user-triggered RX mode. */

protected:
    /**
     * SPI transactions
     *
     * Common code for SPI transactions including CSN toggle
     *
     */
    inline void beginTransaction();

    inline void endTransaction();

    /** Whether ack payloads are enabled. */
    bool ack_payloads_enabled;
    /** The address width to use (3, 4 or 5 bytes). */
    uint8_t addr_width;
    /** Whether dynamic payloads are enabled. */
    bool dynamic_payloads_enabled;

    /**
     * Read a chunk of data in from a register
     *
     * @param reg Which register. Use constants from nRF24L01.h
     * @param[out] buf Where to put the data
     * @param len How many bytes of data to transfer
     * @note This returns nothing. Older versions of this function returned the status
     * byte, but that it now saved to a private member on all SPI transactions.
     */
    void read_register(uint8_t reg, uint8_t* buf, uint8_t len);

    /**
     * Read single byte from a register
     *
     * @param reg Which register. Use constants from nRF24L01.h
     * @return Current value of register @p reg
     */
    uint8_t read_register(uint8_t reg);

public:
    /**
     * @name Primary public interface
     *
     *  These are the main methods you need to operate the chip
     */
    /**@{*/

    /**
     * RF24 Constructor
     *
     * Creates a new instance of this driver.  Before using, you create an instance
     * and send in the unique pins that this chip is connected to.
     *
     * See [Related Pages](pages.html) for device specific information
     *
     * @param _cepin The pin attached to Chip Enable on the RF module
     * @param _cspin The pin attached to Chip Select (often labeled CSN) on the radio module.
     * - For the Arduino Due board, the [Arduino Due extended SPI feature](https://www.arduino.cc/en/Reference/DueExtendedSPI)
     * is not supported. This means that the Due's pins 4, 10, or 52 are not mandated options (can use any digital output pin) for
     * the radio's CSN pin.
     * @param _spi_speed The SPI speed in Hz ie: 1000000 == 1Mhz
     * - Users can specify default SPI speed by modifying @ref RF24_SPI_SPEED in @ref RF24_config.h
     *     - For Arduino, the default SPI speed will only be properly configured this way on devices supporting SPI TRANSACTIONS
     *     - Older/Unsupported Arduino devices will use a default clock divider & settings configuration
     *     - For Linux: The old way of setting SPI speeds using BCM2835 driver enums has been removed as of v1.3.7
     */
    RF24(rf24_gpio_pin_t _cepin, rf24_gpio_pin_t _cspin, uint32_t _spi_speed = RF24_SPI_SPEED);

    /**
     * A constructor for initializing the radio's hardware dynamically
     * @warning You MUST use begin(rf24_gpio_pin_t, rf24_gpio_pin_t) or begin(_SPI*, rf24_gpio_pin_t, rf24_gpio_pin_t) to pass both the
     * digital output pin numbers connected to the radio's CE and CSN pins.
     * @param _spi_speed The SPI speed in Hz ie: 1000000 == 1Mhz
     * - Users can specify default SPI speed by modifying @ref RF24_SPI_SPEED in @ref RF24_config.h
     *     - For Arduino, the default SPI speed will only be properly configured this way on devices supporting SPI TRANSACTIONS
     *     - Older/Unsupported Arduino devices will use a default clock divider & settings configuration
     *     - For Linux: The old way of setting SPI speeds using BCM2835 driver enums has been removed as of v1.3.7
     */
    RF24(uint32_t _spi_speed = RF24_SPI_SPEED);

#if defined(RF24_LINUX)
    virtual ~RF24() {};
#endif

    /**
     * Begin operation of the chip
     *
     * Call this in setup(), before calling any other methods.
     * @code
     * if (!radio.begin()) {
     *   Serial.println(F("radio hardware not responding!"));
     *   while (1) {} // hold program in infinite loop to prevent subsequent errors
     * }
     * @endcode
     * @return
     * - `true` if the radio was successfully initialized
     * - `false` if the MCU failed to communicate with the radio hardware
     */
    bool begin(void);

#if defined(RF24_SPI_PTR) || defined(DOXYGEN_FORCED)
    /**
     * Same as begin(), but allows specifying a non-default SPI bus to use.
     *
     * @note This function assumes the `SPI::begin()` method was called before to
     * calling this function.
     *
     * @warning This function is for the Arduino platforms only
     *
     * @param spiBus A pointer or reference to an instantiated SPI bus object.
     * The `_SPI` datatype is a "wrapped" definition that will represent
     * various SPI implementations based on the specified platform.
     * @see Review the [Arduino support page](arduino.md).
     *
     * @return same result as begin()
     */
    bool begin(_SPI* spiBus);

    /**
     * Same as begin(), but allows dynamically specifying a SPI bus, CE pin,
     * and CSN pin to use.
     *
     * @note This function assumes the `SPI::begin()` method was called before to
     * calling this function.
     *
     * @warning This function is for the Arduino platforms only
     *
     * @param spiBus A pointer or reference to an instantiated SPI bus object.
     * The `_SPI` datatype is a "wrapped" definition that will represent
     * various SPI implementations based on the specified platform.
     * @param _cepin The pin attached to Chip Enable on the RF module
     * @param _cspin The pin attached to Chip Select (often labeled CSN) on the radio module.
     * - For the Arduino Due board, the [Arduino Due extended SPI feature](https://www.arduino.cc/en/Reference/DueExtendedSPI)
     * is not supported. This means that the Due's pins 4, 10, or 52 are not mandated options (can use any digital output pin) for the radio's CSN pin.
     *
     * @see Review the [Arduino support page](arduino.md).
     *
     * @return same result as begin()
     */
    bool begin(_SPI* spiBus, rf24_gpio_pin_t _cepin, rf24_gpio_pin_t _cspin);
#endif // defined (RF24_SPI_PTR) || defined (DOXYGEN_FORCED)

    /**
     * Same as begin(), but allows dynamically specifying a CE pin
     * and CSN pin to use.
     * @param _cepin The pin attached to Chip Enable on the RF module
     * @param _cspin The pin attached to Chip Select (often labeled CSN) on the radio module.
     * - For the Arduino Due board, the [Arduino Due extended SPI feature](https://www.arduino.cc/en/Reference/DueExtendedSPI)
     * is not supported. This means that the Due's pins 4, 10, or 52 are not mandated options (can use any digital output pin) for the radio's CSN pin.
     * @return same result as begin()
     */
    bool begin(rf24_gpio_pin_t _cepin, rf24_gpio_pin_t _cspin);

    /**
     * Checks if the chip is connected to the SPI bus
     */
    bool isChipConnected();

    /**
     * Start listening on the pipes opened for reading.
     *
     * 1. Be sure to call openReadingPipe() first.
     * 2. Do not call write() while in this mode, without first calling stopListening().
     * 3. Call available() to check for incoming traffic, and read() to get it.
     *
     * Open reading pipe 1 using address `0xCCCECCCECC`
     * @code
     * byte address[] = {0xCC, 0xCE, 0xCC, 0xCE, 0xCC};
     * radio.openReadingPipe(1,address);
     * radio.startListening();
     * @endcode
     *
     * @note If there was a call to openReadingPipe() about pipe 0 prior to
     * calling this function, then this function will re-write the address
     * that was last set to reading pipe 0. This is because openWritingPipe()
     * will overwrite the address to reading pipe 0 for proper auto-ack
     * functionality.
     */
    void startListening(void);

    /**
     * Stop listening for incoming messages, and switch to transmit mode.
     *
     * Do this before calling write().
     * @code
     * radio.stopListening();
     * radio.write(&data, sizeof(data));
     * @endcode
     *
     * @note When the ACK payloads feature is enabled, the TX FIFO buffers are
     * flushed when calling this function. This is meant to discard any ACK
     * payloads that were not appended to acknowledgment packets.
     */
    void stopListening(void);

    /**
     * Check whether there are bytes available to be read
     * @code
     * if(radio.available()){
     *   radio.read(&data,sizeof(data));
     * }
     * @endcode
     *
     * @see available(uint8_t*)
     *
     * @return True if there is a payload available, false if none is
     *
     * @warning This function relies on the information about the pipe number
     * that received the next available payload. According to the datasheet,
     * the data about the pipe number that received the next available payload
     * is "unreliable" during a FALLING transition on the IRQ pin. This means
     * you should call whatHappened() before calling this function
     * during an ISR (Interrupt Service Routine). For example:
     * @code
     * void isrCallbackFunction() {
     *   bool tx_ds, tx_df, rx_dr;
     *   radio.whatHappened(tx_ds, tx_df, rx_dr); // resets the IRQ pin to HIGH
     *   radio.available();                       // returned data should now be reliable
     * }
     *
     * void setup() {
     *   pinMode(IRQ_PIN, INPUT);
     *   attachInterrupt(digitalPinToInterrupt(IRQ_PIN), isrCallbackFunction, FALLING);
     * }
     * @endcode
     */
    bool available(void);

    /**
     * Read payload data from the RX FIFO buffer(s).
     *
     * The length of data read is usually the next available payload's length
     * @see
     * - getPayloadSize()
     * - getDynamicPayloadSize()
     *
     * @note I specifically chose `void*` as a data type to make it easier
     * for beginners to use.  No casting needed.
     *
     * @param buf Pointer to a buffer where the data should be written
     * @param len Maximum number of bytes to read into the buffer. This
     * value should match the length of the object referenced using the
     * `buf` parameter. The absolute maximum number of bytes that can be read
     * in one call is 32 (for dynamic payload lengths) or whatever number was
     * previously passed to setPayloadSize() (for static payload lengths).
     * @remark
     * @parblock
     * Remember that each call to read() fetches data from the
     * RX FIFO beginning with the first byte from the first available
     * payload. A payload is not removed from the RX FIFO until it's
     * entire length (or more) is fetched using read().
     *
     * - If `len` parameter's value is less than the available payload's
     *   length, then the payload remains in the RX FIFO.
     * - If `len` parameter's value is greater than the first of multiple
     *   available payloads, then the data saved to the `buf`
     *   parameter's object will be supplemented with data from the next
     *   available payload.
     * - If `len` parameter's value is greater than the last available
     *   payload's length, then the last byte in the payload is used as
     *   padding for the data saved to the `buf` parameter's object.
     *   The nRF24L01 will repeatedly use the last byte from the last
     *   payload even when read() is called with an empty RX FIFO.
     * @endparblock
     * @note To use this function in the python wrapper, remember that
     * only the `len` parameter is required because this function (in the
     * python wrapper) returns the payload data as a buffer protocol object
     * (bytearray object).
     * @code{.py}
     * # let `radio` be the instantiated RF24 object
     * if radio.available():
     *     length = radio.getDynamicPayloadSize()  # or radio.getPayloadSize() for static payload sizes
     *     received_payload = radio.read(length)
     * @endcode
     *
     * @note This function no longer returns a boolean. Use available to
     * determine if packets are available. The `RX_DR` Interrupt flag is now
     * cleared with this function instead of when calling available().
     * @code
     * if(radio.available()) {
     *   radio.read(&data, sizeof(data));
     * }
     * @endcode
     */
    void read(void* buf, uint8_t len);

    /**
     * Be sure to call openWritingPipe() first to set the destination
     * of where to write to.
     *
     * This blocks until the message is successfully acknowledged by
     * the receiver or the timeout/retransmit maxima are reached.  In
     * the current configuration, the max delay here is 60-70ms.
     *
     * The maximum size of data written is the fixed payload size, see
     * getPayloadSize().  However, you can write less, and the remainder
     * will just be filled with zeroes.
     *
     * TX/RX/RT interrupt flags will be cleared every time write is called
     *
     * @param buf Pointer to the data to be sent
     * @param len Number of bytes to be sent
     *
     * @code
     * radio.stopListening();
     * radio.write(&data,sizeof(data));
     * @endcode
     *
     * @note The `len` parameter must be omitted when using the python
     * wrapper because the length of the payload is determined automatically.
     * To use this function in the python wrapper:
     * @code{.py}
     * # let `radio` be the instantiated RF24 object
     * buffer = b"Hello World"  # a `bytes` object
     * radio.write(buffer)
     * @endcode
     *
     * @return
     * - `true` if the payload was delivered successfully and an acknowledgement
     *   (ACK packet) was received. If auto-ack is disabled, then any attempt
     *   to transmit will also return true (even if the payload was not
     *   received).
     * - `false` if the payload was sent but was not acknowledged with an ACK
     *   packet. This condition can only be reported if the auto-ack feature
     *   is on.
     */
    bool write(const void* buf, uint8_t len);

    /**
     * New: Open a pipe for writing via byte array. Old addressing format retained
     * for compatibility.
     *
     * Only one writing pipe can be opened at once, but this function changes
     * the address that is used to transmit (ACK payloads/packets do not apply
     * here). Be sure to call stopListening() prior to calling this function.
     *
     * Addresses are assigned via a byte array, default is 5 byte address length
     *
     * @code
     * uint8_t addresses[][6] = {"1Node", "2Node"};
     * radio.openWritingPipe(addresses[0]);
     * @endcode
     * @code
     * uint8_t address[] = { 0xCC, 0xCE, 0xCC, 0xCE, 0xCC };
     * radio.openWritingPipe(address);
     * address[0] = 0x33;
     * radio.openReadingPipe(1, address);
     * @endcode
     *
     * @warning This function will overwrite the address set to reading pipe 0
     * as stipulated by the datasheet for proper auto-ack functionality in TX
     * mode. Use this function to ensure proper transmission acknowledgement
     * when the address set to reading pipe 0 (via openReadingPipe()) does not
     * match the address passed to this function. If the auto-ack feature is
     * disabled, then this function will still overwrite the address for
     * reading pipe 0 regardless.
     *
     * @see
     * - setAddressWidth()
     * - startListening()
     *
     * @param address The address to be used for outgoing transmissions (uses
     * pipe 0). Coordinate this address amongst other receiving nodes (the
     * pipe numbers don't need to match).
     *
     * @remark There is no address length parameter because this function will
     * always write the number of bytes that the radio addresses are configured
     * to use (set with setAddressWidth()).
     */

    void openWritingPipe(const uint8_t* address);

    /**
     * Open a pipe for reading
     *
     * Up to 6 pipes can be open for reading at once.  Open all the required
     * reading pipes, and then call startListening().
     *
     * @see
     * - openWritingPipe()
     * - setAddressWidth()
     *
     * @note Pipes 0 and 1 will store a full 5-byte address. Pipes 2-5 will technically
     * only store a single byte, borrowing up to 4 additional bytes from pipe 1 per the
     * assigned address width.
     * Pipes 1-5 should share the same address, except the first byte.
     * Only the first byte in the array should be unique, e.g.
     * @code
     * uint8_t addresses[][6] = {"Prime", "2Node", "3xxxx", "4xxxx"};
     * openReadingPipe(0, addresses[0]); // address used is "Prime"
     * openReadingPipe(1, addresses[1]); // address used is "2Node"
     * openReadingPipe(2, addresses[2]); // address used is "3Node"
     * openReadingPipe(3, addresses[3]); // address used is "4Node"
     * @endcode
     *
     * @warning
     * @parblock
     * If the reading pipe 0 is opened by this function, the address
     * passed to this function (for pipe 0) will be restored at every call to
     * startListening().
     *
     * Read
     * http://maniacalbits.blogspot.com/2013/04/rf24-addressing-nrf24l01-radios-require.html
     * to understand how to avoid using malformed addresses. This address
     * restoration is implemented because of the underlying necessary
     * functionality of openWritingPipe().
     * @endparblock
     *
     * @param number Which pipe to open. Only pipe numbers 0-5 are available,
     * an address assigned to any pipe number not in that range will be ignored.
     * @param address The 24, 32 or 40 bit address of the pipe to open.
     *
     * There is no address length parameter because this function will
     * always write the number of bytes (for pipes 0 and 1) that the radio
     * addresses are configured to use (set with setAddressWidth()).
     */
    void openReadingPipe(uint8_t number, const uint8_t* address);

    /**@}*/
    /**
     * @name Advanced Operation
     *
     * Methods you can use to drive the chip in more advanced ways
     */
    /**@{*/

    /**
     * Print a giant block of debugging information to stdout
     *
     * @warning Does nothing if stdout is not defined.  See fdevopen in stdio.h
     * The printf.h file is included with the library for Arduino.
     * @code
     * #include <printf.h>
     * setup(){
     *   Serial.begin(115200);
     *   printf_begin();
     *   ...
     * }
     * @endcode
     */
    void printDetails(void);

    /**
     * Print a giant block of debugging information to stdout. This function
     * differs from printDetails() because it makes the information more
     * understandable without having to look up the datasheet or convert
     * hexadecimal to binary. Only use this function if your application can
     * spare extra bytes of memory.
     *
     * @warning Does nothing if stdout is not defined.  See fdevopen in stdio.h
     * The printf.h file is included with the library for Arduino.
     * @code
     * #include <printf.h>
     * setup(){
     *   Serial.begin(115200);
     *   printf_begin();
     *   // ...
     * }
     * @endcode
     *
     * @note If the automatic acknowledgements feature is configured differently
     * for each pipe, then a binary representation is used in which bits 0-5
     * represent pipes 0-5 respectively. A `0` means the feature is disabled, and
     * a `1` means the feature is enabled.
     */
    void printPrettyDetails(void);

    /**
     * Put a giant block of debugging information in a char array. This function
     * differs from printPrettyDetails() because it uses `sprintf()` and does not use
     * a predefined output stream (like `Serial` or stdout). Only use this function if
     * your application can spare extra bytes of memory. This can also be used for boards that
     * do not support `printf()` (which is required for printDetails() and printPrettyDetails()).
     *
     * @remark
     * The C standard function [sprintf()](http://www.cplusplus.com/reference/cstdio/sprintf)
     * formats a C-string in the exact same way as `printf()` but outputs (by reference)
     * into a char array. The formatted string literal for sprintf() is stored
     * in nonvolatile program memory.
     *
     * @warning Use a buffer of sufficient size for the `debugging_information`. Start
     * with a char array that has at least 870 elements. There is no overflow protection when using
     * sprintf(), so the output buffer must be sized correctly or the resulting behavior will
     * be undefined.
     * @code
     * char buffer[870] = {'\0'};
     * uint16_t used_chars = radio.sprintfPrettyDetails(buffer);
     * Serial.println(buffer);
     * Serial.print(F("strlen = "));
     * Serial.println(used_chars + 1); // +1 for c-strings' null terminating byte
     * @endcode
     *
     * @param debugging_information The c-string buffer that the debugging
     * information is stored to. This must be allocated to a minimum of 870 bytes of memory.
     * @returns The number of characters altered in the given buffer. Remember that,
     * like `sprintf()`, this returned number does not include the null terminating byte.
     *
     * This function is available in the python wrapper, but it accepts no parameters and
     * returns a string. It does not return the number of characters in the string.
     * @code{.py}
     * debug_info = radio.sprintfPrettyDetails()
     * print(debug_info)
     * print("str_len =", len(debug_info))
     * @endcode
     *
     * @note If the automatic acknowledgements feature is configured differently
     * for each pipe, then a binary representation is used in which bits 0-5
     * represent pipes 0-5 respectively. A `0` means the feature is disabled, and
     * a `1` means the feature is enabled.
     */
    uint16_t sprintfPrettyDetails(char* debugging_information);

    /**
     * Encode radio debugging information into an array of uint8_t. This function
     * differs from other debug output methods because the debug information can
     * be decoded by an external program.
     *
     * This function is not available in the python wrapper because it is intended for
     * use on processors with very limited available resources.
     *
     * @remark
     * This function uses much less ram than other `*print*Details()` methods.
     *
     * @code
     * uint8_t encoded_details[43] = {0};
     * radio.encodeRadioDetails(encoded_details);
     * @endcode
     *
     * @param encoded_status The uint8_t array that RF24 radio details are
     * encoded into. This array must be at least 43 bytes in length; any less would surely
     * cause undefined behavior.
     *
     * Registers names and/or data corresponding to the index of the `encoded_details` array:
     * | index | register/data |
     * |------:|:--------------|
     * | 0 |     NRF_CONFIG |
     * | 1 |     EN_AA |
     * | 2 |     EN_RXADDR |
     * | 3 |     SETUP_AW |
     * | 4 |     SETUP_RETR |
     * | 5 |     RF_CH |
     * | 6 |     RF_SETUP |
     * | 7 |     NRF_STATUS |
     * | 8 |     OBSERVE_TX |
     * | 9 |     CD (aka RPD) |
     * | 10-14 | RX_ADDR_P0 |
     * | 15-19 | RX_ADDR_P1 |
     * | 20 |    RX_ADDR_P2 |
     * | 21 |    RX_ADDR_P3 |
     * | 22 |    RX_ADDR_P4 |
     * | 23 |    RX_ADDR_P5 |
     * | 24-28 | TX_ADDR |
     * | 29 |    RX_PW_P0 |
     * | 30 |    RX_PW_P1 |
     * | 31 |    RX_PW_P2 |
     * | 32 |    RX_PW_P3 |
     * | 33 |    RX_PW_P4 |
     * | 34 |    RX_PW_P5 |
     * | 35 |    FIFO_STATUS |
     * | 36 |    DYNPD |
     * | 37 |    FEATURE |
     * | 38-39 | ce_pin |
     * | 40-41 | csn_pin |
     * | 42 |    SPI speed (in MHz) or'd with (isPlusVariant << 4) |
     */
    void encodeRadioDetails(uint8_t* encoded_status);

    /**
     * Test whether there are bytes available to be read from the
     * FIFO buffers.
     *
     * @note This function is named `available_pipe()` in the python wrapper.
     * @parblock
     * Additionally, the `available_pipe()` function (which
     * takes no arguments) returns a 2 item tuple containing (ordered by
     * tuple's indices):
     * - A boolean describing if there is a payload available to read from
     *   the RX FIFO buffers.
     * - The pipe number that received the next available payload in the RX
     *   FIFO buffers. If the item at the tuple's index 0 is `False`, then
     *   this pipe number is invalid.
     *
     * To use this function in python:
     * @code{.py}
     * # let `radio` be the instantiated RF24 object
     * has_payload, pipe_number = radio.available_pipe()  # expand the tuple to 2 variables
     * if has_payload:
     *     print("Received a payload with pipe", pipe_number)
     * @endcode
     * @endparblock
     *
     * @param[out] pipe_num Which pipe has the payload available
     * @code
     * uint8_t pipeNum;
     * if(radio.available(&pipeNum)){
     *   radio.read(&data, sizeof(data));
     *   Serial.print("Received data on pipe ");
     *   Serial.println(pipeNum);
     * }
     * @endcode
     *
     * @warning According to the datasheet, the data saved to `pipe_num` is
     * "unreliable" during a FALLING transition on the IRQ pin. This means you
     * should call whatHappened() before calling this function during
     * an ISR (Interrupt Service Routine). For example:
     * @code
     * void isrCallbackFunction() {
     *   bool tx_ds, tx_df, rx_dr;
     *   radio.whatHappened(tx_ds, tx_df, rx_dr); // resets the IRQ pin to HIGH
     *   uint8_t pipe;                            // initialize pipe data
     *   radio.available(&pipe);                  // pipe data should now be reliable
     * }
     *
     * void setup() {
     *   pinMode(IRQ_PIN, INPUT);
     *   attachInterrupt(digitalPinToInterrupt(IRQ_PIN), isrCallbackFunction, FALLING);
     * }
     * @endcode
     *
     * @return
     * - `true` if there is a payload available in the top (first out)
     *   level RX FIFO.
     * - `false` if there is nothing available in the RX FIFO because it is
     *   empty.
     */
    bool available(uint8_t* pipe_num);

    /**
     * Use this function to check if the radio's RX FIFO levels are all
     * occupied. This can be used to prevent data loss because any incoming
     * transmissions are rejected if there is no unoccupied levels in the RX
     * FIFO to store the incoming payload. Remember that each level can hold
     * up to a maximum of 32 bytes.
     * @return
     * - `true` if all three 3 levels of the RX FIFO buffers are occupied.
     * - `false` if there is one or more levels available in the RX FIFO
     *   buffers. Remember that this does not always mean that the RX FIFO
     *   buffers are empty; use available() to see if the RX FIFO buffers are
     *   empty or not.
     */
    bool rxFifoFull();

    /**
     * @param about_tx `true` focuses on the TX FIFO, `false` focuses on the RX FIFO
     * @return
     * - @ref RF24_FIFO_OCCUPIED (`0`) if the specified FIFO is neither full nor empty.
     * - @ref RF24_FIFO_EMPTY (`1`) if the specified FIFO is empty.
     * - @ref RF24_FIFO_FULL (`2`) if the specified FIFO is full.
     * - @ref RF24_FIFO_INVALID (`3`) if the data fetched over SPI was malformed.
     */
    rf24_fifo_state_e isFifo(bool about_tx);

    /**
     * @deprecated Use RF24::isFifo(bool about_tx) instead.
     * @param about_tx `true` focuses on the TX FIFO, `false` focuses on the RX FIFO
     * @param check_empty
     * - `true` checks if the specified FIFO is empty
     * - `false` checks is the specified FIFO is full.
     * @return A boolean answer to the question "is the [TX/RX] FIFO [empty/full]?"
     */
    bool isFifo(bool about_tx, bool check_empty);

    /**
     * Enter low-power mode
     *
     * To return to normal power mode, call powerUp().
     *
     * @note After calling startListening(), a basic radio will consume about 13.5mA
     * at max PA level.
     * During active transmission, the radio will consume about 11.5mA, but this will
     * be reduced to 26uA (.026mA) between sending.
     * In full powerDown mode, the radio will consume approximately 900nA (.0009mA)
     *
     * @code
     * radio.powerDown();
     * avr_enter_sleep_mode(); // Custom function to sleep the device
     * radio.powerUp();
     * @endcode
     */
    void powerDown(void);

    /**
     * Leave low-power mode - required for normal radio operation after calling powerDown()
     *
     * To return to low power mode, call powerDown().
     * @note This will take up to 5ms for maximum compatibility
     */
    void powerUp(void);

    /**
     * Write for single NOACK writes. Optionally disable
     * acknowledgements/auto-retries for a single payload using the
     * multicast parameter set to true.
     *
     * Can be used with enableAckPayload() to request a response
     * @see
     * - setAutoAck()
     * - write()
     *
     * @param buf Pointer to the data to be sent
     * @param len Number of bytes to be sent
     * @param multicast Request ACK response (false), or no ACK response
     * (true). Be sure to have called enableDynamicAck() at least once before
     * setting this parameter.
     * @return
     * - `true` if the payload was delivered successfully and an acknowledgement
     *   (ACK packet) was received. If auto-ack is disabled, then any attempt
     *   to transmit will also return true (even if the payload was not
     *   received).
     * - `false` if the payload was sent but was not acknowledged with an ACK
     *   packet. This condition can only be reported if the auto-ack feature
     *   is on.
     *
     * @note The `len` parameter must be omitted when using the python
     * wrapper because the length of the payload is determined automatically.
     * To use this function in the python wrapper:
     * @code{.py}
     * # let `radio` be the instantiated RF24 object
     * buffer = b"Hello World"  # a `bytes` object
     * radio.write(buffer, False)  # False = the multicast parameter
     * @endcode
     */
    bool write(const void* buf, uint8_t len, const bool multicast);

    /**
     * This will not block until the 3 FIFO buffers are filled with data.
     * Once the FIFOs are full, writeFast() will simply wait for a buffer to
     * become available or a transmission failure (returning `true` or `false`
     * respectively).
     *
     * @warning
     * @parblock
     * It is important to never keep the nRF24L01 in TX mode and FIFO full for more than 4ms at a time. If the auto
     * retransmit is enabled, the nRF24L01 is never in TX mode long enough to disobey this rule. Allow the FIFO
     * to clear by issuing txStandBy() or ensure appropriate time between transmissions.
     *
     * Use txStandBy() when this function returns `false`.
     *
     * Example (Partial blocking):
     * @code
     * radio.writeFast(&buf,32);  // Writes 1 payload to the buffers
     * txStandBy();     		   // Returns 0 if failed. 1 if success. Blocks only until MAX_RT timeout or success. Data flushed on fail.
     *
     * radio.writeFast(&buf,32);  // Writes 1 payload to the buffers
     * txStandBy(1000);		   // Using extended timeouts, returns 1 if success. Retries failed payloads for 1 seconds before returning 0.
     * @endcode
     * @endparblock
     *
     * @see
     * - setAutoAck()
     * - txStandBy()
     * - write()
     * - writeBlocking()
     *
     * @param buf Pointer to the data to be sent
     * @param len Number of bytes to be sent
     * @return
     * - `true` if the payload passed to `buf` was loaded in the TX FIFO.
     * - `false` if the payload passed to `buf` was not loaded in the TX FIFO
     *   because a previous payload already in the TX FIFO failed to
     *   transmit. This condition can only be reported if the auto-ack feature
     *   is on.
     *
     * @note The `len` parameter must be omitted when using the python
     * wrapper because the length of the payload is determined automatically.
     * To use this function in the python wrapper:
     * @code{.py}
     * # let `radio` be the instantiated RF24 object
     * buffer = b"Hello World"  # a `bytes` object
     * radio.writeFast(buffer)
     * @endcode
     */
    bool writeFast(const void* buf, uint8_t len);

    /**
     * Similar to writeFast(const void*, uint8_t) but allows for single NOACK writes.
     * Optionally disable acknowledgements/auto-retries for a single payload using the
     * multicast parameter set to `true`.
     *
     * @warning If the auto-ack feature is enabled, then it is strongly encouraged to call
     * txStandBy() when this function returns `false`.
     *
     * @see
     * - setAutoAck()
     * - txStandBy()
     * - write()
     * - writeBlocking()
     *
     * @param buf Pointer to the data to be sent
     * @param len Number of bytes to be sent
     * @param multicast Request ACK response (false), or no ACK response
     * (true). Be sure to have called enableDynamicAck() at least once before
     * setting this parameter.
     * @return
     * - `true` if the payload passed to `buf` was loaded in the TX FIFO.
     * - `false` if the payload passed to `buf` was not loaded in the TX FIFO
     *   because a previous payload already in the TX FIFO failed to
     *   transmit. This condition can only be reported if the auto-ack feature
     *   is on (and the multicast parameter is set to false).
     *
     * @note The `len` parameter must be omitted when using the python
     * wrapper because the length of the payload is determined automatically.
     * To use this function in the python wrapper:
     * @code{.py}
     * # let `radio` be the instantiated RF24 object
     * buffer = b"Hello World"  # a `bytes` object
     * radio.writeFast(buffer, False)  # False = the multicast parameter
     * @endcode
     */
    bool writeFast(const void* buf, uint8_t len, const bool multicast);

    /**
     * This function extends the auto-retry mechanism to any specified duration.
     * It will not block until the 3 FIFO buffers are filled with data.
     * If so the library will auto retry until a new payload is written
     * or the user specified timeout period is reached.
     * @warning It is important to never keep the nRF24L01 in TX mode and FIFO full for more than 4ms at a time. If the auto
     * retransmit is enabled, the nRF24L01 is never in TX mode long enough to disobey this rule. Allow the FIFO
     * to clear by issuing txStandBy() or ensure appropriate time between transmissions.
     *
     * Example (Full blocking):
     * @code
     * radio.writeBlocking(&buf, sizeof(buf), 1000); // Wait up to 1 second to write 1 payload to the buffers
     * radio.txStandBy(1000);                        // Wait up to 1 second for the payload to send. Return 1 if ok, 0 if failed.
     *                                               // Blocks only until user timeout or success. Data flushed on fail.
     * @endcode
     * @note If used from within an interrupt, the interrupt should be disabled until completion, and sei(); called to enable millis().
     * @see
     * - txStandBy()
     * - write()
     * - writeFast()
     *
     * @param buf Pointer to the data to be sent
     * @param len Number of bytes to be sent
     * @param timeout User defined timeout in milliseconds.
     *
     * @note The `len` parameter must be omitted when using the python
     * wrapper because the length of the payload is determined automatically.
     * To use this function in the python wrapper:
     * @code{.py}
     * # let `radio` be the instantiated RF24 object
     * buffer = b"Hello World"  # a `bytes` object
     * radio.writeBlocking(buffer, 1000)  # 1000 means wait at most 1 second
     * @endcode
     *
     * @return
     * - `true` if the payload passed to `buf` was loaded in the TX FIFO.
     * - `false` if the payload passed to `buf` was not loaded in the TX FIFO
     *   because a previous payload already in the TX FIFO failed to
     *   transmit. This condition can only be reported if the auto-ack feature
     *   is on.
     */
    bool writeBlocking(const void* buf, uint8_t len, uint32_t timeout);

    /**
     * This function should be called as soon as transmission is finished to
     * drop the radio back to STANDBY-I mode. If not issued, the radio will
     * remain in STANDBY-II mode which, per the data sheet, is not a recommended
     * operating mode.
     *
     * @note When transmitting data in rapid succession, it is still recommended by
     * the manufacturer to drop the radio out of TX or STANDBY-II mode if there is
     * time enough between sends for the FIFOs to empty. This is not required if auto-ack
     * is enabled.
     *
     * Relies on built-in auto retry functionality.
     *
     * Example (Partial blocking):
     * @code
     * radio.writeFast(&buf, 32);
     * radio.writeFast(&buf, 32);
     * radio.writeFast(&buf, 32);   //Fills the FIFO buffers up
     * bool ok = radio.txStandBy(); //Returns 0 if failed. 1 if success.
     *                              //Blocks only until MAX_RT timeout or success. Data flushed on fail.
     * @endcode
     * @see txStandBy(uint32_t timeout, bool startTx)
     * @return
     * - `true` if all payloads in the TX FIFO were delivered successfully and
     *   an acknowledgement (ACK packet) was received for each. If auto-ack is
     *   disabled, then any attempt to transmit will also return true (even if
     *   the payload was not received).
     * - `false` if a payload was sent but was not acknowledged with an ACK
     *   packet. This condition can only be reported if the auto-ack feature
     *   is on.
     */
    bool txStandBy();

    /**
     * This function allows extended blocking and auto-retries per a user defined timeout
     *
     * Fully Blocking Example:
     * @code
     * radio.writeFast(&buf, 32);
     * radio.writeFast(&buf, 32);
     * radio.writeFast(&buf, 32);       //Fills the FIFO buffers up
     * bool ok = radio.txStandBy(1000); //Returns 0 if failed after 1 second of retries. 1 if success.
     *                                  //Blocks only until user defined timeout or success. Data flushed on fail.
     * @endcode
     * @note If used from within an interrupt, the interrupt should be disabled until completion, and sei(); called to enable millis().
     * @param timeout Number of milliseconds to retry failed payloads
     * @param startTx If this is set to `true`, then this function puts the nRF24L01
     * in TX Mode. `false` leaves the primary mode (TX or RX) as it is, which can
     * prevent the mandatory wait time to change modes.
     * @return
     * - `true` if all payloads in the TX FIFO were delivered successfully and
     *   an acknowledgement (ACK packet) was received for each. If auto-ack is
     *   disabled, then any attempt to transmit will also return true (even if
     *   the payload was not received).
     * - `false` if a payload was sent but was not acknowledged with an ACK
     *   packet. This condition can only be reported if the auto-ack feature
     *   is on.
     */
    bool txStandBy(uint32_t timeout, bool startTx = 0);

    /**
     * Write an acknowledgement (ACK) payload for the specified pipe
     *
     * The next time a message is received on a specified `pipe`, the data in
     * `buf` will be sent back in the ACK payload.
     *
     * @see
     * - enableAckPayload()
     * - enableDynamicPayloads()
     *
     * @note ACK payloads are handled automatically by the radio chip when a
     * regular payload is received. It is important to discard regular payloads
     * in the TX FIFO (using flush_tx()) before loading the first ACK payload
     * into the TX FIFO. This function can be called before and after calling
     * startListening().
     *
     * @warning Only three of these can be pending at any time as there are
     * only 3 FIFO buffers. Dynamic payloads must be enabled.
     *
     * @note ACK payloads are dynamic payloads. Calling enableAckPayload()
     * will automatically enable dynamic payloads on pipe 0 (required for TX
     * mode when expecting ACK payloads) & pipe 1. To use ACK payloads on any other
     * pipe in RX mode, call enableDynamicPayloads().
     *
     * @param pipe Which pipe# (typically 1-5) will get this response.
     * @param buf Pointer to data that is sent
     * @param len Length of the data to send, up to 32 bytes max.  Not affected
     * by the static payload size set by setPayloadSize().
     *
     * @note The `len` parameter must be omitted when using the python
     * wrapper because the length of the payload is determined automatically.
     * To use this function in the python wrapper:
     * @code{.py}
     * # let `radio` be the instantiated RF24 object
     * buffer = b"Hello World"  # a `bytes` object
     * radio.writeAckPayload(1, buffer)  # load an ACK payload for response on pipe 1
     * @endcode
     *
     * @return
     * - `true` if the payload was loaded into the TX FIFO.
     * - `false` if the payload wasn't loaded into the TX FIFO because it is
     *   already full or the ACK payload feature is not enabled using
     *   enableAckPayload().
     */
    bool writeAckPayload(uint8_t pipe, const void* buf, uint8_t len);

    /**
     * Call this when you get an Interrupt Request (IRQ) to find out why
     *
     * This function describes what event triggered the IRQ pin to go active
     * LOW and clears the status of all events.
     *
     * @see maskIRQ()
     *
     * @param[out] tx_ok The transmission attempt completed (TX_DS). This does
     * not imply that the transmitted data was received by another radio, rather
     * this only reports if the attempt to send was completed. This will
     * always be `true` when the auto-ack feature is disabled.
     * @param[out] tx_fail The transmission failed to be acknowledged, meaning
     * too many retries (MAX_RT) were made while expecting an ACK packet. This
     * event is only triggered when auto-ack feature is enabled.
     * @param[out] rx_ready There is a newly received payload (RX_DR) saved to
     * RX FIFO buffers. Remember that the RX FIFO can only hold up to 3
     * payloads. Once the RX FIFO is full, all further received transmissions
     * are rejected until there is space to save new data in the RX FIFO
     * buffers.
     *
     * @note This function expects no parameters in the python wrapper.
     * Instead, this function returns a 3 item tuple describing the IRQ
     * events' status. To use this function in the python wrapper:
     * @code{.py}
     * # let`radio` be the instantiated RF24 object
     * tx_ds, tx_df, rx_dr = radio.whatHappened()  # get IRQ status flags
     * print("tx_ds: {}, tx_df: {}, rx_dr: {}".format(tx_ds, tx_df, rx_dr))
     * @endcode
     */
    void whatHappened(bool& tx_ok, bool& tx_fail, bool& rx_ready);

    /**
     * Non-blocking write to the open writing pipe used for buffered writes
     *
     * @note Optimization: This function now leaves the CE pin high, so the radio
     * will remain in TX or STANDBY-II Mode until a txStandBy() command is issued. Can be used as an alternative to startWrite()
     * if writing multiple payloads at once.
     * @warning It is important to never keep the nRF24L01 in TX mode with FIFO full for more than 4ms at a time. If the auto
     * retransmit/autoAck is enabled, the nRF24L01 is never in TX mode long enough to disobey this rule. Allow the FIFO
     * to clear by issuing txStandBy() or ensure appropriate time between transmissions.
     *
     * @see
     * - write()
     * - writeFast()
     * - startWrite()
     * - writeBlocking()
     * - setAutoAck() (for single noAck writes)
     *
     * @param buf Pointer to the data to be sent
     * @param len Number of bytes to be sent
     * @param multicast Request ACK response (false), or no ACK response
     * (true). Be sure to have called enableDynamicAck() at least once before
     * setting this parameter.
     * @param startTx If this is set to `true`, then this function sets the
     * nRF24L01's CE pin to active (enabling TX transmissions). `false` has no
     * effect on the nRF24L01's CE pin and simply loads the payload into the
     * TX FIFO.
     *
     * @note The `len` parameter must be omitted when using the python
     * wrapper because the length of the payload is determined automatically.
     * To use this function in the python wrapper:
     * @code{.py}
     * # let `radio` be the instantiated RF24 object
     * buffer = b"Hello World"  # a `bytes` object
     * radio.startFastWrite(buffer, False, True)  # 3rd parameter is optional
     * #     False means expecting ACK response (multicast parameter)
     * #     True means initiate transmission (startTx parameter)
     * @endcode
     */
    void startFastWrite(const void* buf, uint8_t len, const bool multicast, bool startTx = 1);

    /**
     * Non-blocking write to the open writing pipe
     *
     * Just like write(), but it returns immediately. To find out what happened
     * to the send, catch the IRQ and then call whatHappened().
     *
     * @see
     * - write()
     * - writeFast()
     * - startFastWrite()
     * - whatHappened()
     * - setAutoAck() (for single noAck writes)
     *
     * @param buf Pointer to the data to be sent
     * @param len Number of bytes to be sent
     * @param multicast Request ACK response (false), or no ACK response
     * (true). Be sure to have called enableDynamicAck() at least once before
     * setting this parameter.
     *
     * @return
     * - `true` if payload was written to the TX FIFO buffers and the
     *   transmission was started.
     * - `false` if the TX FIFO is full and the payload could not be written. In
     *   this condition, the transmission process is restarted.
     * @note The `len` parameter must be omitted when using the python
     * wrapper because the length of the payload is determined automatically.
     * To use this function in the python wrapper:
     * @code{.py}
     * # let `radio` be the instantiated RF24 object
     * buffer = b"Hello World"  # a `bytes` object
     * radio.startWrite(buffer, False)  # False = the multicast parameter
     * @endcode
     */
    bool startWrite(const void* buf, uint8_t len, const bool multicast);

    /**
     * The function will instruct the radio to re-use the payload in the
     * top level (first out) of the TX FIFO buffers. This is used internally
     * by writeBlocking() to initiate retries when a TX failure
     * occurs. Retries are automatically initiated except with the standard
     * write(). This way, data is not flushed from the buffer until calling
     * flush_tx(). If the TX FIFO has only the one payload (in the top level),
     * the re-used payload can be overwritten by using write(), writeFast(),
     * writeBlocking(), startWrite(), or startFastWrite(). If the TX FIFO has
     * other payloads enqueued, then the aforementioned functions will attempt
     * to enqueue the a new payload in the TX FIFO (does not overwrite the top
     * level of the TX FIFO). Currently, stopListening() also calls flush_tx()
     * when ACK payloads are enabled (via enableAckPayload()).
     *
     * Upon exiting, this function will set the CE pin HIGH to initiate the
     * re-transmission process. If only 1 re-transmission is desired, then the
     * CE pin should be set to LOW after the mandatory minumum pulse duration
     * of 10 microseconds.
     *
     * @remark This function only applies when taking advantage of the
     * auto-retry feature. See setAutoAck() and setRetries() to configure the
     * auto-retry feature.
     *
     * @note This is to be used AFTER auto-retry fails if wanting to resend
     * using the built-in payload reuse feature. After issuing reUseTX(), it
     * will keep resending the same payload until a transmission failure
     * occurs or the CE pin is set to LOW (whichever comes first). In the
     * event of a re-transmission failure, simply call this function again to
     * resume re-transmission of the same payload.
     */
    void reUseTX();

    /**
     * Empty all 3 of the TX (transmit) FIFO buffers. This is automatically
     * called by stopListening() if ACK payloads are enabled. However,
     * startListening() does not call this function.
     *
     * @return Current value of status register
     */
    uint8_t flush_tx(void);

    /**
     * Empty all 3 of the RX (receive) FIFO buffers.
     *
     * @return Current value of status register
     */
    uint8_t flush_rx(void);

    /**
     * Test whether there was a carrier on the line for the
     * previous listening period.
     *
     * Useful to check for interference on the current channel.
     *
     * @return true if was carrier, false if not
     */
    bool testCarrier(void);

    /**
     * Test whether a signal (carrier or otherwise) greater than
     * or equal to -64dBm is present on the channel. Valid only
     * on nRF24L01P (+) hardware. On nRF24L01, use testCarrier().
     *
     * Useful to check for interference on the current channel and
     * channel hopping strategies.
     *
     * @code
     * bool goodSignal = radio.testRPD();
     * if(radio.available()){
     *    Serial.println(goodSignal ? "Strong signal > -64dBm" : "Weak signal < -64dBm" );
     *    radio.read(&payload,sizeof(payload));
     * }
     * @endcode
     * @return true if a signal greater than or equal to -64dBm was detected,
     * false if not.
     */
    bool testRPD(void);

    /**
     * Test whether this is a real radio, or a mock shim for
     * debugging.  Setting either pin to 0xff is the way to
     * indicate that this is not a real radio.
     *
     * @return true if this is a legitimate radio
     */
    bool isValid();

    /**
     * Close a pipe after it has been previously opened.
     * Can be safely called without having previously opened a pipe.
     * @param pipe Which pipe number to close, any integer not in range [0, 5]
     * is ignored.
     */
    void closeReadingPipe(uint8_t pipe);

#if defined(FAILURE_HANDLING)
    /**
     *
     * If a failure has been detected, it usually indicates a hardware issue. By default the library
     * will cease operation when a failure is detected.
     * This should allow advanced users to detect and resolve intermittent hardware issues.
     *
     * In most cases, the radio must be re-enabled via radio.begin(); and the appropriate settings
     * applied after a failure occurs, if wanting to re-enable the device immediately.
     *
     * The three main failure modes of the radio include:
     *
     * 1. Writing to radio: Radio unresponsive
     *     - Fixed internally by adding a timeout to the internal write functions in RF24 (failure handling)
     * 2. Reading from radio: Available returns true always
     *     - Fixed by adding a timeout to available functions by the user. This is implemented internally in  RF24Network.
     * 3. Radio configuration settings are lost
     *     - Fixed by monitoring a value that is different from the default, and re-configuring the radio if this setting reverts to the default.
     *
     * See the included example, GettingStarted_HandlingFailures
     *
     * @code
     * if(radio.failureDetected) {
     *   radio.begin();                          // Attempt to re-configure the radio with defaults
     *   radio.failureDetected = 0;              // Reset the detection value
     *   radio.openWritingPipe(addresses[1]);    // Re-configure pipe addresses
     *   radio.openReadingPipe(1, addresses[0]);
     *   report_failure();                       // Blink LEDs, send a message, etc. to indicate failure
     * }
     * @endcode
     */
    bool failureDetected;
#endif // defined (FAILURE_HANDLING)

    /**@}*/
    /**
     * @name Optional Configurators
     *
     *  Methods you can use to get or set the configuration of the chip.
     *  None are required.  Calling begin() sets up a reasonable set of
     *  defaults.
     */
    /**@{*/

    /**
     * Set the address width from 3 to 5 bytes (24, 32 or 40 bit)
     *
     * @param a_width The address width (in bytes) to use; this can be 3, 4 or
     * 5.
     */
    void setAddressWidth(uint8_t a_width);

    /**
     * Set the number of retry attempts and delay between retry attempts when
     * transmitting a payload. The radio is waiting for an acknowledgement
     * (ACK) packet during the delay between retry attempts.
     *
     * @param delay How long to wait between each retry, in multiples of
     * 250 us. The minimum of 0 means 250 us, and the maximum of 15 means
     * 4000 us. The default value of 5 means 1500us (5 * 250 + 250).
     * @param count How many retries before giving up. The default/maximum is 15. Use
     * 0 to disable the auto-retry feature all together.
     *
     * @note Disable the auto-retry feature on a transmitter still uses the
     * auto-ack feature (if enabled), except it will not retry to transmit if
     * the payload was not acknowledged on the first attempt.
     */
    void setRetries(uint8_t delay, uint8_t count);

    /**
     * Set RF communication channel. The frequency used by a channel is
     * calculated as:
     * @verbatim 2400 MHz + <channel number> @endverbatim
     * Meaning the default channel of 76 uses the approximate frequency of
     * 2476 MHz.
     *
     * @note In the python wrapper, this function is the setter of the
     * `channel` attribute.To use this function in the python wrapper:
     * @code{.py}
     * # let `radio` be the instantiated RF24 object
     * radio.channel = 2  # set the channel to 2 (2402 MHz)
     * @endcode
     *
     * @param channel Which RF channel to communicate on, 0-125
     */
    void setChannel(uint8_t channel);

    /**
     * Get RF communication channel
     *
     * @note In the python wrapper, this function is the getter of the
     * `channel` attribute.To use this function in the python wrapper:
     * @code{.py}
     * # let `radio` be the instantiated RF24 object
     * chn = radio.channel  # get the channel
     * @endcode
     *
     * @return The currently configured RF Channel
     */
    uint8_t getChannel(void);

    /**
     * Set Static Payload Size
     *
     * This implementation uses a pre-established fixed payload size for all
     * transmissions.  If this method is never called, the driver will always
     * transmit the maximum payload size (32 bytes), no matter how much
     * was sent to write().
     *
     * @note In the python wrapper, this function is the setter of the
     * `payloadSize` attribute.To use this function in the python wrapper:
     * @code{.py}
     * # let `radio` be the instantiated RF24 object
     * radio.payloadSize = 16  # set the static payload size to 16 bytes
     * @endcode
     *
     * @param size The number of bytes in the payload
     */
    void setPayloadSize(uint8_t size);

    /**
     * Get Static Payload Size
     *
     * @note In the python wrapper, this function is the getter of the
     * `payloadSize` attribute.To use this function in the python wrapper:
     * @code{.py}
     * # let `radio` be the instantiated RF24 object
     * pl_size = radio.payloadSize  # get the static payload size
     * @endcode
     *
     * @see setPayloadSize()
     *
     * @return The number of bytes in the payload
     */
    uint8_t getPayloadSize(void);

    /**
     * Get Dynamic Payload Size
     *
     * For dynamic payloads, this pulls the size of the payload off
     * the chip
     *
     * @note Corrupt packets are now detected and flushed per the
     * manufacturer.
     * @code
     * if(radio.available()){
     *   if(radio.getDynamicPayloadSize() < 1){
     *     // Corrupt payload has been flushed
     *     return;
     *   }
     *   radio.read(&data,sizeof(data));
     * }
     * @endcode
     *
     * @return Payload length of last-received dynamic payload
     */
    uint8_t getDynamicPayloadSize(void);

    /**
     * Enable custom payloads in the acknowledge packets
     *
     * ACK payloads are a handy way to return data back to senders without
     * manually changing the radio modes on both units.
     *
     * @remarks The ACK payload feature requires the auto-ack feature to be
     * enabled for any pipe using ACK payloads. This function does not
     * automatically enable the auto-ack feature on pipe 0 since the auto-ack
     * feature is enabled for all pipes by default.
     *
     * @see setAutoAck()
     *
     * @note ACK payloads are dynamic payloads. This function automatically
     * enables dynamic payloads on pipes 0 & 1 by default. Call
     * enableDynamicPayloads() to enable on all pipes (especially for RX nodes
     * that use pipes other than pipe 0 to receive transmissions expecting
     * responses with ACK payloads).
     */
    void enableAckPayload(void);

    /**
     * Disable custom payloads on the acknowledge packets
     *
     * @see enableAckPayload()
     */
    void disableAckPayload(void);

    /**
     * Enable dynamically-sized payloads
     *
     * This way you don't always have to send large packets just to send them
     * once in a while. This enables dynamic payloads on ALL pipes.
     *
     */
    void enableDynamicPayloads(void);

    /**
     * Disable dynamically-sized payloads
     *
     * This disables dynamic payloads on ALL pipes. Since Ack Payloads
     * requires Dynamic Payloads, Ack Payloads are also disabled.
     * If dynamic payloads are later re-enabled and ack payloads are desired
     * then enableAckPayload() must be called again as well.
     *
     */
    void disableDynamicPayloads(void);

    /**
     * Enable dynamic ACKs (single write multicast or unicast) for chosen
     * messages.
     *
     * @note This function must be called once before using the multicast
     * parameter for any functions that offer it. To use multicast behavior
     * about all outgoing payloads (using pipe 0) or incoming payloads
     * (concerning all RX pipes), use setAutoAck()
     *
     * @see
     * - setAutoAck() for all pipes
     * - setAutoAck(uint8_t, bool) for individual pipes
     *
     * @code
     * radio.write(&data, 32, 1); // Sends a payload with no acknowledgement requested
     * radio.write(&data, 32, 0); // Sends a payload using auto-retry/autoACK
     * @endcode
     */
    void enableDynamicAck();

    /**
     * Determine whether the hardware is an nRF24L01+ or not.
     *
     * @return true if the hardware is nRF24L01+ (or compatible) and false
     * if its not.
     */
    bool isPVariant(void);

    /**
     * Enable or disable the auto-acknowledgement feature for all pipes. This
     * feature is enabled by default. Auto-acknowledgement responds to every
     * received payload with an empty ACK packet. These ACK packets get sent
     * from the receiving radio back to the transmitting radio. To attach an
     * ACK payload to a ACK packet, use writeAckPayload().
     *
     * If this feature is disabled on a transmitting radio, then the
     * transmitting radio will always report that the payload was received
     * (even if it was not). Please remember that this feature's configuration
     * needs to match for transmitting and receiving radios.
     *
     * @warning When using the `multicast` parameter to write(), this feature
     * can be disabled for an individual payload. However, if this feature is
     * disabled, then the `multicast` parameter will have no effect.
     *
     * @note If disabling auto-acknowledgment packets, the ACK payloads
     * feature is also disabled as this feature is required to send ACK
     * payloads.
     *
     * @see
     * - write()
     * - writeFast()
     * - startFastWrite()
     * - startWrite()
     * - writeAckPayload()
     *
     * @param enable Whether to enable (true) or disable (false) the
     * auto-acknowledgment feature for all pipes
     */
    void setAutoAck(bool enable);

    /**
     * Enable or disable the auto-acknowledgement feature for a specific pipe.
     * This feature is enabled by default for all pipes. Auto-acknowledgement
     * responds to every received payload with an empty ACK packet. These ACK
     * packets get sent from the receiving radio back to the transmitting
     * radio. To attach an ACK payload to a ACK packet, use writeAckPayload().
     *
     * Pipe 0 is used for TX operations, which include sending ACK packets. If
     * using this feature on both TX & RX nodes, then pipe 0 must have this
     * feature enabled for the RX & TX operations. If this feature is disabled
     * on a transmitting radio's pipe 0, then the transmitting radio will
     * always report that the payload was received (even if it was not).
     * Remember to also enable this feature for any pipe that is openly
     * listening to a transmitting radio with this feature enabled.
     *
     * @warning If this feature is enabled for pipe 0, then the `multicast`
     * parameter to write() can be used to disable this feature for an
     * individual payload. However, if this feature is disabled for pipe 0,
     * then the `multicast` parameter will have no effect.
     *
     * @note If disabling auto-acknowledgment packets on pipe 0, the ACK
     * payloads feature is also disabled as this feature is required on pipe 0
     * to send ACK payloads.
     *
     * @see
     * - write()
     * - writeFast()
     * - startFastWrite()
     * - startWrite()
     * - writeAckPayload()
     * - enableAckPayload()
     * - disableAckPayload()
     *
     * @param pipe Which pipe to configure. This number should be in range
     * [0, 5].
     * @param enable Whether to enable (true) or disable (false) the
     * auto-acknowledgment feature for the specified pipe
     */
    void setAutoAck(uint8_t pipe, bool enable);

    /**
     * Set Power Amplifier (PA) level and Low Noise Amplifier (LNA) state
     *
     * @param level The desired @ref PALevel as defined by @ref rf24_pa_dbm_e.
     * @param lnaEnable Enable or Disable the LNA (Low Noise Amplifier) Gain.
     * See table for Si24R1 modules below. @p lnaEnable only affects
     * nRF24L01 modules with an LNA chip.
     *
     * | @p level (enum value) | nRF24L01<br>description | Si24R1<br>description when<br> @p lnaEnable = 1 | Si24R1<br>description when<br> @p lnaEnable = 0 |
     * |:---------------------:|:-------:|:--------:|:-------:|
     * | @ref RF24_PA_MIN (0)  | -18 dBm |  -6 dBm  | -12 dBm |
     * | @ref RF24_PA_LOW (1)  | -12 dBm |  -0 dBm  | -4 dBm  |
     * | @ref RF24_PA_HIGH (2) | -6 dBm  |  3 dBm   | 1 dBm   |
     * | @ref RF24_PA_MAX (3)  |  0 dBm  |  7 dBm   | 4 dBm   |
     *
     * @note The getPALevel() function does not care what was passed @p lnaEnable parameter.
     */
    void setPALevel(uint8_t level, bool lnaEnable = 1);

    /**
     * Fetches the current @ref PALevel.
     *
     * @return One of the values defined by @ref rf24_pa_dbm_e.
     * See tables in @ref rf24_pa_dbm_e or setPALevel()
     */
    uint8_t getPALevel(void);

    /**
     * Returns automatic retransmission count (ARC_CNT)
     *
     * Value resets with each new transmission. Allows roughly estimating signal strength.
     *
     * @return Returns values from 0 to 15.
     */
    uint8_t getARC(void);

    /**
     * Set the transmission @ref Datarate
     *
     * @warning setting @ref RF24_250KBPS will fail for non-plus modules (when
     * isPVariant() returns false).
     *
     * @param speed Specify one of the following values (as defined by
     * @ref rf24_datarate_e):
     * | @p speed (enum value) | description  |
     * |:---------------------:|:------------:|
     * | @ref RF24_1MBPS (0)   | for 1 Mbps   |
     * | @ref RF24_2MBPS (1)   | for 2 Mbps   |
     * | @ref RF24_250KBPS (2) | for 250 kbps |
     *
     * @return true if the change was successful
     */
    bool setDataRate(rf24_datarate_e speed);

    /**
     * Fetches the currently configured transmission @ref Datarate
     *
     * @return One of the values defined by @ref rf24_datarate_e.
     * See table in @ref rf24_datarate_e or setDataRate()
     */
    rf24_datarate_e getDataRate(void);

    /**
     * Set the @ref CRCLength (in bits)
     *
     * CRC cannot be disabled if auto-ack is enabled
     * @param length Specify one of the values (as defined by @ref rf24_crclength_e)
     * | @p length (enum value)     | description                    |
     * |:--------------------------:|:------------------------------:|
     * | @ref RF24_CRC_DISABLED (0) | to disable using CRC checksums |
     * | @ref RF24_CRC_8 (1)        | to use 8-bit checksums         |
     * | @ref RF24_CRC_16 (2)       | to use 16-bit checksums        |
     */
    void setCRCLength(rf24_crclength_e length);

    /**
     * Get the @ref CRCLength (in bits)
     *
     * CRC checking cannot be disabled if auto-ack is enabled
     * @return One of the values defined by @ref rf24_crclength_e.
     * See table in @ref rf24_crclength_e or setCRCLength()
     */
    rf24_crclength_e getCRCLength(void);

    /**
     * Disable CRC validation
     *
     * @warning CRC cannot be disabled if auto-ack/ESB is enabled.
     */
    void disableCRC(void);

    /**
     * This function is used to configure what events will trigger the Interrupt
     * Request (IRQ) pin active LOW.
     * The following events can be configured:
     * 1. "data sent": This does not mean that the data transmitted was
     * received, only that the attempt to send it was complete.
     * 2. "data failed": This means the data being sent was not received. This
     * event is only triggered when the auto-ack feature is enabled.
     * 3. "data received": This means that data from a receiving payload has
     * been loaded into the RX FIFO buffers. Remember that there are only 3
     * levels available in the RX FIFO buffers.
     *
     * By default, all events are configured to trigger the IRQ pin active LOW.
     * When the IRQ pin is active, use whatHappened() to determine what events
     * triggered it. Remember that calling whatHappened() also clears these
     * events' status, and the IRQ pin will then be reset to inactive HIGH.
     *
     * The following code configures the IRQ pin to only reflect the "data received"
     * event:
     * @code
     * radio.maskIRQ(1, 1, 0);
     * @endcode
     *
     * @param tx_ok  `true` ignores the "data sent" event, `false` reflects the
     * "data sent" event on the IRQ pin.
     * @param tx_fail  `true` ignores the "data failed" event, `false` reflects the
     * "data failed" event on the IRQ pin.
     * @param rx_ready `true` ignores the "data received" event, `false` reflects the
     * "data received" event on the IRQ pin.
     */
    void maskIRQ(bool tx_ok, bool tx_fail, bool rx_ready);

    /**
     *
     * The driver will delay for this duration when stopListening() is called
     *
     * When responding to payloads, faster devices like ARM(RPi) are much faster than Arduino:
     * 1. Arduino sends data to RPi, switches to RX mode
     * 2. The RPi receives the data, switches to TX mode and sends before the Arduino radio is in RX mode
     * 3. If AutoACK is disabled, this can be set as low as 0. If AA/ESB enabled, set to 100uS minimum on RPi
     *
     * @warning If set to 0, ensure 130uS delay after stopListening() and before any sends
     */
    uint32_t txDelay;

    /**
     *
     * On all devices but Linux and ATTiny, a small delay is added to the CSN toggling function
     *
     * This is intended to minimize the speed of SPI polling due to radio commands
     *
     * If using interrupts or timed requests, this can be set to 0 Default:5
     */
    uint32_t csDelay;

    /**
     * Transmission of constant carrier wave with defined frequency and output power
     *
     * @param level Output power to use
     * @param channel The channel to use
     *
     * @warning If isPVariant() returns true, then this function takes extra
     * measures that alter some settings. These settings alterations include:
     * - setAutoAck() to false (for all pipes)
     * - setRetries() to retry `0` times with a delay of 250 microseconds
     * - set the TX address to 5 bytes of `0xFF`
     * - flush_tx()
     * - load a 32 byte payload of `0xFF` into the TX FIFO's top level
     * - disableCRC()
     */
    void startConstCarrier(rf24_pa_dbm_e level, uint8_t channel);

    /**
     * Stop transmission of constant wave and reset PLL and CONT registers
     *
     * @warning this function will powerDown() the radio per recommendation of
     * datasheet.
     * @note If isPVariant() returns true, please remember to re-configure the radio's settings
     * @code
     * // re-establish default settings
     * setCRCLength(RF24_CRC_16);
     * setAutoAck(true);
     * setRetries(5, 15);
     * @endcode
     * @see startConstCarrier()
     */
    void stopConstCarrier(void);

    /**
     * @brief Open or close all data pipes.
     *
     * This function does not alter the addresses assigned to pipes. It is simply a
     * convenience function that allows controlling all pipes at once.
     * @param isEnabled `true` opens all pipes; `false` closes all pipes.
     */
    void toggleAllPipes(bool isEnabled);

    /**
     * @brief configure the RF_SETUP register in 1 transaction
     * @param level This parameter is the same input as setPALevel()'s `level` parameter.
     * See @ref rf24_pa_dbm_e enum for accepted values.
     * @param speed This parameter is the same input as setDataRate()'s `speed` parameter.
     * See @ref rf24_datarate_e enum for accepted values.
     * @param lnaEnable This optional parameter is the same as setPALevel()'s `lnaEnable`
     * optional parameter. Defaults to `true` (meaning LNA feature is enabled) when not specified.
     */
    void setRadiation(uint8_t level, rf24_datarate_e speed, bool lnaEnable = true);

    /**@}*/
    /**
     * @name Deprecated
     *
     *  Methods provided for backwards compatibility.
     */
    /**@{*/

    /**
     * Open a pipe for reading
     * @deprecated For compatibility with old code only, see newer function
     * openReadingPipe()
     *
     * @note Pipes 1-5 should share the first 32 bits.
     * Only the least significant byte should be unique, e.g.
     * @code
     * openReadingPipe(1, 0xF0F0F0F0AA);
     * openReadingPipe(2, 0xF0F0F0F066);
     * @endcode
     *
     * @warning
     * @parblock
     * Pipe 0 is also used by the writing pipe so should typically be avoided as a reading pipe.
     * If used, the reading pipe 0 address needs to be restored at every call to startListening().
     *
     * See http://maniacalbits.blogspot.com/2013/04/rf24-addressing-nrf24l01-radios-require.html
     * @endparblock
     *
     * @param number Which pipe# to open, 0-5.
     * @param address The 40-bit address of the pipe to open.
     */
    void openReadingPipe(uint8_t number, uint64_t address);

    /**
     * Open a pipe for writing
     * @deprecated For compatibility with old code only, see newer function
     * openWritingPipe()
     *
     * Addresses are 40-bit hex values, e.g.:
     *
     * @code
     * openWritingPipe(0xF0F0F0F0F0);
     * @endcode
     *
     * @param address The 40-bit address of the pipe to open.
     */
    void openWritingPipe(uint64_t address);

    /**
     * Determine if an ack payload was received in the most recent call to
     * write(). The regular available() can also be used.
     *
     * @deprecated For compatibility with old code only, see synonymous function available().
     * Use read() to retrieve the ack payload and getDynamicPayloadSize() to get the ACK payload size.
     *
     * @return True if an ack payload is available.
     */
    bool isAckPayloadAvailable(void);

private:
    /**@}*/
    /**
     * @name Low-level internal interface.
     *
     *  Protected methods that address the chip directly.  Regular users cannot
     *  ever call these.  They are documented for completeness and for developers who
     *  may want to extend this class.
     */
    /**@{*/

    /**
     * initializing function specific to all constructors
     * (regardless of constructor parameters)
     */
    void _init_obj();

    /**
     * initialize radio by performing a soft reset.
     * @warning This function assumes the SPI bus object's begin() method has been
     * previously called.
     */
    bool _init_radio();

    /**
     * initialize the GPIO pins
     */
    bool _init_pins();

    /**
     * Set chip select pin
     *
     * Running SPI bus at PI_CLOCK_DIV2 so we don't waste time transferring data
     * and best of all, we make use of the radio's FIFO buffers. A lower speed
     * means we're less likely to effectively leverage our FIFOs and pay a higher
     * AVR runtime cost as toll.
     *
     * @param mode HIGH to take this unit off the SPI bus, LOW to put it on
     */
    void csn(bool mode);

    /**
     * Set chip enable
     *
     * @param level HIGH to actively begin transmission or LOW to put in standby.  Please see data sheet
     * for a much more detailed description of this pin.
     */
    void ce(bool level);

    /**
     * Write a chunk of data to a register
     *
     * @param reg Which register. Use constants from nRF24L01.h
     * @param buf Where to get the data
     * @param len How many bytes of data to transfer
     * @return Nothing. Older versions of this function returned the status
     * byte, but that it now saved to a private member on all SPI transactions.
     */
    void write_register(uint8_t reg, const uint8_t* buf, uint8_t len);

    /**
     * Write a single byte to a register
     *
     * @param reg Which register. Use constants from nRF24L01.h
     * @param value The new value to write
     * @return Nothing. Older versions of this function returned the status
     * byte, but that it now saved to a private member on all SPI transactions.
     */
    void write_register(uint8_t reg, uint8_t value);

    /**
     * Write the transmit payload
     *
     * The size of data written is the fixed payload size, see getPayloadSize()
     *
     * @param buf Where to get the data
     * @param len Number of bytes to be sent
     * @param writeType Specify if individual payload should be acknowledged
     * @return Nothing. Older versions of this function returned the status
     * byte, but that it now saved to a private member on all SPI transactions.
     */
    void write_payload(const void* buf, uint8_t len, const uint8_t writeType);

    /**
     * Read the receive payload
     *
     * The size of data read is the fixed payload size, see getPayloadSize()
     *
     * @param buf Where to put the data
     * @param len Maximum number of bytes to read
     * @return Nothing. Older versions of this function returned the status
     * byte, but that it now saved to a private member on all SPI transactions.
     */
    void read_payload(void* buf, uint8_t len);

    /**
     * Retrieve the current status of the chip
     *
     * @return Current value of status register
     */
    uint8_t get_status(void);

#if !defined(MINIMAL)

    /**
     * Decode and print the given status to stdout
     *
     * @param status Status value to print
     *
     * @warning Does nothing if stdout is not defined.  See fdevopen in stdio.h
     */
    void print_status(uint8_t status);

    /**
     * Decode and print the given 'observe_tx' value to stdout
     *
     * @param value The observe_tx value to print
     *
     * @warning Does nothing if stdout is not defined.  See fdevopen in stdio.h
     */
    void print_observe_tx(uint8_t value);

    /**
     * Print the name and value of an 8-bit register to stdout
     *
     * Optionally it can print some quantity of successive
     * registers on the same line.  This is useful for printing a group
     * of related registers on one line.
     *
     * @param name Name of the register
     * @param reg Which register. Use constants from nRF24L01.h
     * @param qty How many successive registers to print
     */
    void print_byte_register(const char* name, uint8_t reg, uint8_t qty = 1);

    /**
     * Print the name and value of a 40-bit address register to stdout
     *
     * Optionally it can print some quantity of successive
     * registers on the same line.  This is useful for printing a group
     * of related registers on one line.
     *
     * @param name Name of the register
     * @param reg Which register. Use constants from nRF24L01.h
     * @param qty How many successive registers to print
     */
    void print_address_register(const char* name, uint8_t reg, uint8_t qty = 1);

    /**
     * Put the value of a 40-bit address register into a char array
     *
     * Optionally it can print some quantity of successive
     * registers on the same line.  This is useful for printing a group
     * of related registers on one line.
     *
     * @param out_buffer Output buffer, char array
     * @param reg Which register. Use constants from nRF24L01.h
     * @param qty How many successive registers to print
     * @return The total number of characters written to the given buffer.
     */
    uint8_t sprintf_address_register(char* out_buffer, uint8_t reg, uint8_t qty = 1);
#endif

    /**
     * Turn on or off the special features of the chip
     *
     * The chip has certain 'features' which are only available when the 'features'
     * are enabled.  See the datasheet for details.
     */
    void toggle_features(void);

#if defined(FAILURE_HANDLING) || defined(RF24_LINUX)

    void errNotify(void);

#endif

    /**
     * @brief Manipulate the @ref Datarate and txDelay
     *
     * This is a helper function to setRadiation() and setDataRate()
     * @param speed The desired data rate.
     */
    inline uint8_t _data_rate_reg_value(rf24_datarate_e speed);

    /**
     * @brief Manipulate the @ref PALevel
     *
     * This is a helper function to setRadiation() and setPALevel()
     * @param level The desired @ref PALevel.
     * @param lnaEnable Toggle the LNA feature.
     */
    inline uint8_t _pa_level_reg_value(uint8_t level, bool lnaEnable);

    /**@}*/
};

/**
 * @example{lineno} examples/GettingStarted/GettingStarted.ino
 * Written by [2bndy5](http://github.com/2bndy5) in 2020
 *
 * A simple example of sending data from 1 nRF24L01 transceiver to another.
 *
 * This example was written to be used on 2 devices acting as "nodes".
 * Use the Serial Monitor to change each node's behavior.
 */

/**
 * @example{lineno} examples/AcknowledgementPayloads/AcknowledgementPayloads.ino
 * Written by [2bndy5](http://github.com/2bndy5) in 2020
 *
 * A simple example of sending data from 1 nRF24L01 transceiver to another
 * with Acknowledgement (ACK) payloads attached to ACK packets.
 *
 * This example was written to be used on 2 devices acting as "nodes".
 * Use the Serial Monitor to change each node's behavior.
 */

/**
 * @example{lineno} examples/ManualAcknowledgements/ManualAcknowledgements.ino
 * Written by [2bndy5](http://github.com/2bndy5) in 2020
 *
 * A simple example of sending data from 1 nRF24L01 transceiver to another
 * with manually transmitted (non-automatic) Acknowledgement (ACK) payloads.
 * This example still uses ACK packets, but they have no payloads. Instead the
 * acknowledging response is sent with `write()`. This tactic allows for more
 * updated acknowledgement payload data, where actual ACK payloads' data are
 * outdated by 1 transmission because they have to loaded before receiving a
 * transmission.
 *
 * This example was written to be used on 2 devices acting as "nodes".
 * Use the Serial Monitor to change each node's behavior.
 */

/**
 * @example{lineno} examples/StreamingData/StreamingData.ino
 * Written by [2bndy5](http://github.com/2bndy5) in 2020
 *
 * A simple example of streaming data from 1 nRF24L01 transceiver to another.
 *
 * This example was written to be used on 2 devices acting as "nodes".
 * Use the Serial Monitor to change each node's behavior.
 */

/**
 * @example{lineno} examples/MulticeiverDemo/MulticeiverDemo.ino
 * Written by [2bndy5](http://github.com/2bndy5) in 2020
 *
 * A simple example of sending data from as many as 6 nRF24L01 transceivers to
 * 1 receiving transceiver. This technique is trademarked by
 * Nordic Semiconductors as "MultiCeiver".
 *
 * This example was written to be used on up to 6 devices acting as TX nodes &
 * only 1 device acting as the RX node (that's a maximum of 7 devices).
 * Use the Serial Monitor to change each node's behavior.
 */

/**
 * @example{lineno} examples/InterruptConfigure/InterruptConfigure.ino
 * Written by [2bndy5](http://github.com/2bndy5) in 2020
 *
 * This example uses Acknowledgement (ACK) payloads attached to ACK packets to
 * demonstrate how the nRF24L01's IRQ (Interrupt Request) pin can be
 * configured to detect when data is received, or when data has transmitted
 * successfully, or when data has failed to transmit.
 *
 * This example was written to be used on 2 devices acting as "nodes".
 * Use the Serial Monitor to change each node's behavior.
 */

/**
 * @example{lineno} examples/old_backups/GettingStarted_HandlingFailures/GettingStarted_HandlingFailures.ino
 * Written by [TMRh20](http://github.com/TMRh20) in 2019
 *
 * This example demonstrates the basic getting started functionality, but with
 * failure handling for the radio chip. Addresses random radio failures etc,
 * potentially due to loose wiring on breadboards etc.
 */

/**
 * @example{lineno} examples/old_backups/TransferTimeouts/TransferTimeouts.ino
 * Written by [TMRh20](https://github.com/TMRh20)
 *
 * This example demonstrates the use of and extended timeout period and
 * auto-retries/auto-reUse to increase reliability in noisy or low signal scenarios.
 *
 * Write this sketch to two different nodes.  Put one of the nodes into 'transmit'
 * mode by connecting with the serial monitor and sending a 'T'.  The data <br>
 * transfer will begin, with the receiver displaying the payload count and the
 * data transfer rate.
 */

/**
 * @example{lineno} examples/old_backups/pingpair_irq/pingpair_irq.ino
 * Updated by [TMRh20](https://github.com/TMRh20)
 *
 * This is an example of how to user interrupts to interact with the radio, and a demonstration
 * of how to use them to sleep when receiving, and not miss any payloads.<br>
 * The pingpair_sleepy example expands on sleep functionality with a timed sleep option for the transmitter.
 * Sleep functionality is built directly into my fork of the RF24Network library<br>
 */

/**
 * @example{lineno} examples/old_backups/pingpair_sleepy/pingpair_sleepy.ino
 * Updated by [TMRh20](https://github.com/TMRh20)
 *
 * This is an example of how to use the RF24 class to create a battery-
 * efficient system.  It is just like the GettingStarted_CallResponse example, but the<br>
 * ping node powers down the radio and sleeps the MCU after every
 * ping/pong cycle, and the receiver sleeps between payloads. <br>
 */

/**
 * @example{lineno} examples/rf24_ATTiny/rf24ping85/rf24ping85.ino
 * <b>2014 Contribution by [tong67](https://github.com/tong67)</b><br>
 * Updated 2020 by [2bndy5](http://github.com/2bndy5) for the
 * [SpenceKonde ATTinyCore](https://github.com/SpenceKonde/ATTinyCore)<br>
 * The RF24 library uses the [ATTinyCore by
 * SpenceKonde](https://github.com/SpenceKonde/ATTinyCore)
 *
 * This sketch is a duplicate of the ManualAcknowledgements.ino example
 * (without all the Serial input/output code), and it demonstrates
 * a ATTiny25/45/85 or ATTiny24/44/84 driving the nRF24L01 transceiver using
 * the RF24 class to communicate with another node.
 *
 * A simple example of sending data from 1 nRF24L01 transceiver to another
 * with manually transmitted (non-automatic) Acknowledgement (ACK) payloads.
 * This example still uses ACK packets, but they have no payloads. Instead the
 * acknowledging response is sent with `write()`. This tactic allows for more
 * updated acknowledgement payload data, where actual ACK payloads' data are
 * outdated by 1 transmission because they have to loaded before receiving a
 * transmission.
 *
 * This example was written to be used on 2 devices acting as "nodes".
 */

/**
 * @example{lineno} examples/rf24_ATTiny/timingSearch3pin/timingSearch3pin.ino
 * <b>2014 Contribution by [tong67](https://github.com/tong67)</b><br>
 * Updated 2020 by [2bndy5](http://github.com/2bndy5) for the
 * [SpenceKonde ATTinyCore](https://github.com/SpenceKonde/ATTinyCore)<br>
 * The RF24 library uses the [ATTinyCore by
 * SpenceKonde](https://github.com/SpenceKonde/ATTinyCore)
 *
 * This sketch can be used to determine the best settle time values to use for
 * RF24::csDelay in RF24::csn() (private function).
 * @see RF24::csDelay
 *
 * The settle time values used here are 100/20. However, these values depend
 * on the actual used RC combination and voltage drop by LED. The
 * intermediate results are written to TX (PB3, pin 2 -- using Serial).
 *
 * For schematic details, see introductory comment block in the rf24ping85.ino sketch.
 */

/**
 * @example{lineno} examples/old_backups/pingpair_dyn/pingpair_dyn.ino
 *
 * This is an example of how to use payloads of a varying (dynamic) size on Arduino.
 */

/**
 * @example{lineno} examples_linux/getting_started.py
 * Written by [2bndy5](http://github.com/2bndy5) in 2020
 *
 * This is a simple example of using the RF24 class on a Raspberry Pi.
 *
 * Remember to install the [Python wrapper](python_wrapper.md), then
 * navigate to the "RF24/examples_linux" folder.
 * <br>To run this example, enter
 * @code{.sh}python3 getting_started.py @endcode and follow the prompts.
 *
 * @note this example requires python v3.7 or newer because it measures
 * transmission time with `time.monotonic_ns()`.
 */

/**
 * @example{lineno} examples_linux/acknowledgement_payloads.py
 * Written by [2bndy5](http://github.com/2bndy5) in 2020
 *
 * This is a simple example of using the RF24 class on a Raspberry Pi to
 * transmit and retrieve custom automatic acknowledgment payloads.
 *
 * Remember to install the [Python wrapper](python_wrapper.md), then
 * navigate to the "RF24/examples_linux" folder.
 * <br>To run this example, enter
 * @code{.sh}python3 acknowledgement_payloads.py @endcode and follow the prompts.
 *
 * @note this example requires python v3.7 or newer because it measures
 * transmission time with `time.monotonic_ns()`.
 */

/**
 * @example{lineno} examples_linux/manual_acknowledgements.py
 * Written by [2bndy5](http://github.com/2bndy5) in 2020
 *
 * This is a simple example of using the RF24 class on a Raspberry Pi to
 * transmit and respond with acknowledgment (ACK) transmissions. Notice that
 * the auto-ack feature is enabled, but this example doesn't use automatic ACK
 * payloads because automatic ACK payloads' data will always be outdated by 1
 * transmission. Instead, this example uses a call and response paradigm.
 *
 * Remember to install the [Python wrapper](python_wrapper.md), then
 * navigate to the "RF24/examples_linux" folder.
 * <br>To run this example, enter
 * @code{.sh}python3 manual_acknowledgements.py @endcode and follow the prompts.
 *
 * @note this example requires python v3.7 or newer because it measures
 * transmission time with `time.monotonic_ns()`.
 */

/**
 * @example{lineno} examples_linux/streaming_data.py
 * Written by [2bndy5](http://github.com/2bndy5) in 2020
 *
 * This is a simple example of using the RF24 class on a Raspberry Pi for
 * streaming multiple payloads.
 *
 * Remember to install the [Python wrapper](python_wrapper.md), then
 * navigate to the "RF24/examples_linux" folder.
 * <br>To run this example, enter
 * @code{.sh}python3 streaming_data.py @endcode and follow the prompts.
 *
 * @note this example requires python v3.7 or newer because it measures
 * transmission time with `time.monotonic_ns()`.
 */

/**
 * @example{lineno} examples_linux/interrupt_configure.py
 * Written by [2bndy5](http://github.com/2bndy5) in 2020
 *
 * This is a simple example of using the RF24 class on a Raspberry Pi to
 * detecting (and verifying) the IRQ (interrupt) pin on the nRF24L01.
 *
 * Remember to install the [Python wrapper](python_wrapper.md), then
 * navigate to the "RF24/examples_linux" folder.
 * <br>To run this example, enter
 * @code{.sh}python3 interrupt_configure.py @endcode and follow the prompts.
 *
 * @note this example requires python v3.7 or newer because it measures
 * transmission time with `time.monotonic_ns()`.
 */

/**
 * @example{lineno} examples_linux/multiceiver_demo.py
 * Written by [2bndy5](http://github.com/2bndy5) in 2020
 *
 * This is a simple example of using the RF24 class on a Raspberry Pi for
 * using 1 nRF24L01 to receive data from up to 6 other transceivers. This
 * technique is called "multiceiver" in the datasheet.
 *
 * Remember to install the [Python wrapper](python_wrapper.md), then
 * navigate to the "RF24/examples_linux" folder.
 * <br>To run this example, enter
 * @code{.sh}python3 multiceiver_demo.py @endcode and follow the prompts.
 *
 * @note this example requires python v3.7 or newer because it measures
 * transmission time with `time.monotonic_ns()`.
 */

/**
 * @example{lineno} examples_linux/scanner.cpp
 *
 * Example to detect interference on the various channels available.
 * This is a good diagnostic tool to check whether you're picking a
 * good channel for your application.
 *
 * Inspired by cpixip.
 * See http://arduino.cc/forum/index.php/topic,54795.0.html
 *
 * Use ctrl+C to exit
 */

/**
 * @example{lineno} examples/scanner/scanner.ino
 *
 * Example to detect interference on the various channels available.
 * This is a good diagnostic tool to check whether you're picking a
 * good channel for your application.
 *
 * Inspired by cpixip.
 * See http://arduino.cc/forum/index.php/topic,54795.0.html
 */

/**
 * @example{lineno} examples_linux/gettingstarted.cpp
 * Written by [2bndy5](http://github.com/2bndy5) in 2020
 *
 * A simple example of sending data from 1 nRF24L01 transceiver to another.
 *
 * This example was written  * This example was written to be used on up to 6 devices acting as TX nodes &
 * only 1 device acting as the RX node (that's a maximum of 7 devices).
 acting as "nodes".
 * Use `ctrl+c` to quit at any time.
 */

/**
 * @example{lineno} examples_linux/acknowledgementPayloads.cpp
 * Written by [2bndy5](http://github.com/2bndy5) in 2020
 *
 * A simple example of sending data from 1 nRF24L01 transceiver to another
 * with Acknowledgement (ACK) payloads attached to ACK packets.
 *
 * This example was written to be used on 2 devices acting as "nodes".
 * Use `ctrl+c` to quit at any time.
 */

/**
 * @example{lineno} examples_linux/manualAcknowledgements.cpp
 * Written by [2bndy5](http://github.com/2bndy5) in 2020
 *
 * A simple example of sending data from 1 nRF24L01 transceiver to another
 * with manually transmitted (non-automatic) Acknowledgement (ACK) payloads.
 * This example still uses ACK packets, but they have no payloads. Instead the
 * acknowledging response is sent with `write()`. This tactic allows for more
 * updated acknowledgement payload data, where actual ACK payloads' data are
 * outdated by 1 transmission because they have to loaded before receiving a
 * transmission.
 *
 * This example was written to be used on 2 devices acting as "nodes".
 * Use `ctrl+c` to quit at any time.
 */

/**
 * @example{lineno} examples_linux/streamingData.cpp
 * Written by [2bndy5](http://github.com/2bndy5) in 2020
 *
 * A simple example of sending data from 1 nRF24L01 transceiver to another.
 *
 * This example was written to be used on 2 devices acting as "nodes".
 * Use `ctrl+c` to quit at any time.
 */

/**
 * @example{lineno} examples_linux/multiceiverDemo.cpp
 * Written by [2bndy5](http://github.com/2bndy5) in 2020
 *
 * A simple example of sending data from as many as 6 nRF24L01 transceivers to
 * 1 receiving transceiver. This technique is trademarked by
 * Nordic Semiconductors as "MultiCeiver".
 *
 * This example was written to be used on up to 6 devices acting as TX nodes &
 * only 1 device acting as the RX node (that's a maximum of 7 devices).
 * Use `ctrl+c` to quit at any time.
 */

#endif // RF24_H_
