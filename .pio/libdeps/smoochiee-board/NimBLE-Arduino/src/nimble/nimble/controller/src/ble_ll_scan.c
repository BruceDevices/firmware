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

#ifndef ESP_PLATFORM

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "nimble/porting/nimble/include/syscfg/syscfg.h"
#include "nimble/porting/nimble/include/os/os.h"
#include "nimble/nimble/include/nimble/ble.h"
#include "nimble/nimble/include/nimble/hci_common.h"
#include "nimble/nimble/controller/include/controller/ble_phy.h"
#include "nimble/nimble/controller/include/controller/ble_hw.h"
#include "nimble/nimble/controller/include/controller/ble_ll.h"
#include "nimble/nimble/controller/include/controller/ble_ll_sched.h"
#include "nimble/nimble/controller/include/controller/ble_ll_scan.h"
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
#include "nimble/nimble/controller/include/controller/ble_ll_scan_aux.h"
#endif
#include "nimble/nimble/controller/include/controller/ble_ll_tmr.h"
#include "nimble/nimble/controller/include/controller/ble_ll_hci.h"
#include "nimble/nimble/controller/include/controller/ble_ll_whitelist.h"
#include "nimble/nimble/controller/include/controller/ble_ll_resolv.h"
#include "nimble/nimble/controller/include/controller/ble_ll_rfmgmt.h"
#include "nimble/nimble/controller/include/controller/ble_ll_trace.h"
#include "nimble/nimble/controller/include/controller/ble_ll_sync.h"
#include "ble_ll_conn_priv.h"
#include "ble_ll_priv.h"

#if MYNEWT_VAL(BLE_LL_ROLE_OBSERVER)

/*
 * XXX:
 * 1) I think I can guarantee that we dont process things out of order if
 * I send an event when a scan request is sent. The scan_rsp_pending flag
 * code might be made simpler.
 *
 * 2) Interleave sending scan requests to different advertisers? I guess I need
 * a list of advertisers to which I sent a scan request and have yet to
 * receive a scan response from? Implement this.
 */

/* Dont allow more than 255 of these entries */
#if MYNEWT_VAL(BLE_LL_NUM_SCAN_RSP_ADVS) > 255
    #error "Cannot have more than 255 scan response entries!"
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CODED_PHY)
#define SCAN_VALID_PHY_MASK     (BLE_HCI_LE_PHY_1M_PREF_MASK | BLE_HCI_LE_PHY_CODED_PREF_MASK)
#else
#define SCAN_VALID_PHY_MASK     (BLE_HCI_LE_PHY_1M_PREF_MASK)
#endif

struct ble_ll_scan_params {
    uint8_t own_addr_type;
    uint8_t scan_filt_policy;
    struct ble_ll_scan_phy scan_phys[BLE_LL_SCAN_PHY_NUMBER];
};

static struct ble_ll_scan_params g_ble_ll_scan_params;

/* The scanning state machine global object */
static struct ble_ll_scan_sm g_ble_ll_scan_sm;

/*
 * Structure used to store advertisers. This is used to limit sending scan
 * requests to the same advertiser and also to filter duplicate events sent
 * to the host.
 */
struct ble_ll_scan_advertisers
{
    uint16_t            sc_adv_flags;
    uint16_t            adi;
    struct ble_dev_addr adv_addr;
};

#define BLE_LL_SC_ADV_F_RANDOM_ADDR     (0x01)
#define BLE_LL_SC_ADV_F_SCAN_RSP_RXD    (0x02)
#define BLE_LL_SC_ADV_F_DIRECT_RPT_SENT (0x04)
#define BLE_LL_SC_ADV_F_ADV_RPT_SENT    (0x08)
#define BLE_LL_SC_ADV_F_SCAN_RSP_SENT   (0x10)

/* Contains list of advertisers that we have heard scan responses from */
static uint8_t g_ble_ll_scan_num_rsp_advs;
struct ble_ll_scan_advertisers
g_ble_ll_scan_rsp_advs[MYNEWT_VAL(BLE_LL_NUM_SCAN_RSP_ADVS)];

/* Duplicates filtering data */
#define BLE_LL_SCAN_ENTRY_TYPE_LEGACY(addr_type) \
    ((addr_type) & 1)
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
#define BLE_LL_SCAN_ENTRY_TYPE_EXT(addr_type, has_aux, is_anon, adi) \
    (((adi >> 8) & 0xF0) | (1 << 3) | (is_anon << 2) | (has_aux << 1) | ((addr_type) & 1))
#endif

#define BLE_LL_SCAN_DUP_F_ADV_REPORT_SENT       (0x01)
#define BLE_LL_SCAN_DUP_F_DIR_ADV_REPORT_SENT   (0x02)
#define BLE_LL_SCAN_DUP_F_SCAN_RSP_SENT         (0x04)

struct ble_ll_scan_dup_entry {
    uint8_t type;       /* entry type, see BLE_LL_SCAN_ENTRY_TYPE_* */
    uint8_t addr[6];
    uint8_t flags;      /* use BLE_LL_SCAN_DUP_F_xxx */
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    uint16_t adi;
#endif
    TAILQ_ENTRY(ble_ll_scan_dup_entry) link;
};

static os_membuf_t g_scan_dup_mem[ OS_MEMPOOL_SIZE(
                                   MYNEWT_VAL(BLE_LL_NUM_SCAN_DUP_ADVS),
                                   sizeof(struct ble_ll_scan_dup_entry)) ];
static struct os_mempool g_scan_dup_pool;
static TAILQ_HEAD(ble_ll_scan_dup_list, ble_ll_scan_dup_entry) g_scan_dup_list;

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
static int
ble_ll_scan_start(struct ble_ll_scan_sm *scansm);

#endif

static inline uint32_t
ble_ll_scan_time_hci_to_ticks(uint16_t value)
{
    return ble_ll_tmr_u2t(value * BLE_HCI_SCAN_ITVL);
}

/* See Vol 6 Part B Section 4.4.3.2. Active scanning backoff */
static void
ble_ll_scan_req_backoff(struct ble_ll_scan_sm *scansm, int success)
{
    BLE_LL_ASSERT(scansm->backoff_count == 0);
    BLE_LL_ASSERT(scansm->scan_rsp_pending == 0);

    if (success) {
        scansm->scan_rsp_cons_fails = 0;
        ++scansm->scan_rsp_cons_ok;
        if (scansm->scan_rsp_cons_ok == 2) {
            scansm->scan_rsp_cons_ok = 0;
            if (scansm->upper_limit > 1) {
                scansm->upper_limit >>= 1;
            }
        }
        STATS_INC(ble_ll_stats, scan_req_txg);
    } else {
        scansm->scan_rsp_cons_ok = 0;
        ++scansm->scan_rsp_cons_fails;
        if (scansm->scan_rsp_cons_fails == 2) {
            scansm->scan_rsp_cons_fails = 0;
            if (scansm->upper_limit < 256) {
                scansm->upper_limit <<= 1;
            }
        }
        STATS_INC(ble_ll_stats, scan_req_txf);
    }

    scansm->backoff_count = ble_ll_rand() & (scansm->upper_limit - 1);
    ++scansm->backoff_count;
    BLE_LL_ASSERT(scansm->backoff_count <= 256);
}

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
static void
ble_ll_scan_refresh_nrpa(struct ble_ll_scan_sm *scansm)
{
    ble_npl_time_t now;

    now = ble_npl_time_get();
    if (LL_TMR_GEQ(now, scansm->scan_nrpa_timer)) {
        /* Generate new NRPA */
        ble_ll_rand_data_get(scansm->scan_nrpa, BLE_DEV_ADDR_LEN);
        scansm->scan_nrpa[5] &= ~0xc0;

        /* We'll use the same timeout as for RPA rotation */
        scansm->scan_nrpa_timer = now + ble_ll_resolv_get_rpa_tmo();
    }
}

uint8_t *
ble_ll_get_scan_nrpa(void)
{
    struct ble_ll_scan_sm *scansm = &g_ble_ll_scan_sm;

    ble_ll_scan_refresh_nrpa(scansm);

    return scansm->scan_nrpa;
}
#endif

uint8_t
ble_ll_scan_get_own_addr_type(void)
{
    return g_ble_ll_scan_sm.own_addr_type;
}

uint8_t
ble_ll_scan_get_filt_policy(void)
{
    return g_ble_ll_scan_sm.scan_filt_policy;
}

uint8_t
ble_ll_scan_get_filt_dups(void)
{
    return g_ble_ll_scan_sm.scan_filt_dups;
}

uint8_t
ble_ll_scan_backoff_kick(void)
{
    struct ble_ll_scan_sm *scansm = &g_ble_ll_scan_sm;

    if (scansm->backoff_count > 0) {
        scansm->backoff_count--;
    }

    return scansm->backoff_count;
}

void
ble_ll_scan_backoff_update(int success)
{
    struct ble_ll_scan_sm *scansm = &g_ble_ll_scan_sm;

    ble_ll_scan_req_backoff(scansm, success);
}

static void
ble_ll_scan_req_pdu_prepare(struct ble_ll_scan_sm *scansm,
                            const uint8_t *adv_addr, uint8_t adv_addr_type,
                            int8_t rpa_index)
{
    uint8_t hdr_byte;
    struct ble_ll_scan_pdu_data *pdu_data;
    uint8_t *scana;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
    struct ble_ll_resolv_entry *rl;
    uint8_t rpa[BLE_DEV_ADDR_LEN];
#endif

    pdu_data = &scansm->pdu_data;

    /* Construct first PDU header byte */
    hdr_byte = BLE_ADV_PDU_TYPE_SCAN_REQ;
    if (adv_addr_type) {
        hdr_byte |= BLE_ADV_PDU_HDR_RXADD_RAND;
    }

    /* Determine ScanA */
    if (scansm->own_addr_type & 0x01) {
        hdr_byte |= BLE_ADV_PDU_HDR_TXADD_RAND;
        scana = g_random_addr;
    } else {
        scana = g_dev_addr;
    }

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
    if (scansm->own_addr_type & 0x02) {
        if (rpa_index >= 0) {
            rl = &g_ble_ll_resolv_list[rpa_index];
        } else {
            rl = NULL;
        }

        /*
         * If device is on RL and we have local IRK, we use RPA generated using
         * that IRK as ScanA. Otherwise we use NRPA as ScanA to prevent our
         * device from being tracked when doing an active scan (Core 5.1, Vol 6,
         * Part B, section 6.3)
         */
        if (rl && rl->rl_has_local) {
            ble_ll_resolv_get_priv_addr(rl, 1, rpa);
            scana = rpa;
        } else {
            ble_ll_scan_refresh_nrpa(scansm);
            scana = scansm->scan_nrpa;
        }

        hdr_byte |= BLE_ADV_PDU_HDR_TXADD_RAND;
    }
#endif

    /* Save scan request data */
    pdu_data->hdr_byte = hdr_byte;
    memcpy(pdu_data->scana, scana, BLE_DEV_ADDR_LEN);
    memcpy(pdu_data->adva, adv_addr, BLE_DEV_ADDR_LEN);
}

static uint8_t
ble_ll_scan_req_tx_pdu_cb(uint8_t *dptr, void *pducb_arg, uint8_t *hdr_byte)
{
    struct ble_ll_scan_sm *scansm = pducb_arg;
    struct ble_ll_scan_pdu_data *pdu_data = &scansm->pdu_data;

    memcpy(dptr, pdu_data->scana, BLE_DEV_ADDR_LEN);
    memcpy(dptr + BLE_DEV_ADDR_LEN, pdu_data->adva, BLE_DEV_ADDR_LEN);

    *hdr_byte = pdu_data->hdr_byte;

    return BLE_DEV_ADDR_LEN * 2;
}

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
/* if copy_from is provided new report is initialized with that instead of
 * defaults
 */
