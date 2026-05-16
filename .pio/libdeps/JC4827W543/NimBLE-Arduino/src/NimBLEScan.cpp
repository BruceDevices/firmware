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

#include "NimBLEScan.h"
#if CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_OBSERVER

# include "NimBLEDevice.h"
# include "NimBLELog.h"
# ifdef USING_NIMBLE_ARDUINO_HEADERS
#  include "nimble/porting/nimble/include/nimble/nimble_port.h"
# else
#  include "nimble/nimble_port.h"
# endif

# include <string>
# include <climits>

# define DEFAULT_SCAN_RESP_TIMEOUT_MS 10240 // max advertising interval (10.24s)

static const char*         LOG_TAG = "NimBLEScan";
static NimBLEScanCallbacks defaultScanCallbacks;

/**
 * @brief This handles an event run in the host task when the scan response timeout for the head of
 * the waiting list is triggered and directly invokes the onResult callback with the current device.
 */
void NimBLEScan::srTimerCb(ble_npl_event* event) {
    auto pScan = NimBLEDevice::getScan();
    auto pDev  = pScan->m_pWaitingListHead;

    if (pDev == nullptr) {
        ble_npl_callout_stop(&pScan->m_srTimer);
        return;
    }

    if (ble_npl_time_get() - pDev->m_time < pScan->m_srTimeoutTicks) {
        // This can happen if a scan response was received and the device was removed from the waiting list
        // after this was put in the queue. In this case, just reset the timer for this device.
        pScan->resetWaitingTimer();
        return;
    }

    NIMBLE_LOGI(LOG_TAG, "Scan response timeout for: %s", pDev->getAddress().toString().c_str());
    pScan->m_stats.incMissedSrCount();
    pScan->removeWaitingDevice(pDev);
    pDev->m_callbackSent = 2;
    pScan->m_pScanCallbacks->onResult(pDev);
    if (pScan->m_maxResults == 0) {
        pScan->erase(pDev);
    }
}

/**
 * @brief Scan constructor.
 */
NimBLEScan::NimBLEScan()
    : m_pScanCallbacks{&defaultScanCallbacks},
      // default interval + window, no whitelist scan filter,not limited scan, no scan response, filter_duplicates
      m_scanParams{0, 0, BLE_HCI_SCAN_FILT_NO_WL, 0, 1, 1},
      m_pTaskData{nullptr},
      m_maxResults{0xFF} {
    ble_npl_callout_init(&m_srTimer, nimble_port_get_dflt_eventq(), NimBLEScan::srTimerCb, nullptr);
    ble_npl_time_ms_to_ticks(DEFAULT_SCAN_RESP_TIMEOUT_MS, &m_srTimeoutTicks);
} // NimBLEScan::NimBLEScan

/**
 * @brief Scan destructor, release any allocated resources.
 */
NimBLEScan::~NimBLEScan() {
    ble_npl_callout_deinit(&m_srTimer);

    for (const auto& dev : m_scanResults.m_deviceVec) {
        delete dev;
    }
}

/**
 * @brief Add a device to the waiting list for scan responses.
 * @param [in] pDev The device to add to the list.
 */
void NimBLEScan::addWaitingDevice(NimBLEAdvertisedDevice* pDev) {
    if (pDev == nullptr) {
        return;
    }

    ble_npl_hw_enter_critical();

    // Self-pointer is the "not in list" sentinel; anything else means already in list.
    if (pDev->m_pNextWaiting != pDev) {
        ble_npl_hw_exit_critical(0);
        return;
    }

    // Initialize link field before inserting into the list.
    pDev->m_pNextWaiting = nullptr;
    if (m_pWaitingListTail == nullptr) {
        m_pWaitingListHead = pDev;
        m_pWaitingListTail = pDev;
        ble_npl_hw_exit_critical(0);
        return;
    }

    m_pWaitingListTail->m_pNextWaiting = pDev;
    m_pWaitingListTail                 = pDev;
    ble_npl_hw_exit_critical(0);
}

/**
 * @brief Remove a device from the waiting list.
 * @param [in] pDev The device to remove from the list.
 */
