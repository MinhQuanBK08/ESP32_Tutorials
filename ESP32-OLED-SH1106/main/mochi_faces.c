#include "mochi_faces.h"

#include <stdbool.h>
#include <stddef.h>

#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LEFT_EYE_X       38
#define RIGHT_EYE_X      90
#define EYE_Y            29
#define EYE_RX           15
#define EYE_RY           13
#define PUPIL_R           4
#define MOUTH_Y          52

static const char *const s_face_names[MOCHI_FACE_COUNT] = {
    "NEUTRAL", "HAPPY", "SAD", "ANGRY", "SURPRISED",
    "SLEEPY", "WINK LEFT", "WINK RIGHT", "LOVE", "CONFUSED",
    "SCARED", "ANNOYED", "EXCITED", "CRYING", "DIZZY"
};

static void draw_label(sh1106_t *display, const char *text)
{
    int16_t x = (int16_t)((SH1106_WIDTH - sh1106_get_string_width(text)) / 2);
    sh1106_draw_string_no_wrap(display, x, 1, text, SH1106_COLOR_WHITE);
}

static void draw_open_eye(sh1106_t *display,
                          int16_t cx,
                          int16_t cy,
                          int16_t rx,
                          int16_t ry,
                          int16_t pupil_x,
                          int16_t pupil_y,
                          int16_t pupil_r)
{
    sh1106_fill_ellipse(display, cx, cy, rx, ry, SH1106_COLOR_WHITE);
    sh1106_fill_circle(display,
                       cx + pupil_x,
                       cy + pupil_y,
                       pupil_r,
                       SH1106_COLOR_BLACK);

    if (pupil_r >= 3) {
        sh1106_draw_pixel(display,
                          cx + pupil_x - 1,
                          cy + pupil_y - 2,
                          SH1106_COLOR_WHITE);
    }
}

static void draw_closed_eye(sh1106_t *display, int16_t cx, int16_t cy)
{
    sh1106_draw_line(display, cx - 13, cy, cx, cy + 5, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, cx, cy + 5, cx + 13, cy, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, cx - 13, cy + 1, cx, cy + 6, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, cx, cy + 6, cx + 13, cy + 1, SH1106_COLOR_WHITE);
}

static void draw_happy_eye(sh1106_t *display, int16_t cx, int16_t cy)
{
    sh1106_draw_line(display, cx - 13, cy + 5, cx, cy - 4, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, cx, cy - 4, cx + 13, cy + 5, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, cx - 13, cy + 6, cx, cy - 3, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, cx, cy - 3, cx + 13, cy + 6, SH1106_COLOR_WHITE);
}

static void draw_x_eye(sh1106_t *display, int16_t cx, int16_t cy)
{
    sh1106_draw_line(display, cx - 8, cy - 8, cx + 8, cy + 8, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, cx + 8, cy - 8, cx - 8, cy + 8, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, cx - 7, cy - 8, cx + 9, cy + 8, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, cx + 7, cy - 8, cx - 9, cy + 8, SH1106_COLOR_WHITE);
}

static void draw_star_eye(sh1106_t *display, int16_t cx, int16_t cy)
{
    sh1106_draw_line(display, cx - 9, cy, cx + 9, cy, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, cx, cy - 9, cx, cy + 9, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, cx - 6, cy - 6, cx + 6, cy + 6, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, cx + 6, cy - 6, cx - 6, cy + 6, SH1106_COLOR_WHITE);
}

static void draw_heart(sh1106_t *display, int16_t cx, int16_t cy, int16_t size)
{
    int16_t r = (int16_t)(size / 3);
    int16_t d = (int16_t)(size / 3);

    sh1106_fill_circle(display, cx - d, cy - d, r, SH1106_COLOR_WHITE);
    sh1106_fill_circle(display, cx + d, cy - d, r, SH1106_COLOR_WHITE);
    sh1106_fill_triangle(display,
                         cx - size, cy - d,
                         cx + size, cy - d,
                         cx, cy + size,
                         SH1106_COLOR_WHITE);
}

static void draw_flat_mouth(sh1106_t *display, int16_t half_width)
{
    sh1106_draw_line(display,
                     64 - half_width, MOUTH_Y,
                     64 + half_width, MOUTH_Y,
                     SH1106_COLOR_WHITE);
    sh1106_draw_line(display,
                     64 - half_width, MOUTH_Y + 1,
                     64 + half_width, MOUTH_Y + 1,
                     SH1106_COLOR_WHITE);
}

static void draw_smile(sh1106_t *display)
{
    sh1106_draw_line(display, 52, 49, 58, 54, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, 58, 54, 64, 56, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, 64, 56, 70, 54, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, 70, 54, 76, 49, SH1106_COLOR_WHITE);
}

