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
 *  Refactored by Senape3000 to reuse Adafruit_PN532 constants only (v1.2)
 *  This is a library for the communication with an I2C PN532 NFC/RFID breakout board.
 *  adapted from Adafruit's library.
 *  This library supports only I2C to communicate.
 */

#include "pn532_srix.h"

// Static ACK pattern
static const uint8_t pn532ack[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
static const uint8_t pn532response_firmwarevers[] = {0x00, 0x00, 0xFF, 0x06, 0xFA, 0xD5};

// ========== CONSTRUCTORS ==========

Arduino_PN532_SRIX::Arduino_PN532_SRIX(uint8_t irq, uint8_t reset) : _irq(irq), _reset(reset) {
    if (_irq != 255) pinMode(_irq, INPUT);
    if (_reset != 255) pinMode(_reset, OUTPUT);
}

Arduino_PN532_SRIX::Arduino_PN532_SRIX() : _irq(255), _reset(255) {}

// ========== INITIALIZATION ==========

bool Arduino_PN532_SRIX::init() {
    // Note: Wire.begin() must be called BEFORE creating this object

    // Hardware reset if available
    if (_reset != 255) {
        digitalWrite(_reset, HIGH);
        digitalWrite(_reset, LOW);
        delay(400);
        digitalWrite(_reset, HIGH);
        delay(10);
    }

    return SAMConfig();
}

bool Arduino_PN532_SRIX::SAMConfig() {
    _packetbuffer[0] = PN532_COMMAND_SAMCONFIGURATION;
    _packetbuffer[1] = 0x01; // Normal mode
    _packetbuffer[2] = 0x14; // Timeout 50ms * 20 = 1s
    _packetbuffer[3] = 0x01; // Use IRQ pin

    if (!sendCommandCheckAck(_packetbuffer, 4)) { return false; }

    readData(_packetbuffer, 8);
    return (_packetbuffer[6] == 0x15);
}

// ========== LOW-LEVEL I2C FUNCTIONS ==========

void Arduino_PN532_SRIX::readData(uint8_t *buffer, uint8_t n) {
    delay(2);
    Wire.requestFrom((uint8_t)PN532_I2C_ADDRESS, (uint8_t)(n + 2));

    // Discard RDY byte
    Wire.read();

    // Read data
    for (uint8_t i = 0; i < n; i++) {
        delay(1);
        buffer[i] = Wire.read();
    }
}

bool Arduino_PN532_SRIX::readACK() {
    uint8_t ackBuffer[6];
    readData(ackBuffer, 6);

#ifdef SRIX_LIB_DEBUG
    Serial.print("[PN532] ACK Read: ");
    for(int i=0; i<6; i++) {
        if(ackBuffer[i] < 0x10) Serial.print("0");
        Serial.print(ackBuffer[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
#endif

    bool result = (memcmp(ackBuffer, pn532ack, 6) == 0);
    
#ifdef SRIX_LIB_DEBUG
    if(!result) Serial.println("[PN532] ❌ ACK Failed");
#endif

    return result;
}

bool Arduino_PN532_SRIX::isReady() {
    // --- POLLING MODE (No IRQ pin) ---
    if (_irq == 255) {
        // Request 1 byte from PN532 (Status Byte)
        delayMicroseconds(500);
        Wire.requestFrom((uint8_t)PN532_I2C_ADDRESS, (uint8_t)1);

        // If no response or bus locked, assume not ready
        if (!Wire.available()) {
#ifdef SRIX_LIB_DEBUG
            Serial.println("[PN532] isReady: No response from I2C (bus busy or chip offline)");
#endif
            return false;
        }

        // Read status byte
        uint8_t status = Wire.read();

#ifdef SRIX_LIB_DEBUG
        Serial.print("[PN532] Status byte: 0x");
        if(status < 0x10) Serial.print("0");
        Serial.print(status, HEX);
        Serial.print(" -> ");
        Serial.println((status & 0x01) ? "READY" : "BUSY");
#endif

        // Check Bit 0 (LSB):
        // 1 = Ready
        // 0 = Busy
        return (status & 0x01);
    }

    // --- IRQ MODE (Hardware pin) ---
    // IRQ pin goes LOW when chip is ready
    bool ready = (digitalRead(_irq) == LOW);

#ifdef SRIX_LIB_DEBUG
    Serial.print("[PN532] IRQ pin ");
    Serial.print(_irq);
    Serial.print(": ");
    Serial.println(ready ? "LOW (Ready)" : "HIGH (Busy)");
#endif

    return ready;
}

bool Arduino_PN532_SRIX::waitReady(uint16_t timeout) {
    uint16_t timer = 0;

    while (!isReady()) {
        if (timeout != 0) {
            timer += 10;
            if (timer > timeout) return false;
        }
        delay(10);
    }
    SRIX_LIB_LOG("Ready from waitReady");
    return true;
}

void Arduino_PN532_SRIX::writeCommand(uint8_t *command, uint8_t commandLength) {
    uint8_t checksum;
    commandLength++;

    delay(2); // Wakeup delay

    Wire.beginTransmission(PN532_I2C_ADDRESS);

    checksum = PN532_PREAMBLE + PN532_STARTCODE1 + PN532_STARTCODE2;
    Wire.write(PN532_PREAMBLE);
    Wire.write(PN532_STARTCODE1);
    Wire.write(PN532_STARTCODE2);

    Wire.write(commandLength);
    Wire.write(~commandLength + 1);

    Wire.write(PN532_HOSTTOPN532);
    checksum += PN532_HOSTTOPN532;

    for (uint8_t i = 0; i < commandLength - 1; i++) {
        Wire.write(command[i]);
        checksum += command[i];
    }

    Wire.write((uint8_t)~checksum);
    Wire.write(PN532_POSTAMBLE);

    Wire.endTransmission();
}

bool Arduino_PN532_SRIX::sendCommandCheckAck(uint8_t *command, uint8_t commandLenght, uint16_t timeout) {
    // default timeout of one second
    // write the command
    writeCommand(command, commandLenght);

    // Wait for chip to say its ready!
    if (!waitReady(timeout)) { return false; }

#ifdef SRIX_LIB_DEBUG
    Serial.println(F("\nI2C IRQ received"));
#endif

    // Check acknowledgement
    if (!readACK()) {
#ifdef SRIX_LIB_DEBUG
        Serial.println(F("\nNo ACK frame received!"));
#endif
        return false;
    }

    return true; // ACK is valid
}

// ========== PN532 GENERIC FUNCTIONS ==========

uint32_t Arduino_PN532_SRIX::getFirmwareVersion() {
    _packetbuffer[0] = PN532_COMMAND_GETFIRMWAREVERSION;

    if (!sendCommandCheckAck(_packetbuffer, 1)) { return 0; }

    readData(_packetbuffer, 12);

    if (memcmp(_packetbuffer, pn532response_firmwarevers, 6) != 0) { return 0; }

    uint32_t response = _packetbuffer[7];
    response <<= 8;
    response |= _packetbuffer[8];
    response <<= 8;
    response |= _packetbuffer[9];
    response <<= 8;
    response |= _packetbuffer[10];

    return response;
}

bool Arduino_PN532_SRIX::setPassiveActivationRetries(uint8_t maxRetries) {
    _packetbuffer[0] = PN532_COMMAND_RFCONFIGURATION;
    _packetbuffer[1] = 5;
    _packetbuffer[2] = 0xFF;
    _packetbuffer[3] = 0x01;
    _packetbuffer[4] = maxRetries;

    return sendCommandCheckAck(_packetbuffer, 5);
}

// ========== SRIX FUNCTIONS ==========

bool Arduino_PN532_SRIX::SRIX_init() {
    _packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
    _packetbuffer[1] = 0x01;
    _packetbuffer[2] = 0x03;
    _packetbuffer[3] = 0x00;

    return sendCommandCheckAck(_packetbuffer, 4);
}

bool Arduino_PN532_SRIX::SRIX_initiate_select() {
    uint8_t chip_id;

    // INITIATE
    _packetbuffer[0] = PN532_COMMAND_INCOMMUNICATETHRU;
    _packetbuffer[1] = SRIX4K_INITIATE;
    _packetbuffer[2] = 0x00;

    if (!sendCommandCheckAck(_packetbuffer, 3)) return false;

    readData(_packetbuffer, 10);

    if (_packetbuffer[7] != 0x00) return false;

    chip_id = _packetbuffer[8];

    // SELECT
    _packetbuffer[0] = PN532_COMMAND_INCOMMUNICATETHRU;
    _packetbuffer[1] = SRIX4K_SELECT;
    _packetbuffer[2] = chip_id;

    if (!sendCommandCheckAck(_packetbuffer, 3)) return false;

    readData(_packetbuffer, 10);

    return (_packetbuffer[7] == 0x00);
}

bool Arduino_PN532_SRIX::SRIX_read_block(uint8_t address, uint8_t *block) {
    _packetbuffer[0] = PN532_COMMAND_INCOMMUNICATETHRU;
    _packetbuffer[1] = SRIX4K_READBLOCK;
    _packetbuffer[2] = address;

    if (!sendCommandCheckAck(_packetbuffer, 3)) return false;

    readData(_packetbuffer, 13);

    if (_packetbuffer[7] != 0x00) return false;

    block[0] = _packetbuffer[8];
    block[1] = _packetbuffer[9];
    block[2] = _packetbuffer[10];
    block[3] = _packetbuffer[11];

    return true;
}

bool Arduino_PN532_SRIX::SRIX_write_block(uint8_t address, uint8_t *block) {
    _packetbuffer[0] = PN532_COMMAND_INCOMMUNICATETHRU;
    _packetbuffer[1] = SRIX4K_WRITEBLOCK;
    _packetbuffer[2] = address;
    _packetbuffer[3] = block[0];
    _packetbuffer[4] = block[1];
    _packetbuffer[5] = block[2];
    _packetbuffer[6] = block[3];

    if (!sendCommandCheckAck(_packetbuffer, 7)) {
        SRIX_LIB_LOG("[SRIX_LIB] Write FAIL");
        return false; }
    SRIX_LIB_LOG("[SRIX_LIB] Write OK");
    return true;
}

bool Arduino_PN532_SRIX::SRIX_get_uid(uint8_t *buffer) {
    _packetbuffer[0] = PN532_COMMAND_INCOMMUNICATETHRU;
    _packetbuffer[1] = SRIX4K_GETUID;

    if (!sendCommandCheckAck(_packetbuffer, 2)) return false;

    readData(_packetbuffer, 16);

    if (_packetbuffer[7] != 0x00) return false;

    for (int i = 0; i < 8; i++) { buffer[i] = _packetbuffer[8 + i]; }

    return true;
}
