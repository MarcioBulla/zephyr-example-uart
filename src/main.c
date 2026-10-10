/*
 * Copyright (c) 2026, Marcio Bulla
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

#define UART1_NODE DT_NODELABEL(uart1)

static const struct device * const uart1 = DEVICE_DT_GET(UART1_NODE);
static const struct gpio_dt_spec user_button =
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, user_button_gpios);

int main(void) {

  if (!device_is_ready(uart1)) {
    printk("UART1 not Ready\n");
    return -ENODEV;
  }

  if (!gpio_is_ready_dt(&user_button)) {
    printk("User Button not ready\n");
    return -ENODEV;
  }

  gpio_pin_configure_dt(&user_button, GPIO_INPUT);

  printk("Hello World\n");

  return 0;
}
