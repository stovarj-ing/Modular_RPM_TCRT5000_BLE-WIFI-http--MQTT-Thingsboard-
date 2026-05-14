# AGENTS

## Proyecto

Usar ESP-IDF v5.x, C y arquitectura modular. No usar Arduino.

## Convenciones

- Cada fase debe poder compilarse como proyecto ESP-IDF independiente.
- La logica de hardware reutilizable va en componentes o modulos separados.
- El componente `rpm_sensor` se mantiene con la misma API en todas las fases.
- Evitar bloqueos largos dentro de handlers HTTP, eventos WiFi, callbacks BLE o callbacks MQTT.
- Mantener macros de configuracion visibles en `main.c` para facilitar pruebas de laboratorio.
