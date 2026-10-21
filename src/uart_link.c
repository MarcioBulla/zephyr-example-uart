/**
 *  @file uart_link.c
 *  @brief UART1 transport with interrupt-driven RX and TX.
 *
 * Copyright (c) 2026, Marcio Bulla
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "uart_link.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>

#define UART_TIMEOUT_US                                                        \
  (3U * 10U * 1000000U / DT_PROP(DT_NODELABEL(uart1), current_speed))

LOG_MODULE_REGISTER(uart_link, LOG_LEVEL_INF);

#define UART1_NODE DT_NODELABEL(uart1)

static const struct device *const uart1 = DEVICE_DT_GET(UART1_NODE);

static uart_link_rx_byte_cb_t rx_callback;
static const void *rx_context;

K_FIFO_DEFINE(rx_fifo);
K_FIFO_DEFINE(tx_fifo);
K_SEM_DEFINE(rx_loss_sem, 0, 1);

NET_BUF_POOL_VAR_DEFINE(rx_pool, 4, 1 + 1 + 255 + 2, 0, NULL);
NET_BUF_POOL_VAR_DEFINE(tx_pool, 4, 1 + 1 + 255 + 2, 0, NULL);

enum frame_state_t {
  STATE_WAIT_START,
  STATE_READ_LEN,
  STATE_READ_DATA,
};

static void uart1_isr(const struct device *dev, void *user_data) {
  uart_irq_update(uart1);

  if (uart_irq_tx_ready(uart1)) {
    static struct net_buf *tx_buf;
    int tx_len;

    if (!tx_buf) {
      tx_buf = k_fifo_get(&tx_fifo, K_NO_WAIT);
    }
    if (!tx_buf) {
      uart_irq_tx_disable(uart1);
    } else {

      tx_len = uart_fifo_fill(uart1, tx_buf->data, tx_buf->len);
      if (tx_len > 0) {
        net_buf_pull(tx_buf, tx_len);
        if (!tx_buf->len) {
          net_buf_unref(tx_buf);
          tx_buf = NULL;
        }
      }
    }
  }

  if (uart_irq_rx_ready(uart1)) {
    static struct net_buf *rx_buf;
    static enum frame_state_t state = STATE_WAIT_START;
    static uint16_t rx_len;
    uint8_t byte;

    while (uart_fifo_read(uart1, &byte, 1) > 0) {
      switch (state) {
      case STATE_WAIT_START:
        if (byte == 0x14) {
          state = STATE_READ_LEN;
        }
        break;

      case STATE_READ_LEN:
        if (byte == 0U) {
          state = STATE_WAIT_START;
          break;
        }

        rx_len = byte + 2;
        rx_buf = net_buf_alloc_len(&rx_pool, rx_len, K_NO_WAIT);
        if (rx_buf) {
          state = STATE_READ_DATA;
        } else {
          k_sem_give(&rx_loss_sem);
          state = STATE_WAIT_START;
        }
        break;

      case STATE_READ_DATA:
        net_buf_add_u8(rx_buf, byte);
        if (--rx_len == 0) {
          k_fifo_put(&rx_fifo, rx_buf);
          rx_buf = NULL;
          state = STATE_WAIT_START;
        }
        break;
      }
    }
  }
}

static void rx_read_entry(void *p1, void *p2, void *p3) {
  static struct net_buf *msg_buf;
  struct k_poll_event rx_events[2];

  k_poll_event_init(&rx_events[0], K_POLL_TYPE_FIFO_DATA_AVAILABLE,
                    K_POLL_MODE_NOTIFY_ONLY, &rx_fifo);

  k_poll_event_init(&rx_events[1], K_POLL_TYPE_SEM_AVAILABLE,
                    K_POLL_MODE_NOTIFY_ONLY, &rx_loss_sem);

  while (true) {
    rx_events[0].state = K_POLL_STATE_NOT_READY;
    rx_events[1].state = K_POLL_STATE_NOT_READY;

    int ret = k_poll(rx_events, 2, K_FOREVER);
    if (ret != 0) {
      continue;
    }

    if (k_sem_take(&rx_loss_sem, K_NO_WAIT) == 0) {
      struct net_buf *discarded;

      while ((discarded = k_fifo_get(&rx_fifo, K_NO_WAIT)) != NULL) {
        net_buf_unref(discarded);
      }

      LOG_WRN("Lost a message");
      continue;
    }

    msg_buf = k_fifo_get(&rx_fifo, K_NO_WAIT);
    if (!msg_buf) {
      continue;
    }

    rx_callback(msg_buf->data, msg_buf->len, rx_context);

    net_buf_pull(msg_buf, msg_buf->len);
    if (!msg_buf->len) {
      net_buf_unref(msg_buf);
      msg_buf = NULL;
    }
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

  ret = uart_irq_callback_user_data_set(uart1, &uart1_isr, NULL);
  if (ret != 0) {
    LOG_ERR("Failed to set IRQ on uart1: %d", ret);
    return ret;
  }

  uart_irq_rx_enable(uart1);

  return 0;
}

int uart1_send(char *text_msg, size_t size_msg) {
  if (text_msg == NULL || size_msg == 0) {
    LOG_WRN("Empty message");
    return -EINVAL;
  }

  if (size_msg > 259U) {
    LOG_WRN("Message too large");
    return -EMSGSIZE;
  }

  struct net_buf *buf;

  buf = net_buf_alloc_len(&tx_pool, size_msg, K_NO_WAIT);
  if (!buf) {
    LOG_ERR("Failed to allocate buffer");
    return -ENOMEM;
  }

  net_buf_add_mem(buf, text_msg, size_msg);
  k_fifo_put(&tx_fifo, buf);
  uart_irq_tx_enable(uart1);

  return 0;
}
