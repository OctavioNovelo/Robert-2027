/*
 * sx1278.c
 *
 *  Created on: Mar 14, 2026
 *      Author: shuten-doji
 */
#include "main.h"
#include "sx1278.h"

extern SPI_HandleTypeDef hspi1;

#define SX1278_CS_PORT LORA_NSS_GPIO_Port
#define SX1278_CS_PIN  LORA_NSS_Pin

#define SX1278_RESET_PORT LORA_RST_GPIO_Port
#define SX1278_RESET_PIN  LORA_RST_Pin

// Registros (modo LoRa)
#define REG_FIFO           0x00
#define REG_OP_MODE        0x01
#define REG_FRF_MSB        0x06
#define REG_FRF_MID        0x07
#define REG_FRF_LSB        0x08
#define REG_PA_CONFIG      0x09
#define REG_LNA            0x0C
#define REG_FIFO_ADDR_PTR  0x0D
#define REG_FIFO_TX_BASE   0x0E
#define REG_FIFO_RX_BASE   0x0F
#define REG_IRQ_FLAGS      0x12
#define REG_MODEM_CONFIG1  0x1D
#define REG_MODEM_CONFIG2  0x1E
#define REG_PREAMBLE_MSB   0x20
#define REG_PREAMBLE_LSB   0x21
#define REG_PAYLOAD_LEN    0x22
#define REG_MODEM_CONFIG3  0x26
#define REG_SYNC_WORD      0x39
#define REG_DIO_MAPPING1   0x40
#define REG_VERSION        0x42

#define MODO_LORA      0x80
#define MODO_SLEEP     0x00
#define MODO_STANDBY   0x01
#define MODO_TX        0x03

#define IRQ_TX_DONE    0x08
#define SX1278_VERSION 0x12
#define TX_TIMEOUT_MS  500   // un paquete de 32 bytes tarda ~65 ms en el aire

// Para verlos con el depurador
volatile uint8_t  sx1278_version = 0;
volatile uint32_t sx1278_tx_timeouts = 0;

static volatile uint8_t tx_done_irq = 0;
static uint8_t  ocupado = 0;
static uint32_t t_inicio_tx = 0;

static void cs_low()
{
    HAL_GPIO_WritePin(SX1278_CS_PORT, SX1278_CS_PIN, GPIO_PIN_RESET);
}

static void cs_high()
{
    HAL_GPIO_WritePin(SX1278_CS_PORT, SX1278_CS_PIN, GPIO_PIN_SET);
}

