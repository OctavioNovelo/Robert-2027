/*
 * supercap.c
 *
 * Ver supercap.h para la descripción de las fases.
 */
#include <math.h>
#include <string.h>
#include "supercap.h"

void supercap_init(supercap_t *s)
{
    memset(s, 0, sizeof(*s));
    s->fase = SC_REFERENCIA;
}

float altitud_desde_presion(float presion_pa, float p0_pa)
{
    // Fórmula barométrica internacional (troposfera)
    return 44330.0f * (1.0f - powf(presion_pa / p0_pa, 0.1902949f));
}

// Cuenta lecturas seguidas que cumplen; true al llegar a SUPERCAP_CONFIRMACIONES.
static bool confirmar(supercap_t *s, bool cumple)
{
    s->cuenta = cumple ? (uint8_t)(s->cuenta + 1) : 0;
    if (s->cuenta < SUPERCAP_CONFIRMACIONES) return false;
    s->cuenta = 0;
    return true;
}

bool supercap_actualizar(supercap_t *s, uint32_t presion_pa)
{
    if (presion_pa < PRESION_MIN_PA || presion_pa > PRESION_MAX_PA)
    {
        s->cuenta = 0;
        return false;
    }

    if (s->fase == SC_REFERENCIA)
    {
        // El BME280 tarda unas lecturas en asentarse (filtro IIR)
        if (s->descartadas < ALTURA_REF_DESCARTAR)
        {
            s->descartadas++;
            return false;
        }
        s->suma_ref += (float)presion_pa;
        if (++s->n_ref >= ALTURA_REF_MUESTRAS)
        {
            s->p0 = s->suma_ref / (float)s->n_ref;
            s->fase = SC_EN_TIERRA;
        }
        return false;
    }

    s->altitud_m = altitud_desde_presion((float)presion_pa, s->p0);
    if (s->altitud_m > s->altitud_max_m) s->altitud_max_m = s->altitud_m;

    switch (s->fase)
    {
    case SC_EN_TIERRA:
        if (confirmar(s, s->altitud_m >= SUPERCAP_ALTURA_ARMADO_M))
            s->fase = SC_ARMADO;
        return false;

    case SC_ARMADO:
    {
        bool bajando = s->altitud_m <= s->altitud_max_m - SUPERCAP_MARGEN_DESCENSO_M;
        if (confirmar(s, bajando && s->altitud_m <= SUPERCAP_ALTURA_ACTIVACION_M))
        {
            s->fase = SC_ACTIVADO;
            return true;
        }
        return false;
    }

    default:
        return false;
    }
}
