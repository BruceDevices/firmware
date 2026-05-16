/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/**
 * @file ble_l2cap.h
 *
 * @brief L2CAP (Logical Link Control and Adaptation Protocol) API
 *
 * This header file provides the public API for interacting with the L2CAP layer of the BLE
 * (Bluetooth Low Energy) stack. L2CAP is responsible for managing logical channels between
 * two connected BLE devices.
 *
 * @defgroup bt_host_l2cap Bluetooth Host Logical Link Control and Adaptation Protocol
 * @ingroup bt_host
 * @{
 */

#ifndef H_BLE_L2CAP_
#define H_BLE_L2CAP_

#include "nimble/nimble/include/nimble/nimble_opt.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief L2CAP Signaling Connection Parameters Update Request.
 *
 * This structure represents a request to update the L2CAP connection parameters.
 */
struct ble_l2cap_sig_update_req;

/**
 * @brief BLE Host Connection structure.
 *
 * This structure represents a connection between the BLE host and a remote device.
 */
struct ble_hs_conn;

/**
 * @defgroup ble_l2cap_channel_ids Channel Identifiers
 * @{
 */
/** Attribute Protocol (ATT) CID. */
#define BLE_L2CAP_CID_ATT           4

/** L2CAP LE Signaling CID. */
#define BLE_L2CAP_CID_SIG           5

/** Security Manager (SM) CID. */
#define BLE_L2CAP_CID_SM            6

/** @} */

/**
 * @defgroup ble_l2cap_signaling_op_codes Signaling Commands Operation Codes
 * @{
 */

/** Reject */
#define BLE_L2CAP_SIG_OP_REJECT                 0x01

/** Connect Request */
#define BLE_L2CAP_SIG_OP_CONNECT_REQ            0x02

/** Connect Response */
#define BLE_L2CAP_SIG_OP_CONNECT_RSP            0x03

/** Configuration Request */
#define BLE_L2CAP_SIG_OP_CONFIG_REQ             0x04

/** Configuration Response */
#define BLE_L2CAP_SIG_OP_CONFIG_RSP             0x05

/** Disconnect Request */
#define BLE_L2CAP_SIG_OP_DISCONN_REQ            0x06

/** Disconnect Response */
#define BLE_L2CAP_SIG_OP_DISCONN_RSP            0x07

/** Echo Request */
#define BLE_L2CAP_SIG_OP_ECHO_REQ               0x08

/** Echo Response */
#define BLE_L2CAP_SIG_OP_ECHO_RSP               0x09

/** Information Request */
#define BLE_L2CAP_SIG_OP_INFO_REQ               0x0a

/** Information Response */
#define BLE_L2CAP_SIG_OP_INFO_RSP               0x0b

/** Create Channel Request */
#define BLE_L2CAP_SIG_OP_CREATE_CHAN_REQ        0x0c

/** Create Channel Response */
#define BLE_L2CAP_SIG_OP_CREATE_CHAN_RSP        0x0d

/** Move Channel Request */
#define BLE_L2CAP_SIG_OP_MOVE_CHAN_REQ          0x0e

/** Move Channel Response */
#define BLE_L2CAP_SIG_OP_MOVE_CHAN_RSP          0x0f

/** Move Channel Confirmation Request */
#define BLE_L2CAP_SIG_OP_MOVE_CHAN_CONF_REQ     0x10

/** Move Channel Confirmation Response */
#define BLE_L2CAP_SIG_OP_MOVE_CHAN_CONF_RSP     0x11

/** Update Request */
#define BLE_L2CAP_SIG_OP_UPDATE_REQ             0x12

/** Update Response */
#define BLE_L2CAP_SIG_OP_UPDATE_RSP             0x13

/** LE Credit Based Connection Request */
#define BLE_L2CAP_SIG_OP_LE_CREDIT_CONNECT_REQ  0x14

/** LE Credit Based Connection Response */
#define BLE_L2CAP_SIG_OP_LE_CREDIT_CONNECT_RSP  0x15

/** Credit Based Flow Control Credit */
#define BLE_L2CAP_SIG_OP_FLOW_CTRL_CREDIT       0x16

/** Credit Based Connection Request */
#define BLE_L2CAP_SIG_OP_CREDIT_CONNECT_REQ     0x17

