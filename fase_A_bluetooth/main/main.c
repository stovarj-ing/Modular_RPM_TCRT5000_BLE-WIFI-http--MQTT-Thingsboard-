#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "ble_server.h"
#include "led_control.h"
#include "rpm_sensor.h"

void app_main(void)
{
    led_init();

    rpm_sensor_config_t config = {
        .gpio_num = GPIO_NUM_4,  // Example GPIO, adjust as needed
        .pulses_per_revolution = 1,
        .sample_period_ms = RPM_SENSOR_DEFAULT_SAMPLE_PERIOD_MS,
        .debounce_us = RPM_SENSOR_DEFAULT_DEBOUNCE_US,
        .interrupt_type = GPIO_INTR_POSEDGE,
        .enable_pullup = true,
        .enable_pulldown = false,
        .task_stack_size = RPM_SENSOR_DEFAULT_TASK_STACK_SIZE,
        .task_priority = RPM_SENSOR_DEFAULT_TASK_PRIORITY,
    };

    rpm_sensor_init(&config);
    rpm_sensor_start();

    ble_init();

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}