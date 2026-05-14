#include "led_control.h"
#include "driver/gpio.h"

#define LED_GPIO GPIO_NUM_8

void led_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
    };

    gpio_config(&io_conf);

    gpio_set_level(LED_GPIO, 0);
}

void led_set(bool state)
{
    gpio_set_level(LED_GPIO, state);
}