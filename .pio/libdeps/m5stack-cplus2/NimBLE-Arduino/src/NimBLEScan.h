/*
 * Copyright 2020-2026 Ryan Powell <ryan@nable-embedded.io> and
 * esp-nimble-cpp, NimBLE-Arduino contributors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NIMBLE_CPP_SCAN_H_
#define NIMBLE_CPP_SCAN_H_

#include "nimconfig.h"
#if CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_OBSERVER

# include "NimBLEAdvertisedDevice.h"
# include "NimBLEUtils.h"

# ifdef USING_NIMBLE_ARDUINO_HEADERS
#  include "nimble/nimble/host/include/host/ble_gap.h"
# else
#  include "host/ble_gap.h"
# endif

# include <vector>
# include <cinttypes>
# include <cstdio>

class NimBLEDevice;
class NimBLEScan;
class NimBLEAdvertisedDevice;
class NimBLEScanCallbacks;
class NimBLEAddress;

/**
 * @brief A class that contains and operates on the results of a BLE scan.
 * @details When a scan completes, we have a set of found devices.  Each device is described
 * by a NimBLEAdvertisedDevice object.  The number of items in the set is given by
 * getCount().  We can retrieve a device by calling getDevice() passing in the
 * index (starting at 0) of the desired device.
 */
class NimBLEScanResults {
  public:
    void                                                 dump() const;
    int                                                  getCount() const;
    const NimBLEAdvertisedDevice*                        getDevice(uint32_t idx) const;
    const NimBLEAdvertisedDevice*                        getDevice(const NimBLEAddress& address) const;
    std::vector<NimBLEAdvertisedDevice*>::const_iterator begin() const;
    std::vector<NimBLEAdvertisedDevice*>::const_iterator end() const;

  private:
    friend NimBLEScan;
    std::vector<NimBLEAdvertisedDevice*> m_deviceVec;
};

/**
 * @brief Perform and manage %BLE scans.
 *
 * Scanning is associated with a %BLE client that is attempting to locate BLE servers.
 */
class NimBLEScan {
  public:
    bool              start(uint32_t duration, bool isContinue = false, bool restart = true);
    bool              isScanning();
    void              setScanCallbacks(NimBLEScanCallbacks* pScanCallbacks, bool wantDuplicates = false);
    void              setActiveScan(bool active);
    void              setInterval(uint16_t intervalMs);
    void              setWindow(uint16_t windowMs);
    void              setDuplicateFilter(uint8_t enabled);
    void              setLimitedOnly(bool enabled);
    void              setFilterPolicy(uint8_t filter);
    bool              stop();
    void              clearResults();
    NimBLEScanResults getResults();
    NimBLEScanResults getResults(uint32_t duration, bool is_continue = false);
    void              setMaxResults(uint8_t maxResults);
    void              erase(const NimBLEAddress& address);
    void              erase(const NimBLEAdvertisedDevice* device);
    void              setScanResponseTimeout(uint32_t timeoutMs);
    std::string       getStatsString() const { return m_stats.toString(); }

# if CONFIG_BT_NIMBLE_EXT_ADV
    enum Phy { SCAN_1M = 0x01, SCAN_CODED = 0x02, SCAN_ALL = 0x03 };
    void setPhy(Phy phyMask);
    void setPeriod(uint32_t periodMs);
# endif

  private:
    friend class NimBLEDevice;

    struct stats {
# if MYNEWT_VAL(NIMBLE_CPP_LOG_LEVEL) >= 4
        uint32_t devCount        = 0; // unique devices seen for the first time
        uint32_t dupCount        = 0; // repeat advertisements from already-known devices
        uint32_t srMinMs         = UINT32_MAX;
        uint32_t srMaxMs         = 0;
        uint64_t srTotalMs       = 0; // uint64 to avoid overflow on long/busy scans
        uint32_t srCount         = 0; // matched scan responses (advertisement + SR pair)
        uint32_t orphanedSrCount = 0; // scan responses received with no prior advertisement
        uint32_t missedSrCount   = 0; // scannable devices for which no SR ever arrived

        void reset() {
            devCount        = 0;
            dupCount        = 0;
            srMinMs         = UINT32_MAX;
            srMaxMs         = 0;
            srTotalMs       = 0;
            srCount         = 0;
            orphanedSrCount = 0;
            missedSrCount   = 0;
        }

