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

#include "MFRC522.h"
#include "system_stm32f4xx.h"
#include "usart_driver.h"
#include "keyboard_driver.h"
#include "lock_driver.h"
#include "mfrc522_driver.h"

 // a private key to scramble data writing/reading to/from RFID card:
// static uint8_t Mx1[7][5]={
// {0x01,0x01,0x01,0x01},
// {0x02,0x02,0x02,0x02},
// {0x03,0x03,0x03,0x03},
// {0x04,0x04,0x04,0x04},
// {0x05,0x05,0x05,0x05},
// {0x06,0x06,0x06,0x06}
// };

static uint8_t Mx1[7][5]={{0x12,0x45,0xF2,0xA8},{0xB2,0x6C,0x39,0x83},{0x55,0xE5,0xDA,0x18},
		  	  	  	{0x1F,0x09,0xCA,0x75},{0x99,0xA2,0x50,0xEC},{0x2C,0x88,0x7F,0x3D}};

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
    mfrc522_init();

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
    /*
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
    */
    uint8_t msg_num;
    //Card test
    uint8_t cardstr[17] = {0};
    uint8_t status = MFRC522_ERR;
    status = mfrc522_request(MFRC522_PICC_REQIDL, cardstr);
    if (status == MFRC522_OK)
    {
        char buffer[50];
        sprintf(buffer, "Card talking! Card:%x,%x,%x\r\n", cardstr[0], cardstr[1], cardstr[2]);
        msg_num = strlen(buffer);
        USART2_Send(buffer, msg_num);

        status = mfrc522_anticoll(cardstr);
        if (status == MFRC522_OK)
        {
            char UID[5];
            sprintf(buffer, "Card UID: %x %x %x %x\r\n", cardstr[0], cardstr[1], cardstr[2], cardstr[3]);
            UID[0] = cardstr[0];
            UID[1] = cardstr[1];
            UID[2] = cardstr[2];
            UID[3] = cardstr[3];
            UID[4] = cardstr[4];
            msg_num = strlen(buffer);
            USART2_Send(buffer, msg_num);

            status = mfrc522_select_tag(cardstr);
            if (status > 0)
            {
                uint8_t SectorKey[7];
                SectorKey[0] = ((Mx1[0][0])^(UID[0])) + ((Mx1[0][1])^(UID[1])) + ((Mx1[0][2])^(UID[2])) + ((Mx1[0][3])^(UID[3]));// 0x11; //KeyA[0]
                SectorKey[1] = ((Mx1[1][0])^(UID[0])) + ((Mx1[1][1])^(UID[1])) + ((Mx1[1][2])^(UID[2])) + ((Mx1[1][3])^(UID[3]));// 0x11; //KeyA[0]
                SectorKey[2] = ((Mx1[2][0])^(UID[0])) + ((Mx1[2][1])^(UID[1])) + ((Mx1[2][2])^(UID[2])) + ((Mx1[2][3])^(UID[3]));// 0x11; //KeyA[0]
                SectorKey[3] = ((Mx1[3][0])^(UID[0])) + ((Mx1[3][1])^(UID[1])) + ((Mx1[3][2])^(UID[2])) + ((Mx1[3][3])^(UID[3]));// 0x11; //KeyA[0]
                SectorKey[4] = ((Mx1[4][0])^(UID[0])) + ((Mx1[4][1])^(UID[1])) + ((Mx1[4][2])^(UID[2])) + ((Mx1[4][3])^(UID[3]));// 0x11; //KeyA[0]
                SectorKey[5] = ((Mx1[5][0])^(UID[0])) + ((Mx1[5][1])^(UID[1])) + ((Mx1[5][2])^(UID[2])) + ((Mx1[5][3])^(UID[3]));// 0x11; //KeyA[0]

                status = mfrc522_auth(0x60, 3, SectorKey, cardstr);
                if (status == MFRC522_OK)
                {
                    
                    const char * msg = "Auth. OK\r\n";
                    msg_num = strlen(buffer);
                    USART2_Send(msg, msg_num);
                    uint8_t card_data[17];
                    card_data[0] = 0xFF;
                    card_data[1] = 0xFF;
                    card_data[2] = 0xFF;
                    card_data[3] = 0xFF;
                    card_data[4] = 0xFF;
                    card_data[5] = 0xFF;
                    card_data[6] = 0xFF; //Access_bits[6]
                    card_data[7] = 0x07; //Access_bits[7]
                    card_data[8] = 0x80; //Access_bits[8]
                    card_data[9] = 0x88; //user_byte[9]
                    card_data[10] = 0x88; //user_byte[10]
                    card_data[11] = 0x88; //user_byte[11]
                    card_data[12] = 0x88; //user_byte[12]
                    card_data[13] = 0x88; //user_byte[13]
                    card_data[14] = 0x88; //user_byte[14]
                    card_data[15] = 0x88; //user_byte[15]
                    status = mfrc522_write(3, card_data);
                    if (status == MFRC522_OK)
                    {
                        const char * msg2 = "Card Cleared!\r\n";
                        msg_num = strlen(buffer);
                        USART2_Send(msg2, msg_num);
                    }

                }
                else
                {
                    for (uint8_t i = 0; i < 16; i++)
                    {
                        cardstr[i] = 0x0;
                    }

                    status = 0;
                    status = mfrc522_request(MFRC522_PICC_REQIDL, cardstr);
                    status = mfrc522_anticoll(cardstr);
                    status = mfrc522_select_tag(cardstr);
                    SectorKey[0] = 0xFF;
                    SectorKey[1] = 0xFF;
                    SectorKey[2] = 0xFF;
                    SectorKey[3] = 0xFF;
                    SectorKey[4] = 0xFF;
                    SectorKey[5] = 0xFF;

                    status = mfrc522_auth(0x60, 3, SectorKey, cardstr);
                    if (status == MFRC522_OK)
                    {
                        const char * msg = "Auth. OK. New card!\r\n";
                        msg_num = strlen(buffer);
                        USART2_Send(msg, msg_num);
                        uint8_t card_data[17];
                        card_data[0] = ((Mx1[0][0])^(UID[0])) + ((Mx1[0][1])^(UID[1])) + ((Mx1[0][2])^(UID[2])) + ((Mx1[0][3])^(UID[3]));// 0x11; //KeyA[0]
                        card_data[1] = ((Mx1[1][0])^(UID[0])) + ((Mx1[1][1])^(UID[1])) + ((Mx1[1][2])^(UID[2])) + ((Mx1[1][3])^(UID[3]));// 0x11; //KeyA[0]
                        card_data[2] = ((Mx1[2][0])^(UID[0])) + ((Mx1[2][1])^(UID[1])) + ((Mx1[2][2])^(UID[2])) + ((Mx1[2][3])^(UID[3]));// 0x11; //KeyA[0]
                        card_data[3] = ((Mx1[3][0])^(UID[0])) + ((Mx1[3][1])^(UID[1])) + ((Mx1[3][2])^(UID[2])) + ((Mx1[3][3])^(UID[3]));// 0x11; //KeyA[0]
                        card_data[4] = ((Mx1[4][0])^(UID[0])) + ((Mx1[4][1])^(UID[1])) + ((Mx1[4][2])^(UID[2])) + ((Mx1[4][3])^(UID[3]));// 0x11; //KeyA[0]
                        card_data[5] = ((Mx1[5][0])^(UID[0])) + ((Mx1[5][1])^(UID[1])) + ((Mx1[5][2])^(UID[2])) + ((Mx1[5][3])^(UID[3]));// 0x11; //KeyA[0]
                        card_data[6] = 0xFF; //Access_bits[6]
                        card_data[7] = 0x07; //Access_bits[7]
                        card_data[8] = 0x80; //Access_bits[8]
                        card_data[9] = 0x88; //user_byte[9]
                        card_data[10] = 0x88; //user_byte[10]
                        card_data[11] = 0x88; //user_byte[11]
                        card_data[12] = 0x88; //user_byte[12]
                        card_data[13] = 0x88; //user_byte[13]
                        card_data[14] = 0x88; //user_byte[14]
                        card_data[15] = 0x88; //user_byte[15]

                        status = mfrc522_write(3, card_data);
                        if (status == MFRC522_OK)
                        {
                            const char * msg2 = "Card Set!\r\n";
                            msg_num = strlen(msg2);
                            USART2_Send(msg2, msg_num);
                        }
                    }
                    else if (status != MFRC522_OK)
                    {
                        const char * msg = "Auth. BAD. New card!\r\n";
                        msg_num = strlen(buffer);
                        USART2_Send(msg, msg_num);
                    }
                    mfrc522_stop_crypto1();
                }
            }
        }
        mfrc522_halt();
    }
}