/*
 * bme280.h
 *
 * Driver mínimo del BME280 por I2C1 (compensación entera de Bosch).
 */
#ifndef BME280_H
#define BME280_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    int32_t  temperatura;  // 0.01 °C (2534 = 25.34 °C)
    uint32_t presion;      // Pa
    uint32_t humedad;      // 0.01 %RH (4550 = 45.50 %RH)
} bme280_dato_t;

// Busca el sensor en 0x76 y 0x77, lo reinicia y lo deja midiendo en modo normal.
bool bme280_init(void);

// false si falla el I2C, aún no hay medición o la presión está fuera de rango.
bool bme280_leer(bme280_dato_t *dato);

#endif
