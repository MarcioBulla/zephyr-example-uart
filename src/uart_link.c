/**
 *  @file uart_link.c
 *  @brief UART1 transport with interrupt-driven RX and polling TX.
 *
 * Copyright (c) 2026, Marcio Bulla
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "uart_link.h"
#include <stddef.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart_link, LOG_LEVEL_INF);

#define UART1_NODE DT_NODELABEL(uart1)

static const struct device *const uart1 = DEVICE_DT_GET(UART1_NODE);

static uart_link_rx_byte_cb_t rx_callback;
static const void *rx_context;

static uint8_t rx_msg[1];

K_MSGQ_DEFINE(rx_msgq, 1, 16, 1);

static void uart1_rx_cb(const struct device *dev, void *user_data) {
  uart_irq_update(uart1);

  if (uart_irq_rx_ready(uart1) == 1) {
    while (uart_fifo_read(uart1, rx_msg, 1) > 0) {
      k_msgq_put(&rx_msgq, rx_msg, K_NO_WAIT);
    }
  }
}

static void rx_read_entry(void *p1, void *p2, void *p3) {
  uint8_t msg;

  while (true) {
    k_msgq_get(&rx_msgq, &msg, K_FOREVER);
    rx_callback(msg, rx_context);
  }
}

K_THREAD_DEFINE(rx_read_tid, 1024, &rx_read_entry, NULL, NULL, NULL, 5, 0, 0);

int uart1_init(uart_link_rx_byte_cb_t on_byte, const void *user_data) {
  rx_callback = on_byte;
  rx_context = user_data;
  int ret;

  if (!device_is_ready(uart1)) {
    printk("UART1 not Ready\n");
    return -ENODEV;
  }

  ret = uart_irq_callback_user_data_set(uart1, &uart1_rx_cb, NULL);
  if (ret != 0) {
    LOG_ERR("Failed to set IRQ on uart1: %d", ret);
    return ret;
  }

  uart_irq_rx_enable(uart1);

  return 0;
}

void uart1_send(char *text_msg, size_t size_msg) {

  for (size_t i = 0; i < size_msg; i++) {
    uart_poll_out(uart1, text_msg[i]);
  }
}
