/*
 * Receptor LoRa de telemetría del CanSat.
 *
 * Recibe el paquete binario de 32 bytes del STM32 transmisor (BME280 + BNO085 +
 * estado) y lo reenvía por USB serial (115200) como una línea de texto para
 * OBC/puente/puente.py:
 *
 *   T,<seq>,<temp>,<alt>,<pres>,<verif>,<rssi>,<snr>,<hum>,<ax>,<ay>,<az>,<gx>,<gy>,<gz>,<estado>,<t_ms>
 *       temp °C, alt m, pres hPa, verif 1 = BME y BNO OK, hum %RH,
 *       accel m/s², gyro rad/s, estado = bits EST_* (paquete.h), t_ms del STM32
 *   X,<motivo>    descartado: len | magic | cksum | radio
 *   H             latido cada segundo
 *   E,radio_init  el SX1278 no respondió (se reintenta cada segundo)
 *
 * LEDs:
 *   PIN_LED_RX     encendido mientras llegan paquetes válidos; se apaga si pasan
 *                  ENLACE_TIMEOUT_MS sin recibir
 *   PIN_LED_MAL    destello con cada paquete descartado
 *   PIN_LED_ERROR  parpadeo rápido si la radio no arranca
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

#define LATIDO_MS         1000
#define DESTELLO_MS       60
#define ENLACE_TIMEOUT_MS 1500   // 3 paquetes perdidos (el STM32 envía cada 500 ms)

SX1278 radio = new Module(PIN_NSS, PIN_DIO0, PIN_RST, PIN_DIO1);

volatile bool paqueteListo = false;

IRAM_ATTR void alRecibir() { paqueteListo = true; }

#define LED_ON  (LED_ACTIVO_BAJO ? LOW : HIGH)
#define LED_OFF (LED_ACTIVO_BAJO ? HIGH : LOW)

const int LEDS[] = {PIN_LED_RX, PIN_LED_MAL, PIN_LED_ERROR};

void led(int pin, int nivel) {
  if (pin >= 0) digitalWrite(pin, nivel);
}

uint32_t ultimoValido = 0;      // millis() del último paquete válido; 0 = nunca
uint32_t apagarMalEn = 0;

int iniciarRadio() {
  return radio.begin(LORA_FREQ_MHZ, LORA_BW_KHZ, LORA_SF, LORA_CR,
                     LORA_SYNC, LORA_POWER_DBM, LORA_PREAMBLE);
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

  while (iniciarRadio() != RADIOLIB_ERR_NONE) {
    // Se queda avisando para que el puente lo reconozca y la UI lo muestre.
    Serial.println("E,radio_init");
    for (int i = 0; i < 10; i++) {
      led(PIN_LED_ERROR, i % 2 ? LED_OFF : LED_ON);
      delay(100);
    }
  }
  led(PIN_LED_ERROR, LED_OFF);

  radio.setPacketReceivedAction(alRecibir);
  radio.startReceive();
}

void descartado(const char *motivo) {
  Serial.print("X,");
  Serial.println(motivo);
  if (PIN_LED_MAL >= 0) {
    led(PIN_LED_MAL, LED_ON);
    apagarMalEn = millis() + DESTELLO_MS;
  }
}

void procesarPaquete() {
  uint8_t buf[64];
  size_t len = radio.getPacketLength();

  if (len > sizeof(buf)) {
    radio.readData(buf, sizeof(buf));
    descartado("len");
    return;
  }
  if (radio.readData(buf, len) != RADIOLIB_ERR_NONE) {
    // incluye CRC de LoRa inválido
    descartado("radio");
    return;
  }

  Telemetria t;
  const char *motivo = "";
  if (!decode_packet(buf, len, &t, &motivo)) {
    descartado(motivo);
    return;
  }

  Serial.printf("T,%u,%.2f,%.2f,%.2f,%u,%.0f,%.2f,%.2f,%.2f,%.2f,%.2f,%.3f,%.3f,%.3f,%u,%lu\n",
                t.seq, t.temperatura, t.altitud, t.presion, sensores_ok(t.estado) ? 1 : 0,
                radio.getRSSI(), radio.getSNR(), t.humedad,
                t.accel[0], t.accel[1], t.accel[2], t.gyro[0], t.gyro[1], t.gyro[2],
                t.estado, (unsigned long)t.t_ms);

  ultimoValido = millis();
  if (ultimoValido == 0) ultimoValido = 1;
  led(PIN_LED_RX, LED_ON);
}

void loop() {
  static uint32_t ultimoLatido = 0;

  if (paqueteListo) {
    paqueteListo = false;
    procesarPaquete();
    radio.startReceive();
  }

  uint32_t ahora = millis();

  if (ultimoValido && ahora - ultimoValido >= ENLACE_TIMEOUT_MS) {
    ultimoValido = 0;
    led(PIN_LED_RX, LED_OFF);
  }

  if (apagarMalEn && (int32_t)(ahora - apagarMalEn) >= 0) {
    apagarMalEn = 0;
    led(PIN_LED_MAL, LED_OFF);
  }

  if (ahora - ultimoLatido >= LATIDO_MS) {
    ultimoLatido = ahora;
    Serial.println("H");
  }
}
