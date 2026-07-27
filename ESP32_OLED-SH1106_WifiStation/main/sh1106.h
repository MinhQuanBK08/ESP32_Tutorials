#ifndef SH1106_H
#define SH1106_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/i2c.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SH1106_WIDTH               128
#define SH1106_HEIGHT               64
#define SH1106_PAGE_COUNT            8
#define SH1106_BUFFER_SIZE        (SH1106_WIDTH * SH1106_PAGE_COUNT)

/*
 * Most 128x64 SH1106 modules expose 128 visible columns from the controller's
 * 132-column display RAM. Therefore, the visible image usually starts at
 * controller column 2. Change this value if your module is horizontally shifted.
 */
#ifndef SH1106_COLUMN_OFFSET
#define SH1106_COLUMN_OFFSET         2
#endif

typedef enum {
    SH1106_COLOR_BLACK = 0,
    SH1106_COLOR_WHITE = 1,
    SH1106_COLOR_INVERT = 2
} sh1106_color_t;

typedef struct {
    i2c_port_t i2c_port;
    uint8_t i2c_address;
    bool initialized;
    uint8_t framebuffer[SH1106_BUFFER_SIZE];
} sh1106_t;

/**
 * @brief Initialize an SH1106 device on an already installed I2C master port.
 *
 * The I2C port must first be configured with i2c_param_config() and installed
 * with i2c_driver_install().
 *
 * @param display Pointer to the display object.
 * @param i2c_port ESP-IDF I2C controller number, for example I2C_NUM_0.
 * @param i2c_address 7-bit I2C address, normally 0x3C or 0x3D.
 *
 * @return ESP_OK on success, otherwise an ESP-IDF error code.
 */
esp_err_t sh1106_init(sh1106_t *display,
                      i2c_port_t i2c_port,
                      uint8_t i2c_address);

/**
 * @brief Copy the local framebuffer to the OLED display RAM.
 */
esp_err_t sh1106_display(sh1106_t *display);

/**
 * @brief Clear or fill the complete local framebuffer.
 *
 * @param color BLACK clears the buffer; WHITE fills it.
 */
void sh1106_clear(sh1106_t *display, sh1106_color_t color);

/**
 * @brief Draw one pixel into the local framebuffer.
 */
void sh1106_draw_pixel(sh1106_t *display,
                       int16_t x,
                       int16_t y,
                       sh1106_color_t color);

/**
 * @brief Draw a line using Bresenham's line algorithm.
 */
void sh1106_draw_line(sh1106_t *display,
                      int16_t x0,
                      int16_t y0,
                      int16_t x1,
                      int16_t y1,
                      sh1106_color_t color);

/**
 * @brief Draw an unfilled rectangle.
 */
void sh1106_draw_rectangle(sh1106_t *display,
                           int16_t x,
                           int16_t y,
                           int16_t width,
                           int16_t height,
                           sh1106_color_t color);

/**
 * @brief Draw a filled rectangle.
 */
void sh1106_fill_rectangle(sh1106_t *display,
                           int16_t x,
                           int16_t y,
                           int16_t width,
                           int16_t height,
                           sh1106_color_t color);

/**
 * @brief Draw an unfilled circle.
 */
void sh1106_draw_circle(sh1106_t *display,
                        int16_t center_x,
                        int16_t center_y,
                        int16_t radius,
                        sh1106_color_t color);

/**
 * @brief Draw a filled circle.
 */
void sh1106_fill_circle(sh1106_t *display,
                        int16_t center_x,
                        int16_t center_y,
                        int16_t radius,
                        sh1106_color_t color);

/**
 * @brief Draw an unfilled ellipse.
 *
 * @param radius_x Horizontal radius in pixels.
 * @param radius_y Vertical radius in pixels.
 */
void sh1106_draw_ellipse(sh1106_t *display,
                         int16_t center_x,
                         int16_t center_y,
                         int16_t radius_x,
                         int16_t radius_y,
                         sh1106_color_t color);

/**
 * @brief Draw a filled ellipse.
 */
void sh1106_fill_ellipse(sh1106_t *display,
                         int16_t center_x,
                         int16_t center_y,
                         int16_t radius_x,
                         int16_t radius_y,
                         sh1106_color_t color);

/**
 * @brief Draw an unfilled triangle.
 */
void sh1106_draw_triangle(sh1106_t *display,
                          int16_t x0,
                          int16_t y0,
                          int16_t x1,
                          int16_t y1,
                          int16_t x2,
                          int16_t y2,
                          sh1106_color_t color);

/**
 * @brief Draw a filled triangle.
 */
void sh1106_fill_triangle(sh1106_t *display,
                          int16_t x0,
                          int16_t y0,
                          int16_t x1,
                          int16_t y1,
                          int16_t x2,
                          int16_t y2,
                          sh1106_color_t color);

/**
 * @brief Draw a monochrome bitmap stored as one byte per vertical group of
 *        eight pixels.
 *
 * The bitmap byte layout matches the SH1106 framebuffer layout:
 * index = x + (y / 8) * width, bit = y % 8.
 */
void sh1106_draw_bitmap(sh1106_t *display,
                        int16_t x,
                        int16_t y,
                        int16_t width,
                        int16_t height,
                        const uint8_t *bitmap,
                        sh1106_color_t color);

/**
 * @brief Draw one 5x7 ASCII character.
 *
 * The rendered cell is 6 pixels wide and 8 pixels high, including spacing.
 */
void sh1106_draw_char(sh1106_t *display,
                      int16_t x,
                      int16_t y,
                      char character,
                      sh1106_color_t color);

/**
 * @brief Draw a null-terminated ASCII string with automatic wrapping.
 *
 * Newline characters are supported.
 */
void sh1106_draw_string(sh1106_t *display,
                        int16_t x,
                        int16_t y,
                        const char *text,
                        sh1106_color_t color);

/**
 * @brief Draw a null-terminated ASCII string without automatic wrapping.
 *
 * This function is useful for horizontal scrolling text. Characters outside
 * the display are clipped by sh1106_draw_pixel().
 */
void sh1106_draw_string_no_wrap(sh1106_t *display,
                                int16_t x,
                                int16_t y,
                                const char *text,
                                sh1106_color_t color);

/**
 * @brief Return the width of a single-line 5x7 ASCII string in pixels.
 */
int16_t sh1106_get_string_width(const char *text);

/**
 * @brief Set display contrast.
 *
 * @param contrast Value from 0x00 to 0xFF.
 */
esp_err_t sh1106_set_contrast(sh1106_t *display, uint8_t contrast);

/**
 * @brief Enable or disable hardware display inversion.
 */
esp_err_t sh1106_set_invert(sh1106_t *display, bool invert);

/**
 * @brief Turn the OLED panel on or off without deleting the framebuffer.
 */
esp_err_t sh1106_set_power(sh1106_t *display, bool on);

#ifdef __cplusplus
}
#endif

#endif /* SH1106_H */
