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
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "version_stmbl.h"
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
CRC_HandleTypeDef hcrc;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define APP_ADDRESS 0x08020000
#define FW_TMP_ADDRESS 0x08080000
//#define FW_SECTOR FLASH_SECTOR_7
#define FW_TMP_SIZE 128*1024
#define VERSION_INFO_OFFSET 0x188

uint32_t crc_tmp = 1;
uint32_t crc_app = 1;
HAL_StatusTypeDef flash_ret;

const version_info_t *tmp_fw_info = (void *)(FW_TMP_ADDRESS + VERSION_INFO_OFFSET);
const uint8_t *fw_tmp = (char *)FW_TMP_ADDRESS;

const version_info_t *fw_info = (void *)(APP_ADDRESS + VERSION_INFO_OFFSET);
const uint8_t *fw = (char *)APP_ADDRESS;
volatile uint8_t byte;
volatile uint32_t addr;

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
  MX_CRC_Init();
  /* USER CODE BEGIN 2 */

if(tmp_fw_info->image_size < 256*1024){
  crc_tmp =  HAL_CRC_Calculate(&hcrc, (uint32_t*)fw_tmp, tmp_fw_info->image_size / 4);
  if(crc_tmp == 0){
    HAL_FLASH_Unlock();

    //delete old firmware
    uint32_t PageError = 0;
    FLASH_EraseInitTypeDef eraseinitstruct;
    eraseinitstruct.TypeErase    = FLASH_TYPEERASE_SECTORS;
    eraseinitstruct.Banks        = FLASH_BANK_1;
    eraseinitstruct.Sector       = FLASH_SECTOR_5;
    eraseinitstruct.NbSectors    = 1;
    eraseinitstruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    HAL_FLASHEx_Erase(&eraseinitstruct, &PageError);
    eraseinitstruct.TypeErase    = FLASH_TYPEERASE_SECTORS;
    eraseinitstruct.Banks        = FLASH_BANK_1;
    eraseinitstruct.Sector       = FLASH_SECTOR_6;
    eraseinitstruct.NbSectors    = 1;
    eraseinitstruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    HAL_FLASHEx_Erase(&eraseinitstruct, &PageError);

    //copy image
    for(uint32_t i = 0; i < tmp_fw_info->image_size; i++){
      //HAL_StatusTypeDef ret;
      //SEGGER_RTT_printf(0, "writing: %d -> %d\n",rx_buf[i],(uint32_t)fw_tmp + total_size + i);
      byte = fw_tmp[i];
      addr = APP_ADDRESS + i;
      flash_ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, addr, byte);
      //if(ret != HAL_OK){
      //SEGGER_RTT_printf(0, "flash error\n");
    }

    //check new app crc
    if(fw_info->image_size < 256*1024){
      crc_app =  HAL_CRC_Calculate(&hcrc, (uint32_t*)fw, fw_info->image_size / 4);
    }
    if(crc_app == 0){
        uint32_t PageError = 0;
        HAL_StatusTypeDef status;
        FLASH_EraseInitTypeDef eraseinitstruct;
        eraseinitstruct.TypeErase    = FLASH_TYPEERASE_SECTORS;
        eraseinitstruct.Banks        = FLASH_BANK_1;
        eraseinitstruct.Sector       = FLASH_SECTOR_8;
        eraseinitstruct.NbSectors    = 1;
        eraseinitstruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
        HAL_FLASHEx_Erase(&eraseinitstruct, &PageError);

        eraseinitstruct.TypeErase    = FLASH_TYPEERASE_SECTORS;
        eraseinitstruct.Banks        = FLASH_BANK_1;
        eraseinitstruct.Sector       = FLASH_SECTOR_9;
        eraseinitstruct.NbSectors    = 1;
        eraseinitstruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
        HAL_FLASHEx_Erase(&eraseinitstruct, &PageError);
    }
    HAL_FLASH_Lock();
  }
}

//__disable_irq();
HAL_DeInit();
SysTick->CTRL  &= ~SysTick_CTRL_ENABLE_Msk;
NVIC_DisableIRQ(SysTick_IRQn);
SCB->VTOR = APP_ADDRESS;

uint32_t stack = ((const uint32_t *)APP_ADDRESS)[0];
uint32_t entry = ((const uint32_t *)APP_ADDRESS)[1];
asm volatile(
  "msr    msp, %0        \n\t"
  "bx     %1             \n\t"
  :
  : "r"(stack), "r"(entry));
while(1);

  /* USER CODE END 2 */

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

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
