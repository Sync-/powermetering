#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "cmsis_os.h"
#include "SEGGER_RTT.h"
#include "stm32f4xx_hal.h"
#include "version.h"
#include <lwip/sockets.h> 
#include <string.h>

uint32_t fwupdate_size;

int socket_fd,accept_fd;
int addr_size,sent_data; 
char data_buffer[80]; struct sockaddr_in sa,ra,isa;
uint8_t rx_buf[128];
int rec_size;
uint32_t total_size;
CRC_HandleTypeDef hcrc;

#define FW_TMP_ADDRESS 0x08080000
//#define FW_SECTOR FLASH_SECTOR_7
#define FW_TMP_SIZE 128*1024
#define VERSION_INFO_OFFSET 0x188
const version_info_t *tmp_fw_info = (void *)(FW_TMP_ADDRESS + VERSION_INFO_OFFSET);
const uint8_t *fw_tmp = (char *)FW_TMP_ADDRESS;

void fwupdate_task(void){
    socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = INADDR_ANY;
    sa.sin_port = htons(2000);

    if (bind(socket_fd, (struct sockaddr *)&sa, sizeof(sa)) == -1)
    {
        SEGGER_RTT_printf(0, "bind failed\n");
        //TODO: handle fault
    }

    listen(socket_fd,5);
    addr_size = sizeof(isa);
    while(1){
        accept_fd = accept(socket_fd, (struct sockaddr*)&isa,&addr_size);
        if(accept_fd < 0)
        {
            SEGGER_RTT_printf(0, "accept failed\n");
            //TODO: handle fault
        }

        SEGGER_RTT_printf(0, "erasing flash page...\n");
        taskENTER_CRITICAL();
        HAL_FLASH_Unlock();
        uint32_t PageError = 0;
        HAL_StatusTypeDef status;
        FLASH_EraseInitTypeDef eraseinitstruct;
        eraseinitstruct.TypeErase    = FLASH_TYPEERASE_SECTORS;
        eraseinitstruct.Banks        = FLASH_BANK_1;
        eraseinitstruct.Sector       = FLASH_SECTOR_8;
        eraseinitstruct.NbSectors    = 1;
        eraseinitstruct.VoltageRange = FLASH_VOLTAGE_RANGE_1;

        status = HAL_FLASHEx_Erase(&eraseinitstruct, &PageError);

        if(status != HAL_OK) {
            SEGGER_RTT_printf(0, "error erasing page8: %d\n",PageError);
            //TODO: handle fault
        }

        eraseinitstruct.TypeErase    = FLASH_TYPEERASE_SECTORS;
        eraseinitstruct.Banks        = FLASH_BANK_1;
        eraseinitstruct.Sector       = FLASH_SECTOR_9;
        eraseinitstruct.NbSectors    = 1;
        eraseinitstruct.VoltageRange = FLASH_VOLTAGE_RANGE_1;

        status = HAL_FLASHEx_Erase(&eraseinitstruct, &PageError);
        taskEXIT_CRITICAL();

        if(status != HAL_OK) {
            SEGGER_RTT_printf(0, "error erasing page9: %d\n",PageError);
            HAL_FLASH_Lock();
            //TODO: handle fault
        }

        total_size = 0;
        int connected = 1;
        while(connected){
            rec_size = recv(accept_fd, rx_buf, sizeof(rx_buf), 0);
            if (rec_size == 0) {//disconnected
                SEGGER_RTT_printf(0, "recv zero\n");
                connected = 0;
                close(accept_fd);
                HAL_FLASH_Lock();
            } else {
                SEGGER_RTT_printf(0, "recv: %d\n",rec_size);
                for(uint32_t i = 0; i < rec_size; i++){
                    HAL_StatusTypeDef ret;
                    //SEGGER_RTT_printf(0, "writing: %d -> %d\n",rx_buf[i],(uint32_t)fw_tmp + total_size + i);
                    ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, (uint32_t)fw_tmp + total_size + i, rx_buf[i]);
                    if(ret != HAL_OK){
                        SEGGER_RTT_printf(0, "flash error\n");
                    }
                }
                total_size += rec_size;
            }
        }

        HAL_FLASH_Lock();
        //strcpy(data_buffer,"Hello World\n");
        //sent_data = send(accept_fd, data_buffer,strlen(data_buffer),0);
        //close(accept_fd);
        // if(sent_data < 0 )
        // {
        //     SEGGER_RTT_printf(0, "send failed\n");
        // }
        SEGGER_RTT_printf(0, "total size: %d\n",total_size);
        SEGGER_RTT_printf(0, "name: %s\n",tmp_fw_info->product_name);
        SEGGER_RTT_printf(0, "size: %d\n",tmp_fw_info->image_size);

        uint32_t crc = 1;
        crc =  HAL_CRC_Calculate(&hcrc, (uint32_t*)fw_tmp, tmp_fw_info->image_size / 4);
        SEGGER_RTT_printf(0, "crc: %d\n",crc);
        vTaskDelay(100);
        if(crc == 0){
            HAL_NVIC_SystemReset();
        }
        vTaskDelay(100);
    }
}

void fwupdate_init(){
    xTaskCreate((TaskFunction_t)fwupdate_task, "Firmware update task", configMINIMAL_STACK_SIZE*2, NULL, configMAX_PRIORITIES - 1, NULL);
    //RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
    __HAL_RCC_CRC_CLK_ENABLE();
    hcrc.Instance = CRC;
    HAL_CRC_Init(&hcrc);
}