void NimBLEScan::removeWaitingDevice(NimBLEAdvertisedDevice* pDev) {
    if (pDev == nullptr) {
        return;
    }

    if (pDev->m_pNextWaiting == pDev) {
        return; // Not in the list
    }

    bool resetTimer = false;
    ble_npl_hw_enter_critical();
    if (m_pWaitingListHead == pDev) {
        m_pWaitingListHead = pDev->m_pNextWaiting;
        if (m_pWaitingListHead == nullptr) {
            m_pWaitingListTail = nullptr;
        } else {
            resetTimer = true;
        }
    } else {
        NimBLEAdvertisedDevice* current = m_pWaitingListHead;
        while (current != nullptr) {
            if (current->m_pNextWaiting == pDev) {
                current->m_pNextWaiting = pDev->m_pNextWaiting;
                if (m_pWaitingListTail == pDev) {
                    m_pWaitingListTail = current;
                }
                break;
            }
            current = current->m_pNextWaiting;
        }
    }
    ble_npl_hw_exit_critical(0);
    pDev->m_pNextWaiting = pDev; // Restore sentinel: self-pointer means "not in list"
    if (resetTimer) {
        resetWaitingTimer();
    }
}

/**
 * @brief Clear all devices from the waiting list.
 */
void NimBLEScan::clearWaitingList() {
    // Stop the timer and remove any pending timeout events since we're clearing
    // the list and won't be processing any more timeouts for these devices
    ble_npl_callout_stop(&m_srTimer);
    ble_npl_hw_enter_critical();
    NimBLEAdvertisedDevice* current = m_pWaitingListHead;
    while (current != nullptr) {
        NimBLEAdvertisedDevice* next = current->m_pNextWaiting;
        current->m_pNextWaiting      = current; // Restore sentinel
        current                      = next;
    }
    m_pWaitingListHead = nullptr;
    m_pWaitingListTail = nullptr;
    ble_npl_hw_exit_critical(0);
}

/**
 * @brief Reset the timer for the next waiting device at the head of the FIFO list.
 */
void NimBLEScan::resetWaitingTimer() {
    if (m_srTimeoutTicks == 0 || m_pWaitingListHead == nullptr) {
        ble_npl_callout_stop(&m_srTimer);
        return;
    }

    ble_npl_time_t now      = ble_npl_time_get();
    ble_npl_time_t elapsed  = now - m_pWaitingListHead->m_time;
    ble_npl_time_t nextTime = elapsed >= m_srTimeoutTicks ? 1 : m_srTimeoutTicks - elapsed;
    ble_npl_callout_reset(&m_srTimer, nextTime);
}

/**
 * @brief Handle GAP events related to scans.
 * @param [in] event The event type for this event.
 * @param [in] param Parameter data for this event.
 */
