/*
 * leds.c
 */
#include "main.h"
#include "leds.h"

bool leds_patron(led_modo_t modo, uint32_t ahora_ms)
{
  uint32_t f = ahora_ms % 1000;

  switch (modo)
  {
  case LED_OK:
    return f < 50;
  case LED_FALLA_SENSOR:
    return f < 80 || (f >= 200 && f < 280);
  case LED_FALLA_RADIO:
    return (ahora_ms % 200) < 100;
  case LED_ARRANCANDO:
  default:
    return true;
  }
}

void leds_servicio(led_modo_t modo, uint32_t ahora_ms)
{
  // El LED de la BlackPill enciende con 0
  HAL_GPIO_WritePin(LED_ESTADO_GPIO_Port, LED_ESTADO_Pin,
                    leds_patron(modo, ahora_ms) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}
