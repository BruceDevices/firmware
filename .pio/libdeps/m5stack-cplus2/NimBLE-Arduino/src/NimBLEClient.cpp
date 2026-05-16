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

#include "NimBLEClient.h"
#if CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_CENTRAL

# include "NimBLERemoteService.h"
# include "NimBLERemoteCharacteristic.h"
# include "NimBLEDevice.h"
# include "NimBLELog.h"

# ifdef USING_NIMBLE_ARDUINO_HEADERS
#  include "nimble/porting/nimble/include/nimble/nimble_port.h"
# else
#  include "nimble/nimble_port.h"
# endif

# include <climits>

static const char*           LOG_TAG = "NimBLEClient";
static NimBLEClientCallbacks defaultCallbacks;

namespace {
constexpr inline uint32_t connIntervalToMs(uint16_t interval) {
    return (static_cast<uint32_t>(interval) * 5U) / 4U;
} // connIntervalToMs
} // namespace

/*
 * Design
 * ------
 * When we perform a getService() request, we are asking the BLE server to return each of the services
 * that it exposes.  For each service, we receive a callback which contains details
 * of the exposed service including its UUID.
 *
 * The objects we will invent for a NimBLEClient will be as follows:
 * * NimBLERemoteService - A model of a remote service.
 * * NimBLERemoteCharacteristic - A model of a remote characteristic
 * * NimBLERemoteDescriptor - A model of a remote descriptor.
 *
 * Since there is a hierarchical relationship here, we will have the idea that from a NimBLERemoteService will own
 * zero or more remote characteristics and a NimBLERemoteCharacteristic will own zero or more NimBLERemoteDescriptors.
 *
 * We will assume that a NimBLERemoteService contains a vector of owned characteristics
 * and that a NimBLERemoteCharacteristic contains a vector of owned descriptors.
 */

/**
 * @brief Constructor, private - only callable by NimBLEDevice::createClient
 * to ensure proper handling of the list of client objects.
 */
NimBLEClient::NimBLEClient(const NimBLEAddress& peerAddress)
    : m_peerAddress(peerAddress),
      m_lastErr{0},
      m_connectTimeout{30000},
      m_pTaskData{nullptr},
      m_svcVec{},
      m_pClientCallbacks{&defaultCallbacks},
      m_connHandle{BLE_HS_CONN_HANDLE_NONE},
      m_terminateFailCount{0},
      m_asyncSecureAttempt{0},
      m_connStatus{DISCONNECTED},
      m_connectCallbackPending{false},
      m_connectFailRetryCount{0},
# if CONFIG_BT_NIMBLE_EXT_ADV
      m_phyMask{BLE_GAP_LE_PHY_1M_MASK | BLE_GAP_LE_PHY_2M_MASK | BLE_GAP_LE_PHY_CODED_MASK},
# endif
      m_connParams{16,
                   16,
                   BLE_GAP_INITIAL_CONN_ITVL_MIN,
                   BLE_GAP_INITIAL_CONN_ITVL_MAX,
                   BLE_GAP_INITIAL_CONN_LATENCY,
                   BLE_GAP_INITIAL_SUPERVISION_TIMEOUT,
                   BLE_GAP_INITIAL_CONN_MIN_CE_LEN,
                   BLE_GAP_INITIAL_CONN_MAX_CE_LEN} {
    ble_npl_callout_init(&m_connectEstablishedTimer,
                         nimble_port_get_dflt_eventq(),
                         NimBLEClient::connectEstablishedTimerCb,
                         this);
} // NimBLEClient

/**
 * @brief Destructor, private - only callable by NimBLEDevice::deleteClient
 * to ensure proper disconnect and removal from device list.
 */
NimBLEClient::~NimBLEClient() {
    ble_npl_callout_stop(&m_connectEstablishedTimer);
    ble_npl_callout_deinit(&m_connectEstablishedTimer);

    // We may have allocated service references associated with this client.
    // Before we are finished with the client, we must release resources.
    deleteServices();

    if (m_config.deleteCallbacks) {
        delete m_pClientCallbacks;
    }
} // ~NimBLEClient

/**
 * @brief Delete all service objects created by this client and clear the vector.
 */
void NimBLEClient::deleteServices() {
    // Delete all the services.
    for (auto& it : m_svcVec) {
        delete it;
    }

    std::vector<NimBLERemoteService*>().swap(m_svcVec);
} // deleteServices

/**
 * @brief Delete a service by UUID from the local database to free resources.
 * @param [in] uuid The UUID of the service to be deleted.
 * @return Number of services left.
 */
size_t NimBLEClient::deleteService(const NimBLEUUID& uuid) {
    // Delete the requested service.
    for (auto it = m_svcVec.begin(); it != m_svcVec.end(); ++it) {
        if ((*it)->getUUID() == uuid) {
            delete *it;
            m_svcVec.erase(it);
            break;
        }
    }

    return m_svcVec.size();
} // deleteService

# if CONFIG_BT_NIMBLE_ROLE_OBSERVER
/**
 * @brief Connect to an advertising device.
 * @param [in] pDevice A pointer to the advertised device instance to connect to.
 * @param [in] deleteAttributes If true this will delete any attribute objects this client may already\n
 * have created when last connected.
 * @param [in] asyncConnect If true, the connection will be made asynchronously and this function will return immediately.\n
 * If false, this function will block until the connection is established or the connection attempt times out.
 * @param [in] exchangeMTU If true, the client will attempt to exchange MTU with the server after connection.\n
 * If false, the client will use the default MTU size and the application will need to call exchangeMTU() later.
 * @return true on success.
 */
bool NimBLEClient::connect(const NimBLEAdvertisedDevice* pDevice, bool deleteAttributes, bool asyncConnect, bool exchangeMTU) {
    NimBLEAddress address(pDevice->getAddress());
    return connect(address, deleteAttributes, asyncConnect, exchangeMTU);
} // connect
# endif

/**
 * @brief Connect to the BLE Server using the address of the last connected device, or the address\n
 * passed to the constructor.
 * @param [in] deleteAttributes If true this will delete any attribute objects this client may already\n
 * have created when last connected.
 * @param [in] asyncConnect If true, the connection will be made asynchronously and this function will return immediately.\n
 * If false, this function will block until the connection is established or the connection attempt times out.
 * @param [in] exchangeMTU If true, the client will attempt to exchange MTU with the server after connection.\n
 * If false, the client will use the default MTU size and the application will need to call exchangeMTU() later.
 * @return true on success.
 */
bool NimBLEClient::connect(bool deleteAttributes, bool asyncConnect, bool exchangeMTU) {
    return connect(m_peerAddress, deleteAttributes, asyncConnect, exchangeMTU);
} // connect