static struct ble_hci_ev *
ble_ll_scan_get_ext_adv_report(struct ext_adv_report *copy_from)
{
    struct ble_hci_ev_le_subev_ext_adv_rpt *ev;
    struct ext_adv_report *report;
    struct ble_hci_ev *hci_ev;

    hci_ev = ble_transport_alloc_evt(1);
    if (!hci_ev) {
        return NULL;
    }

    hci_ev->opcode = BLE_HCI_EVCODE_LE_META;
    hci_ev->length = sizeof(*ev) + sizeof(*report);
    ev = (void *) hci_ev->data;

    memset(ev, 0, sizeof(*ev));
    ev->subev_code = BLE_HCI_LE_SUBEV_EXT_ADV_RPT;
    /* We support only one report per event now */
    ev->num_reports = 1;

    report = ev->reports;

    if (copy_from) {
        memcpy(report, copy_from, sizeof(*report));
        report->data_len = 0;
    } else {
        memset(report, 0, sizeof(*report));

        report->pri_phy = BLE_PHY_1M;
        /* Init SID with "Not available" which is 0xFF */
        report->sid = 0xFF;
        /* Init TX Power with "Not available" which is 127 */
        report->tx_power = 127;
        /* Init RSSI with "Not available" which is 127 */
        report->rssi = 127;
        /* Init address type with "anonymous" which is 0xFF */
        report->addr_type = 0xFF;
    }

    return hci_ev;
}
#endif

void
ble_ll_scan_halt(void)
{
    struct ble_ll_scan_sm *scansm = &g_ble_ll_scan_sm;

    /* Update backoff if we failed to receive scan response */
    if (scansm->scan_rsp_pending) {
        scansm->scan_rsp_pending = 0;
        ble_ll_scan_req_backoff(scansm, 0);
    }
}

/**
 * Checks to see if we have received a scan response from this advertiser.
 *
 * @param adv_addr Address of advertiser
 * @param txadd TxAdd bit (0: public; random otherwise)
 *
 * @return int 0: have not received a scan response; 1 otherwise.
 */
int
ble_ll_scan_have_rxd_scan_rsp(uint8_t *addr, uint8_t txadd,
                              uint8_t ext_adv, uint16_t adi)
{
    uint8_t num_advs;
    struct ble_ll_scan_advertisers *adv;

    /* Do we have an address match? Must match address type */
    adv = &g_ble_ll_scan_rsp_advs[0];
    num_advs = g_ble_ll_scan_num_rsp_advs;
    while (num_advs) {
        if (!memcmp(&adv->adv_addr, addr, BLE_DEV_ADDR_LEN)) {
            /* Address type must match */
            if (txadd) {
                if (adv->sc_adv_flags & BLE_LL_SC_ADV_F_RANDOM_ADDR) {
                    if (ext_adv) {
                        if (adi == adv->adi) {
                            return 1;
                        }
                        goto next;
                    }
                    return 1;
                }
            } else {
                if ((adv->sc_adv_flags & BLE_LL_SC_ADV_F_RANDOM_ADDR) == 0) {
                    if (ext_adv) {
                        if (adi == adv->adi) {
                            return 1;
                        }
                        goto next;
                    }
                    return 1;
                }
            }
        }
next:
        ++adv;
        --num_advs;
    }

    return 0;
}

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
void
ble_ll_scan_add_scan_rsp_adv(uint8_t *addr, uint8_t txadd,
                             uint8_t ext_adv, uint16_t adi)
{
    uint8_t num_advs;
    struct ble_ll_scan_advertisers *adv;

    /* XXX: for now, if we dont have room, just leave */
    num_advs = g_ble_ll_scan_num_rsp_advs;
    if (num_advs >= MYNEWT_VAL(BLE_LL_NUM_SCAN_RSP_ADVS)) {
        return;
    }

    /* Check if address is already on the list */
    if (ble_ll_scan_have_rxd_scan_rsp(addr, txadd, ext_adv, adi)) {
        return;
    }

    /* Add the advertiser to the array */
    adv = &g_ble_ll_scan_rsp_advs[num_advs];
    memcpy(&adv->adv_addr, addr, BLE_DEV_ADDR_LEN);
    adv->sc_adv_flags = BLE_LL_SC_ADV_F_SCAN_RSP_RXD;
    if (txadd) {
        adv->sc_adv_flags |= BLE_LL_SC_ADV_F_RANDOM_ADDR;
    }
    adv->adi = adi;
    ++g_ble_ll_scan_num_rsp_advs;

    return;
}

static int
ble_ll_hci_send_legacy_ext_adv_report(uint8_t evtype,
                                      const uint8_t *addr, uint8_t addr_type,
                                      int8_t rssi,
                                      uint8_t adv_data_len,
                                      struct os_mbuf *adv_data,
                                      const uint8_t *inita, uint8_t inita_type)
{
    struct ble_hci_ev_le_subev_ext_adv_rpt *ev;
    struct ext_adv_report *report;
    struct ble_hci_ev *hci_ev;

    if (!ble_ll_hci_is_le_event_enabled(BLE_HCI_LE_SUBEV_EXT_ADV_RPT)) {
        return -1;
    }

    /* Drop packet if adv data doesn't fit  */
    if ((sizeof(*ev) + sizeof(ev->reports[0]) + adv_data_len) > BLE_HCI_MAX_DATA_LEN) {
        STATS_INC(ble_ll_stats, adv_evt_dropped);
        return -1;
    }

    hci_ev = ble_ll_scan_get_ext_adv_report(NULL);
    if (!hci_ev) {
        return -1;
    }

    ev = (void *) hci_ev->data;
    report = ev->reports;

    switch (evtype) {
    case BLE_HCI_ADV_RPT_EVTYPE_ADV_IND:
        report->evt_type = BLE_HCI_LEGACY_ADV_EVTYPE_ADV_IND;
        break;
    case BLE_HCI_ADV_RPT_EVTYPE_DIR_IND:
        report->evt_type = BLE_HCI_LEGACY_ADV_EVTYPE_ADV_DIRECT_IND;
        break;
    case BLE_HCI_ADV_RPT_EVTYPE_NONCONN_IND:
        report->evt_type = BLE_HCI_LEGACY_ADV_EVTYPE_ADV_NONCON_IND;
        break;
    case BLE_HCI_ADV_RPT_EVTYPE_SCAN_RSP:
        report->evt_type = BLE_HCI_LEGACY_ADV_EVTYPE_SCAN_RSP_ADV_IND;
         break;
    case BLE_HCI_ADV_RPT_EVTYPE_SCAN_IND:
        report->evt_type = BLE_HCI_LEGACY_ADV_EVTYPE_ADV_SCAN_IND;
        break;
    default:
        BLE_LL_ASSERT(0);
        ble_transport_free(hci_ev);
        return -1;
    }

    report->addr_type = addr_type;
    memcpy(report->addr, addr, BLE_DEV_ADDR_LEN);
    report->pri_phy = BLE_PHY_1M;
    report->sid = 0xFF;
    report->tx_power = 127;
    report->rssi = rssi;

    if (inita) {
        report->dir_addr_type = inita_type;
        memcpy(report->dir_addr, inita, BLE_DEV_ADDR_LEN);
    }

    if (adv_data_len) {
        hci_ev->length += adv_data_len;
        report->data_len = adv_data_len;
        os_mbuf_copydata(adv_data, 0, adv_data_len, report->data);
    }

    return ble_ll_hci_event_send(hci_ev);
}
#endif

static int
ble_ll_hci_send_adv_report(uint8_t evtype,
                           const uint8_t *addr, uint8_t addr_type, int8_t rssi,
                           uint8_t adv_data_len, struct os_mbuf *adv_data)
{
    struct ble_hci_ev_le_subev_adv_rpt *ev;
    struct ble_hci_ev *hci_ev;
    int8_t *ev_rssi;

    if (!ble_ll_hci_is_le_event_enabled(BLE_HCI_LE_SUBEV_ADV_RPT)) {
        return -1;
    }

    /* Drop packet if adv data doesn't fit, note extra 1 is for RSSI   */
    if ((sizeof(*ev) + sizeof(ev->reports[0]) + adv_data_len + 1) > BLE_HCI_MAX_DATA_LEN) {
        STATS_INC(ble_ll_stats, adv_evt_dropped);
        return -1;
    }

    hci_ev = ble_transport_alloc_evt(1);
    if (!hci_ev) {
        return -1;
    }

    hci_ev->opcode = BLE_HCI_EVCODE_LE_META;
    hci_ev->length = sizeof(*ev) + sizeof(ev->reports[0]) + adv_data_len + 1;
    ev = (void *) hci_ev->data;

    ev->subev_code = BLE_HCI_LE_SUBEV_ADV_RPT;
    ev->num_reports = 1;

    ev->reports[0].type = evtype;
    ev->reports[0].addr_type = addr_type;
    memcpy(ev->reports[0].addr, addr, BLE_DEV_ADDR_LEN);
    ev->reports[0].data_len = adv_data_len;
    os_mbuf_copydata(adv_data, 0, adv_data_len, ev->reports[0].data);

    /* RSSI is after adv data... */
    ev_rssi = (int8_t *) (hci_ev->data + sizeof(*ev) + sizeof(ev->reports[0]) + adv_data_len);
    *ev_rssi = rssi;

    return ble_ll_hci_event_send(hci_ev);
}

static int
ble_ll_hci_send_dir_adv_report(const uint8_t *addr, uint8_t addr_type,
                               const uint8_t *inita, uint8_t inita_type,
                               int8_t rssi)
{
    struct ble_hci_ev_le_subev_direct_adv_rpt *ev;
    struct ble_hci_ev *hci_ev;

    if (!ble_ll_hci_is_le_event_enabled(BLE_HCI_LE_SUBEV_DIRECT_ADV_RPT)) {
        return -1;
    }

    hci_ev = ble_transport_alloc_evt(1);
    if (!hci_ev) {
        return -1;
    }

    hci_ev->opcode = BLE_HCI_EVCODE_LE_META;
    hci_ev->length = sizeof(*ev) + sizeof(*(ev->reports));
    ev = (void *) hci_ev->data;

    ev->subev_code = BLE_HCI_LE_SUBEV_DIRECT_ADV_RPT;
    ev->num_reports = 1;

    ev->reports[0].type = BLE_HCI_ADV_RPT_EVTYPE_DIR_IND;
    ev->reports[0].addr_type = addr_type;
    memcpy(ev->reports[0].addr, addr, BLE_DEV_ADDR_LEN);
    ev->reports[0].dir_addr_type = inita_type;
    memcpy(ev->reports[0].dir_addr, inita, BLE_DEV_ADDR_LEN);
    ev->reports[0].rssi = rssi;

    return ble_ll_hci_event_send(hci_ev);
}

static int
ble_ll_scan_dup_update_legacy(uint8_t addr_type, const uint8_t *addr,
                              uint8_t subev, uint8_t evtype)
{
    struct ble_ll_scan_dup_entry *e;
    uint8_t type;

    type = BLE_LL_SCAN_ENTRY_TYPE_LEGACY(addr_type);

    /*
     * We assume ble_ll_scan_dup_check() was called before which either matched
     * some entry or allocated new one and placed in on the top of queue.
     */

    e = TAILQ_FIRST(&g_scan_dup_list);
    BLE_LL_ASSERT(e && e->type == type && !memcmp(e->addr, addr, 6));

    if (subev == BLE_HCI_LE_SUBEV_DIRECT_ADV_RPT) {
        e->flags |= BLE_LL_SCAN_DUP_F_DIR_ADV_REPORT_SENT;
    } else {
        if (evtype == BLE_HCI_ADV_RPT_EVTYPE_SCAN_RSP) {
            e->flags |= BLE_LL_SCAN_DUP_F_SCAN_RSP_SENT;
        } else {
            e->flags |= BLE_LL_SCAN_DUP_F_ADV_REPORT_SENT;
        }
    }

    return 0;
}

