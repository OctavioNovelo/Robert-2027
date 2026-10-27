# Puente

- **Dónde:** `OBC/puente/` — `puente.py` (programa), `protocolo.py` (parser puro), `test_protocolo.py` (pytest).
- **Hace:** abre el puerto del [[Receptor ESP32]] y del [[LiDAR VL53L8CX]], traduce cada línea a JSON y la difunde por `ws://localhost:8765` a la [[UI OBC]]. También sirve la UI en `http://localhost:8765`.
- **Autodetección:** sin `--rx/--lidar` prueba cada puerto USB ~4 s y lo clasifica por la primera línea válida. Abre con DTR/RTS en bajo para no reiniciar las ESP32.
- **Robustez:** un hilo por puerto; si se desconecta, reintenta cada 2 s y avisa con `link`.
- Dependencias: `pyserial`, `websockets>=13`, `pytest`.
