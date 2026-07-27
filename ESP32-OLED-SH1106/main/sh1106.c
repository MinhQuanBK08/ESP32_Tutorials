#include "sh1106.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "font5x7.h"

#define SH1106_I2C_FREQUENCY_HZ       400000
#define SH1106_I2C_TIMEOUT_MS             100

/* I2C control bytes defined by the SH1106 serial interface. */
#define SH1106_CONTROL_COMMAND          0x00
#define SH1106_CONTROL_DATA             0x40

/* Frequently used SH1106 commands. */
#define SH1106_CMD_DISPLAY_OFF          0xAE
#define SH1106_CMD_DISPLAY_ON           0xAF
#define SH1106_CMD_SET_CONTRAST         0x81
#define SH1106_CMD_NORMAL_DISPLAY       0xA6
#define SH1106_CMD_INVERT_DISPLAY       0xA7

static const char *TAG = "sh1106";

static int16_t sh1106_min3(int16_t a, int16_t b, int16_t c)
{
    int16_t result = (a < b) ? a : b;
    return (result < c) ? result : c;
}

static int16_t sh1106_max3(int16_t a, int16_t b, int16_t c)
{
    int16_t result = (a > b) ? a : b;
    return (result > c) ? result : c;
}

/**
 * @brief Calculate floor(sqrt(value)) using integer arithmetic.
 *
 * This avoids floating-point math in ellipse drawing and works safely for the
 * small coordinate ranges used by a 128x64 display.
 */
static uint32_t sh1106_integer_sqrt(uint64_t value)
{
    uint64_t result = 0;
    uint64_t bit = (uint64_t)1 << 62;

    while (bit > value) {
        bit >>= 2;
    }

    while (bit != 0) {
        if (value >= result + bit) {
            value -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }

        bit >>= 2;
    }

    return (uint32_t)result;
}

/**
 * @brief Return the signed edge function for triangle rasterization.
 */
static int64_t sh1106_edge_function(int16_t ax,
                                    int16_t ay,
                                    int16_t bx,
                                    int16_t by,
                                    int16_t px,
                                    int16_t py)
{
    return (int64_t)(px - ax) * (by - ay) -
           (int64_t)(py - ay) * (bx - ax);
}

/**
 * @brief Send one or more command bytes to the display.
 */
static esp_err_t sh1106_write_commands(sh1106_t *display,
                                       const uint8_t *commands,
                                       size_t command_count)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "Display pointer is NULL");
    ESP_RETURN_ON_FALSE(display->i2c_device != NULL, ESP_ERR_INVALID_STATE, TAG,
                        "I2C device is not initialized");
    ESP_RETURN_ON_FALSE(commands != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "Command pointer is NULL");
    ESP_RETURN_ON_FALSE(command_count > 0, ESP_ERR_INVALID_ARG, TAG,
                        "Command count is zero");

    /*
     * The first byte is the I2C control byte. A value of 0x00 tells the
     * controller that all following bytes in this transaction are commands.
     */
    uint8_t packet[32];

    ESP_RETURN_ON_FALSE(command_count <= (sizeof(packet) - 1),
                        ESP_ERR_INVALID_SIZE, TAG,
                        "Too many command bytes");

    packet[0] = SH1106_CONTROL_COMMAND;
    memcpy(&packet[1], commands, command_count);

    return i2c_master_transmit(display->i2c_device,
                               packet,
                               command_count + 1,
                               SH1106_I2C_TIMEOUT_MS);
}

/**
 * @brief Send display data bytes.
 */
static esp_err_t sh1106_write_data(sh1106_t *display,
                                   const uint8_t *data,
                                   size_t data_length)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "Display pointer is NULL");
    ESP_RETURN_ON_FALSE(display->i2c_device != NULL, ESP_ERR_INVALID_STATE, TAG,
                        "I2C device is not initialized");
    ESP_RETURN_ON_FALSE(data != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "Data pointer is NULL");
    ESP_RETURN_ON_FALSE(data_length <= SH1106_WIDTH, ESP_ERR_INVALID_SIZE, TAG,
                        "Data block is too large");

    /*
     * One page contains 128 bytes. The extra first byte (0x40) selects
     * display-data mode for the entire I2C transaction.
     */
    uint8_t packet[SH1106_WIDTH + 1];

    packet[0] = SH1106_CONTROL_DATA;
    memcpy(&packet[1], data, data_length);

    return i2c_master_transmit(display->i2c_device,
                               packet,
                               data_length + 1,
                               SH1106_I2C_TIMEOUT_MS);
}

