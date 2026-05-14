# 📋 GUÍA DE DIAGNÓSTICO MQTT - Thingsboard + ESP32 + Celular AP

## 🔴 PROBLEMA IDENTIFICADO
El celular es un AP independiente. El ESP32 conectado al celular **NO PUEDE ALCANZAR** a tu PC en `10.124.164.156` porque están en redes diferentes.

```
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│  CELULAR (AP "Seb")    PC (10.124.164.156)                 │
│       │                      │                              │
│   [ESP32] ─────X─────── [Thingsboard:1883]                 │
│                                                             │
│  ❌ ESP32 NO ALCANZA a PC desde red del celular            │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## ✅ SOLUCIONES

### Opción 1: RECOMENDADA - Conectar Celular + PC a MISMA WiFi
**Lo ideal es que AMBOS estén en la misma red**

1. Conecta tu PC a una red WiFi común (tu router, no AP del celular)
2. Conecta el celular a la MISMA red WiFi
3. Encuentra la IP local de la PC: `ipconfig` (dentro de esa red)
4. Actualiza en el ESP32: `#define MQTT_BROKER_URI "mqtt://<IP_PC>:1883"`
5. Carga el firmware en ESP32 con la nueva IP

---

### Opción 2: Usar el celular como puente (intermedio)
Si NECESITAS que sea AP del celular:

1. En el celular, configura un "puente" de tethering para que la PC y ESP32 estén en la misma subnet
2. Verifica que ambos reciben IP del mismo rango (ej: 192.168.x.x)
3. Usa `ipconfig` en PC para ver la IP que recibió del celular

---

### Opción 3: Exponer Thingsboard hacia internet (avanzado)
- Usar ngrok o similar para tunelizar Thingsboard
- Configurar SSL/TLS
- ⚠️ Solo para desarrollo, riesgoso para producción

---

## 🧪 PRUEBAS E2E PASO A PASO

### Paso 1: Verificar Topología de Red
```bash
# En la PC, verifica todas las IPs
ipconfig /all

# Busca interfaces que muestren:
# - IP de red: 192.168.x.x o 10.x.x.x
# - Gateway: debería apuntar al celular
```

### Paso 2: Prueba MQTT desde PC
```bash
# En PowerShell, ejecuta la prueba MQTT
python mqtt_test.py

# Debe mostrar:
# ✅ Conectado al broker MQTT
# ✉️ Mensajes enviados
# 📨 Confirmación de Thingsboard
```

### Paso 3: Verificar Conectividad ESP32 → PC

Desde el monitor serial del ESP32, deberías ver:
```
wifi_manager: WiFi connecting...
wifi_manager: WiFi connected!
wifi_manager: IP: 192.168.x.x (o la que asigne el celular)

MQTT: Conectando a 10.124.164.156:1883...
❌ FALLO - No puede alcanzar esa IP

O

MQTT: CONECTADO ✅
MQTT MESSAGE PUBLISHED
```

Si dice "No puede alcanzar", el problema es la topología de red.

---

## 🔧 RESOLUCIÓN RECOMENDADA

### Mejor opción - Celular + PC en misma red
1. **PC**: Conecta a tu WiFi del router (NO hotspot personal)
2. **Celular**: Conecta a la MISMA red WiFi
3. **Encuentra IP de PC en esa red**:
   ```bash
   ipconfig | findstr "IPv4" # busca IP 192.168.x.x
   ```
4. **Actualiza ESP32**:
   ```c
   // En main/main.c
   #define MQTT_BROKER_URI "mqtt://<IP_NUEVA>:1883"
   ```
5. **Recarga firmware** y verifica logs

### Si NO tienes otro WiFi:
1. **Conecta Celular a PC por USB**
2. **Activa USB Tethering** en celular
3. **PC recibirá IP local**
4. **Comparte esa conexión** del celular a través del AP del celular
5. O **Activa Hotspot WiFi** del celular Y **cambia ESP32 a esa IP**

---

## 📊 CHECKLIST FINAL

- [ ] Celular y PC en misma red
- [ ] `ping` desde PC al celular: ✅
- [ ] Thingsboard corriendo: `docker ps` ✅
- [ ] Puerto 1883 abierto: ✅ (ya verificado)
- [ ] ESP32 conectado al WiFi del celular: Verificar en logs
- [ ] ESP32 puede hacer ping a IP de PC desde celular
- [ ] Prueba MQTT desde PC: `python mqtt_test.py`
- [ ] Thingsboard recibe mensajes
- [ ] ESP32 y Thingsboard conectados

---

## 📞 Próximos Pasos
1. Verifica tu topología de red (¿dónde está conectada tu PC?)
2. Ejecuta: `python mqtt_test.py` para probar desde PC
3. Envía los logs del ESP32 (espera en monitor)
4. Compartimos la misma red o hacemos ajustes