/** Credit Based Connection Response */
#define BLE_L2CAP_SIG_OP_CREDIT_CONNECT_RSP     0x18

/** Credit Based Reconfiguration Request */
#define BLE_L2CAP_SIG_OP_CREDIT_RECONFIG_REQ    0x19

/** Credit Based Reconfiguration Response */
#define BLE_L2CAP_SIG_OP_CREDIT_RECONFIG_RSP    0x1A

/** Signaling Command Maximum Value */
#define BLE_L2CAP_SIG_OP_MAX                    0x1B

/** @} */

/**
 * @defgroup ble_l2cap_signaling_err_codes Signaling Commands Error Codes
 * @{
 */

/** Command Not Understood */
#define BLE_L2CAP_SIG_ERR_CMD_NOT_UNDERSTOOD    0x0000

/** Maximum Transmission Unit (MTU) Exceeded */
#define BLE_L2CAP_SIG_ERR_MTU_EXCEEDED          0x0001

/** Invalid CID */
#define BLE_L2CAP_SIG_ERR_INVALID_CID           0x0002

/** @} */

/**
 * @defgroup ble_l2cap_coc_err_codes Connection-Oriented Channels (CoC) Error Codes
 * @{
 */

/** Connection Success */
#define BLE_L2CAP_COC_ERR_CONNECTION_SUCCESS        0x0000

/** Unknown LE Protocol/Service Multiplexer (PSM) */
#define BLE_L2CAP_COC_ERR_UNKNOWN_LE_PSM            0x0002

/** No Resources */
#define BLE_L2CAP_COC_ERR_NO_RESOURCES              0x0004

/** Insufficient Authentication */
#define BLE_L2CAP_COC_ERR_INSUFFICIENT_AUTHEN       0x0005

/** Insufficient Authorization */
#define BLE_L2CAP_COC_ERR_INSUFFICIENT_AUTHOR       0x0006

/** Insufficient Key Size */
#define BLE_L2CAP_COC_ERR_INSUFFICIENT_KEY_SZ       0x0007

/** Insufficient Encryption */
#define BLE_L2CAP_COC_ERR_INSUFFICIENT_ENC          0x0008

/** Invalid Source CID */
#define BLE_L2CAP_COC_ERR_INVALID_SOURCE_CID        0x0009

/** Source CID Already Used */
#define BLE_L2CAP_COC_ERR_SOURCE_CID_ALREADY_USED   0x000A

/** Unacceptable Parameters */
#define BLE_L2CAP_COC_ERR_UNACCEPTABLE_PARAMETERS   0x000B

/** Invalid Parameters */
#define BLE_L2CAP_COC_ERR_INVALID_PARAMETERS        0x000C

/** @} */

/**
 * @defgroup ble_l2cap_reconfig_err_codes Channel Reconfiguration Error Codes
 * @{
 */

/** Reconfiguration Succeeded */
#define BLE_L2CAP_ERR_RECONFIG_SUCCEED                       0x0000

/** Reduction of Maximum Transmission Unit (MTU) Not Allowed */
#define BLE_L2CAP_ERR_RECONFIG_REDUCTION_MTU_NOT_ALLOWED     0x0001

/** Reduction of Maximum Packet Size (MPS) Not Allowed */
#define BLE_L2CAP_ERR_RECONFIG_REDUCTION_MPS_NOT_ALLOWED     0x0002

/** Invalid Destination CID */
#define BLE_L2CAP_ERR_RECONFIG_INVALID_DCID                  0x0003

/** Unaccepted Parameters */
#define BLE_L2CAP_ERR_RECONFIG_UNACCEPTED_PARAM              0x0004

/** @} */

/**
 * @defgroup ble_l2cap_coc_event_types Connection-Oriented Channel (CoC) Event Types
 * @{
 */

/** CoC Connected */
#define BLE_L2CAP_EVENT_COC_CONNECTED                 0

/** CoC Disconnected */
#define BLE_L2CAP_EVENT_COC_DISCONNECTED              1

/** CoC Accept */
#define BLE_L2CAP_EVENT_COC_ACCEPT                    2

/** CoC Data Received */
#define BLE_L2CAP_EVENT_COC_DATA_RECEIVED             3

/** CoC Transmission Unstalled */
#define BLE_L2CAP_EVENT_COC_TX_UNSTALLED              4

