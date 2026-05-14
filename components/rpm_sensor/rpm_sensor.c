#include "rpm_sensor.h"

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct {
    rpm_sensor_config_t config;
    TaskHandle_t task_handle;
    portMUX_TYPE mux;
    volatile uint64_t total_pulses;
    volatile uint32_t window_pulses;
    volatile int64_t last_pulse_time_us;
    float rpm;
    bool initialized;
    bool running;
} rpm_sensor_state_t;

static rpm_sensor_state_t s_sensor = {
    .mux = portMUX_INITIALIZER_UNLOCKED,
};

static void IRAM_ATTR rpm_sensor_isr_handler(void *arg)
{
    (void)arg;

    const int64_t now_us = esp_timer_get_time();

    portENTER_CRITICAL_ISR(&s_sensor.mux);
    if ((now_us - s_sensor.last_pulse_time_us) >= s_sensor.config.debounce_us) {
        s_sensor.total_pulses++;
        s_sensor.window_pulses++;
        s_sensor.last_pulse_time_us = now_us;
    }
    portEXIT_CRITICAL_ISR(&s_sensor.mux);
}

static void rpm_sensor_task(void *arg)
{
    (void)arg;

    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t period_ticks = pdMS_TO_TICKS(s_sensor.config.sample_period_ms);

    while (true) {
        vTaskDelayUntil(&last_wake_time, period_ticks);

        portENTER_CRITICAL(&s_sensor.mux);
        const uint32_t pulses = s_sensor.window_pulses;
        s_sensor.window_pulses = 0;
        portEXIT_CRITICAL(&s_sensor.mux);

        const float revolutions = (float)pulses / (float)s_sensor.config.pulses_per_revolution;
        const float minutes = (float)s_sensor.config.sample_period_ms / 60000.0f;

        portENTER_CRITICAL(&s_sensor.mux);
        s_sensor.rpm = revolutions / minutes;
        portEXIT_CRITICAL(&s_sensor.mux);
    }
}

static rpm_sensor_config_t rpm_sensor_apply_defaults(const rpm_sensor_config_t *config)
{
    rpm_sensor_config_t normalized = *config;

    if (normalized.sample_period_ms == 0) {
        normalized.sample_period_ms = RPM_SENSOR_DEFAULT_SAMPLE_PERIOD_MS;
    }

    if (normalized.debounce_us == 0) {
        normalized.debounce_us = RPM_SENSOR_DEFAULT_DEBOUNCE_US;
    }

    if (normalized.task_stack_size == 0) {
        normalized.task_stack_size = RPM_SENSOR_DEFAULT_TASK_STACK_SIZE;
    }

    if (normalized.task_priority == 0) {
        normalized.task_priority = RPM_SENSOR_DEFAULT_TASK_PRIORITY;
    }

    if (normalized.interrupt_type == GPIO_INTR_DISABLE) {
        normalized.interrupt_type = GPIO_INTR_POSEDGE;
    }

    return normalized;
}

esp_err_t rpm_sensor_init(const rpm_sensor_config_t *config)
{
    if (config == NULL || config->pulses_per_revolution == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_sensor.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    s_sensor.config = rpm_sensor_apply_defaults(config);

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << s_sensor.config.gpio_num,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = s_sensor.config.enable_pullup ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = s_sensor.config.enable_pulldown ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = s_sensor.config.interrupt_type,
    };

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        return err;
    }

    err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    err = gpio_isr_handler_add(s_sensor.config.gpio_num, rpm_sensor_isr_handler, NULL);
    if (err != ESP_OK) {
        return err;
    }

    portENTER_CRITICAL(&s_sensor.mux);
    s_sensor.total_pulses = 0;
    s_sensor.window_pulses = 0;
    s_sensor.last_pulse_time_us = 0;
    s_sensor.rpm = 0.0f;
    s_sensor.running = false;
    s_sensor.initialized = true;
    portEXIT_CRITICAL(&s_sensor.mux);

    return ESP_OK;
}

esp_err_t rpm_sensor_start(void)
{
    if (!s_sensor.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_sensor.running) {
        return ESP_OK;
    }

    BaseType_t result = xTaskCreate(
        rpm_sensor_task,
        "rpm_sensor",
        s_sensor.config.task_stack_size,
        NULL,
        s_sensor.config.task_priority,
        &s_sensor.task_handle);

    if (result != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = gpio_intr_enable(s_sensor.config.gpio_num);
    if (err != ESP_OK) {
        vTaskDelete(s_sensor.task_handle);
        s_sensor.task_handle = NULL;
        return err;
    }

    portENTER_CRITICAL(&s_sensor.mux);
    s_sensor.running = true;
    portEXIT_CRITICAL(&s_sensor.mux);

    return ESP_OK;
}

esp_err_t rpm_sensor_stop(void)
{
    if (!s_sensor.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!s_sensor.running) {
        return ESP_OK;
    }

    esp_err_t err = gpio_intr_disable(s_sensor.config.gpio_num);
    if (err != ESP_OK) {
        return err;
    }

    if (s_sensor.task_handle != NULL) {
        vTaskDelete(s_sensor.task_handle);
        s_sensor.task_handle = NULL;
    }

    portENTER_CRITICAL(&s_sensor.mux);
    s_sensor.running = false;
    s_sensor.rpm = 0.0f;
    s_sensor.window_pulses = 0;
    portEXIT_CRITICAL(&s_sensor.mux);

    return ESP_OK;
}

esp_err_t rpm_sensor_reset(void)
{
    if (!s_sensor.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    portENTER_CRITICAL(&s_sensor.mux);
    s_sensor.total_pulses = 0;
    s_sensor.window_pulses = 0;
    s_sensor.last_pulse_time_us = 0;
    s_sensor.rpm = 0.0f;
    portEXIT_CRITICAL(&s_sensor.mux);

    return ESP_OK;
}

esp_err_t rpm_sensor_deinit(void)
{
    if (!s_sensor.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = rpm_sensor_stop();
    if (err != ESP_OK) {
        return err;
    }

    err = gpio_isr_handler_remove(s_sensor.config.gpio_num);
    if (err != ESP_OK) {
        return err;
    }

    portENTER_CRITICAL(&s_sensor.mux);
    s_sensor.initialized = false;
    s_sensor.rpm = 0.0f;
    s_sensor.total_pulses = 0;
    s_sensor.window_pulses = 0;
    portEXIT_CRITICAL(&s_sensor.mux);

    return ESP_OK;
}

float rpm_sensor_get_rpm(void)
{
    portENTER_CRITICAL(&s_sensor.mux);
    const float rpm = s_sensor.rpm;
    portEXIT_CRITICAL(&s_sensor.mux);

    return rpm;
}

uint64_t rpm_sensor_get_total_pulses(void)
{
    portENTER_CRITICAL(&s_sensor.mux);
    const uint64_t pulses = s_sensor.total_pulses;
    portEXIT_CRITICAL(&s_sensor.mux);

    return pulses;
}

bool rpm_sensor_is_running(void)
{
    portENTER_CRITICAL(&s_sensor.mux);
    const bool running = s_sensor.running;
    portEXIT_CRITICAL(&s_sensor.mux);

    return running;
}
