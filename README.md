# ESP32 Tutorials

A collection of practical ESP32 examples developed with the Espressif IoT Development Framework (ESP-IDF).

This repository is intended for students, educators, makers, and embedded-system developers who want to learn ESP32 programming through complete and well-commented examples.

## Repository Contents

| Project                                                         | Description                                                                                                                   | ESP-IDF Compatibility       |
| --------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------- | --------------------------- |
| [ESP32-OLED-SH1106](ESP32-OLED-SH1106/)                         | SH1106 OLED driver, drawing functions, scrolling text, animations, and Mochi face expressions using the modern I2C master API | ESP-IDF 5.2 or later        |
| [ESP32_OLED-SH1106_WifiStation](ESP32_OLED-SH1106_WifiStation/) | SH1106 OLED examples with Wi-Fi station connectivity and IPv4 address display using the legacy I2C API                        | ESP-IDF 4.4 and ESP-IDF 5.x |

Each project is self-contained and includes its own source code, CMake configuration, and detailed README file.

## Current Features

* ESP32 programming with ESP-IDF
* SH1106 128×64 monochrome OLED driver
* I2C communication
* Pixel, line, rectangle, circle, ellipse, and triangle drawing
* Filled geometric shapes
* ASCII text rendering with a 5×7 font
* Right-to-left scrolling text
* Monochrome bitmap rendering
* Animated Mochi eyes
* Fifteen animated Mochi face expressions
* Wi-Fi station connection
* Automatic reconnection with a configurable retry limit
* IPv4 address display on the OLED
* Source code with detailed English comments

## Hardware Requirements

* ESP32 development board
* SH1106 128×64 I2C OLED display
* USB cable
* Jumper wires
* Optional external I2C pull-up resistors

## OLED Wiring

| SH1106 Pin | ESP32 Pin | Description  |
| ---------- | --------- | ------------ |
| VCC        | 3.3 V     | Power supply |
| GND        | GND       | Ground       |
| SDA        | GPIO 23   | I2C data     |
| SCL/SCK    | GPIO 22   | I2C clock    |

The default OLED I2C address used by these examples is:

```c
#define APP_SH1106_I2C_ADDRESS 0x3C
```

Some SH1106 modules may use address `0x3D`.

## Software Requirements

Before building the examples, install:

* [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
* Git
* Visual Studio Code with the ESP-IDF extension, or an ESP-IDF command-line environment
* The appropriate USB-to-serial driver for your ESP32 board

## Getting Started

### 1. Clone the repository

```bash
git clone https://github.com/MinhQuanBK08/ESP32_Tutorials.git
cd ESP32_Tutorials
```

### 2. Select an example

For the standard SH1106 graphics and animation example:

```bash
cd ESP32-OLED-SH1106
```

For the Wi-Fi station and OLED IP display example:

```bash
cd ESP32_OLED-SH1106_WifiStation
```

### 3. Set the target board

For a standard ESP32:

```bash
idf.py set-target esp32
```

For an ESP32-S3:

```bash
idf.py set-target esp32s3
```

### 4. Build the project

```bash
idf.py build
```

### 5. Flash and monitor

On Windows:

```bash
idf.py -p COMx flash monitor
```

Replace `COMx` with the serial port assigned to your ESP32 board, for example:

```bash
idf.py -p COM5 flash monitor
```

On Linux:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Press `Ctrl+]` to exit the ESP-IDF serial monitor.

## Wi-Fi Configuration

For the Wi-Fi station example, open:

```text
ESP32_OLED-SH1106_WifiStation/main/wifi_station.h
```

Update the following definitions:

```c
#define WIFI_STATION_SSID      "YOUR_WIFI_NAME"
#define WIFI_STATION_PASSWORD  "YOUR_WIFI_PASSWORD"
```

Do not commit real Wi-Fi passwords, API keys, access tokens, or other sensitive information to a public repository.

For production applications, use a secure configuration method such as:

* ESP-IDF Wi-Fi provisioning
* NVS-based configuration
* Encrypted NVS
* ESP-IDF menuconfig
* A separate configuration file excluded by `.gitignore`

## Mochi Face Expressions

The SH1106 example includes fifteen reusable expressions:

* Neutral
* Happy
* Sad
* Angry
* Surprised
* Sleepy
* Wink left
* Wink right
* Love
* Confused
* Scared
* Annoyed
* Excited
* Crying
* Dizzy

Display a single expression:

```c
ESP_ERROR_CHECK(
    mochi_face_show(
        &display,
        MOCHI_FACE_HAPPY,
        0));
```

Run the complete expression demonstration:

```c
ESP_ERROR_CHECK(
    mochi_face_demo_all(
        &display,
        1500,
        1));
```

## Project Structure

```text
ESP32_Tutorials/
├── ESP32-OLED-SH1106/
│   ├── main/
│   │   ├── app_demos.c
│   │   ├── app_demos.h
│   │   ├── font5x7.h
│   │   ├── main.c
│   │   ├── mochi_faces.c
│   │   ├── mochi_faces.h
│   │   ├── sh1106.c
│   │   └── sh1106.h
│   ├── CMakeLists.txt
│   └── README.md
│
├── ESP32_OLED-SH1106_WifiStation/
│   ├── main/
│   │   ├── app_demos.c
│   │   ├── app_demos.h
│   │   ├── font5x7.h
│   │   ├── main.c
│   │   ├── mochi_faces.c
│   │   ├── mochi_faces.h
│   │   ├── sh1106.c
│   │   ├── sh1106.h
│   │   ├── wifi_station.c
│   │   └── wifi_station.h
│   ├── CMakeLists.txt
│   └── README.md
│
└── README.md
```

## Troubleshooting

### `driver/i2c_master.h: No such file or directory`

The modern I2C master API requires a compatible ESP-IDF version.

Check the installed version:

```bash
idf.py --version
```

You can either:

* Upgrade to ESP-IDF 5.2 or later and use `ESP32-OLED-SH1106`, or
* Use `ESP32_OLED-SH1106_WifiStation`, which uses the legacy `driver/i2c.h` API.

### OLED does not display anything

Check the following:

1. Verify the VCC, GND, SDA, and SCL connections.
2. Confirm that the display uses I2C address `0x3C`.
3. Try address `0x3D`.
4. Check whether the OLED module includes I2C pull-up resistors.
5. Try changing the SH1106 column offset between `0` and `2`.

### Build errors after changing ESP-IDF versions

Clean the previous build files:

```bash
idf.py fullclean
idf.py build
```

### Serial port cannot be opened

* Check the USB cable.
* Close other serial-monitor applications.
* Verify the correct COM port or Linux device.
* Install the correct USB-to-serial driver.

## Learning Objectives

These examples help learners understand:

* ESP-IDF project organization
* Component-based embedded-software design
* I2C peripheral communication
* Framebuffer-based graphics
* Embedded animation techniques
* FreeRTOS delays and application timing
* ESP32 Wi-Fi event handling
* DHCP and IP address acquisition
* Error handling and diagnostic logging

## Contributing

Contributions, corrections, and new ESP32 examples are welcome.

To contribute:

1. Fork this repository.
2. Create a new branch.
3. Add or update an example.
4. Commit your changes.
5. Open a pull request.

When adding a new example, please include:

* A clear project README
* Hardware connection instructions
* Build and flashing commands
* Well-commented source code
* Troubleshooting notes

## Disclaimer

These projects are provided for educational and experimental purposes. Verify electrical connections and power requirements before connecting hardware.
