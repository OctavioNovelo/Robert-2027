/*
 * telemetry.c
 *
 *  Created on: Mar 13, 2026
 *      Author: shuten-doji
 */
#include "telemetry.h"

static uint8_t pkt = 0;

static uint8_t telemetry_checksum(const uint8_t *data, uint8_t len)
{
    uint8_t c = 0;

    for(int i = 0; i < len; i++)
        c ^= data[i];

    return c;
}

// Redondea y satura para que un valor fuera de rango no dé la vuelta.
static int32_t a_entero(float v, float min, float max)
{
    if (v != v) return 0;  // NaN
    if (v > max) v = max;
    if (v < min) v = min;
    return (int32_t)(v >= 0.0f ? v + 0.5f : v - 0.5f);
}

static int16_t a_int16(float v)
{
    return (int16_t)a_entero(v, -32768.0f, 32767.0f);
}

void telemetry_build_LoRa(TelemetryPacketLoRa *protocolLoRa, const TelemetryDatos *datos)
{
	protocolLoRa->magic = TELEMETRY_MAGIC;
	protocolLoRa->pkt = pkt++;
	protocolLoRa->t_ms = datos->t_ms;

	protocolLoRa->temperatura = a_int16((float)datos->temperatura);
	protocolLoRa->presion = datos->presion;
	protocolLoRa->humedad = (uint16_t)(datos->humedad > 10000 ? 10000 : datos->humedad);
	protocolLoRa->altitud = a_entero(datos->altitud_m * 100.0f, -2.0e9f, 2.0e9f);

	for (int i = 0; i < 3; i++)
	{
		protocolLoRa->accel[i] = a_int16(datos->accel[i] * 100.0f);
		protocolLoRa->gyro[i] = a_int16(datos->gyro[i] * 1000.0f);
	}

	protocolLoRa->estado = datos->estado;
	protocolLoRa->checksum = telemetry_checksum((const uint8_t*)protocolLoRa, sizeof(TelemetryPacketLoRa) - 1);
}
