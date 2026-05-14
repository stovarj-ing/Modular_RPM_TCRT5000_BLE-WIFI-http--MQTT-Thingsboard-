#include "led_control.h"

#include "freertos/FreeRTOS.h"

typedef struct {
    led_control_config_t config;
    portMUX_TYPE mux;
    bool initialized;
    bool on;
} led_control_state_t;

static led_control_state_t s_led = {
    .mux = portMUX_INITIALIZER_UNLOCKED,
};

esp_err_t led_control_init(const led_control_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    s_led.config = *config;

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << s_led.config.gpio_num,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        return err;
    }

    s_led.initialized = true;
    return led_control_set(false);
}

esp_err_t led_control_set(bool on)
{
    if (!s_led.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    const uint32_t level = on == s_led.config.active_high ? 1U : 0U;
    esp_err_t err = gpio_set_level(s_led.config.gpio_num, level);
    if (err != ESP_OK) {
        return err;
    }

    portENTER_CRITICAL(&s_led.mux);
    s_led.on = on;
    portEXIT_CRITICAL(&s_led.mux);

    return ESP_OK;
}

bool led_control_get(void)
{
    portENTER_CRITICAL(&s_led.mux);
    const bool on = s_led.on;
    portEXIT_CRITICAL(&s_led.mux);

    return on;
}
