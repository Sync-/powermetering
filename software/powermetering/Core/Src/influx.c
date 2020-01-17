#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "cmsis_os.h"
#include "SEGGER_RTT.h"
#include "stm32f4xx_hal.h"
#include "version_stmbl.h"
#include "ade.h"
#include "config.h"
#include <lwip/sockets.h> 
#include <string.h>
#include <math.h>

//char request[] = "POST /write?db=trifasi_db HTTP/1.1\r\nHost: 192.168.178.121:8086\r\nUser-Agent: lwip\r\nAccept: */*\r\nContent-Length: 50\r\nContent-Type: application/x-www-form-urlencoded\r\nConnection: close\r\n\r\nmymeas,mytag=2 myfield=100,test=1,test2=3,blah=123\r\n\r\n";
int influx_socket;
int influx_request_len;
char influx_request[1000];
char influx_data[1000];
struct sockaddr_in influx_addr;

char influx_ip[CONFIG_STRINGLENGTH];
int  influx_port;
char influx_db[CONFIG_STRINGLENGTH];
char influx_tag[CONFIG_STRINGLENGTH];
char influx_tag_value[CONFIG_STRINGLENGTH];
char influx_measurement[CONFIG_STRINGLENGTH];

extern union ade_burst_rx_t foobar;
extern uint8_t ahz[10], bhz[10], chz[10], status0[10], status1[10];

void influx_task(void){
    int ret;

    while(1){
        extern struct ade_float_t ade_f;
        int influx_data_len = snprintf(influx_data, sizeof(influx_data), "%s,%s=%s avrms=%.2f,bvrms=%.2f,cvrms=%.2f,airms=%.2f,birms=%.2f,cirms=%.2f,nirms=%.2f,avthd=%.3f,bvthd=%3f,cvthd=%3f,aithd=%3f,bithd=%3f,cithd=%3f,apf=%3f,bpf=%3f,cpf=%3f,ahz=%.4f,bhz=%.4f,chz=%.4f,isum=%.4f,awatt=%.4f,bwatt=%.4f,cwatt=%.4f,afwatt=%.4f,bfwatt=%.4f,cfwatt=%.4f,avar=%.4f,bvar=%.4f,cvar=%.4f,afvar=%.4f,bfvar=%.4f,cfvar=%.4f,ava=%.4f,bva=%.4f,cva=%.4f,afva=%.4f,bfva=%.4f,cfva=%.4f\r\n\r\n",
        influx_measurement,
        influx_tag,
        influx_tag_value,
                ade_f.avrms1012, ade_f.bvrms1012, ade_f.cvrms1012, ade_f.airms1012, ade_f.birms1012, ade_f.cirms1012, ade_f.nirms1012, ade_f.avthd, ade_f.bvthd, ade_f.cvthd, ade_f.aithd, ade_f.bithd, ade_f.cithd, ade_f.apf, ade_f.bpf, ade_f.cpf, ade_f.ahz, ade_f.bhz, ade_f.chz,ade_f.isumrms,ade_f.awatt, ade_f.bwatt, ade_f.cwatt, ade_f.afwatt, ade_f.bfwatt, ade_f.cfwatt,ade_f.avar, ade_f.bvar, ade_f.cvar, ade_f.afvar, ade_f.bfvar, ade_f.cfvar,ade_f.ava, ade_f.bva, ade_f.cva, ade_f.afva, ade_f.bfva, ade_f.cfva);
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
    config_get_string("influx_ip", &influx_ip);
    config_get_int("influx_port", &influx_port);
    config_get_string("influx_db", &influx_db);
    config_get_string("influx_tag", &influx_tag);
    config_get_string("influx_tag_value", &influx_tag_value);
    config_get_string("influx_measurement", &influx_measurement);

    memset(&influx_addr, 0, sizeof(influx_addr));
    influx_addr.sin_len = sizeof(influx_addr);
    influx_addr.sin_family = AF_INET;
    influx_addr.sin_port = PP_HTONS(influx_port);
    influx_addr.sin_addr.s_addr = inet_addr(influx_ip);
    xTaskCreate((TaskFunction_t)influx_task, "Influx task", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
}