int NimBLEScan::handleGapEvent(ble_gap_event* event, void* arg) {
    (void)arg;
    NimBLEScan* pScan = NimBLEDevice::getScan();

    switch (event->type) {
        case BLE_GAP_EVENT_EXT_DISC:
        case BLE_GAP_EVENT_DISC: {
            if (!pScan->isScanning()) {
                NIMBLE_LOGI(LOG_TAG, "Scan stopped, ignoring event");
                return 0;
            }

# if CONFIG_BT_NIMBLE_EXT_ADV
            const auto& disc        = event->ext_disc;
            const bool  isLegacyAdv = disc.props & BLE_HCI_ADV_LEGACY_MASK;
            const auto  event_type  = isLegacyAdv ? disc.legacy_event_type : disc.props;
# else
            const auto& disc        = event->disc;
            const bool  isLegacyAdv = true;
            const auto  event_type  = disc.event_type;
# endif
            NimBLEAddress advertisedAddress(disc.addr);

# if CONFIG_BT_NIMBLE_ROLE_CENTRAL
            // stop processing if already connected
            NimBLEClient* pClient = NimBLEDevice::getClientByPeerAddress(advertisedAddress);
            if (pClient != nullptr && pClient->isConnected()) {
                NIMBLE_LOGI(LOG_TAG, "Ignoring device: address: %s, already connected", advertisedAddress.toString().c_str());
                return 0;
            }
# endif
            NimBLEAdvertisedDevice* advertisedDevice = nullptr;

            // If we've seen this device before get a pointer to it from the vector
            for (const auto& dev : pScan->m_scanResults.m_deviceVec) {
# if CONFIG_BT_NIMBLE_EXT_ADV
                // Same address but different set ID should create a new advertised device.
                if (dev->getAddress() == advertisedAddress && dev->getSetId() == disc.sid)
# else
                if (dev->getAddress() == advertisedAddress)
# endif
                {
                    advertisedDevice = dev;
                    break;
                }
            }

            // If we haven't seen this device before; create a new instance and insert it in the vector.
            // Otherwise just update the relevant parameters of the already known device.
            if (advertisedDevice == nullptr) {
                pScan->m_stats.incDevCount();

                // Check if we have reach the scan results limit, ignore this one if so.
                // We still need to store each device when maxResults is 0 to be able to append the scan results
                if (pScan->m_maxResults > 0 && pScan->m_maxResults < 0xFF &&
                    (pScan->m_scanResults.m_deviceVec.size() >= pScan->m_maxResults)) {
                    return 0;
                }

                if (isLegacyAdv && event_type == BLE_HCI_ADV_RPT_EVTYPE_SCAN_RSP) {
                    pScan->m_stats.incOrphanedSrCount();
                    NIMBLE_LOGI(LOG_TAG, "Scan response without advertisement: %s", advertisedAddress.toString().c_str());
                }

                advertisedDevice = new NimBLEAdvertisedDevice(event, event_type);
                pScan->m_scanResults.m_deviceVec.push_back(advertisedDevice);
                advertisedDevice->m_time = ble_npl_time_get();
                NIMBLE_LOGI(LOG_TAG, "New advertiser: %s", advertisedAddress.toString().c_str());
            } else {
                advertisedDevice->update(event, event_type);
                if (isLegacyAdv) {
                    if (event_type == BLE_HCI_ADV_RPT_EVTYPE_SCAN_RSP) {
                        pScan->m_stats.recordSrTime(ble_npl_time_get() - advertisedDevice->m_time);
                        NIMBLE_LOGI(LOG_TAG, "Scan response from: %s", advertisedAddress.toString().c_str());
                        // Remove device from waiting list since we got the response
                        pScan->removeWaitingDevice(advertisedDevice);
                    } else {
                        pScan->m_stats.incDupCount();
                        NIMBLE_LOGI(LOG_TAG, "Duplicate; updated: %s", advertisedAddress.toString().c_str());
                        // Restart scan-response timeout when we see a new non-scan-response
                        // legacy advertisement during active scanning for a scannable device.
                        advertisedDevice->m_time = ble_npl_time_get();
                        // Re-add to the tail so FIFO timeout order matches advertisement order.
                        if (advertisedDevice->isScannable()) {
                            pScan->removeWaitingDevice(advertisedDevice);
                            pScan->addWaitingDevice(advertisedDevice);
                        }

                        // If we're not filtering duplicates, we need to reset the callbackSent count
                        // so that callbacks will be triggered again for this device
                        if (!pScan->m_scanParams.filter_duplicates) {
                            advertisedDevice->m_callbackSent = 0;
                        }
                    }
                }
            }

# if CONFIG_BT_NIMBLE_EXT_ADV
            if (advertisedDevice->getDataStatus() == BLE_GAP_EXT_ADV_DATA_STATUS_INCOMPLETE) {
                NIMBLE_LOGD(LOG_TAG, "EXT ADV data incomplete, waiting for more");
                return 0;
            }
# endif

            if (!advertisedDevice->m_callbackSent) {
                advertisedDevice->m_callbackSent++;
                pScan->m_pScanCallbacks->onDiscovered(advertisedDevice);
            }

            // If not active scanning or scan response is not available
            // or extended advertisement scanning, report the result to the callback now.
            if (pScan->m_scanParams.passive || !isLegacyAdv || !advertisedDevice->isScannable()) {
                advertisedDevice->m_callbackSent++;
                pScan->m_pScanCallbacks->onResult(advertisedDevice);
            } else if (isLegacyAdv && event_type == BLE_HCI_ADV_RPT_EVTYPE_SCAN_RSP) {
                advertisedDevice->m_callbackSent++;
                // got the scan response report the full data.
                pScan->m_pScanCallbacks->onResult(advertisedDevice);
            } else if (isLegacyAdv && advertisedDevice->isScannable()) {
                // Add to waiting list for scan response and start the timer
                pScan->addWaitingDevice(advertisedDevice);
                if (pScan->m_pWaitingListHead == advertisedDevice) {
                    pScan->resetWaitingTimer();
                }
            }

            // If not storing results and we have invoked the callback, delete the device.
            if (pScan->m_maxResults == 0 && advertisedDevice->m_callbackSent >= 2) {
                pScan->erase(advertisedDevice);
            }

            return 0;
        }

        case BLE_GAP_EVENT_DISC_COMPLETE: {
            ble_npl_callout_stop(&pScan->m_srTimer);

            // If we have any scannable devices that haven't received a scan response,
            // we should trigger the callback with whatever data we have since the scan is complete
            // and we won't be getting any more updates for these devices.
            while (pScan->m_pWaitingListHead != nullptr) {
                auto pDev = pScan->m_pWaitingListHead;
                pScan->m_stats.incMissedSrCount();
                pScan->removeWaitingDevice(pDev);
                pDev->m_callbackSent = 2;
                pScan->m_pScanCallbacks->onResult(pDev);
            }

            if (pScan->m_maxResults == 0) {
                pScan->clearResults();
            }

            NIMBLE_LOGD(LOG_TAG, "discovery complete; reason=%d", event->disc_complete.reason);
            NIMBLE_LOGD(LOG_TAG, "%s", pScan->getStatsString().c_str());

            pScan->m_pScanCallbacks->onScanEnd(pScan->m_scanResults, event->disc_complete.reason);

            if (pScan->m_pTaskData != nullptr) {
                NimBLEUtils::taskRelease(*pScan->m_pTaskData, event->disc_complete.reason);
            }

            return 0;
        }

        default:
            return 0;
    }
} // handleGapEvent