static void draw_frown(sh1106_t *display)
{
    sh1106_draw_line(display, 52, 56, 58, 52, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, 58, 52, 64, 50, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, 64, 50, 70, 52, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, 70, 52, 76, 56, SH1106_COLOR_WHITE);
}

static void draw_open_mouth(sh1106_t *display,
                            int16_t cx,
                            int16_t cy,
                            int16_t rx,
                            int16_t ry)
{
    sh1106_fill_ellipse(display, cx, cy, rx, ry, SH1106_COLOR_WHITE);

    if (rx >= 5 && ry >= 5) {
        sh1106_fill_ellipse(display,
                            cx,
                            cy + 1,
                            rx - 3,
                            ry - 3,
                            SH1106_COLOR_BLACK);
    }
}

static void draw_sad_brows(sh1106_t *display)
{
    sh1106_draw_line(display, 25, 20, 48, 15, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, 80, 15, 103, 20, SH1106_COLOR_WHITE);
}

static void draw_angry_brows(sh1106_t *display)
{
    sh1106_draw_line(display, 24, 15, 48, 21, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, 80, 21, 104, 15, SH1106_COLOR_WHITE);
}

static void draw_tear(sh1106_t *display, int16_t x, int16_t y)
{
    sh1106_fill_circle(display, x, y + 2, 3, SH1106_COLOR_WHITE);
    sh1106_fill_triangle(display,
                         x, y - 5,
                         x - 3, y + 1,
                         x + 3, y + 1,
                         SH1106_COLOR_WHITE);
}

static void face_neutral(sh1106_t *display, uint32_t frame)
{
    static const int8_t look[] = {0, -2, -4, -2, 0, 2, 4, 2};
    int16_t offset = look[frame % (sizeof(look) / sizeof(look[0]))];
    bool blink = ((frame % 24U) >= 22U);

    if (blink) {
        draw_closed_eye(display, LEFT_EYE_X, EYE_Y);
        draw_closed_eye(display, RIGHT_EYE_X, EYE_Y);
    } else {
        draw_open_eye(display, LEFT_EYE_X, EYE_Y, EYE_RX, EYE_RY,
                      offset, 0, PUPIL_R);
        draw_open_eye(display, RIGHT_EYE_X, EYE_Y, EYE_RX, EYE_RY,
                      offset, 0, PUPIL_R);
    }

    draw_flat_mouth(display, 8);
}

static void face_happy(sh1106_t *display, uint32_t frame)
{
    int16_t bounce = (frame % 2U == 0U) ? 0 : 1;
    draw_happy_eye(display, LEFT_EYE_X, EYE_Y + bounce);
    draw_happy_eye(display, RIGHT_EYE_X, EYE_Y + bounce);
    draw_smile(display);
}

static void face_sad(sh1106_t *display, uint32_t frame)
{
    int16_t pupil_y = (frame % 4U < 2U) ? 2 : 3;
    draw_sad_brows(display);
    draw_open_eye(display, LEFT_EYE_X, EYE_Y, EYE_RX, EYE_RY,
                  0, pupil_y, PUPIL_R);
    draw_open_eye(display, RIGHT_EYE_X, EYE_Y, EYE_RX, EYE_RY,
                  0, pupil_y, PUPIL_R);
    draw_frown(display);
}

static void face_angry(sh1106_t *display, uint32_t frame)
{
    int16_t shake = (frame % 2U == 0U) ? -1 : 1;
    draw_angry_brows(display);
    draw_open_eye(display, LEFT_EYE_X + shake, EYE_Y, 14, 9,
                  2, 1, 4);
    draw_open_eye(display, RIGHT_EYE_X + shake, EYE_Y, 14, 9,
                  -2, 1, 4);
    draw_frown(display);
}

static void face_surprised(sh1106_t *display, uint32_t frame)
{
    int16_t pulse = (frame % 4U < 2U) ? 0 : 1;
    draw_open_eye(display, LEFT_EYE_X, EYE_Y, 16 + pulse, 15 + pulse,
                  0, 0, 3);
    draw_open_eye(display, RIGHT_EYE_X, EYE_Y, 16 + pulse, 15 + pulse,
                  0, 0, 3);
    draw_open_mouth(display, 64, 53, 7 + pulse, 8 + pulse);
}

static void face_sleepy(sh1106_t *display, uint32_t frame)
{
    int16_t y = EYE_Y + (int16_t)(frame % 2U);
    draw_closed_eye(display, LEFT_EYE_X, y);
    draw_closed_eye(display, RIGHT_EYE_X, y);
    draw_flat_mouth(display, 5);
    sh1106_draw_string_no_wrap(display, 105, 18, "Z", SH1106_COLOR_WHITE);
    sh1106_draw_string_no_wrap(display, 113, 10, "z", SH1106_COLOR_WHITE);
}

