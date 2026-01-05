/*
 * This file is part of arduino-pn532-srix.
 * arduino-pn532-srix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * arduino-pn532-srix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with arduino-pn532-srix.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Lilz
 * @license  GNU Lesser General Public License v3.0 (see license.txt)
 *  Refactored by Senape3000 to reuse Adafruit_PN532 constants only
 *  This is a library for the communication with an I2C PN532 NFC/RFID breakout board.
 *  adapted from Adafruit's library.
 *  This library supports only I2C to communicate.
 */

#ifndef PN532_SRIX_H
#define PN532_SRIX_H

#include <Adafruit_PN532.h> // Only for constants (#define)
#include <Arduino.h>
#include <Wire.h>

// SRIX4K-specific commands
#define SRIX4K_INITIATE (0x06)
#define SRIX4K_SELECT (0x0E)
#define SRIX4K_READBLOCK (0x08)
#define SRIX4K_WRITEBLOCK (0x09)
#define SRIX4K_GETUID (0x0B)

class Arduino_PN532_SRIX {
public:
    Arduino_PN532_SRIX(uint8_t irq, uint8_t reset);
    Arduino_PN532_SRIX();

    bool init();
    uint32_t getFirmwareVersion();
    bool setPassiveActivationRetries(uint8_t maxRetries);

    bool SRIX_init();
    bool SRIX_initiate_select();
    bool SRIX_read_block(uint8_t address, uint8_t *block);
    bool SRIX_write_block(uint8_t address, uint8_t *block);
    bool SRIX_get_uid(uint8_t *buffer);

private:
    uint8_t _irq, _reset;
    uint8_t _packetbuffer[64];

    // Low-level I2C functions (reimplemented from original)
    bool SAMConfig();
    void readData(uint8_t *buffer, uint8_t n);
    bool readACK();
    bool isReady();
    bool waitReady(uint16_t timeout);
    void writeCommand(uint8_t *command, uint8_t commandLength);
    bool sendCommandCheckAck(uint8_t *command, uint8_t commandLength, uint16_t timeout = 100);
};

#endif