/**
 * @brief Set the scan response timeout.
 * @param [in] timeoutMs The timeout in milliseconds to wait for a scan response, default: max advertising interval (10.24s)
 * @details If a scan response is not received within the timeout period,
 * the pending device will be reported to the scan result callback with whatever
 * data was present in the advertisement; no synthetic scan-response event is generated.
 * If set to 0, the scan result callback will only be triggered when a scan response
 * is received from the advertiser or when the scan completes, at which point any
 * pending scannable devices will be reported with the advertisement data only.
 */
void NimBLEScan::setScanResponseTimeout(uint32_t timeoutMs) {
    if (timeoutMs == 0) {
        ble_npl_callout_stop(&m_srTimer);
        m_srTimeoutTicks = 0;
        return;
    }

    ble_npl_time_ms_to_ticks(timeoutMs, &m_srTimeoutTicks);
    resetWaitingTimer();
} // setScanResponseTimeout

/**
 * @brief Should we perform an active or passive scan?
 * The default is a passive scan. An active scan means that we will request a scan response.
 * @param [in] active If true, we perform an active scan otherwise a passive scan.
 */
void NimBLEScan::setActiveScan(bool active) {
    m_scanParams.passive = !active;
} // setActiveScan

/**
 * @brief Set whether or not the BLE controller should only report results
 * from devices it has not already seen.
 * @param [in] enabled If set to 1 (true), scanned devices will only be reported once.
 * If set to 0 duplicates will be reported each time they are seen.
 * If using extended scanning this can be set to 2 which will reset the duplicate filter
 * at the end of each scan period if the scan period is set.
 * @note The controller has a limited buffer and will start reporting
duplicate devices once the limit is reached.
 */
void NimBLEScan::setDuplicateFilter(uint8_t enabled) {
    m_scanParams.filter_duplicates = enabled;
} // setDuplicateFilter

/**
 * @brief Set whether or not the BLE controller only reports scan results
 * from devices advertising in limited discovery mode.
 * @param [in] enabled If true, only limited discovery devices will be in scan results.
 */
void NimBLEScan::setLimitedOnly(bool enabled) {
    m_scanParams.limited = enabled;
} // setLimitedOnly

/**
 * @brief Sets the scan filter policy.
 * @param [in] filter Can be one of:
 * * BLE_HCI_SCAN_FILT_NO_WL             (0)
 *      Scanner processes all advertising packets (white list not used) except\n
 *      directed, connectable advertising packets not sent to the scanner.
 * * BLE_HCI_SCAN_FILT_USE_WL            (1)
 *      Scanner processes advertisements from white list only. A connectable,\n
 *      directed advertisement is ignored unless it contains scanners address.
 * * BLE_HCI_SCAN_FILT_NO_WL_INITA       (2)
 *      Scanner process all advertising packets (white list not used). A\n
 *      connectable, directed advertisement shall not be ignored if the InitA
 *      is a resolvable private address.
 * * BLE_HCI_SCAN_FILT_USE_WL_INITA      (3)
 *      Scanner process advertisements from white list only. A connectable,\n
 *      directed advertisement shall not be ignored if the InitA is a
 *      resolvable private address.
 */
