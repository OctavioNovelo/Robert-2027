/*
 * telemetry.h
 *
 *  Created on: Mar 13, 2026
 *      Author: shuten-doji
 *
 * Paquete LoRa único con BME280 + BNO085 + estado del transmisor.
 * Debe ser idéntico a Telemetria/Receptor_ESP32/lib/paquete/paquete.h.
 * STM32 y ESP32 son little-endian y el struct va sin padding.
 */
#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdint.h>

#define TELEMETRY_MAGIC 0xCA

// Bits de TelemetryPacketLoRa.estado
#define EST_BME_OK    (1u << 0)  // la última lectura del BME280 fue válida
#define EST_BNO_OK    (1u << 1)  // hay datos recientes del BNO085
#define EST_REF_OK    (1u << 2)  // ya se tomó la presión de referencia del suelo
#define EST_ARMADO    (1u << 3)  // superó la altura de armado (está en vuelo)
#define EST_SUPERCAP  (1u << 4)  // supercapacitor activado

#pragma pack(push,1)
typedef struct
{
    uint8_t  magic;         // TELEMETRY_MAGIC
    uint8_t  pkt;           // número de secuencia (da la vuelta en 255)
    uint32_t t_ms;          // ms desde que arrancó el STM32

    int16_t  temperatura;   // 0.01 °C
    uint32_t presion;       // Pa
    uint16_t humedad;       // 0.01 %RH
    int32_t  altitud;       // cm sobre el punto donde arrancó la placa

    int16_t  accel[3];      // 0.01 m/s²  (x, y, z)
    int16_t  gyro[3];       // 0.001 rad/s (x, y, z)

    uint8_t  estado;        // bits EST_*
    uint8_t  checksum;      // XOR de todos los bytes anteriores
} TelemetryPacketLoRa;
#pragma pack(pop)

_Static_assert(sizeof(TelemetryPacketLoRa) == 32, "el receptor espera 32 bytes");

// Datos en unidades naturales; telemetry_build_LoRa los escala al paquete.
typedef struct
{
    uint32_t t_ms;
    int32_t  temperatura;   // 0.01 °C
    uint32_t presion;       // Pa
    uint32_t humedad;       // 0.01 %RH
    float    altitud_m;
    float    accel[3];      // m/s²
    float    gyro[3];       // rad/s
    uint8_t  estado;
} TelemetryDatos;

void telemetry_build_LoRa(TelemetryPacketLoRa *protocolLoRa, const TelemetryDatos *datos);

#endif