/**
 * Send an advertising report to the host.
 *
 * NOTE: while we are allowed to send multiple devices in one report, we
 * will just send for one for now.
 *
 * @param pdu_type
 * @param txadd
 * @param rxbuf
 * @param hdr
 * @param scansm
 */
static void
ble_ll_scan_send_adv_report(uint8_t pdu_type,
                            const uint8_t *adva, uint8_t adva_type,
                            const uint8_t *inita, uint8_t inita_type,
                            struct os_mbuf *om,
                            struct ble_mbuf_hdr *hdr,
                            struct ble_ll_scan_sm *scansm)
{
    uint8_t subev = BLE_HCI_LE_SUBEV_ADV_RPT;
    uint8_t adv_data_len;
    uint8_t evtype;
    int rc;

    if (pdu_type == BLE_ADV_PDU_TYPE_ADV_DIRECT_IND) {
        if (ble_ll_is_rpa(inita, inita_type)) {
            /* For resolvable we send separate subevent */
            subev = BLE_HCI_LE_SUBEV_DIRECT_ADV_RPT;
        }

        evtype = BLE_HCI_ADV_RPT_EVTYPE_DIR_IND;
        adv_data_len = 0;
    } else {
        if (pdu_type == BLE_ADV_PDU_TYPE_ADV_IND) {
            evtype = BLE_HCI_ADV_RPT_EVTYPE_ADV_IND;
        } else if (pdu_type == BLE_ADV_PDU_TYPE_ADV_SCAN_IND) {
            evtype = BLE_HCI_ADV_RPT_EVTYPE_SCAN_IND;
        } else if (pdu_type == BLE_ADV_PDU_TYPE_ADV_NONCONN_IND) {
            evtype = BLE_HCI_ADV_RPT_EVTYPE_NONCONN_IND;
        } else {
            evtype = BLE_HCI_ADV_RPT_EVTYPE_SCAN_RSP;
        }
        adv_data_len = om->om_data[1] - BLE_DEV_ADDR_LEN;
        os_mbuf_adj(om, BLE_LL_PDU_HDR_LEN + BLE_DEV_ADDR_LEN);
    }

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
    /* If RPA has been used, make sure we use correct address types
     * in the advertising report.
     */
    if (BLE_MBUF_HDR_RESOLVED(hdr)) {
        adva_type += 2;
    }
    if (BLE_MBUF_HDR_TARGETA_RESOLVED(hdr)) {
        inita_type += 2;
    } else {
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
        if (scansm->ext_scanning) {
            if (ble_ll_is_rpa(inita, inita_type)) {
                inita_type = 0xfe;
            }
        }
#endif
    }
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    if (scansm->ext_scanning) {
        rc = ble_ll_hci_send_legacy_ext_adv_report(evtype,
                                                   adva, adva_type,
                                                   hdr->rxinfo.rssi - ble_ll_rx_gain(),
                                                   adv_data_len, om,
                                                   inita, inita_type);
goto done;
}
#endif

    if (subev == BLE_HCI_LE_SUBEV_DIRECT_ADV_RPT) {
        rc = ble_ll_hci_send_dir_adv_report(adva, adva_type, inita, inita_type,
                                            hdr->rxinfo.rssi - ble_ll_rx_gain());
        goto done;
    }

    rc = ble_ll_hci_send_adv_report(evtype, adva, adva_type,
                                    hdr->rxinfo.rssi - ble_ll_rx_gain(),
                                    adv_data_len, om);
done:
    if (!rc && scansm->scan_filt_dups) {
        ble_ll_scan_dup_update_legacy(adva_type, adva, subev, evtype);
    }
}

/**
 * Called to enable the receiver for scanning.
 *
 * Context: Link Layer task
 *
 * @param sch
 *
 * @return int
 */
static int
ble_ll_scan_start(struct ble_ll_scan_sm *scansm)
{
    int rc;
    struct ble_ll_scan_phy *scanp = scansm->scanp;
    uint8_t chan;
#if (BLE_LL_BT5_PHY_SUPPORTED == 1)
    uint8_t phy_mode;
    int phy;
#endif

    BLE_LL_ASSERT(scansm->scan_rsp_pending == 0);

    /* Set channel */
    chan = scanp->scan_chan;
    rc = ble_phy_setchan(chan, BLE_ACCESS_ADDR_ADV, BLE_LL_CRCINIT_ADV);
    BLE_LL_ASSERT(rc == 0);

    /*
     * Set transmit end callback to NULL in case we transmit a scan request.
     * There is a callback for the connect request.
     */
    ble_phy_set_txend_cb(NULL, NULL);

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_ENCRYPTION)
    ble_phy_encrypt_disable();
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
    if (ble_ll_resolv_enabled()) {
        ble_phy_resolv_list_enable();
    } else {
        ble_phy_resolv_list_disable();
    }
#endif

#if (BLE_LL_BT5_PHY_SUPPORTED == 1)
    phy = scanp->phy;
    phy_mode = ble_ll_phy_to_phy_mode(phy, BLE_HCI_LE_PHY_CODED_ANY);
    ble_phy_mode_set(phy_mode, phy_mode);
#endif

    /* if scan is not passive we need to set tx power as we may end up sending
     * package
     */
    if (scansm->scanp->scan_type != BLE_SCAN_TYPE_PASSIVE) {
        ble_ll_tx_power_set(g_ble_ll_tx_power);
    }

    rc = ble_phy_rx_set_start_time(ble_ll_tmr_get() +
                                   g_ble_ll_sched_offset_ticks, 0);
    if (!rc || rc == BLE_PHY_ERR_RX_LATE) {
        /* If we are late here, it is still OK because we keep scanning.
         * Clear error
         */
        rc = 0;

        /* Enable/disable whitelisting */
        if (scansm->scan_filt_policy & 1) {
            ble_ll_whitelist_enable();
        } else {
            ble_ll_whitelist_disable();
        }

        ble_ll_state_set(BLE_LL_STATE_SCANNING);
    }

    return rc;
}

static uint8_t
ble_ll_scan_get_next_adv_prim_chan(uint8_t chan)
{
    ++chan;
    if (chan == BLE_PHY_NUM_CHANS) {
        chan = BLE_PHY_ADV_CHAN_START;
    }

    return chan;
}

static uint32_t
ble_ll_scan_move_window_to(struct ble_ll_scan_phy *scanp, uint32_t time)
{
    uint32_t end_time;

    /*
     * Move window until given tick is before or inside window and move to next
     * channel for each skipped interval.
     */

    end_time = scanp->timing.start_time + scanp->timing.window;
    while (LL_TMR_GEQ(time, end_time)) {
        scanp->timing.start_time += scanp->timing.interval;
        scanp->scan_chan = ble_ll_scan_get_next_adv_prim_chan(scanp->scan_chan);
        end_time = scanp->timing.start_time + scanp->timing.window;
    }

    return scanp->timing.start_time;
}

static bool
ble_ll_scan_is_inside_window(struct ble_ll_scan_phy *scanp, uint32_t time)
{
    uint32_t start_time;

    /* Make sure we are checking against closest window */
    start_time = ble_ll_scan_move_window_to(scanp, time);

    if (scanp->timing.window == scanp->timing.interval) {
        /* always inside window in continuous scan */
        return true;
    }

    return LL_TMR_GEQ(time, start_time) &&
           LL_TMR_LT(time, start_time + scanp->timing.window);
}

/**
 * Stop the scanning state machine
 */
void
ble_ll_scan_sm_stop(int chk_disable)
{
    os_sr_t sr;
    uint8_t lls;
    struct ble_ll_scan_sm *scansm;

    /* Stop the scanning timer  */
    scansm = &g_ble_ll_scan_sm;
    ble_ll_tmr_stop(&scansm->scan_timer);

    /* Only set state if we are currently in a scan window */
    if (chk_disable) {
        OS_ENTER_CRITICAL(sr);
        lls = ble_ll_state_get();

        if (lls == BLE_LL_STATE_SCANNING) {
            /* Disable phy */
            ble_phy_disable();

            /* Set LL state to standby */
            ble_ll_state_set(BLE_LL_STATE_STANDBY);
        }
        OS_EXIT_CRITICAL(sr);
    }

    OS_ENTER_CRITICAL(sr);

    /* Disable scanning state machine */
    scansm->scan_enabled = 0;
    scansm->restart_timer_needed = 0;

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    if (scansm->ext_scanning) {
        ble_ll_sched_rmv_elem_type(BLE_LL_SCHED_TYPE_SCAN_AUX, ble_ll_scan_aux_sched_remove);
        scansm->ext_scanning = 0;
    }
#endif

#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    scansm->connsm = NULL;
#endif

    /* Update backoff if we failed to receive scan response */
    if (scansm->scan_rsp_pending) {
        scansm->scan_rsp_pending = 0;
        ble_ll_scan_req_backoff(scansm, 0);
    }
    OS_EXIT_CRITICAL(sr);

    /* Count # of times stopped */
    STATS_INC(ble_ll_stats, scan_stops);

    /* No need for RF anymore */
    OS_ENTER_CRITICAL(sr);
    ble_ll_rfmgmt_scan_changed(false, 0);
    ble_ll_rfmgmt_release();
    OS_EXIT_CRITICAL(sr);
}

