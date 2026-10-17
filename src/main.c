/*
 * Copyright (c) 2026, Marcio Bulla
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "button.h"
#include <stdbool.h>
#include <stdint.h>
#include <sys/errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

static const char board[] = CONFIG_BOARD_TARGET;

#define UART1_NODE DT_NODELABEL(uart1)
static const struct device *const uart1 = DEVICE_DT_GET(UART1_NODE);

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

K_MSGQ_DEFINE(rx_msgq, 1, 16, 1);

static uint8_t rx_msg[1];
static atomic_t rx_peak = ATOMIC_INIT(0);
static atomic_t rx_dropped = ATOMIC_INIT(0);

static void uart1_rx_cb(const struct device *dev, void *user_data) {
  uart_irq_update(uart1);

  if (uart_irq_rx_ready(uart1) == 1) {
    while (uart_fifo_read(uart1, rx_msg, 1) > 0) {
      if (k_msgq_put(&rx_msgq, rx_msg, K_NO_WAIT) == 0) {
        uint32_t used = k_msgq_num_used_get(&rx_msgq);

        if (used > atomic_get(&rx_peak)) {
          atomic_set(&rx_peak, used);
        }
      } else {
        atomic_inc(&rx_dropped);
      }
    }
  }
}

void rx_read_entry(void *, void *, void *) {
  uint8_t msg;

  while (true) {
    k_msgq_get(&rx_msgq, &msg, K_FOREVER);
    printk("%c", msg);
    if (msg == '\n') {
      gpio_pin_toggle_dt(&led);

      printk("RX queue peak: %ld, dropped: %ld\n", (long)atomic_get(&rx_peak),
             (long)atomic_get(&rx_dropped));
    }
  }
}

K_THREAD_DEFINE(rx_read_tid, 1024, &rx_read_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {

  if (!device_is_ready(uart1)) {
    printk("UART1 not Ready\n");
    return -ENODEV;
  }

  if (!gpio_is_ready_dt(&led)) {
    printk("LED0 not ready\n");
    return -ENODEV;
  }

  int ret;

  ret = button_init();
  if (ret < 0) {
    LOG_ERR("Failed to initialize button: %d", ret);
    return ret;
  }

  ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
  if (ret != 0) {
    printk("Pin Led Configure error!\n");
    return ret;
  }

  ret = uart_irq_callback_user_data_set(uart1, &uart1_rx_cb, NULL);
  if (ret != 0) {
    printk("Error to set IRQ on uart1!");
    return ret;
  }

  uart_irq_rx_enable(uart1);

  char text_msg[sizeof(board) + 1];
  snprintf(text_msg, sizeof(text_msg), "%s\n", board);

  while (true) {
    ret = button_wait_press();
    if (ret < 0) {
      LOG_ERR("Failed to wait for button press: %d", ret);
      return ret;
    }

    for (uint8_t i = 0; i < strlen(text_msg); i++) {
      uart_poll_out(uart1, text_msg[i]);
    }
  }

  return 0;
}
