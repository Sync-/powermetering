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
#include "crc.h"
#include "fatfs.h"
#include "i2c.h"
#include "mbedtls.h"
#include "rtc.h"
#include "sdio.h"
#include "spi.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "httpd.h"
#include "SEGGER_RTT.h"
#include "ADE9000.h"
#include "fs.h"
#include "string.h"
#include "mqtt.h"
#include "influx.h"
#include "ade.h"
#include <math.h>
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
void LEDBlink(void)
{
  while (1)
  {
    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    vTaskDelay(100);
  }
}

uint8_t spi_rec_buffer[10];
uint32_t reg2, reg3;
uint32_t link = 0;

extern struct netif gnetif;

void write_ADE9000_16(uint16_t reg_num, uint16_t data)
{
  uint16_t tx_reg;
  uint8_t access_reg[4] = {0, 0, 0xFF, 0xFF};

  tx_reg = reg_num;
  tx_reg = (tx_reg << 4);
  tx_reg = (tx_reg & 0xFFF7);

  access_reg[0] = (uint8_t)tx_reg;
  access_reg[1] = (uint8_t)(tx_reg >> 8);
  access_reg[2] = (uint8_t)((data)&0x00FF);
  access_reg[3] = (uint8_t)((data & 0xFF00) >> 8);

  HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(&hspi1, access_reg, 2, 10);
  HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_SET);
}

void write_ADE9000_32(uint16_t reg_num, uint32_t data)
{
  uint16_t tx_reg;
  uint8_t access_reg[6] = {0, 0, 0, 0};

  tx_reg = reg_num;
  tx_reg = (tx_reg << 4);
  tx_reg = (tx_reg & 0xFFF7);

  access_reg[0] = (uint8_t)tx_reg;
  access_reg[1] = (uint8_t)(tx_reg >> 8);
  access_reg[3] = (uint8_t)((data & 0xFF000000) >> 24);
  access_reg[2] = (uint8_t)((data & 0x00FF0000) >> 16);
  access_reg[5] = (uint8_t)((data & 0x00000FF00) >> 8);
  access_reg[4] = (uint8_t)((data & 0x000000FF));

  HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(&hspi1, access_reg, 3, 10);
  HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_SET);
}

uint8_t arms[10], brms[10], crms[10], uthd[10], ithd[10], hz[10];

void get_ADE9000_data_reg(uint16_t reg_num, uint8_t *arr)
{
  uint8_t rx_reg[6] = {0, 0, 0, 0, 0, 0};
  rx_reg[1] = (uint8_t)(((reg_num << 4) & 0xFF00) >> 8);
  rx_reg[0] = (uint8_t)(((reg_num << 4) | (1 << 3)) & 0x00FF);

  HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_RESET);
  //  HAL_SPI_Transmit(&hspi1, access_reg, 2, 10);
  //  HAL_SPI_Receive(&hspi1, spi_rec_buffer, 2, 10);
  HAL_SPI_TransmitReceive(&hspi1, rx_reg, arr, 4, 10);
  HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_SET);
}

void get_ADE9000_data(uint16_t reg_num)
{
  uint8_t rx_reg[6] = {0, 0, 0, 0, 0, 0};
  rx_reg[1] = (uint8_t)(((reg_num << 4) & 0xFF00) >> 8);
  rx_reg[0] = (uint8_t)(((reg_num << 4) | (1 << 3)) & 0x00FF);

  HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_RESET);
  //  HAL_SPI_Transmit(&hspi1, access_reg, 2, 10);
  //  HAL_SPI_Receive(&hspi1, spi_rec_buffer, 2, 10);
  HAL_SPI_TransmitReceive(&hspi1, rx_reg, spi_rec_buffer, 4, 10);
  HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_SET);
}

uint8_t burst_tx[10];
uint8_t burst_tx_empty[750];

union ade_burst_rx_t foobar;

uint8_t ahz[10], bhz[10], chz[10], status0[10], status1[10];

void SPI_get_data(void)
{
  while (1)
  {
    /*
    get_ADE9000_data_reg(ADDR_CWATT, arms);
    get_ADE9000_data_reg(ADDR_CIRMS, brms);
    get_ADE9000_data_reg(ADDR_CPF, crms);
    get_ADE9000_data_reg(ADDR_CVAR, uthd);
    get_ADE9000_data_reg(ADDR_CVA, ithd);
    get_ADE9000_data_reg(ADDR_CPERIOD, hz);

    get_ADE9000_data_reg(ADDR_CVRMS, spi_rec_buffer);
*/

    get_ADE9000_data_reg(ADDR_APERIOD, ahz);
    get_ADE9000_data_reg(ADDR_APERIOD, bhz);
    get_ADE9000_data_reg(ADDR_APERIOD, chz);

    get_ADE9000_data_reg(ADDR_STATUS0, status0);
    get_ADE9000_data_reg(ADDR_STATUS1, status1);

    burst_tx[1] = (uint8_t)(((0x600 << 4) & 0xFF00) >> 8);
    burst_tx[0] = (uint8_t)(((0x600 << 4) | (1 << 3)) & 0x00FF);
    HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, burst_tx, 1, 10);
    HAL_SPI_TransmitReceive(&hspi1, burst_tx_empty, foobar.bytes, 0x3E * 2, 10);
    HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_SET);

    //    get_ADE9000_data(0x801);

    //write_ADE9000_data(ADDR_RUN, 1);

         vTaskDelay(1);
  }
}

