idf_component_register(
    SRCS "rpm_sensor.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_gpio
    PRIV_REQUIRES esp_timer freertos
)
