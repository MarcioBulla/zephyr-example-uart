/*
 * @file button.c
 * @brief implementation of button's configuration and application.
 *
 * Copyright (c) 2026, Marcio Bulla
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "button.h"
#include <sys/errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(button, LOG_LEVEL_INF);

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

static const struct gpio_dt_spec user_button =
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, user_button_gpios);

static struct gpio_callback user_button_cb_data;

static K_SEM_DEFINE(press_sem, 0, 10);

static void user_button_work_handler(struct k_work *work) {
  if (gpio_pin_get_dt(&user_button) == 1) {
    k_sem_give(&press_sem);
  }
}

K_WORK_DELAYABLE_DEFINE(user_button_work, user_button_work_handler);

static void user_button_cb(const struct device *port, struct gpio_callback *cb,
                           gpio_port_pins_t pins) {
  k_work_reschedule(&user_button_work, K_MSEC(30));
}

int button_init(void) {
  int ret;

  if (!gpio_is_ready_dt(&user_button)) {
    LOG_ERR("Button GPIO device is not ready");
    return -ENODEV;
  }

  ret = gpio_pin_configure_dt(&user_button, GPIO_INPUT);
  if (ret < 0) {
    LOG_ERR("Failed to configure button GPIO: %d", ret);
    return ret;
  }

  gpio_init_callback(&user_button_cb_data, &user_button_cb,
                     BIT(user_button.pin));

  ret = gpio_add_callback(user_button.port, &user_button_cb_data);
  if (ret < 0) {
    LOG_ERR("Failed to add button callback %d", ret);
    return ret;
  }

  ret = gpio_pin_interrupt_configure_dt(&user_button, GPIO_INT_EDGE_TO_ACTIVE);
  if (ret < 0) {
    LOG_ERR("Failed to configure button interrupt %d", ret);
    return ret;
  }

  return 0;
}

int button_wait_press(void) {
  int ret;

  ret = k_sem_take(&press_sem, K_FOREVER);
  if (ret < 0) {
    LOG_ERR("Failed to take button semaphore: %d", ret);
    return ret;
  }

  return 0;
}
