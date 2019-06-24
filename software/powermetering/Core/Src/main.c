/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "lwip.h"
#include "rtc.h"
#include "spi.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"
#include "math.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "httpd.h"
#include "SEGGER_RTT.h"
#include "ADE9000.h"
#include "fs.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void LEDBlink(void) {
  while (1) {
    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    vTaskDelay(100);
  }
}

uint8_t spi_rec_buffer[10];
uint32_t reg2, reg3;
uint32_t link = 0;

extern struct netif gnetif;

void write_ADE9000_16(uint16_t reg_num, uint16_t data) {
  uint16_t tx_reg;
  uint8_t access_reg[4] = {0,0,0xFF,0xFF};

  tx_reg = reg_num;
  tx_reg = (tx_reg << 4);
  tx_reg = (tx_reg & 0xFFF7);

  access_reg[0] = (uint8_t) tx_reg;
  access_reg[1] = (uint8_t) (tx_reg >> 8);
  access_reg[2] = (uint8_t) ((data) & 0x00FF);
  access_reg[3] = (uint8_t) ((data & 0xFF00) >> 8);

  HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(&hspi1, access_reg, 2, 10);
  HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_SET);
}

void write_ADE9000_32(uint16_t reg_num, uint32_t data) {
  uint16_t tx_reg;
  uint8_t access_reg[6] = {0,0,0,0};

  tx_reg = reg_num;
  tx_reg = (tx_reg << 4);
  tx_reg = (tx_reg & 0xFFF7);

  access_reg[0] = (uint8_t) tx_reg;
  access_reg[1] = (uint8_t) (tx_reg >> 8);
  access_reg[2] = (uint8_t) ((data & 0xFF000000) >> 24);
  access_reg[3] = (uint8_t) ((data & 0x00FF0000) >> 16);
  access_reg[5] = (uint8_t) ((data) & 0x000000FF);
  access_reg[4] = (uint8_t) ((data & 0x00000FF00) >> 8);

  HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(&hspi1, access_reg, 2, 10);
  HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_SET);
}

uint8_t arms[10], brms[10], crms[10], uthd[10], ithd[10], hz[10];

void get_ADE9000_data_reg(uint16_t reg_num, uint8_t* arr) {
  uint8_t rx_reg[6] = {0,0,0,0,0,0};
  rx_reg[1] = (uint8_t) (((reg_num << 4) & 0xFF00) >> 8);
  rx_reg[0] = (uint8_t) (((reg_num << 4)+ (1 << 3)) & 0x00FF);
  
  HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_RESET);
//  HAL_SPI_Transmit(&hspi1, access_reg, 2, 10);
//  HAL_SPI_Receive(&hspi1, spi_rec_buffer, 2, 10);
  HAL_SPI_TransmitReceive(&hspi1, rx_reg, arr, 4, 10);
  HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_SET);
}

void get_ADE9000_data(uint16_t reg_num) {
  uint8_t rx_reg[6] = {0,0,0,0,0,0};
  rx_reg[1] = (uint8_t) (((reg_num << 4) & 0xFF00) >> 8);
  rx_reg[0] = (uint8_t) (((reg_num << 4)+ (1 << 3)) & 0x00FF);
  
  HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_RESET);
//  HAL_SPI_Transmit(&hspi1, access_reg, 2, 10);
//  HAL_SPI_Receive(&hspi1, spi_rec_buffer, 2, 10);
  HAL_SPI_TransmitReceive(&hspi1, rx_reg, spi_rec_buffer, 4, 10);
  HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_SET);
}

void SPI_get_data(void) {
  while (1) {
    get_ADE9000_data_reg(ADDR_CWATT, arms);
    get_ADE9000_data_reg(ADDR_CIRMS, brms);
    get_ADE9000_data_reg(ADDR_CPF, crms);
    get_ADE9000_data_reg(ADDR_CVAR, uthd);
    get_ADE9000_data_reg(ADDR_CVA, ithd);
    get_ADE9000_data_reg(ADDR_CPERIOD, hz);

    get_ADE9000_data_reg(ADDR_CVRMS, spi_rec_buffer);

//    get_ADE9000_data(0x801);

    //write_ADE9000_data(ADDR_RUN, 1);

    HAL_ETH_ReadPHYRegister(&heth, PHY_SR, &reg2);
    if ((reg2 & 256))
    {
      if (link == 0)
      {
        link = 1;
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, 1);
        netif_set_up(&gnetif);
        netif_set_link_up(&gnetif);
      }
    }
    else
    {
      link = 0;
      HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, 0);
      netif_set_link_down(&gnetif);
    }
  }
}
char const* TAGCHAR="YOLO";
char const** TAGS=&TAGCHAR;

