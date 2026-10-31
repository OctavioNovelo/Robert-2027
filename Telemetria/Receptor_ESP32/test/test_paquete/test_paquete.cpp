#include <unity.h>
#include <string.h>
#include "paquete.h"

// Arma el paquete igual que telemetry_build_LoRa() del STM32 transmisor.
static void armar(uint8_t *b, uint8_t pkt, int16_t temp, int32_t alt, uint32_t pres, uint8_t estado)
{
    TelemetryPacketLoRa p;
    memset(&p, 0, sizeof(p));
    p.magic = 0xCA;
    p.pkt = pkt;
    p.t_ms = 123456;
    p.temperatura = temp;
    p.presion = pres;
    p.humedad = 4550;
    p.altitud = alt;
    p.accel[0] = 12; p.accel[1] = -34; p.accel[2] = 981;
    p.gyro[0] = 1500; p.gyro[1] = -250; p.gyro[2] = 0;
    p.estado = estado;
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
    armar(b, 42, -512, 12034, 101325, EST_BME_OK | EST_BNO_OK | EST_REF_OK);
    Telemetria t;
    const char *motivo = nullptr;
    TEST_ASSERT_TRUE(decode_packet(b, sizeof(b), &t, &motivo));
    TEST_ASSERT_EQUAL_UINT8(42, t.seq);
    TEST_ASSERT_EQUAL_UINT32(123456, t.t_ms);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -5.12f, t.temperatura);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 120.34f, t.altitud);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1013.25f, t.presion);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 45.5f, t.humedad);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -0.34f, t.accel[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 9.81f, t.accel[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.5f, t.gyro[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -0.25f, t.gyro[1]);
    TEST_ASSERT_TRUE(sensores_ok(t.estado));
}

void test_sensor_caido(void)
{
    TEST_ASSERT_FALSE(sensores_ok(EST_BME_OK | EST_REF_OK));
    TEST_ASSERT_FALSE(sensores_ok(EST_BNO_OK));
}

void test_checksum_malo(void)
{
    uint8_t b[PAQUETE_LEN];
    armar(b, 1, 2000, 0, 90000, 0);
    b[PAQUETE_LEN - 1] ^= 0xFF;
    Telemetria t;
    const char *motivo = nullptr;
    TEST_ASSERT_FALSE(decode_packet(b, sizeof(b), &t, &motivo));
    TEST_ASSERT_EQUAL_STRING("cksum", motivo);
}

void test_magic_malo(void)
{
    uint8_t b[PAQUETE_LEN];
    armar(b, 1, 2000, 0, 90000, 0);
    b[0] = 0xCB;
    Telemetria t;
    const char *motivo = nullptr;
    TEST_ASSERT_FALSE(decode_packet(b, sizeof(b), &t, &motivo));
    TEST_ASSERT_EQUAL_STRING("magic", motivo);
}

void test_longitud_mala(void)
{
    uint8_t b[PAQUETE_LEN];
    armar(b, 1, 2000, 0, 90000, 0);
    Telemetria t;
    const char *motivo = nullptr;
    TEST_ASSERT_FALSE(decode_packet(b, PAQUETE_LEN - 1, &t, &motivo));
    TEST_ASSERT_EQUAL_STRING("len", motivo);
    // el paquete viejo de 10 bytes también se rechaza
    TEST_ASSERT_FALSE(decode_packet(b, 10, &t, &motivo));
    TEST_ASSERT_EQUAL_STRING("len", motivo);
}

int main(int, char **)
{
    UNITY_BEGIN();
    RUN_TEST(test_paquete_valido);
    RUN_TEST(test_sensor_caido);
    RUN_TEST(test_checksum_malo);
    RUN_TEST(test_magic_malo);
    RUN_TEST(test_longitud_mala);
    return UNITY_END();
}