static int
ble_ll_scan_sm_start(struct ble_ll_scan_sm *scansm)
{
    struct ble_ll_scan_phy *scanp;
    struct ble_ll_scan_phy *scanp_next;

    if (!ble_ll_is_valid_own_addr_type(scansm->own_addr_type, g_random_addr)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    BLE_LL_ASSERT(scansm->scanp);
    scanp = scansm->scanp;
    scanp_next = scansm->scanp_next;

    /* Count # of times started */
    STATS_INC(ble_ll_stats, scan_starts);

    /* Set flag telling us that scanning is enabled */
    scansm->scan_enabled = 1;

    /* Set first advertising channel */
    scanp->scan_chan = BLE_PHY_ADV_CHAN_START;
    if (scanp_next) {
        scanp_next->scan_chan = BLE_PHY_ADV_CHAN_START;
    }

    /* Reset scan request backoff parameters to default */
    scansm->upper_limit = 1;
    scansm->backoff_count = 1;
    scansm->scan_rsp_pending = 0;

    /* Forget filtered advertisers from previous scan. */
    g_ble_ll_scan_num_rsp_advs = 0;

    os_mempool_clear(&g_scan_dup_pool);
    TAILQ_INIT(&g_scan_dup_list);

    /*
     * First scan window can start when RF is enabled. Add 1 tick since we are
     * most likely not aligned with ticks so RF may be effectively enabled 1
     * tick later.
     */
    scanp->timing.start_time = ble_ll_rfmgmt_enable_now();
    ble_ll_rfmgmt_scan_changed(true, scanp->timing.start_time);

    if (scanp_next) {
        /* Schedule start time right after first phy */
        scanp_next->timing.start_time = scanp->timing.start_time +
                                        scanp->timing.window;
    }

    /* Start scan at 1st window */
    ble_ll_tmr_start(&scansm->scan_timer, scanp->timing.start_time);

    return BLE_ERR_SUCCESS;
}

static void
ble_ll_scan_interrupted_event_cb(struct ble_npl_event *ev)
{
    struct ble_ll_scan_sm *scansm = &g_ble_ll_scan_sm;

    if (!scansm->scan_enabled) {
        return;
    }

    /*
    * If we timed out waiting for a response, the scan response pending
    * flag should be set. Deal with scan backoff. Put device back into rx.
    */

    if (scansm->scan_rsp_pending) {
        scansm->scan_rsp_pending = 0;
        ble_ll_scan_req_backoff(scansm, 0);
    }

    ble_ll_scan_chk_resume();
}

/**
 * Called to process the scanning OS event which was posted to the LL task
 *
 * Context: Link Layer task.
 *
 * @param arg
 */
static void
ble_ll_scan_event_proc(struct ble_npl_event *ev)
{
    struct ble_ll_scan_sm *scansm;
    os_sr_t sr;
    bool start_scan;
    bool inside_window;
    struct ble_ll_scan_phy *scanp;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    bool inside_window_next;
    struct ble_ll_scan_phy *scanp_next;
#endif
    uint32_t next_proc_time;
    uint32_t now;
    /*
     * Get the scanning state machine. If not enabled (this is possible), just
     * leave and do nothing (just make sure timer is stopped).
     */
    scansm = (struct ble_ll_scan_sm *)ble_npl_event_get_arg(ev);
    scanp = scansm->scanp;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    scanp_next = scansm->scanp_next;
#endif

    OS_ENTER_CRITICAL(sr);
    if (!scansm->scan_enabled) {
        ble_ll_tmr_stop(&scansm->scan_timer);
        ble_ll_rfmgmt_scan_changed(false, 0);
        ble_ll_rfmgmt_release();
        OS_EXIT_CRITICAL(sr);
        return;
    }

    if (scansm->scan_rsp_pending) {
        /* Aux scan in progress. Wait */
        STATS_INC(ble_ll_stats, scan_timer_stopped);
        scansm->restart_timer_needed = 1;
        OS_EXIT_CRITICAL(sr);
        return;
    }

    now = ble_ll_tmr_get();

    inside_window = ble_ll_scan_is_inside_window(scanp, now);

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    /* Update also next PHY if configured */
    if (scanp_next) {
        inside_window_next = ble_ll_scan_is_inside_window(scanp_next, now);

        /*
         * Switch PHY if current PHY is outside window and next PHY is either
         * inside window or has next window earlier than current PHY.
         */
        if (!inside_window &&
            ((inside_window_next || LL_TMR_LEQ(scanp_next->timing.start_time,
                                               scanp->timing.start_time)))) {
            scansm->scanp = scanp_next;
            scansm->scanp_next = scanp;
            scanp = scansm->scanp;
            scanp_next = scansm->scanp_next;
            inside_window = inside_window_next;
        }
    }
#endif

    /*
     * At this point scanp and scanp_next point to current or closest scan
     * window on both PHYs (scanp is the closer one). Make sure RF is enabled
     * on time.
     */
    ble_ll_rfmgmt_scan_changed(true, scanp->timing.start_time);

    /*
     * If we are inside window, next scan proc should happen at the end of
     * current window to either disable scan or switch to next PHY.
     * If we are outside window, next scan proc should happen at the time of
     * closest scan window.
     */
    if (inside_window) {
        next_proc_time = scanp->timing.start_time + scanp->timing.window;
    } else {
        next_proc_time = scanp->timing.start_time;
    }

    /*
     * If we are not in the standby state it means that the scheduled
     * scanning event was overlapped in the schedule. In this case all we do
     * is post the scan schedule end event.
     */
    start_scan = inside_window;
    switch (ble_ll_state_get()) {
#if MYNEWT_VAL(BLE_LL_ROLE_BROADCASTER)
    case BLE_LL_STATE_ADV:
        start_scan = false;
        break;
#endif
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL) || MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    case BLE_LL_STATE_CONNECTION:
        start_scan = false;
        break;
#endif
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PERIODIC_ADV)
    case BLE_LL_STATE_SYNC:
        start_scan = false;
        break;
#endif
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    case BLE_LL_STATE_SCAN_AUX:
        start_scan = false;
        break;
#endif
#if MYNEWT_VAL(BLE_LL_EXT)
    case BLE_LL_STATE_EXTERNAL:
        start_scan = false;
        break;
#endif
    case BLE_LL_STATE_SCANNING:
        /* Must disable PHY since we will move to a new channel */
        ble_phy_disable();
        if (!inside_window) {
            ble_ll_state_set(BLE_LL_STATE_STANDBY);
        }
        break;
#if MYNEWT_VAL(BLE_LL_ISO_BROADCASTER)
    case BLE_LL_STATE_BIG:
        start_scan = false;
        break;
#endif
    case BLE_LL_STATE_STANDBY:
        break;
    default:
        BLE_LL_ASSERT(0);
        break;
    }

    if (start_scan) {
        ble_ll_scan_start(scansm);
    } else {
        ble_ll_rfmgmt_release();
    }

    OS_EXIT_CRITICAL(sr);
    ble_ll_tmr_start(&scansm->scan_timer, next_proc_time);
}

/**
 * ble ll scan rx pdu start
 *
 * Called when a PDU reception has started and the Link Layer is in the
 * scanning state.
 *
 * Context: Interrupt
 *
 * @param pdu_type
 * @param rxflags
 *
 * @return int
 *  0: we will not attempt to reply to this frame
 *  1: we may send a response to this frame.
 */
int
ble_ll_scan_rx_isr_start(uint8_t pdu_type, uint16_t *rxflags)
{
    int rc;
    struct ble_ll_scan_sm *scansm;
    struct ble_ll_scan_phy *scanp;

    rc = 0;
    scansm = &g_ble_ll_scan_sm;
    scanp = scansm->scanp;

    switch (scanp->scan_type) {
    case BLE_SCAN_TYPE_ACTIVE:
        /* If adv ind or scan ind, we may send scan request */
        if ((pdu_type == BLE_ADV_PDU_TYPE_ADV_IND) ||
            (pdu_type == BLE_ADV_PDU_TYPE_ADV_SCAN_IND)) {
            rc = 1;
        }

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
        if ((pdu_type == BLE_ADV_PDU_TYPE_ADV_EXT_IND && scansm->ext_scanning)) {
            *rxflags |= BLE_MBUF_HDR_F_EXT_ADV;
            rc = 1;
        }
#endif

        /*
         * If this is the first PDU after we sent the scan response (as
         * denoted by the scan rsp pending flag), we set a bit in the ble
         * header so the link layer can check to see if the scan request
         * was successful. We do it this way to let the Link Layer do the
         * work for successful scan requests. If failed, we do the work here.
         */
        if (scansm->scan_rsp_pending) {
            scansm->scan_rsp_pending = 0;

            if (pdu_type == BLE_ADV_PDU_TYPE_SCAN_RSP) {
                *rxflags |= BLE_MBUF_HDR_F_SCAN_RSP_RXD;
            } else if (pdu_type == BLE_ADV_PDU_TYPE_AUX_SCAN_RSP) {
                *rxflags |= BLE_MBUF_HDR_F_SCAN_RSP_RXD;
            } else {
                ble_ll_scan_req_backoff(scansm, 0);
            }
        }
        break;
    case BLE_SCAN_TYPE_PASSIVE:
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
        if ((pdu_type == BLE_ADV_PDU_TYPE_ADV_EXT_IND && scansm->ext_scanning)) {
            *rxflags |= BLE_MBUF_HDR_F_EXT_ADV;
        }
        break;
#endif
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    case BLE_SCAN_TYPE_INITIATE:
        if ((pdu_type == BLE_ADV_PDU_TYPE_ADV_IND) ||
            (pdu_type == BLE_ADV_PDU_TYPE_ADV_DIRECT_IND)) {
            rc = 1;
        }

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
        if ((pdu_type == BLE_ADV_PDU_TYPE_ADV_EXT_IND && scansm->ext_scanning)) {
            *rxflags |= BLE_MBUF_HDR_F_EXT_ADV;
            rc = 1;
        }
#endif
        break;
#endif
    default:
        break;
    }

    return rc;
}

static void
ble_ll_scan_get_addr_data_from_legacy(uint8_t pdu_type, uint8_t *rxbuf,
                                     struct ble_ll_scan_addr_data *addrd)
{
    BLE_LL_ASSERT(pdu_type < BLE_ADV_PDU_TYPE_ADV_EXT_IND);

    addrd->adva = rxbuf + BLE_LL_PDU_HDR_LEN;
    addrd->adva_type = ble_ll_get_addr_type(rxbuf[0] & BLE_ADV_PDU_HDR_TXADD_MASK);

    if (pdu_type == BLE_ADV_PDU_TYPE_ADV_DIRECT_IND) {
        addrd->targeta = rxbuf + BLE_LL_PDU_HDR_LEN + BLE_DEV_ADDR_LEN;
        addrd->targeta_type = ble_ll_get_addr_type(rxbuf[0] & BLE_ADV_PDU_HDR_RXADD_MASK);
    } else {
        addrd->targeta = NULL;
        addrd->targeta_type = 0;
    }
}

int
ble_ll_scan_rx_filter(uint8_t own_addr_type, uint8_t scan_filt_policy,
                      struct ble_ll_scan_addr_data *addrd, uint8_t *scan_ok)
{
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
    struct ble_ll_resolv_entry *rl = NULL;
#endif
    bool scan_req_allowed = true;
    bool resolved;

    /* Note: caller is expected to fill adva, targeta and rpa_index in addrd */

    /* Use AdvA as initial advertiser address, we may change it if resolved */
    addrd->adv_addr = addrd->adva;
    addrd->adv_addr_type = addrd->adva_type;

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
    addrd->adva_resolved = 0;
    addrd->targeta_resolved = 0;

    BLE_LL_ASSERT((addrd->rpa_index < 0) ||
                  (ble_ll_addr_subtype(addrd->adva, addrd->adva_type) ==
                   BLE_LL_ADDR_SUBTYPE_RPA));

    switch (ble_ll_addr_subtype(addrd->adva, addrd->adva_type)) {
    case BLE_LL_ADDR_SUBTYPE_RPA:
        if (addrd->rpa_index >= 0) {
            addrd->adva_resolved = 1;

            /* Use resolved identity address as advertiser address */
            rl = &g_ble_ll_resolv_list[addrd->rpa_index];
            addrd->adv_addr = rl->rl_identity_addr;
            addrd->adv_addr_type = rl->rl_addr_type;
            break;
        }

        /* fall-through */
    case BLE_LL_ADDR_SUBTYPE_IDENTITY:
        /* If AdvA is an identity address, we need to check if that device was
         * added to RL in order to use proper privacy mode.
         */
        rl = ble_ll_resolv_list_find(addrd->adva, addrd->adva_type);
        if (!rl) {
            break;
        }

        /* Ignore device if using network privacy mode and it has IRK */
        if ((rl->rl_priv_mode == BLE_HCI_PRIVACY_NETWORK) && rl->rl_has_peer) {
            return -1;
        }

        addrd->rpa_index = ble_ll_resolv_get_idx(rl);
        break;
    default:
        /* NRPA goes through filtering policy directly */
        break;
    }

    if (addrd->targeta) {
        switch (ble_ll_addr_subtype(addrd->targeta, addrd->targeta_type)) {
        case BLE_LL_ADDR_SUBTYPE_RPA:
            /* Check if TargetA can be resolved using the same RL entry as AdvA */
            if (rl && ble_ll_resolv_rpa(addrd->targeta, rl->rl_local_irk)) {
                addrd->targeta_resolved = 1;
                break;
            }

            /* Check if scan filter policy allows unresolved RPAs to be processed */
            if (!(scan_filt_policy & 0x02)) {
                return -2;
            }

            /* Do not send scan request even if scan policy allows unresolved
             * RPAs - we do not know if this one if directed to us.
             */
            scan_req_allowed = false;
            break;
        case BLE_LL_ADDR_SUBTYPE_IDENTITY:
            /* We shall ignore identity in TargetA if we are using RPA */
            if ((own_addr_type & 0x02) && rl && rl->rl_has_local) {
                return -1;
            }

            /* Ignore if not directed to us */
            if ((addrd->targeta_type != (own_addr_type & 0x01)) ||
                !ble_ll_is_our_devaddr(addrd->targeta, addrd->targeta_type)) {
                return -1;
            }
            break;
        default:
            /* NRPA goes through filtering policy directly */
            break;
        }
    }

