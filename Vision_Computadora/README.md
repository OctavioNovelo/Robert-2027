# Visión por computadora

Pruebas en Python con OpenCV. Por ahora usan una cámara web de la PC (`cv.VideoCapture(0)`), no la XIAO.

```bash
pip install opencv-python numpy
```

| Archivo | Qué hace |
|---|---|
| `Pruebas/Pruebas_de_camara.py` | Abre la cámara y muestra el video. `q` para salir. |
| `Pruebas/Calibracion.py` | Barras deslizantes para ajustar los rangos HSV (H, S, V mín/máx) y aislar un color. |
