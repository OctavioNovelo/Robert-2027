/*
 * sx1278.h
 *
 *  Created on: Mar 14, 2026
 *      Author: shuten-doji
 */

#ifndef INC_SX1278_H_
#define INC_SX1278_H_

#include <stdbool.h>
#include <stdint.h>

// Debe coincidir con Telemetria/Receptor_ESP32/src/main.cpp
#define SX1278_FREQ_HZ  434000000UL   // registros 0x06-0x08 = 0x6C8000

// Resetea, comprueba RegVersion (0x12) y configura LoRa. false si no responde.
bool sx1278_init(void);

// Empieza a transmitir y enciende LED_TX. false si sigue enviando el anterior.
bool sx1278_send(const uint8_t *data, uint8_t len);

// Llamar en cada vuelta del while: detecta el fin del envío (o el timeout).
void sx1278_servicio(void);

bool sx1278_ocupado(void);
void sx1278_onDio0Irq(void);
uint8_t sx1278_read(uint8_t reg);

#endif /* INC_SX1278_H_ */
