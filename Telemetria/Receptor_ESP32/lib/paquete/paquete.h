/*
 * paquete.h
 *
 * Decodificación del paquete de telemetría que envía el STM32 transmisor
 * (Telemetria/Transmisor_STM32/.../Core/Inc/telemetry.h). No depende de Arduino
 * para poder probarse en la PC con `pio test -e native`.
 */

#ifndef PAQUETE_H
#define PAQUETE_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#pragma pack(push, 1)   // mismo layout que el transmisor: sin padding
typedef struct
{
    uint8_t  magic;        // 0xCA
    uint8_t  pkt;          // número de secuencia
    int16_t  temperatura;  // °C enteros
    int16_t  altitud;      // metros
    uint16_t presion;      // hPa
    uint8_t  verificacion; // flag
    uint8_t  checksum;     // XOR de todos los bytes anteriores
} TelemetryPacketLoRa;     // 10 bytes
#pragma pack(pop)

#define PAQUETE_LEN   sizeof(TelemetryPacketLoRa)
#define PAQUETE_MAGIC 0xCA

typedef struct
{
    uint8_t  seq;
    int16_t  temperatura;
    int16_t  altitud;
    uint16_t presion;
    uint8_t  verificacion;
} Telemetria;

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

    out->seq          = p.pkt;
    out->temperatura  = p.temperatura;
    out->altitud      = p.altitud;
    out->presion      = p.presion;
    out->verificacion = p.verificacion;
    return true;
}

#endif
