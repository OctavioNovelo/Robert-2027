/*
 * supercap.h
 *
 * Altitud barométrica y lógica de activación del supercapacitor (burn wire del
 * sistema de amortiguación). No usa HAL para poder probarse en la PC.
 *
 * Fases:
 *   SC_REFERENCIA  promedia la presión en el suelo al arrancar (altitud 0)
 *   SC_EN_TIERRA   espera a subir por encima de SUPERCAP_ALTURA_ARMADO_M
 *   SC_ARMADO      en vuelo; se activa al ir bajando y estar a <= SUPERCAP_ALTURA_ACTIVACION_M
 *   SC_ACTIVADO    salida encendida para siempre (hasta reiniciar la placa)
 *
 * Si el apogeo queda por debajo de SUPERCAP_ALTURA_ACTIVACION_M se activa en cuanto
 * se confirma el descenso (SUPERCAP_MARGEN_DESCENSO_M bajo el apogeo).
 */
#ifndef SUPERCAP_H
#define SUPERCAP_H

#include <stdbool.h>
#include <stdint.h>

// ---------- Ajustes de la misión ----------
#define SUPERCAP_ALTURA_ACTIVACION_M  400.0f  // en la bajada, a esta altura se activa
#define SUPERCAP_ALTURA_ARMADO_M       50.0f  // subir más que esto confirma que va en vuelo
#define SUPERCAP_MARGEN_DESCENSO_M     15.0f  // bajar esto desde el apogeo confirma el descenso
#define SUPERCAP_CONFIRMACIONES          5    // lecturas seguidas que deben cumplir (5 x 50 ms)

#define ALTURA_REF_DESCARTAR             5    // primeras lecturas del BME que se ignoran
#define ALTURA_REF_MUESTRAS             20    // lecturas promediadas para la presión del suelo

#define PRESION_MIN_PA               30000u   // fuera de este rango la lectura se ignora
#define PRESION_MAX_PA              110000u

typedef enum
{
    SC_REFERENCIA = 0,
    SC_EN_TIERRA,
    SC_ARMADO,
    SC_ACTIVADO,
} sc_fase_t;

typedef struct
{
    sc_fase_t fase;
    uint8_t   descartadas;
    uint8_t   n_ref;
    float     suma_ref;
    float     p0;              // presión del suelo (Pa)
    float     altitud_m;       // altitud actual sobre el suelo
    float     altitud_max_m;   // apogeo visto hasta ahora
    uint8_t   cuenta;          // lecturas seguidas que cumplen la condición de la fase
} supercap_t;

void supercap_init(supercap_t *s);

// Procesa una lectura de presión. Devuelve true solo en la lectura en que se
// debe encender la salida del supercapacitor.
bool supercap_actualizar(supercap_t *s, uint32_t presion_pa);

float altitud_desde_presion(float presion_pa, float p0_pa);

#endif
