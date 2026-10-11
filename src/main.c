/*
 * Copyright (c) 2026, Marcio Bulla
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <sys/errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util_macro.h>

#define UART1_NODE DT_NODELABEL(uart1)
static const struct device *const uart1 = DEVICE_DT_GET(UART1_NODE);

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)
static const struct gpio_dt_spec user_button =
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, user_button_gpios);
static struct gpio_callback user_button_cb_data;

K_SEM_DEFINE(press_sem, 0, 10);

static void user_button_cb(const struct device *port, struct gpio_callback *cb,
                           gpio_port_pins_t pins) {
  k_sem_give(&press_sem);
}

K_MSGQ_DEFINE(rx_msgq, 1, 10, 1);

static uint8_t rx_msg[1];

static void uart1_rx_cb(const struct device *dev, void *user_data) {
  uart_irq_update(uart1);

  if (uart_irq_rx_ready(uart1) == 1) {
    while (uart_fifo_read(uart1, rx_msg, 1) > 0) {
      k_msgq_put(&rx_msgq, rx_msg, K_NO_WAIT);
    }
  }
}

int main(void) {

  if (!device_is_ready(uart1)) {
    printk("UART1 not Ready\n");
    return -ENODEV;
  }

  if (!gpio_is_ready_dt(&user_button)) {
    printk("User Button not ready\n");
    return -ENODEV;
  }

  int ret;

  ret = gpio_pin_configure_dt(&user_button, GPIO_INPUT);
  if (ret != 0) {
    printk("Pin Configure error!\n");
    return ret;
  }

  gpio_init_callback(&user_button_cb_data, &user_button_cb,
                     BIT(user_button.pin));

  ret = gpio_add_callback(user_button.port, &user_button_cb_data);
  if (ret != 0) {
    printk("Error to add callback in user_button!\n");
    return ret;
  }

  ret = gpio_pin_interrupt_configure_dt(&user_button, GPIO_INT_EDGE_TO_ACTIVE);
  if (ret != 0) {
    printk("Error to configure interrupt!\n");
    return ret;
  }

  ret = uart_irq_callback_user_data_set(uart1, &uart1_rx_cb, NULL);
  if (ret != 0) {
    printk("Error to set IRQ on uart1!");
    return ret;
  }

  uart_irq_rx_enable(uart1);

  while (true) {
    ret = k_sem_take(&press_sem, K_FOREVER);
    if (ret != 0) {
      printk("Error to take semaphore: %d\n", ret);
      return ret;
    }

    uart_poll_out(uart1, 'B');
  }

  return 0;
}
