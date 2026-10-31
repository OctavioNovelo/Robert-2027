# Robert 2027 — Rover lunar

Código del rover para la convocatoria `ROVER_LUNAR_convocatoria-2026-4.pdf`.
Arquitectura general: `rover_lunar_arquitectura_final.png`.

| Carpeta | Qué es | Estado |
|---|---|---|
| [`OBC/`](OBC/README.md) | Estación terrena: UI web + puente Python que junta video, telemetría y LiDAR. **Empieza aquí.** | en uso |
| [`Telemetria/`](Telemetria/README.md) | Enlace LoRa: computadora de vuelo STM32 (transmisor) y receptor ESP32. | en uso |
| [`Imagen/`](Imagen/README.md) | XIAO ESP32S3 Sense: video, audio y servidor de la UI. | en uso |
| [`LiDar/`](LiDar/README.md) | Sensor VL53L8CX 8×8 en una ESP32. | en uso |
| [`Vision_Computadora/`](Vision_Computadora/README.md) | Pruebas de visión por computadora con OpenCV. | pruebas |
| [`Brazo/`](Brazo/README.md) | Brazo del rover. | sin código aún |

```
 STM32 (rover) ──LoRa 434MHz──▶ ESP32 receptora ──USB──┐
                                                        ├──▶ PC: OBC/puente ──WebSocket──┐
 ESP32 + VL53L8CX (LiDAR) ──────────────────USB─────────┘                                │
                                                                                         ▼
 XIAO ESP32S3 Sense (Wi-Fi "ROVER-OBC") ──video/audio──▶ Navegador: http://192.168.4.1
```

## Herramientas

- [PlatformIO](https://platformio.org/) (extensión de VS Code o `pio`) para las ESP32: `Imagen/`, `LiDar/`, `Telemetria/Receptor_ESP32/`.
- STM32CubeIDE, o CMake + `arm-none-eabi-gcc` + STM32CubeProgrammer, para el transmisor STM32.
- Python 3 para `OBC/puente/` y `Vision_Computadora/`.

Las notas del proyecto (decisiones, pendientes, protocolo) están en la bóveda de Obsidian `OBC/Obsidian/Rovert/`, que es local y no se sube a git.
