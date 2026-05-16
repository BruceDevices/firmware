/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#ifndef H_BLE_ESP_GAP_
#define H_BLE_ESP_GAP_

#ifdef __cplusplus
extern "C" {
#endif

enum gap_status {
    BLE_GAP_STATUS_ADV = 0,
    BLE_GAP_STATUS_EXT_ADV,
    BLE_GAP_STATUS_SCAN,
    BLE_GAP_STATUS_CONN,
    BLE_GAP_STATUS_PAIRED,
    BLE_GAP_STATUS_GATTS,
    BLE_GAP_STATUS_HOST_PRIVACY,
    BLE_GAP_STATUS_PERIODIC,
};

typedef enum gap_status gap_status_t;

#define BLE_DUPLICATE_SCAN_EXCEPTIONAL_INFO_ADV_ADDR             0
#define BLE_DUPLICATE_SCAN_EXCEPTIONAL_INFO_MESH_LINK_ID         1
#define BLE_DUPLICATE_SCAN_EXCEPTIONAL_INFO_MESH_BEACON_TYPE     2
#define BLE_DUPLICATE_SCAN_EXCEPTIONAL_INFO_MESH_PROV_SRV_ADV    3
#define BLE_DUPLICATE_SCAN_EXCEPTIONAL_INFO_MESH_PROXY_SRV_ADV   4

#define ESP_BLE_VENDOR_LEGACY_SCAN_REQ_EVT_MASK       BIT(0)
#define ESP_BLE_VENDOR_CHAN_MAP_UPDATE_CMPL_EVT_MASK  BIT(1)
#define ESP_BLE_VENDOR_TL_RUNNING_STATUS_EVT_MASK     BIT(2)
#define ESP_BLE_VENDOR_SLEEP_WAKEUP_EVT_MASK         BIT(3)
#define ESP_BLE_VENDOR_CONNECT_IND_REQ_EVT_MASK      BIT(4)
#define ESP_BLE_VENDOR_AUX_CONNECT_RSP_EVT_MASK      BIT(5)

#if MYNEWT_VAL(BLE_ENABLE_CONN_REATTEMPT)
#ifdef CONFIG_BT_NIMBLE_MAX_CONN_REATTEMPT
#define MAX_REATTEMPT_ALLOWED CONFIG_BT_NIMBLE_MAX_CONN_REATTEMPT
#else
#define MAX_REATTEMPT_ALLOWED 0
#endif
#endif

/**
 * Configure LE Data Length in controller (OGF = 0x08, OCF = 0x0022).
 *
 * @param conn_handle      Connection handle.
 * @param tx_octets        The preferred value of payload octets that the Controller
 *                         should use for a new connection (Range
 *                         0x001B-0x00FB).
 * @param tx_time          The preferred maximum number of microseconds that the local Controller
 *                         should use to transmit a single link layer packet
 *                         (Range 0x0148-0x4290).
 *
 * @return              0 on success,
 *                      other error code on failure.
 */
int ble_hs_hci_util_set_data_len(uint16_t conn_handle, uint16_t tx_octets,
                                 uint16_t tx_time);

/**
 * Read host's suggested values for the controller's maximum transmitted number of payload octets
 * and maximum packet transmission time (OGF = 0x08, OCF = 0x0024).
 *
 * @param out_sugg_max_tx_octets    The Host's suggested value for the Controller's maximum transmitted
 *                                  number of payload octets in LL Data PDUs to be used for new
 *                                  connections. (Range 0x001B-0x00FB).
 * @param out_sugg_max_tx_time      The Host's suggested value for the Controller's maximum packet
 *                                  transmission time for packets containing LL Data PDUs to be used
 *                                  for new connections. (Range 0x0148-0x4290).
 *
 * @return                          0 on success,
 *                                  other error code on failure.
 */
int ble_hs_hci_util_read_sugg_def_data_len(uint16_t *out_sugg_max_tx_octets,
                                           uint16_t *out_sugg_max_tx_time);
/**
 * Configure host's suggested maximum transmitted number of payload octets and maximum packet
 * transmission time in controller (OGF = 0x08, OCF = 0x0024).
 *
 * @param sugg_max_tx_octets    The Host's suggested value for the Controller's maximum transmitted
 *                              number of payload octets in LL Data PDUs to be used for new
 *                              connections. (Range 0x001B-0x00FB).
 * @param sugg_max_tx_time      The Host's suggested value for the Controller's maximum packet
 *                              transmission time for packets containing LL Data PDUs to be used
 *                              for new connections. (Range 0x0148-0x4290).
 *
 * @return                      0 on success,
 *                              other error code on failure.
 */
int ble_hs_hci_util_write_sugg_def_data_len(uint16_t sugg_max_tx_octets, uint16_t sugg_max_tx_time);

/**
 * Removes the address from controller's white list.
 *
 * @param addrs                 The entry to be removed from the white list.
 *
 * @return                      0 on success; nonzero on failure.
 */
int ble_gap_wl_tx_rmv(const ble_addr_t *addrs);

/**
 * Adds the address in controller's white list.
 *
 * @param addrs                 The entry to be added in the white list.
 *
 * @return                      0 on success; nonzero on failure.
 */
int ble_gap_wl_tx_add(const ble_addr_t *addrs);

/**
 * Clears all addresses from controller's white list.
 *
 * @return                      0 on success; nonzero on failure.
 */
int ble_gap_wl_tx_clear(void);

/**
 * Retrieves the size of the controller's white list.
 *
 * @param size                  On success, total size of whitelist will be stored here.
 *
 * @return                      0 on success; nonzero on failure.
 */
int ble_gap_wl_read_size(uint8_t *size);

/**
 * This API gives the current status of various stack operations
 *
 * @return                      0 on success; nonzero bits indicating different
 *                              operations as per enum gap_status.
 */
int ble_gap_host_check_status(void);

/**
 * This API is called to get local used address and address type.
 *
 * @param addr                  On success, locally used address will be stored here.
 *
 * @return                      0 on success; nonzero on failure.
*/
int ble_gap_get_local_used_addr(ble_addr_t *addr);

/**
 * This API is called to get ADV data for a specific type.

 *
 * @param adv_data                  Pointer of ADV data which to be resolved.
 * @param adv_type                  Finding ADV data type.
 * @param adv_data_len              Total length of Advertising data.
 * @param length                    Return the length of ADV data not including type.
 *
 * @return                          Pointer of type specific ADV data.
 */
uint8_t* ble_resolve_adv_data(const uint8_t *adv_data, uint8_t adv_type, uint8_t adv_data_len , uint8_t * length);

#if MYNEWT_VAL(BLE_HCI_VS)
#if MYNEWT_VAL(BLE_POWER_CONTROL)

#define ESP_1M_LOW    (-70)
#define ESP_1M_HIGH   (-60)
#define ESP_2M_LOW    (-68)
#define ESP_2M_HIGH   (-58)
#define ESP_S2_LOW    (-75)
#define ESP_S2_HIGH   (-65)
#define ESP_S8_LOW    (-80)
#define ESP_S8_HIGH   (-70)
#define ESP_MIN_TIME  (15)

/* Represents the set of lower / upper values of rssi of given chip
 *
 * Lower Limit Values Range: -54 to -80
 * Upper Limit Values Range: -40 to -70
 *
 * */
struct ble_gap_set_auto_pcl_params {

    /* Connection Handle of the ACL Link */
    int16_t conn_handle;

    /* The Lower RSSI limit when 1M phy is used */
    int8_t m1_lower_limit;

    /* The Upper RSSI limit when 1M phy is used */
    int8_t m1_upper_limit;

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_2M_PHY)
    /* The Lower RSSI limit when 2M phy is used */
    int8_t m2_lower_limit;

    /* The Upper RSSI limit when 2M phy is used */
    int8_t m2_upper_limit;
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CODED_PHY)
     /* The Lower RSSI limit when S2 Coded phy is used */
    int8_t s2_lower_limit;

    /* The Upper RSSI limit when S2 Coded phy is used */
    int8_t s2_upper_limit;

    /* The Lower RSSI limit when S8 Coded phy is used */
    int8_t s8_lower_limit;

    /* The Upper RSSI limit when S8 Coded phy is used */
    int8_t s8_upper_limit;
