/*
 * bno085.h
 *
 * BNO085 por I2C1 con la librería SH2 de CEVA: acelerómetro y giroscopio a 50 Hz.
 * Si el BNO no responde o deja de mandar datos, se reinicia y se reintenta solo.
 */
#ifndef BNO085_H
#define BNO085_H

#include <stdbool.h>

void bno085_iniciar(void);

// Llamar en cada vuelta del while: atiende al BNO y reintenta si hace falta.
void bno085_servicio(void);

// true si llegaron acelerómetro y giroscopio hace poco.
bool bno085_ok(void);

// Copia accel (m/s²) y gyro (rad/s). Si bno085_ok() es false deja ceros.
void bno085_leer(float accel[3], float gyro[3]);

#endif