/** CoC Reconfiguration Completed */
#define BLE_L2CAP_EVENT_COC_RECONFIG_COMPLETED        5

/** CoC Peer Reconfigured  */
#define BLE_L2CAP_EVENT_COC_PEER_RECONFIGURED         6

/** @} */


/**
 * @brief Function signature for L2CAP signaling update event callback.
 *
 * This function is used to handle signaling update events in the L2CAP layer,
 * such as changes in connection parameters.
 *
 * @param conn_handle   The connection handle associated with the signaling update event.
 * @param status        The status of the signaling update event.
 * @param arg           A pointer to additional arguments passed to the callback function.
 */
typedef void ble_l2cap_sig_update_fn(uint16_t conn_handle, int status,
                                     void *arg);

/**
 * @brief Represents the signaling update in L2CAP.
 *
 * This structure holds the parameters required for initiating a signaling update in the L2CAP layer.
 * It defines the connection interval, slave latency, and supervision timeout multiplier for the update.
 */
struct ble_l2cap_sig_update_params {
    /**
     * The minimum desired connection interval in increments of 1.25 ms.
     * This value defines the lower bound for the connection interval range.
     */
    uint16_t itvl_min;

    /**
     * The maximum desired connection interval in increments of 1.25 ms.
     * This value defines the upper bound for the connection interval range.
     */
    uint16_t itvl_max;

    /**
     * The desired number of connection events that a slave device can skip.
     * It specifies the maximum allowed latency between consecutive connection events.
     */
    uint16_t slave_latency;

    /**
     * The desired supervision timeout multiplier.
     * The supervision timeout defines the time limit for detecting the loss of a connection.
     * This value is multiplied by the connection interval to determine the supervision timeout duration.
     */
    uint16_t timeout_multiplier;
};

/**
 * @brief Initiate an L2CAP connection update procedure.
 *
 * This function initiates an L2CAP connection update procedure for the specified connection handle.
 * The update procedure is used to modify the connection parameters, such as interval, latency, and timeout.
 *
 * @param conn_handle   The connection handle of the L2CAP connection.
 *
 * @param params        A pointer to a structure containing the desired update parameters.
 *                      This includes the new connection interval, slave latency, and
 *                      supervision timeout multiplier.
 *
 * @param cb            The callback function to be called when the update request completes.
 *                      The function signature for the callback is defined by `ble_l2cap_sig_update_fn`.
 *
 * @param cb_arg        An optional argument to be passed to the callback function.
 *
 * @return              0 on success;
 *                      A non-zero value on failure.
 */
int ble_l2cap_sig_update(uint16_t conn_handle,
                         struct ble_l2cap_sig_update_params *params,
                         ble_l2cap_sig_update_fn *cb, void *cb_arg);

/**
 * @brief Structure representing a L2CAP channel.
 *
 * It is used to maintain the state and track the properties of an L2CAP channel
 * during its lifecycle.
 */
struct ble_l2cap_chan;

/**
 * @brief Represents a L2CAP-related event.
 *
 * When such an event occurs, the host notifies the application by passing an
 * instance of this structure to an application-specified callback.
 */
struct ble_l2cap_event {
    /**
     * Indicates the type of L2CAP event that occurred.  This is one of the
     * BLE_L2CAP_EVENT codes.
     */
    uint8_t type;

    /**
     * A discriminated union containing additional details concerning the L2CAP
     * event.  The 'type' field indicates which member of the union is valid.
     */
    union {
        /**
         * Represents a connection attempt. Valid for the following event
         * types:
         *     o BLE_L2CAP_EVENT_COC_CONNECTED */
        struct {
            /**
             * The status of the connection attempt;
             *     o 0: the connection was successfully established.
             *     o BLE host error code: the connection attempt failed for
             *       the specified reason.
             */
            int status;

            /** Connection handle of the relevant connection */
            uint16_t conn_handle;

            /** The L2CAP channel of the relevant L2CAP connection. */
            struct ble_l2cap_chan *chan;
        } connect;

        /**
         * Represents a terminated connection. Valid for the following event
         * types:
         *     o BLE_L2CAP_EVENT_COC_DISCONNECTED
         */
        struct {
            /** Connection handle of the relevant connection */
            uint16_t conn_handle;

