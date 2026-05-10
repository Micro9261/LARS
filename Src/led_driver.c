#include "led_driver.h"
#include "stm32f446xx.h"

volatile led_status_t leds[LED_NUM] = {0};

void led_init(void)
{
    //GPIOA clock enable
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    //PA12 and PA11 as output
    GPIOA->MODER &= ~(GPIO_MODER_MODER11_Msk | GPIO_MODER_MODER12_Msk);
    GPIOA->MODER |= (GPIO_MODER_MODER11_0 | GPIO_MODER_MODER12_0);

    //Set low
    GPIOA->BSRR = GPIO_BSRR_BR11 | GPIO_BSRR_BR12;

    //TIM2 clock enable
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    //TIM2 configuration 1ms tick
    TIM2->PSC = 16000 - 1; // Prescaler for 1MHz timer clock
    TIM2->ARR = 500 - 1; // Auto-reload for 1ms tick
    TIM2->CNT = 0;
    TIM2->DIER |= TIM_DIER_UIE;

    NVIC_EnableIRQ(TIM2_IRQn); // Enable TIM2 interrupt
}

void led_turn_on(uint8_t led)
{
    if (led != LED_GREEN && led != LED_RED)
    {
        return;
    }

    if (led == LED_GREEN)
    {
        GPIOA->BSRR = GPIO_BSRR_BS11;
    }
    else if (led == LED_RED)
    {
        GPIOA->BSRR = GPIO_BSRR_BS12;
    }

    leds[led] = LED_STATUS_ON;
}

void led_turn_off(uint8_t led)
{
    if (led != LED_GREEN && led != LED_RED)
    {
        return;
    }

    if (led == LED_GREEN)
    {
        GPIOA->BSRR = GPIO_BSRR_BR11;
    }
    else if (led == LED_RED)
    {
        GPIOA->BSRR = GPIO_BSRR_BR12;
    }

    leds[led] = LED_STATUS_OFF;
}

void led_play_pattern(uint8_t led)
{
    led_turn_off(led);

    TIM2->CNT = 0;
    TIM2->CR1 |= TIM_CR1_CEN; // Enable TIM2
    leds[led] = LED_STATUS_PATTERN;
}

led_status_t get_led_status(uint8_t led)
{
    if (led != LED_GREEN && led != LED_RED)
    {
        return LED_STATUS_OFF;
    }

    return leds[led];
}

void TIM2_IRQHandler(void)
{
    static uint8_t pattern_cnt[LED_NUM] = {0};
    if (TIM2->SR & TIM_SR_UIF)
    {
        TIM2->SR &= ~TIM_SR_UIF; // Clear interrupt flag
        if (leds[LED_GREEN] == LED_STATUS_PATTERN)
        {
            if (pattern_cnt[LED_GREEN] < LED_PATTERN_CYCLES * 2)
            {
                if (GPIOA->IDR & GPIO_IDR_ID11)
                {
                    GPIOA->BSRR = GPIO_BSRR_BR11;
                }
                else
                {
                    GPIOA->BSRR = GPIO_BSRR_BS11;
                }
                pattern_cnt[LED_GREEN]++;
            }
            else
            {
                pattern_cnt[LED_GREEN] = 0;
                TIM2->CR1 &= ~TIM_CR1_CEN;
                leds[LED_GREEN] = LED_STATUS_OFF;
            }
        }
        else if (leds[LED_RED] == LED_STATUS_PATTERN)
        {
            if (pattern_cnt[LED_RED] < LED_PATTERN_CYCLES * 2 + 1)
            {
                if (GPIOA->IDR & GPIO_IDR_ID12)
                {
                    GPIOA->BSRR = GPIO_BSRR_BR12;
                }
                else
                {
                    GPIOA->BSRR = GPIO_BSRR_BS12;
                }
                pattern_cnt[LED_RED]++;
            }
            else
            {
                pattern_cnt[LED_RED] = 0;
                TIM2->CR1 &= ~TIM_CR1_CEN;
                leds[LED_GREEN] = LED_STATUS_ON;
            }
        }
    }
}