esp_err_t sh1106_init(sh1106_t *display,
                      i2c_master_bus_handle_t bus_handle,
                      uint8_t i2c_address)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "Display pointer is NULL");
    ESP_RETURN_ON_FALSE(bus_handle != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "I2C bus handle is NULL");

    memset(display, 0, sizeof(*display));

    /*
     * Register the OLED as one device on the existing I2C bus.
     * device_address is the raw 7-bit address, not an 8-bit read/write address.
     */
    const i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = i2c_address,
        .scl_speed_hz = SH1106_I2C_FREQUENCY_HZ,
    };

    ESP_RETURN_ON_ERROR(
        i2c_master_bus_add_device(bus_handle,
                                  &device_config,
                                  &display->i2c_device),
        TAG,
        "Failed to add SH1106 to the I2C bus");

    /*
     * Initialization sequence for a common 128x64 SH1106 OLED module.
     * Some unusual modules may require different segment/common remapping.
     */
    static const uint8_t init_sequence[] = {
        0xAE,       /* Display OFF while configuration is in progress. */
        0xD5, 0x80, /* Set display clock divide ratio and oscillator frequency. */
        0xA8, 0x3F, /* Set multiplex ratio to 1/64 duty. */
        0xD3, 0x00, /* Set vertical display offset to zero. */
        0x40,       /* Set display start line to line 0. */
        0xAD, 0x8B, /* Enable the internal DC-DC converter. */
        0xA1,       /* Remap segment 0 to column 127. */
        0xC8,       /* Scan COM outputs from COM63 to COM0. */
        0xDA, 0x12, /* Set COM pins hardware configuration. */
        0x81, 0x7F, /* Set medium display contrast. */
        0xD9, 0x22, /* Set pre-charge and discharge periods. */
        0xDB, 0x35, /* Set VCOM deselect level. */
        0xA4,       /* Use display RAM contents, not "all pixels on". */
        0xA6,       /* Use normal, non-inverted display mode. */
        0xAF        /* Display ON. */
    };

    ESP_RETURN_ON_ERROR(
        sh1106_write_commands(display,
                              init_sequence,
                              sizeof(init_sequence)),
        TAG,
        "SH1106 initialization sequence failed");

    sh1106_clear(display, SH1106_COLOR_BLACK);
    ESP_RETURN_ON_ERROR(sh1106_display(display), TAG,
                        "Failed to clear the OLED after initialization");

    ESP_LOGI(TAG, "SH1106 initialized at I2C address 0x%02X", i2c_address);
    return ESP_OK;
}

esp_err_t sh1106_display(sh1106_t *display)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "Display pointer is NULL");

    /*
     * SH1106 memory is organized as 8 pages of 8 vertical pixels.
     * For every page:
     *   1. Select the page.
     *   2. Select the starting controller column.
     *   3. Transfer 128 bytes from the local framebuffer.
     */
    for (uint8_t page = 0; page < SH1106_PAGE_COUNT; ++page) {
        const uint8_t column = SH1106_COLUMN_OFFSET;
        const uint8_t address_commands[] = {
            (uint8_t)(0xB0 | page),
            (uint8_t)(0x00 | (column & 0x0F)),
            (uint8_t)(0x10 | ((column >> 4) & 0x0F))
        };

        ESP_RETURN_ON_ERROR(
            sh1106_write_commands(display,
                                  address_commands,
                                  sizeof(address_commands)),
            TAG,
            "Failed to set page/column address");

        ESP_RETURN_ON_ERROR(
            sh1106_write_data(display,
                              &display->framebuffer[page * SH1106_WIDTH],
                              SH1106_WIDTH),
            TAG,
            "Failed to transfer framebuffer page");
    }

    return ESP_OK;
}

