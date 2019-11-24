/**
 * \file
 * Microsecond timer using TIM7
 *
 * \author    Thomas Kindler <mail@t-kindler.de>
 */
#include "ustime.h"
#include "stm32f4xx.h"

/**
 * Get time count in microseconds.
 *
 * \note
 *   this function must be called at least
 *   once every 65ms to work correctly.
 *
 * \todo: Use a 32 bit timer.
 *
 */
uint64_t get_us_time64(void)
{
    static uint16_t t0;
    static uint64_t tickcount;

    __disable_irq();

    uint16_t  t = TIM7->CNT;
    if (t < t0)
        tickcount += t + 0x10000 - t0;
    else
        tickcount += t - t0;

    t0 = t;

    __enable_irq();

    return tickcount;
}

uint32_t get_us_time32(void)
{
    return get_us_time64();
}

/**
 * Perform a microsecond delay
 *
 * \param  us  number of microseconds to wait.
 * \note   The actual delay will last between us and (us-1) microseconds.
 *         To wait _at_least_ 1 us, you should use delay_us(2).
 */
void delay_us(uint32_t us)
{
    uint16_t  t0 = TIM7->CNT;

    while (us > 0) {
        uint16_t t  = TIM7->CNT;
        uint16_t dt = t - t0;

        if (dt > us)
            break;

        us -= dt;
        t0  = t;
    }
}


/**
 * Perform a millisecond delay
 *
 * \param  ms  number of milliseconds to wait.
 */
void delay_ms(uint32_t ms)
{
    delay_us(ms * 1000);
}


/**
 * Set up TIM7 as a 16bit microsecond-timer.
 *
 */
void init_us_timer(void)
{
    // Enable peripheral clocks
    //
    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;

    // Timer 6 runs at SYSCLK/4 * 2 = 84 MHz
    //
    TIM7->PSC = (SystemCoreClock / 2 / 1000000) - 1;
    TIM7->ARR = 0xFFFF;
    TIM7->CR1 = TIM_CR1_CEN;
}

