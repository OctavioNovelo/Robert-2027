# Receptor ESP32

- **Dónde:** `Telemetria/Receptor_ESP32` (PlatformIO + RadioLib).
- **Hace:** recibe el paquete del [[Transmisor STM32]], valida magic/checksum (`lib/paquete/paquete.h`, `decode_packet`) e imprime líneas por USB para el [[Puente]]. Ver [[Protocolo]].
- **Dos placas, mismo firmware** (entornos de `platformio.ini`):
  - `xiao` — XIAO ESP32S3 Sense (`/dev/ttyACM0`). Pines detectados en la placa real: SCK D8/7 · MISO D9/8 · MOSI D10/9 · NSS D6/43 · RST D5/6 · DIO0 D4/5. LED naranja (GPIO21, activo en bajo): destello = válido, fijo = radio no arrancó. **Probado** 2026-10-26: arranca la radio, latido `H`, el [[Puente]] la autodetecta.
  - `devkit` — ESP32 DevKit V1 30 pines, `Esquema_Receptor.png`. SCK 18 · MISO 19 · MOSI 23 · NSS 5 · RST 14 · DIO0 2 (R5 0 Ω). LEDs azul 27 = válido, amarillo 26 = descartado, rojo 25 = radio no arrancó. Compila; **falta probar** en la placa.
- **Entornos:** `xiao` (por defecto), `devkit`, `native` (pruebas en la PC).
- El `ReceptorLoRa.ino` viejo tenía `Module(5, 26, 14, 27)`: DIO0 en el LED amarillo (26) en vez del 2 → nunca detectaba paquetes.
- Reemplaza a `Telemetria/Receptor_STM32` y a `Telemetria/Posibles_Receptores/LoRa_Arduino/ReceptorLoRa` (ver [[Decisiones]]).