int NimBLEClient::startConnectionAttempt(const ble_addr_t* peerAddr) {
    int rc = 0;

    do {
# if CONFIG_BT_NIMBLE_EXT_ADV
        rc = ble_gap_ext_connect(NimBLEDevice::m_ownAddrType,
                                 peerAddr,
                                 m_connectTimeout,
                                 m_phyMask,
                                 &m_connParams,
                                 &m_connParams,
                                 &m_connParams,
                                 NimBLEClient::handleGapEvent,
                                 this);

# else
        rc = ble_gap_connect(NimBLEDevice::m_ownAddrType,
                             peerAddr,
                             m_connectTimeout,
                             &m_connParams,
                             NimBLEClient::handleGapEvent,
                             this);
# endif
        switch (rc) {
            case 0:
                break;

            case BLE_HS_EBUSY:
# if CONFIG_BT_NIMBLE_ROLE_OBSERVER

                // Scan was active, stop it through the NimBLEScan API to release any tasks and call the callback.
                if (!NimBLEDevice::getScan()->stop()) {
                    rc = BLE_HS_EUNKNOWN;
                }
# else
                rc = BLE_HS_EUNKNOWN;
# endif
                break;

            case BLE_HS_EDONE:
                // A connection to this device already exists, do not connect twice.
                NIMBLE_LOGE(LOG_TAG, "Already connected to device; addr=%s", std::string(m_peerAddress).c_str());
                break;

            case BLE_HS_EALREADY:
                NIMBLE_LOGE(LOG_TAG, "Already attempting to connect");
                break;

            default:
                NIMBLE_LOGE(LOG_TAG,
                            "Failed to connect to %s, rc=%d; %s",
                            std::string(m_peerAddress).c_str(),
                            rc,
                            NimBLEUtils::returnCodeToString(rc));
                break;
        }

    } while (rc == BLE_HS_EBUSY);

    return rc;
}

/**
 * @brief Connect to a BLE Server by address.
 * @param [in] address The address of the server.
 * @param [in] deleteAttributes If true this will delete any attribute objects this client may already\n
 * have created when last connected.
 * @param [in] asyncConnect If true, the connection will be made asynchronously and this function will return immediately.\n
 * If false, this function will block until the connection is established or the connection attempt times out.
 * @param [in] exchangeMTU If true, the client will attempt to exchange MTU with the server after connection.\n
 * If false, the client will use the default MTU size and the application will need to call exchangeMTU() later.
 * @return true on success.
 */
bool NimBLEClient::connect(const NimBLEAddress& address, bool deleteAttributes, bool asyncConnect, bool exchangeMTU) {
    NIMBLE_LOGD(LOG_TAG, ">> connect(%s)", address.toString().c_str());
    NimBLETaskData    taskData(this);
    const ble_addr_t* peerAddr = address.getBase();
    int               rc       = 0;

    if (!NimBLEDevice::m_synced) {
        NIMBLE_LOGE(LOG_TAG, "Host not synced with controller.");
        rc = BLE_HS_ENOTSYNCED;
        goto error;
    }

    if (m_connStatus != DISCONNECTED) {
        NIMBLE_LOGE(LOG_TAG, "Client not disconnected, cannot connect");
        rc = BLE_HS_EREJECT;
        goto error;
    }

    if (address.isNull()) {
        NIMBLE_LOGE(LOG_TAG, "Invalid peer address; (NULL)");
        rc = BLE_HS_EINVAL;
        goto error;
    }

    m_connStatus             = CONNECTING;
    m_peerAddress            = address;
    m_config.asyncConnect    = asyncConnect;
    m_config.exchangeMTU     = exchangeMTU;
    m_connectCallbackPending = false;
    m_connectFailRetryCount  = 0;

    rc = startConnectionAttempt(peerAddr);

    if (deleteAttributes) {
        deleteServices();
    }

    if (rc != 0) {
        goto error;
    }

    if (m_config.asyncConnect) {
        return true;
    }

    m_pTaskData = &taskData;

    // Wait for the connect timeout time +retry time * retries for the connection to complete
    if (!NimBLEUtils::taskWait(
            taskData,
            (m_connectTimeout + connIntervalToMs(m_connParams.itvl_max) * 7) * (m_config.connectFailRetries + 1U))) {
        if (m_connStatus != CONNECTED) {
            // if the controller doesn't cancel the connection at the timeout, cancel it here.
            NIMBLE_LOGE(LOG_TAG, "Connect timeout - cancelling");
            ble_gap_conn_cancel();
            taskData.m_flags = BLE_HS_ETIMEOUT;
        }
    }

    m_pTaskData = nullptr;
    rc          = taskData.m_flags;
    if (rc != 0) {
        NIMBLE_LOGE(LOG_TAG, "Connection failed; status=%d %s", rc, NimBLEUtils::returnCodeToString(rc));
        goto error;
    }

    NIMBLE_LOGD(LOG_TAG, "<< connect()");
    return true;

error:
    m_connStatus = DISCONNECTED;
    m_lastErr    = rc;
    if (m_config.deleteOnConnectFail) {
        NimBLEDevice::deleteClient(this);
    }
    return false;
} // connect

/**
 * @brief Initiate a secure connection (pair/bond) with the server.\n
 * Called automatically when a characteristic or descriptor requires encryption or authentication to access it.
 * @param [in] async If true, the connection will be secured asynchronously and this function will return immediately.\n
 * If false, this function will block until the connection is secured or the client disconnects.
 * @return True on success.
 * @details If async=false, this function will block and should not be used in a callback.
 */
bool NimBLEClient::secureConnection(bool async) const {
    NIMBLE_LOGD(LOG_TAG, ">> secureConnection()");

    int rc = 0;
    if (async && !NimBLEDevice::startSecurity(m_connHandle, &rc)) {
        m_lastErr            = rc;
        m_asyncSecureAttempt = 0;
        return false;
    }

    if (async) {
        m_asyncSecureAttempt++;
        return true;
    }

    NimBLETaskData taskData(const_cast<NimBLEClient*>(this), BLE_HS_ENOTCONN);
    m_pTaskData    = &taskData;
    int retryCount = 1;
    do {
        if (NimBLEDevice::startSecurity(m_connHandle)) {
            NimBLEUtils::taskWait(taskData, BLE_NPL_TIME_FOREVER);
        }
    } while (taskData.m_flags == BLE_HS_HCI_ERR(BLE_ERR_PINKEY_MISSING) && retryCount--);

    m_pTaskData = nullptr;

    if (taskData.m_flags == 0) {
        NIMBLE_LOGD(LOG_TAG, "<< secureConnection: success");
        return true;
    }

    m_lastErr = taskData.m_flags;
    NIMBLE_LOGE(LOG_TAG,
                "secureConnection: failed rc=%d %s",
                taskData.m_flags,
                NimBLEUtils::returnCodeToString(taskData.m_flags));
    return false;

} // secureConnection

/**
 * @brief Disconnect from the peer.
 * @return True if the command was successfully sent.
 */
bool NimBLEClient::disconnect(uint8_t reason) {
    int rc = ble_gap_terminate(m_connHandle, reason);
    switch (rc) {
        case 0:
            m_connStatus = DISCONNECTING;
            return true;
        case BLE_HS_ENOTCONN:
        case BLE_HS_EALREADY:
        case BLE_HS_HCI_ERR(BLE_ERR_UNK_CONN_ID): // should not happen but just in case
            return true;
    }

    NIMBLE_LOGE(LOG_TAG, "ble_gap_terminate failed: rc=%d %s", rc, NimBLEUtils::returnCodeToString(rc));
    m_lastErr = rc;
    return false;
} // disconnect

/**
 * @brief Cancel an ongoing connection attempt.
 * @return True if the command was successfully sent.
 */
bool NimBLEClient::cancelConnect() const {
    int rc = ble_gap_conn_cancel();
    if (rc != 0 && rc != BLE_HS_EALREADY) {
        NIMBLE_LOGE(LOG_TAG, "ble_gap_conn_cancel failed: rc=%d %s", rc, NimBLEUtils::returnCodeToString(rc));
        m_lastErr = rc;
        return false;
    }

    return true;
} // cancelConnect

