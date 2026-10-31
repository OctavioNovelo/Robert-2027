/*
 * paquete.h
 *
 * Decodificación del paquete de telemetría que envía el STM32 transmisor
 * (Telemetria/Transmisor_STM32/Core/Inc/telemetry.h). No depende de Arduino
 * para poder probarse en la PC con `pio test -e native`.
 */

#ifndef PAQUETE_H
#define PAQUETE_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Bits de TelemetryPacketLoRa.estado
#define EST_BME_OK    (1u << 0)  // la última lectura del BME280 fue válida
#define EST_BNO_OK    (1u << 1)  // hay datos recientes del BNO085
#define EST_REF_OK    (1u << 2)  // ya se tomó la presión de referencia del suelo
#define EST_ARMADO    (1u << 3)  // superó la altura de armado (está en vuelo)
#define EST_SUPERCAP  (1u << 4)  // supercapacitor activado

#pragma pack(push, 1)   // mismo layout que el transmisor: sin padding
typedef struct
{
    uint8_t  magic;         // 0xCA
    uint8_t  pkt;           // número de secuencia
    uint32_t t_ms;          // ms desde que arrancó el STM32
    int16_t  temperatura;   // 0.01 °C
    uint32_t presion;       // Pa
    uint16_t humedad;       // 0.01 %RH
    int32_t  altitud;       // cm sobre el punto donde arrancó la placa
    int16_t  accel[3];      // 0.01 m/s²
    int16_t  gyro[3];       // 0.001 rad/s
    uint8_t  estado;        // bits EST_*
    uint8_t  checksum;      // XOR de todos los bytes anteriores
} TelemetryPacketLoRa;      // 32 bytes
#pragma pack(pop)

static_assert(sizeof(TelemetryPacketLoRa) == 32, "debe coincidir con telemetry.h del STM32");

#define PAQUETE_LEN   sizeof(TelemetryPacketLoRa)
#define PAQUETE_MAGIC 0xCA

// Ya en unidades físicas
typedef struct
{
    uint8_t  seq;
    uint32_t t_ms;
    float    temperatura;   // °C
    float    presion;       // hPa
    float    humedad;       // %RH
    float    altitud;       // m
    float    accel[3];      // m/s²
    float    gyro[3];       // rad/s
    uint8_t  estado;
} Telemetria;

// true si el BME280 y el BNO085 del transmisor están dando datos
static inline bool sensores_ok(uint8_t estado)
{
    return (estado & (EST_BME_OK | EST_BNO_OK)) == (EST_BME_OK | EST_BNO_OK);
}

// Valida longitud, magic y checksum. Si falla, *motivo queda en "len", "magic" o "cksum".
static inline bool decode_packet(const uint8_t *buf, size_t len, Telemetria *out, const char **motivo)
{
    if (len != PAQUETE_LEN) { *motivo = "len"; return false; }

    TelemetryPacketLoRa p;
    memcpy(&p, buf, PAQUETE_LEN);   // STM32 y ESP32 son little-endian

    if (p.magic != PAQUETE_MAGIC) { *motivo = "magic"; return false; }

    uint8_t c = 0;
    for (size_t i = 0; i < PAQUETE_LEN - 1; i++) c ^= buf[i];
    if (c != p.checksum) { *motivo = "cksum"; return false; }

    out->seq         = p.pkt;
    out->t_ms        = p.t_ms;
    out->temperatura = p.temperatura / 100.0f;
    out->presion     = p.presion / 100.0f;
    out->humedad     = p.humedad / 100.0f;
    out->altitud     = p.altitud / 100.0f;
    for (int i = 0; i < 3; i++)
    {
        out->accel[i] = p.accel[i] / 100.0f;
        out->gyro[i]  = p.gyro[i] / 1000.0f;
    }
    out->estado = p.estado;
    return true;
}

#endif
