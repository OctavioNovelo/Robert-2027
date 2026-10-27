# Transmisor STM32

- **Dónde:** `Telemetria/Transmisor_STM32/Fly_Computer_MODEL_EGGS/Fly_Computer_CANSAT_2026` (STM32CubeIDE).
- **MCU:** STM32F411CEU. Sensores: BME280 (temp/presión/altitud), BNO085 (orientación). Burn wire con IRLZ44N. Esquemático: `Telemetria/Esquema_Pines.jpeg`.
- **Radio:** SX1278 por SPI, driver propio `Core/Src/sx1278.c` (registros escritos a mano).
- **Envía:** `TelemetryPacketLoRa` con `telemetry_build_LoRa()` (`Core/Src/telemetry.c`). Ver [[Protocolo]].
- Lo recibe el [[Receptor ESP32]].
