/* 
 * Copyright (c) 2026, Marcio Bulla
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

static const struct gpio_dt_spec user_button = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, user_button_gpios);

int main(void) {

  gpio_pin_configure_dt(&user_button, GPIO_INPUT);
  
  printk("Hello World\n");

  return 0;
}
