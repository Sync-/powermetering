#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "cmsis_os.h"
#include "SEGGER_RTT.h"
#include "stm32f4xx_hal.h"
#include "version_stmbl.h"
#include "ade.h"
#include "config.h"
#include <mqtt.h>
#include <lwip/sockets.h> 
#include <string.h>
#include <math.h>

ip4_addr_t mqtt_server_addr;
mqtt_client_t* static_client;

void example_do_connect(mqtt_client_t *client);

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
  err_t err;
  SEGGER_RTT_printf(0, "Entered conn_cb.%d..\n",status);
  if (status == MQTT_CONNECT_ACCEPTED)
  {
    SEGGER_RTT_printf(0, "mqtt_connection_cb: Successfully connected\n");
  }
}



/* Called when publish is complete either with sucess or failure */
static void mqtt_pub_request_cb(void *arg, err_t result)
{
  SEGGER_RTT_printf(0, "mqtt_pub_request_cb.%d..\n",result);
  if (result != ERR_OK)
  {
    SEGGER_RTT_printf(0, "Publish result: %d\n", result);
  }
}

void example_publish(mqtt_client_t *client, void *arg)
{
  extern struct ade_float_t ade_f;
  char insert[LWIP_HTTPD_MAX_TAG_INSERT_LEN];
  float total = ade_f.awatt + ade_f.bwatt + ade_f.cwatt;
  int len =  snprintf(insert, LWIP_HTTPD_MAX_TAG_INSERT_LEN - 2,
                //"{\"status\": [%d,%d], "
                "{\"U\": {\"L1\": %.2f, \"L2\": %.2f, \"L3\": %.2f, \"L1(one)\": %.2f, \"L2(one)\": %.2f, \"L3(one)\": %.2f, \"L1(1012)\": %.2f, \"L2(1012)\": %.2f, \"L3(1012)\": %.2f}, "
                //"\"ang_U\":{ \"L2\":%.2f, \"L3\": %.2f}, "
                //"\"ang_I\":{ \"L1\":%.2f, \"L2\": %.2f, \"L3\": %.2f}, "
                //"\"I\": {\"L1\": %.2f, \"L2\": %.2f, \"L3\": %.2f, \"N\": %.2f, \"L1(one)\": %.2f, \"L2(one)\": %.2f, \"L3(one)\": %.2f, \"N(one)\": %.2f, \"L1(1012)\": %.2f, \"L2(1012)\": %.2f, \"L3(1012)\": %.2f, \"N(1012)\": %.2f, \"ISUMRMS\": %.4f},"
                //"\"P\": {\"L1\": %.2f, \"L2\": %.2f, \"L3\": %.2f, \"L1f\": %.2f, \"L2f\": %.2f, \"L3f\": %.2f},"
                "\"Q\": {\"total\": %.2f,\"L1\": %.2f, \"L2\": %.2f, \"L3\": %.2f, \"L1f\": %.2f, \"L2f\": %.2f, \"L3f\": %.2f}"
                //"\"S\": {\"L1\": %.2f, \"L2\": %.2f, \"L3\": %.2f, \"L1f\": %.2f, \"L2f\": %.2f, \"L3f\": %.2f}"
                //"\"pf\": {\"L1\": %.2f, \"L2\": %.2f, \"L3\": %.2f},"
                "}",
                //ade_f.status0, ade_f.status1,
                ade_f.avrms, ade_f.bvrms, ade_f.cvrms, ade_f.avrmsone, ade_f.bvrmsone, ade_f.cvrmsone, ade_f.avrms1012, ade_f.bvrms1012, ade_f.cvrms1012,
                //ade_f.angl_va_vb, ade_f.angl_va_vc,
                //ade_f.angl_va_ia, ade_f.angl_vb_ib, ade_f.angl_vc_ic,
                //ade_f.airms, ade_f.birms, ade_f.cirms, ade_f.nirms, ade_f.airmsone, ade_f.birmsone, ade_f.cirmsone, ade_f.nirmsone, ade_f.airms1012, ade_f.birms1012, ade_f.cirms1012, ade_f.nirms1012, ade_f.isumrms,
                total, ade_f.awatt, ade_f.bwatt, ade_f.cwatt, ade_f.afwatt, ade_f.bfwatt, ade_f.cfwatt
                //ade_f.avar, ade_f.bvar, ade_f.cvar, ade_f.afvar, ade_f.bfvar, ade_f.cfvar,
                //ade_f.ava, ade_f.bva, ade_f.cva, ade_f.afva, ade_f.bfva, ade_f.cfva
                //ade_f.apf, ade_f.bpf, ade_f.cpf
                );
  //int len = snprintf(insert, 200 - 2, "test\n");
  //int len = snprintf(insert, 200 - 2, "%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.3f;%3f;%3f;%3f;%3f;%3f;%3f;%3f;%3f;%.4f;%.4f;%.4f;", avrms, bvrms, cvrms, airms, birms, cirms, nirms, avthd, bvthd, cvthd, aithd, bithd, cithd, apf, bpf, cpf, ahz_f, bhz_f, chz_f);
  err_t err;
  u8_t qos = 2;    /* 0 1 or 2, see MQTT specification */
  u8_t retain = 0; /* No don't retain such crappy payload... */
  err = mqtt_publish(client, "trifasipower", insert, len, qos, retain, mqtt_pub_request_cb, arg);
  //HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
  if (err != ERR_OK)
  {
    SEGGER_RTT_printf(0, "Publish err: %d\n", err);
    if(err == -11) {
      example_do_connect(static_client);
      SEGGER_RTT_printf(0, "Connecting due to error...\n");
    }
  }
}

struct mqtt_connect_client_info_t ci;
void example_do_connect(mqtt_client_t *client)
{
  err_t err;

  /* Setup an empty client info structure */
  memset(&ci, 0, sizeof(ci));

  /* Minimal amount of information required is client identifier, so set it here */
  ci.client_id = "Powermeter";
  ci.client_user = "pwr";
  ci.client_pass = "pwr";

  /* Initiate client and connect to server, if this fails immediately an error code is returned
     otherwise mqtt_connection_cb will be called with connection result after attempting 
     to establish a connection with the server. 
     For now MQTT version 3.1.1 is always used */
  //SEGGER_RTT_printf(0, "Trying to connect...\n");
  err = mqtt_client_connect(client, &mqtt_server_addr, MQTT_PORT, mqtt_connection_cb, 0, &ci);

  /* For now just print the result code if something goes wrong */
  //if (err != ERR_OK)
  //{
    SEGGER_RTT_printf(0, "mqtt_connect return %d\n", err);
  //}
}

void mqtt_stuff(void)
{
  vTaskDelay(2000);
  vTaskDelay(15000);
  IP4_ADDR(&mqtt_server_addr, 192, 168, 177, 2);
  //SEGGER_RTT_printf(0, "Entered mqtt...\n");
  //static_client.conn_state = TCP_DISCONNECTED;
  static_client = mqtt_client_new();
  example_do_connect(static_client);
  while (1)
  {
    //MEM_STATS_DISPLAY();
    //SYS_STATS_DISPLAY();
    example_publish(static_client, 0);
    vTaskDelay(1000);
  }
}

void mqtt_init(void){
  xTaskCreate((TaskFunction_t)mqtt_stuff, "Do MQTT stuff", 1024, NULL, configMAX_PRIORITIES - 3, NULL);
}
