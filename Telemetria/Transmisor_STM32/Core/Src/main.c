/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bme280.h"
#include "bno085.h"
#include "leds.h"
#include "supercap.h"
#include "sx1278.h"
#include "telemetry.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BME_PERIODO_MS      50    // 20 Hz: alimenta la altitud y la lógica del supercapacitor
#define BME_FALLOS_MAX      20    // 1 s fallando seguido: se reinicia el BME280
#define BME_REINTENTO_MS    1000
#define RADIO_REINTENTO_MS  2000
#define TX_PERIODO_MS       500   // un paquete LoRa cada 500 ms (~65 ms en el aire)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */

// Variables para ver en vivo con el depurador (ST-Link)
volatile uint8_t  radio_ok = 0;
volatile uint8_t  bme_ok = 0;
volatile uint32_t tx_count = 0;
volatile uint32_t tx_omitidos = 0;   // la radio seguía ocupada al tocar enviar

static bme280_dato_t bme_dato;
static uint8_t  bme_iniciado = 0;
static uint8_t  bme_fallos = 0;
static supercap_t vuelo;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static uint8_t estado_actual(void)
{
  uint8_t e = 0;
  if (bme_ok) e |= EST_BME_OK;
  if (bno085_ok()) e |= EST_BNO_OK;
  if (vuelo.fase != SC_REFERENCIA) e |= EST_REF_OK;
  if (vuelo.fase == SC_ARMADO || vuelo.fase == SC_ACTIVADO) e |= EST_ARMADO;
  if (vuelo.fase == SC_ACTIVADO) e |= EST_SUPERCAP;
  return e;
}

static led_modo_t modo_led(void)
{
  if (!radio_ok) return LED_FALLA_RADIO;
  if (!bme_ok || !bno085_ok()) return LED_FALLA_SENSOR;
  return LED_OK;
}

void i2c1_recuperar(void)
{
  HAL_I2C_DeInit(&hi2c1);
  __HAL_RCC_I2C1_FORCE_RESET();
  __HAL_RCC_I2C1_RELEASE_RESET();
  MX_I2C1_Init();
}

static void leer_bme(uint32_t ahora)
{
  static uint32_t t_init = 0;

  if (!bme_iniciado)
  {
    bme_ok = 0;
    if (ahora - t_init >= BME_REINTENTO_MS)
    {
      t_init = ahora;
      i2c1_recuperar();
      bme_iniciado = bme280_init();
      bme_fallos = 0;
    }
    return;
  }

  if (!bme280_leer(&bme_dato))
  {
    bme_ok = 0;
    if (++bme_fallos >= BME_FALLOS_MAX)
    {
      bme_iniciado = 0;
      t_init = ahora - BME_REINTENTO_MS;  // reintentar ya
    }
    return;
  }

  bme_ok = 1;
  bme_fallos = 0;

  if (supercap_actualizar(&vuelo, bme_dato.presion))
  {
    // Activación del supercapacitor: queda encendido hasta reiniciar la placa
    HAL_GPIO_WritePin(SUPERCAP_GPIO_Port, SUPERCAP_Pin, GPIO_PIN_SET);
  }
}

static void enviar_telemetria(void)
{
  TelemetryDatos d = {0};

  d.t_ms = HAL_GetTick();
  d.estado = estado_actual();
  if (bme_ok)
  {
    d.temperatura = bme_dato.temperatura;
    d.presion = bme_dato.presion;
    d.humedad = bme_dato.humedad;
  }
  d.altitud_m = vuelo.altitud_m;
  bno085_leer(d.accel, d.gyro);

  TelemetryPacketLoRa protocolLoRa;
  telemetry_build_LoRa(&protocolLoRa, &d);

  if (sx1278_send((const uint8_t*)&protocolLoRa, sizeof(protocolLoRa)))
    tx_count++;
  else
    tx_omitidos++;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  leds_servicio(LED_ARRANCANDO, 0);
  supercap_init(&vuelo);

  radio_ok = sx1278_init();
  bme_iniciado = bme280_init();
  bno085_iniciar();

  uint32_t t_bme = HAL_GetTick();
  uint32_t t_tx = t_bme;
  uint32_t t_radio = t_bme;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint32_t ahora = HAL_GetTick();

    bno085_servicio();
    sx1278_servicio();

    if (ahora - t_bme >= BME_PERIODO_MS)
    {
      t_bme = ahora;
      leer_bme(ahora);
    }

    if (!radio_ok && ahora - t_radio >= RADIO_REINTENTO_MS)
    {
      t_radio = ahora;
      radio_ok = sx1278_init();
    }

    if (ahora - t_tx >= TX_PERIODO_MS)
    {
      t_tx += TX_PERIODO_MS;
      if (ahora - t_tx >= TX_PERIODO_MS) t_tx = ahora;  // se atrasó: sin ráfagas
      if (radio_ok) enviar_telemetria();
    }

    leds_servicio(modo_led(), ahora);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_ESTADO_GPIO_Port, LED_ESTADO_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LORA_RST_Pin|LORA_NSS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED_TX_Pin|SUPERCAP_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BNO_RST_GPIO_Port, BNO_RST_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : LED_ESTADO_Pin */
  GPIO_InitStruct.Pin = LED_ESTADO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_ESTADO_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LORA_RST_Pin LORA_NSS_Pin */
  GPIO_InitStruct.Pin = LORA_RST_Pin|LORA_NSS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LORA_DIO0_Pin */
  GPIO_InitStruct.Pin = LORA_DIO0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(LORA_DIO0_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_TX_Pin SUPERCAP_Pin BNO_RST_Pin */
  GPIO_InitStruct.Pin = LED_TX_Pin|SUPERCAP_Pin|BNO_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : BNO_INT_Pin */
  GPIO_InitStruct.Pin = BNO_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BNO_INT_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == LORA_DIO0_Pin) {
    sx1278_onDio0Irq();
  }
}

// printf sale por SWO (ITM) si el depurador lo habilita; si no, se descarta.
// Sin esto, syscalls.c saltaría a la dirección 0 al primer printf.
int __io_putchar(int ch)
{
  ITM_SendChar((uint32_t)ch);
  return ch;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* Error fatal: LED de estado y LED de TX fijos */
  __disable_irq();
  HAL_GPIO_WritePin(LED_ESTADO_GPIO_Port, LED_ESTADO_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_TX_GPIO_Port, LED_TX_Pin, GPIO_PIN_SET);
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