void NimBLEScan::setFilterPolicy(uint8_t filter) {
    m_scanParams.filter_policy = filter;
} // setFilterPolicy

/**
 * @brief Sets the max number of results to store.
 * @param [in] maxResults The number of results to limit storage to\n
 * 0 == none (callbacks only) 0xFF == unlimited, any other value is the limit.
 */
void NimBLEScan::setMaxResults(uint8_t maxResults) {
    m_maxResults = maxResults;
} // setMaxResults

/**
 * @brief Set the call backs to be invoked.
 * @param [in] pScanCallbacks Call backs to be invoked.
 * @param [in] wantDuplicates  True if we wish to be called back with duplicates, default: false.
 */
void NimBLEScan::setScanCallbacks(NimBLEScanCallbacks* pScanCallbacks, bool wantDuplicates) {
    setDuplicateFilter(!wantDuplicates);
    if (pScanCallbacks == nullptr) {
        m_pScanCallbacks = &defaultScanCallbacks;
        return;
    }
    m_pScanCallbacks = pScanCallbacks;
} // setScanCallbacks

/**
 * @brief Set the interval to scan.
 * @param [in] intervalMs The scan interval in milliseconds.
 * @details The interval is the time between the start of two consecutive scan windows.
 * When a new interval starts the controller changes the channel it's scanning on.
 */
void NimBLEScan::setInterval(uint16_t intervalMs) {
    m_scanParams.itvl = (intervalMs * 16) / 10;
} // setInterval

/**
 * @brief Set the window to actively scan.
 * @param [in] windowMs How long during the interval to actively scan in milliseconds.
 */
void NimBLEScan::setWindow(uint16_t windowMs) {
    m_scanParams.window = (windowMs * 16) / 10;
} // setWindow

/**
 * @brief Get the status of the scanner.
 * @return true if scanning or scan starting.
 */
bool NimBLEScan::isScanning() {
    return ble_gap_disc_active();
}

# if CONFIG_BT_NIMBLE_EXT_ADV
/**
 * @brief Set the PHYs to scan.
 * @param [in] phyMask The PHYs to scan, a bit mask of:
 * * NIMBLE_CPP_SCAN_1M
 * * NIMBLE_CPP_SCAN_CODED
 */
void NimBLEScan::setPhy(Phy phyMask) {
    m_phy = phyMask;
} // setScanPhy

/**
 * @brief Set the extended scanning period.
 * @param [in] periodMs The scan period in milliseconds
 * @details The period is the time between the start of two consecutive scan periods.
 * This works as a timer to restart scanning at the specified amount of time in periodMs.
 * @note The duration used when this is set must be less than period.
 */
void NimBLEScan::setPeriod(uint32_t periodMs) {
    m_period = (periodMs + 500) / 1280; // round up 1.28 second units
} // setScanPeriod
# endif

/**
 * @brief Start scanning.
 * @param [in] duration The duration in milliseconds for which to scan. 0 == scan forever.
 * @param [in] isContinue Set to true to save previous scan results, false to clear them.
 * @param [in] restart Set to true to restart the scan if already in progress.
 * this is useful to clear the duplicate filter so all devices are reported again.
 * @return True if scan started or false if there was an error.
 */
