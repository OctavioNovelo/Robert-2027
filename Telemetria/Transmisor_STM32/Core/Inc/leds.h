/*
 * leds.h
 *
 * LED_ESTADO (LED azul de la BlackPill, PC13) dice cómo está el transmisor:
 *
 *   fijo encendido         arrancando (si dura más de ~3 s: error fatal)
 *   destello corto / 1 s   todo bien: radio, BME280 y BNO085 funcionando
 *   doble destello / 1 s   falla un sensor (BME280 o BNO085); ver bits de estado
 *   parpadeo rápido 5 Hz   el SX1278 no responde: no hay telemetría
 *
 * LED_TX (LED "para debug" de la placa, PB4) lo maneja sx1278.c: encendido
 * mientras un paquete está en el aire.
 *
 * Error fatal (Error_Handler / HardFault): los dos LEDs fijos.
 */
#ifndef LEDS_H
#define LEDS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    LED_ARRANCANDO,
    LED_OK,
    LED_FALLA_SENSOR,
    LED_FALLA_RADIO,
} led_modo_t;

bool leds_patron(led_modo_t modo, uint32_t ahora_ms);
void leds_servicio(led_modo_t modo, uint32_t ahora_ms);

#endif
