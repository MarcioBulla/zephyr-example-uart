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
#include <zephyr/sleep.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util_macro.h>

#define UART1_NODE DT_NODELABEL(uart1)
static const struct device *const uart1 = DEVICE_DT_GET(UART1_NODE);

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)
static const struct gpio_dt_spec user_button =
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, user_button_gpios);
static struct gpio_callback user_button_cb_data;

static atomic_t button_event = ATOMIC_INIT(0);

static void user_button_cb(const struct device *port, struct gpio_callback *cb,
                           gpio_port_pins_t pins) {
  atomic_inc(&button_event);
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

  while (true) {
    printk("Button events: %ld\n", (long)atomic_get(&button_event));
    k_msleep(500);
  }

  return 0;
}