        void incDevCount() { devCount++; }
        void incDupCount() { dupCount++; }
        void incMissedSrCount() { missedSrCount++; }
        void incOrphanedSrCount() { orphanedSrCount++; }

        std::string toString() const {
            std::string out;
            out.resize(400); // should be more than enough for the stats string
            snprintf(&out[0],
                     out.size(),
                     "Scan stats:\n"
                     "  Devices seen      : %" PRIu32 "\n"
                     "  Duplicate advs    : %" PRIu32 "\n"
                     "  Scan responses    : %" PRIu32 "\n"
                     "  SR timing (ms)    : min=%" PRIu32 ", max=%" PRIu32 ", avg=%" PRIu64 "\n"
                     "  Orphaned SR       : %" PRIu32 "\n"
                     "  Missed SR         : %" PRIu32 "\n",
                     devCount,
                     dupCount,
                     srCount,
                     srCount ? srMinMs : 0,
                     srCount ? srMaxMs : 0,
                     srCount ? srTotalMs / srCount : 0,
                     orphanedSrCount,
                     missedSrCount);
            return out;
        }

        // Records scan-response round-trip time.
        void recordSrTime(uint32_t ticks) {
            uint32_t ms;
            ble_npl_time_ticks_to_ms(ticks, &ms);

            if (ms < srMinMs) {
                srMinMs = ms;
            }
            if (ms > srMaxMs) {
                srMaxMs = ms;
            }
            srTotalMs += ms;
            srCount++;
            return;
        }
# else
        void        reset() {}
        void        incDevCount() {}
        void        incDupCount() {}
        void        incMissedSrCount() {}
        void        incOrphanedSrCount() {}
        std::string toString() const { return ""; }
        void        recordSrTime(uint32_t ticks) {}
# endif
    } m_stats;

    NimBLEScan();
    ~NimBLEScan();
    static int  handleGapEvent(ble_gap_event* event, void* arg);
    void        onHostSync();
    static void srTimerCb(ble_npl_event* event);

    // Linked list helpers for devices awaiting scan responses
    void addWaitingDevice(NimBLEAdvertisedDevice* pDev);
    void removeWaitingDevice(NimBLEAdvertisedDevice* pDev);
    void clearWaitingList();
    void resetWaitingTimer();

    NimBLEScanCallbacks*    m_pScanCallbacks;
    ble_gap_disc_params     m_scanParams;
    NimBLEScanResults       m_scanResults;
    NimBLETaskData*         m_pTaskData;
    ble_npl_callout         m_srTimer{};
    ble_npl_time_t          m_srTimeoutTicks{};
    uint8_t                 m_maxResults;
    NimBLEAdvertisedDevice* m_pWaitingListHead{}; // head of linked list for devices awaiting scan responses
    NimBLEAdvertisedDevice* m_pWaitingListTail{}; // tail of linked list for FIFO ordering

# if CONFIG_BT_NIMBLE_EXT_ADV
    uint8_t  m_phy{SCAN_ALL};
    uint16_t m_period{0};
# endif
};

/**
 * @brief A callback handler for callbacks associated device scanning.
 */
class NimBLEScanCallbacks {
  public:
    virtual ~NimBLEScanCallbacks() {}

    /**
     * @brief Called when a new device is discovered, before the scan result is received (if applicable).
     * @param [in] advertisedDevice The device which was discovered.
     */
    virtual void onDiscovered(const NimBLEAdvertisedDevice* advertisedDevice);

    /**
     * @brief Called when a new scan result is complete, including scan response data (if applicable).
     * @param [in] advertisedDevice The device for which the complete result is available.
     */
    virtual void onResult(const NimBLEAdvertisedDevice* advertisedDevice);

    /**
     * @brief Called when a scan operation ends.
     * @param [in] scanResults The results of the scan that ended.
     * @param [in] reason The reason code for why the scan ended.
     */
    virtual void onScanEnd(const NimBLEScanResults& scanResults, int reason);
};

#endif // CONFIG_BT_ENABLED CONFIG_BT_NIMBLE_ROLE_OBSERVER
#endif // NIMBLE_CPP_SCAN_H_