#endif

    /* Number of tx/rx packets to wait before initiating the LE power control Request.
     * The default value is (min time spent variable =  (tx/rxpackets 15)).*/
    uint8_t min_time_spent;
};

/**
 * This API  is used to initiate the LE Power Control Request Procedure for the ACL connection
 * identified by the conn_handle parameter and other parameters.
 *
 * The parameters passed are used by controller for the subsquent LE Power Control Requests
 * that get initiated across all the connections.
 *
 * The Min_Time_Spent parameter indicates the number of tx/rx packets that the Controller
 * shall observe the RSSI  has crossed the threshold (upper and lower limit of active phy)
 * before the controller initiates the LE POWER CONTROL PROCEDURE in the link layer.
 *
 * @param params	  Instance of ble_gap_set_auto_pcl_params with different parameters
 *
 * @return                0 on success; nonzero on failure.
 */
int ble_gap_set_auto_pcl_param(struct ble_gap_set_auto_pcl_params *params);
#endif

/**
 * This API  is used to send the Duplicate Exception list VSC to controller
 *
 * @param subcode	  The operation to be done (add/remove/clean)
 *
 * @param type            Exception list type
 *
 * @param value           Device address
 *
 * @cb 		          Registered callback
 *
 * @return                0 on success; nonzero on failure.
 */
int ble_gap_duplicate_exception_list(uint8_t subcode, uint8_t type, uint8_t *value, void *cb);

/**
 * This API is used to clean up residue memory in controller for legacy advertisement
 *
 * @return                0 on success; nonzero on failure.
 */
int ble_gap_clear_legacy_adv(void);

/**
 * This API is used to let controller know which CSA to use. Not applicable for ESP32
 *
 * @return                0 on success; nonzero of failure.
 */
int ble_gap_set_chan_select(uint8_t select);

#endif

/**
 * Authorizes or deauthorizes a BLE device for a connection.
 *
 * This function updates the security flags of a BLE connection to authorize or
 * deauthorize a device for the specified connection.
 *
 * @param conn_handle           The handle corresponding to the connection to
 *                              authorize.
 * @param authorized            Authorized the device or not.
 *
 * @return                      0 on success;
 *                              BLE_HS_ENOTCONN if the connection handle is not found.
 *                              BLE_HS_EAUTHOR if the device is not authenticated before authorization.
 */
int
ble_gap_dev_authorization(uint16_t conn_handle, bool authorized);

/**
 * Sets the vendor-specific event mask for BLE host.
 *
 * This function configures the vendor-specific event mask, enabling or disabling
 * specific vendor-defined events from being reported by the controller.
 *
 * @param event_mask            Bitmask representing the events to enable.
 *
 * @return                      0 on success;
 *                              A nonzero value indicating an error if the command fails.
 */
int
ble_hs_send_vs_event_mask(uint32_t event_mask);

void ble_gap_rx_test_evt(const void *buf, uint8_t len);
void ble_gap_tx_test_evt(const void *buf, uint8_t len);
void ble_gap_end_test_evt(const void *buf, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif
