# Extras — sketches de prueba del VL53L8CX

Sketches sueltos de Arduino IDE para probar el sensor sin el proyecto PlatformIO.

| Sketch | Qué hace |
|---|---|
| `01_i2c_scanner.ino` | Busca dispositivos en el bus I2C: sirve para confirmar que el sensor está bien conectado. |
| `02_vl53l8_realtime.ino` | Imprime la matriz 8×8 de distancias por el Monitor Serie. |
| `03_vl53l8_csv.ino` | Imprime cada frame como una línea `F,...` separada por comas, para `LiDar/visualizador.html`. Falta probarlo. |

Para ver la rejilla en la UI del rover o en `visualizador.html` se usa el firmware de `LiDar/src/main.cpp`, no estos sketches.
