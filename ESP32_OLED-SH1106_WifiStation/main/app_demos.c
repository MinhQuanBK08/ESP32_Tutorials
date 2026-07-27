#include "app_demos.h"

#include <stddef.h>

#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MOCHI_LEFT_EYE_X             38
#define MOCHI_RIGHT_EYE_X            90
#define MOCHI_EYE_CENTER_Y           31
#define MOCHI_EYE_RADIUS_X           19
#define MOCHI_EYE_RADIUS_Y           18
#define MOCHI_PUPIL_RADIUS            5

/**
 * @brief Delay the current FreeRTOS task using a millisecond value.
 */
static void app_delay_ms(uint32_t delay_ms)
{
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
}

/**
 * @brief Draw a single normal mochi-eyes frame.
 *
 * White filled ellipses form the eyes. Black circles form the pupils.
 * pupil_offset_x and pupil_offset_y create the look direction.
 */
static void app_draw_mochi_frame(sh1106_t *display,
                                 int16_t pupil_offset_x,
                                 int16_t pupil_offset_y,
                                 int16_t eye_opening)
{
    sh1106_clear(display, SH1106_COLOR_BLACK);

    if (eye_opening <= 2) {
        /*
         * During a blink, draw short horizontal eye lines instead of ellipses.
         */
        sh1106_draw_line(display,
                         MOCHI_LEFT_EYE_X - 15,
                         MOCHI_EYE_CENTER_Y,
                         MOCHI_LEFT_EYE_X + 15,
                         MOCHI_EYE_CENTER_Y,
                         SH1106_COLOR_WHITE);
        sh1106_draw_line(display,
                         MOCHI_RIGHT_EYE_X - 15,
                         MOCHI_EYE_CENTER_Y,
                         MOCHI_RIGHT_EYE_X + 15,
                         MOCHI_EYE_CENTER_Y,
                         SH1106_COLOR_WHITE);
        return;
    }

    sh1106_fill_ellipse(display,
                        MOCHI_LEFT_EYE_X,
                        MOCHI_EYE_CENTER_Y,
                        MOCHI_EYE_RADIUS_X,
                        eye_opening,
                        SH1106_COLOR_WHITE);

    sh1106_fill_ellipse(display,
                        MOCHI_RIGHT_EYE_X,
                        MOCHI_EYE_CENTER_Y,
                        MOCHI_EYE_RADIUS_X,
                        eye_opening,
                        SH1106_COLOR_WHITE);

    /*
     * Keep the pupils within the visible eye area.
     */
    sh1106_fill_circle(display,
                       MOCHI_LEFT_EYE_X + pupil_offset_x,
                       MOCHI_EYE_CENTER_Y + pupil_offset_y,
                       MOCHI_PUPIL_RADIUS,
                       SH1106_COLOR_BLACK);

    sh1106_fill_circle(display,
                       MOCHI_RIGHT_EYE_X + pupil_offset_x,
                       MOCHI_EYE_CENTER_Y + pupil_offset_y,
                       MOCHI_PUPIL_RADIUS,
                       SH1106_COLOR_BLACK);

    /*
     * Add one small highlight pixel to each pupil.
     */
    sh1106_fill_circle(display,
                       MOCHI_LEFT_EYE_X + pupil_offset_x - 2,
                       MOCHI_EYE_CENTER_Y + pupil_offset_y - 2,
                       1,
                       SH1106_COLOR_WHITE);

    sh1106_fill_circle(display,
                       MOCHI_RIGHT_EYE_X + pupil_offset_x - 2,
                       MOCHI_EYE_CENTER_Y + pupil_offset_y - 2,
                       1,
                       SH1106_COLOR_WHITE);
}

/**
 * @brief Draw a happy closed-eye frame using two inverted V shapes.
 */
