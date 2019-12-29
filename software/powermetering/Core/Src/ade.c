#include "ade.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "ADE9000.h"
#include "main.h"
#include "config.h"
#include <math.h>

volatile union ade_burst_rx_t ade_raw;
struct ade_float_t ade_f;
uint8_t ahz[10], bhz[10], chz[10], status0[10], status1[10];
uint8_t isumrms[10];
uint8_t angl_va_vb[10], angl_va_vc[10], angl_va_ia[10], angl_vb_ib[10], angl_vc_ic[10];
uint8_t burst_tx[10];
int CUR_PRI;

extern SPI_HandleTypeDef hspi1;

void get_ADE9000_data_reg(uint16_t reg_num, uint8_t *arr)
{
  uint8_t rx_reg[6] = {0, 0, 0, 0, 0, 0};
  rx_reg[1] = (uint8_t)(((reg_num << 4) & 0xFF00) >> 8);
  rx_reg[0] = (uint8_t)(((reg_num << 4) | (1 << 3)) & 0x00FF);

  HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_RESET);
  HAL_SPI_TransmitReceive(&hspi1, rx_reg, arr, 4, 10);
  HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_SET);
}

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

  //DMA setup, SPI1 DMA2 Channel 3, rx: stream2, tx: stream3
  __HAL_RCC_DMA2_CLK_ENABLE();
  DMA2_Stream2->CR  &= ~DMA_SxCR_EN;
  DMA2_Stream2->PAR  = (uint32_t)&(SPI1->DR);
  DMA2_Stream2->M0AR = (uint32_t)ade_raw.bytes;
  DMA2_Stream2->CR   = DMA_SxCR_MINC | DMA_SxCR_CHSEL_0 | DMA_SxCR_CHSEL_1 | DMA_SxCR_MSIZE_0 | DMA_SxCR_PSIZE_0;
  SPI1->CR2 |= SPI_CR2_RXDMAEN;

  config_get_int("cur_pri", &CUR_PRI);
  if(CUR_PRI < 1){
    CUR_PRI = 400;
  }

  while (1)
  { 
    //dummy read, first read after dma transfer is broken for some reason
    uint8_t status0_dummy[10];
    get_ADE9000_data_reg(ADDR_STATUS0, status0_dummy);

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

    DMA2_Stream2->CR  &= ~DMA_SxCR_EN;
    DMA2_Stream2->NDTR = 0x3E * 2;
    DMA2->LIFCR        = DMA_LIFCR_CTCIF2 | DMA_LIFCR_CFEIF2;
    DMA2_Stream2->CR  |= DMA_SxCR_EN;
    SPI1->CR1 |= SPI_CR1_RXONLY;
    vTaskDelay(2);
    SPI1->CR1 &= ~SPI_CR1_RXONLY;

    HAL_GPIO_WritePin(ADE_CS_GPIO_Port, ADE_CS_Pin, GPIO_PIN_SET);
    vTaskDelay(2);
    ade_convert();
  }
}

