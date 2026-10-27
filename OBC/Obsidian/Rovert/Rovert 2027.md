# Rovert 2027 — Índice

Rover lunar para la convocatoria 2026-4 (`ROVER_LUNAR_convocatoria-2026-4.pdf`, arquitectura en `rover_lunar_arquitectura_final.png`).

## Mapa
- [[Arquitectura]] — cómo se conectan las placas y la PC
- [[Protocolo]] — líneas serie, paquete LoRa y mensajes JSON
- [[Decisiones]] — por qué se hizo así
- [[Pendientes]]

## Subsistemas
- [[Transmisor STM32]] — computadora de vuelo, sensores, LoRa TX
- [[Receptor ESP32]] — XIAO ESP32S3 o DevKit V1, LoRa RX → USB
- [[Cámara XIAO]] — video, audio, sirve la UI, AP Wi-Fi
- [[LiDAR VL53L8CX]] — rejilla 8×8 → USB
- [[Puente]] — PC: serie → WebSocket
- [[UI OBC]] — estación terrena en el navegador

## Carpetas del repo
| Carpeta | Contenido |
|---|---|
| `Telemetria/` | [[Transmisor STM32]], [[Receptor ESP32]], receptores viejos |
| `Imagen/` | [[Cámara XIAO]] |
| `LiDar/` | [[LiDAR VL53L8CX]] |
| `OBC/` | [[Puente]], [[UI OBC]], esta bóveda |
| `Vision_Computadora/` | pruebas de cámara y calibración en Python |
| `Brazo/` | brazo del rover |
| `docs/superpowers/` | spec y plan de la UI del OBC (2026-10-25) |