    resolved = addrd->adva_resolved;
#else
    /* Ignore if not directed to us */
    if (addrd->targeta &&
        ((addrd->targeta_type != (own_addr_type & 0x01)) ||
         !ble_ll_is_our_devaddr(addrd->targeta, addrd->targeta_type))) {
        return -1;
    }

    resolved = false;
#endif

    if (scan_filt_policy & 0x01) {
        /* Check on WL if required by scan filter policy */
        if (!ble_ll_whitelist_match(addrd->adv_addr, addrd->adv_addr_type,
                                    resolved)) {
            return -2;
        }
    }

    if (scan_ok) {
        *scan_ok = scan_req_allowed;
    }

    return 0;
}

#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
int
ble_ll_scan_rx_check_init(struct ble_ll_scan_addr_data *addrd)
{
    struct ble_ll_scan_sm *scansm;
    struct ble_ll_conn_sm *connsm;

    scansm = &g_ble_ll_scan_sm;
    connsm = scansm->connsm;
    BLE_LL_ASSERT(connsm);

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
    if ((connsm->peer_addr_type > BLE_ADDR_RANDOM) && !addrd->adva_resolved) {
        return -1;
    }
#endif
    if ((addrd->adv_addr_type != (connsm->peer_addr_type & 0x01)) ||
        memcmp(addrd->adv_addr, connsm->peer_addr, 6) != 0) {
        return -1;
    }

    return 0;
}
#endif

static int
ble_ll_scan_rx_isr_end_on_adv(uint8_t pdu_type, uint8_t *rxbuf,
                              struct ble_mbuf_hdr *hdr,
                              struct ble_ll_scan_addr_data *addrd)
{
    struct ble_ll_scan_sm *scansm = &g_ble_ll_scan_sm;
    struct ble_ll_scan_phy *scanp = scansm->scanp;
    struct ble_mbuf_hdr_rxinfo *rxinfo = &hdr->rxinfo;
    uint8_t scan_ok;
    int rc;

    ble_ll_scan_get_addr_data_from_legacy(pdu_type, rxbuf, addrd);

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
    addrd->rpa_index = ble_hw_resolv_list_match();
#endif

    rc = ble_ll_scan_rx_filter(scansm->own_addr_type,
                               scansm->scan_filt_policy, addrd, &scan_ok);
    if (rc < 0) {
        return 0;
    }

#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    if ((scanp->scan_type == BLE_SCAN_TYPE_INITIATE) &&
        !(scansm->scan_filt_policy & 0x01)) {
        rc = ble_ll_scan_rx_check_init(addrd);
        if (rc < 0) {
            return 0;
        }
    }
#endif

    rxinfo->flags |= BLE_MBUF_HDR_F_DEVMATCH;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
    rxinfo->rpa_index = addrd->rpa_index;
    if (addrd->adva_resolved) {
        rxinfo->flags |= BLE_MBUF_HDR_F_RESOLVED;
    }
    if (addrd->targeta_resolved) {
        rxinfo->flags |= BLE_MBUF_HDR_F_TARGETA_RESOLVED;
    }
#endif

    if (!scan_ok) {
        /* Scan request forbidden by filter policy */
        return 0;
    }

    /* Allow responding to all PDUs when initiating since unwanted PDUs were
     * already filtered out in isr_start.
     */
    if ((scanp->scan_type == BLE_SCAN_TYPE_ACTIVE) &&
            ((pdu_type == BLE_ADV_PDU_TYPE_ADV_IND) ||
             (pdu_type == BLE_ADV_PDU_TYPE_ADV_SCAN_IND))) {
        return 1;
    }

#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    if (scanp->scan_type == BLE_SCAN_TYPE_INITIATE) {
        return 1;
    }
#endif

    return 0;
}

static int
ble_ll_scan_rx_isr_end_on_scan_rsp(uint8_t pdu_type, uint8_t *rxbuf,
                                   struct ble_mbuf_hdr *hdr,
                                   struct ble_ll_scan_addr_data *addrd)
{
    struct ble_ll_scan_sm *scansm = &g_ble_ll_scan_sm;
    struct ble_mbuf_hdr_rxinfo *rxinfo = &hdr->rxinfo;
    uint8_t sreq_adva_type;
    uint8_t *sreq_adva;

    ble_ll_scan_get_addr_data_from_legacy(pdu_type, rxbuf, addrd);

    if (!BLE_MBUF_HDR_SCAN_RSP_RXD(hdr)) {
        /*
         * We were not expecting scan response so just ignore and do not
         * update backoff.
         */
        return -1;
    }

    sreq_adva_type = !!(scansm->pdu_data.hdr_byte & BLE_ADV_PDU_HDR_RXADD_MASK);
    sreq_adva = scansm->pdu_data.adva;

    /*
     * Ignore scan response if AdvA does not match AdvA in request and also
     * update backoff as if there was no scan response.
     */
    if ((addrd->adva_type != sreq_adva_type) ||
        memcmp(addrd->adva, sreq_adva, BLE_DEV_ADDR_LEN)) {
        ble_ll_scan_req_backoff(scansm, 0);
        return -1;
    }

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
    /*
     * We are not pushing this one through filters so need to update
     * rpa_index here as otherwise pkt_in won't be able to determine
     * advertiser address properly.
     */
    rxinfo->rpa_index = ble_hw_resolv_list_match();
    if (rxinfo->rpa_index >= 0) {
        rxinfo->flags |= BLE_MBUF_HDR_F_RESOLVED;
    }
#endif

    rxinfo->flags |= BLE_MBUF_HDR_F_DEVMATCH;

    return 0;
}

static bool
ble_ll_scan_send_scan_req(uint8_t pdu_type, uint8_t *rxbuf,
                          struct ble_mbuf_hdr *hdr,
                          struct ble_ll_scan_addr_data *addrd)
{
    struct ble_ll_scan_sm *scansm = &g_ble_ll_scan_sm;
    struct ble_mbuf_hdr_rxinfo *rxinfo = &hdr->rxinfo;
    bool is_ext_adv = false;
    int8_t rpa_index;
    uint16_t adi = 0;
    int rc;

    /* Check if we already scanned this device successfully */
    if (ble_ll_scan_have_rxd_scan_rsp(addrd->adv_addr, addrd->adv_addr_type,
                                      is_ext_adv, adi)) {
        return false;
    }

    /* Better not be a scan response pending */
    BLE_LL_ASSERT(scansm->scan_rsp_pending == 0);

    /* We want to send a request. See if backoff allows us */
    if (ble_ll_scan_backoff_kick() != 0) {
        return false;
    }

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
    rpa_index = addrd->rpa_index;
#else
    rpa_index = -1;
#endif

    /* Use original AdvA in scan request (Core 5.1, Vol 6, Part B, section 6.3) */
    ble_ll_scan_req_pdu_prepare(scansm, addrd->adva, addrd->adva_type,
                                rpa_index);

    rc = ble_phy_tx(ble_ll_scan_req_tx_pdu_cb, scansm, BLE_PHY_TRANSITION_TX_RX);
    if (rc) {
        return false;
    }

    scansm->scan_rsp_pending = 1;
    rxinfo->flags |= BLE_MBUF_HDR_F_SCAN_REQ_TXD;

    return true;
}

/**
 * Called when a receive PDU has ended.
 *
 * Context: Interrupt
 *
 * @param rxpdu
 *
 * @return int
 *       < 0: Disable the phy after reception.
 *      == 0: Success. Do not disable the PHY.
 *       > 0: Do not disable PHY as that has already been done.
 */
int
ble_ll_scan_rx_isr_end(struct os_mbuf *rxpdu, uint8_t crcok)
{
    struct ble_ll_scan_sm *scansm = &g_ble_ll_scan_sm;
    struct ble_mbuf_hdr *hdr = BLE_MBUF_HDR_PTR(rxpdu);
    struct ble_mbuf_hdr_rxinfo *rxinfo = &hdr->rxinfo;
    uint8_t *rxbuf;
    uint8_t pdu_type;
    struct ble_ll_scan_addr_data addrd;
    int rc;

    /*
     * If buffer for incoming PDU was not allocated we need to force scan to be
     * restarted since LL will not be notified. Keep PHY enabled.
     */
    if (rxpdu == NULL) {
        ble_ll_scan_interrupted(scansm);
        return 0;
    }

    if (!crcok) {
        goto scan_rx_isr_ignore;
    }

    rxbuf = rxpdu->om_data;
    pdu_type = rxbuf[0] & BLE_ADV_PDU_HDR_TYPE_MASK;

    switch (pdu_type) {
    case BLE_ADV_PDU_TYPE_ADV_IND:
    case BLE_ADV_PDU_TYPE_ADV_DIRECT_IND:
    case BLE_ADV_PDU_TYPE_ADV_NONCONN_IND:
    case BLE_ADV_PDU_TYPE_ADV_SCAN_IND:
        rc = ble_ll_scan_rx_isr_end_on_adv(pdu_type, rxbuf, hdr, &addrd);
        break;
    case BLE_ADV_PDU_TYPE_SCAN_RSP:
        rc = ble_ll_scan_rx_isr_end_on_scan_rsp(pdu_type, rxbuf, hdr, &addrd);
        break;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    case BLE_ADV_PDU_TYPE_ADV_EXT_IND:
        rc = ble_ll_scan_aux_rx_isr_end_on_ext(&g_ble_ll_scan_sm, rxpdu);
        if (rc < 0) {
            rxinfo->flags |= BLE_MBUF_HDR_F_IGNORED;
        }
        ble_ll_state_set(BLE_LL_STATE_STANDBY);
        /* Return here, we do not want any further processing since it's all
         * handled in scan_aux.
         */
        return -1;
#endif
    default:
        /* This is not something we would like to process here */
        rc = -1;
        break;
    }

    if (rc == -1) {
        goto scan_rx_isr_ignore;
    }

    if (rc == 1) {
        switch (scansm->scanp->scan_type) {
        case BLE_SCAN_TYPE_ACTIVE:
            if (ble_ll_scan_send_scan_req(pdu_type, rxbuf, hdr, &addrd)) {
                /* Keep PHY active and LL in scanning state */
                return 0;
            }
            break;
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
        case BLE_SCAN_TYPE_INITIATE:
            if (ble_ll_conn_send_connect_req(rxpdu, &addrd, 0) == 0) {
                hdr->rxinfo.flags |= BLE_MBUF_HDR_F_CONNECT_IND_TXD;
                return 0;
            }
            break;
#endif
        }
    }

    /* We are done with this PDU so go to standby and let LL resume if needed */
    ble_ll_state_set(BLE_LL_STATE_STANDBY);
    return -1;

scan_rx_isr_ignore:
    rxinfo->flags |= BLE_MBUF_HDR_F_IGNORED;
    ble_ll_state_set(BLE_LL_STATE_STANDBY);
    return -1;
}

/**
 * Called to resume scanning. This is called after an advertising event or
 * connection event has ended. It is also called if we receive a packet while
 * in the initiating or scanning state.
 *
 * If periodic advertising is enabled this is also called on sync event end
 * or sync packet received if chaining
 *
 * Context: Link Layer task
 */
