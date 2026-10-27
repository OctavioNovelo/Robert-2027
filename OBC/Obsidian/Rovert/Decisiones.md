# Decisiones

**2026-10-25 — UI del OBC**

- **La UI vive en la [[Cámara XIAO]]** (modo AP): el usuario se conecta a su Wi-Fi y abre `192.168.4.1`.
- **Puente en la PC (opción B)** en vez de Web Serial: Web Serial solo funciona en HTTPS o `localhost`, y la página de la XIAO es `http://192.168.4.1`. Una página así sí puede abrir `ws://localhost`.
- **El puente también sirve la UI en `http://localhost:8765`** como respaldo por si Chrome (Local Network Access) bloquea la conexión a localhost. Probado OK en Chromium 154 headless; Chrome normal puede pedir permiso.
- **Sin modo simulación**: el hardware existe.
- **Receptor nuevo en ESP32** (no el `Receptor_STM32`): el STM32 no tenía salida serie y su bucle descartaba los paquetes válidos.
- **Recepción binaria**: el `ReceptorLoRa.ino` viejo recibía un `String`, y el paquete contiene bytes `0x00`.
- **Sin FSK**: el transmisor no envía FSK; el modo FSK del receptor viejo hacía perder telemetría.
- **Hay dos receptores con el mismo firmware**: la XIAO ESP32S3 Sense (identificada con esptool el 2026-10-26; pines detectados con un programa que buscó el SX1278 en los pads D0-D10, tras respaldar su flash) y la PCB con ESP32 DevKit V1 de `Esquema_Receptor.png`. Un entorno de PlatformIO por placa; los LEDs se configuran por `build_flags`.
- **Latido `H`** cada segundo en el receptor para que el puente lo reconozca aunque el rover esté apagado.
- **Estética**: control de misión (fondo `#0a0d10`, ámbar `#ffb000`, verde `#3dff8a`, rojo solo alarmas, monoespaciada) con dos vistas, CONSOLA y HUD.
- **Sin recursos externos en la UI**: la XIAO en AP no tiene internet.

Spec: `docs/superpowers/specs/2026-10-25-obc-ui-design.md` · Plan: `docs/superpowers/plans/2026-10-25-obc-ui.md`
