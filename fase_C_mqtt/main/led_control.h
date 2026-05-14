#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_err.h"

typedef struct {
    gpio_num_t gpio_num;
    bool active_high;
} led_control_config_t;

esp_err_t led_control_init(const led_control_config_t *config);
esp_err_t led_control_set(bool on);
bool led_control_get(void);

#endif