static void face_wink(sh1106_t *display, bool wink_left, uint32_t frame)
{
    bool open_briefly = ((frame % 10U) == 9U);

    if (wink_left && !open_briefly) {
        draw_closed_eye(display, LEFT_EYE_X, EYE_Y);
    } else {
        draw_open_eye(display, LEFT_EYE_X, EYE_Y, EYE_RX, EYE_RY,
                      2, 0, PUPIL_R);
    }

    if (!wink_left && !open_briefly) {
        draw_closed_eye(display, RIGHT_EYE_X, EYE_Y);
    } else {
        draw_open_eye(display, RIGHT_EYE_X, EYE_Y, EYE_RX, EYE_RY,
                      -2, 0, PUPIL_R);
    }

    draw_smile(display);
}

static void face_love(sh1106_t *display, uint32_t frame)
{
    int16_t pulse = (frame % 4U < 2U) ? 9 : 11;
    draw_heart(display, LEFT_EYE_X, EYE_Y, pulse);
    draw_heart(display, RIGHT_EYE_X, EYE_Y, pulse);
    draw_smile(display);
}

static void face_confused(sh1106_t *display, uint32_t frame)
{
    int16_t look = (frame % 6U < 3U) ? -3 : 3;

    draw_open_eye(display, LEFT_EYE_X, EYE_Y, 15, 13, look, 0, 4);
    draw_open_eye(display, RIGHT_EYE_X, EYE_Y + 2, 12, 9, look, 0, 3);
    sh1106_draw_line(display, 25, 16, 49, 18, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, 80, 19, 102, 14, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, 53, 53, 60, 49, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, 60, 49, 67, 54, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, 67, 54, 75, 50, SH1106_COLOR_WHITE);
}

static void face_scared(sh1106_t *display, uint32_t frame)
{
    int16_t tremble = (frame % 2U == 0U) ? -1 : 1;
    draw_open_eye(display, LEFT_EYE_X + tremble, EYE_Y, 17, 15,
                  0, 1, 2);
    draw_open_eye(display, RIGHT_EYE_X + tremble, EYE_Y, 17, 15,
                  0, 1, 2);
    draw_open_mouth(display, 64 + tremble, 53, 5, 7);
}

static void face_annoyed(sh1106_t *display, uint32_t frame)
{
    int16_t pupil_x = (frame % 8U < 4U) ? 4 : -4;

    sh1106_fill_ellipse(display, LEFT_EYE_X, EYE_Y + 2, 16, 7,
                        SH1106_COLOR_WHITE);
    sh1106_fill_ellipse(display, RIGHT_EYE_X, EYE_Y + 2, 16, 7,
                        SH1106_COLOR_WHITE);
    sh1106_fill_circle(display, LEFT_EYE_X + pupil_x, EYE_Y + 3, 3,
                       SH1106_COLOR_BLACK);
    sh1106_fill_circle(display, RIGHT_EYE_X + pupil_x, EYE_Y + 3, 3,
                       SH1106_COLOR_BLACK);
    sh1106_draw_line(display, 23, 18, 53, 20, SH1106_COLOR_WHITE);
    sh1106_draw_line(display, 75, 20, 105, 18, SH1106_COLOR_WHITE);
    draw_flat_mouth(display, 12);
}

static void face_excited(sh1106_t *display, uint32_t frame)
{
    int16_t bounce = (frame % 2U == 0U) ? -1 : 1;
    draw_star_eye(display, LEFT_EYE_X, EYE_Y + bounce);
    draw_star_eye(display, RIGHT_EYE_X, EYE_Y + bounce);
    draw_open_mouth(display, 64, 53, 11, 7);
}

static void face_crying(sh1106_t *display, uint32_t frame)
{
    int16_t tear_a = (int16_t)(frame % 9U);
    int16_t tear_b = (int16_t)((frame + 4U) % 9U);

    draw_sad_brows(display);
    draw_open_eye(display, LEFT_EYE_X, EYE_Y, 14, 11, 0, 2, 4);
    draw_open_eye(display, RIGHT_EYE_X, EYE_Y, 14, 11, 0, 2, 4);
    draw_tear(display, LEFT_EYE_X - 8, 39 + tear_a);
    draw_tear(display, RIGHT_EYE_X + 8, 39 + tear_b);
    draw_frown(display);
}