void ade_convert(){
    ade_f.status0 = (status0[3] << 24) + (status0[2] << 16) + (status0[5] << 8) + status0[4];
    ade_f.status1 = (status1[3] << 24) + (status1[2] << 16) + (status1[5] << 8) + status1[4];
    ade_f.avrms = ((ade_raw.avrms_h << 16) + ade_raw.avrms_l) / VOLT_CONST;
    ade_f.bvrms = ((ade_raw.bvrms_h << 16) + ade_raw.bvrms_l) / VOLT_CONST;
    ade_f.cvrms = ((ade_raw.cvrms_h << 16) + ade_raw.cvrms_l) / VOLT_CONST;
    ade_f.avrmsone = ((ade_raw.avrmsone_h << 16) + ade_raw.avrmsone_l) / VOLT_CONST;
    ade_f.bvrmsone = ((ade_raw.bvrmsone_h << 16) + ade_raw.bvrmsone_l) / VOLT_CONST;
    ade_f.cvrmsone = ((ade_raw.cvrmsone_h << 16) + ade_raw.cvrmsone_l) / VOLT_CONST;
    ade_f.avrms1012 = ((ade_raw.avrms1012_h << 16) + ade_raw.avrms1012_l) / VOLT_CONST;
    ade_f.bvrms1012 = ((ade_raw.bvrms1012_h << 16) + ade_raw.bvrms1012_l) / VOLT_CONST;
    ade_f.cvrms1012 = ((ade_raw.cvrms1012_h << 16) + ade_raw.cvrms1012_l) / VOLT_CONST;
    ade_f.airms = ((ade_raw.airms_h << 16) + ade_raw.airms_l) / CUR_CONST;
    ade_f.birms = ((ade_raw.birms_h << 16) + ade_raw.birms_l) / CUR_CONST;
    ade_f.cirms = ((ade_raw.cirms_h << 16) + ade_raw.cirms_l) / CUR_CONST;
    ade_f.nirms = ((ade_raw.nirms_h << 16) + ade_raw.nirms_l) / CUR_CONST;
    ade_f.airmsone = ((ade_raw.airmsone_h << 16) + ade_raw.airmsone_l) / CUR_CONST;
    ade_f.birmsone = ((ade_raw.birmsone_h << 16) + ade_raw.birmsone_l) / CUR_CONST;
    ade_f.cirmsone = ((ade_raw.cirmsone_h << 16) + ade_raw.cirmsone_l) / CUR_CONST;
    ade_f.nirmsone = ((ade_raw.nirmsone_h << 16) + ade_raw.nirmsone_l) / CUR_CONST;
    ade_f.airms1012 = ((ade_raw.airms1012_h << 16) + ade_raw.airms1012_l) / CUR_CONST;
    ade_f.birms1012 = ((ade_raw.birms1012_h << 16) + ade_raw.birms1012_l) / CUR_CONST;
    ade_f.cirms1012 = ((ade_raw.cirms1012_h << 16) + ade_raw.cirms1012_l) / CUR_CONST;
    ade_f.nirms1012 = ((ade_raw.nirms1012_h << 16) + ade_raw.nirms1012_l) / CUR_CONST;
    ade_f.isumrms = ((isumrms[3] << 24) + (isumrms[2] << 16) + (isumrms[5] << 8) + isumrms[4]) / CUR_CONST;
    ade_f.awatt = ((ade_raw.awatt_h << 16) + ade_raw.awatt_l) * 0.004667801635019326;
    ade_f.bwatt = ((ade_raw.bwatt_h << 16) + ade_raw.bwatt_l) * 0.004667801635019326;
    ade_f.cwatt = ((ade_raw.cwatt_h << 16) + ade_raw.cwatt_l) * 0.004667801635019326;
    ade_f.afwatt = ((ade_raw.afwatt_h << 16) + ade_raw.afwatt_l) * 0.004667801635019326;
    ade_f.bfwatt = ((ade_raw.bfwatt_h << 16) + ade_raw.bfwatt_l) * 0.004667801635019326;
    ade_f.cfwatt = ((ade_raw.cfwatt_h << 16) + ade_raw.cfwatt_l) * 0.004667801635019326;
    ade_f.ava = ((ade_raw.ava_h << 16) + ade_raw.ava_l) * 0.004667801635019326;
    ade_f.bva = ((ade_raw.bva_h << 16) + ade_raw.bva_l) * 0.004667801635019326;
    ade_f.cva = ((ade_raw.cva_h << 16) + ade_raw.cva_l) * 0.004667801635019326;
    ade_f.afva = ((ade_raw.afva_h << 16) + ade_raw.afva_l) * 0.004667801635019326;
    ade_f.bfva = ((ade_raw.bfva_h << 16) + ade_raw.bfva_l) * 0.004667801635019326;
    ade_f.cfva = ((ade_raw.cfva_h << 16) + ade_raw.cfva_l) * 0.004667801635019326;
    ade_f.avar = ((ade_raw.avar_h << 16) + ade_raw.avar_l) * 0.004667801635019326;
    ade_f.bvar = ((ade_raw.bvar_h << 16) + ade_raw.bvar_l) * 0.004667801635019326;
    ade_f.cvar = ((ade_raw.cvar_h << 16) + ade_raw.cvar_l) * 0.004667801635019326;
    ade_f.afvar = ((ade_raw.afvar_h << 16) + ade_raw.afvar_l) * 0.004667801635019326;
    ade_f.bfvar = ((ade_raw.bfvar_h << 16) + ade_raw.bfvar_l) * 0.004667801635019326;
    ade_f.cfvar = ((ade_raw.cfvar_h << 16) + ade_raw.cfvar_l) * 0.004667801635019326;
    ade_f.angl_va_vb = ((angl_va_vb[3] << 8) + angl_va_vb[2]) * 0.017578125f;
    ade_f.angl_va_vc = ((angl_va_vc[3] << 8) + angl_va_vc[2]) * 0.017578125f;
    ade_f.angl_va_ia = ((angl_va_ia[3] << 8) + angl_va_ia[2]) * 0.017578125f;
    ade_f.angl_vb_ib = ((angl_vb_ib[3] << 8) + angl_vb_ib[2]) * 0.017578125f;
    ade_f.angl_vc_ic = ((angl_vc_ic[3] << 8) + angl_vc_ic[2]) * 0.017578125f;
    ade_f.avthd = ((ade_raw.avthd_h << 16) + ade_raw.avthd_l) * powf(2, -27);
    ade_f.bvthd = ((ade_raw.bvthd_h << 16) + ade_raw.bvthd_l) * powf(2, -27);
    ade_f.cvthd = ((ade_raw.cvthd_h << 16) + ade_raw.cvthd_l) * powf(2, -27);
    ade_f.aithd = ((ade_raw.aithd_h << 16) + ade_raw.aithd_l) * powf(2, -27);
    ade_f.bithd = ((ade_raw.bithd_h << 16) + ade_raw.bithd_l) * powf(2, -27);
    ade_f.cithd = ((ade_raw.cithd_h << 16) + ade_raw.cithd_l) * powf(2, -27);
    ade_f.apf = ((ade_raw.apf_h << 16) + ade_raw.apf_l) * powf(2, -27);
    ade_f.bpf = ((ade_raw.bpf_h << 16) + ade_raw.bpf_l) * powf(2, -27);
    ade_f.cpf = ((ade_raw.cpf_h << 16) + ade_raw.cpf_l) * powf(2, -27);
    int32_t ahz_i = (ahz[3] << 24) + (ahz[2] << 16) + (ahz[5] << 8) + ahz[4];
    int32_t bhz_i = (bhz[3] << 24) + (bhz[2] << 16) + (bhz[5] << 8) + bhz[4];
    int32_t chz_i = (chz[3] << 24) + (chz[2] << 16) + (chz[5] << 8) + chz[4];
    ade_f.ahz = (8000.0f * powf(2.0f, 16.0f)) / (ahz_i + 1.0f);
    ade_f.bhz = (8000.0f * powf(2.0f, 16.0f)) / (bhz_i + 1.0f);
    ade_f.chz = (8000.0f * powf(2.0f, 16.0f)) / (chz_i + 1.0f);
}
