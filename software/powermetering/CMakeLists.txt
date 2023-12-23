cmake_minimum_required(VERSION 3.14)

set(CMAKE_TOOLCHAIN_FILE ./arm-none-eabi-gcc.cmake)
enable_language(C ASM)
#set(CMAKE_C_STANDARD 11)
#set(CMAKE_C_EXTENSIONS GNU)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(powermeter)

add_executable(${PROJECT_NAME}.elf
    Core/Src/main.c
    Core/Src/fwupdate.c
    Core/Src/influx.c
    Core/Src/mqtt_client.c
    Core/Src/config.c
    Core/Src/version_stmbl.c
    Core/Src/gpio.c
    Core/Src/SEGGER_RTT.c
    Core/Src/SEGGER_RTT_printf.c
    Core/Src/rtc.c
    Core/Src/spi.c
    LWIP/App/lwip.c
    LWIP/Target/ethernetif.c
    Core/Src/stm32f4xx_it.c
    Core/Src/stm32f4xx_hal_msp.c
    Core/Src/stm32f4xx_hal_timebase_tim.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_eth.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rtc.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rtc_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_crc.c
    Core/Src/system_stm32f4xx.c
    Middlewares/Third_Party/FreeRTOS/Source/croutine.c
    Middlewares/Third_Party/FreeRTOS/Source/event_groups.c
    Middlewares/Third_Party/FreeRTOS/Source/list.c
    Middlewares/Third_Party/FreeRTOS/Source/queue.c
    Middlewares/Third_Party/FreeRTOS/Source/stream_buffer.c
    Middlewares/Third_Party/FreeRTOS/Source/tasks.c
    Middlewares/Third_Party/FreeRTOS/Source/timers.c
    Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.c
    Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/auth.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/ccp.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/chap_ms.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/chap-md5.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/chap-new.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/demand.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/eap.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/eui64.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/fsm.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/ipcp.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/ipv6cp.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/lcp.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/magic.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/mppe.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/multilink.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/ppp.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/pppapi.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/pppcrypt.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/pppoe.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/pppol2tp.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/pppos.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/upap.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/utils.c
    Middlewares/Third_Party/LwIP/src/netif/ppp/vj.c
    Middlewares/Third_Party/LwIP/src/netif/ethernet.c
    Middlewares/Third_Party/LwIP/src/netif/slipif.c
    Middlewares/Third_Party/LwIP/src/netif/lowpan6.c
    Middlewares/Third_Party/LwIP/src/api/err.c
    Middlewares/Third_Party/LwIP/src/api/api_lib.c
    Middlewares/Third_Party/LwIP/src/api/netifapi.c
    Middlewares/Third_Party/LwIP/src/api/api_msg.c
    Middlewares/Third_Party/LwIP/src/api/sockets.c
    Middlewares/Third_Party/LwIP/src/api/tcpip.c
    Middlewares/Third_Party/LwIP/src/api/netdb.c
    Middlewares/Third_Party/LwIP/src/api/netbuf.c
    Middlewares/Third_Party/LwIP/src/core/udp.c
    Middlewares/Third_Party/LwIP/src/core/ip.c
    Middlewares/Third_Party/LwIP/src/core/tcp.c
    Middlewares/Third_Party/LwIP/src/core/timeouts.c
    Middlewares/Third_Party/LwIP/src/core/pbuf.c
    Middlewares/Third_Party/LwIP/src/core/tcp_out.c
    Middlewares/Third_Party/LwIP/src/core/raw.c
    Middlewares/Third_Party/LwIP/src/core/mem.c
    Middlewares/Third_Party/LwIP/src/core/memp.c
    Middlewares/Third_Party/LwIP/src/core/dns.c
    Middlewares/Third_Party/LwIP/src/core/sys.c
    Middlewares/Third_Party/LwIP/src/core/tcp_in.c
    Middlewares/Third_Party/LwIP/src/core/inet_chksum.c
    Middlewares/Third_Party/LwIP/src/core/netif.c
    Middlewares/Third_Party/LwIP/src/core/init.c
    Middlewares/Third_Party/LwIP/src/core/def.c
    Middlewares/Third_Party/LwIP/src/core/stats.c
    Middlewares/Third_Party/LwIP/src/core/ipv4/autoip.c
    Middlewares/Third_Party/LwIP/src/core/ipv4/igmp.c
    Middlewares/Third_Party/LwIP/src/core/ipv4/icmp.c
    Middlewares/Third_Party/LwIP/src/core/ipv4/ip4.c
    Middlewares/Third_Party/LwIP/src/core/ipv4/dhcp.c
    Middlewares/Third_Party/LwIP/src/core/ipv4/ip4_addr.c
    Middlewares/Third_Party/LwIP/src/core/ipv4/etharp.c
    Middlewares/Third_Party/LwIP/src/core/ipv4/ip4_frag.c
    Middlewares/Third_Party/LwIP/src/core/ipv6/ip6.c
    Middlewares/Third_Party/LwIP/src/core/ipv6/ethip6.c
    Middlewares/Third_Party/LwIP/src/core/ipv6/nd6.c
    Middlewares/Third_Party/LwIP/src/core/ipv6/inet6.c
    Middlewares/Third_Party/LwIP/src/core/ipv6/dhcp6.c
    Middlewares/Third_Party/LwIP/src/core/ipv6/icmp6.c
    Middlewares/Third_Party/LwIP/src/core/ipv6/mld6.c
    Middlewares/Third_Party/LwIP/src/core/ipv6/ip6_addr.c
    Middlewares/Third_Party/LwIP/src/core/ipv6/ip6_frag.c
    Middlewares/Third_Party/LwIP/system/OS/sys_arch.c
    Middlewares/Third_Party/LwIP/src/apps/mqtt/mqtt.c
    Core/Src/i2c.c
    Core/Src/sdio.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_sdmmc.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_sd.c
    Middlewares/Third_Party/LwIP/src/apps/httpd/httpd.c
    Middlewares/Third_Party/LwIP/src/apps/httpd/fs.c
    Core/Src/crc.c
    Core/Src/ustime.c
    Core/Src/ade.c
    Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c
    startup_stm32f407xx.s
)

