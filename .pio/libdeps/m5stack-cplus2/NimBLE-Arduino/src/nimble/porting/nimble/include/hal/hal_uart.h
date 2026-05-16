/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup HAL
 * @{
 *   @defgroup HALUart HAL UART
 *   @{
 */

#ifndef H_HAL_UART_H_
#define H_HAL_UART_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include "driver/uart.h"

/**
 * Function prototype for UART driver to ask for more data to send.
 * Returns -1 if no more data is available for TX.
 * Driver must call this with interrupts disabled.
 */
typedef int (*hal_uart_tx_char)(void *arg);

/**
 * Function prototype for UART driver to report that transmission is
 * complete. This should be called when transmission of last byte is
 * finished.
 * Driver must call this with interrupts disabled.
 */
typedef void (*hal_uart_tx_done)(void *arg);

/**
 * Function prototype for UART driver to report incoming byte of data.
 * Returns -1 if data was dropped.
 * Driver must call this with interrupts disabled.
 */
typedef int (*hal_uart_rx_char)(void *arg, uint8_t byte);

/**
 * Initializes given uart. Mapping of logical UART number to physical
 * UART/GPIO pins is in BSP.
 */
int hal_uart_init_cbs(int uart, hal_uart_tx_char tx_func,
  hal_uart_tx_done tx_done, hal_uart_rx_char rx_func, void *arg);

/**
 * De Initialize already registerd callbacks
 */
void hal_uart_deinit_cbs(void);

enum hal_uart_parity {
    /** No Parity */
    HAL_UART_PARITY_NONE = 0,
    /** Odd parity */
    HAL_UART_PARITY_ODD = 1,
    /** Even parity */
    HAL_UART_PARITY_EVEN = 2
};

enum hal_uart_flow_ctl {
    /** No Flow Control */
    HAL_UART_FLOW_CTL_NONE = 0,
    /** RTS/CTS */
    HAL_UART_FLOW_CTL_RTS_CTS = 1
};

/**
 * Initialize the HAL uart.
 *
 * @param uart  The uart number to configure
 * @param cfg   Hardware specific uart configuration.  This is passed from BSP
 *              directly to the MCU specific driver.
 *
 * @return 0 on success, non-zero error code on failure
 */
int hal_uart_init(int uart, void *cfg);

/**
 * Applies given configuration to UART.
 *
 * @param uart The UART number to configure
 * @param speed The baudrate in bps to configure
 * @param databits The number of databits to send per byte
 * @param stopbits The number of stop bits to send
 * @param parity The UART parity
 * @param flow_ctl Flow control settings on the UART
 *
 * @return 0 on success, non-zero error code on failure
 */
int hal_uart_config(int uart, int32_t speed, uint8_t databits, uint8_t stopbits,
  enum hal_uart_parity parity, enum hal_uart_flow_ctl flow_ctl);

/**
 * Close UART port. Can call hal_uart_config() with different settings after
 * calling this.
 *
 * @param uart The UART number to close
 */
int hal_uart_close(int uart);

/**
 * More data queued for transmission. UART driver will start asking for that
 * data.
 *
 * @param uart The UART number to start TX on
 */
void hal_uart_start_tx(int uart);

/**
 * Upper layers have consumed some data, and are now ready to receive more.
 * This is meaningful after uart_rx_char callback has returned -1 telling
 * that no more data can be accepted.
 *
 * @param uart The UART number to begin RX on
 */
void hal_uart_start_rx(int uart);

/**
 * This is type of write where UART has to block until character has been sent.
 * Used when printing diag output from system crash.
 * Must be called with interrupts disabled.
 *
 * @param uart The UART number to TX on
 * @param byte The byte to TX on the UART
 */
void hal_uart_blocking_tx(int uart, uint8_t byte);

#ifdef __cplusplus
}
#endif


#endif /* H_HAL_UART_H_ */


/**
 *   @} HALUart
 * @} HAL
 */