/**
 * @brief Set or unset a flag to delete this client when disconnected or connection failed.
 * @param [in] deleteOnDisconnect Set the client to self delete when disconnected.
 * @param [in] deleteOnConnectFail Set the client to self delete when a connection attempt fails.
 */
void NimBLEClient::setSelfDelete(bool deleteOnDisconnect, bool deleteOnConnectFail) {
    m_config.deleteOnDisconnect  = deleteOnDisconnect;
    m_config.deleteOnConnectFail = deleteOnConnectFail;
} // setSelfDelete

/**
 * @brief Get a copy of the clients configuration.
 * @return A copy of the clients configuration.
 */
NimBLEClient::Config NimBLEClient::getConfig() const {
    return m_config;
} // getConfig

/**
 * @brief Set the client configuration options.
 * @param [in] config The config options instance to set the client configuration to.
 */
void NimBLEClient::setConfig(NimBLEClient::Config config) {
    m_config = config;
} // setConfig

# if CONFIG_BT_NIMBLE_EXT_ADV
/**
 * @brief Set the PHY types to use when connecting to a server.
 * @param [in] mask A bitmask indicating what PHYS to connect with.\n
 * The available bits are:
 * * 0x01 BLE_GAP_LE_PHY_1M_MASK
 * * 0x02 BLE_GAP_LE_PHY_2M_MASK
 * * 0x04 BLE_GAP_LE_PHY_CODED_MASK
 */
void NimBLEClient::setConnectPhy(uint8_t mask) {
    m_phyMask = mask;
} // setConnectPhy
# endif

/**
 * @brief Request a change to the PHY used for this peer connection.
 * @param [in] txPhyMask TX PHY. Can be mask of following:
 * - BLE_GAP_LE_PHY_1M_MASK
 * - BLE_GAP_LE_PHY_2M_MASK
 * - BLE_GAP_LE_PHY_CODED_MASK
 * - BLE_GAP_LE_PHY_ANY_MASK
 * @param [in] rxPhyMask RX PHY. Can be mask of following:
 * - BLE_GAP_LE_PHY_1M_MASK
 * - BLE_GAP_LE_PHY_2M_MASK
 * - BLE_GAP_LE_PHY_CODED_MASK
 * - BLE_GAP_LE_PHY_ANY_MASK
 * @param phyOptions Additional PHY options. Valid values are:
 * - BLE_GAP_LE_PHY_CODED_ANY (default)
 * - BLE_GAP_LE_PHY_CODED_S2
 * - BLE_GAP_LE_PHY_CODED_S8
 * @return True if successful.
 */
bool NimBLEClient::updatePhy(uint8_t txPhyMask, uint8_t rxPhyMask, uint16_t phyOptions) {
    int rc = ble_gap_set_prefered_le_phy(m_connHandle, txPhyMask, rxPhyMask, phyOptions);
    if (rc != 0) {
        NIMBLE_LOGE(LOG_TAG, "Failed to update phy; rc=%d %s", rc, NimBLEUtils::returnCodeToString(rc));
    }

    return rc == 0;
} // updatePhy

/**
 * @brief Get the PHY used for this peer connection.
 * @param [out] txPhy The TX PHY.
 * @param [out] rxPhy The RX PHY.
 * @return True if successful.
 */
bool NimBLEClient::getPhy(uint8_t* txPhy, uint8_t* rxPhy) {
    int rc = ble_gap_read_le_phy(m_connHandle, txPhy, rxPhy);
    if (rc != 0) {
        NIMBLE_LOGE(LOG_TAG, "Failed to read phy; rc=%d %s", rc, NimBLEUtils::returnCodeToString(rc));
    }

    return rc == 0;
} // getPhy

/**
 * @brief Set the connection parameters to use when connecting to a server.
 * @param [in] minInterval The minimum connection interval in 1.25ms units.
 * @param [in] maxInterval The maximum connection interval in 1.25ms units.
 * @param [in] latency The number of packets allowed to skip (extends max interval).
 * @param [in] timeout The timeout time in 10ms units before disconnecting.
 * @param [in] scanInterval The scan interval to use when attempting to connect in 0.625ms units.
 * @param [in] scanWindow The scan window to use when attempting to connect in 0.625ms units.
 */
void NimBLEClient::setConnectionParams(
    uint16_t minInterval, uint16_t maxInterval, uint16_t latency, uint16_t timeout, uint16_t scanInterval, uint16_t scanWindow)
/*, uint16_t minConnEvtTime, uint16_t maxConnEvtTime)*/
{
    m_connParams.itvl_min            = minInterval;
    m_connParams.itvl_max            = maxInterval;
    m_connParams.latency             = latency;
    m_connParams.supervision_timeout = timeout;
    m_connParams.scan_itvl           = scanInterval;
    m_connParams.scan_window         = scanWindow;

    // These are not used by NimBLE at this time - Must leave at defaults
    // m_connParams.min_ce_len = minConnEvtTime;     // Minimum length of connection event in 0.625ms units
    // m_connParams.max_ce_len = maxConnEvtTime;     // Maximum length of connection event in 0.625ms units
} // setConnectionParams

/**
 * @brief Update the connection parameters:
 * * Can only be used after a connection has been established.
 * @param [in] minInterval The minimum connection interval in 1.25ms units.
 * @param [in] maxInterval The maximum connection interval in 1.25ms units.
 * @param [in] latency The number of packets allowed to skip (extends max interval).
 * @param [in] timeout The timeout time in 10ms units before disconnecting.
 */
bool NimBLEClient::updateConnParams(uint16_t minInterval, uint16_t maxInterval, uint16_t latency, uint16_t timeout) {
    ble_gap_upd_params params{.itvl_min            = minInterval,
                              .itvl_max            = maxInterval,
                              .latency             = latency,
                              .supervision_timeout = timeout,
                              // These are not used by NimBLE at this time - leave at defaults
                              .min_ce_len          = BLE_GAP_INITIAL_CONN_MIN_CE_LEN,
                              .max_ce_len          = BLE_GAP_INITIAL_CONN_MAX_CE_LEN};

    int rc = ble_gap_update_params(m_connHandle, &params);
    if (rc != 0) {
        NIMBLE_LOGE(LOG_TAG, "Update params error: %d, %s", rc, NimBLEUtils::returnCodeToString(rc));
        m_lastErr = rc;
    }

    return rc == 0;
} // updateConnParams

/**
 * @brief Request an update of the data packet length.
 * * Can only be used after a connection has been established.
 * @details Sends a data length update request to the server the client is connected to.
 * The Data Length Extension (DLE) allows to increase the Data Channel Payload from 27 bytes to up to 251 bytes.
 * The server needs to support the Bluetooth 4.2 specifications, to be capable of DLE.
 * @param [in] txOctets The preferred number of payload octets to use (Range 0x001B-0x00FB).
 */
bool NimBLEClient::setDataLen(uint16_t txOctets) {
# if !defined(USING_NIMBLE_ARDUINO_HEADERS) && !defined(ESP_IDF_VERSION) || \
     (ESP_IDF_VERSION_MAJOR * 100 + ESP_IDF_VERSION_MINOR * 10 + ESP_IDF_VERSION_PATCH) < 432
    return false;
# else
    uint16_t txTime = (txOctets + 14) * 8;
    int      rc     = ble_gap_set_data_len(m_connHandle, txOctets, txTime);
    if (rc != 0) {
        NIMBLE_LOGE(LOG_TAG, "Set data length error: %d, %s", rc, NimBLEUtils::returnCodeToString(rc));
    }

    return rc == 0;
# endif
} // setDataLen

