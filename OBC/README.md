# OBC — Estación terrena del rover

Una sola UI web que junta en la misma pantalla:

- **Video y audio** de la XIAO ESP32S3 Sense (`Imagen/`).
- **Telemetría LoRa** del CanSat/rover: BME280 + BNO085 + estado (STM32 transmisor → ESP32 receptora, `Telemetria/`).
- **LiDAR 8×8** VL53L8CX (`LiDar/`).

```
 STM32 (rover) ──LoRa 434MHz──▶ ESP32 receptora ──USB──┐
                                                        ├──▶ PC: puente.py ──ws://localhost:8765──┐
 ESP32 + VL53L8CX (LiDAR) ──────────────────USB─────────┘                                        │
                                                                                                  ▼
 XIAO ESP32S3 Sense (AP "ROVER-OBC") ──Wi-Fi──▶ Navegador en la PC: http://192.168.4.1
   :80 /        → UI                                ├─ <img> ← :81/stream (MJPEG VGA)
   :81 /stream  → video                             ├─ audio ← :82/audio  (PCM 16 bit, 16 kHz)
   :82 /audio   → micrófono                         └─ datos ← ws://localhost:8765
```

| Carpeta | Qué es |
|---|---|
| `UI/index.html` | La UI (fuente única). Al compilar la cámara se copia sola a `Imagen/data/index.html`. |
| `puente/` | Script de Python que lee los dos puertos USB y manda los datos a la UI por WebSocket. |
| `Obsidian/Rovert/` | Notas del proyecto (bóveda de Obsidian). Es local: no se sube a git. |

### Por qué está armado así

- **La UI vive en la XIAO** (modo AP): basta con conectarse a su Wi-Fi y abrir `192.168.4.1`.
- **Puente en la PC en vez de Web Serial**: Web Serial solo funciona en HTTPS o `localhost`, y la página
  de la XIAO es `http://192.168.4.1`. Esa página sí puede abrir `ws://localhost`.
- **Sin recursos externos** (ni CDN ni Google Fonts): la XIAO en modo AP no tiene internet. Todo va en un
  solo `index.html` (HTML + CSS + JS, gráficas en `<canvas>`).
- **Sin modo simulación**: el hardware existe; para probar hay que conectarlo.

## Puesta en marcha

### 1. Una sola vez: preparar el puente

```bash
cd OBC/puente
python -m venv .venv
source .venv/bin/activate          # Windows: .venv\Scripts\activate
pip install -r requirements.txt
```

En Linux, si no puedes abrir los puertos: `sudo usermod -aG uucp $USER` (Arch) o `dialout` (Ubuntu) y vuelve a iniciar sesión.

### 2. Flashear las placas (solo si cambió su código)

| Placa | Carpeta | Comando |
|---|---|---|
| XIAO cámara | `Imagen/` | `pio run -t upload` |
| Receptor LoRa | `Telemetria/Receptor_ESP32` | `pio run -e xiao -t upload` o `pio run -e devkit -t upload` según la placa |
| LiDAR | `LiDar/` | `pio run -t upload` |
| Transmisor | `Telemetria/Transmisor_STM32` | ver `Telemetria/README.md` |

### 3. Cada vez que se use

1. Enciende la XIAO de la cámara.
2. Conecta por USB a la PC el **receptor** y el **LiDAR**.
3. Corre el puente (con el venv activado):
   ```bash
   python puente.py                                          # autodetecta los puertos
   python puente.py --rx /dev/ttyACM0 --lidar /dev/ttyUSB0   # si la autodetección falla
   ```
   Debe imprimir `[rx] conectado` y `[lidar] conectado`. `--port` cambia el puerto WebSocket (8765).
4. Conecta la PC al Wi-Fi **`ROVER-OBC`** (clave `12345678`).
5. Abre **http://192.168.4.1** en Chrome, Edge o Firefox.
   - Si Chrome pide permiso para "acceder a otros dispositivos/apps de tu red local", dale **Permitir**
     (la página necesita hablar con el puente en `localhost`).
   - Si aun así el indicador **WS** queda en rojo, abre **http://localhost:8765**: el puente sirve
     la misma UI y el video se sigue tomando de `192.168.4.1`.

**Nota:** el Monitor Serie de VS Code/PlatformIO y el puente no pueden tener abierto el mismo puerto a la vez.

## Usar la UI

