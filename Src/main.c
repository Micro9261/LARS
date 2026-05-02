/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Micro9261
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include <inttypes.h>
#include <stm32f4xx.h>
#include "stm32f446xx.h"
#include "usart_driver.h"
#include <string.h>

int main(void)
{
    //Configure GPIOA
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    //PA5 as output
    GPIOA->MODER |= GPIO_MODER_MODER5_0;

    //SysTick config
    SysTick->LOAD = 16000000 - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = (1UL << SysTick_CTRL_CLKSOURCE_Pos) | (1UL << SysTick_CTRL_TICKINT_Pos) | (1UL << SysTick_CTRL_ENABLE_Pos);

    __NVIC_EnableIRQ(SysTick_IRQn);

    //USART
    USART2_Init();

    while(1)
    {
        //nothing
    }
}

void SysTick_Handler(void)
{
    static uint8_t counter = 0;
    counter++;
    GPIOA->ODR ^= GPIO_ODR_OD5; // Toggle PA5
    char * msg = "Hello, USART2!\r\n";
    if (counter % 2 == 0) // Send message every 5 seconds
    {
        msg = "Hello, USART2! This is an even message.\r\n";
    }
    uint8_t msg_num = strlen(msg);
    USART2_Send(msg, msg_num);
    msg = "Testing Testing buffer overflow\r\n";
    msg_num = strlen(msg);
    USART2_Send(msg, msg_num);
}