/**
 * @brief Get detailed information about the current peer connection.
 * @return A NimBLEConnInfo instance with the data, or a NULL instance if not found.
 */
NimBLEConnInfo NimBLEClient::getConnInfo() const {
    NimBLEConnInfo connInfo{};
    if (ble_gap_conn_find(m_connHandle, &connInfo.m_desc) != 0) {
        NIMBLE_LOGE(LOG_TAG, "Connection info not found");
    }

    return connInfo;
} // getConnInfo

/**
 * @brief Set the timeout to wait for connection attempt to complete.
 * @param [in] time The number of milliseconds before timeout, default is 30 seconds.
 */
void NimBLEClient::setConnectTimeout(uint32_t time) {
    m_connectTimeout = time;
} // setConnectTimeout

/**
 * @brief Get the connection handle for this client.
 * @return The connection handle.
 */
uint16_t NimBLEClient::getConnHandle() const {
    return m_connHandle;
} // getConnHandle

/**
 * @brief Retrieve the address of the peer.
 * @return A NimBLEAddress instance with the peer address data.
 */
NimBLEAddress NimBLEClient::getPeerAddress() const {
    return m_peerAddress;
} // getPeerAddress

/**
 * @brief Set the peer address.
 * @param [in] address The address of the peer that this client is connected or should connect to.
 * @return True if successful.
 */
bool NimBLEClient::setPeerAddress(const NimBLEAddress& address) {
    if (m_connStatus == CONNECTED || m_connStatus == CONNECTING) {
        NIMBLE_LOGE(LOG_TAG, "Cannot set peer address while connected/connecting");
        return false;
    }

    m_peerAddress = address;
    return true;
} // setPeerAddress

/**
 * @brief Ask the BLE server for the RSSI value.
 * @return The RSSI value or 0 if there was an error.
 */
int NimBLEClient::getRssi() const {
    if (m_connStatus != CONNECTED) {
        NIMBLE_LOGE(LOG_TAG, "getRssi(): Not connected");
        return 0;
    }

    int8_t rssi = 0;
    int    rc   = ble_gap_conn_rssi(m_connHandle, &rssi);
    if (rc != 0) {
        NIMBLE_LOGE(LOG_TAG, "Failed to read RSSI error code: %d, %s", rc, NimBLEUtils::returnCodeToString(rc));
        m_lastErr = rc;
        return 0;
    }

    return rssi;
} // getRssi

/**
 * @brief Get iterator to the beginning of the vector of remote service pointers.
 * @return An iterator to the beginning of the vector of remote service pointers.
 */
std::vector<NimBLERemoteService*>::iterator NimBLEClient::begin() {
    return m_svcVec.begin();
} // begin

/**
 * @brief Get iterator to the end of the vector of remote service pointers.
 * @return An iterator to the end of the vector of remote service pointers.
 */
std::vector<NimBLERemoteService*>::iterator NimBLEClient::end() {
    return m_svcVec.end();
} // end

/**
 * @brief Get the service BLE Remote Service instance corresponding to the uuid.
 * @param [in] uuid The UUID of the service being sought.
 * @return A pointer to the service or nullptr if not found.
 */
NimBLERemoteService* NimBLEClient::getService(const char* uuid) {
    return getService(NimBLEUUID(uuid));
} // getService

/**
 * @brief Get the service object corresponding to the uuid.
 * @param [in] uuid The UUID of the service being sought.
 * @return A pointer to the service or nullptr if not found.
 */
NimBLERemoteService* NimBLEClient::getService(const NimBLEUUID& uuid) {
    NIMBLE_LOGD(LOG_TAG, ">> getService: uuid: %s", uuid.toString().c_str());

    for (auto& it : m_svcVec) {
        if (it->getUUID() == uuid) {
            NIMBLE_LOGD(LOG_TAG, "<< getService: found the service with uuid: %s", uuid.toString().c_str());
            return it;
        }
    }

    size_t prevSize = m_svcVec.size();
    if (retrieveServices(&uuid)) {
        if (m_svcVec.size() > prevSize) {
            return m_svcVec.back();
        }

        // If the request was successful but 16/32 bit uuid not found
        // try again with the 128 bit uuid.
        if (uuid.bitSize() == BLE_UUID_TYPE_16 || uuid.bitSize() == BLE_UUID_TYPE_32) {
            NimBLEUUID uuid128(uuid);
            uuid128.to128();
            if (retrieveServices(&uuid128)) {
                if (m_svcVec.size() > prevSize) {
                    return m_svcVec.back();
                }
            }
        } else {
            // If the request was successful but the 128 bit uuid not found
            // try again with the 16 bit uuid.
            NimBLEUUID uuid16(uuid);
            uuid16.to16();
            // if the uuid was 128 bit but not of the BLE base type this check will fail
            if (uuid16.bitSize() == BLE_UUID_TYPE_16) {
                if (retrieveServices(&uuid16)) {
                    if (m_svcVec.size() > prevSize) {
                        return m_svcVec.back();
                    }
                }
            }
        }
    }

    NIMBLE_LOGD(LOG_TAG, "<< getService: not found");
    return nullptr;
} // getService

/**
 * @brief Get a pointer to the vector of found services.
 * @param [in] refresh If true the current services vector will be cleared and\n
 * all services will be retrieved from the peripheral.\n
 * If false the vector will be returned with the currently stored services.
 * @return A pointer to the vector of available services.
 */
const std::vector<NimBLERemoteService*>& NimBLEClient::getServices(bool refresh) {
    if (refresh) {
        deleteServices();
        if (!retrieveServices()) {
            NIMBLE_LOGE(LOG_TAG, "Error: Failed to get services");
        } else {
            NIMBLE_LOGI(LOG_TAG, "Found %d services", m_svcVec.size());
        }
    }

    return m_svcVec;
} // getServices

/**
 * @brief Retrieves the full database of attributes that the peripheral has available.
 * @return True if successful.
 */
bool NimBLEClient::discoverAttributes() {
    deleteServices();
    if (!retrieveServices()) {
        return false;
    }

    for (auto svc : m_svcVec) {
        if (!svc->retrieveCharacteristics()) {
            return false;
        }

        for (auto chr : svc->m_vChars) {
            if (!chr->retrieveDescriptors()) {
                return false;
            }
        }
    }

    return true;
} // discoverAttributes

/**
 * @brief Ask the remote BLE server for its services.
 * * Here we ask the server for its set of services and wait until we have received them all.
 * @return true on success otherwise false if an error occurred
 */
bool NimBLEClient::retrieveServices(const NimBLEUUID* uuidFilter) {
    if (m_connStatus != CONNECTED) {
        NIMBLE_LOGE(LOG_TAG, "Disconnected, could not retrieve services -aborting");
        return false;
    }

    int            rc = 0;
    NimBLETaskData taskData(this);

    if (uuidFilter == nullptr) {
        rc = ble_gattc_disc_all_svcs(m_connHandle, NimBLEClient::serviceDiscoveredCB, &taskData);
    } else {
        rc = ble_gattc_disc_svc_by_uuid(m_connHandle, uuidFilter->getBase(), NimBLEClient::serviceDiscoveredCB, &taskData);
    }

    if (rc != 0) {
        NIMBLE_LOGE(LOG_TAG, "ble_gattc_disc_all_svcs: rc=%d %s", rc, NimBLEUtils::returnCodeToString(rc));
        m_lastErr = rc;
        return false;
    }

    NimBLEUtils::taskWait(taskData, BLE_NPL_TIME_FOREVER);
    rc = taskData.m_flags;
    if (rc == 0 || rc == BLE_HS_EDONE) {
        return true;
    }

    m_lastErr = rc;
    NIMBLE_LOGE(LOG_TAG, "Could not retrieve services, rc=%d %s", rc, NimBLEUtils::returnCodeToString(rc));
    return false;
} // getServices

