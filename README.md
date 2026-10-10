# Simple Zephyr Application on Different Boards

The purpose of this project is to learn Zephyr by running a simple UART application on different boards.

## Building

The application supports the following Zephyr board targets:

- `esp32s2_lolin_mini/esp32s2`
- `rpi_pico/rp2040` (used for the YD-RP2040 board)

Both targets can be built using the provided script:

```bash
./scripts/build-all.sh
```

The script invokes `west build` for each board, using separate build directories:

```text
build/
├── esp32s2_lolin_mini/
└── rpi_pico/
```

Build directories are reused for incremental builds. The script stops if a build fails.

To build a single board manually:

```bash
west build -b rpi_pico/rp2040 -d build/rpi_pico -p auto .
```

The script must be executed from the application root directory.

## ESP32-S2 Bring-up

One of the selected boards is the Wemos ESP32-S2 Mini:
[documentation](https://www.wemos.cc/en/latest/s2/s2_mini.html)

### Objective

First, we need to build the project and successfully flash it to the board.

### Environment

- Zephyr revision: `v4.4.0-16104-g64437be51c30`
- Board target: `esp32s2_lolin_mini/esp32s2`
- Application: `zephyr-example-uart`
- MCU: `ESP32-S2FNR2`
- USB device: `/dev/ttyACM0`
- Flash: 4 MB
- PSRAM: 2 MB

### Console Configuration

- `zephyr,console = uart0`
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

## YD-RP2040 Bring-up

The selected board is the YD-RP2040 by VCC-GND Studio.

Zephyr does not provide an official board definition for this board, so `rpi_pico/rp2040` is currently being used as a compatibility baseline.

Reference:
https://circuitpython.org/board/vcc_gnd_yd_rp2040/

### Differences from the Raspberry Pi Pico

- 16 MB flash on this board revision;
- USB-C connector;
- Reset button;
- User button on GPIO24;
- NeoPixel RGB LED on GPIO23.

### Current Baseline

- Zephyr board target: `rpi_pico/rp2040`
- Flash assumed by Zephyr: 2 MB
- Actual board flash: 16 MB
- `zephyr,console = uart0`
- Baud rate: `115200`
- TX: GPIO0
- RX: GPIO1

### Flashing

The board was placed in BOOTSEL mode:

1. Hold BOOT.
2. Press RESET.
3. Release RESET.
4. Release BOOT.

The board appeared as a removable USB drive.

The generated firmware:

`build/zephyr/zephyr.uf2`

was copied to the BOOTSEL USB drive.

After the copy completed, the drive disappeared and the board rebooted.

### Verification

- UART0 TX idle voltage measured approximately 3.3 V on GPIO0.
- This is consistent with UART TX being idle HIGH on a 3.3 V logic interface.