void
ble_ll_scan_chk_resume(void)
{
    os_sr_t sr;
    struct ble_ll_scan_sm *scansm;
    uint32_t now;

    scansm = &g_ble_ll_scan_sm;
    if (scansm->scan_enabled) {
        OS_ENTER_CRITICAL(sr);
        if (scansm->restart_timer_needed) {
            scansm->restart_timer_needed = 0;
            ble_ll_event_add(&scansm->scan_sched_ev);
            STATS_INC(ble_ll_stats, scan_timer_restarted);
            OS_EXIT_CRITICAL(sr);
            return;
        }

        now = ble_ll_tmr_get();
        if (ble_ll_state_get() == BLE_LL_STATE_STANDBY &&
            ble_ll_scan_is_inside_window(scansm->scanp, now)) {
            /* Turn on the receiver and set state */
            ble_ll_scan_start(scansm);
        }
        OS_EXIT_CRITICAL(sr);
    }
}

/**
 * Scan timer callback; means that the scan window timeout has been reached
 * and we should perform the appropriate actions.
 *
 * Context: Interrupt (cputimer)
 *
 * @param arg Pointer to scan state machine.
 */
void
ble_ll_scan_timer_cb(void *arg)
{
    struct ble_ll_scan_sm *scansm;

    scansm = (struct ble_ll_scan_sm *)arg;
    ble_ll_event_add(&scansm->scan_sched_ev);
}

void
ble_ll_scan_interrupted(struct ble_ll_scan_sm *scansm)
{
    ble_ll_event_add(&scansm->scan_interrupted_ev);
}

/**
 * Called when the wait for response timer expires while in the scanning
 * state.
 *
 * Context: Interrupt.
 */
void
ble_ll_scan_wfr_timer_exp(void)
{
    struct ble_ll_scan_sm *scansm = &g_ble_ll_scan_sm;

    /* Update backoff if we failed to receive scan response */
    if (scansm->scan_rsp_pending) {
        scansm->scan_rsp_pending = 0;
        ble_ll_scan_req_backoff(scansm, 0);
    }

    ble_phy_restart_rx();
}

static inline void
ble_ll_scan_dup_move_to_head(struct ble_ll_scan_dup_entry *e)
{
    if (e != TAILQ_FIRST(&g_scan_dup_list)) {
        TAILQ_REMOVE(&g_scan_dup_list, e, link);
        TAILQ_INSERT_HEAD(&g_scan_dup_list, e, link);
    }
}

static inline struct ble_ll_scan_dup_entry *
ble_ll_scan_dup_new(void)
{
    struct ble_ll_scan_dup_entry *e;

    e = os_memblock_get(&g_scan_dup_pool);
    if (!e) {
        e = TAILQ_LAST(&g_scan_dup_list, ble_ll_scan_dup_list);
        TAILQ_REMOVE(&g_scan_dup_list, e, link);
    }

    memset(e, 0, sizeof(*e));

    return e;
}

static int
ble_ll_scan_dup_check_legacy(uint8_t addr_type, uint8_t *addr, uint8_t pdu_type)
{
    struct ble_ll_scan_dup_entry *e;
    uint8_t type;
    int rc;

    type = BLE_LL_SCAN_ENTRY_TYPE_LEGACY(addr_type);

    TAILQ_FOREACH(e, &g_scan_dup_list, link) {
        if ((e->type == type) && !memcmp(e->addr, addr, 6)) {
            break;
        }
    }

    if (e) {
        if (pdu_type == BLE_ADV_PDU_TYPE_ADV_DIRECT_IND) {
            rc = e->flags & BLE_LL_SCAN_DUP_F_DIR_ADV_REPORT_SENT;
        } else if (pdu_type == BLE_ADV_PDU_TYPE_SCAN_RSP) {
            rc = e->flags & BLE_LL_SCAN_DUP_F_SCAN_RSP_SENT;
        } else {
            rc = e->flags & BLE_LL_SCAN_DUP_F_ADV_REPORT_SENT;
        }

        ble_ll_scan_dup_move_to_head(e);
    } else {
        rc = 0;

        e = ble_ll_scan_dup_new();
        e->flags = 0;
        e->type = type;
        memcpy(e->addr, addr, 6);

        TAILQ_INSERT_HEAD(&g_scan_dup_list, e, link);
    }

    return rc;
}

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
int
ble_ll_scan_dup_check_ext(uint8_t addr_type, uint8_t *addr, bool has_aux,
                          uint16_t adi)
{
    struct ble_ll_scan_dup_entry *e;
    bool is_anon;
    uint8_t type;
    int rc;

    is_anon = addr == NULL;
    adi = has_aux ? adi : 0;

    type = BLE_LL_SCAN_ENTRY_TYPE_EXT(addr_type, has_aux, is_anon, adi);

    TAILQ_FOREACH(e, &g_scan_dup_list, link) {
        if ((e->type == type) &&
            (is_anon || !memcmp(e->addr, addr, BLE_DEV_ADDR_LEN))) {
            break;
        }
    }

    if (e) {
        if (e->adi != adi) {
            rc = 0;

            e->flags = 0;
            e->adi = adi;
        } else {
            rc = e->flags & BLE_LL_SCAN_DUP_F_ADV_REPORT_SENT;
        }

        ble_ll_scan_dup_move_to_head(e);
    } else {
        rc = 0;

        e = ble_ll_scan_dup_new();
        e->flags = 0;
        e->type = type;
        e->adi = adi;
        if (!is_anon) {
            memcpy(e->addr, addr, 6);
        }

        TAILQ_INSERT_HEAD(&g_scan_dup_list, e, link);
    }

    return rc;
}

int
ble_ll_scan_dup_update_ext(uint8_t addr_type, uint8_t *addr, bool has_aux,
                           uint16_t adi)
{
    struct ble_ll_scan_dup_entry *e;
    bool is_anon;
    uint8_t type;

    is_anon = addr == NULL;
    adi = has_aux ? adi : 0;

    type = BLE_LL_SCAN_ENTRY_TYPE_EXT(addr_type, has_aux, is_anon, adi);

    /*
     * We assume ble_ll_scan_dup_check() was called before which either matched
     * some entry or allocated new one and placed in on the top of queue.
     */

    e = TAILQ_FIRST(&g_scan_dup_list);
    BLE_LL_ASSERT(e && e->type == type && (is_anon || !memcmp(e->addr, addr, 6)));

    e->flags |= BLE_LL_SCAN_DUP_F_ADV_REPORT_SENT;

    return 0;
}
#endif

static void
ble_ll_scan_rx_pkt_in_restore_addr_data(struct ble_mbuf_hdr *hdr,
                                        struct ble_ll_scan_addr_data *addrd)
{
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
    struct ble_ll_scan_sm *scansm = &g_ble_ll_scan_sm;
    struct ble_mbuf_hdr_rxinfo *rxinfo = &hdr->rxinfo;
    struct ble_ll_resolv_entry *rl;
#endif

    addrd->adv_addr = addrd->adva;
    addrd->adv_addr_type = addrd->adva_type;

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
    addrd->rpa_index = rxinfo->rpa_index;

    if (hdr->rxinfo.flags & BLE_MBUF_HDR_F_RESOLVED) {
        BLE_LL_ASSERT(rxinfo->rpa_index >= 0);
        rl = &g_ble_ll_resolv_list[rxinfo->rpa_index];
        addrd->adv_addr = rl->rl_identity_addr;
        addrd->adv_addr_type = rl->rl_addr_type;
        addrd->adva_resolved = 1;
    } else {
        addrd->adva_resolved = 0;
    }

    if (hdr->rxinfo.flags & BLE_MBUF_HDR_F_TARGETA_RESOLVED) {
        addrd->targeta = ble_ll_get_our_devaddr(scansm->own_addr_type & 1);
        addrd->targeta_type = scansm->own_addr_type & 1;
        addrd->targeta_resolved = 1;
    } else {
        addrd->targeta_resolved = 0;
    }
#endif
}

static void
ble_ll_scan_rx_pkt_in_on_legacy(uint8_t pdu_type, struct os_mbuf *om,
                                struct ble_mbuf_hdr *hdr,
                                struct ble_ll_scan_addr_data *addrd)
{
    struct ble_ll_scan_sm *scansm = &g_ble_ll_scan_sm;
    uint8_t *rxbuf = om->om_data;
    bool send_hci_report;


    if (!BLE_MBUF_HDR_DEVMATCH(hdr) ||
        !BLE_MBUF_HDR_CRC_OK(hdr) ||
        BLE_MBUF_HDR_IGNORED(hdr) ||
        !scansm->scan_enabled) {
        return;
    }

    ble_ll_scan_get_addr_data_from_legacy(pdu_type, rxbuf, addrd);
    ble_ll_scan_rx_pkt_in_restore_addr_data(hdr, addrd);

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
    if (addrd->adva_resolved) {
        BLE_LL_ASSERT(addrd->rpa_index >= 0);
        ble_ll_resolv_set_peer_rpa(addrd->rpa_index, addrd->adva);
    }
#endif

    send_hci_report = !scansm->scan_filt_dups ||
                      !ble_ll_scan_dup_check_legacy(addrd->adv_addr_type,
                                                    addrd->adv_addr,
                                                    pdu_type);
    if (send_hci_report) {
        /* Sending advertising report will also update scan_dup list */
        ble_ll_scan_send_adv_report(pdu_type,
                                    addrd->adv_addr, addrd->adv_addr_type,
                                    addrd->targeta, addrd->targeta_type,
                                    om, hdr, scansm);
    }

    if (BLE_MBUF_HDR_SCAN_RSP_RXD(hdr)) {
        ble_ll_scan_req_backoff(scansm, 1);
    }
}

/**
 * Process a received PDU while in the scanning state.
 *
 * Context: Link Layer task.
 *
 * @param pdu_type
 * @param rxbuf
 */
void
ble_ll_scan_rx_pkt_in(uint8_t ptype, struct os_mbuf *om, struct ble_mbuf_hdr *hdr)
{
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    struct ble_mbuf_hdr_rxinfo *rxinfo;
    uint8_t *targeta;
#endif
    struct ble_ll_scan_sm *scansm;
    struct ble_ll_scan_addr_data addrd;
    uint8_t max_pdu_type;

    scansm = &g_ble_ll_scan_sm;

    /* Ignore PDUs we do not expect here */
    max_pdu_type = BLE_ADV_PDU_TYPE_ADV_SCAN_IND;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    if (scansm->ext_scanning) {
        /* Note: We do not expect AUX_CONNECT_RSP here */
        max_pdu_type = BLE_ADV_PDU_TYPE_ADV_EXT_IND;
    }
#endif
    if (ptype > max_pdu_type) {
        ble_ll_scan_chk_resume();
        return;
    }

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    if (ptype == BLE_ADV_PDU_TYPE_ADV_EXT_IND) {
        ble_ll_scan_aux_pkt_in_on_ext(om, hdr);
        ble_ll_scan_chk_resume();
        return;
    }
#endif

    switch (scansm->scanp->scan_type) {
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    case BLE_SCAN_TYPE_INITIATE:
        rxinfo = &hdr->rxinfo;
        if (rxinfo->flags & BLE_MBUF_HDR_F_CONNECT_IND_TXD) {
            /* We need to keep original TargetA in case it was resolved, so rl
             * can be updated properly.
             */
            ble_ll_scan_get_addr_data_from_legacy(ptype, om->om_data, &addrd);
            targeta = addrd.targeta;
            ble_ll_scan_rx_pkt_in_restore_addr_data(hdr, &addrd);

            ble_ll_scan_sm_stop(0);
            ble_ll_conn_created_on_legacy(om, &addrd, targeta);
            return;
        }
        break;
#endif
    default:
        ble_ll_scan_rx_pkt_in_on_legacy(ptype, om, hdr, &addrd);
        break;
    }

    ble_ll_scan_chk_resume();
}