/**
 * @brief Callback for the service discovery API function.
 * @details When a service is found or there is none left or there was an error
 * the API will call this and report findings.
 */
int NimBLEClient::serviceDiscoveredCB(uint16_t                     connHandle,
                                      const struct ble_gatt_error* error,
                                      const struct ble_gatt_svc*   service,
                                      void*                        arg) {
    NIMBLE_LOGD(LOG_TAG,
                "Service Discovered >> status: %d handle: %d",
                error->status,
                (error->status == 0) ? service->start_handle : -1);

    NimBLETaskData* pTaskData = (NimBLETaskData*)arg;
    NimBLEClient*   pClient   = (NimBLEClient*)pTaskData->m_pInstance;

    if (error->status == BLE_HS_ENOTCONN) {
        NIMBLE_LOGE(LOG_TAG, "<< Service Discovered; Disconnected");
        NimBLEUtils::taskRelease(*pTaskData, error->status);
        return error->status;
    }

    // Make sure the service discovery is for this device
    if (pClient->getConnHandle() != connHandle) {
        return 0;
    }

    if (error->status == 0) {
        // Found a service - add it to the vector
        pClient->m_svcVec.push_back(new NimBLERemoteService(pClient, service));
        return 0;
    }

    NimBLEUtils::taskRelease(*pTaskData, error->status);
    NIMBLE_LOGD(LOG_TAG, "<< Service Discovered");
    return error->status;
} // serviceDiscoveredCB

/**
 * @brief Get the value of a specific characteristic associated with a specific service.
 * @param [in] serviceUUID The service that owns the characteristic.
 * @param [in] characteristicUUID The characteristic whose value we wish to read.
 * @returns characteristic value or an empty value if not found.
 */
NimBLEAttValue NimBLEClient::getValue(const NimBLEUUID& serviceUUID, const NimBLEUUID& characteristicUUID) {
    NIMBLE_LOGD(LOG_TAG,
                ">> getValue: serviceUUID: %s, characteristicUUID: %s",
                serviceUUID.toString().c_str(),
                characteristicUUID.toString().c_str());

    NimBLEAttValue ret{};
    auto           pService = getService(serviceUUID);
    if (pService != nullptr) {
        auto pChar = pService->getCharacteristic(characteristicUUID);
        if (pChar != nullptr) {
            ret = pChar->readValue();
        }
    }

    NIMBLE_LOGD(LOG_TAG, "<< getValue");
    return ret;
} // getValue

/**
 * @brief Set the value of a specific characteristic associated with a specific service.
 * @param [in] serviceUUID The service that owns the characteristic.
 * @param [in] characteristicUUID The characteristic whose value we wish to write.
 * @param [in] value The value to write to the characteristic.
 * @param [in] response If true, uses write with response operation.
 * @returns true if successful otherwise false
 */
bool NimBLEClient::setValue(const NimBLEUUID&     serviceUUID,
                            const NimBLEUUID&     characteristicUUID,
                            const NimBLEAttValue& value,
                            bool                  response) {
    NIMBLE_LOGD(LOG_TAG,
                ">> setValue: serviceUUID: %s, characteristicUUID: %s",
                serviceUUID.toString().c_str(),
                characteristicUUID.toString().c_str());

    bool ret      = false;
    auto pService = getService(serviceUUID);
    if (pService != nullptr) {
        NimBLERemoteCharacteristic* pChar = pService->getCharacteristic(characteristicUUID);
        if (pChar != nullptr) {
            ret = pChar->writeValue(value, response);
        }
    }

    NIMBLE_LOGD(LOG_TAG, "<< setValue");
    return ret;
} // setValue

/**
 * @brief Get the remote characteristic with the specified handle.
 * @param [in] handle The handle of the desired characteristic.
 * @returns The matching remote characteristic, nullptr otherwise.
 */
NimBLERemoteCharacteristic* NimBLEClient::getCharacteristic(uint16_t handle) {
    for (const auto& svc : m_svcVec) {
        if (svc->getStartHandle() <= handle && handle <= svc->getEndHandle()) {
            for (const auto& chr : svc->m_vChars) {
                if (chr->getHandle() == handle) {
                    return chr;
                }
            }
        }
    }

    return nullptr;
} // getCharacteristic

/**
 * @brief Get the current mtu of this connection.
 * @returns The MTU value.
 */
uint16_t NimBLEClient::getMTU() const {
    return ble_att_mtu(m_connHandle);
} // getMTU

/**
 * @brief Callback for the MTU exchange API function.
 * @details When the MTU exchange is complete the API will call this and report the new MTU.
 */
int NimBLEClient::exchangeMTUCb(uint16_t conn_handle, const ble_gatt_error* error, uint16_t mtu, void* arg) {
    NIMBLE_LOGD(LOG_TAG, "exchangeMTUCb: status=%d, mtu=%d", error->status, mtu);

    NimBLEClient* pClient = (NimBLEClient*)arg;
    if (pClient->getConnHandle() != conn_handle) {
        return 0;
    }

    if (error->status != 0) {
        NIMBLE_LOGE(LOG_TAG, "exchangeMTUCb() rc=%d %s", error->status, NimBLEUtils::returnCodeToString(error->status));
        pClient->m_lastErr = error->status;
    }

    return 0;
} // exchangeMTUCb

/**
 * @brief Begin the MTU exchange process with the server.
 * @returns true if the request was sent successfully.
 */
bool NimBLEClient::exchangeMTU() {
    int rc = ble_gattc_exchange_mtu(m_connHandle, NimBLEClient::exchangeMTUCb, this);
    if (rc != 0 && rc != BLE_HS_EALREADY) {
        NIMBLE_LOGE(LOG_TAG, "MTU exchange error; rc=%d %s", rc, NimBLEUtils::returnCodeToString(rc));
        m_lastErr = rc;
        return false;
    }

    return true;
} // exchangeMTU

void NimBLEClient::startConnectEstablishedTimer(uint16_t connInterval) {
    // As per Bluetooth spec, the connection is only established after receiving a PDU
    // within 6 connections events, so we wait for 7 connection events for a margin.
    uint32_t waitMs = connIntervalToMs(connInterval) * 7;
    if (waitMs == 0) {
        waitMs = 1;
    }

    ble_npl_time_t waitTicks = 1;
    ble_npl_time_ms_to_ticks(waitMs, &waitTicks);
    if (waitTicks == 0) {
        waitTicks = 1;
    }

    ble_npl_callout_reset(&m_connectEstablishedTimer, waitTicks);
} // startConnectEstablishedTimer

