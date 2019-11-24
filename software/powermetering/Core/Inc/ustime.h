#pragma once

#include <stdint.h>

uint64_t get_us_time64(void);
uint32_t get_us_time32(void);

void     delay_us(uint32_t us);
void     delay_ms(uint32_t ms);

void     init_us_timer(void);