static void app_draw_happy_mochi_frame(sh1106_t *display)
{
    sh1106_clear(display, SH1106_COLOR_BLACK);

    sh1106_draw_line(display,
                     MOCHI_LEFT_EYE_X - 16,
                     MOCHI_EYE_CENTER_Y + 6,
                     MOCHI_LEFT_EYE_X,
                     MOCHI_EYE_CENTER_Y - 5,
                     SH1106_COLOR_WHITE);
    sh1106_draw_line(display,
                     MOCHI_LEFT_EYE_X,
                     MOCHI_EYE_CENTER_Y - 5,
                     MOCHI_LEFT_EYE_X + 16,
                     MOCHI_EYE_CENTER_Y + 6,
                     SH1106_COLOR_WHITE);

    sh1106_draw_line(display,
                     MOCHI_RIGHT_EYE_X - 16,
                     MOCHI_EYE_CENTER_Y + 6,
                     MOCHI_RIGHT_EYE_X,
                     MOCHI_EYE_CENTER_Y - 5,
                     SH1106_COLOR_WHITE);
    sh1106_draw_line(display,
                     MOCHI_RIGHT_EYE_X,
                     MOCHI_EYE_CENTER_Y - 5,
                     MOCHI_RIGHT_EYE_X + 16,
                     MOCHI_EYE_CENTER_Y + 6,
                     SH1106_COLOR_WHITE);

    /*
     * Thicken the expression by drawing two adjacent copies.
     */
    sh1106_draw_line(display,
                     MOCHI_LEFT_EYE_X - 16,
                     MOCHI_EYE_CENTER_Y + 7,
                     MOCHI_LEFT_EYE_X,
                     MOCHI_EYE_CENTER_Y - 4,
                     SH1106_COLOR_WHITE);
    sh1106_draw_line(display,
                     MOCHI_LEFT_EYE_X,
                     MOCHI_EYE_CENTER_Y - 4,
                     MOCHI_LEFT_EYE_X + 16,
                     MOCHI_EYE_CENTER_Y + 7,
                     SH1106_COLOR_WHITE);

    sh1106_draw_line(display,
                     MOCHI_RIGHT_EYE_X - 16,
                     MOCHI_EYE_CENTER_Y + 7,
                     MOCHI_RIGHT_EYE_X,
                     MOCHI_EYE_CENTER_Y - 4,
                     SH1106_COLOR_WHITE);
    sh1106_draw_line(display,
                     MOCHI_RIGHT_EYE_X,
                     MOCHI_EYE_CENTER_Y - 4,
                     MOCHI_RIGHT_EYE_X + 16,
                     MOCHI_EYE_CENTER_Y + 7,
                     SH1106_COLOR_WHITE);
}

/**
 * @brief Animate pupil movement toward one target offset.
 */
static esp_err_t app_animate_look(sh1106_t *display,
                                  int16_t start_x,
                                  int16_t start_y,
                                  int16_t target_x,
                                  int16_t target_y,
                                  uint32_t step_count)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, "demo",
                        "Display pointer is NULL");
    ESP_RETURN_ON_FALSE(step_count > 0, ESP_ERR_INVALID_ARG, "demo",
                        "Step count must be greater than zero");

    for (uint32_t step = 0; step <= step_count; ++step) {
        const int16_t x =
            (int16_t)(start_x +
                      ((int32_t)(target_x - start_x) * (int32_t)step) /
                          (int32_t)step_count);
        const int16_t y =
            (int16_t)(start_y +
                      ((int32_t)(target_y - start_y) * (int32_t)step) /
                          (int32_t)step_count);

        app_draw_mochi_frame(display, x, y, MOCHI_EYE_RADIUS_Y);
        ESP_RETURN_ON_ERROR(sh1106_display(display),
                            "demo",
                            "Failed to draw eye movement frame");
        app_delay_ms(45);
    }

    return ESP_OK;
}

/**
 * @brief Play one complete blink.
 */
static esp_err_t app_animate_blink(sh1106_t *display)
{
    static const int16_t opening_sequence[] = {
        18, 14, 10, 6, 2, 6, 10, 14, 18
    };

    for (size_t frame = 0;
         frame < (sizeof(opening_sequence) / sizeof(opening_sequence[0]));
         ++frame) {
        app_draw_mochi_frame(display,
                             0,
                             0,
                             opening_sequence[frame]);

        ESP_RETURN_ON_ERROR(sh1106_display(display),
                            "demo",
                            "Failed to draw blink frame");
        app_delay_ms(40);
    }

    return ESP_OK;
}