- **CONSOLA / HUD** (o tecla **H**): retícula de control de misión, o video a pantalla completa con capas encima.
  La vista elegida se recuerda en el navegador.
- **AUDIO** (o tecla **M**): escucha el micrófono de la XIAO.
- Indicadores de la barra: **CAM** video, **RX** receptor LoRa, **LDR** LiDAR, **WS** puente.
  - verde `OK` · ámbar `LOS Ns` / `ESPERA` (conectado pero sin datos hace N s) · rojo `OFF` (desconectado).

| Panel | Muestra |
|---|---|
| `CAM-A` | Video de la XIAO. |
| `TLM-01` | Temperatura, altitud, presión y humedad (BME280) con gráfica; estado de los sensores BME280/BNO085 y **FASE** de vuelo. |
| `IMU-01` | Magnitud de aceleración y giro, y los ejes X/Y/Z del BNO085 (solo en CONSOLA). |
| `RF-LINK` | RSSI, SNR, secuencia, perdidos, tiempo encendido del transmisor y cuántas veces se reinició. |
| `LDR-8x8` | Rejilla del LiDAR (cerca = brillante; zonas inválidas rayadas en gris) y mín/máx en mm. |
| `EVENTOS` | Conexiones, pérdidas de enlace, paquetes malos, cambios de fase y fallas de sensores. |

- **FASE**: `SIN ALTÍMETRO` → `CALIBRANDO SUELO` → `EN TIERRA` → `ARMADO · EN VUELO` → `SUPERCAP ACTIVADO`,
  según los bits de `estado` del transmisor.
- **PERDIDOS** cuenta saltos en el número de secuencia más paquetes descartados.
- Si el reloj del STM32 (`t_ms`) va hacia atrás, la UI asume que el transmisor se reinició: suma uno a
  **REINICIOS TX** y no lo cuenta como pérdida.
- Pasa el mouse sobre las gráficas o la rejilla del LiDAR para ver los valores exactos.
- Parámetros opcionales en la URL: `?cam=192.168.4.1&ws=ws://localhost:8765`.

## Protocolo

Líneas serie (115200 baud):

| Origen | Línea | Significado |
|---|---|---|
| Receptor | `T,seq,temp,alt,pres,verif,rssi,snr,hum,ax,ay,az,gx,gy,gz,estado,t_ms` | paquete válido |
| Receptor | `X,motivo` | descartado: `len`, `magic`, `cksum` o `radio` (CRC LoRa) |
| Receptor | `H` | latido cada segundo (no se reenvía a la UI) |
| Receptor | `E,radio_init` | el SX1278 no responde (revisa cableado/pines) |
| LiDAR | `F,d0:s0,...,d63:s63` | frame: distancia mm : status (válido si 5 o 9) |
| LiDAR | `E,init_failed` | el VL53L8CX no inicializó |

Unidades de `T`: temp °C, alt m (sobre el punto de arranque), pres hPa, hum %RH, accel m/s², gyro rad/s,
`verif` = 1 si BME280 y BNO085 están bien, `t_ms` = ms desde que arrancó el STM32.
`estado` son bits: 0 `bme_ok`, 1 `bno_ok`, 2 `ref_ok`, 3 `armado`, 4 `supercap`.
El paquete binario LoRa que da origen a esta línea está en `Telemetria/README.md`.

Mensajes JSON del puente a la UI (ver `puente/protocolo.py`):

```json
{"t":"tlm","seq":42,"temp":21.5,"alt":120.3,"pres":1013.2,"hum":40.1,"verif":1,"rssi":-87,"snr":9.5,
 "accel":[0.1,0.0,9.8],"gyro":[0.0,0.0,0.0],"t_ms":123456,
 "estado":{"bme_ok":true,"bno_ok":true,"ref_ok":true,"armado":false,"supercap":false},"ts":1730000000.1}
{"t":"bad","why":"cksum","ts":...}
{"t":"lidar","d":[64 enteros mm],"s":[64 status],"ts":...}
{"t":"err","src":"rx|lidar","msg":"radio_init|init_failed","ts":...}
{"t":"link","rx":true,"lidar":false}
```

Sin argumentos, el puente autodetecta abriendo cada puerto USB y clasificándolo por la primera línea
reconocible (`T,`/`X,`/`H` → receptor, `F,` → LiDAR). Un puerto caído se reintenta cada 2 s y cada cambio
emite `link`.

## Pruebas

```bash
cd OBC/puente && pytest -v
```
