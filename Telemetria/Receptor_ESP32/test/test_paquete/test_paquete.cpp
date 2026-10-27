#include <unity.h>
#include <string.h>
#include "paquete.h"

// Arma el paquete igual que telemetry_build_LoRa() del STM32 transmisor.
static void armar(uint8_t *b, uint8_t pkt, int16_t temp, int16_t alt, uint16_t pres, uint8_t verif)
{
    TelemetryPacketLoRa p;
    p.magic = 0xCA;
    p.pkt = pkt;
    p.temperatura = temp;
    p.altitud = alt;
    p.presion = pres;
    p.verificacion = verif;
    uint8_t c = 0;
    const uint8_t *raw = (const uint8_t *)&p;
    for (size_t i = 0; i < sizeof(p) - 1; i++) c ^= raw[i];
    p.checksum = c;
    memcpy(b, &p, sizeof(p));
}

void setUp(void) {}
void tearDown(void) {}

void test_paquete_valido(void)
{
    uint8_t b[PAQUETE_LEN];
    armar(b, 42, -5, 120, 1013, 1);
    Telemetria t;
    const char *motivo = nullptr;
    TEST_ASSERT_TRUE(decode_packet(b, sizeof(b), &t, &motivo));
    TEST_ASSERT_EQUAL_UINT8(42, t.seq);
    TEST_ASSERT_EQUAL_INT16(-5, t.temperatura);
    TEST_ASSERT_EQUAL_INT16(120, t.altitud);
    TEST_ASSERT_EQUAL_UINT16(1013, t.presion);
    TEST_ASSERT_EQUAL_UINT8(1, t.verificacion);
}

void test_checksum_malo(void)
{
    uint8_t b[PAQUETE_LEN];
    armar(b, 1, 20, 0, 900, 1);
    b[PAQUETE_LEN - 1] ^= 0xFF;
    Telemetria t;
    const char *motivo = nullptr;
    TEST_ASSERT_FALSE(decode_packet(b, sizeof(b), &t, &motivo));
    TEST_ASSERT_EQUAL_STRING("cksum", motivo);
}

void test_magic_malo(void)
{
    uint8_t b[PAQUETE_LEN];
    armar(b, 1, 20, 0, 900, 1);
    b[0] = 0xCB;
    Telemetria t;
    const char *motivo = nullptr;
    TEST_ASSERT_FALSE(decode_packet(b, sizeof(b), &t, &motivo));
    TEST_ASSERT_EQUAL_STRING("magic", motivo);
}

void test_longitud_mala(void)
{
    uint8_t b[PAQUETE_LEN];
    armar(b, 1, 20, 0, 900, 1);
    Telemetria t;
    const char *motivo = nullptr;
    TEST_ASSERT_FALSE(decode_packet(b, PAQUETE_LEN - 1, &t, &motivo));
    TEST_ASSERT_EQUAL_STRING("len", motivo);
}

int main(int, char **)
{
    UNITY_BEGIN();
    RUN_TEST(test_paquete_valido);
    RUN_TEST(test_checksum_malo);
    RUN_TEST(test_magic_malo);
    RUN_TEST(test_longitud_mala);
    return UNITY_END();
}
