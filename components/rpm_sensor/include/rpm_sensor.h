#ifndef RPM_SENSOR_H
#define RPM_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RPM_SENSOR_DEFAULT_SAMPLE_PERIOD_MS 1000U
#define RPM_SENSOR_DEFAULT_DEBOUNCE_US     1000U
#define RPM_SENSOR_DEFAULT_TASK_STACK_SIZE  2048U
#define RPM_SENSOR_DEFAULT_TASK_PRIORITY    5U

typedef struct {
    gpio_num_t gpio_num;
    uint32_t pulses_per_revolution;
    uint32_t sample_period_ms;
    uint32_t debounce_us;
    gpio_int_type_t interrupt_type;
    bool enable_pullup;
    bool enable_pulldown;
    uint32_t task_stack_size;
    UBaseType_t task_priority;
} rpm_sensor_config_t;

esp_err_t rpm_sensor_init(const rpm_sensor_config_t *config);
esp_err_t rpm_sensor_start(void);
esp_err_t rpm_sensor_stop(void);
esp_err_t rpm_sensor_reset(void);
esp_err_t rpm_sensor_deinit(void);

float rpm_sensor_get_rpm(void);
uint64_t rpm_sensor_get_total_pulses(void);
bool rpm_sensor_is_running(void);

#ifdef __cplusplus
}
#endif

#endif