bool NimBLEClient::completeConnectEstablished() {
    if (!m_connectCallbackPending) {
        return false;
    }

    m_connectCallbackPending = false;
    ble_npl_callout_stop(&m_connectEstablishedTimer);
    auto pTaskData = m_pTaskData; // save a copy in case something in the callback changes it
    m_pTaskData    = nullptr;     // clear before callback to prevent other handlers from releasing
    m_pClientCallbacks->onConnect(this);

    if (pTaskData != nullptr) {
        NimBLEUtils::taskRelease(*pTaskData, 0);
    }

    return true;
} // completeConnectEstablished

void NimBLEClient::connectEstablishedTimerCb(struct ble_npl_event* event) {
    auto* pClient = static_cast<NimBLEClient*>(ble_npl_event_get_arg(event));
    if (pClient == nullptr || pClient->m_connStatus != CONNECTED) {
        return;
    }

    pClient->completeConnectEstablished();
} // connectEstablishedTimerCb

/**
 * @brief Set the number of times to retry connecting after a connection establishment error (0x3e).
 * @param [in] numRetries The number of retries to attempt before giving up and reporting the failure.
 * @details Max is 7, Default is 2.
 */
void NimBLEClient::setConnectRetries(uint8_t numRetries) {
    m_config.connectFailRetries = std::min<uint8_t>(numRetries, 7U);
} // setConnectRetries

/**
 * @brief Handle a received GAP event.
 * @param [in] event The event structure sent by the NimBLE stack.
 * @param [in] arg A pointer to the client instance that registered for this callback.
 */
