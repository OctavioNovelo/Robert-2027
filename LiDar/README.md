# LiDAR — VL53L8CX en ESP32

`src/main.cpp` lee el sensor en 8×8 a 15 Hz (I2C SDA=21, SCL=22) y por serial (115200) imprime una
línea por frame:

```
F,d0:s0,d1:s1,...,d63:s63      d = distancia en mm, s = target_status (válido si 5 o 9)
E,init_failed                  el sensor no inicializó (se repite cada segundo)
```

```bash
cd LiDar
pio run -t upload
```

## Ver los datos

- **En la UI del rover (lo normal):** conecta esta ESP32 por USB a la PC y corre el puente
  (`OBC/puente/puente.py`). La rejilla aparece en el panel **LDR-8x8**. Ver `OBC/README.md`.
- **Solo el LiDAR, sin puente:** `visualizador.html` lee el puerto directo con Web Serial (Chrome/Edge):
  ```bash
  python -m http.server 8000      # y abrir http://localhost:8000/visualizador.html
  ```

El puerto serie solo admite un programa a la vez: cierra el visualizador o el Monitor Serie antes de
correr el puente.

`Extras/` tiene los sketches de prueba (escáner I2C, matriz en tiempo real, salida CSV).