esp_err_t app_demo_shapes(sh1106_t *display, uint32_t hold_time_ms)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, "demo",
                        "Display pointer is NULL");

    sh1106_clear(display, SH1106_COLOR_BLACK);
    sh1106_draw_string(display,
                       2,
                       1,
                       "SHAPES",
                       SH1106_COLOR_WHITE);
    sh1106_draw_line(display,
                     0,
                     10,
                     SH1106_WIDTH - 1,
                     10,
                     SH1106_COLOR_WHITE);

    /*
     * Top row: outlined versions.
     */
    sh1106_draw_circle(display,
                       18,
                       27,
                       12,
                       SH1106_COLOR_WHITE);

    sh1106_draw_rectangle(display,
                          40,
                          15,
                          27,
                          24,
                          SH1106_COLOR_WHITE);

    sh1106_draw_ellipse(display,
                        88,
                        27,
                        17,
                        11,
                        SH1106_COLOR_WHITE);

    sh1106_draw_triangle(display,
                         111,
                         38,
                         124,
                         15,
                         127,
                         38,
                         SH1106_COLOR_WHITE);

    /*
     * Bottom row: small filled versions.
     */
    sh1106_fill_circle(display,
                       18,
                       52,
                       7,
                       SH1106_COLOR_WHITE);

    sh1106_fill_rectangle(display,
                          44,
                          46,
                          19,
                          13,
                          SH1106_COLOR_WHITE);

    sh1106_fill_ellipse(display,
                        88,
                        52,
                        12,
                        7,
                        SH1106_COLOR_WHITE);

    sh1106_fill_triangle(display,
                         111,
                         59,
                         120,
                         43,
                         127,
                         59,
                         SH1106_COLOR_WHITE);

    ESP_RETURN_ON_ERROR(sh1106_display(display),
                        "demo",
                        "Failed to display shape demo");

    app_delay_ms(hold_time_ms);
    return ESP_OK;
}

esp_err_t app_demo_scrolling_text(sh1106_t *display,
                                  const char *text,
                                  int16_t y,
                                  uint32_t frame_delay_ms,
                                  uint32_t repetitions)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, "demo",
                        "Display pointer is NULL");
    ESP_RETURN_ON_FALSE(text != NULL, ESP_ERR_INVALID_ARG, "demo",
                        "Text pointer is NULL");
    ESP_RETURN_ON_FALSE(y >= 0 && y <= (SH1106_HEIGHT - 8),
                        ESP_ERR_INVALID_ARG, "demo",
                        "Text Y coordinate is outside the screen");

    const int16_t text_width = sh1106_get_string_width(text);

    for (uint32_t repetition = 0;
         repetition < repetitions;
         ++repetition) {
        /*
         * Start just beyond the right edge and stop after the complete text
         * has moved beyond the left edge.
         */
        for (int16_t x = SH1106_WIDTH; x >= -text_width; --x) {
            sh1106_clear(display, SH1106_COLOR_BLACK);

            sh1106_draw_rectangle(display,
                                  0,
                                  0,
                                  SH1106_WIDTH,
                                  SH1106_HEIGHT,
                                  SH1106_COLOR_WHITE);

            sh1106_draw_string(display,
                               35,
                               9,
                               "MARQUEE",
                               SH1106_COLOR_WHITE);

            sh1106_draw_line(display,
                             3,
                             20,
                             124,
                             20,
                             SH1106_COLOR_WHITE);

            sh1106_draw_string_no_wrap(display,
                                       x,
                                       y,
                                       text,
                                       SH1106_COLOR_WHITE);

            ESP_RETURN_ON_ERROR(sh1106_display(display),
                                "demo",
                                "Failed to display scrolling-text frame");

            app_delay_ms(frame_delay_ms);
        }
    }

    return ESP_OK;
}

esp_err_t app_demo_mochi_eyes(sh1106_t *display, uint32_t cycles)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, "demo",
                        "Display pointer is NULL");

    for (uint32_t cycle = 0; cycle < cycles; ++cycle) {
        app_draw_mochi_frame(display, 0, 0, MOCHI_EYE_RADIUS_Y);
        ESP_RETURN_ON_ERROR(sh1106_display(display),
                            "demo",
                            "Failed to draw neutral eye frame");
        app_delay_ms(450);

        ESP_RETURN_ON_ERROR(
            app_animate_look(display, 0, 0, -6, 0, 6),
            "demo",
            "Left-look animation failed");
        app_delay_ms(250);

        ESP_RETURN_ON_ERROR(
            app_animate_look(display, -6, 0, 6, 0, 10),
            "demo",
            "Right-look animation failed");
        app_delay_ms(250);

        ESP_RETURN_ON_ERROR(
            app_animate_look(display, 6, 0, 0, 0, 6),
            "demo",
            "Center-look animation failed");

        ESP_RETURN_ON_ERROR(app_animate_blink(display),
                            "demo",
                            "Blink animation failed");
        app_delay_ms(250);

        app_draw_happy_mochi_frame(display);
        ESP_RETURN_ON_ERROR(sh1106_display(display),
                            "demo",
                            "Failed to draw happy eye frame");
        app_delay_ms(800);

        ESP_RETURN_ON_ERROR(app_animate_blink(display),
                            "demo",
                            "Second blink animation failed");
    }

    return ESP_OK;
}
