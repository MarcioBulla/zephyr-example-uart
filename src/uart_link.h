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
 * @brief Handle a received UART1 message candidate.
 *
 * Called from the RX thread after the current receiver has collected the
 * declared number of data bytes. The buffer is not validated by this module
 * and is valid only until the callback returns.
 *
 * @param data Received bytes; not guaranteed to be null-terminated.
 * @param len Number of valid bytes in @p data.
 * @param user_data Context provided to uart1_init().
 */
typedef void (*uart_link_rx_byte_cb_t)(const uint8_t *data, size_t len,
                                      const void *user_data);

/**
 * @brief Initialize UART1 and enable interrupt-driven reception.
 *
 * The callback and its context are retained and used by the RX thread.
 *
 * @param on_byte Callback invoked for each received message candidate; must
 *                not be NULL.
 * @param user_data Context passed unchanged to @p on_byte.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int uart1_init(uart_link_rx_byte_cb_t on_byte, const void *user_data);

/**
 * @brief Queue @p size_msg bytes for interrupt-driven transmission on UART1.
 *
 * The bytes are queued as provided. This function does not append framing,
 * CRC, a string terminator, or a delimiter. A successful return indicates
 * that the data was queued, not that transmission has finished.
 *
 * @param text_msg Buffer containing the bytes to send.
 * @param size_msg Number of bytes to send.
 *
 * @return 0 if queued, -EINVAL for an empty message, -EMSGSIZE if too large,
 *         or -ENOMEM if a transmit buffer is unavailable.
 */
int uart1_send(char text_msg[], size_t size_msg);


#endif /* UART_LINK_H_ */
