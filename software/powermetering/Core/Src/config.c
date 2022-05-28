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
#include "def.h"

const uint8_t *flash_config = (uint8_t *)CONFIG_ADDRESS;

void config_write(uint8_t* data, uint32_t len){
    if(len > CONFIG_SIZE - 1){
        return;
    }
    SEGGER_RTT_printf(0, "config_write erasing flash page...\n");
    taskENTER_CRITICAL();
    HAL_FLASH_Unlock();
    uint32_t PageError = 0;
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef eraseinitstruct;
    eraseinitstruct.TypeErase    = FLASH_TYPEERASE_SECTORS;
    eraseinitstruct.Banks        = FLASH_BANK_1;
    eraseinitstruct.Sector       = CONFIG_SECTOR;
    eraseinitstruct.NbSectors    = 1;
    eraseinitstruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    status = HAL_FLASHEx_Erase(&eraseinitstruct, &PageError);

    if(status != HAL_OK) {
        SEGGER_RTT_printf(0, "error erasing page: %d\n",PageError);
        //TODO: handle fault
    }
    taskEXIT_CRITICAL();
    SEGGER_RTT_printf(0, "flashing....\n");
    HAL_StatusTypeDef ret;
    for(uint32_t i = 0; i < len; i++){
        ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, (uint32_t)flash_config + i, data[i]);
        if(ret != HAL_OK){
            SEGGER_RTT_printf(0, "flash error\n");
        }
    }
    //terminate string
    ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, (uint32_t)flash_config + (uint32_t)len, '\0');
    if(ret != HAL_OK){
        SEGGER_RTT_printf(0, "flash error\n");
    }
    SEGGER_RTT_printf(0, "flashing done\n");

    HAL_FLASH_Lock();
}

uint32_t config_read(uint8_t* data, uint32_t size){
    uint32_t len = LWIP_MIN(size, strnlen(flash_config, CONFIG_SIZE));
    SEGGER_RTT_printf(0, "config_read len:%d\n",len);
    strncpy(data,flash_config,len);
    return len;
}

void config_get_string(const char* key2, char* value2){
    // SEGGER_RTT_printf(0, "getting string %s\n",key2);
    // SEGGER_RTT_printf(0, "string: %s\n",flash_config);
    char keyvalue[CONFIG_STRINGLENGTH*2+2];
    char key[CONFIG_STRINGLENGTH];
    char value[CONFIG_STRINGLENGTH];
    int ret = 0;
    int pos = 0;
    int keys_left = 1;

    while (flash_config[pos] != '\0') {
        ret = sscanf(flash_config+pos, "%65[^&]", keyvalue);
        if (ret != 1) {
            break;
        }
        
        // move pointer to next string, skip '&' if exists (not \0)
        pos += strlen(keyvalue);
        if (flash_config[pos] == '&')
            pos++;
        
        ret = sscanf(keyvalue,"%32[^=]=%32[^=]",key,value);
        if(ret != 2)
            continue;

        if(strncmp(key2,key,CONFIG_STRINGLENGTH) == 0){
            //SEGGER_RTT_printf(0, "found %s => %s\n", key, value);
            strncpy(value2,value,CONFIG_STRINGLENGTH);
            break;
        }
        //printf("next parsed char: '%c'\n",*(string+pos));
    }
}

void config_get_int(const char* name, int32_t* value){
    char* string[CONFIG_STRINGLENGTH];
    string[0]='\0';
    config_get_string(name, string);
    *value = atoi(string);
}

void config_get_float(const char* name, float* value){
    char* string[CONFIG_STRINGLENGTH];
    string[0]='\0';
    config_get_string(name, string);
    *value = strtof(string,NULL);
}
