/*
 * Copyright (c) 2026, Marcio Bulla
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "button.h"
#include "uart_link.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

static const char board[] = CONFIG_BOARD_TARGET;

#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static void app_rx_byte(uint8_t byte, const void *user_data) {
  const struct gpio_dt_spec *led = user_data;

  printk("%c", byte);

  if (byte == '\n') {
    gpio_pin_toggle_dt(led);
  }
}

int main(void) {

  if (!gpio_is_ready_dt(&led)) {
    printk("LED0 not ready\n");
    return -ENODEV;
  }

  int ret;

  ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
  if (ret != 0) {
    printk("Pin Led Configure error!\n");
    return ret;
  }

  ret = button_init();
  if (ret < 0) {
    LOG_ERR("Failed to initialize button: %d", ret);
    return ret;
  }

  ret = uart1_init(app_rx_byte, &led);
  if (ret < 0) {
    LOG_ERR("Failed to initialize UART1: %d", ret);
    return ret;
  }

  char text_msg[sizeof(board) + 1];
  snprintf(text_msg, sizeof(text_msg), "%s\n", board);
  size_t size_msg = sizeof(text_msg) - 1;

  while (true) {
    ret = button_wait_press();
    if (ret < 0) {
      LOG_ERR("Failed to wait for button press: %d", ret);
      return ret;
    }

    uart1_send(text_msg, size_msg);
  }

  return 0;
}
