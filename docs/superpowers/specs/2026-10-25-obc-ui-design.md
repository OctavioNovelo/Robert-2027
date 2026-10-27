# Diseño: UI unificada del OBC (cámara + telemetría + LiDAR)

Fecha: 2026-10-25

## Objetivo

Una sola UI web, servida por la XIAO ESP32S3 Sense de Imagen en modo AP, que muestre
en la misma pantalla:

- el video MJPEG (y audio) de la propia XIAO,
- la telemetría LoRa del rover (STM32 transmisor → ESP32 receptora conectada por USB a la PC),
- la rejilla 8×8 del LiDAR VL53L8CX (ESP32 conectada por USB a la PC).

El usuario conecta la PC al Wi-Fi de la XIAO, abre `http://192.168.4.1` y ve todo en vivo.

**Éxito:** con el hardware real conectado, la UI muestra video, telemetría y LiDAR
actualizándose, e indica claramente cuando alguno de los enlaces se pierde.

## Restricciones

- El hardware ya existe y está cableado. No hay modo de simulación en el código entregado.
- La XIAO en modo AP no tiene internet: la UI no puede depender de CDNs ni de Google Fonts.
- Web Serial no funciona en páginas servidas por `http://192.168.4.1` (no es contexto seguro),
  por eso los puertos serie los lee un puente en la PC (opción B).
- Se conservan el stream de video (`:81/stream`) y el de audio (`:82/audio`) existentes.

## Arquitectura

```
 STM32 (rover) ──LoRa 434MHz──▶ ESP32 receptora ──USB serial──┐
                                                               ├──▶ PC: puente.py ──ws://localhost:8765──┐
 ESP32 + VL53L8CX (LiDAR) ──────────USB serial─────────────────┘                                        │
                                                                                                         ▼
 XIAO ESP32S3 Sense (modo AP)  ──Wi-Fi──▶  Navegador en la PC (http://192.168.4.1)
   :80 /        → UI                          ├─ <img> ← :81/stream
   :81 /stream  → MJPEG                       ├─ audio ← :82/audio
   :82 /audio   → PCM 16 bit 16 kHz           └─ WebSocket ← ws://localhost:8765
```

Una página de `http://192.168.4.1` puede abrir `ws://localhost` porque los navegadores
tratan `localhost` como origen confiable; no se necesitan flags.

### Unidades

| Unidad | Ubicación | Responsabilidad |
|---|---|---|
| Firmware cámara | `Imagen/Camaras/Prueba_Camara` (modificado) | Video y audio como hoy; sirve la UI en `/`. La UI se embebe en compilación desde `data/index.html` (`board_build.embed_txtfiles`). SSID del AP: `ROVER-OBC`. |
| Firmware receptor | `Telemetria/Receptor_ESP32/` (nuevo, PlatformIO, RadioLib) | Solo LoRa. Recibe bytes, valida y emite una línea de texto por paquete. |
| LiDAR | `LiDar/src/main.cpp` | Sin cambios. Emite `F,d0:s0,...,d63:s63` por frame y `E,init_failed` si falla. |
| Puente | `OBC/puente/puente.py` | Lee los dos puertos, parsea líneas, difunde JSON por WebSocket. |
| UI (fuente) | `OBC/UI/index.html` | Fuente única; se copia a `Imagen/Camaras/Prueba_Camara/data/index.html`. |

## Firmware receptor

Basado en `Telemetria/Posibles_Receptores/LoRa_Arduino/ReceptorLoRa/ReceptorLoRa.ino`, con
estos cambios:

- **Radio:** `radio.begin(434.0, 125.0, 7, 5, 0x34, 17, 8)`, que coincide con el transmisor STM32
  (registros de frecuencia `0x6C8000` = 434.0 MHz, `0x1D=0x72`, `0x1E=0x74`, sync `0x34`, preámbulo 8, CRC activado).
- **Recepción binaria:** `radio.readData(buf, len)` sobre un `uint8_t[]` en lugar de `String`
  (el paquete contiene bytes `0x00`).
- **Validación:** función pura `decode_packet(const uint8_t*, size_t, Telemetry*)` que exige
  longitud 10, `magic == 0xCA` y checksum XOR de los 9 bytes anteriores.
  Estructura (little-endian, empaquetada): `magic u8, pkt u8, temperatura i16, altitud i16, presion u16, verificacion u8, checksum u8`.
- **Salida serie (115200):**
  - paquete válido: `T,<seq>,<temp>,<alt>,<pres>,<verif>,<rssi>,<snr>`
  - paquete inválido: `X,<motivo>` con motivo `len`, `magic`, `cksum` o `radio` (CRC LoRa)
  - latido: `H` cada segundo (para que el puente lo autodetecte con el rover apagado; no se reenvía a la UI)
  - error de radio al iniciar: `E,radio_init` (repetido cada segundo)
- **Sin modo FSK** (el transmisor no envía FSK hoy).
- **Placas y pines** (un entorno de PlatformIO por placa):
  - `xiao`, XIAO ESP32S3 Sense (detectados en la placa real el 2026-10-26): SCK 7 (D8), MISO 8 (D9),
    MOSI 9 (D10), NSS 43 (D6), RST 6 (D5), DIO0 5 (D4). LED de la placa (GPIO21): destello = válido, fijo = radio no arrancó.
  - `devkit`, ESP32 DevKit V1 (`Esquema_Receptor.png`): SCK 18, MISO 19, MOSI 23, NSS 5, RST 14, DIO0 2.
    LEDs azul 27 = válido, amarillo 26 = descartado, rojo 25 = radio no arrancó.

## Puente (`OBC/puente/puente.py`)

