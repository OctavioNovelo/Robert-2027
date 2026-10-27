# Telemetría

Enlace LoRa entre el rover y la estación terrena.

```
STM32F411 + BME280 + SX1278 ──LoRa──▶ ESP32 + SX1278 ──USB──▶ PC (OBC/puente) ──▶ UI
      Transmisor_STM32                  Receptor_ESP32
```

| Carpeta | Qué es |
|---|---|
| `Transmisor_STM32/` | Computadora de vuelo (STM32CubeIDE). Lee sensores y envía el paquete LoRa. |
| `Receptor_ESP32/` | **Receptor en uso** (PlatformIO). Recibe, valida e imprime por USB para el puente. |
| `Receptor_STM32/` | Receptor anterior en STM32. Ya no se usa: no tiene salida serie y su bucle descarta los paquetes válidos. |
| `Posibles_Receptores/` | Pruebas viejas (RadioLib con modo FSK, Arduino LoRa, ESP32-CAM por SPI). Solo referencia. |
| `Esquema_Pines.jpeg` | Esquemático de la placa del **transmisor**. |
| `Guia_SX1278.pdf` | Guía del módulo de radio. |

## Paquete LoRa

`TelemetryPacketLoRa` (10 bytes, empaquetado, little-endian), definido en
`Transmisor_STM32/.../Core/Inc/telemetry.h`:

| Byte | Campo | Tipo |
|---|---|---|
| 0 | `magic` = `0xCA` | u8 |
| 1 | `pkt` (secuencia, da la vuelta en 255) | u8 |
| 2-3 | `temperatura` °C | i16 |
| 4-5 | `altitud` m | i16 |
| 6-7 | `presion` hPa | u16 |
| 8 | `verificacion` | u8 |
| 9 | `checksum` = XOR de bytes 0-8 | u8 |

Radio (transmisor y receptor deben coincidir): **434.0 MHz, BW 125 kHz, SF7, CR 4/5, sync word `0x34`, preámbulo 8, CRC activado**.

## Receptor_ESP32

Mismo firmware para **dos receptores**; cambian pines y LEDs (`platformio.ini`, un entorno por placa):

| Entorno | Placa | Estado |
|---|---|---|
| `xiao` (por defecto) | XIAO ESP32S3 Sense | probado en la placa real (2026-10-26) |
| `devkit` | ESP32 DevKit V1 30 pines — `Esquema_Receptor.png` | compila; falta probar en la placa |

```bash
cd Telemetria/Receptor_ESP32
pio test -e native            # pruebas de decodificación en la PC, sin placa
pio run -e xiao   -t upload   # XIAO
pio run -e devkit -t upload   # DevKit
pio device monitor            # ver las líneas (cerrar antes de correr el puente)
```

### Pines

| SX1278 | XIAO (pad / GPIO) | DevKit V1 (GPIO) |
|---|---|---|
| SCK | D8 / 7 | 18 |
| MISO | D9 / 8 | 19 |
| MOSI | D10 / 9 | 23 |
| NSS | D6 / 43 | 5 |
| RST | D5 / 6 | 14 |
| DIO0 (IO0) | D4 / 5 | 2 (por R5 = 0 Ω) |
| DIO1 | no conectado | no conectado |

Los de la XIAO se detectaron en la placa real (el SX1278 respondió a su registro de versión `0x42 = 0x12`).
Los de la DevKit salen del esquemático.

El `ReceptorLoRa.ino` viejo usaba `Module(5, 26, 14, 27)`: coincide con la DevKit en NSS y RST, pero
ponía DIO0 en el 26 (que es el LED amarillo) en vez del 2, así que nunca detectaba los paquetes.

### LEDs

| Evento | XIAO | DevKit |
|---|---|---|
| Paquete válido (destello) | LED naranja de la placa | azul (GPIO27) |
| Paquete descartado (destello) | — | amarillo (GPIO26) |
| La radio no arrancó (fijo; también imprime `E,radio_init`) | LED naranja de la placa | rojo (GPIO25) |

### Salida serie (115200)

| Línea | Significado |
|---|---|
| `T,seq,temp,alt,pres,verif,rssi,snr` | paquete válido |
| `X,len` / `X,magic` / `X,cksum` / `X,radio` | paquete descartado |
| `H` | latido cada segundo (permite que el puente lo reconozca aunque el rover esté apagado) |
| `E,radio_init` | el SX1278 no respondió al arrancar |
