# Imagen

## Camaras/Prueba_Camara — XIAO ESP32S3 Sense

Cámara + micrófono del rover, y servidor de la UI de la estación terrena.

- Modo **AP**: red `ROVER-OBC`, clave `12345678`, IP `192.168.4.1`.
- `:80 /` → UI del OBC (ver `OBC/README.md`).
- `:81 /stream` → video MJPEG VGA 640×480.
- `:82 /audio` → audio PCM 16 bit, 16 kHz, mono.

La UI se edita en **`OBC/UI/index.html`**. Al compilar, `copiar_ui.py` la copia a `data/index.html`
y se embebe en el firmware (`board_build.embed_txtfiles`). No edites `data/index.html` a mano.

```bash
cd Imagen/Camaras/Prueba_Camara
pio run -t upload
pio device monitor        # muestra "AP listo. IP: 192.168.4.1"
```

La telemetría y el LiDAR **no** pasan por esta placa: llegan al navegador desde el puente que corre en
la PC (`OBC/puente/puente.py`).
