# UI OBC

- **Fuente:** `OBC/UI/index.html` (un solo archivo, sin librerías ni recursos externos). Servida por la [[Cámara XIAO]] y, de respaldo, por el [[Puente]].
- **Vistas:** CONSOLA (paneles CAM-A, EVENTOS, TLM-01, RF-LINK, LDR-8x8) y HUD (video completo + capas). Tecla **H** alterna, **M** audio. La vista se recuerda en `localStorage`.
- **Indicadores:** CAM, RX, LDR, WS → OK (verde) / LOS o ESPERA (ámbar) / OFF (rojo). LOS a los 3 s sin datos.
- **PERDIDOS:** saltos de secuencia (módulo 256) + paquetes `bad`.
- **LiDAR:** escala secuencial ámbar (cerca = claro), zonas inválidas en gris rayado; tooltip por zona.
- **Parámetros URL:** `?cam=IP&ws=URL`. Si se abre desde `localhost`, toma el video de `192.168.4.1`.
- Diseño revisado con capturas en 1440×900 y 390×844; ver [[Decisiones]].
