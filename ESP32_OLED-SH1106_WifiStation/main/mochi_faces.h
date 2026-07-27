#ifndef MOCHI_FACES_H
#define MOCHI_FACES_H

#include <stdint.h>
#include "esp_err.h"
#include "sh1106.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MOCHI_FACE_NEUTRAL = 0,
    MOCHI_FACE_HAPPY,
    MOCHI_FACE_SAD,
    MOCHI_FACE_ANGRY,
    MOCHI_FACE_SURPRISED,
    MOCHI_FACE_SLEEPY,
    MOCHI_FACE_WINK_LEFT,
    MOCHI_FACE_WINK_RIGHT,
    MOCHI_FACE_LOVE,
    MOCHI_FACE_CONFUSED,
    MOCHI_FACE_SCARED,
    MOCHI_FACE_ANNOYED,
    MOCHI_FACE_EXCITED,
    MOCHI_FACE_CRYING,
    MOCHI_FACE_DIZZY,
    MOCHI_FACE_COUNT
} mochi_face_expression_t;

/** Draw one expression frame into the local framebuffer. */
void mochi_face_draw(sh1106_t *display,
                     mochi_face_expression_t expression,
                     uint32_t frame);

/** Draw one expression frame and transfer it to the OLED. */
esp_err_t mochi_face_show(sh1106_t *display,
                          mochi_face_expression_t expression,
                          uint32_t frame);

/** Return the English display name of an expression. */
const char *mochi_face_name(mochi_face_expression_t expression);

/** Play all expressions with simple animation. */
esp_err_t mochi_face_demo_all(sh1106_t *display,
                              uint32_t hold_time_ms,
                              uint32_t repeat_count);

#ifdef __cplusplus
}
#endif

#endif /* MOCHI_FACES_H */