bool NimBLEScan::start(uint32_t duration, bool isContinue, bool restart) {
    NIMBLE_LOGD(LOG_TAG, ">> start: duration=%" PRIu32, duration);
    if (isScanning()) {
        if (restart) {
            NIMBLE_LOGI(LOG_TAG, "Scan already in progress, restarting it");
            if (!stop()) {
                return false;
            }

            if (!isContinue) {
                clearResults();
                m_stats.reset();
            }
        }
    } else { // Don't clear results while scanning is active
        if (!isContinue) {
            clearResults();
            m_stats.reset();
        }
    }

    // If scanning is already active, call the functions anyway as the parameters can be changed.

# if CONFIG_BT_NIMBLE_EXT_ADV
    ble_gap_ext_disc_params scan_params;
    scan_params.passive = m_scanParams.passive;
    scan_params.itvl    = m_scanParams.itvl;
    scan_params.window  = m_scanParams.window;
    int rc              = ble_gap_ext_disc(NimBLEDevice::m_ownAddrType,
                                           duration / 10, // 10ms units
                                           m_period,
                                           m_scanParams.filter_duplicates,
                                           m_scanParams.filter_policy,
                                           m_scanParams.limited,
                                           m_phy & SCAN_1M ? &scan_params : NULL,
                                           m_phy & SCAN_CODED ? &scan_params : NULL,
                                           NimBLEScan::handleGapEvent,
                                           NULL);
# else
    int rc = ble_gap_disc(NimBLEDevice::m_ownAddrType,
                          duration ? duration : BLE_HS_FOREVER,
                          &m_scanParams,
                          NimBLEScan::handleGapEvent,
                          NULL);
# endif
    switch (rc) {
        case 0:
        case BLE_HS_EALREADY:
            NIMBLE_LOGD(LOG_TAG, "Scan started");
            break;

        case BLE_HS_EBUSY:
            NIMBLE_LOGE(LOG_TAG, "Unable to scan - connection in progress.");
            break;

        case BLE_HS_ETIMEOUT_HCI:
        case BLE_HS_EOS:
        case BLE_HS_ECONTROLLER:
        case BLE_HS_ENOTSYNCED:
            NIMBLE_LOGE(LOG_TAG, "Unable to scan - Host Reset");
            break;

        default:
            NIMBLE_LOGE(LOG_TAG, "Error starting scan; rc=%d, %s", rc, NimBLEUtils::returnCodeToString(rc));
            break;
    }

    NIMBLE_LOGD(LOG_TAG, "<< start()");
    return rc == 0 || rc == BLE_HS_EALREADY;
} // start

/**
 * @brief Stop an in progress scan.
 * @return True if successful.
 */
bool NimBLEScan::stop() {
    NIMBLE_LOGD(LOG_TAG, ">> stop()");

    int rc = ble_gap_disc_cancel();
    if (rc != 0 && rc != BLE_HS_EALREADY) {
        NIMBLE_LOGE(LOG_TAG, "Failed to cancel scan; rc=%d", rc);
        return false;
    }

    clearWaitingList();

    if (m_maxResults == 0) {
        clearResults();
    }

    if (m_pTaskData != nullptr) {
        NimBLEUtils::taskRelease(*m_pTaskData);
    }

    NIMBLE_LOGD(LOG_TAG, "<< stop()");
    return true;
} // stop

/**
 * @brief Delete peer device from the scan results vector.
 * @param [in] address The address of the device to delete from the results.
 */
void NimBLEScan::erase(const NimBLEAddress& address) {
    NIMBLE_LOGD(LOG_TAG, "erase device: %s", address.toString().c_str());
    for (auto it = m_scanResults.m_deviceVec.begin(); it != m_scanResults.m_deviceVec.end(); ++it) {
        if ((*it)->getAddress() == address) {
            removeWaitingDevice(*it);
            delete *it;
            m_scanResults.m_deviceVec.erase(it);
            break;
        }
    }
}

/**
 * @brief Delete peer device from the scan results vector.
 * @param [in] device The device to delete from the results.
 */
void NimBLEScan::erase(const NimBLEAdvertisedDevice* device) {
    NIMBLE_LOGD(LOG_TAG, "erase device: %s", device->getAddress().toString().c_str());
    for (auto it = m_scanResults.m_deviceVec.begin(); it != m_scanResults.m_deviceVec.end(); ++it) {
        if ((*it) == device) {
            removeWaitingDevice(*it);
            delete *it;
            m_scanResults.m_deviceVec.erase(it);
            break;
        }
    }
}

/**
 * @brief If the host reset and re-synced this is called.
 * If the application was scanning indefinitely with a callback, restart it.
 */
void NimBLEScan::onHostSync() {
    m_pScanCallbacks->onScanEnd(m_scanResults, BLE_HS_ENOTSYNCED);
}

/**
 * @brief Start scanning and block until scanning has been completed.
 * @param [in] duration The duration in milliseconds for which to scan.
 * @param [in] is_continue Set to true to save previous scan results, false to clear them.
 * @return The scan results.
 */
