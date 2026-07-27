# ESP32 + SH1106 128x64 OLED using ESP-IDF

This project contains a standalone SH1106 driver and three demonstration
programs for ESP-IDF 5.2 or newer. It uses the bus-device I2C master API from
`driver/i2c_master.h`.

All source-code comments are written in English.

## Wiring

| SH1106 pin | ESP32 pin |
|---|---|
| VCC | 3.3 V |
| GND | GND |
| SDA | GPIO 23 |
| SCL or SCK | GPIO 22 |

For I2C, the clock signal is normally called `SCL`. Some OLED modules print
`SCK` on the PCB for the same clock pin.

Many OLED modules already contain I2C pull-up resistors. For robust hardware,
use appropriate external pull-ups to 3.3 V when they are not present.

## Project structure

```text
esp32_sh1106_espidf/
├── CMakeLists.txt
├── README.md
└── main/
    ├── CMakeLists.txt
    ├── app_demos.c
    ├── app_demos.h
    ├── font5x7.h
    ├── main.c
    ├── sh1106.c
    └── sh1106.h
```

## Included drawing functions

- Pixel
- Line
- Rectangle and filled rectangle
- Circle and filled circle
- Ellipse and filled ellipse
- Triangle and filled triangle
- Monochrome bitmap
- ASCII character and text
- No-wrap text for marquee animation

## Included demonstrations

`main/app_demos.c` contains three reusable programs:

```c
esp_err_t app_demo_shapes(sh1106_t *display, uint32_t hold_time_ms);

esp_err_t app_demo_scrolling_text(sh1106_t *display,
                                  const char *text,
                                  int16_t y,
                                  uint32_t frame_delay_ms,
                                  uint32_t repetitions);

esp_err_t app_demo_mochi_eyes(sh1106_t *display, uint32_t cycles);
```

The default `app_main()` repeatedly runs:

1. Shape demonstration.
2. Right-to-left scrolling text.
3. Mochi-eyes animation with movement, blinking, and a happy expression.

To run only one effect, replace the `while (true)` content in `main/main.c`
with the selected demo call.

## I2C configuration

The requested pins are configured in `main/main.c`:

```c
#define APP_I2C_SDA_GPIO              23
#define APP_I2C_SCL_GPIO              22
#define APP_SH1106_I2C_ADDRESS      0x3C
```

Try address `0x3D` if the OLED does not acknowledge at `0x3C`.

If the image is shifted horizontally, edit this macro in `main/sh1106.h`:

```c
#define SH1106_COLUMN_OFFSET           2
```

Depending on the module, try column offset `0` or `2`.

## Build and flash

Open an ESP-IDF terminal in this directory:

```bash
idf.py set-target esp32
idf.py build
idf.py -p COMx flash monitor
```

Replace `COMx` with the serial port used by the board.

For ESP32-S3:

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

## Complete mochi-face expression program

The project now includes `main/mochi_faces.c` and `main/mochi_faces.h`.
The module provides 15 reusable animated expressions:

- Neutral
- Happy
- Sad
- Angry
- Surprised
- Sleepy
- Wink left
- Wink right
- Love
- Confused
- Scared
- Annoyed
- Excited
- Crying
- Dizzy

Display one expression directly:

```c
ESP_ERROR_CHECK(
    mochi_face_show(
        &display,
        MOCHI_FACE_HAPPY,
        0));
```

Animate one selected expression by increasing the frame index:

```c
for (uint32_t frame = 0; frame < 100; ++frame) {
    ESP_ERROR_CHECK(
        mochi_face_show(
            &display,
            MOCHI_FACE_CRYING,
            frame));

    vTaskDelay(pdMS_TO_TICKS(80));
}
```

Run the complete collection:

```c
ESP_ERROR_CHECK(
    mochi_face_demo_all(
        &display,
        1500,
        1));
```

`hold_time_ms` controls the approximate time spent on each expression.
`repeat_count` controls the number of complete demonstration passes.
