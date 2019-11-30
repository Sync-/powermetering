#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "cmsis_os.h"
#include "SEGGER_RTT.h"
#include "stm32f4xx_hal.h"
#include "version_stmbl.h"
#include "ade.h"
#include <lwip/sockets.h> 
#include <string.h>
#include <math.h>

//char request[] = "POST /write?db=trifasi_db HTTP/1.1\r\nHost: 192.168.178.121:8086\r\nUser-Agent: lwip\r\nAccept: */*\r\nContent-Length: 50\r\nContent-Type: application/x-www-form-urlencoded\r\nConnection: close\r\n\r\nmymeas,mytag=2 myfield=100,test=1,test2=3,blah=123\r\n\r\n";
int influx_socket;
int influx_request_len;
char influx_request[1000];
char influx_data[1000];
struct sockaddr_in influx_addr;

char influx_ip[] = "192.168.178.240";
int  influx_port = 8086;
char influx_db[] = "powermetering";
char influx_tag[] = "location";
char influx_tag_value[] = "VT_Vorn";
char influx_measurement[] = "Threephase";

extern union ade_burst_rx_t foobar;
extern uint8_t ahz[10], bhz[10], chz[10], status0[10], status1[10];

void influx_task(void){
    int ret;

    while(1){
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

        int influx_data_len = snprintf(influx_data, sizeof(influx_data), "%s,%s=%s avrms=%.2f,bvrms=%.2f,cvrms=%.2f,airms=%.2f,birms=%.2f,cirms=%.2f,nirms=%.2f,avthd=%.3f,bvthd=%3f,cvthd=%3f,aithd=%3f,bithd=%3f,cithd=%3f,apf=%3f,bpf=%3f,cpf=%3f,ahz=%.4f,bhz=%.4f,chz=%.4f\r\n\r\n",
        influx_measurement,
        influx_tag,
        influx_tag_value,
        avrms, bvrms, cvrms, airms, birms, cirms, nirms, avthd, bvthd, cvthd, aithd, bithd, cithd, apf, bpf, cpf, ahz_f, bhz_f, chz_f);
        influx_request[0] = '\0';
        influx_request_len = snprintf(influx_request,sizeof(influx_request),"POST /write?db=%s HTTP/1.1\r\nHost: %s:%d\r\nUser-Agent: lwip\r\nAccept: */*\r\nContent-Length: %d\r\nContent-Type: application/x-www-form-urlencoded\r\nConnection: close\r\n\r\n%s",influx_db,influx_ip,influx_port,influx_data_len-4,influx_data);
        influx_socket = lwip_socket(AF_INET, SOCK_STREAM, 0);
        //SEGGER_RTT_printf(0, "%s\n",influx_request);
        //SEGGER_RTT_printf(0, "size: %d\n",influx_request_len);
        ret = lwip_connect(influx_socket, (struct sockaddr*)&influx_addr, sizeof(influx_addr));
        //SEGGER_RTT_printf(0, "connect %d %d\n",ret,errno);
        ret = lwip_write(influx_socket, influx_request, influx_request_len);
        //SEGGER_RTT_printf(0, "write %d %d\n",ret,errno);
        ret = lwip_close(influx_socket);
        //SEGGER_RTT_printf(0, "close %d %d\n",ret,errno);
        vTaskDelay(500);
    }
}

void influx_init(){
    memset(&influx_addr, 0, sizeof(influx_addr));
    influx_addr.sin_len = sizeof(influx_addr);
    influx_addr.sin_family = AF_INET;
    influx_addr.sin_port = PP_HTONS(influx_port);
    influx_addr.sin_addr.s_addr = inet_addr(influx_ip);
    //xTaskCreate((TaskFunction_t)influx_task, "Influx task", configMINIMAL_STACK_SIZE*4, NULL, configMAX_PRIORITIES - 1, NULL);
}
