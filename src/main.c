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
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util_macro.h>

static const char board[] = CONFIG_BOARD_TARGET;

#define UART1_NODE DT_NODELABEL(uart1)
static const struct device *const uart1 = DEVICE_DT_GET(UART1_NODE);

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)
static const struct gpio_dt_spec user_button =
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, user_button_gpios);
static struct gpio_callback user_button_cb_data;

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

K_SEM_DEFINE(press_sem, 0, 10);

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

  if (!gpio_is_ready_dt(&user_button)) {
    printk("User Button not ready\n");
    return -ENODEV;
  }

  if (!gpio_is_ready_dt(&led)) {
    printk("LED0 not ready\n");
    return -ENODEV;
  }

  int ret;

  ret = gpio_pin_configure_dt(&user_button, GPIO_INPUT);
  if (ret != 0) {
    printk("Pin Configure error!\n");
    return ret;
  }

  ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
  if (ret != 0) {
    printk("Pin Led Configure error!\n");
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

  char text_msg[sizeof(board) + 1];
  snprintf(text_msg, sizeof(text_msg), "%s\n", board);

  while (true) {
    ret = k_sem_take(&press_sem, K_FOREVER);
    if (ret != 0) {
      printk("Error to take semaphore: %d\n", ret);
      return ret;
    }

    for (uint8_t i = 0; i < strlen(text_msg); i++) {
      uart_poll_out(uart1, text_msg[i]);
    }
  }

  return 0;
}
