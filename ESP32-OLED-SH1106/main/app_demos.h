#ifndef APP_DEMOS_H
#define APP_DEMOS_H

#include <stdint.h>

#include "esp_err.h"
#include "sh1106.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Display examples of circle, rectangle, ellipse, and triangle shapes.
 */
esp_err_t app_demo_shapes(sh1106_t *display, uint32_t hold_time_ms);

/**
 * @brief Scroll one line of text from right to left.
 *
 * @param display Initialized SH1106 display.
 * @param text Null-terminated ASCII text.
 * @param y Top coordinate of the 8-pixel-high text row.
 * @param frame_delay_ms Delay between animation frames.
 * @param repetitions Number of complete scrolling passes.
 */
esp_err_t app_demo_scrolling_text(sh1106_t *display,
                                  const char *text,
                                  int16_t y,
                                  uint32_t frame_delay_ms,
                                  uint32_t repetitions);

/**
 * @brief Play a finite "mochi eyes" animation.
 *
 * The animation contains eye movement, blinking, and a happy expression.
 *
 * @param cycles Number of complete animation cycles.
 */
esp_err_t app_demo_mochi_eyes(sh1106_t *display, uint32_t cycles);

#ifdef __cplusplus
}
#endif

#endif /* APP_DEMOS_H */
