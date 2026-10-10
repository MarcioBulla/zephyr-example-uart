#!/usr/bin/env bash

set -e

west build -b esp32s2_lolin_mini/esp32s2 \
    -d build/esp32s2_lolin_mini_esp32s2 \
    -p auto .

west build -b rpi_pico/rp2040 \
    -d build/rpi_pico_rp2040 \
    -p auto 
    --snippet cdc-acm-console .
