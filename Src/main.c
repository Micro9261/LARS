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

#include "stm32f446xx.h"
#include <inttypes.h>
#include <stm32f4xx.h>

int main(void)
{
    //Configure GPIOA
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    //PA5 as output
    GPIOA->MODER |= GPIO_MODER_MODER5_0;

    //SysTick config
    SysTick->LOAD = 1600000 - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;

    __NVIC_EnableIRQ(SysTick_IRQn);
    
    while(1)
    {
        //nothing
    }
}

void SysTick_Handler(void)
{
    GPIOA->ODR ^= GPIO_ODR_OD5; // Toggle PA5
}