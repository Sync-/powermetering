#pragma once

#define V_PER_BIT (0.707f / 52702092.0f)

#define R_SHUNT (0.05f * 2.0f)                 //double the pcb value
#define CUR_PRI 400.0f                         //primary current
#define CUR_SEC 5.00f                          //secondary current
#define CUR_TF ((R_SHUNT * CUR_SEC) / CUR_PRI) //volts per amp
#define CUR_CONST (CUR_TF / V_PER_BIT * 2.0f)  //amps per bit

#define R_HIGH 800000.0f
#define R_LOW 1000.0f
#define VOLT_TF ((1.0f / (R_HIGH + R_LOW)) * R_LOW) //volts per volt
#define VOLT_CONST (VOLT_TF / V_PER_BIT * 2.0f)

#define PWR_CONST (((VOLT_TF * CUR_TF) / (1.0f / 20694066.0f)) * 2.0f)

//#pragma pack(push, 1)
typedef union ade_burst_rx_t {
  uint8_t bytes[750];
  struct
  {
    //    uint8_t padding[2];

    int32_t av_pcf;
    int32_t bv_pcf;
    int32_t cv_pcf;
    int32_t ni_pcf;
    int32_t ai_pcf;
    int32_t bi_pcf;
    int32_t ci_pcf;

    int32_t airms;
    int32_t birms;
    int32_t cirms;

    int32_t avrms;
    int32_t bvrms;
    int32_t cvrms;

    int32_t nirms;

    int32_t awatt;
    int32_t bwatt;
    int32_t cwatt;

    int32_t ava;
    int32_t bva;
    int32_t cva;

    int32_t avar;
    int32_t bvar;
    int32_t cvar;

    int32_t afvar;
    int32_t bfvar;
    int32_t cfvar;

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

    int32_t afwatt;
    int32_t bfwatt;
    int32_t cfwatt;

    int32_t afva;
    int32_t bfva;
    int32_t cfva;

    int32_t afirms;
    int32_t bfirms;
    int32_t cfirms;

    int32_t afvrms;
    int32_t bfvrms;
    int32_t cfvrms;

    int32_t airmsone;
    int32_t birmsone;
    int32_t cirmsone;

    int32_t avrmsone;
    int32_t bvrmsone;
    int32_t cvrmsone;

    int32_t nirmsone;

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