void linktask(void)
{
  extern ETH_HandleTypeDef heth;
  while (1) {
    HAL_ETH_ReadPHYRegister(&heth, PHY_SR, &reg2);
    if ((reg2 & 256))
    {
      if (link == 0)
      {
        link = 1;
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, 1);
        SEGGER_RTT_printf(0, "Link is up\n");
        netif_set_up(&gnetif);
        netif_set_link_up(&gnetif);
      }
    }
    else
    {
      link = 0;
      HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, 0);
      SEGGER_RTT_printf(0, "Link is down\n");
      netif_set_link_down(&gnetif);
    }
    vTaskDelay(100);
  }
}
char const *TAGCHAR = "YOLO";
char const **TAGS = &TAGCHAR;

uint16_t ssi_handler(uint32_t index, char *insert, uint32_t insertlen)
{
  if (index == 0)
  {
    static int count = 0;
    SEGGER_RTT_printf(0, "ssi %d CUR_CONST: %lf\n", count, CUR_CONST);

    int32_t status0_i = (status0[3] << 24) + (status0[2] << 16) + (status0[5] << 8) + status0[4];
    int32_t status1_i = (status1[3] << 24) + (status1[2] << 16) + (status1[5] << 8) + status1[4];

    float avrms = ((foobar.avrms1012_h << 16) + foobar.avrms1012_l) / VOLT_CONST;
    float bvrms = ((foobar.bvrms1012_h << 16) + foobar.bvrms1012_l) / VOLT_CONST;
    float cvrms = ((foobar.cvrms1012_h << 16) + foobar.cvrms1012_l) / VOLT_CONST;
    /*
    if(status1 && (0b1111 << 28)) {
      HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);
      write_ADE9000_16(ADDR_CONFIG1, 1 << 0);
    } else {
      HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET);
    }
*/
    return snprintf(insert, LWIP_HTTPD_MAX_TAG_INSERT_LEN - 2, "STATUS0: %lx STATUS1: %lx cnt: %d <br> L1: %fV L2: %fV L3: %fV", status0_i, status1_i, count++, avrms, bvrms, cvrms);
  }
  return 0;
}

static void mqtt_sub_request_cb(void *arg, err_t result)
{
  /* Just print the result code here for simplicity, 
     normal behaviour would be to take some action if subscribe fails like 
     notifying user, retry subscribe or disconnect from server */
  SEGGER_RTT_printf(0, "Subscribe result: %d\n", result);
}

ip4_addr_t mqtt_server_addr;

void example_do_connect(mqtt_client_t *client);

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
  err_t err;
  SEGGER_RTT_printf(0, "Entered conn_cb...\n");
  if (status == MQTT_CONNECT_ACCEPTED)
  {
    SEGGER_RTT_printf(0, "mqtt_connection_cb: Successfully connected\n");
  }
}

mqtt_client_t static_client;

/* Called when publish is complete either with sucess or failure */
static void mqtt_pub_request_cb(void *arg, err_t result)
{
  if (result != ERR_OK)
  {
    SEGGER_RTT_printf(0, "Publish result: %d\n", result);
  }
}

