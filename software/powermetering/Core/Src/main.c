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
#include "def.h"
#include "config.h"
#include "SEGGER_RTT.h"
#include "ADE9000.h"
#include "fs.h"
#include "string.h"
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
  HAL_SPI_TransmitReceive(&hspi1, rx_reg, arr, 4, 10);
  HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_SET);
}

void get_ADE9000_data(uint16_t reg_num)
{
  uint8_t rx_reg[6] = {0, 0, 0, 0, 0, 0};
  rx_reg[1] = (uint8_t)(((reg_num << 4) & 0xFF00) >> 8);
  rx_reg[0] = (uint8_t)(((reg_num << 4) | (1 << 3)) & 0x00FF);

  HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_RESET);
  HAL_SPI_TransmitReceive(&hspi1, rx_reg, spi_rec_buffer, 4, 10);
  HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_SET);
}

uint8_t burst_tx[10];
uint8_t burst_tx_empty[750];

union ade_burst_rx_t foobar;

uint8_t ahz[10], bhz[10], chz[10], status0[10], status1[10];
uint8_t angl_va_vb[10], angl_va_vc[10], angl_va_ia[10], angl_vb_ib[10], angl_vc_ic[10];
uint8_t isumrms[10];


int32_t status0_i;
int32_t status1_i;
float avrms;
float bvrms;
float cvrms;
float avrmsone;
float bvrmsone;
float cvrmsone;
float avrms1012;
float bvrms1012;
float cvrms1012;
float airms;
float birms;
float cirms;
float nirms;
float airmsone;
float birmsone;
float cirmsone;
float nirmsone;
float airms1012;
float birms1012;
float cirms1012;
float nirms1012;
float isumrms_f;
float awatt;
float bwatt;
float cwatt;
float afwatt;
float bfwatt;
float cfwatt;
float ava;
float bva;
float cva;
float afva;
float bfva;
float cfva;
float avar;
float bvar;
float cvar;
float afvar;
float bfvar;
float cfvar;
float angl_va_vb_f;
float angl_va_vc_f;
float angl_va_ia_f;
float angl_vb_ib_f;
float angl_vc_ic_f;


void SPI_get_data(void)
{
  write_ADE9000_16(ADDR_CONFIG1, 1 << 0); //SWRST

  vTaskDelay(10);

  write_ADE9000_32(ADDR_VLEVEL, 2740646); //magic number™
  write_ADE9000_16(ADDR_CONFIG1, 1 << 11);
  write_ADE9000_16(ADDR_PGA_GAIN, 0b0001010101010101);
  write_ADE9000_32(ADDR_CONFIG0, 1<<0); //-IN for fault current

  write_ADE9000_32(ADDR_AVGAIN, 0x115afd);
  write_ADE9000_32(ADDR_BVGAIN, 0x1141e0);
  write_ADE9000_32(ADDR_CVGAIN, 0x10bd00);

  write_ADE9000_32(ADDR_AIGAIN, 0xff97dd4a);
  write_ADE9000_32(ADDR_BIGAIN, 0xff94bcdc);
  write_ADE9000_32(ADDR_CIGAIN, 0xff78404e);
  write_ADE9000_32(ADDR_NIGAIN, 0xff8c79e6);

  write_ADE9000_16(ADDR_RUN, 1);
  write_ADE9000_16(ADDR_EP_CFG, 1 << 0);

  while (1)
  { 

    get_ADE9000_data_reg(ADDR_APERIOD, ahz);
    get_ADE9000_data_reg(ADDR_BPERIOD, bhz);
    get_ADE9000_data_reg(ADDR_CPERIOD, chz);

    get_ADE9000_data_reg(ADDR_ANGL_VA_VB, angl_va_vb);
    get_ADE9000_data_reg(ADDR_ANGL_VA_VC, angl_va_vc);
    get_ADE9000_data_reg(ADDR_ANGL_VA_IA, angl_va_ia);
    get_ADE9000_data_reg(ADDR_ANGL_VB_IB, angl_vb_ib);
    get_ADE9000_data_reg(ADDR_ANGL_VC_IC, angl_vc_ic);

    get_ADE9000_data_reg(ADDR_ISUMRMS, isumrms);


    get_ADE9000_data_reg(ADDR_STATUS0, status0);
    get_ADE9000_data_reg(ADDR_STATUS1, status1);

    burst_tx[1] = (uint8_t)(((0x600 << 4) & 0xFF00) >> 8);
    burst_tx[0] = (uint8_t)(((0x600 << 4) | (1 << 3)) & 0x00FF);
    HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, burst_tx, 1, 10);
    HAL_SPI_TransmitReceive(&hspi1, burst_tx_empty, foobar.bytes, 0x3E * 2, 10);
    HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_SET);

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