int
ble_ll_scan_hci_set_params(const uint8_t *cmdbuf, uint8_t len)
{
    const struct ble_hci_le_set_scan_params_cp *cmd = (const void *)cmdbuf;
    uint16_t scan_itvl;
    uint16_t scan_window;
    struct ble_ll_scan_sm *scansm;
    struct ble_ll_scan_phy *scanp;

    if (len != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /* If already enabled, we return an error */
    scansm = &g_ble_ll_scan_sm;
    if (scansm->scan_enabled) {
        return BLE_ERR_CMD_DISALLOWED;
    }

    /* Get the scan interval and window */
    scan_itvl = le16toh(cmd->scan_itvl);
    scan_window = le16toh(cmd->scan_window);

    /* Check scan type */
    if ((cmd->scan_type != BLE_HCI_SCAN_TYPE_PASSIVE) &&
        (cmd->scan_type != BLE_HCI_SCAN_TYPE_ACTIVE)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /* Check interval and window */
    if ((scan_itvl < BLE_HCI_SCAN_ITVL_MIN) ||
        (scan_itvl > BLE_HCI_SCAN_ITVL_MAX) ||
        (scan_window < BLE_HCI_SCAN_WINDOW_MIN) ||
        (scan_window > BLE_HCI_SCAN_WINDOW_MAX) ||
        (scan_itvl < scan_window)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /* Check own addr type */
    if (cmd->own_addr_type > BLE_HCI_ADV_OWN_ADDR_MAX) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /* Check scanner filter policy */
    if (cmd->filter_policy > BLE_HCI_SCAN_FILT_MAX) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /* Store scan parameters */
    g_ble_ll_scan_params.own_addr_type = cmd->own_addr_type;
    g_ble_ll_scan_params.scan_filt_policy = cmd->filter_policy;

    scanp = &g_ble_ll_scan_params.scan_phys[PHY_UNCODED];
    scanp->configured = 1;
    scanp->scan_type = cmd->scan_type;
    scanp->timing.interval = ble_ll_scan_time_hci_to_ticks(scan_itvl);
    scanp->timing.window = ble_ll_scan_time_hci_to_ticks(scan_window);

#if (BLE_LL_SCAN_PHY_NUMBER == 2)
    scanp = &g_ble_ll_scan_params.scan_phys[PHY_CODED];
    scanp->configured = 0;
#endif

    return 0;
}

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
static int
ble_ll_scan_check_phy_params(uint8_t type, uint16_t itvl, uint16_t window)
{
    /* Check scan type */
    if ((type != BLE_HCI_SCAN_TYPE_PASSIVE) &&
        (type != BLE_HCI_SCAN_TYPE_ACTIVE)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /* Check interval and window */
    if ((itvl < BLE_HCI_SCAN_ITVL_MIN) ||
        // (itvl > BLE_HCI_SCAN_ITVL_MAX_EXT) ||
        (window < BLE_HCI_SCAN_WINDOW_MIN) ||
        // (window > BLE_HCI_SCAN_WINDOW_MAX_EXT) ||
        (itvl < window)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    return 0;
}

int
ble_ll_scan_hci_set_ext_params(const uint8_t *cmdbuf, uint8_t len)
{
    const struct ble_hci_le_set_ext_scan_params_cp *cmd = (const void *) cmdbuf;
    const struct scan_params *params = cmd->scans;

    struct ble_ll_scan_phy new_params[BLE_LL_SCAN_PHY_NUMBER] = { };
    struct ble_ll_scan_phy *uncoded = &new_params[PHY_UNCODED];
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CODED_PHY)
    struct ble_ll_scan_phy *coded = &new_params[PHY_CODED];
#endif
    uint16_t interval;
    uint16_t window;
    int rc;

    if (len <= sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    len -= sizeof(*cmd);

    /* If already enabled, we return an error */
    if (g_ble_ll_scan_sm.scan_enabled) {
        return BLE_ERR_CMD_DISALLOWED;
    }

    /* Check own addr type */
    if (cmd->own_addr_type > BLE_HCI_ADV_OWN_ADDR_MAX) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /* Check scanner filter policy */
    if (cmd->filter_policy > BLE_HCI_SCAN_FILT_MAX) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /* Check if no reserved bits in PHYS are set and that at least one valid PHY
     * is set.
     */
    if (!(cmd->phys & SCAN_VALID_PHY_MASK) ||
        (cmd->phys & ~SCAN_VALID_PHY_MASK)) {
         return BLE_ERR_INV_HCI_CMD_PARMS;
     }

    if (cmd->phys & BLE_HCI_LE_PHY_1M_PREF_MASK) {
        if (len < sizeof(*params)) {
            return BLE_ERR_INV_HCI_CMD_PARMS;
        }

        interval = le16toh(params->itvl);
        window = le16toh(params->window);

        rc = ble_ll_scan_check_phy_params(params->type, interval, window);
        if (rc) {
            return rc;
        }

        uncoded->scan_type = params->type;
        uncoded->timing.interval = ble_ll_scan_time_hci_to_ticks(interval);
        uncoded->timing.window = ble_ll_scan_time_hci_to_ticks(window);

        /* That means user wants to use this PHY for scanning */
        uncoded->configured = 1;
        params++;
        len -= sizeof(*params);
    }

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CODED_PHY)
    if (cmd->phys & BLE_HCI_LE_PHY_CODED_PREF_MASK) {
        if (len < sizeof(*params)) {
            return BLE_ERR_INV_HCI_CMD_PARMS;
        }

        interval = le16toh(params->itvl);
        window = le16toh(params->window);

        rc = ble_ll_scan_check_phy_params(params->type, interval, window);
        if (rc) {
            return rc;
        }

        coded->scan_type = params->type;
        coded->timing.interval = ble_ll_scan_time_hci_to_ticks(interval);
        coded->timing.window = ble_ll_scan_time_hci_to_ticks(window);

        /* That means user wants to use this PHY for scanning */
        coded->configured = 1;
    }

    /* if any of PHYs is configured for continuous scan we alter interval to
     * fit other PHY
     */
    if (coded->configured && uncoded->configured) {
        if (coded->timing.interval == coded->timing.window) {
            coded->timing.interval += uncoded->timing.window;
        }

        if (uncoded->timing.interval == uncoded->timing.window) {
            uncoded->timing.interval += coded->timing.window;
        }
    }
#endif

    g_ble_ll_scan_params.own_addr_type = cmd->own_addr_type;
    g_ble_ll_scan_params.scan_filt_policy = cmd->filter_policy;

    memcpy(g_ble_ll_scan_params.scan_phys, new_params, sizeof(new_params));

    return 0;
}

#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
static void
ble_ll_scan_duration_period_timers_restart(struct ble_ll_scan_sm *scansm)
{
    ble_npl_callout_stop(&scansm->duration_timer);
    ble_npl_callout_stop(&scansm->period_timer);

    if (scansm->duration_ticks) {
        ble_npl_callout_reset(&scansm->duration_timer,
                              scansm->duration_ticks);

        if (scansm->period_ticks) {
            ble_npl_callout_reset(&scansm->period_timer,
                                  scansm->period_ticks);
        }
    }
}

static void
ble_ll_scan_duration_timer_cb(struct ble_npl_event *ev)
{
    struct ble_ll_scan_sm *scansm = ble_npl_event_get_arg(ev);

    ble_ll_scan_sm_stop(2);

    /* if period is set both timers get started from period cb */
    if (!scansm->period_ticks) {
        ble_ll_hci_ev_send_scan_timeout();
    }
}

static void
ble_ll_scan_period_timer_cb(struct ble_npl_event *ev)
{
    struct ble_ll_scan_sm *scansm = ble_npl_event_get_arg(ev);

    ble_ll_scan_sm_start(scansm);

    /* always start timer regardless of ble_ll_scan_sm_start result
     * if it failed will restart in next period
     */
    ble_ll_scan_duration_period_timers_restart(scansm);
}
#endif

/**
 * ble ll scan set enable
 *
 * HCI scan set enable command processing function
 *
 *  Context: Link Layer task (HCI Command parser).
 *
 * @return int BLE error code.
 */
static int
ble_ll_scan_set_enable(uint8_t enable, uint8_t filter_dups, uint16_t period,
                       uint16_t dur, bool ext)
{
    int rc;
    struct ble_ll_scan_sm *scansm;
    struct ble_ll_scan_phy *scanp;
    struct ble_ll_scan_phy *scanp_phy;
    int i;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    ble_npl_time_t period_ticks = 0;
    ble_npl_time_t dur_ticks = 0;
#endif

    /* Check for valid parameters */
    if ((filter_dups > 1) || (enable > 1)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    scansm = &g_ble_ll_scan_sm;

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    if (ext) {
        /*
         * If Enable is set to 0x01 and the Host has not issued the
         * HCI_LE_Set_Extended_Scan_Parameters command, the Controller shall
         * either use vendor-specified parameters or return the error code
         * Command Disallowed (0x0C)
         *
         * To keep things simple for devices without public address we
         * reject in such case.
         */
        for (i = 0; i < BLE_LL_SCAN_PHY_NUMBER; i++) {
            if (g_ble_ll_scan_params.scan_phys[i].configured) {
                break;
            }
        }

        if (i == BLE_LL_SCAN_PHY_NUMBER) {
            return BLE_ERR_CMD_DISALLOWED;
        }
    }

    /* we can do that here since value will never change until reset */
    scansm->ext_scanning = ext;

    if (ext) {
        /* Period parameter is ignored when the Duration parameter is zero */
        if (!dur) {
            period = 0;
        }

        /* period is in 1.28 sec units */
        if (ble_npl_time_ms_to_ticks(period * 1280, &period_ticks)) {
            return BLE_ERR_INV_HCI_CMD_PARMS;
        }

        /* duration is in 10ms units */
        dur_ticks = ble_npl_time_ms_to_ticks32(dur * 10);

        if (dur_ticks && period_ticks && (dur_ticks >= period_ticks)) {
            return BLE_ERR_INV_HCI_CMD_PARMS;
        }
    }
#endif

    /* disable*/
    if (!enable) {
        if (scansm->scan_enabled) {
            ble_ll_scan_sm_stop(1);
        }
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
        ble_npl_callout_stop(&scansm->duration_timer);
        ble_npl_callout_stop(&scansm->period_timer);
#endif

        return BLE_ERR_SUCCESS;
    }

    /* if already enable we just need to update parameters */
    if (scansm->scan_enabled) {
        /* Controller does not allow initiating and scanning.*/
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
        for (i = 0; i < BLE_LL_SCAN_PHY_NUMBER; i++) {
            scanp_phy = &scansm->scan_phys[i];
            if (scanp_phy->configured &&
                                scanp_phy->scan_type == BLE_SCAN_TYPE_INITIATE) {
                return BLE_ERR_CMD_DISALLOWED;
            }
        }
#endif

#if MYNEWT_VAL(BLE_LL_NUM_SCAN_DUP_ADVS)
        /* update filter policy */
        scansm->scan_filt_dups = filter_dups;
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
        /* restart timers according to new settings */
        scansm->duration_ticks = dur_ticks;
        scansm->period_ticks = period_ticks;
        ble_ll_scan_duration_period_timers_restart(scansm);
#endif

        return BLE_ERR_SUCCESS;
    }

    /* we can store those upfront regardless of start scan result since scan is
     * disabled now
     */

#if MYNEWT_VAL(BLE_LL_NUM_SCAN_DUP_ADVS)
    scansm->scan_filt_dups = filter_dups;
#endif
    scansm->scanp = NULL;
    scansm->scanp_next = NULL;

    scansm->own_addr_type = g_ble_ll_scan_params.own_addr_type;
    scansm->scan_filt_policy = g_ble_ll_scan_params.scan_filt_policy;

    for (i = 0; i < BLE_LL_SCAN_PHY_NUMBER; i++) {
        scanp_phy = &scansm->scan_phys[i];
        scanp = &g_ble_ll_scan_params.scan_phys[i];

        if (!scanp->configured) {
            continue;
        }

        scanp_phy->configured = scanp->configured;
        scanp_phy->scan_type = scanp->scan_type;
        scanp_phy->timing = scanp->timing;

        if (!scansm->scanp) {
            scansm->scanp = scanp_phy;
        } else {
            scansm->scanp_next = scanp_phy;
        }
    }

    /* spec is not really clear if we should use defaults in this case
     * or just disallow starting scan without explicit configuration
     * For now be nice to host and just use values based on LE Set Scan
     * Parameters defaults.
     */
    if (!scansm->scanp) {
        scansm->scanp = &scansm->scan_phys[PHY_UNCODED];
        scansm->own_addr_type = BLE_ADDR_PUBLIC;
        scansm->scan_filt_policy = BLE_HCI_SCAN_FILT_NO_WL;

        scanp_phy = scansm->scanp;
        scanp_phy->configured = 1;
        scanp_phy->scan_type = BLE_SCAN_TYPE_PASSIVE;
        scanp_phy->timing.interval =
                        ble_ll_scan_time_hci_to_ticks(BLE_HCI_SCAN_ITVL_DEF);
        scanp_phy->timing.window =
                        ble_ll_scan_time_hci_to_ticks(BLE_HCI_SCAN_WINDOW_DEF);
    }

    rc = ble_ll_scan_sm_start(scansm);

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    if (rc == BLE_ERR_SUCCESS) {
        scansm->duration_ticks = dur_ticks;
        scansm->period_ticks = period_ticks;
        ble_ll_scan_duration_period_timers_restart(scansm);
    }
#endif

    return rc;
}

int ble_ll_scan_hci_set_enable(const uint8_t *cmdbuf, uint8_t len)
{
    const struct ble_hci_le_set_scan_enable_cp *cmd = (const void *) cmdbuf;

    if (len != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    return ble_ll_scan_set_enable(cmd->enable, cmd->filter_duplicates, 0, 0,
                                  false);
}

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
int ble_ll_scan_hci_set_ext_enable(const uint8_t *cmdbuf, uint8_t len)
{
    const struct ble_hci_le_set_ext_scan_enable_cp *cmd = (const void *) cmdbuf;

    if (len != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    return ble_ll_scan_set_enable(cmd->enable, cmd->filter_dup,
                                  le16toh(cmd->period), le16toh(cmd->duration),
                                  true);
}
#endif

/**
 * Checks if controller can change the whitelist. If scanning is enabled and
 * using the whitelist the controller is not allowed to change the whitelist.
 *
 * @return int 0: not allowed to change whitelist; 1: change allowed.
 */
int
ble_ll_scan_can_chg_whitelist(void)
{
    int rc;
    struct ble_ll_scan_sm *scansm;

    scansm = &g_ble_ll_scan_sm;
    if (scansm->scan_enabled && (scansm->scan_filt_policy & 1)) {
        rc = 0;
    } else {
        rc = 1;
    }

    return rc;
}

#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
int
ble_ll_scan_initiator_start(struct ble_ll_conn_sm *connsm, uint8_t ext,
                            struct ble_ll_conn_create_scan *cc_scan)
{
    struct ble_ll_scan_sm *scansm;
    struct ble_ll_scan_phy *scanp_uncoded;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV) && MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CODED_PHY)
    struct ble_ll_scan_phy *scanp_coded;
#endif
    uint8_t init_phy_mask;
    int rc;

    scansm = &g_ble_ll_scan_sm;
    scansm->own_addr_type = cc_scan->own_addr_type;
    scansm->scan_filt_policy = cc_scan->filter_policy;
    scansm->scanp = NULL;
    scansm->scanp_next = NULL;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    scansm->ext_scanning = ext;
    init_phy_mask = cc_scan->init_phy_mask;
#else
    init_phy_mask = BLE_PHY_MASK_1M;
#endif
    scansm->connsm = connsm;

    scanp_uncoded = &scansm->scan_phys[PHY_UNCODED];
    if (init_phy_mask & BLE_PHY_MASK_1M) {
        scanp_uncoded->configured = 1;
        scanp_uncoded->timing.interval = ble_ll_scan_time_hci_to_ticks(
                                cc_scan->scan_params[PHY_UNCODED].itvl);
        scanp_uncoded->timing.window = ble_ll_scan_time_hci_to_ticks(
                                cc_scan->scan_params[PHY_UNCODED].window);
        scanp_uncoded->scan_type = BLE_SCAN_TYPE_INITIATE;
        scansm->scanp = scanp_uncoded;
    } else {
        scanp_uncoded->configured = 0;
    }

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV) && MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CODED_PHY)
    scanp_coded = &scansm->scan_phys[PHY_CODED];
    if (init_phy_mask & BLE_PHY_MASK_CODED) {
        scanp_coded->configured = 1;
        scanp_coded->timing.interval = ble_ll_scan_time_hci_to_ticks(
                                cc_scan->scan_params[PHY_CODED].itvl);
        scanp_coded->timing.window = ble_ll_scan_time_hci_to_ticks(
                                cc_scan->scan_params[PHY_CODED].window);
        scanp_coded->scan_type = BLE_SCAN_TYPE_INITIATE;
        if (scansm->scanp) {
            scansm->scanp_next = scanp_coded;
        } else {
            scansm->scanp = scanp_coded;
        }
    } else {
        scanp_coded->configured = 0;
    }

    /* if any of PHYs is configured for continuous scan we alter interval to
     * fit other PHY
     */
    if (scansm->scanp && scansm->scanp_next && scanp_coded->configured &&
        scanp_uncoded->configured) {
        if (scanp_coded->timing.interval == scanp_coded->timing.window) {
            scanp_coded->timing.interval += scanp_uncoded->timing.window;
        }

        if (scanp_uncoded->timing.interval == scanp_uncoded->timing.window) {
            scanp_uncoded->timing.interval += scanp_coded->timing.window;
        }
    }
#endif

    rc = ble_ll_scan_sm_start(scansm);
    if (rc == 0) {
        g_ble_ll_conn_create_sm.connsm = connsm;
    }

    return rc;
}
#endif

/**
 * Checks to see if the scanner is enabled.
 *
 * @return int 0: not enabled; enabled otherwise
 */
int
ble_ll_scan_enabled(void)
{
    return (int)g_ble_ll_scan_sm.scan_enabled;
}

/**
 * Returns the peer resolvable private address of last device connecting to us
 *
 * @return uint8_t*
 */
uint8_t *
ble_ll_scan_get_peer_rpa(void)
{
    struct ble_ll_scan_sm *scansm;

    /* XXX: should this go into IRK list or connection? */
    scansm = &g_ble_ll_scan_sm;
    return scansm->scan_peer_rpa;
}

/**
 * Returns the local resolvable private address currently being using by
 * the scanner/initiator
 *
 * @return uint8_t*
 */
uint8_t *
ble_ll_scan_get_local_rpa(void)
{
    return g_ble_ll_scan_sm.pdu_data.scana;
}

/**
 * Set the Resolvable Private Address in the scanning (or initiating) state
 * machine.
 *
 * XXX: should this go into IRK list or connection?
 *
 * @param rpa
 */
void
ble_ll_scan_set_peer_rpa(uint8_t *rpa)
{
    struct ble_ll_scan_sm *scansm;

    scansm = &g_ble_ll_scan_sm;
    memcpy(scansm->scan_peer_rpa, rpa, BLE_DEV_ADDR_LEN);
}

struct ble_ll_scan_pdu_data *
ble_ll_scan_get_pdu_data(void)
{
    return &g_ble_ll_scan_sm.pdu_data;
}

static void
ble_ll_scan_common_init(void)
{
    struct ble_ll_scan_sm *scansm;
    struct ble_ll_scan_phy *scanp;
    int i;

    /* Clear state machine in case re-initialized */
    scansm = &g_ble_ll_scan_sm;
    memset(scansm, 0, sizeof(struct ble_ll_scan_sm));

    /* Clear scan parameters in case re-initialized */
    memset(&g_ble_ll_scan_params, 0, sizeof(g_ble_ll_scan_params));

    /* Initialize scanning window end event */
    ble_npl_event_init(&scansm->scan_sched_ev, ble_ll_scan_event_proc, scansm);

    for (i = 0; i < BLE_LL_SCAN_PHY_NUMBER; i++) {
        /* Set all non-zero default parameters */
        scanp = &g_ble_ll_scan_params.scan_phys[i];
        scanp->timing.interval =
                        ble_ll_scan_time_hci_to_ticks(BLE_HCI_SCAN_ITVL_DEF);
        scanp->timing.window =
                        ble_ll_scan_time_hci_to_ticks(BLE_HCI_SCAN_WINDOW_DEF);
    }

    scansm->scan_phys[PHY_UNCODED].phy = BLE_PHY_1M;
#if (BLE_LL_SCAN_PHY_NUMBER == 2)
    scansm->scan_phys[PHY_CODED].phy = BLE_PHY_CODED;
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
    /* Make sure we'll generate new NRPA if necessary */
    scansm->scan_nrpa_timer = ble_npl_time_get();
#endif

    /* Initialize scanning timer */
    ble_ll_tmr_init(&scansm->scan_timer, ble_ll_scan_timer_cb, scansm);

    /* Initialize extended scan timers */
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    ble_npl_callout_init(&scansm->duration_timer, &g_ble_ll_data.ll_evq,
                                        ble_ll_scan_duration_timer_cb, scansm);
    ble_npl_callout_init(&scansm->period_timer, &g_ble_ll_data.ll_evq,
                                        ble_ll_scan_period_timer_cb, scansm);
#endif

    ble_npl_event_init(&scansm->scan_interrupted_ev, ble_ll_scan_interrupted_event_cb, NULL);
}

/**
 * Called when the controller receives the reset command. Resets the
 * scanning state machine to its initial state.
 *
 * @return int
 */
void
ble_ll_scan_reset(void)
{
    struct ble_ll_scan_sm *scansm;

    scansm = &g_ble_ll_scan_sm;

    /* If enabled, stop it. */
    if (scansm->scan_enabled) {
        ble_ll_scan_sm_stop(0);
    }

    /* stop extended scan timers */
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    ble_npl_callout_stop(&scansm->duration_timer);
    ble_npl_callout_stop(&scansm->period_timer);
#endif

    /* Reset duplicate advertisers and those from which we rxd a response */
    g_ble_ll_scan_num_rsp_advs = 0;
    memset(&g_ble_ll_scan_rsp_advs[0], 0, sizeof(g_ble_ll_scan_rsp_advs));

    os_mempool_clear(&g_scan_dup_pool);
    TAILQ_INIT(&g_scan_dup_list);

    /* Call the common init function again */
    ble_ll_scan_common_init();
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    ble_ll_scan_aux_init();
#endif
}

/**
 * ble ll scan init
 *
 * Initialize a scanner. Must be called before scanning can be started.
 * Expected to be called with a un-initialized scanning state machine.
 */
void
ble_ll_scan_init(void)
{
    os_error_t err;

    err = os_mempool_init(&g_scan_dup_pool,
                          MYNEWT_VAL(BLE_LL_NUM_SCAN_DUP_ADVS),
                          sizeof(struct ble_ll_scan_dup_entry),
                          g_scan_dup_mem,
                          "ble_ll_scan_dup_pool");
    BLE_LL_ASSERT(err == 0);

    TAILQ_INIT(&g_scan_dup_list);

    ble_ll_scan_common_init();
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    ble_ll_scan_aux_init();
#endif
}

#endif
#endif