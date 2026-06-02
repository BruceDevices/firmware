/**
 * @file PN532.h
 * @author Rennan Cockles (https://github.com/rennancockles)
 * @brief Read and Write RFID tags using PN532 module
 * @version 0.1
 * @date 2024-08-19
 */

#include "RFIDInterface.h"
#define private public
#include <Adafruit_PN532.h>
#undef private

class PN532 : public RFIDInterface {
public:
    enum CONNECTION_TYPE { I2C = 1, I2C_SPI = 2, SPI = 3 };
    enum PICC_Type {
        PICC_TYPE_MIFARE_MINI = 0x09, // MIFARE Classic protocol, 320 bytes
        PICC_TYPE_MIFARE_1K = 0x08,   // MIFARE Classic protocol, 1KB
        PICC_TYPE_MIFARE_4K = 0x18,   // MIFARE Classic protocol, 4KB
        PICC_TYPE_MIFARE_UL = 0x00,   // MIFARE Ultralight or Ultralight C
    };

// Devices such as T-Embed CC1101 uses an embedded PN532 that needs the IRQ and RST pins to work
// If using other device that uses, set -DPN532_IRQ=pin_num and -DPN532_RF_REST=pin_num to platformio.ini
// of this particular device, should not be used in other devices on I2C mode
#if defined(PN532_IRQ) && defined(PN532_RF_REST)
    Adafruit_PN532 nfc = Adafruit_PN532(PN532_IRQ, PN532_RF_REST);
#else
    Adafruit_PN532 nfc = Adafruit_PN532();
#endif

    /////////////////////////////////////////////////////////////////////////////////////
    // Constructor
    /////////////////////////////////////////////////////////////////////////////////////
    PN532(CONNECTION_TYPE connection_type = I2C);

    /////////////////////////////////////////////////////////////////////////////////////
    // Life Cycle
    /////////////////////////////////////////////////////////////////////////////////////
    bool begin();

    /////////////////////////////////////////////////////////////////////////////////////
    // Operations
    /////////////////////////////////////////////////////////////////////////////////////
    int read(int cardBaudRate = PN532_MIFARE_ISO14443A);
    int clone();
    int erase();
    int write(int cardBaudRate = PN532_MIFARE_ISO14443A);
    int write_ndef();
    int emulate() override;
    int load();
    int save(String filename);

private:
    bool _use_i2c;
    CONNECTION_TYPE _connection_type;

    /////////////////////////////////////////////////////////////////////////////////////
    // Converters
    /////////////////////////////////////////////////////////////////////////////////////
    void format_data();
    void format_data_felica(uint8_t idm[8], uint8_t pmm[8], uint16_t sys_code);
    void parse_data();
    void set_uid();

    /////////////////////////////////////////////////////////////////////////////////////
    // PICC Helpers
    /////////////////////////////////////////////////////////////////////////////////////
    String get_tag_type();
    int read_data_blocks();
    int read_mifare_classic_data_blocks();
    int read_mifare_classic_data_sector(byte sector);
    int authenticate_mifare_classic(byte block);
    int read_mifare_ultralight_data_blocks();

    int write_data_blocks();
    bool write_mifare_classic_data_block(int block, String data);
    bool write_mifare_ultralight_data_block(int block, String data);

    int read_felica_data();

    int erase_data_blocks();
    int write_ndef_blocks();

    int write_felica_data_block(int block, String data);
};
