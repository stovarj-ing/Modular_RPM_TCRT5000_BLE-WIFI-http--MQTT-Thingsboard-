# ESP32-C6 IoT Multiconnectivity Platform

![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.x-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Status](https://img.shields.io/badge/status-Active-brightgreen)

Plataforma modular en ESP-IDF para implementar múltiples estrategias de comunicación inalámbrica con un ESP32-C6. Mide RPM de un motor DC usando un sensor óptico TCRT5000 y expone los datos mediante **Bluetooth Low Energy (BLE)**, **servidor web HTTP** y **protocolo MQTT**.

## 🎯 Características Principales

- **Fase A - Bluetooth BLE**: Servidor GATT que transmite RPM en tiempo real y recibe comandos de control LED
- **Fase B - WiFi HTTP**: Estación WiFi con servidor web responsivo para lectura de RPM y control del LED desde navegador
- **Fase C - MQTT**: Conectividad WiFi con publicación de RPM y suscripción a tópicos de control
- **Componente Reutilizable**: Módulo `rpm_sensor` compartido entre todas las fases
- **Código Modular**: Arquitectura basada en componentes con separación clara de responsabilidades
- **FreeRTOS**: Multitarea eficiente sin bloqueos
- **Bajo Consumo**: Optimizado para dispositivos IoT de bajo consumo

## 📋 Requisitos

### Hardware
- **Microcontrolador**: ESP32-C6 (o compatible)
- **Sensor de RPM**: TCRT5000 (sensor óptico reflexivo)
- **Actuador**: Motor DC 9V con rueda perforada
- **LED Indicador**: LED RGB o simple (ánodo/cátodo común)
- **Conexiones**: Protoboard, cables jumper, resistencias

### Software
- **Toolchain**: ESP-IDF v5.x o superior
- **Python**: v3.7+
- **IDE**: VS Code con extensión ESP-IDF
- **Dependencias de Compilación**: CMake 3.5+

### Herramientas Opcionales
- **Esptool**: Para flashing y monitoreo (incluido en ESP-IDF)
- **Cliente MQTT**: mosquitto_pub/sub o equivalente
- **Cliente HTTP**: curl, Postman o navegador web

## 📁 Estructura del Proyecto

```
Entrega3/
├── components/
│   └── rpm_sensor/              # Módulo compartido de medición de RPM
│       ├── include/
│       │   └── rpm_sensor.h
│       ├── CMakeLists.txt
│       └── rpm_sensor.c
│
├── fase_A_bluetooth/            # Fase 1: Conectividad BLE
│   ├── main/
│   │   ├── ble_server.c
│   │   ├── led_control.c
│   │   └── main.c
│   ├── components/              # Copia local del rpm_sensor
│   ├── CMakeLists.txt
│   └── sdkconfig
│
├── fase_B_wifi/                 # Fase 2: Servidor HTTP
│   ├── main/
│   │   ├── wifi_manager.c
│   │   ├── web_server.c
│   │   ├── led_control.c
│   │   └── main.c
│   ├── components/              # Copia local del rpm_sensor
│   ├── CMakeLists.txt
│   └── sdkconfig
│
├── fase_C_mqtt/                 # Fase 3: Protocolo MQTT
│   ├── main/
│   │   ├── wifi_manager.c
│   │   ├── mqtt_manager.c
│   │   ├── led_control.c
│   │   └── main.c
│   ├── components/              # Copia local del rpm_sensor
│   ├── CMakeLists.txt
│   └── sdkconfig
│
├── main/                        # Código base/experimental
├── build/                       # Directorio de compilación (generado)
├── CMakeLists.txt
├── context.md
└── README.md
```

## 🏗️ Arquitectura de Componentes

### Módulo RPM Sensor
Componente de bajo nivel que encapsula toda la lógica de medición de RPM.

**API Principal:**
```c
// Inicialización y control
esp_err_t rpm_sensor_init(const rpm_sensor_config_t *config);
esp_err_t rpm_sensor_start(void);
esp_err_t rpm_sensor_stop(void);
esp_err_t rpm_sensor_deinit(void);

// Lectura de datos
float rpm_sensor_get_rpm(void);                // RPM actual
uint64_t rpm_sensor_get_total_pulses(void);    // Contador total de pulsos
bool rpm_sensor_is_running(void);              // Estado del sensor
```

**Características Técnicas:**
- Interrupción en flanco ascendente (GPIO_INTR_POSEDGE)
- Debounce configurable (defecto: 1000 µs)
- Período de muestreo configurable (defecto: 1000 ms)
- Contador de pulsos con precisión de 64 bits
- Task FreeRTOS dedicada con prioridad configurable

### Fase A: Bluetooth Low Energy

**Características:**
- Servidor GATT con servicio de RPM
- Características notificables en tiempo real
- Control remoto del LED desde app BLE

**Estructura de Servicios:**
```
Service: RPM Monitor (UUID customizado)
├── Characteristic: RPM Value (read + notify)
├── Characteristic: LED Control (write)
└── Characteristic: Status (read)
```

**Componentes Locales:**
- `ble_server`: Gestión del servidor GATT
- `led_control`: Control del LED
- `rpm_sensor`: Lectura de RPM

### Fase B: WiFi + Servidor HTTP

**Características:**
- Estación WiFi con reconexión automática
- Servidor HTTP en puerto 80
- Interfaz web responsiva
- JSON API para integración

**Endpoints HTTP:**
```
GET  /              → Página web (HTML)
GET  /api/rpm       → {"rpm": 1234.5}
GET  /api/status    → {"uptime": 3600, "ssid": "WiFi", "ip": "192.168.1.x"}
POST /api/led/on    → {"status": "ok"}
POST /api/led/off   → {"status": "ok"}
```

**Componentes Locales:**
- `wifi_manager`: Gestión de conexión WiFi
- `web_server`: Servidor HTTP
- `led_control`: Control del LED
- `rpm_sensor`: Lectura de RPM

### Fase C: MQTT + IoT Cloud

**Características:**
- Conexión MQTT broker
- Publicación periódica de RPM
- Suscripción a tópicos de control

**Tópicos MQTT:**
```
Publish:
  device/esp32c6/rpm              → Valor actual de RPM
  device/esp32c6/status/uptime    → Tiempo en funcionamiento
  device/esp32c6/status/rssi      → Potencia de señal WiFi

Subscribe:
  device/esp32c6/control/led      → Comandos: "on", "off"
  device/esp32c6/config/rpm_rate  → Cambiar período de muestreo
```

**Componentes Locales:**
- `wifi_manager`: Gestión de WiFi
- `mqtt_manager`: Cliente MQTT
- `led_control`: Control del LED
- `rpm_sensor`: Lectura de RPM

## 🚀 Instalación y Setup

### 1. Requisitos Previos

```powershell
# Verificar Python 3.7+
python --version

# Verificar Git
git --version

# ESP-IDF debe estar instalado y en PATH
idf.py --version
```

### 2. Clonar el Repositorio

```powershell
git clone https://github.com/username/Entrega3.git
cd Entrega3
```

### 3. Configuración Inicial

```powershell
# Inicializar ESP-IDF en la carpeta raíz (opcional)
idf.py set-target esp32c6
```

## 🔧 Guía de Compilación por Fase

### Fase A: Bluetooth BLE

```powershell
# Navegar a la fase
cd fase_A_bluetooth

# Seleccionar target
idf.py set-target esp32c6

# Configuración interactiva (opcional)
idf.py menuconfig

# Compilar
idf.py build

# Flashear en el dispositivo
idf.py flash

# Monitorear serial
idf.py monitor

# Todo en uno
idf.py build flash monitor
```

**Configuración en `main.c`:**
```c
#define RPM_SENSOR_GPIO             GPIO_NUM_4
#define RPM_SENSOR_PULSES_PER_REV   1U
#define RPM_SENSOR_SAMPLE_PERIOD_MS 1000U
#define RPM_SENSOR_DEBOUNCE_US      1000U
#define LED_GPIO                    GPIO_NUM_8
```

### Fase B: WiFi + HTTP

```powershell
# Navegar a la fase
cd fase_B_wifi

# Seleccionar target
idf.py set-target esp32c6

# Compilar
idf.py build

# Flashear
idf.py flash monitor
```

**Configuración en `main.c`:**
```c
#define WIFI_SSID       "TU_RED_WIFI"
#define WIFI_PASSWORD   "TU_CONTRASEÑA"
#define RPM_SENSOR_GPIO GPIO_NUM_4
#define LED_GPIO        GPIO_NUM_8
```

**Uso:**
1. El ESP32-C6 se conecta a la red WiFi
2. Obtiene una dirección IP por DHCP
3. Accede a `http://<IP_DEL_ESP32>/` en el navegador
4. Lee RPM en tiempo real y controla el LED

### Fase C: MQTT + IoT

```powershell
# Navegar a la fase
cd fase_C_mqtt

# Seleccionar target
idf.py set-target esp32c6

# Compilar
idf.py build

# Flashear
idf.py flash monitor
```

**Configuración en `main.c`:**
```c
#define WIFI_SSID       "TU_RED_WIFI"
#define WIFI_PASSWORD   "TU_CONTRASEÑA"
#define MQTT_BROKER     "mqtt.broker.com"
#define MQTT_PORT       1883
#define RPM_SENSOR_GPIO GPIO_NUM_4
#define LED_GPIO        GPIO_NUM_8
```

**Monitoreo con mosquitto:**
```bash
# Suscribirse a todos los tópicos
mosquitto_sub -h <BROKER_IP> -t "device/esp32c6/#" -v

# Enviar comando
mosquitto_pub -h <BROKER_IP> -t "device/esp32c6/control/led" -m "on"
```

## ⚙️ Configuración de Hardware

### Conexiones GPIO Recomendadas

| Componente | GPIO | Notas |
|-----------|------|-------|
| Sensor TCRT5000 | GPIO_4 | Entrada con pullup |
| LED Indicador | GPIO_8 | Salida activa en alto |
| UART TX | GPIO_21 | Serial para debug |
| UART RX | GPIO_20 | Serial para debug |

### Diagrama de Conexiones

```
ESP32-C6
┌─────────────────┐
│                 │
│  GPIO_4 ────────┤─── TCRT5000 OUT
│  GPIO_8 ────────┤─── LED (+ 220Ω → GND)
│                 │
│  GND ───────────┤─── Motor GND
│                 │
└─────────────────┘
```

## 🧪 Pruebas y Validación

### Verificar Compilación
```powershell
# Compilar todas las fases
cd fase_A_bluetooth && idf.py build && cd ..
cd fase_B_wifi && idf.py build && cd ..
cd fase_C_mqtt && idf.py build && cd ..
```

### Validar Sensor RPM
```
# Monitor serial mostrará:
[rpm_sensor] Pulsos: 150, RPM: 1500.0
[rpm_sensor] Pulsos: 155, RPM: 1550.0
```

### Pruebas Fase B (HTTP)
```bash
# Desde terminal (PowerShell)
curl.exe http://<IP_ESP>/api/rpm
curl.exe -X POST http://<IP_ESP>/api/led/on
```

## 📝 Notas de Desarrollo

### Mejores Prácticas
- ✅ Cada fase es independiente y compilable
- ✅ Componentes reutilizables y testables
- ✅ Logs detallados con ESP_LOG en cada módulo
- ✅ Manejo de errores con ESP_ERROR_CHECK
- ✅ Configuración por macros para facilitar debugging

### Consideraciones de Rendimiento
- El sensor RPM usa interrupción GPIO (sin polling)
- FreeRTOS permite multitarea sin bloqueos
- Período de muestreo: 1000 ms (configurable)
- Stack size por task: 2048 bytes (ESP-IDF default)

### Debugging
```c
// Habilitar logs detallados
esp_log_level_set("*", ESP_LOG_DEBUG);
esp_log_level_set("rpm_sensor", ESP_LOG_VERBOSE);
```

## 🤝 Contribuciones

Para reportar bugs o sugerir mejoras:
1. Verificar que no exista issue similar
2. Crear issue con descripción clara
3. Incluir logs y pasos para reproducir

## 📄 Licencia

Este proyecto está bajo licencia MIT. Ver [LICENSE](LICENSE) para más detalles.

## ✅ Checklist de Compilación

Antes de hacer commit:
- [ ] Todas las fases compilan sin errores
- [ ] No hay advertencias de compilación críticas
- [ ] El código sigue el estilo C de ESP-IDF
- [ ] Se ejecutaron pruebas básicas de funcionalidad
- [ ] Credenciales WiFi/MQTT no están en el repositorio
- [ ] README está actualizado

---

**Versión**: 1.0.0  
**Última Actualización**: Mayo 2026  
**Autor**: Tu Nombre  
**Estado**: ✅ Funcional