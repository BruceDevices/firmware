#ifndef __HC11_RADIO_H__
#define __HC11_RADIO_H__

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

/*
 * HC-11 radio layer: RF over the CC1101 transceiver using HC-11 packet
 * formats for communication with compatible modules and peers.
 *
 * The layer owns the radio for the whole app session (half-duplex): it
 * packetizes transparent-mode host data into HC-11 FU1/FU3 packets and
 * delivers received HC-11 packets (own address, CRC OK) back to the app.
 * There is no virtual fallback: hc11RadioInit() fails without a CC1101 and
 * the app must show an error and exit.
 */

// Ring buffer between the host (transparent mode) and the radio. Bounded:
// on overflow new bytes are dropped and flagged for the UI.
#define HC11_RADIO_TX_BUF_SIZE 1024

// Max bytes written to the TX FIFO in one packet:
// FU3: 1 len + addr + 0x44 + up to 55 payload = 58; FU1 short packet: 58.
#define HC11_RADIO_MAX_FIFO_PKT 58

enum Hc11RadioState : uint8_t {
    HC11_RADIO_OFF = 0, // not initialized / stopped
    HC11_RADIO_RX,      // listening (SRX)
    HC11_RADIO_TX_CHAIN // transmitting a burst, packet by packet
};

struct Hc11RadioStatus {
    Hc11RadioState state = HC11_RADIO_OFF;
    float lastRssiDbm = 0.0f;  // RSSI of the last received packet
    uint8_t lastLqi = 0;       // LQI of the last received packet
    uint32_t droppedBytes = 0; // bytes dropped on TX buffer overflow
    bool overflow = false;     // sticky overflow indication for the UI
};

// Called from hc11RadioPoll() with the payload of a valid HC-11 packet
// (own address, CRC OK). ctx is passed through unchanged.
typedef void (*Hc11RadioRxCb)(const uint8_t *data, size_t len, void *ctx);

// Bring the radio up with the given HC-11 configuration (factory values of
// the emulated module on app launch). txByteCounter, if set, is incremented
// by the payload bytes actually transmitted over the air. The counter
// reflects transmissions, not buffer writes.
// Returns false when no CC1101 is present/answering - the caller must show
// an error and exit (there is no virtual mode).
bool hc11RadioInit(
    uint16_t channel, uint16_t address, uint8_t txPower, uint8_t fuMode, uint32_t *txByteCounter
);

// Release the radio completely: ISR off, IDLE, antenna path off.
void hc11RadioStop(void);

// Set the receive callback (payload delivery for the host port).
void hc11RadioSetRxCallback(Hc11RadioRxCb cb, void *ctx);

// Queue a live configuration change (AT command or config screen).
// Applied by hc11RadioPoll() at the next safe moment (radio in RX, no
// packet in flight); takes effect without an app restart.
void hc11RadioApplyConfig(uint16_t channel, uint16_t address, uint8_t txPower, uint8_t fuMode);

// Push transparent-mode host bytes towards the radio (TX buffer).
void hc11RadioFeed(const uint8_t *data, size_t len);

// Drive the radio state machine: receive completed packets, apply pending
// config, start/continue TX bursts. Must be called from every app UI loop
// (hc11ProcessUsb() does it), like the USB drain itself.
void hc11RadioPoll(void);

// Snapshot for the status screen.
Hc11RadioStatus hc11RadioGetStatus(void);

#endif // __HC11_RADIO_H__
