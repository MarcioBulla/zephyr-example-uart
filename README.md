# Simple Zephyr Application on Different Boards

The purpose of this project is to learn Zephyr by running a simple UART application on different boards.

## ESP32-S2 Bring-up

One of the selected boards is the Wemos ESP32-S2 Mini:
[documentation](https://www.wemos.cc/en/latest/s2/s2_mini.html)

### Objective

First, we need to build the project and successfully flash it to the board.

### Environment

- Zephyr revision: `v4.4.0`
- Board target: `esp32s2_lolin_mini/esp32s2`
- Application: `zephyr-example-uart`
- MCU: `ESP32-S2FNR2`
- USB device: `/dev/ttyACM0`
- Flash: 4 MB
- PSRAM: 2 MB

### Console Configuration

- `zephyr,console = uart0`: `build/zephyr/zephyr.dts`
- Baud rate: `115200`
- TX: GPIO16
- RX: GPIO17
- Pinctrl source:
  `zephyr/boards/wemos/esp32s2_lolin_mini/esp32s2_lolin_mini-pinctrl.dtsi`

### Flashing

`west flash` failed because the board did not reliably enter the bootloader using automatic reset.

Manual boot sequence:

1. Hold BOOT.
2. Press RESET.
3. Release RESET.
4. Release BOOT.

Flashing worked using `esptool` with `--before no-reset`.

### Verification

- UART0 TX idle voltage measured approximately 3.3 V on GPIO16.
- After a normal reset, `esptool --before no-reset chip-id` could not connect, indicating that the MCU was no longer running the ROM bootloader.

### Next Step

Validate UART output and start the bring-up of the second MCU.