static void face_dizzy(sh1106_t *display, uint32_t frame)
{
    int16_t shift = (frame % 2U == 0U) ? -1 : 1;
    draw_x_eye(display, LEFT_EYE_X, EYE_Y);
    draw_x_eye(display, RIGHT_EYE_X, EYE_Y);
    sh1106_draw_line(display, 50, 53 + shift, 57, 49 - shift,
                     SH1106_COLOR_WHITE);
    sh1106_draw_line(display, 57, 49 - shift, 64, 54 + shift,
                     SH1106_COLOR_WHITE);
    sh1106_draw_line(display, 64, 54 + shift, 71, 49 - shift,
                     SH1106_COLOR_WHITE);
    sh1106_draw_line(display, 71, 49 - shift, 78, 53 + shift,
                     SH1106_COLOR_WHITE);
}

const char *mochi_face_name(mochi_face_expression_t expression)
{
    if ((uint32_t)expression >= (uint32_t)MOCHI_FACE_COUNT) {
        return "UNKNOWN";
    }

    return s_face_names[expression];
}

void mochi_face_draw(sh1106_t *display,
                     mochi_face_expression_t expression,
                     uint32_t frame)
{
    if (display == NULL) {
        return;
    }

    sh1106_clear(display, SH1106_COLOR_BLACK);
    //draw_label(display, mochi_face_name(expression));

    switch (expression) {
        case MOCHI_FACE_NEUTRAL:
            face_neutral(display, frame);
            break;
        case MOCHI_FACE_HAPPY:
            face_happy(display, frame);
            break;
        case MOCHI_FACE_SAD:
            face_sad(display, frame);
            break;
        case MOCHI_FACE_ANGRY:
            face_angry(display, frame);
            break;
        case MOCHI_FACE_SURPRISED:
            face_surprised(display, frame);
            break;
        case MOCHI_FACE_SLEEPY:
            face_sleepy(display, frame);
            break;
        case MOCHI_FACE_WINK_LEFT:
            face_wink(display, true, frame);
            break;
        case MOCHI_FACE_WINK_RIGHT:
            face_wink(display, false, frame);
            break;
        case MOCHI_FACE_LOVE:
            face_love(display, frame);
            break;
        case MOCHI_FACE_CONFUSED:
            face_confused(display, frame);
            break;
        case MOCHI_FACE_SCARED:
            face_scared(display, frame);
            break;
        case MOCHI_FACE_ANNOYED:
            face_annoyed(display, frame);
            break;
        case MOCHI_FACE_EXCITED:
            face_excited(display, frame);
            break;
        case MOCHI_FACE_CRYING:
            face_crying(display, frame);
            break;
        case MOCHI_FACE_DIZZY:
            face_dizzy(display, frame);
            break;
        default:
            face_neutral(display, frame);
            break;
    }
}

esp_err_t mochi_face_show(sh1106_t *display,
                          mochi_face_expression_t expression,
                          uint32_t frame)
{
    ESP_RETURN_ON_FALSE(display != NULL,
                        ESP_ERR_INVALID_ARG,
                        "mochi",
                        "Display pointer is NULL");

    mochi_face_draw(display, expression, frame);
    return sh1106_display(display);
}

esp_err_t mochi_face_demo_all(sh1106_t *display,
                              uint32_t hold_time_ms,
                              uint32_t repeat_count)
{
    ESP_RETURN_ON_FALSE(display != NULL,
                        ESP_ERR_INVALID_ARG,
                        "mochi",
                        "Display pointer is NULL");
    ESP_RETURN_ON_FALSE(hold_time_ms >= 100,
                        ESP_ERR_INVALID_ARG,
                        "mochi",
                        "Hold time must be at least 100 ms");

    const uint32_t frame_period_ms = 80;
    uint32_t frame_count = hold_time_ms / frame_period_ms;

    if (frame_count == 0) {
        frame_count = 1;
    }

    for (uint32_t repeat = 0; repeat < repeat_count; ++repeat) {
        for (mochi_face_expression_t expression = MOCHI_FACE_NEUTRAL;
             expression < MOCHI_FACE_COUNT;
             expression++) {
            for (uint32_t frame = 0; frame < frame_count; ++frame) {
                ESP_RETURN_ON_ERROR(
                    mochi_face_show(display, expression, frame),
                    "mochi",
                    "Failed to display expression frame");

                vTaskDelay(pdMS_TO_TICKS(frame_period_ms));
            }

            sh1106_clear(display, SH1106_COLOR_BLACK);
            ESP_RETURN_ON_ERROR(sh1106_display(display),
                                "mochi",
                                "Failed to clear the OLED");
            vTaskDelay(pdMS_TO_TICKS(120));
        }
    }

    return ESP_OK;
}
