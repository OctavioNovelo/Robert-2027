# OBC — Estación terrena del rover

Aquí vive la UI que junta en una sola pantalla:

- **Video y audio** de la XIAO ESP32S3 Sense (`Imagen/Camaras/Prueba_Camara`).
- **Telemetría LoRa** del rover (STM32 transmisor → ESP32 receptora `Telemetria/Receptor_ESP32`).
- **LiDAR 8×8** VL53L8CX (`LiDar/`).

```
 STM32 (rover) ──LoRa 434MHz──▶ ESP32 receptora ──USB──┐
                                                        ├──▶ PC: puente.py ──ws://localhost:8765──┐
 ESP32 + VL53L8CX (LiDAR) ──────────────────USB─────────┘                                        │
                                                                                                  ▼
 XIAO ESP32S3 Sense (AP "ROVER-OBC") ──Wi-Fi──▶ Navegador en la PC: http://192.168.4.1
```

| Carpeta | Qué es |
|---|---|
| `UI/index.html` | La UI (fuente única). Al compilar la cámara se copia sola a `Imagen/Camaras/Prueba_Camara/data/`. |
| `puente/` | Script de Python que lee los dos puertos USB y manda los datos a la UI por WebSocket. |
| `Obsidian/Rovert/` | Notas del proyecto (abrir la carpeta como bóveda en Obsidian). |

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
| XIAO cámara | `Imagen/Camaras/Prueba_Camara` | `pio run -t upload` |
| Receptor LoRa | `Telemetria/Receptor_ESP32` | `pio run -e xiao -t upload` o `pio run -e devkit -t upload` según la placa |
| LiDAR | `LiDar` | `pio run -t upload` |

### 3. Cada vez que se use

1. Enciende la XIAO de la cámara.
2. Conecta por USB a la PC el **receptor** y el **LiDAR**.
3. Corre el puente (con el venv activado):
   ```bash
   python puente.py                                  # autodetecta los puertos
   python puente.py --rx /dev/ttyACM0 --lidar /dev/ttyUSB0   # si la autodetección falla
   ```
   Debe imprimir `[rx] conectado` y `[lidar] conectado`.
4. Conecta la PC al Wi-Fi **`ROVER-OBC`** (clave `12345678`).
5. Abre **http://192.168.4.1** en Chrome, Edge o Firefox.
   - Si Chrome pide permiso para "acceder a otros dispositivos/apps de tu red local", dale **Permitir**
     (la página necesita hablar con el puente en `localhost`).
   - Si aun así el indicador **WS** queda en rojo, abre **http://localhost:8765**: el puente sirve
     la misma UI y el video se sigue tomando de `192.168.4.1`.

**Nota:** el Monitor Serie de VS Code/PlatformIO y el puente no pueden tener abierto el mismo puerto a la vez.

## Usar la UI

- **CONSOLA / HUD** (o tecla **H**): retícula de control de misión, o video a pantalla completa con capas encima.
- **AUDIO** (o tecla **M**): escucha el micrófono de la XIAO.
- Indicadores de la barra: **CAM** video, **RX** receptor LoRa, **LDR** LiDAR, **WS** puente.
  - verde `OK` · ámbar `LOS Ns` / `ESPERA` (conectado pero sin datos hace N s) · rojo `OFF` (desconectado).
- **PERDIDOS** cuenta saltos en el número de secuencia más paquetes descartados por checksum.
- Pasa el mouse sobre las gráficas o la rejilla del LiDAR para ver los valores exactos.
- Parámetros opcionales en la URL: `?cam=192.168.4.1&ws=ws://localhost:8765`.

## Protocolo

Líneas serie (115200 baud):

| Origen | Línea | Significado |
|---|---|---|
| Receptor | `T,seq,temp,alt,pres,verif,rssi,snr` | paquete válido |
| Receptor | `X,motivo` | descartado: `len`, `magic`, `cksum` o `radio` (CRC LoRa) |
| Receptor | `H` | latido cada segundo |
| Receptor | `E,radio_init` | el SX1278 no responde (revisa cableado/pines) |
| LiDAR | `F,d0:s0,...,d63:s63` | frame: distancia mm : status (válido si 5 o 9) |
| LiDAR | `E,init_failed` | el VL53L8CX no inicializó |

Mensajes JSON del puente a la UI: `tlm`, `bad`, `lidar`, `err`, `link` (ver `puente/protocolo.py`).

## Pruebas

```bash
cd OBC/puente && pytest -v
```
