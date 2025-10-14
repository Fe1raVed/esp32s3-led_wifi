#pragma once
#include <stdint.h>
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} rgb_color_t;

void led_controller_init(void);
void led_controller_set_color(uint8_t red, uint8_t green, uint8_t blue);
void led_controller_set_color_hsv(uint16_t hue, uint8_t saturation, uint8_t value);
void led_controller_get_current_color(rgb_color_t *color);
void led_controller_deinit(void);

#ifdef __cplusplus
}
#endif
