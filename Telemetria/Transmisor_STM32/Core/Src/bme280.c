/*
 * bme280.c
 *
 * Movido desde main.c. Configuración pensada para altimetría:
 * presión x8, temperatura x1, humedad x1, filtro IIR x4, modo normal (~40 Hz).
 */
#include <string.h>
#include "main.h"
#include "bme280.h"

extern I2C_HandleTypeDef hi2c1;

#define BME280_ADDR_A  0x76
#define BME280_ADDR_B  0x77
#define BME280_CHIP_ID 0x60

// BME280 registers
#define REG_ID        0xD0
#define REG_RESET     0xE0
#define REG_CTRL_HUM  0xF2
#define REG_STATUS    0xF3
#define REG_CTRL_MEAS 0xF4
#define REG_CONFIG    0xF5
#define REG_PRESS_MSB 0xF7

#define ADC_SIN_MEDIR 0x80000  // valor de reset: aún no hay medición

typedef struct {
  // calibration data
  uint16_t dig_T1; int16_t dig_T2, dig_T3;
  uint16_t dig_P1; int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
  uint8_t dig_H1; int16_t dig_H2; uint8_t dig_H3; int16_t dig_H4, dig_H5; int8_t dig_H6;
  int32_t t_fine;
} bme280_cal_t;

static bme280_cal_t bme;
static uint8_t bme_addr = 0;

// ---------- I2C helpers ----------
static HAL_StatusTypeDef i2c_wr(uint8_t addr7, uint8_t reg, uint8_t *data, uint16_t len) {
  uint8_t buf[1 + 32];
  if (len > 32) return HAL_ERROR;
  buf[0] = reg;
  if (len) memcpy(&buf[1], data, len);
  return HAL_I2C_Master_Transmit(&hi2c1, (addr7 << 1), buf, 1 + len, 100);
}

static HAL_StatusTypeDef i2c_rd(uint8_t addr7, uint8_t reg, uint8_t *data, uint16_t len) {
  if (HAL_I2C_Master_Transmit(&hi2c1, (addr7 << 1), &reg, 1, 100) != HAL_OK) return HAL_ERROR;
  return HAL_I2C_Master_Receive(&hi2c1, (addr7 << 1), data, len, 100);
}

static HAL_StatusTypeDef wr8(uint8_t reg, uint8_t v) {
  return i2c_wr(bme_addr, reg, &v, 1);
}

static int bme280_read_calib(void) {
  uint8_t buf[26];

  // temp + pressure calib: 0x88..0xA1 (26 bytes)
  if (i2c_rd(bme_addr, 0x88, buf, 26) != HAL_OK) return 0;
  bme.dig_T1 = (uint16_t)(buf[1]<<8 | buf[0]);
  bme.dig_T2 = (int16_t)(buf[3]<<8 | buf[2]);
  bme.dig_T3 = (int16_t)(buf[5]<<8 | buf[4]);

  bme.dig_P1 = (uint16_t)(buf[7]<<8 | buf[6]);
  bme.dig_P2 = (int16_t)(buf[9]<<8 | buf[8]);
  bme.dig_P3 = (int16_t)(buf[11]<<8 | buf[10]);
  bme.dig_P4 = (int16_t)(buf[13]<<8 | buf[12]);
  bme.dig_P5 = (int16_t)(buf[15]<<8 | buf[14]);
  bme.dig_P6 = (int16_t)(buf[17]<<8 | buf[16]);
  bme.dig_P7 = (int16_t)(buf[19]<<8 | buf[18]);
  bme.dig_P8 = (int16_t)(buf[21]<<8 | buf[20]);
  bme.dig_P9 = (int16_t)(buf[23]<<8 | buf[22]);
  bme.dig_H1 = buf[25];

  // humidity calib: 0xE1..0xE7 (7 bytes)
  uint8_t h[7];
  if (i2c_rd(bme_addr, 0xE1, h, 7) != HAL_OK) return 0;
  bme.dig_H2 = (int16_t)(h[1]<<8 | h[0]);
  bme.dig_H3 = h[2];
  bme.dig_H4 = (int16_t)((h[3] << 4) | (h[4] & 0x0F));
  bme.dig_H5 = (int16_t)((h[5] << 4) | (h[4] >> 4));
  bme.dig_H6 = (int8_t)h[6];

  return 1;
}

static uint8_t bme280_buscar(void) {
  const uint8_t dirs[] = { BME280_ADDR_A, BME280_ADDR_B };
  for (unsigned i = 0; i < sizeof(dirs); i++) {
    uint8_t id = 0;
    if (i2c_rd(dirs[i], REG_ID, &id, 1) == HAL_OK && id == BME280_CHIP_ID) return dirs[i];
  }
  return 0;
}

