/*
 * Receptor LoRa de telemetría del rover.
 *
 * Recibe el paquete binario de 10 bytes del STM32 transmisor y lo reenvía por
 * USB serial (115200) como una línea de texto para OBC/puente/puente.py:
 *
 *   T,<seq>,<temp>,<alt>,<pres>,<verif>,<rssi>,<snr>   paquete válido
 *   X,<motivo>                                        descartado: len | magic | cksum | radio
 *   H                                                 latido cada segundo
 *   E,radio_init                                      el SX1278 no respondió
 *
 * LEDs: destello en PIN_LED_OK = paquete válido, en PIN_LED_MAL = descartado;
 * PIN_LED_ERROR fijo = la radio no arrancó.
 *
 * Los pines y LEDs vienen de platformio.ini (build_flags), un entorno por placa.
 */

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "paquete.h"

// ---------- Radio: debe coincidir con Transmisor_STM32/.../sx1278.c ----------
#define LORA_FREQ_MHZ  434.0   // registros 0x06-0x08 = 0x6C8000
#define LORA_BW_KHZ    125.0   // 0x1D = 0x72
#define LORA_SF        7       // 0x1E = 0x74
#define LORA_CR        5       // 4/5
#define LORA_SYNC      0x34    // 0x39
#define LORA_POWER_DBM 17
#define LORA_PREAMBLE  8       // 0x20-0x21

#define LATIDO_MS   1000
#define DESTELLO_MS 60

SX1278 radio = new Module(PIN_NSS, PIN_DIO0, PIN_RST, PIN_DIO1);

volatile bool paqueteListo = false;

ICACHE_RAM_ATTR void alRecibir() { paqueteListo = true; }

#define LED_ON  (LED_ACTIVO_BAJO ? LOW : HIGH)
#define LED_OFF (LED_ACTIVO_BAJO ? HIGH : LOW)

const int LEDS[] = {PIN_LED_OK, PIN_LED_MAL, PIN_LED_ERROR};

void led(int pin, int nivel) {
  if (pin >= 0) digitalWrite(pin, nivel);
}

uint32_t apagarLedEn = 0;

void destello(int pin) {
  led(pin, LED_ON);
  apagarLedEn = millis() + DESTELLO_MS;
}

void apagarDestellos() {
  led(PIN_LED_OK, LED_OFF);
  led(PIN_LED_MAL, LED_OFF);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  for (int pin : LEDS) {
    if (pin < 0) continue;
    pinMode(pin, OUTPUT);
    led(pin, LED_OFF);
  }

  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_NSS);

  int estado = radio.begin(LORA_FREQ_MHZ, LORA_BW_KHZ, LORA_SF, LORA_CR,
                           LORA_SYNC, LORA_POWER_DBM, LORA_PREAMBLE);
  if (estado != RADIOLIB_ERR_NONE) led(PIN_LED_ERROR, LED_ON);
  while (estado != RADIOLIB_ERR_NONE) {
    // Se queda avisando para que el puente lo reconozca y la UI lo muestre.
    Serial.println("E,radio_init");
    delay(1000);
  }

  radio.setPacketReceivedAction(alRecibir);
  radio.startReceive();
}

void procesarPaquete() {
  uint8_t buf[32];
  size_t len = radio.getPacketLength();

  if (len > sizeof(buf)) {
    radio.readData(buf, sizeof(buf));
    Serial.println("X,len");
    destello(PIN_LED_MAL);
    return;
  }
  if (radio.readData(buf, len) != RADIOLIB_ERR_NONE) {
    // incluye CRC de LoRa inválido
    Serial.println("X,radio");
    destello(PIN_LED_MAL);
    return;
  }

  Telemetria t;
  const char *motivo = "";
  if (!decode_packet(buf, len, &t, &motivo)) {
    Serial.print("X,");
    Serial.println(motivo);
    destello(PIN_LED_MAL);
    return;
  }

  Serial.printf("T,%u,%d,%d,%u,%u,%.0f,%.2f\n",
                t.seq, t.temperatura, t.altitud, t.presion, t.verificacion,
                radio.getRSSI(), radio.getSNR());
  destello(PIN_LED_OK);
}

void loop() {
  static uint32_t ultimoLatido = 0;

  if (paqueteListo) {
    paqueteListo = false;
    procesarPaquete();
    radio.startReceive();
  }

  if (apagarLedEn && (int32_t)(millis() - apagarLedEn) >= 0) {
    apagarLedEn = 0;
    apagarDestellos();
  }

  if (millis() - ultimoLatido >= LATIDO_MS) {
    ultimoLatido = millis();
    Serial.println("H");
  }
}