target_include_directories(${PROJECT_NAME}.elf PRIVATE 
    Core/Inc
    LWIP/App
    LWIP/Target
    Middlewares/Third_Party/LwIP/src/include
    Middlewares/Third_Party/LwIP/system
    Drivers/STM32F4xx_HAL_Driver/Inc
    Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    Middlewares/Third_Party/FreeRTOS/Source/include
    Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS
    Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
    Middlewares/Third_Party/LwIP/src/include/netif/ppp
    Drivers/CMSIS/Device/ST/STM32F4xx/Include
    Middlewares/Third_Party/LwIP/src/include/lwip
    Middlewares/Third_Party/LwIP/src/include/lwip/apps
    Middlewares/Third_Party/LwIP/src/include/lwip/priv
    Middlewares/Third_Party/LwIP/src/include/lwip/prot
    Middlewares/Third_Party/LwIP/src/include/netif
    Middlewares/Third_Party/LwIP/src/include/posix
    Middlewares/Third_Party/LwIP/src/include/posix/sys
    Middlewares/Third_Party/LwIP/system/arch
    Drivers/CMSIS/Include
    ../shared/
    Middlewares/Third_Party/LwIP/src/apps/httpd
)

set_target_properties(${PROJECT_NAME}.elf PROPERTIES ADDITIONAL_CLEAN_FILES "${PROJECT_NAME}.bin;${PROJECT_NAME}.map")

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    -DUSE_HAL_DRIVER
    -DSTM32F407xx
)

target_compile_options(${PROJECT_NAME}.elf PRIVATE
    -mthumb
    -mcpu=cortex-m4
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
    -Wall
    -fdata-sections
    -ffunction-sections
    -g
    -gdwarf-2
    #-nodefaultlibs
    #-nostdlib
    #-nolibc
    #-fno-common
    #-fno-builtin
    #-nostartfiles
    -O0
)

target_link_options(${PROJECT_NAME}.elf PRIVATE
    -mthumb
    -mcpu=cortex-m4
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
    -T${CMAKE_CURRENT_SOURCE_DIR}/STM32F407VGTx_FLASH.ld
    --specs=nosys.specs
    #-specs=nano.specs
    #-lc
    #-lm
    -lnosys 
    -Wl,--print-memory-usage
    -Wl,-Map=${PROJECT_NAME}.map,--cref
    -Wl,--gc-sections
    #-lc
    #-lm 
    #-lnosys 
)

# Print executable size
add_custom_command(TARGET ${PROJECT_NAME}.elf
    POST_BUILD
    COMMAND ${CMAKE_SIZE_UTIL} ${PROJECT_NAME}.elf)

# Create bin file
add_custom_command(TARGET ${PROJECT_NAME}.elf
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary --gap-fill 0xFF ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin)