bool bme280_init(void) {
  bme_addr = bme280_buscar();
  if (!bme_addr) return false;

  // Reset por software y esperar a que copie la calibración a sus registros
  if (wr8(REG_RESET, 0xB6) != HAL_OK) return false;
  HAL_Delay(5);
  uint8_t st = 0x01;
  for (int i = 0; i < 10 && (st & 0x01); i++) {
    HAL_Delay(2);
    if (i2c_rd(bme_addr, REG_STATUS, &st, 1) != HAL_OK) return false;
  }

  if (!bme280_read_calib()) return false;

  // ctrl_hum debe escribirse antes que ctrl_meas; config solo se aplica en sleep
  if (wr8(REG_CTRL_HUM, 0x01) != HAL_OK) return false;                      // humedad x1
  if (wr8(REG_CONFIG, (0 << 5) | (2 << 2)) != HAL_OK) return false;         // standby 0.5 ms, IIR x4
  if (wr8(REG_CTRL_MEAS, (1 << 5) | (4 << 2) | 3) != HAL_OK) return false;  // temp x1, pres x8, normal

  return true;
}

// compensation (fixed-point) -> returns:
// temp in 0.01°C, pressure in Pa, humidity in 0.001%RH
static int32_t bme280_comp_T(int32_t adc_T) {
  int32_t var1, var2;
  var1 = ((((adc_T>>3) - ((int32_t)bme.dig_T1<<1))) * ((int32_t)bme.dig_T2)) >> 11;
  var2 = (((((adc_T>>4) - ((int32_t)bme.dig_T1)) * ((adc_T>>4) - ((int32_t)bme.dig_T1))) >> 12) *
          ((int32_t)bme.dig_T3)) >> 14;
  bme.t_fine = var1 + var2;
  return (bme.t_fine * 5 + 128) >> 8;
}

static uint32_t bme280_comp_P(int32_t adc_P) {
  int64_t var1, var2, p;
  var1 = ((int64_t)bme.t_fine) - 128000;
  var2 = var1 * var1 * (int64_t)bme.dig_P6;
  var2 = var2 + ((var1*(int64_t)bme.dig_P5)<<17);
  var2 = var2 + (((int64_t)bme.dig_P4)<<35);
  var1 = ((var1 * var1 * (int64_t)bme.dig_P3)>>8) + ((var1 * (int64_t)bme.dig_P2)<<12);
  var1 = (((((int64_t)1)<<47) + var1)) * ((int64_t)bme.dig_P1) >> 33;

  if (var1 == 0) return 0; // avoid exception

  p = 1048576 - adc_P;
  p = (((p<<31) - var2) * 3125) / var1;
  var1 = (((int64_t)bme.dig_P9) * (p>>13) * (p>>13)) >> 25;
  var2 = (((int64_t)bme.dig_P8) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((int64_t)bme.dig_P7)<<4);
  return (uint32_t)(p >> 8); // Pa
}

static uint32_t bme280_comp_H(int32_t adc_H) {
  int32_t v_x1;
  v_x1 = (bme.t_fine - ((int32_t)76800));
  v_x1 = (((((adc_H << 14) - (((int32_t)bme.dig_H4) << 20) - (((int32_t)bme.dig_H5) * v_x1)) + ((int32_t)16384)) >> 15) *
          (((((((v_x1 * ((int32_t)bme.dig_H6)) >> 10) * (((v_x1 * ((int32_t)bme.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) *
             ((int32_t)bme.dig_H2) + 8192) >> 14));
  v_x1 = (v_x1 - (((((v_x1 >> 15) * (v_x1 >> 15)) >> 7) * ((int32_t)bme.dig_H1)) >> 4));
  v_x1 = (v_x1 < 0 ? 0 : v_x1);
  v_x1 = (v_x1 > 419430400 ? 419430400 : v_x1);
  return (uint32_t)(v_x1 >> 12); // 0.001%RH
}

bool bme280_leer(bme280_dato_t *dato) {
  uint8_t d[8];
  if (!bme_addr) return false;
  if (i2c_rd(bme_addr, REG_PRESS_MSB, d, 8) != HAL_OK) return false;

  int32_t adc_P = (int32_t)((d[0]<<12) | (d[1]<<4) | (d[2]>>4));
  int32_t adc_T = (int32_t)((d[3]<<12) | (d[4]<<4) | (d[5]>>4));
  int32_t adc_H = (int32_t)((d[6]<<8) | d[7]);
  if (adc_P == ADC_SIN_MEDIR || adc_T == ADC_SIN_MEDIR) return false;

  int32_t t = bme280_comp_T(adc_T);   // primero: calcula t_fine
  uint32_t p = bme280_comp_P(adc_P);
  if (p < 30000 || p > 110000) return false;

  dato->temperatura = t;
  dato->presion = p;
  dato->humedad = (bme280_comp_H(adc_H) + 5) / 10;  // 0.01%RH
  return true;
}
