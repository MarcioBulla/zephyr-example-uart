/**
 *  @file uart_link.h
 *  @brief Interface for the UART1 link.
 *
 * Copyright (c) 2026, Marcio Bulla
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UART_LINK_H_
#define UART_LINK_H_

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Handle one byte received by the UART1 link.
 *
 * Called from the RX thread after a byte is removed from the message queue.
 *
 * @param byte Received byte.
 * @param user_data Context provided to uart1_init().
 */
typedef void (*uart_link_rx_byte_cb_t)(uint8_t byte, const void *user_data);

/**
 * @brief Initialize UART1 and enable interrupt-driven reception.
 *
 * The callback and its context are retained and used by the RX thread.
 *
 * @param on_byte Callback invoked for each received byte; must not be NULL.
 * @param user_data Context passed unchanged to @p on_byte.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int uart1_init(uart_link_rx_byte_cb_t on_byte, const void *user_data);

/**
 * @brief Send exactly @p size_msg bytes through UART1 using polling.
 *
 * This function does not append a string terminator or delimiter.
 *
 * @param text_msg Buffer containing the bytes to send.
 * @param size_msg Number of bytes to send.
 */
void uart1_send(char text_msg[], size_t size_msg);


#endif /* UART_LINK_H_ */
