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
static int influx_socket;
static int influx_request_len;
static char influx_request[1000];
static char influx_data[1000];
static struct sockaddr_in influx_addr;

static char influx_ip[CONFIG_STRINGLENGTH];
static int  influx_port;
static char influx_db[CONFIG_STRINGLENGTH];
static char influx_tag[CONFIG_STRINGLENGTH];
static char influx_tag_value[CONFIG_STRINGLENGTH];
static char influx_measurement[CONFIG_STRINGLENGTH];

uint32_t influxcount = 0;
static GPIO_PinState last = GPIO_PIN_RESET;

#define RISING_EDGE(sig) ({static uint32_t __old_val__ = 0; uint8_t ret = (sig) > __old_val__; __old_val__ = (sig); ret; })

void influx_task2(void){
    int ret;

    while(1){
        //GPIO_PinState now = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_1);
        if(RISING_EDGE(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_1))){
            int influx_data_len = snprintf(influx_data, sizeof(influx_data), "%s,%s=%s count=%lu,tick=1\r\n\r\n",
            influx_measurement,
            influx_tag,
            influx_tag_value,
            influxcount
            );
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
            influxcount++;
        }
        vTaskDelay(100);
    }
}

void influx_init2(){
    config_get_string("influx_ip", &influx_ip);
    config_get_int("influx_port", &influx_port);

    strcpy(influx_db,"heizung");
    strcpy(influx_tag,"location");
    strcpy(influx_tag_value,"vorne");
    strcpy(influx_measurement,"heizwerte");

    //config_get_string("influx_db", &influx_db);
    //config_get_string("influx_tag", &influx_tag);
    //config_get_string("influx_tag_value", &influx_tag_value);
    //config_get_string("influx_measurement", &influx_measurement);

    memset(&influx_addr, 0, sizeof(influx_addr));
    influx_addr.sin_len = sizeof(influx_addr);
    influx_addr.sin_family = AF_INET;
    influx_addr.sin_port = PP_HTONS(influx_port);
    influx_addr.sin_addr.s_addr = inet_addr(influx_ip);
    xTaskCreate((TaskFunction_t)influx_task2, "Influx task2", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
}
