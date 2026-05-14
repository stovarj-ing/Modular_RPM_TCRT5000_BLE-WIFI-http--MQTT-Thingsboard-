#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Herramienta para pruebas E2E de MQTT con Thingsboard
Simula telemetría del ESP32 y verifica conectividad
"""

import paho.mqtt.client as mqtt
import json
import time
import sys
import os
from datetime import datetime

# Configurar encoding para Windows
if sys.platform == 'win32':
    os.environ['PYTHONIOENCODING'] = 'utf-8'

# Configuración
BROKER_HOST = "10.124.164.156"
BROKER_PORT = 1883
DEVICE_TOKEN = "PQTa9DSqjEe9x4mQUJhf"
TELEMETRY_TOPIC = "v1/devices/me/telemetry"

class MQTTTester:
    def __init__(self):
        self.client = mqtt.Client()
        self.connected = False
        self.messages_received = 0
        self.messages_sent = 0
        
        # Callbacks
        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect
        self.client.on_message = self.on_message
        self.client.on_publish = self.on_publish
        
    def on_connect(self, client, userdata, flags, rc):
        """Callback cuando se conecta"""
        if rc == 0:
            print(f"[CONECTADO] Conectado al broker MQTT {BROKER_HOST}:{BROKER_PORT}")
            self.connected = True
            # Suscribirse a respuestas de Thingsboard
            client.subscribe("v1/devices/me/rpc/request/+")
            print("[SUBSCRIBE] Suscrito a: v1/devices/me/rpc/request/+")
        else:
            print(f"[ERROR] Codigo de error: {rc}")
            self.connected = False
            
    def on_disconnect(self, client, userdata, rc):
        """Callback cuando se desconecta"""
        self.connected = False
        if rc != 0:
            print(f"[DESCONECTADO] Desconexion inesperada. Codigo: {rc}")
        else:
            print("[DESCONECTADO] Desconexion normal")
            
    def on_message(self, client, userdata, msg):
        """Callback cuando llega un mensaje"""
        self.messages_received += 1
        print(f"[RX] Topic: {msg.topic}")
        print(f"     Payload: {msg.payload.decode()}")
        print(f"     Total recibidos: {self.messages_received}")
        
    def on_publish(self, client, userdata, mid):
        """Callback cuando se publica"""
        self.messages_sent += 1
        print(f"[TX] Mensaje {mid} publicado exitosamente")
        print(f"     Total enviados: {self.messages_sent}")
        
    def connect(self):
        """Conectar al broker"""
        print(f"\n[CONECTANDO] Conectando a {BROKER_HOST}:{BROKER_PORT}...")
        print(f"             Token: {DEVICE_TOKEN}\n")
        
        try:
            self.client.username_pw_connect(DEVICE_TOKEN, "")
            self.client.connect(BROKER_HOST, BROKER_PORT, keepalive=60)
            self.client.loop_start()
            time.sleep(2)  # Esperar a que se conecte
            return self.connected
        except Exception as e:
            print(f"[ERROR] Error de conexion: {e}")
            return False
            
    def publish_telemetry(self, temperature=25.0, humidity=60.0, rpm=1500):
        """Publicar telemetría simulada"""
        if not self.connected:
            print("[ERROR] No conectado. No se puede publicar.")
            return False
            
        payload = {
            "temperature": temperature,
            "humidity": humidity,
            "rpm": rpm,
            "timestamp": datetime.now().isoformat()
        }
        
        payload_str = json.dumps(payload)
        print(f"\n[PUBLISH] Publicando telemetria:")
        print(f"          Topic: {TELEMETRY_TOPIC}")
        print(f"          Payload: {payload_str}")
        
        try:
            result = self.client.publish(TELEMETRY_TOPIC, payload_str, qos=1)
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                print("[OK] Publicacion exitosa")
                return True
            else:
                print(f"[ERROR] Error en publicacion: {result.rc}")
                return False
        except Exception as e:
            print(f"[ERROR] Error: {e}")
            return False
            
    def run_test_sequence(self, iterations=5, delay=2):
        """Ejecutar secuencia de pruebas"""
        print(f"\n{'='*60}")
        print(f"[TEST] Enviando {iterations} mensajes cada {delay}s")
        print(f"{'='*60}\n")
        
        for i in range(1, iterations + 1):
            print(f"\n--- Iteracion {i}/{iterations} ---")
            temperature = 20 + (i * 2)  # Incrementar temperatura
            rpm = 1000 + (i * 100)
            
            self.publish_telemetry(
                temperature=temperature,
                humidity=50 + i,
                rpm=rpm
            )
            
            if i < iterations:
                print(f"[ESPERA] Esperando {delay}s para siguiente prueba...")
                time.sleep(delay)
        
        print(f"\n{'='*60}")
        print(f"[RESULTADO]")
        print(f"   Mensajes enviados: {self.messages_sent}")
        print(f"   Mensajes recibidos: {self.messages_received}")
        print(f"{'='*60}\n")
        
    def disconnect(self):
        """Desconectar"""
        print("\n[DISCONNECT] Desconectando...")
        self.client.loop_stop()
        self.client.disconnect()
        time.sleep(1)

def main():
    print("""
============================================================
     MQTT E2E TEST - Thingsboard + ESP32                   
     Simulador de Telemetria                               
============================================================
    """)
    
    tester = MQTTTester()
    
    # Conectar
    if not tester.connect():
        print("[ERROR] Fallo conexion al broker MQTT")
        print("\n[DIAGNOSTICO] CHECKLIST:")
        print("   1. Thingsboard esta corriendo? (docker ps)")
        print("   2. Puerto 1883 esta abierto? (netstat -an | grep 1883)")
        print("   3. IP correcta? Debe ser 10.124.164.156")
        print("   4. Firewall permite MQTT?")
        return 1
    
    # Ejecutar pruebas
    try:
        tester.run_test_sequence(iterations=5, delay=2)
    except KeyboardInterrupt:
        print("\n[INTERRUMPIDO] Prueba interrumpida por usuario")
    except Exception as e:
        print(f"\n[ERROR] Error durante prueba: {e}")
    finally:
        tester.disconnect()
        print("[OK] Desconexion completada")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
