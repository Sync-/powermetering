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
#include "i2c.h"
#include "rtc.h"
#include "sdio.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "httpd.h"
#include "def.h"
#include "config.h"
#include "SEGGER_RTT.h"
#include "ADE9000.h"
#include "fs.h"
#include "string.h"
#include "influx.h"
#include "ade.h"
#include "fwupdate.h"
#include "lwip.h"
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
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//rtos heap in ccm
uint8_t ucHeap[configTOTAL_HEAP_SIZE] __attribute__((section(".ccmram")));

//check if malloc locks can be overwritten, do not remove this check
#ifndef _RETARGETABLE_LOCKING
#error "newlib must be configured with --enable-newlib-retargetable-locking"
#endif

void __malloc_lock(struct _reent *r)
{
    vTaskSuspendAll();
}


void __malloc_unlock(struct _reent *r)
{
    xTaskResumeAll();
}


void LEDBlink(void)
{
  while (1)
  {
    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    vTaskDelay(100);
  }
}

void linktask(void)
{
  uint32_t reg;
  uint32_t link = 0;
  extern struct netif gnetif;
  extern ETH_HandleTypeDef heth;
  while (1) {
    HAL_ETH_ReadPHYRegister(&heth, PHY_SR, &reg);
    if ((reg & 256))
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

//static void *current_connection;
//static void *valid_connection;

err_t httpd_post_begin(void *connection, const char *uri, const char *http_request,
                 u16_t http_request_len, int content_len, char *response_uri,
                 u16_t response_uri_len, u8_t *post_auto_wnd)
{
  LWIP_UNUSED_ARG(connection);
  LWIP_UNUSED_ARG(http_request);
  LWIP_UNUSED_ARG(http_request_len);
  LWIP_UNUSED_ARG(content_len);
  LWIP_UNUSED_ARG(post_auto_wnd);
  SEGGER_RTT_printf(0, "httpd_post_begin: %s %i\n", uri, content_len);

  return ERR_OK;
}

err_t httpd_post_receive_data(void *connection, struct pbuf *p)
{
  SEGGER_RTT_printf(0, "httpd_post_receive_data: %u %s\n", p->tot_len,p->payload);
  config_write(p->payload,p->tot_len);
  return ERR_OK;
}

void httpd_post_finished(void *connection, char *response_uri, u16_t response_uri_len)
{
  SEGGER_RTT_printf(0, "httpd_post_finished\n");
}



char* tags[] = {"STAT","CONFIG","NAME", "PKTCNT", "PHASOR", "VERSION", "RESET"};

uint16_t ssi_handler(uint32_t index, char *insert, uint32_t insertlen)
{
 if(index == 0){//STAT
    uint32_t len = 0;
    len += snprintf(insert + len, LWIP_HTTPD_MAX_TAG_INSERT_LEN - len - 1,"vTaskGetRunTimeStats\n");
    vTaskGetRunTimeStats(insert + len);
    len += strlen(insert);
    len += snprintf(insert + len, LWIP_HTTPD_MAX_TAG_INSERT_LEN - len - 1,"\n\nvTaskList\n");
    vTaskList(insert + len);
    len += strlen(insert);
    len += snprintf(insert + len, LWIP_HTTPD_MAX_TAG_INSERT_LEN - len - 1,"\n\nxTaskGetTickCount() %lu\n",xTaskGetTickCount());
    return len;
    //return snprintf(insert, LWIP_HTTPD_MAX_TAG_INSERT_LEN - 2, "I bims vong terst inne tag 2 her");
  }else if(index == 1){//CONFIG
    return config_read(insert, LWIP_HTTPD_MAX_TAG_INSERT_LEN);
  }else if(index == 2){//NAME
    config_get_string("name", insert);
    return strnlen(insert, LWIP_HTTPD_MAX_TAG_INSERT_LEN);
  }
  else if (index == 3)
  { //PACKETCOUNTER
    return snprintf(insert, LWIP_HTTPD_MAX_TAG_INSERT_LEN - 2, "IP packet counter:\nMMCTGFCR: %u\nMMCTGFMSCCR: %u\nMCTGFSCCR: %u\nMMCRGUFCR: %u\nMMCRFAECR: %u\nMMCRFCECR: %u \nxPortGetFreeHeapSize: %d\nxPortGetMinimumEverFreeHeapSize: %d", ETH->MMCTGFCR, ETH->MMCTGFMSCCR, ETH->MMCTGFSCCR, ETH->MMCRGUFCR, ETH->MMCRFAECR, ETH->MMCRFCECR, xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
  }
  else if (index == 4)
  { //PHASOR
    extern struct ade_float_t ade_f;
    int len =  snprintf(insert, LWIP_HTTPD_MAX_TAG_INSERT_LEN - 2,
                    "{\"status\": [%d,%d], "
                    "\"U\": {\"L1\": %.2f, \"L2\": %.2f, \"L3\": %.2f, \"L1(one)\": %.2f, \"L2(one)\": %.2f, \"L3(one)\": %.2f, \"L1(1012)\": %.2f, \"L2(1012)\": %.2f, \"L3(1012)\": %.2f}, "
                    "\"ang_U\":{ \"L2\":%.2f, \"L3\": %.2f}, "
                    "\"ang_I\":{ \"L1\":%.2f, \"L2\": %.2f, \"L3\": %.2f}, "
                    "\"I\": {\"L1\": %.2f, \"L2\": %.2f, \"L3\": %.2f, \"N\": %.2f, \"L1(one)\": %.2f, \"L2(one)\": %.2f, \"L3(one)\": %.2f, \"N(one)\": %.2f, \"L1(1012)\": %.2f, \"L2(1012)\": %.2f, \"L3(1012)\": %.2f, \"N(1012)\": %.2f, \"ISUMRMS\": %.4f},"
                    "\"P\": {\"L1\": %.2f, \"L2\": %.2f, \"L3\": %.2f, \"L1f\": %.2f, \"L2f\": %.2f, \"L3f\": %.2f},"
                    "\"Q\": {\"L1\": %.2f, \"L2\": %.2f, \"L3\": %.2f, \"L1f\": %.2f, \"L2f\": %.2f, \"L3f\": %.2f},"
                    "\"S\": {\"L1\": %.2f, \"L2\": %.2f, \"L3\": %.2f, \"L1f\": %.2f, \"L2f\": %.2f, \"L3f\": %.2f}"
                    //"\"pf\": {\"L1\": %.2f, \"L2\": %.2f, \"L3\": %.2f},"
                    "}",
                    ade_f.status0, ade_f.status1,
                    ade_f.avrms, ade_f.bvrms, ade_f.cvrms, ade_f.avrmsone, ade_f.bvrmsone, ade_f.cvrmsone, ade_f.avrms1012, ade_f.bvrms1012, ade_f.cvrms1012,
                    ade_f.angl_va_vb, ade_f.angl_va_vc,
                    ade_f.angl_va_ia, ade_f.angl_vb_ib, ade_f.angl_vc_ic,
                    ade_f.airms, ade_f.birms, ade_f.cirms, ade_f.nirms, ade_f.airmsone, ade_f.birmsone, ade_f.cirmsone, ade_f.nirmsone, ade_f.airms1012, ade_f.birms1012, ade_f.cirms1012, ade_f.nirms1012, ade_f.isumrms,
                    ade_f.awatt, ade_f.bwatt, ade_f.cwatt, ade_f.afwatt, ade_f.bfwatt, ade_f.cfwatt,
                    ade_f.avar, ade_f.bvar, ade_f.cvar, ade_f.afvar, ade_f.bfvar, ade_f.cfvar,
                    ade_f.ava, ade_f.bva, ade_f.cva, ade_f.afva, ade_f.bfva, ade_f.cfva
                    //ade_f.apf, ade_f.bpf, ade_f.cpf
                    );
      return len;
  } else if(index == 5) {//VERSION
    extern volatile const version_info_t version_info_stmbl;
    uint32_t len = 0;
    len += snprintf(insert + len, LWIP_HTTPD_MAX_TAG_INSERT_LEN - len - 1,"%s v%i.%i.%i %s\n",
      version_info_stmbl.product_name,
      version_info_stmbl.major,
      version_info_stmbl.minor,
      version_info_stmbl.patch,
      version_info_stmbl.git_version);
    len += snprintf(insert + len, LWIP_HTTPD_MAX_TAG_INSERT_LEN - len - 1,"Branch %s\n", version_info_stmbl.git_branch);
    len += snprintf(insert + len, LWIP_HTTPD_MAX_TAG_INSERT_LEN - len - 1,"Compiled %s %s ", version_info_stmbl.build_date, version_info_stmbl.build_time);
    len += snprintf(insert + len, LWIP_HTTPD_MAX_TAG_INSERT_LEN - len - 1,"by %s on %s\n", version_info_stmbl.build_user, version_info_stmbl.build_host);
    len += snprintf(insert + len, LWIP_HTTPD_MAX_TAG_INSERT_LEN - len - 1,"GCC        %s\n", __VERSION__);
    len += snprintf(insert + len, LWIP_HTTPD_MAX_TAG_INSERT_LEN - len - 1,"newlib     %s\n", _NEWLIB_VERSION);
    len += snprintf(insert + len, LWIP_HTTPD_MAX_TAG_INSERT_LEN - len - 1,"size: %lu crc:%lx\n", version_info_stmbl.image_size, version_info_stmbl.image_crc);
    return len;
  } else if(index == 6) {//RESET
    HAL_NVIC_SystemReset();
    return 0;
  }

  return 0;
}

void vApplicationStackOverflowHook(TaskHandle_t *pxTask, char *pcTaskName)
{
    taskDISABLE_INTERRUPTS();
    SEGGER_RTT_printf(0, "StackOverflow in %s\n",pcTaskName);
    for(;;);
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

  //debug: 95,96,97,98
  //pb8, pb9,pe0, pe1
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN 2 */
  MX_LWIP_Init();
  ETH->MMCRIMR |= ETH_MMCRIMR_RGUFM | ETH_MMCRIMR_RFAEM   | ETH_MMCRIMR_RFCEM;
  ETH->MMCTIMR |= ETH_MMCTIMR_TGFM  | ETH_MMCTIMR_TGFMSCM | ETH_MMCTIMR_TGFSCM;

  http_set_ssi_handler((tSSIHandler) ssi_handler, (char const **)tags, LWIP_ARRAYSIZE(tags));
  httpd_init();

  SEGGER_RTT_Init();
  SEGGER_RTT_printf(0, "yolo\n");

  xTaskCreate((TaskFunction_t)linktask, "linktask", configMINIMAL_STACK_SIZE * 2, NULL, configMAX_PRIORITIES - 1, NULL);
  fwupdate_init();

  xTaskCreate((TaskFunction_t)LEDBlink, "LED Keepalive", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, NULL);
  xTaskCreate((TaskFunction_t)SPI_get_data, "Get ADE values", configMINIMAL_STACK_SIZE * 2, NULL, configMAX_PRIORITIES - 2, NULL);
  influx_init();


  SEGGER_RTT_printf(0, "Tasks running\n");

  /* USER CODE END 2 */

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