int NimBLEClient::handleGapEvent(struct ble_gap_event* event, void* arg) {
    NimBLEClient*   pClient   = (NimBLEClient*)arg;
    int             rc        = 0;
    NimBLETaskData* pTaskData = pClient->m_pTaskData; // save a copy in case client is deleted

    NIMBLE_LOGD(LOG_TAG, ">> handleGapEvent %s", NimBLEUtils::gapEventToString(event->type));

    switch (event->type) {
        case BLE_GAP_EVENT_DISCONNECT: {
            // workaround for bug in NimBLE stack where disconnect event argument is not passed correctly
            pClient = NimBLEDevice::getClientByHandle(event->disconnect.conn.conn_handle);
            if (pClient == nullptr) {
                pClient = NimBLEDevice::getClientByPeerAddress(event->disconnect.conn.peer_ota_addr);
            }

            if (pClient == nullptr) {
                pClient = NimBLEDevice::getClientByPeerAddress(event->disconnect.conn.peer_id_addr);
            }

            if (pClient == nullptr) {
                NIMBLE_LOGE(LOG_TAG, "Disconnected client not found, conn_handle=%d", event->disconnect.conn.conn_handle);
                return 0;
            }

            pClient->m_connectCallbackPending = false;
            ble_npl_callout_stop(&pClient->m_connectEstablishedTimer);

            rc = event->disconnect.reason;
            // If Host reset tell the device now before returning to prevent
            // any errors caused by calling host functions before re-syncing.
            switch (rc) {
                case BLE_HS_ECONTROLLER:
                case BLE_HS_ETIMEOUT_HCI:
                case BLE_HS_ENOTSYNCED:
                case BLE_HS_EOS:
                    NIMBLE_LOGE(LOG_TAG, "Disconnect - host reset, rc=%d", rc);
                    NimBLEDevice::onReset(rc);
                    break;
                default:
                    break;
            }

            NIMBLE_LOGD(LOG_TAG, "disconnect; reason=%d, %s", rc, NimBLEUtils::returnCodeToString(rc));

            pClient->m_terminateFailCount = 0;
            pClient->m_asyncSecureAttempt = 0;

            // set this incase the client instance was changed due to incorrect event arg bug above
            pTaskData = pClient->m_pTaskData;

            const int connEstablishFailReason = BLE_HS_HCI_ERR(BLE_ERR_CONN_ESTABLISHMENT);
            if (rc == connEstablishFailReason && pClient->m_connectFailRetryCount < pClient->m_config.connectFailRetries) {
                pClient->m_connHandle = BLE_HS_CONN_HANDLE_NONE;
                ++pClient->m_connectFailRetryCount;
                pClient->m_connStatus = CONNECTING;
                NIMBLE_LOGW(LOG_TAG,
                            "Connection establishment failed (0x3e), retry %u/%u",
                            pClient->m_connectFailRetryCount,
                            pClient->m_config.connectFailRetries);

                const int retryRc = pClient->startConnectionAttempt(pClient->m_peerAddress.getBase());
                if (retryRc == 0) {
                    // A retry attempt is in progress; suppress user callbacks until final outcome.
                    return 0;
                }

                NIMBLE_LOGE(LOG_TAG, "Retry connect start failed, rc=%d %s", retryRc, NimBLEUtils::returnCodeToString(retryRc));
            }

            if (rc == connEstablishFailReason) {
                pClient->m_pClientCallbacks->onConnectFail(pClient, rc);
            } else {
                pClient->m_pClientCallbacks->onDisconnect(pClient, rc);
            }

            pClient->m_connHandle = BLE_HS_CONN_HANDLE_NONE;
            pClient->m_connStatus = DISCONNECTED;

            if (pClient->m_config.deleteOnDisconnect ||
                (rc == connEstablishFailReason && pClient->m_config.deleteOnConnectFail)) {
                // If we are set to self delete on disconnect but we have a task waiting on the connection
                // completion we will set the flag to delete on connect fail instead of deleting here
                if (pTaskData != nullptr && rc == connEstablishFailReason) {
                    pClient->m_config.deleteOnConnectFail = true;
                    break;
                }
                NimBLEDevice::deleteClient(pClient);
            }

            break;
        } // BLE_GAP_EVENT_DISCONNECT

        case BLE_GAP_EVENT_CONNECT: {
            // If we aren't waiting for this connection response we should drop the connection immediately.
            if (pClient->m_connStatus != CONNECTING) {
                ble_gap_terminate(event->connect.conn_handle, BLE_ERR_REM_USER_CONN_TERM);
                return 0;
            }

            rc = event->connect.status;
            if (rc == BLE_ERR_UNSUPP_REM_FEATURE) {
                rc = 0; // Workaround: Ignore unsupported remote feature error as it is not a real error.
            }

            if (rc == 0) {
                pClient->m_connStatus             = CONNECTED;
                pClient->m_connHandle             = event->connect.conn_handle;
                pClient->m_connectCallbackPending = true;

                ble_gap_conn_desc desc;
                if (ble_gap_conn_find(event->connect.conn_handle, &desc) == 0) {
                    pClient->startConnectEstablishedTimer(desc.conn_itvl);
                } else {
                    pClient->startConnectEstablishedTimer(pClient->m_connParams.itvl_max);
                }

                if (pClient->m_config.exchangeMTU) {
                    pClient->exchangeMTU();
                }
                // return as we may have a task waiting on the connection completion
                // and will release it in the timer callback after the connection is fully established.
                return 0;
            } else {
                pClient->m_connStatus             = DISCONNECTED;
                pClient->m_connHandle             = BLE_HS_CONN_HANDLE_NONE;
                pClient->m_connectCallbackPending = false;
                ble_npl_callout_stop(&pClient->m_connectEstablishedTimer);

                if (pClient->m_config.asyncConnect) {
                    pClient->m_pClientCallbacks->onConnectFail(pClient, rc);
                    if (pClient->m_config.deleteOnConnectFail) {
                        NimBLEDevice::deleteClient(pClient);
                    }
                }
            }

            break;
        } // BLE_GAP_EVENT_CONNECT

        case BLE_GAP_EVENT_TERM_FAILURE: {
            if (pClient->m_connHandle != event->term_failure.conn_handle) {
                return 0;
            }

            NIMBLE_LOGE(LOG_TAG, "Connection termination failure; rc=%d - retrying", event->term_failure.status);
            if (++pClient->m_terminateFailCount > 2) {
                ble_hs_sched_reset(BLE_HS_ECONTROLLER);
            } else {
                ble_gap_terminate(event->term_failure.conn_handle, BLE_ERR_REM_USER_CONN_TERM);
            }
            return 0;
        } // BLE_GAP_EVENT_TERM_FAILURE

        case BLE_GAP_EVENT_NOTIFY_RX: {
            if (pClient->m_connHandle != event->notify_rx.conn_handle) {
                return 0;
            }

            if (pClient->completeConnectEstablished()) {
                pTaskData = nullptr;
            }

            NIMBLE_LOGD(LOG_TAG, "Notify Received for handle: %d", event->notify_rx.attr_handle);

            NimBLERemoteCharacteristic* pChr = pClient->getCharacteristic(event->notify_rx.attr_handle);
            if (pChr == nullptr) {
                NIMBLE_LOGW(LOG_TAG, "unknown handle: %d", event->notify_rx.attr_handle);
                return BLE_ATT_ERR_INVALID_HANDLE;
            }

            auto len = event->notify_rx.om->om_len;
            if (pChr->m_value.setValue(event->notify_rx.om->om_data, len)) {
                os_mbuf* next;
                next = SLIST_NEXT(event->notify_rx.om, om_next);
                while (next != NULL) {
                    pChr->m_value.append(next->om_data, next->om_len);
                    if (pChr->m_value.length() != len + next->om_len) {
                        rc = BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
                        break;
                    }
                    len  += next->om_len;
                    next  = SLIST_NEXT(next, om_next);
                }
            } else {
                rc = BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }

            if (rc != 0) { // This should never happen
                NIMBLE_LOGE(LOG_TAG, "notification value error; exceeds limit");
                return rc;
            }

            if (pChr->m_notifyCallback != nullptr) {
                // TODO: change this callback to use the NimBLEAttValue class instead of raw data and length
                pChr->m_notifyCallback(pChr,
                                       const_cast<uint8_t*>(pChr->m_value.getValue().data()),
                                       pChr->m_value.length(),
                                       !event->notify_rx.indication);
            }

            return 0;
        } // BLE_GAP_EVENT_NOTIFY_RX

        case BLE_GAP_EVENT_CONN_UPDATE_REQ:
        case BLE_GAP_EVENT_L2CAP_UPDATE_REQ: {
            if (pClient->m_connHandle != event->conn_update_req.conn_handle) {
                return 0;
            }

            if (pClient->completeConnectEstablished()) {
                pTaskData = nullptr;
            }

            NIMBLE_LOGD(LOG_TAG, "Peer requesting to update connection parameters");
            NIMBLE_LOGD(LOG_TAG,
                        "MinInterval: %d, MaxInterval: %d, Latency: %d, Timeout: %d",
                        event->conn_update_req.peer_params->itvl_min,
                        event->conn_update_req.peer_params->itvl_max,
                        event->conn_update_req.peer_params->latency,
                        event->conn_update_req.peer_params->supervision_timeout);

            rc = pClient->m_pClientCallbacks->onConnParamsUpdateRequest(pClient, event->conn_update_req.peer_params)
                     ? 0
                     : BLE_ERR_CONN_PARMS;

            if (!rc && event->type == BLE_GAP_EVENT_CONN_UPDATE_REQ) {
                event->conn_update_req.self_params->itvl_min            = pClient->m_connParams.itvl_min;
                event->conn_update_req.self_params->itvl_max            = pClient->m_connParams.itvl_max;
                event->conn_update_req.self_params->latency             = pClient->m_connParams.latency;
                event->conn_update_req.self_params->supervision_timeout = pClient->m_connParams.supervision_timeout;
            }

            NIMBLE_LOGD(LOG_TAG, "%s peer params", (rc == 0) ? "Accepted" : "Rejected");
            return rc;
        } // BLE_GAP_EVENT_CONN_UPDATE_REQ, BLE_GAP_EVENT_L2CAP_UPDATE_REQ

        case BLE_GAP_EVENT_CONN_UPDATE: {
            if (pClient->m_connHandle != event->conn_update.conn_handle) {
                return 0;
            }

            if (pClient->completeConnectEstablished()) {
                pTaskData = nullptr;
            }

            if (event->conn_update.status == 0) {
                NIMBLE_LOGI(LOG_TAG, "Connection parameters updated.");
            } else {
                NIMBLE_LOGE(LOG_TAG, "Update connection parameters failed.");
            }
            return 0;
        } // BLE_GAP_EVENT_CONN_UPDATE

        case BLE_GAP_EVENT_ENC_CHANGE: {
            if (pClient->m_connHandle != event->enc_change.conn_handle) {
                return 0;
            }

            if (pClient->completeConnectEstablished()) {
                pTaskData = nullptr;
            }

            if (event->enc_change.status == 0 || event->enc_change.status == BLE_HS_HCI_ERR(BLE_ERR_PINKEY_MISSING)) {
                NimBLEConnInfo peerInfo;
                rc = ble_gap_conn_find(event->enc_change.conn_handle, &peerInfo.m_desc);
                if (rc != 0) {
                    rc = 0;
                    break;
                }

                if (event->enc_change.status == BLE_HS_HCI_ERR(BLE_ERR_PINKEY_MISSING)) {
                    // Key is missing, try deleting.
                    ble_store_util_delete_peer(&peerInfo.m_desc.peer_id_addr);
                    // Attempt a retry if async secure failed.
                    if (pClient->m_asyncSecureAttempt == 1) {
                        pClient->secureConnection(true);
                    }
                } else {
                    pClient->m_asyncSecureAttempt = 0;
                    pClient->m_pClientCallbacks->onAuthenticationComplete(peerInfo);
                }
            }

            rc = event->enc_change.status;
            break;
        } // BLE_GAP_EVENT_ENC_CHANGE

        case BLE_GAP_EVENT_IDENTITY_RESOLVED: {
            if (pClient->m_connHandle != event->identity_resolved.conn_handle) {
                return 0;
            }

            if (pClient->completeConnectEstablished()) {
                pTaskData = nullptr;
            }

            NimBLEConnInfo peerInfo;
            rc = ble_gap_conn_find(event->identity_resolved.conn_handle, &peerInfo.m_desc);
            if (rc != 0) {
                rc = 0;
                break;
            }

            pClient->m_pClientCallbacks->onIdentity(peerInfo);
            break;
        } // BLE_GAP_EVENT_IDENTITY_RESOLVED

        case BLE_GAP_EVENT_PHY_UPDATE_COMPLETE: {
            if (pClient->m_connHandle != event->phy_updated.conn_handle) {
                return 0;
            }

            if (pClient->completeConnectEstablished()) {
                pTaskData = nullptr;
            }

            NimBLEConnInfo peerInfo;
            rc = ble_gap_conn_find(event->phy_updated.conn_handle, &peerInfo.m_desc);
            if (rc != 0) {
                return BLE_ATT_ERR_INVALID_HANDLE;
            }

            pClient->m_pClientCallbacks->onPhyUpdate(pClient, event->phy_updated.tx_phy, event->phy_updated.rx_phy);
            return 0;
        } // BLE_GAP_EVENT_PHY_UPDATE_COMPLETE

        case BLE_GAP_EVENT_MTU: {
            if (pClient->m_connHandle != event->mtu.conn_handle) {
                return 0;
            }

            if (pClient->completeConnectEstablished()) {
                pTaskData = nullptr;
            }

            NIMBLE_LOGI(LOG_TAG, "mtu update: mtu=%d", event->mtu.value);
            pClient->m_pClientCallbacks->onMTUChange(pClient, event->mtu.value);
            rc = 0;
            break;
        } // BLE_GAP_EVENT_MTU

        case BLE_GAP_EVENT_PASSKEY_ACTION: {
            if (pClient->m_connHandle != event->passkey.conn_handle) {
                return 0;
            }

            if (pClient->completeConnectEstablished()) {
                pTaskData = nullptr;
            }

            NimBLEConnInfo peerInfo;
            rc = ble_gap_conn_find(event->passkey.conn_handle, &peerInfo.m_desc);
            if (rc != 0) {
                rc = 0;
                break;
            }

            if (event->passkey.params.action == BLE_SM_IOACT_DISP) {
                struct ble_sm_io pkey = {0, 0};
                pkey.action           = event->passkey.params.action;
                pkey.passkey          = NimBLEDevice::getSecurityPasskey();
                if (pkey.passkey == 123456) {
                    pkey.passkey = pClient->m_pClientCallbacks->onPassKeyDisplay(peerInfo);
                }
                rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
                NIMBLE_LOGD(LOG_TAG, "BLE_SM_IOACT_DISP; ble_sm_inject_io result: %d", rc);
            } else if (event->passkey.params.action == BLE_SM_IOACT_NUMCMP) {
                NIMBLE_LOGD(LOG_TAG, "Passkey on device's display: %" PRIu32, event->passkey.params.numcmp);
                pClient->m_pClientCallbacks->onConfirmPasskey(peerInfo, event->passkey.params.numcmp);
            } else if (event->passkey.params.action == BLE_SM_IOACT_OOB) {
                NIMBLE_LOGD(LOG_TAG, "OOB request received");
                // TODO: Handle out of band pairing
                // struct ble_sm_io pkey;
                // pkey.action = BLE_SM_IOACT_OOB;
                // pClient->onOobPairingRequest(pkey.oob);
                // rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
                // NIMBLE_LOGD(LOG_TAG, "ble_sm_inject_io result: %d", rc);
            } else if (event->passkey.params.action == BLE_SM_IOACT_INPUT) {
                NIMBLE_LOGD(LOG_TAG, "Enter the passkey");
                pClient->m_pClientCallbacks->onPassKeyEntry(peerInfo);
            } else if (event->passkey.params.action == BLE_SM_IOACT_NONE) {
                NIMBLE_LOGD(LOG_TAG, "No passkey action required");
            }

            return 0;
        } // BLE_GAP_EVENT_PASSKEY_ACTION

        default: {
            return 0;
        }
    } // Switch

    if (pTaskData != nullptr) {
        NimBLEUtils::taskRelease(*pTaskData, rc);
    }

    NIMBLE_LOGD(LOG_TAG, "<< handleGapEvent");
    return 0;
} // handleGapEvent

