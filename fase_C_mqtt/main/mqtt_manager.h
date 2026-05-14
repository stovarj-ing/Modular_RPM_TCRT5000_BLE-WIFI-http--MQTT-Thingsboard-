#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "esp_err.h"

typedef struct {
    const char *broker_uri;
    const char *rpm_topic;
    const char *led_topic;
    const char *token;
} mqtt_manager_config_t;

esp_err_t mqtt_manager_init(const mqtt_manager_config_t *config);

esp_err_t mqtt_manager_start(void);

#endif