            /** Information about the L2CAP connection prior to termination. */
            struct ble_l2cap_chan *chan;
        } disconnect;

        /**
         * Represents connection accept. Valid for the following event
         * types:
         *     o BLE_L2CAP_EVENT_COC_ACCEPT
         */
        struct {
            /** Connection handle of the relevant connection */
            uint16_t conn_handle;

            /** MTU supported by peer device on the channel */
            uint16_t peer_sdu_size;

            /** The L2CAP channel of the relevant L2CAP connection. */
            struct ble_l2cap_chan *chan;
        } accept;

        /**
         * Represents received data. Valid for the following event
         * types:
         *     o BLE_L2CAP_EVENT_COC_DATA_RECEIVED
         */
        struct {
            /** Connection handle of the relevant connection */
            uint16_t conn_handle;

            /** The L2CAP channel of the relevant L2CAP connection. */
            struct ble_l2cap_chan *chan;

            /** The mbuf with received SDU. */
            struct os_mbuf *sdu_rx;
        } receive;

        /**
         * Represents tx_unstalled data. Valid for the following event
         * types:
         *     o BLE_L2CAP_EVENT_COC_TX_UNSTALLED
         */
        struct {
            /** Connection handle of the relevant connection */
            uint16_t conn_handle;

            /** The L2CAP channel of the relevant L2CAP connection. */
            struct ble_l2cap_chan *chan;

            /**
             * The status of the send attempt which was stalled due to
             * lack of credits; This can be non zero only if there
             * is an issue with memory allocation for following SDU fragments.
             * In such a case last SDU has been partially sent to peer device
             * and it is up to application to decide how to handle it.
             */
            int status;
        } tx_unstalled;

        /**
         * Represents reconfiguration done. Valid for the following event
         * types:
         *      o BLE_L2CAP_EVENT_COC_RECONFIG_COMPLETED
         *      o BLE_L2CAP_EVENT_COC_PEER_RECONFIGURED
         */
        struct {
            /**
             * The status of the reconfiguration attempt;
             *     o 0: the reconfiguration was successfully done.
             *     o BLE host error code: the reconfiguration attempt failed for
             *       the specified reason.
             */
            int status;

            /** Connection handle of the relevant connection */
            uint16_t conn_handle;

            /** The L2CAP channel of the relevant L2CAP connection. */
            struct ble_l2cap_chan *chan;
        } reconfigured;
    };
};

/**
 * @brief Represents information about an L2CAP channel.
 *
 *  This structure is typically used to retrieve or provide information about an existing L2CAP channel.
 */
struct ble_l2cap_chan_info {
    /** Source Channel Identifier. */
    uint16_t scid;

    /** Destination Channel Identifier. */
    uint16_t dcid;

    /** Local L2CAP Maximum Transmission Unit. */
    uint16_t our_l2cap_mtu;

    /** Peer L2CAP Maximum Transmission Unit. */
    uint16_t peer_l2cap_mtu;

    /** Protocol/Service Multiplexer of the channel. */
    uint16_t psm;

    /** Local CoC Maximum Transmission Unit. */
    uint16_t our_coc_mtu;

    /** Peer CoC Maximum Transmission Unit. */
    uint16_t peer_coc_mtu;
};

/**
 * @brief Function pointer type for handling L2CAP events.
 *
 * @param event     A pointer to the L2CAP event structure.
 * @param arg       A pointer to additional arguments passed to the callback function.
 *
 * @return          Integer value representing the status or result of the event handling.
 */
typedef int ble_l2cap_event_fn(struct ble_l2cap_event *event, void *arg);

/**
 * @brief Get the connection handle associated with an L2CAP channel.
 *
 * This function retrieves the connection handle associated with the specified L2CAP channel.
 *
 * @param chan      A pointer to the L2CAP channel structure.
 *
 * @return          The connection handle associated with the L2CAP channel on success;
 *                  A Bluetooth Host Error Code on failure:
 *                  BLE_HS_CONN_HANDLE_NONE: if the provided channel pointer is NULL.
 */
uint16_t ble_l2cap_get_conn_handle(struct ble_l2cap_chan *chan);

