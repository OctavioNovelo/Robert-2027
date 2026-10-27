# LiDAR VL53L8CX

- **Dónde:** `LiDar/` (PlatformIO, `esp32dev`, librería STM32duino VL53L8CX).
- **I2C:** SDA 21, SCL 22, 400 kHz. 8×8 zonas a 15 Hz.
- **Salida:** `F,d:s,...` (64 pares) — status válido 5 o 9. Ver [[Protocolo]].
- Se ve en la [[UI OBC]] (panel LDR-8x8) a través del [[Puente]], o solo con `LiDar/visualizador.html` (Web Serial).
