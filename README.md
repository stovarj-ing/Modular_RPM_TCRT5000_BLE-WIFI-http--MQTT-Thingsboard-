# Entrega3 - Instrumentacion Electronica ESP32-C6

Proyecto modular en ESP-IDF v5.x para medir RPM de un motor DC con un sensor TCRT5000 y controlar un LED usando tres fases de comunicacion.

## Estructura

- `fase_A_bluetooth`: base para BLE GATT Server.
- `fase_B_wifi`: WiFi station + servidor HTTP con lectura de RPM y control de LED.
- `fase_C_mqtt`: base para WiFi + MQTT.

Cada fase es un proyecto ESP-IDF independiente y contiene su propia copia del componente `rpm_sensor`.

## Fase B

Edita estas macros en `fase_B_wifi/main/main.c`:

```c
#define WIFI_SSID     "TU_SSID"
#define WIFI_PASSWORD "TU_PASSWORD"
#define RPM_SENSOR_GPIO GPIO_NUM_4
#define LED_GPIO        GPIO_NUM_8
```

Compila desde la carpeta de la fase:

```powershell
cd fase_B_wifi
idf.py set-target esp32c6
idf.py build
idf.py flash monitor
```

Cuando el ESP32-C6 obtenga IP, abre `http://<IP_DEL_ESP32>/`.