NimBLEScanResults NimBLEScan::getResults(uint32_t duration, bool is_continue) {
    if (duration == 0) {
        NIMBLE_LOGW(LOG_TAG, "Blocking scan called with duration = forever");
    }

    if (m_pTaskData != nullptr) {
        NIMBLE_LOGE(LOG_TAG, "Scan already in progress");
        return m_scanResults;
    }

    NimBLETaskData taskData;
    m_pTaskData = &taskData;

    if (start(duration, is_continue)) {
        NimBLEUtils::taskWait(taskData, BLE_NPL_TIME_FOREVER);
    }

    m_pTaskData = nullptr;
    return m_scanResults;
} // getResults

/**
 * @brief Get the results of the scan.
 * @return NimBLEScanResults object.
 */
NimBLEScanResults NimBLEScan::getResults() {
    return m_scanResults;
}

/**
 * @brief Clear the stored results of the scan.
 */
void NimBLEScan::clearResults() {
    if (isScanning()) {
        NIMBLE_LOGW(LOG_TAG, "Cannot clear results while scan is active");
        return;
    }

    clearWaitingList();
    if (m_scanResults.m_deviceVec.size()) {
        std::vector<NimBLEAdvertisedDevice*> vSwap{};
        ble_npl_hw_enter_critical();
        vSwap.swap(m_scanResults.m_deviceVec);
        ble_npl_hw_exit_critical(0);
        for (const auto& dev : vSwap) {
            delete dev;
        }
    }
} // clearResults

/**
 * @brief Dump the scan results to the log.
 */
void NimBLEScanResults::dump() const {
# if CONFIG_NIMBLE_CPP_LOG_LEVEL >= 3
    for (const auto& dev : m_deviceVec) {
        NIMBLE_LOGI(LOG_TAG, "- %s", dev->toString().c_str());
    }
# endif
} // dump

/**
 * @brief Get the count of devices found in the last scan.
 * @return The number of devices found in the last scan.
 */
int NimBLEScanResults::getCount() const {
    return m_deviceVec.size();
} // getCount

/**
 * @brief Return the specified device at the given index.
 * The index should be between 0 and getCount()-1.
 * @param [in] idx The index of the device.
 * @return The device at the specified index.
 */
const NimBLEAdvertisedDevice* NimBLEScanResults::getDevice(uint32_t idx) const {
    return m_deviceVec[idx];
}

/**
 * @brief Get iterator to the beginning of the vector of advertised device pointers.
 * @return An iterator to the beginning of the vector of advertised device pointers.
 */
std::vector<NimBLEAdvertisedDevice*>::const_iterator NimBLEScanResults::begin() const {
    return m_deviceVec.begin();
}

/**
 * @brief Get iterator to the end of the vector of advertised device pointers.
 * @return An iterator to the end of the vector of advertised device pointers.
 */
std::vector<NimBLEAdvertisedDevice*>::const_iterator NimBLEScanResults::end() const {
    return m_deviceVec.end();
}

/**
 * @brief Get a pointer to the specified device at the given address.
 * If the address is not found a nullptr is returned.
 * @param [in] address The address of the device.
 * @return A pointer to the device at the specified address.
 */
const NimBLEAdvertisedDevice* NimBLEScanResults::getDevice(const NimBLEAddress& address) const {
    for (const auto& dev : m_deviceVec) {
        if (dev->getAddress() == address) {
            return dev;
        }
    }

    return nullptr;
}

static const char* CB_TAG = "NimBLEScanCallbacks";

void NimBLEScanCallbacks::onDiscovered(const NimBLEAdvertisedDevice* pAdvertisedDevice) {
    NIMBLE_LOGD(CB_TAG, "Discovered: %s", pAdvertisedDevice->toString().c_str());
}

void NimBLEScanCallbacks::onResult(const NimBLEAdvertisedDevice* pAdvertisedDevice) {
    NIMBLE_LOGD(CB_TAG, "Result: %s", pAdvertisedDevice->toString().c_str());
}

void NimBLEScanCallbacks::onScanEnd(const NimBLEScanResults& results, int reason) {
    NIMBLE_LOGD(CB_TAG, "Scan ended; reason %d, num results: %d", reason, results.getCount());
}

#endif // CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_OBSERVER
