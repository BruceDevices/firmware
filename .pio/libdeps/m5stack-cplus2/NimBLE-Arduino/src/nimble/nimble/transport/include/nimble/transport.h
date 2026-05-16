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

#ifndef H_NIMBLE_TRANSPORT_
#define H_NIMBLE_TRANSPORT_

#ifdef __cplusplus
extern "C" {
#endif

#include <nimble/nimble/transport/include/nimble/transport_impl.h>
#include <nimble/nimble/transport/include/nimble/transport/monitor.h>
#if MYNEWT_PKG_apache_mynewt_nimble__nimble_transport_common_hci_ipc
#include <nimble/nimble/transport/include/nimble/transport/transport_ipc.h>
#endif

#ifdef ESP_PLATFORM

#include <inttypes.h>
#include "nimble/porting/nimble/include/os/os_mempool.h"

#define BLE_HCI_TRANS_CMD_SZ        260
/*** Type of buffers for holding commands and events. */
/**
 * Controller-to-host event buffers.  Events have one of two priorities:
 * o Low-priority   (BLE_HCI_TRANS_BUF_EVT_LO)
 * o High-priority  (BLE_HCI_TRANS_BUF_EVT_HI)
 *
 * Low-priority event buffers are only used for advertising reports.  If there
 * are no free low-priority event buffers, then an incoming advertising report
 * will get dropped.
 *
 * High-priority event buffers are for everything except advertising reports.
 * If there are no free high-priority event buffers, a request to allocate one
 * will try to allocate a low-priority buffer instead.
 *
 * If you want all events to be given equal treatment, then you should allocate
 * low-priority events only.
 *
 * Event priorities solve the problem of critical events getting dropped due to
 * a flood of advertising reports.  This solution is likely temporary: when
 * HCI flow control is added, event priorities may become obsolete.
 *
 * Not all transports distinguish between low and high priority events.  If the
 * transport does not have separate settings for low and high buffer counts,
 * then it treats all events with equal priority.
 */
#define BLE_HCI_TRANS_BUF_EVT_LO    1
#define BLE_HCI_TRANS_BUF_EVT_HI    2

/* Host-to-controller command. */
#define BLE_HCI_TRANS_BUF_CMD       3

void ble_transport_init(void);

esp_err_t ble_buf_alloc(void);

void ble_buf_free(void);

void ble_transport_deinit(void);

/** Callback function types; executed when HCI packets are received. */
typedef int ble_hci_trans_rx_cmd_fn(uint8_t *cmd, void *arg);
typedef int ble_hci_trans_rx_acl_fn(struct os_mbuf *om, void *arg);

/**
 * Sends an HCI event from the controller to the host.
 *
 * @param cmd                   The HCI event to send.  This buffer must be
 *                                  allocated via ble_hci_trans_buf_alloc().
 *
 * @return                      0 on success;
 *                              A BLE_ERR_[...] error code on failure.
 */
int ble_hci_trans_ll_evt_tx(uint8_t *hci_ev);

/**
 * Sends ACL data from controller to host.
 *
 * @param om                    The ACL data packet to send.
 *
 * @return                      0 on success;
 *                              A BLE_ERR_[...] error code on failure.
 */
int ble_hci_trans_ll_acl_tx(struct os_mbuf *om);

/**
 * Sends an HCI command from the host to the controller.
 *
 * @param cmd                   The HCI command to send.  This buffer must be
 *                                  allocated via ble_hci_trans_buf_alloc().
 *
 * @return                      0 on success;
 *                              A BLE_ERR_[...] error code on failure.
 */
int ble_hci_trans_hs_cmd_tx(uint8_t *cmd);

/**
 * Sends ACL data from host to controller.
 *
 * @param om                    The ACL data packet to send.
 *
 * @return                      0 on success;
 *                              A BLE_ERR_[...] error code on failure.
 */
int ble_hci_trans_hs_acl_tx(struct os_mbuf *om);

/**
 * Allocates a flat buffer of the specified type.
 *
 * @param type                  The type of buffer to allocate; one of the
 *                                  BLE_HCI_TRANS_BUF_[...] constants.
 *
 * @return                      The allocated buffer on success;
 *                              NULL on buffer exhaustion.
 */
int esp_ble_hci_trans_hs_cmd_tx(uint8_t *cmd);

/**
 * Sends ACL data from host to controller.
 *
 * @param om                    The ACL data packet to send.
 *
 * @return                      0 on success;
 *                              A BLE_ERR_[...] error code on failure.
 */
int esp_ble_hci_trans_hs_acl_tx(struct os_mbuf *om);

/**
 * Allocates a flat buffer of the specified type.
 *
 * @param type                  The type of buffer to allocate; one of the
 *                                  BLE_HCI_TRANS_BUF_[...] constants.
 *
 * @return                      The allocated buffer on success;
 *                              NULL on buffer exhaustion.
 */
uint8_t *esp_ble_hci_trans_buf_alloc(int type);

/**
 * Frees the specified flat buffer.  The buffer must have been allocated via
 * ble_hci_trans_buf_alloc().
 *
 * @param buf                   The buffer to free.
 */
void esp_ble_hci_trans_buf_free(uint8_t *buf);

/**
 * Configures the HCI transport to operate with a host.  The transport will
 * execute specified callbacks upon receiving HCI packets from the controller.
 *
 * @param evt_cb                The callback to execute upon receiving an HCI
 *                                  event.
 * @param evt_arg               Optional argument to pass to the event
 *                                  callback.
 * @param acl_cb                The callback to execute upon receiving ACL
 *                                  data.
 * @param acl_arg               Optional argument to pass to the ACL
 *                                  callback.
 */
void esp_ble_hci_trans_cfg_hs(ble_hci_trans_rx_cmd_fn *evt_cb,
                          void *evt_arg,
                          ble_hci_trans_rx_acl_fn *acl_cb,
                          void *acl_arg);

/**
 * Resets the HCI module to a clean state.  Frees all buffers and reinitializes
 * the underlying transport.
 *
 * @return                      0 on success;
 *                              A BLE_ERR_[...] error code on failure.
 */
int esp_ble_hci_trans_reset(void);

#endif /* ESP_PLATFORM */

struct os_mbuf;

/* Initialization */
void ble_transport_init(void);

/* Allocators for supported data types */
void *ble_transport_alloc_cmd(void);
void *ble_transport_alloc_evt(int discardable);
struct os_mbuf *ble_transport_alloc_acl_from_hs(void);
struct os_mbuf *ble_transport_alloc_iso_from_hs(void);
struct os_mbuf *ble_transport_alloc_acl_from_ll(void);
struct os_mbuf *ble_transport_alloc_iso_from_ll(void);

/* Generic deallocator for cmd/evt buffers */
void ble_transport_free(void *buf);

/* Register put callback on acl_from_ll mbufs (for ll-hs flow control) */
int ble_transport_register_put_acl_from_ll_cb(os_mempool_put_fn *cb);


int ble_transport_to_ll_cmd(void *buf);
int ble_transport_to_ll_acl(struct os_mbuf *om);

/* Send data to hs/ll side */
int ble_transport_to_ll_iso(struct os_mbuf *om);
int ble_transport_to_hs_evt(void *buf);
int ble_transport_to_hs_acl(struct os_mbuf *om);
int ble_transport_to_hs_iso(struct os_mbuf *om);

#ifdef __cplusplus
}
#endif

#endif /* H_NIMBLE_TRANSPORT_ */
