#ifndef __RF_UTILS_H__
#define __RF_UTILS_H__

#include "protocols/rf_keeloq.h" // KeeLoq cipher/keystore + bitAt/g5/KEELOQ_* macros
#include "structs.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>
// ESP-IDF 5.5 based framework determines the channels autommatically
// you do not have the hability to choose the channel
// radioReady preserves a caller's already configured frequency and RX preset.
rmt_channel_handle_t setup_rf_rx(bool radioReady = false);

#define RMT_MAX_PULSES 10000 // Maximum number of pulses to record
#define RMT_CLK_DIV 80       /*!< RMT counter clock divider */
#define RMT_1US_TICKS (80000000 / RMT_CLK_DIV / 1000000)
#define RMT_1MS_TICKS (RMT_1US_TICKS * 1000)
#define SIGNAL_STRENGTH_THRESHOLD 1500 // Adjust this threshold as needed

extern const float subghz_frequency_list[57];
extern const char *subghz_frequency_ranges[];
extern const int range_limits[4][2];
extern bool rmtInstalled;

bool initRfModule(String mode = "", float frequency = 0);
void deinitRfModule();
void initCC1101once(SPIClass *SSPI);

// Serializes CC1101 SPI traffic against the display's SPI transactions.
//
// On boards that define RF_SHARED_SPI_BUS_LOCK the CC1101 is wired to the same
// pins as the TFT, so acquireSPIBus() hands the RF layer the display's own
// SPIClass instance. The SmartRC driver opens/closes a transaction around every
// register access (SpiStart()/SpiEnd()), i.e. it calls beginTransaction()/
// endTransaction() on a shared instance. If a console command (serial command
// task) does that while the render task is in the middle of a draw,
// endTransaction() releases an SPI parameter mutex that belongs to the OTHER
// task - FreeRTOS asserts (xTaskPriorityDisinherit) and the board reboots. Every
// RF entry point wraps its CC1101 access in these two helpers: the outermost
// lock also claims the display's bus mutex, so RF and rendering can never
// interleave. Nested calls are cheap (recursive), so callers may lock coarsely.
// On boards without the flag these compile to no-ops.
void rf_spi_lock();
void rf_spi_unlock();

void setMHZ(float frequency);
// getRssi() is one SPI burst as well; go through this wrapper rather than
// calling ELECHOUSE_cc1101.getRssi() directly from a non-render task.
int rfGetRssi();

// Set the CC1101 RX filter bandwidth (kHz). Goes through the bus lock like every
// other register access, so it is safe to call while the display is drawing.
void rfSetRxBW(float bandwidth);

// True while any page has the radio in RX. Used to refuse a mode change under a
// running capture.
bool rfRadioReceiving();
extern volatile int rfRxSessions;
int find_pulse_index(const std::vector<int> &indexed_durations, int duration);
uint64_t crc64_ecma(const std::vector<int> &data);

void addToRecentCodes(struct RfCodes rfcode);
struct RfCodes selectRecentRfMenu();
bool setMHZMenu();
void rf_range_selection(float currentFrequency = 0.0);

uint64_t reverse_bits(uint64_t num, uint8_t bits);

String rf_subghz_header(float frequencyMHz);

#endif