static void led_tx(int encendido)
{
    HAL_GPIO_WritePin(LED_TX_GPIO_Port, LED_TX_Pin, encendido ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void sx1278_write(uint8_t reg, uint8_t value)
{
    uint8_t buffer[2];

    buffer[0] = reg | 0x80;
    buffer[1] = value;

    cs_low();
    HAL_SPI_Transmit(&hspi1, buffer, 2, 100);
    cs_high();
}

uint8_t sx1278_read(uint8_t reg)
{
    uint8_t tx[2] = { reg & 0x7F, 0x00 };
    uint8_t rx[2] = { 0, 0 };

    cs_low();
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2, 100);
    cs_high();

    return rx[1];
}

// Escribe todo el payload en una sola ráfaga SPI
static void escribir_fifo(const uint8_t *data, uint8_t len)
{
    uint8_t dir = REG_FIFO | 0x80;

    cs_low();
    HAL_SPI_Transmit(&hspi1, &dir, 1, 100);
    HAL_SPI_Transmit(&hspi1, (uint8_t*)data, len, 100);
    cs_high();
}

void sx1278_reset()
{
    HAL_GPIO_WritePin(SX1278_RESET_PORT, SX1278_RESET_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);

    HAL_GPIO_WritePin(SX1278_RESET_PORT, SX1278_RESET_PIN, GPIO_PIN_SET);
    HAL_Delay(10);
}

bool sx1278_init(void)
{
    ocupado = 0;
    led_tx(0);
    sx1278_reset();

    sx1278_version = sx1278_read(REG_VERSION);
    if (sx1278_version != SX1278_VERSION) return false;

    // LoRa solo se puede elegir en sleep
    sx1278_write(REG_OP_MODE, MODO_LORA | MODO_SLEEP);
    HAL_Delay(10);
    sx1278_write(REG_OP_MODE, MODO_LORA | MODO_STANDBY);

    // Frf = f * 2^19 / 32 MHz
    uint32_t frf = (uint32_t)(((uint64_t)SX1278_FREQ_HZ << 19) / 32000000UL);
    sx1278_write(REG_FRF_MSB, (uint8_t)(frf >> 16));
    sx1278_write(REG_FRF_MID, (uint8_t)(frf >> 8));
    sx1278_write(REG_FRF_LSB, (uint8_t)frf);

    // PA_BOOST, 17 dBm
    sx1278_write(REG_PA_CONFIG, 0x8F);

    // LNA
    sx1278_write(REG_LNA, 0x23);

    // FIFO base
    sx1278_write(REG_FIFO_TX_BASE, 0x00);
    sx1278_write(REG_FIFO_RX_BASE, 0x00);

    // BW 125kHz, CR 4/5, header explícito | SF7, CRC activado | AGC automático
    sx1278_write(REG_MODEM_CONFIG1, 0x72);
    sx1278_write(REG_MODEM_CONFIG2, 0x74);
    sx1278_write(REG_MODEM_CONFIG3, 0x04);

    // preamble 8
    sx1278_write(REG_PREAMBLE_MSB, 0x00);
    sx1278_write(REG_PREAMBLE_LSB, 0x08);

    // sync word
    sx1278_write(REG_SYNC_WORD, 0x34);

    // DIO0 = TX_DONE
    sx1278_write(REG_DIO_MAPPING1, 0x40);

    sx1278_write(REG_IRQ_FLAGS, 0xFF);
    return true;
}

bool sx1278_send(const uint8_t *data, uint8_t len)
{
    if (ocupado) return false;

    // standby
    sx1278_write(REG_OP_MODE, MODO_LORA | MODO_STANDBY);

    // limpiar IRQ flags (baja DIO0)
    sx1278_write(REG_IRQ_FLAGS, 0xFF);

    // FIFO ptr
    sx1278_write(REG_FIFO_ADDR_PTR, 0x00);

    // escribir payload
    escribir_fifo(data, len);

    // tamaño payload
    sx1278_write(REG_PAYLOAD_LEN, len);

    tx_done_irq = 0;
    ocupado = 1;
    t_inicio_tx = HAL_GetTick();
    led_tx(1);

    // modo TX
    sx1278_write(REG_OP_MODE, MODO_LORA | MODO_TX);
    return true;
}

static void terminar_tx(void)
{
    sx1278_write(REG_IRQ_FLAGS, 0xFF);
    ocupado = 0;
    led_tx(0);
}

void sx1278_servicio(void)
{
    if (!ocupado) return;

    // DIO0 avisa por interrupción; leer el registro cubre el caso de que DIO0 no llegue
    if (tx_done_irq || (sx1278_read(REG_IRQ_FLAGS) & IRQ_TX_DONE))
    {
        terminar_tx();
    }
    else if (HAL_GetTick() - t_inicio_tx >= TX_TIMEOUT_MS)
    {
        sx1278_write(REG_OP_MODE, MODO_LORA | MODO_STANDBY);
        sx1278_tx_timeouts++;
        terminar_tx();
    }
}

bool sx1278_ocupado(void)
{
    return ocupado;
}

// Desde HAL_GPIO_EXTI_Callback (DIO0 = TX_DONE)
void sx1278_onDio0Irq(void)
{
    tx_done_irq = 1;
    led_tx(0);
}
