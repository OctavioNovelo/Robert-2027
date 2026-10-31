/*
 * bno085.c
 *
 * HAL de I2C para SH2 (sh2_hal.h) y lectura de acelerómetro/giroscopio.
 */
#include <string.h>
#include "main.h"
#include "bno085.h"
#include "sh2.h"
#include "sh2_err.h"
#include "sh2_hal.h"
#include "sh2_SensorValue.h"

extern I2C_HandleTypeDef hi2c1;

#define BNO_ADDR_A          0x4A    // pin DI/SA0 a GND
#define BNO_ADDR_B          0x4B    // pin DI/SA0 a 3V3
#define BNO_REPORTE_US      20000   // 50 Hz
#define BNO_DATO_VIGENTE_MS 200     // más viejo que esto: bno085_ok() = false
#define BNO_SIN_DATOS_MS    2000    // sin reportes este tiempo: se reinicia el BNO
#define BNO_REINTENTO_MS    5000
#define BNO_SONDEO_MS       10      // si INT no baja, se pregunta igual cada 10 ms

// Para verlos con el depurador
volatile uint8_t  bno_addr = 0;          // 0x4A/0x4B si respondió, 0 si no
volatile uint32_t bno_errores_i2c = 0;
volatile uint32_t bno_aperturas = 0;

static sh2_Hal_t hal;
static uint8_t  abierto = 0;
static uint8_t  reconfigurar = 0;
static uint32_t t_intento = 0;
static uint32_t t_ultimo_dato = 0;
static uint32_t t_sondeo = 0;

static float    accel[3], gyro[3];
static uint32_t t_accel = 0, t_gyro = 0;
static uint8_t  hay_accel = 0, hay_gyro = 0;

// ---------- Tiempo en µs con el contador de ciclos (DWT) ----------
// CYCCNT da la vuelta cada ~51 s a 84 MHz; aquí se acumula para que los µs
// den la vuelta en 2^32 µs como espera SH2. Solo se llama desde el while.
static uint32_t us_total = 0, cyc_previo = 0, cyc_resto = 0;

static void dwt_init(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  cyc_previo = 0;
}

static uint32_t micros32(void)
{
  uint32_t ciclos_us = SystemCoreClock / 1000000U;
  uint32_t cyc = DWT->CYCCNT;
  cyc_resto += cyc - cyc_previo;
  cyc_previo = cyc;
  us_total += cyc_resto / ciclos_us;
  cyc_resto %= ciclos_us;
  return us_total;
}

// ---------- Funciones HAL que pide SH2 ----------
static int sh2hal_open(sh2_Hal_t *self)
{
  (void)self;

  // Reset del BNO085 (activo en bajo)
  HAL_GPIO_WritePin(BNO_RST_GPIO_Port, BNO_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(10);
  HAL_GPIO_WritePin(BNO_RST_GPIO_Port, BNO_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(100);

  if (HAL_I2C_IsDeviceReady(&hi2c1, BNO_ADDR_A << 1, 3, 10) == HAL_OK)
    bno_addr = BNO_ADDR_A;
  else if (HAL_I2C_IsDeviceReady(&hi2c1, BNO_ADDR_B << 1, 3, 10) == HAL_OK)
    bno_addr = BNO_ADDR_B;
  else
    bno_addr = 0;

  return bno_addr ? SH2_OK : SH2_ERR;
}

static void sh2hal_close(sh2_Hal_t *self)
{
  (void)self;
  HAL_GPIO_WritePin(BNO_RST_GPIO_Port, BNO_RST_Pin, GPIO_PIN_RESET);
}

// SHTP por I2C: primero se leen los 4 bytes de cabecera para saber el largo y
// luego se lee el paquete completo (el BNO lo repite desde la cabecera).
static int sh2hal_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len, uint32_t *t_us)
{
  (void)self;

  // INT (H_INTN) en bajo = hay un paquete listo
  if (HAL_GPIO_ReadPin(BNO_INT_GPIO_Port, BNO_INT_Pin) == GPIO_PIN_SET)
  {
    if (HAL_GetTick() - t_sondeo < BNO_SONDEO_MS) return 0;
  }
  t_sondeo = HAL_GetTick();

  uint32_t t = micros32();
  uint8_t hdr[4];
  if (HAL_I2C_Master_Receive(&hi2c1, bno_addr << 1, hdr, sizeof(hdr), 10) != HAL_OK)
  {
    bno_errores_i2c++;
    return 0;
  }

  unsigned total = ((unsigned)hdr[0] | ((unsigned)hdr[1] << 8)) & 0x7FFF;
  if (total == 0) return 0;
  if (total > len) total = len;  // el resto llega como continuación

  if (HAL_I2C_Master_Receive(&hi2c1, bno_addr << 1, pBuffer, total, 50) != HAL_OK)
  {
    bno_errores_i2c++;
    return 0;
  }

  *t_us = t;
  return (int)total;
}

static int sh2hal_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len)
{
  (void)self;
  if (HAL_I2C_Master_Transmit(&hi2c1, bno_addr << 1, pBuffer, len, 50) != HAL_OK) {
    bno_errores_i2c++;
    return 0;
  }
  return (int)len;
}

