# Proyecto: Instrumentación Electrónica ESP32-C6

## Objetivo

Implementar tres fases de conectividad usando ESP32-C6 y ESP-IDF:

1. Bluetooth BLE
2. WiFi con servidor web
3. MQTT

## Hardware

- ESP32-C6
- Sensor TCRT5000
- Motor DC 9V
- LED indicador

## IDE

- VS Code
- ESP-IDF framework

## Arquitectura

Cada fase es un proyecto independiente:

- fase_A_bluetooth
- fase_B_wifi
- fase_C_mqtt

Todos reutilizan un componente compartido:

- rpm_sensor/

## Función del sensor

El TCRT5000 mide pulsos del motor para calcular RPM.

El cálculo usa:
- interrupciones GPIO
- temporización con FreeRTOS
- conteo de pulsos

## Requisitos fase A

- BLE GATT Server
- enviar RPM al celular
- recibir comando LED ON/OFF

## Requisitos fase B

- conexión WiFi
- servidor web HTTP
- mostrar RPM
- controlar LED desde navegador

## Requisitos fase C

- conexión MQTT
- publicar RPM
- suscribirse a tópico de control LED

## Restricciones

- usar ESP-IDF
- código modular
- usar FreeRTOS
- evitar código Arduino