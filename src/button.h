/*
 *  @file button.h
 *  @brief Button configuration and Function.
 *
 * Copyright (c) 2026, Marcio Bulla
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BUTTON_H_
#define BUTTON_H_

/**
 * @brief Initialize the button.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int button_init(void);


/**
 * @brief Block the calling thread until a debounced button press.
 *
 * @return 0 when a button press is received.
 */
int button_wait_press(void);

#endif /* BUTTON_H_ */