void example_publish(mqtt_client_t *client, void *arg)
{
  float airms = ((foobar.airms1012_h << 16) + foobar.airms1012_l) / CUR_CONST;
  float birms = ((foobar.birms1012_h << 16) + foobar.birms1012_l) / CUR_CONST;
  float cirms = ((foobar.cirms1012_h << 16) + foobar.cirms1012_l) / CUR_CONST;
  float nirms = ((foobar.nirms1012_h << 16) + foobar.nirms1012_l) / CUR_CONST;

  float avrms = ((foobar.avrms1012_h << 16) + foobar.avrms1012_l) / VOLT_CONST;
  float bvrms = ((foobar.bvrms1012_h << 16) + foobar.bvrms1012_l) / VOLT_CONST;
  float cvrms = ((foobar.cvrms1012_h << 16) + foobar.cvrms1012_l) / VOLT_CONST;

  float avthd = ((foobar.avthd_h << 16) + foobar.avthd_l) * powf(2, -27);
  float bvthd = ((foobar.bvthd_h << 16) + foobar.bvthd_l) * powf(2, -27);
  float cvthd = ((foobar.cvthd_h << 16) + foobar.cvthd_l) * powf(2, -27);

  float aithd = ((foobar.aithd_h << 16) + foobar.aithd_l) * powf(2, -27);
  float bithd = ((foobar.bithd_h << 16) + foobar.bithd_l) * powf(2, -27);
  float cithd = ((foobar.cithd_h << 16) + foobar.cithd_l) * powf(2, -27);

  float apf = ((foobar.apf_h << 16) + foobar.apf_l) * powf(2, -27);
  float bpf = ((foobar.bpf_h << 16) + foobar.bpf_l) * powf(2, -27);
  float cpf = ((foobar.cpf_h << 16) + foobar.cpf_l) * powf(2, -27);

  int32_t ahz_i = (ahz[3] << 24) + (ahz[2] << 16) + (ahz[5] << 8) + ahz[4];
  int32_t bhz_i = (bhz[3] << 24) + (bhz[2] << 16) + (bhz[5] << 8) + bhz[4];
  int32_t chz_i = (chz[3] << 24) + (chz[2] << 16) + (chz[5] << 8) + chz[4];
  float ahz_f = (8000.0f * powf(2.0f, 16.0f)) / (ahz_i + 1.0f);
  float bhz_f = (8000.0f * powf(2.0f, 16.0f)) / (bhz_i + 1.0f);
  float chz_f = (8000.0f * powf(2.0f, 16.0f)) / (chz_i + 1.0f);

  char insert[200];
  //int len = snprintf(insert, 200 - 2, "L1: %.2lfV L2: %.2lfV L3: %.2lfV L1: %.2lfA L2: %.2lfA L3: %.2lfA N: %.2lfA", avrms, bvrms, cvrms, airms, birms, cirms, nirms);
  int len = snprintf(insert, 200 - 2, "%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.3f;%3f;%3f;%3f;%3f;%3f;%3f;%3f;%3f;%.4f;%.4f;%.4f;", avrms, bvrms, cvrms, airms, birms, cirms, nirms, avthd, bvthd, cvthd, aithd, bithd, cithd, apf, bpf, cpf, ahz_f, bhz_f, chz_f);
  err_t err;
  u8_t qos = 2;    /* 0 1 or 2, see MQTT specification */
  u8_t retain = 0; /* No don't retain such crappy payload... */
  err = mqtt_publish(client, "trifasipower", insert, len, qos, retain, mqtt_pub_request_cb, arg);
  HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
  if (err != ERR_OK)
  {
    SEGGER_RTT_printf(0, "Publish err: %d\n", err);
    if(err == -11) {
      example_do_connect(&static_client);
      SEGGER_RTT_printf(0, "Connecting due to error...\n");
    }
  }
}

void example_do_connect(mqtt_client_t *client)
{
  struct mqtt_connect_client_info_t ci;
  err_t err;

  /* Setup an empty client info structure */
  memset(&ci, 0, sizeof(ci));

  /* Minimal amount of information required is client identifier, so set it here */
  ci.client_id = "lwip_test";

  /* Initiate client and connect to server, if this fails immediately an error code is returned
     otherwise mqtt_connection_cb will be called with connection result after attempting 
     to establish a connection with the server. 
     For now MQTT version 3.1.1 is always used */
  SEGGER_RTT_printf(0, "Trying to connect...\n");
  err = mqtt_client_connect(client, &mqtt_server_addr, MQTT_PORT, mqtt_connection_cb, 0, &ci);

  /* For now just print the result code if something goes wrong */
  if (err != ERR_OK)
  {
    SEGGER_RTT_printf(0, "mqtt_connect return %d\n", err);
  }
}

void mqtt_stuff(void)
{
  vTaskDelay(2000);
  IP4_ADDR(&mqtt_server_addr, 192, 168, 178, 121);
  SEGGER_RTT_printf(0, "Entered mqtt...\n");
  example_do_connect(&static_client);
  while (1)
  {
    example_publish(&static_client, 0);
    vTaskDelay(1000);
  }
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
  MX_SDIO_SD_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_CRC_Init();
  /* USER CODE BEGIN 2 */
  MX_LWIP_Init();

  //  http_set_ssi_handler((tSSIHandler) ssi_handler, (char const **) TAGS, 1);
  //  httpd_init();

  SEGGER_RTT_Init();
  SEGGER_RTT_printf(0, "yolo\n");

  write_ADE9000_16(ADDR_CONFIG1, 1 << 0);

  write_ADE9000_32(ADDR_VLEVEL, 2740646); //magic number™
  write_ADE9000_16(ADDR_CONFIG1, 1 << 11);
  write_ADE9000_16(ADDR_PGA_GAIN, 0b0001010101010101);
  write_ADE9000_16(ADDR_RUN, 1);
  write_ADE9000_16(ADDR_EP_CFG, 1 << 0);

  xTaskCreate((TaskFunction_t)LEDBlink, "LED Keepalive", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, NULL);
  xTaskCreate((TaskFunction_t)SPI_get_data, "Get ADE9000 values", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, NULL);
  xTaskCreate((TaskFunction_t)linktask, "Handle the Ethernet link status", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
  //xTaskCreate((TaskFunction_t)mqtt_stuff, "Do MQTT stuff", 1024, NULL, configMAX_PRIORITIES - 3, NULL);
  influx_init();
  fwupdate_init();

  SEGGER_RTT_printf(0, "Tasks running\n");

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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE;
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
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
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
  if (htim->Instance == TIM1)
  {
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

#ifdef USE_FULL_ASSERT
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
