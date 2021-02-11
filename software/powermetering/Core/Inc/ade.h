#pragma once

#include <stdint.h>

// The ADC full-scale voltage at gain = 1 is 0.707 V rms.
#define V_PER_BIT (0.707f / 52702092.0f)


#define R_SHUNT (0.05f * 2.0f)                 //double the pcb value
//#define CUR_PRI 100.0f                         //primary current
#define CUR_SEC 5.00f                          //secondary current
#define CUR_TF ((R_SHUNT * CUR_SEC) / CUR_PRI) //volts per amp
#define CUR_CONST (CUR_TF / V_PER_BIT * 2.0f)  //amps per bit

#define R_HIGH 800000.0f
#define R_LOW 1000.0f
#define VOLT_TF ((1.0f / (R_HIGH + R_LOW)) * R_LOW) //volts per volt
#define VOLT_CONST (VOLT_TF / V_PER_BIT * 2.0f)

#define P_FULL_SCALE 20694066.0f
#define PWR_CONST (((VOLT_TF / 0.707f)  * (CUR_TF / 0.707f)  * P_FULL_SCALE) * 4.0f)

void SPI_get_data(void);
void write_ADE9000_16(uint16_t reg_num, uint16_t data);
void write_ADE9000_32(uint16_t reg_num, uint32_t data);
void get_ADE9000_data_reg(uint16_t reg_num, uint8_t *arr);
void ade_convert();

//#pragma pack(push, 1)
union ade_burst_rx_t {
  uint8_t bytes[750];
  struct{
    //    uint8_t padding[2];

    int32_t av_pcf;
    int32_t bv_pcf;
    int32_t cv_pcf;
    int32_t ni_pcf;
    int32_t ai_pcf;
    int32_t bi_pcf;
    int32_t ci_pcf;

    uint16_t airms_h;
    uint16_t airms_l;
    uint16_t birms_h;
    uint16_t birms_l;
    uint16_t cirms_h;
    uint16_t cirms_l;

    uint16_t avrms_h;
    uint16_t avrms_l;
    uint16_t bvrms_h;
    uint16_t bvrms_l;
    uint16_t cvrms_h;
    uint16_t cvrms_l;

    uint16_t nirms_h;
    uint16_t nirms_l;

    uint16_t awatt_h;
    uint16_t awatt_l;
    uint16_t bwatt_h;
    uint16_t bwatt_l;
    uint16_t cwatt_h;
    uint16_t cwatt_l;

    uint16_t ava_h;
    uint16_t ava_l;
    uint16_t bva_h;
    uint16_t bva_l;
    uint16_t cva_h;
    uint16_t cva_l;

    uint16_t avar_h;
    uint16_t avar_l;
    uint16_t bvar_h;
    uint16_t bvar_l;
    uint16_t cvar_h;
    uint16_t cvar_l;

    uint16_t afvar_h;
    uint16_t afvar_l;
    uint16_t bfvar_h;
    uint16_t bfvar_l;
    uint16_t cfvar_h;
    uint16_t cfvar_l;

    int16_t apf_h;
    int16_t apf_l;
    int16_t bpf_h;
    int16_t bpf_l;
    int16_t cpf_h;
    int16_t cpf_l;

    int16_t avthd_h;
    int16_t avthd_l;
    int16_t bvthd_h;
    int16_t bvthd_l;
    int16_t cvthd_h;
    int16_t cvthd_l;

    int16_t aithd_h;
    int16_t aithd_l;
    int16_t bithd_h;
    int16_t bithd_l;
    int16_t cithd_h;
    int16_t cithd_l;

    uint16_t afwatt_h;
    uint16_t afwatt_l;
    uint16_t bfwatt_h;
    uint16_t bfwatt_l;
    uint16_t cfwatt_h;
    uint16_t cfwatt_l;

    uint16_t afva_h;
    uint16_t afva_l;
    uint16_t bfva_h;
    uint16_t bfva_l;
    uint16_t cfva_h;
    uint16_t cfva_l;

    uint16_t afirms_h;
    uint16_t afirms_l;
    uint16_t bfirms_h;
    uint16_t bfirms_l;
    uint16_t cfirms_h;
    uint16_t cfirms_l;

    uint16_t afvrms_h;
    uint16_t afvrms_l;
    uint16_t bfvrms_h;
    uint16_t bfvrms_l;
    uint16_t cfvrms_h;
    uint16_t cfvrms_l;

    uint16_t airmsone_h;
    uint16_t airmsone_l;
    uint16_t birmsone_h;
    uint16_t birmsone_l;
    uint16_t cirmsone_h;
    uint16_t cirmsone_l;

    uint16_t avrmsone_h;
    uint16_t avrmsone_l;
    uint16_t bvrmsone_h;
    uint16_t bvrmsone_l;
    uint16_t cvrmsone_h;
    uint16_t cvrmsone_l;

    uint16_t nirmsone_h;
    uint16_t nirmsone_l;

    uint16_t airms1012_h;
    uint16_t airms1012_l;
    uint16_t birms1012_h;
    uint16_t birms1012_l;
    uint16_t cirms1012_h;
    uint16_t cirms1012_l;

    uint16_t avrms1012_h;
    uint16_t avrms1012_l;
    uint16_t bvrms1012_h;
    uint16_t bvrms1012_l;
    uint16_t cvrms1012_h;
    uint16_t cvrms1012_l;

    uint16_t nirms1012_h;
    uint16_t nirms1012_l;
  };
};
//#pragma pack(pop)

struct ade_float_t {
  int32_t status0;
  int32_t status1;
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
  float isumrms;
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
  float angl_va_vb;
  float angl_va_vc;
  float angl_va_ia;
  float angl_vb_ib;
  float angl_vc_ic;
  float avthd;
  float bvthd;
  float cvthd;
  float aithd;
  float bithd;
  float cithd;
  float apf;
  float bpf;
  float cpf;
  float ahz;
  float bhz;
  float chz;
  float energy_a;
  float energy_b;
  float energy_c;
  float kwh_a;
  float kwh_b;
  float kwh_c;
};
