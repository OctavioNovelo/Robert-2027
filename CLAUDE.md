# Robert 2027 — rover lunar

Antes de cambiar algo, lee la bóveda de Obsidian `OBC/Obsidian/Rovert/` (local, ignorada por git; si no existe, sigue con los README): empieza por `Rovert 2027.md`
(índice), luego `Protocolo.md`, `Decisiones.md`, `Pendientes.md` y la nota del subsistema en `Subsistemas/`.
Cuando cambies algo que esas notas describan, actualízalas en el mismo cambio, junto con el README de la carpeta.

## Reglas que se rompen fácil

- El paquete LoRa está definido dos veces y debe coincidir: `Telemetria/Transmisor_STM32/Core/Inc/telemetry.h`
  y `Telemetria/Receptor_ESP32/lib/paquete/paquete.h`. Un cambio de campo toca la cadena completa
  (ver "Cambiar un campo" en `Protocolo.md`).
- La UI se edita solo en `OBC/UI/index.html`. `Imagen/data/index.html` es una copia generada por `Imagen/copiar_ui.py`.
- La UI no puede usar CDN ni fuentes externas (la XIAO en modo AP no tiene internet).
- Sin modo simulación: el hardware existe.
- Todo el texto, comentarios y nombres van en español.

## Pruebas

```bash
cd OBC/puente && pytest -v                       # parser del puente
cd Telemetria/Receptor_ESP32 && pio test -e native   # decodificación del paquete
```
