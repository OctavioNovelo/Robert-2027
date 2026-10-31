# Imagen — XIAO ESP32S3 Sense

Cámara + micrófono del rover, y servidor de la UI de la estación terrena (proyecto PlatformIO).

- Modo **AP**: red `ROVER-OBC`, clave `12345678`, IP `192.168.4.1`.
- `:80 /` → UI del OBC (ver `OBC/README.md`).
- `:81 /stream` → video MJPEG VGA 640×480.
- `:82 /audio` → audio PCM 16 bit, 16 kHz, mono (micrófono PDM de la placa, CLK 42 / DATA 41).

```bash
cd Imagen
pio run -t upload
pio device monitor        # muestra "AP listo. IP: 192.168.4.1"
```

| Archivo | Qué es |
|---|---|
| `src/main.cpp` | Firmware: cámara, micrófono, AP Wi-Fi y los tres servidores. |
| `copiar_ui.py` | Script que PlatformIO corre antes de compilar: copia `OBC/UI/index.html` a `data/index.html`. |
| `data/index.html` | Copia de la UI que se embebe en el firmware (`board_build.embed_txtfiles`). **No la edites a mano.** |

La UI se edita en **`OBC/UI/index.html`**; al compilar se copia sola.

La telemetría y el LiDAR **no** pasan por esta placa: llegan al navegador desde el puente que corre en
la PC (`OBC/puente/puente.py`).
