# Telemetría

Enlace LoRa entre el rover/CanSat y la estación terrena.

```
STM32F411 + BME280 + BNO085 + SX1278 ──LoRa──▶ ESP32 + SX1278 ──USB──▶ PC (OBC/puente) ──▶ UI
          Transmisor_STM32                       Receptor_ESP32
```

| Carpeta / archivo | Qué es |
|---|---|
| `Transmisor_STM32/` | Computadora de vuelo (STM32CubeMX + CMake). Lee sensores, controla el supercapacitor y envía el paquete LoRa. Su esquemático es `Esquema_Pines.jpeg` dentro de la misma carpeta. |
| `Receptor_ESP32/` | Receptor (PlatformIO). Recibe, valida e imprime por USB para el puente. |
| `Guia_SX1278.pdf` | Guía del módulo de radio. |
| `procesado_imagen.cs` | Prueba en C# (ImageSharp) para armar un anaglifo con dos imágenes. No se usa en el enlace. |

## Paquete LoRa

`TelemetryPacketLoRa`: **32 bytes**, empaquetado (sin padding), little-endian. Definido igual en
`Transmisor_STM32/Core/Inc/telemetry.h` y `Receptor_ESP32/lib/paquete/paquete.h`
(si cambias uno, cambia el otro). El transmisor envía uno cada **500 ms**.

| Bytes | Campo | Tipo | Unidad |
|---|---|---|---|
| 0 | `magic` = `0xCA` | u8 | |
| 1 | `pkt` (secuencia, da la vuelta en 255) | u8 | |
| 2-5 | `t_ms` | u32 | ms desde que arrancó el STM32 |
| 6-7 | `temperatura` | i16 | 0.01 °C |
| 8-11 | `presion` | u32 | Pa |
| 12-13 | `humedad` | u16 | 0.01 %RH |
| 14-17 | `altitud` | i32 | cm sobre el punto donde arrancó la placa |
| 18-23 | `accel[3]` (x, y, z) | i16 ×3 | 0.01 m/s² |
| 24-29 | `gyro[3]` (x, y, z) | i16 ×3 | 0.001 rad/s |
| 30 | `estado` | u8 | bits, ver abajo |
| 31 | `checksum` = XOR de bytes 0-30 | u8 | |

Bits de `estado`:

| Bit | Nombre | Significado |
|---|---|---|
| 0 | `EST_BME_OK` | la última lectura del BME280 fue válida |
| 1 | `EST_BNO_OK` | hay datos recientes del BNO085 |
| 2 | `EST_REF_OK` | ya se tomó la presión de referencia del suelo |
| 3 | `EST_ARMADO` | superó la altura de armado (está en vuelo) |
| 4 | `EST_SUPERCAP` | supercapacitor activado |

Radio (transmisor y receptor deben coincidir): **434.0 MHz, BW 125 kHz, SF7, CR 4/5, sync word `0x34`,
preámbulo 8, CRC activado**.

## Transmisor_STM32

BlackPill **STM32F411CEU**. Proyecto generado con STM32CubeMX (`Fly_Computer_CANSAT_2026.ioc`); compila con
CMake (extensión STM32 de VS Code) o se abre en STM32CubeIDE.

```bash
cd Telemetria/Transmisor_STM32
cmake --preset Debug && cmake --build --preset Debug
STM32_Programmer_CLI --connect port=SWD --write build/Debug/Fly_Computer_CANSAT_2026.elf --verify -rst
```

(Las mismas dos órdenes están como tareas en `.vscode/tasks.json`.)

### Pines

| Señal | Pin |
|---|---|
| SX1278 SCK / MISO / MOSI (SPI1) | PA5 / PA6 / PA7 |
| SX1278 NSS / RST / DIO0 | PA4 / PA3 / PB1 (EXTI1) |
| BME280 y BNO085 SCL / SDA (I2C1, 100 kHz) | PB6 / PB7 |
| BNO085 INT / RST | PB8 / PB9 |
| Supercapacitor (burn wire) | PB5 |
| LED_ESTADO (azul de la placa) | PC13 |
| LED_TX | PB4 |

### Supercapacitor (`Core/Inc/supercap.h`)

La altitud es barométrica, relativa al suelo donde arrancó la placa:

1. **Referencia**: descarta 5 lecturas del BME280 y promedia 20 para la presión del suelo.
2. **En tierra**: espera a subir más de **50 m** (armado).
3. **Armado**: en la bajada, se activa al llegar a **≤ 400 m**. Si el apogeo quedó por debajo de 400 m,
   se activa en cuanto baja 15 m desde el apogeo.
4. **Activado**: la salida queda encendida hasta reiniciar la placa.

Cada condición se debe cumplir 5 lecturas seguidas (5 × 50 ms). Los ajustes están al inicio de `supercap.h`.

### LEDs

| LED_ESTADO (PC13) | Significado |
|---|---|
| fijo encendido | arrancando (si dura más de ~3 s: error fatal) |
| destello corto cada 1 s | todo bien: radio, BME280 y BNO085 |
| doble destello cada 1 s | falla un sensor (ver bits de `estado`) |
| parpadeo rápido 5 Hz | el SX1278 no responde: no hay telemetría |

LED_TX (PB4) está encendido mientras un paquete está en el aire. Los dos LEDs fijos = error fatal.

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

### LEDs

| Evento | XIAO | DevKit |
|---|---|---|
| Llegan paquetes válidos (fijo; se apaga a los 1.5 s sin recibir) | LED naranja de la placa (GPIO21) | azul (GPIO27) |
| Paquete descartado (destello) | — | amarillo (GPIO26) |
| La radio no arrancó (parpadeo rápido; también imprime `E,radio_init`) | LED naranja de la placa | rojo (GPIO25) |

### Salida serie (115200)

| Línea | Significado |
|---|---|
| `T,seq,temp,alt,pres,verif,rssi,snr,hum,ax,ay,az,gx,gy,gz,estado,t_ms` | paquete válido, ya en unidades físicas (°C, m, hPa, %RH, m/s², rad/s); `verif` = 1 si BME280 y BNO085 están bien |
| `X,len` / `X,magic` / `X,cksum` / `X,radio` | paquete descartado |
| `H` | latido cada segundo (permite que el puente lo reconozca aunque el rover esté apagado) |
| `E,radio_init` | el SX1278 no respondió al arrancar (se reintenta cada segundo) |

Qué hace el puente con estas líneas: ver `OBC/README.md`.