/**
 * @brief Are we connected to a server?
 * @return True if we are connected and false if we are not connected.
 */
bool NimBLEClient::isConnected() const {
    return m_connStatus == CONNECTED;
} // isConnected

/**
 * @brief Set the callbacks that will be invoked when events are received.
 * @param [in] pClientCallbacks A pointer to a class to receive the event callbacks.
 * @param [in] deleteCallbacks If true this will delete the callback class sent when the client is destructed.
 */
void NimBLEClient::setClientCallbacks(NimBLEClientCallbacks* pClientCallbacks, bool deleteCallbacks) {
    if (pClientCallbacks != nullptr) {
        m_pClientCallbacks       = pClientCallbacks;
        m_config.deleteCallbacks = deleteCallbacks;
    } else {
        m_pClientCallbacks       = &defaultCallbacks;
        m_config.deleteCallbacks = false;
    }
} // setClientCallbacks

/**
 * @brief Return a string representation of this client.
 * @return A string representation of this client.
 */
std::string NimBLEClient::toString() const {
    std::string res  = "peer address: " + m_peerAddress.toString();
    res             += "\nServices:\n";

    for (const auto& it : m_svcVec) {
        res += it->toString() + "\n";
    }

    return res;
} // toString

static const char* CB_TAG = "NimBLEClientCallbacks";

/**
 * @brief Get the last error code reported by the NimBLE host
 * @return int, the NimBLE error code.
 */
int NimBLEClient::getLastError() const {
    return m_lastErr;
} // getLastError

void NimBLEClientCallbacks::onConnect(NimBLEClient* pClient) {
    NIMBLE_LOGD(CB_TAG, "onConnect: default");
} // onConnect

void NimBLEClientCallbacks::onConnectFail(NimBLEClient* pClient, int reason) {
    NIMBLE_LOGD(CB_TAG, "onConnectFail: default, reason: %d", reason);
} // onConnectFail

void NimBLEClientCallbacks::onDisconnect(NimBLEClient* pClient, int reason) {
    NIMBLE_LOGD(CB_TAG, "onDisconnect: default, reason: %d", reason);
} // onDisconnect

bool NimBLEClientCallbacks::onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params) {
    NIMBLE_LOGD(CB_TAG, "onConnParamsUpdateRequest: default");
    return true;
} // onConnParamsUpdateRequest

void NimBLEClientCallbacks::onPassKeyEntry(NimBLEConnInfo& connInfo) {
    NIMBLE_LOGD(CB_TAG, "onPassKeyEntry: default: 123456");
    NimBLEDevice::injectPassKey(connInfo, 123456);
} // onPassKeyEntry

uint32_t NimBLEClientCallbacks::onPassKeyDisplay(NimBLEConnInfo& connInfo) {
    NIMBLE_LOGD(CB_TAG, "onPassKeyDisplay: default");
    return NimBLEDevice::getSecurityPasskey();
} // onPassKeyDisplay

void NimBLEClientCallbacks::onAuthenticationComplete(NimBLEConnInfo& connInfo) {
    NIMBLE_LOGD(CB_TAG, "onAuthenticationComplete: default");
} // onAuthenticationComplete

void NimBLEClientCallbacks::onIdentity(NimBLEConnInfo& connInfo) {
    NIMBLE_LOGD(CB_TAG, "onIdentity: default");
} // onIdentity

void NimBLEClientCallbacks::onConfirmPasskey(NimBLEConnInfo& connInfo, uint32_t pin) {
    NIMBLE_LOGD(CB_TAG, "onConfirmPasskey: default: true");
    NimBLEDevice::injectConfirmPasskey(connInfo, true);
} // onConfirmPasskey

void NimBLEClientCallbacks::onMTUChange(NimBLEClient* pClient, uint16_t mtu) {
    NIMBLE_LOGD(CB_TAG, "onMTUChange: default");
} // onMTUChange

void NimBLEClientCallbacks::onPhyUpdate(NimBLEClient* pClient, uint8_t txPhy, uint8_t rxPhy) {
    NIMBLE_LOGD(CB_TAG, "onPhyUpdate: default, txPhy: %d, rxPhy: %d", txPhy, rxPhy);
} // onPhyUpdate

#endif // CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_CENTRAL