- Python 3, dependencias `pyserial` y `websockets` (`requirements.txt`).
- Argumentos: `--rx PUERTO`, `--lidar PUERTO`, `--port 8765`. Sin argumentos, autodetecta abriendo
  cada puerto USB y clasificándolo por la primera línea reconocible (`T,`/`X,` → receptor, `F,` → LiDAR).
- Parser puro en `OBC/puente/protocolo.py`: `parse_line(str) -> dict | None`.
- Mensajes a la UI:
  ```json
  {"t":"tlm","seq":42,"temp":21,"alt":120,"pres":1013,"verif":1,"rssi":-87,"snr":9.5,"ts":1730000000.1}
  {"t":"bad","why":"cksum","ts":...}
  {"t":"lidar","d":[64 enteros mm],"s":[64 status],"ts":...}
  {"t":"err","src":"lidar","msg":"init_failed","ts":...}
  {"t":"link","rx":true,"lidar":false}
  ```
- También sirve la UI en `http://localhost:8765` (respaldo si el navegador bloquea la conexión
  de `192.168.4.1` a `localhost` por Local Network Access).
- Un puerto caído se reintenta cada 2 s; cada cambio emite `link`. Al conectar un cliente se le
  envía el `link` actual.

## UI

### Lenguaje visual (compartido)

- Fondo `#0a0d10` con retícula técnica tenue; ámbar `#ffb000` para datos y etiquetas;
  verde fósforo `#3dff8a` para nominal; rojo solo para alarmas.
- Tipografía monoespaciada: pila de sistema (`"JetBrains Mono","IBM Plex Mono",ui-monospace,monospace`);
  sin fuentes externas.
- Etiquetas tipo estación terrena: `CAM-A`, `TLM-01`, `RF-LINK`, `LDR-8x8`, `EVENTOS`.
- Barra superior fija: reloj de misión `T+hh:mm:ss` (desde la carga de la página), indicadores
  `CAM` `RX` `LDR` `WS` (verde/ámbar/rojo), conmutador `CONSOLA|HUD`, botón de audio.
- Atajos: `H` alterna vista, `M` alterna audio. La vista elegida se recuerda en `localStorage`.
- Todo en un solo `index.html` (HTML+CSS+JS inline), sin librerías; gráficas en `<canvas>`.

### Vista CONSOLA

```
┌─ T+00:12:34 ── CAM● RX● LDR● WS● ─────────── [CONSOLA|HUD] ┐
│ ┌─ CAM-A ───────────────────────┐ ┌─ TLM-01 ─────────────┐ │
│ │       video MJPEG 4:3         │ │ TEMP   21 °C  ▁▂▃▅▆  │ │
│ │                               │ │ ALT   120 m   ▁▁▂▃▅  │ │
│ │                               │ │ PRES 1013 hPa ▆▆▅▅▅  │ │
│ │                               │ │ VERIF  OK            │ │
│ └───────────────────────────────┘ ├─ RF-LINK ────────────┤ │
│ ┌─ LDR-8x8 ──────┐ ┌─ EVENTOS ──┐ │ RSSI -87 dBm ▂▃▃▂▃   │ │
│ │ rejilla color  │ │ log        │ │ SNR   9.5 dB         │ │
│ │ + mín/máx mm   │ │            │ │ SEQ 42  PERD 3 (2%)  │ │
│ └────────────────┘ └────────────┘ │ ÚLT 0.4 s            │ │
└────────────────────────────────────────────────────────────┘
```

- Sparklines de los últimos 60 valores para TEMP, ALT, PRES y RSSI.
- PERD = paquetes perdidos, calculado por saltos de `seq` (módulo 256) más mensajes `bad`.
- LiDAR: escala secuencial de un solo tono ámbar (cerca = brillante); zonas con status inválido
  (distinto de 5 y 9) en gris con patrón rayado. Muestra mín/máx en mm.
- EVENTOS: log de los últimos 50 eventos (conexión/desconexión, LOS, checksum fallido, errores).
- En anchos < 900 px las columnas se apilan; sin scroll horizontal.

### Vista HUD

- Video a pantalla completa (`object-fit: contain`), esquinas de visor y cruz central fina.
- Capas semitransparentes con `backdrop-filter: blur`:
  arriba-izq indicadores de enlace; arriba-der TEMP/ALT/PRES grandes;
  abajo-izq mini LiDAR 8×8; abajo-der RSSI/SNR/SEQ.

### Estados de error

- WebSocket caído: reintento cada 2 s; indicador `WS` rojo; paneles de telemetría y LiDAR
  muestran `SIN ENLACE`.
- Sin datos de una fuente por más de 3 s: su indicador pasa a ámbar y el panel muestra `LOS <n>s`;
  en HUD la capa parpadea en rojo.
- Video: `onerror` del `<img>` → indicador `CAM` rojo y reintento de la URL cada 3 s.

## Pruebas

- `OBC/puente/test_protocolo.py` (pytest): líneas `T`, `X`, `F`, `E` válidas, malformadas, cortadas
  y vacías.
- `Telemetria/Receptor_ESP32/test/` (PlatformIO `native`): `decode_packet` con paquetes armados como
  `telemetry_build_LoRa` del STM32, checksum malo, magic malo y longitud incorrecta.
- UI: capturas con Playwright de ambas vistas en escritorio y móvil, alimentando el WebSocket con un
  script temporal fuera del repo; sin errores en consola.
- Integración con hardware: la ejecuta el usuario siguiendo `OBC/README.md`.

## Fuera de alcance

- Modo FSK / transmisión de imágenes por radio.
- Cambios al firmware del STM32 transmisor o del LiDAR.
- Grabación o exportación de datos.
- El receptor STM32 de `Telemetria/Receptor_STM32` (reemplazado por el receptor ESP32).
