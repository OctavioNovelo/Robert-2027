# Arquitectura

```
 STM32 (rover) ──LoRa 434MHz──▶ ESP32 receptora ──USB──┐
                                                        ├──▶ PC: puente.py ──ws://localhost:8765──┐
 ESP32 + VL53L8CX (LiDAR) ──────────────────USB─────────┘                                        │
                                                                                                  ▼
 XIAO ESP32S3 Sense (AP "ROVER-OBC") ──Wi-Fi──▶ Navegador en la PC: http://192.168.4.1
```

- La [[Cámara XIAO]] sirve la [[UI OBC]] y el video. No recibe telemetría.
- [[Receptor ESP32]] y [[LiDAR VL53L8CX]] van por USB a la PC.
- El [[Puente]] los lee y los manda a la UI por WebSocket.
- La PC está conectada al AP de la XIAO; la UI junta video (de la XIAO) y datos (del puente en localhost).

Ver [[Decisiones]] para el porqué y [[Protocolo]] para los formatos.
