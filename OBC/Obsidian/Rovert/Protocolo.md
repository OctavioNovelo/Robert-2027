# Protocolo

## Paquete LoRa (STM32 → receptor)
`TelemetryPacketLoRa`, 10 bytes, empaquetado, little-endian:
`magic u8 (0xCA) · pkt u8 · temperatura i16 °C · altitud i16 m · presion u16 hPa · verificacion u8 · checksum u8 (XOR de los 9 anteriores)`

Radio: **434.0 MHz · BW 125 kHz · SF7 · CR 4/5 · sync 0x34 · preámbulo 8 · CRC on**.
Definido en `Telemetria/Transmisor_STM32/.../Core/Inc/telemetry.h` y decodificado en `Telemetria/Receptor_ESP32/lib/paquete/paquete.h`.

## Líneas serie (115200)
| Origen | Línea |
|---|---|
| [[Receptor ESP32]] | `T,seq,temp,alt,pres,verif,rssi,snr` · `X,len` · `X,magic` · `X,cksum` · `X,radio` · `H` (latido 1 s) · `E,radio_init` |
| [[LiDAR VL53L8CX]] | `F,d0:s0,...,d63:s63` · `E,init_failed` |

## JSON del [[Puente]] a la [[UI OBC]]
```json
{"t":"tlm","seq":42,"temp":21,"alt":120,"pres":1013,"verif":1,"rssi":-87,"snr":9.5,"ts":...}
{"t":"bad","why":"cksum","ts":...}
{"t":"lidar","d":[64 mm],"s":[64 status],"ts":...}
{"t":"err","src":"rx o lidar","msg":"...","ts":...}
{"t":"link","rx":true,"lidar":false}
```
El latido `H` no se reenvía; solo sirve para autodetectar el puerto.
Parser: `OBC/puente/protocolo.py` (`parse_line`, `seq_perdidos`).
