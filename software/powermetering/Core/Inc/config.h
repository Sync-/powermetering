#pragma once

#include "config.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "cmsis_os.h"
#include "SEGGER_RTT.h"
#include "stm32f4xx_hal.h"
#include "version_stmbl.h"
#include <lwip/sockets.h> 
#include <string.h>

#define CONFIG_ADDRESS  0x08008000
#define CONFIG_SIZE     16*1024
#define CONFIG_SECTOR   FLASH_SECTOR_2
#define CONFIG_STRINGLENGTH 32


void config_write(uint8_t*, uint32_t);
uint32_t config_read(uint8_t*, uint32_t);
uint32_t config_read(uint8_t*, uint32_t);
void config_get_string(const char*, char*);
void config_get_int(const char*, int32_t*);
void config_get_float(const char*, float*);