#define V_PER_BIT (0.707 / 52702092.0)

#define R_SHUNT (18.0 * 2.0) //double the pcb value
#define CUR_PRI 20.0 //primary current
#define CUR_SEC 0.02 //secondary current
#define CUR_TF ((R_SHUNT * CUR_SEC)/CUR_PRI) //volts per amp
#define CUR_CONST (CUR_TF / V_PER_BIT) //amps per bit

#define R_HIGH 800000.0
#define R_LOW 1000.0
#define VOLT_TF ((1 / (R_HIGH + R_LOW)) * R_LOW) //volts per volt
#define VOLT_CONST (VOLT_TF / V_PER_BIT)

#define PWR_CONST (((VOLT_TF * CUR_TF) / (1.0/20694066.0))*2.0)

uint16_t ssi_handler(uint32_t index, char* insert, uint32_t insertlen) {
  if(index == 0) {
    static int count = 0;
    SEGGER_RTT_printf(0, "ssi %d CUR_CONST: %lf\n", count, CUR_CONST);
    int32_t a_rms = (arms[3] << 24) + (arms[2] << 16) + (arms[5] << 8) + arms[4];
    int32_t b_rms = (brms[3] << 24) + (brms[2] << 16) + (brms[5] << 8) + brms[4];
    int32_t c_rms = (crms[3] << 24) + (crms[2] << 16) + (crms[5] << 8) + crms[4];

    int32_t ac_rms = (spi_rec_buffer[3] << 24) + (spi_rec_buffer[2] << 16) + (spi_rec_buffer[5] << 8) + spi_rec_buffer[4];

    int32_t ithd_i = (ithd[3] << 24) + (ithd[2] << 16) + (ithd[5] << 8) + ithd[4];
    int32_t uthd_i = (uthd[3] << 24) + (uthd[2] << 16) + (uthd[5] << 8) + uthd[4];
    int32_t hz_i = (hz[3] << 24) + (hz[2] << 16) + (hz[5] << 8) + hz[4];

    double a_rms_f = a_rms / 118648.6953f;
    double b_rms_f = b_rms / 118648.6953f;
    double c_rms_f = c_rms  * powf(2.0,-27.0);
    double ac_rms_f = ac_rms / VOLT_CONST;
    double cc_rms_f = b_rms / CUR_CONST;
    double wc_rms_f = a_rms / PWR_CONST;
    double ithd_f = ithd_i / PWR_CONST;
    double uthd_f = uthd_i / PWR_CONST;
    double hz_f = (8000.0*powf(2,16))/(hz_i + 1);
    return snprintf(insert, LWIP_HTTPD_MAX_TAG_INSERT_LEN - 2, "U: %.2lfV I: %.2lfA P: %.3lfW S: %.3lfVA  Q: %.3lfvar pf: %.2lf Freq: %.2lfHz cnt: %i", ac_rms_f, cc_rms_f, wc_rms_f, ithd_f, uthd_f, c_rms_f, hz_f, count++);
  }
  return 0;
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
  MX_RTC_Init();
  MX_SPI1_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  MX_LWIP_Init();

  http_set_ssi_handler((tSSIHandler) ssi_handler, (char const **) TAGS, 1);
  httpd_init();

  SEGGER_RTT_Init();
  SEGGER_RTT_printf(0, "yolo\n");

  write_ADE9000_32(ADDR_VLEVEL, 2740646); //magic number™
  write_ADE9000_16(ADDR_RUN, 1);
  write_ADE9000_16(ADDR_EP_CFG, 1 << 0);

  xTaskCreate((TaskFunction_t)LEDBlink, "LED Keepalive", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
  xTaskCreate((TaskFunction_t)SPI_get_data, "Get ADE9000 values", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);

  /* USER CODE END 2 */

  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();
  
  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSE, RCC_MCODIV_1);
  HAL_RCC_MCOConfig(RCC_MCO2, RCC_MCO2SOURCE_HSE, RCC_MCODIV_1);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
