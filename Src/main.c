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
#include <stm32f4xx.h>

#include <inttypes.h>
#include <string.h>
#include <stdio.h>

#include "system_stm32f4xx.h"
#include "usart_driver.h"
#include "keyboard_driver.h"
#include "lock_driver.h"
#include "mfrc522_driver.h"


int main(void)
{
    SystemInit();

    //SysTick config
    SysTick->LOAD = 16000000 - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = (1UL << SysTick_CTRL_CLKSOURCE_Pos) | (1UL << SysTick_CTRL_TICKINT_Pos) | (1UL << SysTick_CTRL_ENABLE_Pos);

    __NVIC_EnableIRQ(SysTick_IRQn);

    //Init components
    USART2_Init();
    lock_init();
    keyboard_init();
    // mfrc522_init();

    char buffer[50];
    sprintf(buffer, "MFRC522 Version: 0x%02X\r\n", mfrc522_version());
    uint8_t msg_num = strlen(buffer);
    USART2_Send(buffer, msg_num);

    while(1)
    {
        //nothing
    }
}

void SysTick_Handler(void)
{
    static uint8_t counter = 0;
    counter++;
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
    key_t key = {.key = '$', .long_press = 0};
    if ( keyboard_getchar(&key, false) )
    {
        // Key is available, send it over USART
        char buffer[50];
        sprintf(buffer, "Key pressed: %c. Type: %s\r\n", key.key, key.long_press ? "Long" : "Short");
        msg_num = strlen(buffer);
        USART2_Send(buffer, msg_num);

        if (key.key == 'A' && !key.long_press)
        {
            lock_open();
        }
        else if (key.key == 'A' && key.long_press)
        {
            lock_close();
        }

        if (key.key == 'D' && !key.long_press)
        {
            lock_block();
        }
        else if (key.key == 'D' && key.long_press)
        {
            lock_unblock();
        }
    }
    else 
    {
        // No key available, send a different message
        char buffer[50];
        sprintf(buffer, "No key pressed\r\n");
        msg_num = strlen(buffer);
        USART2_Send(buffer, msg_num);
    }
}