void sh1106_clear(sh1106_t *display, sh1106_color_t color)
{
    if (display == NULL) {
        return;
    }

    memset(display->framebuffer,
           (color == SH1106_COLOR_WHITE) ? 0xFF : 0x00,
           sizeof(display->framebuffer));
}

void sh1106_draw_pixel(sh1106_t *display,
                       int16_t x,
                       int16_t y,
                       sh1106_color_t color)
{
    if (display == NULL ||
        x < 0 || x >= SH1106_WIDTH ||
        y < 0 || y >= SH1106_HEIGHT) {
        return;
    }

    const size_t index = (size_t)x + ((size_t)y / 8U) * SH1106_WIDTH;
    const uint8_t mask = (uint8_t)(1U << ((uint8_t)y & 0x07U));

    switch (color) {
        case SH1106_COLOR_WHITE:
            display->framebuffer[index] |= mask;
            break;

        case SH1106_COLOR_BLACK:
            display->framebuffer[index] &= (uint8_t)~mask;
            break;

        case SH1106_COLOR_INVERT:
            display->framebuffer[index] ^= mask;
            break;

        default:
            break;
    }
}

void sh1106_draw_line(sh1106_t *display,
                      int16_t x0,
                      int16_t y0,
                      int16_t x1,
                      int16_t y1,
                      sh1106_color_t color)
{
    /*
     * Integer-only Bresenham algorithm.
     * It supports horizontal, vertical, diagonal, and reversed lines.
     */
    int16_t dx = (int16_t)abs(x1 - x0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t dy = (int16_t)-abs(y1 - y0);
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t error = (int16_t)(dx + dy);

    while (true) {
        sh1106_draw_pixel(display, x0, y0, color);

        if (x0 == x1 && y0 == y1) {
            break;
        }

        const int16_t error2 = (int16_t)(2 * error);

        if (error2 >= dy) {
            error = (int16_t)(error + dy);
            x0 = (int16_t)(x0 + sx);
        }

        if (error2 <= dx) {
            error = (int16_t)(error + dx);
            y0 = (int16_t)(y0 + sy);
        }
    }
}

void sh1106_draw_rectangle(sh1106_t *display,
                           int16_t x,
                           int16_t y,
                           int16_t width,
                           int16_t height,
                           sh1106_color_t color)
{
    if (display == NULL || width <= 0 || height <= 0) {
        return;
    }

    sh1106_draw_line(display, x, y, x + width - 1, y, color);
    sh1106_draw_line(display, x, y + height - 1,
                     x + width - 1, y + height - 1, color);
    sh1106_draw_line(display, x, y, x, y + height - 1, color);
    sh1106_draw_line(display, x + width - 1, y,
                     x + width - 1, y + height - 1, color);
}

void sh1106_fill_rectangle(sh1106_t *display,
                           int16_t x,
                           int16_t y,
                           int16_t width,
                           int16_t height,
                           sh1106_color_t color)
{
    if (display == NULL || width <= 0 || height <= 0) {
        return;
    }

    for (int16_t row = 0; row < height; ++row) {
        sh1106_draw_line(display,
                         x,
                         y + row,
                         x + width - 1,
                         y + row,
                         color);
    }
}

void sh1106_draw_circle(sh1106_t *display,
                        int16_t center_x,
                        int16_t center_y,
                        int16_t radius,
                        sh1106_color_t color)
{
    if (display == NULL || radius < 0) {
        return;
    }

    /*
     * Midpoint circle algorithm. Eight symmetric points are emitted for every
     * calculated point in the first octant.
     */
    int16_t x = radius;
    int16_t y = 0;
    int16_t decision = (int16_t)(1 - radius);

    while (x >= y) {
        sh1106_draw_pixel(display, center_x + x, center_y + y, color);
        sh1106_draw_pixel(display, center_x + y, center_y + x, color);
        sh1106_draw_pixel(display, center_x - y, center_y + x, color);
        sh1106_draw_pixel(display, center_x - x, center_y + y, color);
        sh1106_draw_pixel(display, center_x - x, center_y - y, color);
        sh1106_draw_pixel(display, center_x - y, center_y - x, color);
        sh1106_draw_pixel(display, center_x + y, center_y - x, color);
        sh1106_draw_pixel(display, center_x + x, center_y - y, color);

        ++y;

        if (decision <= 0) {
            decision = (int16_t)(decision + 2 * y + 1);
        } else {
            --x;
            decision = (int16_t)(decision + 2 * (y - x) + 1);
        }
    }
}

void sh1106_fill_circle(sh1106_t *display,
                        int16_t center_x,
                        int16_t center_y,
                        int16_t radius,
                        sh1106_color_t color)
{
    if (display == NULL || radius < 0) {
        return;
    }

    int16_t x = radius;
    int16_t y = 0;
    int16_t decision = (int16_t)(1 - radius);

    while (x >= y) {
        /*
         * Draw four horizontal spans. Duplicate spans are harmless and keep
         * the implementation straightforward.
         */
        sh1106_draw_line(display,
                         center_x - x, center_y + y,
                         center_x + x, center_y + y,
                         color);
        sh1106_draw_line(display,
                         center_x - x, center_y - y,
                         center_x + x, center_y - y,
                         color);
        sh1106_draw_line(display,
                         center_x - y, center_y + x,
                         center_x + y, center_y + x,
                         color);
        sh1106_draw_line(display,
                         center_x - y, center_y - x,
                         center_x + y, center_y - x,
                         color);

        ++y;

        if (decision <= 0) {
            decision = (int16_t)(decision + 2 * y + 1);
        } else {
            --x;
            decision = (int16_t)(decision + 2 * (y - x) + 1);
        }
    }
}

void sh1106_draw_ellipse(sh1106_t *display,
                         int16_t center_x,
                         int16_t center_y,
                         int16_t radius_x,
                         int16_t radius_y,
                         sh1106_color_t color)
{
    if (display == NULL || radius_x < 0 || radius_y < 0) {
        return;
    }

    if (radius_x == 0) {
        sh1106_draw_line(display,
                         center_x,
                         center_y - radius_y,
                         center_x,
                         center_y + radius_y,
                         color);
        return;
    }

    if (radius_y == 0) {
        sh1106_draw_line(display,
                         center_x - radius_x,
                         center_y,
                         center_x + radius_x,
                         center_y,
                         color);
        return;
    }

    const uint64_t rx2 = (uint64_t)radius_x * radius_x;
    const uint64_t ry2 = (uint64_t)radius_y * radius_y;

    /*
     * Sweep both axes. Combining the two sweeps prevents visible holes on
     * ellipses with very different horizontal and vertical radii.
     */
    for (int16_t dy = (int16_t)-radius_y; dy <= radius_y; ++dy) {
        const uint64_t dy2 = (uint64_t)((int32_t)dy * dy);
        const uint64_t numerator = rx2 * (ry2 - dy2);
        const int16_t x = (int16_t)sh1106_integer_sqrt(numerator / ry2);

        sh1106_draw_pixel(display, center_x + x, center_y + dy, color);
        sh1106_draw_pixel(display, center_x - x, center_y + dy, color);
    }

    for (int16_t dx = (int16_t)-radius_x; dx <= radius_x; ++dx) {
        const uint64_t dx2 = (uint64_t)((int32_t)dx * dx);
        const uint64_t numerator = ry2 * (rx2 - dx2);
        const int16_t y = (int16_t)sh1106_integer_sqrt(numerator / rx2);

        sh1106_draw_pixel(display, center_x + dx, center_y + y, color);
        sh1106_draw_pixel(display, center_x + dx, center_y - y, color);
    }
}

void sh1106_fill_ellipse(sh1106_t *display,
                         int16_t center_x,
                         int16_t center_y,
                         int16_t radius_x,
                         int16_t radius_y,
                         sh1106_color_t color)
{
    if (display == NULL || radius_x < 0 || radius_y < 0) {
        return;
    }

    if (radius_x == 0) {
        sh1106_draw_line(display,
                         center_x,
                         center_y - radius_y,
                         center_x,
                         center_y + radius_y,
                         color);
        return;
    }

    if (radius_y == 0) {
        sh1106_draw_line(display,
                         center_x - radius_x,
                         center_y,
                         center_x + radius_x,
                         center_y,
                         color);
        return;
    }

    const uint64_t rx2 = (uint64_t)radius_x * radius_x;
    const uint64_t ry2 = (uint64_t)radius_y * radius_y;

    for (int16_t dy = (int16_t)-radius_y; dy <= radius_y; ++dy) {
        const uint64_t dy2 = (uint64_t)((int32_t)dy * dy);
        const uint64_t numerator = rx2 * (ry2 - dy2);
        const int16_t x = (int16_t)sh1106_integer_sqrt(numerator / ry2);

        sh1106_draw_line(display,
                         center_x - x,
                         center_y + dy,
                         center_x + x,
                         center_y + dy,
                         color);
    }
}

void sh1106_draw_triangle(sh1106_t *display,
                          int16_t x0,
                          int16_t y0,
                          int16_t x1,
                          int16_t y1,
                          int16_t x2,
                          int16_t y2,
                          sh1106_color_t color)
{
    if (display == NULL) {
        return;
    }

    sh1106_draw_line(display, x0, y0, x1, y1, color);
    sh1106_draw_line(display, x1, y1, x2, y2, color);
    sh1106_draw_line(display, x2, y2, x0, y0, color);
}

void sh1106_fill_triangle(sh1106_t *display,
                          int16_t x0,
                          int16_t y0,
                          int16_t x1,
                          int16_t y1,
                          int16_t x2,
                          int16_t y2,
                          sh1106_color_t color)
{
    if (display == NULL) {
        return;
    }

    /*
     * Clip the bounding box before testing pixels. The edge-function test
     * accepts either clockwise or counter-clockwise vertex order.
     */
    int16_t min_x = sh1106_min3(x0, x1, x2);
    int16_t max_x = sh1106_max3(x0, x1, x2);
    int16_t min_y = sh1106_min3(y0, y1, y2);
    int16_t max_y = sh1106_max3(y0, y1, y2);

    if (min_x < 0) {
        min_x = 0;
    }
    if (min_y < 0) {
        min_y = 0;
    }
    if (max_x >= SH1106_WIDTH) {
        max_x = SH1106_WIDTH - 1;
    }
    if (max_y >= SH1106_HEIGHT) {
        max_y = SH1106_HEIGHT - 1;
    }

    for (int16_t y = min_y; y <= max_y; ++y) {
        for (int16_t x = min_x; x <= max_x; ++x) {
            const int64_t w0 =
                sh1106_edge_function(x1, y1, x2, y2, x, y);
            const int64_t w1 =
                sh1106_edge_function(x2, y2, x0, y0, x, y);
            const int64_t w2 =
                sh1106_edge_function(x0, y0, x1, y1, x, y);

            const bool all_non_negative = (w0 >= 0 && w1 >= 0 && w2 >= 0);
            const bool all_non_positive = (w0 <= 0 && w1 <= 0 && w2 <= 0);

            if (all_non_negative || all_non_positive) {
                sh1106_draw_pixel(display, x, y, color);
            }
        }
    }
}

void sh1106_draw_bitmap(sh1106_t *display,
                        int16_t x,
                        int16_t y,
                        int16_t width,
                        int16_t height,
                        const uint8_t *bitmap,
                        sh1106_color_t color)
{
    if (display == NULL ||
        bitmap == NULL ||
        width <= 0 ||
        height <= 0) {
        return;
    }

    const int16_t page_count = (int16_t)((height + 7) / 8);

    for (int16_t page = 0; page < page_count; ++page) {
        for (int16_t column = 0; column < width; ++column) {
            const uint8_t byte = bitmap[column + page * width];

            for (int16_t bit = 0; bit < 8; ++bit) {
                const int16_t bitmap_y = (int16_t)(page * 8 + bit);

                if (bitmap_y >= height) {
                    break;
                }

                if ((byte & (1U << bit)) != 0U) {
                    sh1106_draw_pixel(display,
                                      x + column,
                                      y + bitmap_y,
                                      color);
                }
            }
        }
    }
}

void sh1106_draw_char(sh1106_t *display,
                      int16_t x,
                      int16_t y,
                      char character,
                      sh1106_color_t color)
{
    if (display == NULL) {
        return;
    }

    uint8_t ascii = (uint8_t)character;

    if (ascii < 0x20 || ascii > 0x7F) {
        ascii = (uint8_t)'?';
    }

    const uint8_t *glyph = font5x7[ascii - 0x20];

    /*
     * Draw five font columns. Each set bit represents one foreground pixel.
     * The sixth column and eighth row are left blank for character spacing.
     */
    for (int16_t column = 0; column < 5; ++column) {
        const uint8_t bits = glyph[column];

        for (int16_t row = 0; row < 7; ++row) {
            if ((bits & (1U << row)) != 0U) {
                sh1106_draw_pixel(display, x + column, y + row, color);
            } else if (color != SH1106_COLOR_INVERT) {
                sh1106_draw_pixel(display,
                                  x + column,
                                  y + row,
                                  (color == SH1106_COLOR_WHITE)
                                      ? SH1106_COLOR_BLACK
                                      : SH1106_COLOR_WHITE);
            }
        }
    }

    /*
     * Clear the spacing column and row when using normal foreground colors.
     */
    if (color != SH1106_COLOR_INVERT) {
        const sh1106_color_t background =
            (color == SH1106_COLOR_WHITE)
                ? SH1106_COLOR_BLACK
                : SH1106_COLOR_WHITE;

        for (int16_t row = 0; row < 8; ++row) {
            sh1106_draw_pixel(display, x + 5, y + row, background);
        }

        for (int16_t column = 0; column < 5; ++column) {
            sh1106_draw_pixel(display, x + column, y + 7, background);
        }
    }
}

void sh1106_draw_string(sh1106_t *display,
                        int16_t x,
                        int16_t y,
                        const char *text,
                        sh1106_color_t color)
{
    if (display == NULL || text == NULL) {
        return;
    }

    const int16_t start_x = x;
    int16_t cursor_x = x;
    int16_t cursor_y = y;

    while (*text != '\0') {
        if (*text == '\n') {
            cursor_x = start_x;
            cursor_y = (int16_t)(cursor_y + 8);
            ++text;
            continue;
        }

        if (cursor_x + 5 >= SH1106_WIDTH) {
            cursor_x = start_x;
            cursor_y = (int16_t)(cursor_y + 8);
        }

        if (cursor_y + 7 >= SH1106_HEIGHT) {
            break;
        }

        sh1106_draw_char(display, cursor_x, cursor_y, *text, color);
        cursor_x = (int16_t)(cursor_x + 6);
        ++text;
    }
}

void sh1106_draw_string_no_wrap(sh1106_t *display,
                                int16_t x,
                                int16_t y,
                                const char *text,
                                sh1106_color_t color)
{
    if (display == NULL || text == NULL) {
        return;
    }

    int16_t cursor_x = x;

    while (*text != '\0') {
        if (*text != '\n') {
            sh1106_draw_char(display, cursor_x, y, *text, color);
            cursor_x = (int16_t)(cursor_x + 6);
        }

        ++text;
    }
}

int16_t sh1106_get_string_width(const char *text)
{
    if (text == NULL) {
        return 0;
    }

    size_t character_count = 0;

    while (text[character_count] != '\0' &&
           text[character_count] != '\n') {
        ++character_count;
    }

    /*
     * Clamp the return value to INT16_MAX even though real OLED strings are
     * normally much shorter.
     */
    if (character_count > ((size_t)INT16_MAX / 6U)) {
        return INT16_MAX;
    }

    return (int16_t)(character_count * 6U);
}

esp_err_t sh1106_set_contrast(sh1106_t *display, uint8_t contrast)
{
    const uint8_t commands[] = {
        SH1106_CMD_SET_CONTRAST,
        contrast
    };

    return sh1106_write_commands(display, commands, sizeof(commands));
}

esp_err_t sh1106_set_invert(sh1106_t *display, bool invert)
{
    const uint8_t command = invert
        ? SH1106_CMD_INVERT_DISPLAY
        : SH1106_CMD_NORMAL_DISPLAY;

    return sh1106_write_commands(display, &command, 1);
}

esp_err_t sh1106_set_power(sh1106_t *display, bool on)
{
    const uint8_t command = on
        ? SH1106_CMD_DISPLAY_ON
        : SH1106_CMD_DISPLAY_OFF;

    return sh1106_write_commands(display, &command, 1);
}