#define USER_PASS_BUFSIZE 16

static void *current_connection;
static void *valid_connection;
static char last_user[USER_PASS_BUFSIZE];

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



char* tags[] = {"STAT","CONFIG","NAME", "PKTCNT", "PHASOR", "VERSION"};

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
    status0_i = (status0[3] << 24) + (status0[2] << 16) + (status0[5] << 8) + status0[4];
    status1_i = (status1[3] << 24) + (status1[2] << 16) + (status1[5] << 8) + status1[4];
    avrms = ((foobar.avrms_h << 16) + foobar.avrms_l) / VOLT_CONST;
    bvrms = ((foobar.bvrms_h << 16) + foobar.bvrms_l) / VOLT_CONST;
    cvrms = ((foobar.cvrms_h << 16) + foobar.cvrms_l) / VOLT_CONST;
    avrmsone = ((foobar.avrmsone_h << 16) + foobar.avrmsone_l) / VOLT_CONST;
    bvrmsone = ((foobar.bvrmsone_h << 16) + foobar.bvrmsone_l) / VOLT_CONST;
    cvrmsone = ((foobar.cvrmsone_h << 16) + foobar.cvrmsone_l) / VOLT_CONST;
    avrms1012 = ((foobar.avrms1012_h << 16) + foobar.avrms1012_l) / VOLT_CONST;
    bvrms1012 = ((foobar.bvrms1012_h << 16) + foobar.bvrms1012_l) / VOLT_CONST;
    cvrms1012 = ((foobar.cvrms1012_h << 16) + foobar.cvrms1012_l) / VOLT_CONST;
    airms = ((foobar.airms_h << 16) + foobar.airms_l) / CUR_CONST;
    birms = ((foobar.birms_h << 16) + foobar.birms_l) / CUR_CONST;
    cirms = ((foobar.cirms_h << 16) + foobar.cirms_l) / CUR_CONST;
    nirms = ((foobar.nirms_h << 16) + foobar.nirms_l) / CUR_CONST;
    airmsone = ((foobar.airmsone_h << 16) + foobar.airmsone_l) / CUR_CONST;
    birmsone = ((foobar.birmsone_h << 16) + foobar.birmsone_l) / CUR_CONST;
    cirmsone = ((foobar.cirmsone_h << 16) + foobar.cirmsone_l) / CUR_CONST;
    nirmsone = ((foobar.nirmsone_h << 16) + foobar.nirmsone_l) / CUR_CONST;
    airms1012 = ((foobar.airms1012_h << 16) + foobar.airms1012_l) / CUR_CONST;
    birms1012 = ((foobar.birms1012_h << 16) + foobar.birms1012_l) / CUR_CONST;
    cirms1012 = ((foobar.cirms1012_h << 16) + foobar.cirms1012_l) / CUR_CONST;
    nirms1012 = ((foobar.nirms1012_h << 16) + foobar.nirms1012_l) / CUR_CONST;
    isumrms_f = ((isumrms[3] << 24) + (isumrms[2] << 16) + (isumrms[5] << 8) + isumrms[4]) / CUR_CONST;
    awatt = ((foobar.awatt_h << 16) + foobar.awatt_l) * 0.004667801635019326;
    bwatt = ((foobar.bwatt_h << 16) + foobar.bwatt_l) * 0.004667801635019326;
    cwatt = ((foobar.cwatt_h << 16) + foobar.cwatt_l) * 0.004667801635019326;
    afwatt = ((foobar.afwatt_h << 16) + foobar.afwatt_l) * 0.004667801635019326;
    bfwatt = ((foobar.bfwatt_h << 16) + foobar.bfwatt_l) * 0.004667801635019326;
    cfwatt = ((foobar.cfwatt_h << 16) + foobar.cfwatt_l) * 0.004667801635019326;
    ava = ((foobar.ava_h << 16) + foobar.ava_l) * 0.004667801635019326;
    bva = ((foobar.bva_h << 16) + foobar.bva_l) * 0.004667801635019326;
    cva = ((foobar.cva_h << 16) + foobar.cva_l) * 0.004667801635019326;
    afva = ((foobar.afva_h << 16) + foobar.afva_l) * 0.004667801635019326;
    bfva = ((foobar.bfva_h << 16) + foobar.bfva_l) * 0.004667801635019326;
    cfva = ((foobar.cfva_h << 16) + foobar.cfva_l) * 0.004667801635019326;
    avar = ((foobar.avar_h << 16) + foobar.avar_l) * 0.004667801635019326;
    bvar = ((foobar.bvar_h << 16) + foobar.bvar_l) * 0.004667801635019326;
    cvar = ((foobar.cvar_h << 16) + foobar.cvar_l) * 0.004667801635019326;
    afvar = ((foobar.afvar_h << 16) + foobar.afvar_l) * 0.004667801635019326;
    bfvar = ((foobar.bfvar_h << 16) + foobar.bfvar_l) * 0.004667801635019326;
    cfvar = ((foobar.cfvar_h << 16) + foobar.cfvar_l) * 0.004667801635019326;
    angl_va_vb_f = ((angl_va_vb[3] << 8) + angl_va_vb[2]) * 0.017578125f;
    angl_va_vc_f = ((angl_va_vc[3] << 8) + angl_va_vc[2]) * 0.017578125f;
    angl_va_ia_f = ((angl_va_ia[3] << 8) + angl_va_ia[2]) * 0.017578125f;
    angl_vb_ib_f = ((angl_vb_ib[3] << 8) + angl_vb_ib[2]) * 0.017578125f;
    angl_vc_ic_f = ((angl_vc_ic[3] << 8) + angl_vc_ic[2]) * 0.017578125f;

    //return snprintf(insert, LWIP_HTTPD_MAX_TAG_INSERT_LEN - 2,"UL1 = %f; UL2 = %f; UL3=%f; ang_UL2=%f; ang_UL3=%f; ang_IL1=%f; ang_IL2=%f; ang_IL3=%f; IL1=%f; IL2=%f; IL3=%f;",
    return snprintf(insert, LWIP_HTTPD_MAX_TAG_INSERT_LEN - 2,
                    "{\"status\": [%d,%d], "
                    "\"U\": {\"L1\": %.2f, \"L2\": %.2f, \"L3\": %.2f, \"L1(one)\": %.2f, \"L2(one)\": %.2f, \"L3(one)\": %.2f, \"L1(1012)\": %.2f, \"L2(1012)\": %.2f, \"L3(1012)\": %.2f}, "
                    "\"ang_U\":{ \"L2\":%.2f, \"L3\": %.2f}, "
                    "\"ang_I\":{ \"L1\":%.2f, \"L2\": %.2f, \"L3\": %.2f}, "
                    "\"I\": {\"L1\": %.2f, \"L2\": %.2f, \"L3\": %.2f, \"N\": %.2f, \"L1(one)\": %.2f, \"L2(one)\": %.2f, \"L3(one)\": %.2f, \"N(one)\": %.2f, \"L1(1012)\": %.2f, \"L2(1012)\": %.2f, \"L3(1012)\": %.2f, \"N(1012)\": %.2f, \"ISUMRMS\": %.4f},"
                    "\"P\": {\"L1\": %.2f, \"L2\": %.2f, \"L3\": %.2f, \"L1f\": %.2f, \"L2f\": %.2f, \"L3f\": %.2f},"
                    "\"Q\": {\"L1\": %.2f, \"L2\": %.2f, \"L3\": %.2f, \"L1f\": %.2f, \"L2f\": %.2f, \"L3f\": %.2f},"
                    "\"S\": {\"L1\": %.2f, \"L2\": %.2f, \"L3\": %.2f, \"L1f\": %.2f, \"L2f\": %.2f, \"L3f\": %.2f}}",
                    status0_i, status1_i,
                    avrms, bvrms, cvrms, avrmsone, bvrmsone, cvrmsone, avrms1012, bvrms1012, cvrms1012,
                    angl_va_vb_f, angl_va_vc_f,
                    angl_va_ia_f, angl_vb_ib_f, angl_vc_ic_f,
                    airms, birms, cirms, nirms, airmsone, birmsone, cirmsone, nirmsone, airms1012, birms1012, cirms1012, nirms1012, isumrms_f,
                    awatt, bwatt, cwatt, afwatt, bfwatt, cfwatt,
                    ava, bva, cva, afva, bfva, cfva,
                    avar, bvar, cvar, afvar, bfvar, cfvar);
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