static uint32_t sh2hal_getTimeUs(sh2_Hal_t *self)
{
  (void)self;
  return micros32();
}

// ---------- Callbacks de SH2 (se llaman dentro de sh2_service) ----------
static void sh2_event_cb(void *cookie, sh2_AsyncEvent_t *pEvent)
{
  (void)cookie;
  // Si el BNO se reinicia solo, olvida los reportes configurados
  if (pEvent->eventId == SH2_RESET) reconfigurar = 1;
}

static void leer_bno(void *cookie, sh2_SensorEvent_t *pEvent)
{
  (void)cookie;
  sh2_SensorValue_t v;
  if (sh2_decodeSensorEvent(&v, pEvent) != SH2_OK) return;

  uint32_t ahora = HAL_GetTick();
  if (v.sensorId == SH2_ACCELEROMETER)
  {
    accel[0] = v.un.accelerometer.x;
    accel[1] = v.un.accelerometer.y;
    accel[2] = v.un.accelerometer.z;
    t_accel = ahora;
    hay_accel = 1;
    t_ultimo_dato = ahora;
  }
  else if (v.sensorId == SH2_GYROSCOPE_CALIBRATED)
  {
    gyro[0] = v.un.gyroscope.x; // rad/s
    gyro[1] = v.un.gyroscope.y;
    gyro[2] = v.un.gyroscope.z;
    t_gyro = ahora;
    hay_gyro = 1;
    t_ultimo_dato = ahora;
  }
}

static int configurar_reportes(void)
{
  sh2_SensorConfig_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.reportInterval_us = BNO_REPORTE_US;

  int rc = sh2_setSensorConfig(SH2_ACCELEROMETER, &cfg);
  if (rc == SH2_OK) rc = sh2_setSensorConfig(SH2_GYROSCOPE_CALIBRATED, &cfg);
  return rc;
}

// Un BNO que no arranca bien ensucia el bus I2C y tumba también al BME280:
// mientras no responda se queda en reset y se limpia el periférico I2C.
static void abrir_fallo(void)
{
  sh2_close();
  HAL_GPIO_WritePin(BNO_RST_GPIO_Port, BNO_RST_Pin, GPIO_PIN_RESET);
  i2c1_recuperar();
}

static void abrir(void)
{
  t_intento = HAL_GetTick();
  bno_aperturas++;

  if (sh2_open(&hal, sh2_event_cb, NULL) != SH2_OK)
  {
    abrir_fallo();
    return;
  }
  sh2_setSensorCallback(leer_bno, NULL);

  reconfigurar = 0;
  if (configurar_reportes() != SH2_OK)
  {
    abrir_fallo();
    return;
  }

  abierto = 1;
  t_ultimo_dato = HAL_GetTick();
}

void bno085_iniciar(void)
{
  hal.open = sh2hal_open;
  hal.close = sh2hal_close;
  hal.read = sh2hal_read;
  hal.write = sh2hal_write;
  hal.getTimeUs = sh2hal_getTimeUs;

  dwt_init();
  abrir();
}

void bno085_servicio(void)
{
  uint32_t ahora = HAL_GetTick();

  if (!abierto)
  {
    if (ahora - t_intento >= BNO_REINTENTO_MS) abrir();
    return;
  }

  sh2_service();  // debe llamarse muy seguido

  if (reconfigurar)
  {
    reconfigurar = 0;
    configurar_reportes();
  }

  if (HAL_GetTick() - t_ultimo_dato >= BNO_SIN_DATOS_MS)
  {
    // Se colgó o se desconectó: cerrar (queda en reset) y volver a abrir ya
    sh2_close();
    i2c1_recuperar();
    abierto = 0;
    t_intento = ahora - BNO_REINTENTO_MS;
  }
}

bool bno085_ok(void)
{
  uint32_t ahora = HAL_GetTick();
  return abierto && hay_accel && hay_gyro &&
         ahora - t_accel < BNO_DATO_VIGENTE_MS &&
         ahora - t_gyro < BNO_DATO_VIGENTE_MS;
}

void bno085_leer(float a[3], float g[3])
{
  bool ok = bno085_ok();
  for (int i = 0; i < 3; i++)
  {
    a[i] = ok ? accel[i] : 0.0f;
    g[i] = ok ? gyro[i] : 0.0f;
  }
}
