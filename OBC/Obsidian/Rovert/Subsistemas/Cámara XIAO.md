# Cámara XIAO

- **Dónde:** `Imagen/Camaras/Prueba_Camara` (PlatformIO, `seeed_xiao_esp32s3`).
- **AP:** `ROVER-OBC` / `12345678` → `192.168.4.1`.
- **Servidores:** `:80 /` [[UI OBC]] · `:81 /stream` MJPEG VGA · `:82 /audio` PCM 16 kHz.
- La UI se embebe desde `data/index.html`; `copiar_ui.py` la copia de `OBC/UI/index.html` antes de cada compilación.
- Cámara en PSRAM, JPEG calidad 12, 2 framebuffers. Micrófono PDM en GPIO 41/42.