/**
 * @brief Create an L2CAP server.
 *
 * This function creates an L2CAP server with the specified Protocol/Service Multiplexer (PSM) and Maximum
 * Transmission Unit (MTU) size. The server is used to accept incoming L2CAP connections from remote clients.
 * When a connection request is received, the provided callback function will be invoked with the corresponding
 * event information.
 *
 * @param psm       The Protocol/Service Multiplexer (PSM) for the server.
 * @param mtu       The Maximum Transmission Unit (MTU) size for the server.
 * @param cb        Pointer to the callback function to be invoked when a connection request is received.
 * @param cb_arg    An optional argument to be passed to the callback function.
 *
 * @return          0 on success;
 *                  A non-zero value on failure.
 */
int ble_l2cap_create_server(uint16_t psm, uint16_t mtu,
                            ble_l2cap_event_fn *cb, void *cb_arg);

/**
 * @brief Initiate an L2CAP connection.
 *
 * This function initiates an L2CAP connection to a remote device with the specified connection handle,
 * Protocol/Service Multiplexer (PSM), Maximum Transmission Unit (MTU) size, and receive SDU buffer.
 * When the connection is established or if there is an error during the connection process, the provided
 * callback function will be invoked with the corresponding event information.
 *
 * @param conn_handle   The connection handle for the remote device.
 * @param psm           The Protocol/Service Multiplexer (PSM) for the connection.
 * @param mtu           The Maximum Transmission Unit (MTU) size for the connection.
 * @param sdu_rx        Pointer to the receive Service Data Unit (SDU) buffer.
 * @param cb            Pointer to the callback function to be invoked when the connection is established.
 * @param cb_arg        An optional argument to be passed to the callback function.
 *
 * @return              0 on success;
 *                      A non-zero value on failure.
 */
int ble_l2cap_connect(uint16_t conn_handle, uint16_t psm, uint16_t mtu,
                      struct os_mbuf *sdu_rx,
                      ble_l2cap_event_fn *cb, void *cb_arg);

/**
 * @brief Disconnect an L2CAP channel.
 *
 * This function disconnects the specified L2CAP channel by sending a disconnect signal.
 *
 * @param chan          Pointer to the L2CAP channel structure representing the channel to disconnect.
 *
 * @return              0 on success;
 *                      A non-zero value on failure.
 */
int ble_l2cap_disconnect(struct ble_l2cap_chan *chan);

/**
 * @brief Send an SDU (Service Data Unit) over an L2CAP channel.
 *
 * This function sends an SDU over the specified L2CAP channel. The SDU is encapsulated
 * in L2CAP frames and transmitted to the remote device.
 *
 * @param chan          Pointer to the L2CAP channel structure representing the channel to send the SDU on.
 * @param sdu_tx        Pointer to the os_mbuf structure containing the SDU (Service Data Unit) to send.
 *
 * @return              0 on success;
 *                      BLE_HS_ESTALLED: if there was not enough credits available to send whole SDU.
 *                      The application needs to wait for the event 'BLE_L2CAP_EVENT_COC_TX_UNSTALLED'
 *                      before being able to transmit more data;
 *                      Another non-zero value on failure.
 */
int ble_l2cap_send(struct ble_l2cap_chan *chan, struct os_mbuf *sdu_tx);

/**
 * @brief Check if the L2CAP channel is ready to receive an SDU.
 *
 * This function checks if the specified L2CAP channel is ready to receive an SDU (Service Data Unit).
 * It can be used to determine if the channel is in a state where it can accept incoming data.
 *
 * @param chan          Pointer to the L2CAP channel structure to check.
 * @param sdu_rx        Pointer to the os_mbuf structure to receive the incoming SDU.
 *
 * @return              0 if the channel is ready to receive an SDU;
 *                      A non-zero value on failure.
 */
int ble_l2cap_recv_ready(struct ble_l2cap_chan *chan, struct os_mbuf *sdu_rx);

/**
 * @brief Get information about an L2CAP channel.
 *
 * This function retrieves information about the specified L2CAP channel and populates
 * the provided `ble_l2cap_chan_info` structure with the channel's details.
 *
 * @param chan          Pointer to the L2CAP channel structure to retrieve information from.
 * @param chan_info     Pointer to the `ble_l2cap_chan_info` structure to populate with channel information.
 *
 * @return              0 on success;
 *                      A non-zero value on failure.
 */
int ble_l2cap_get_chan_info(struct ble_l2cap_chan *chan, struct ble_l2cap_chan_info *chan_info);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
