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
#include <stdint.h>
#include <stm32f4xx.h>

#include <inttypes.h>
#include <string.h>
#include <stdio.h>

#include "fonts.h"
#include "system_stm32f4xx.h"
#include "usart_driver.h"
#include "keyboard_driver.h"
#include "lock_driver.h"
#include "mfrc522_driver.h"
#include "password_manager.h"
#include "lcd_driver.h"
#include "led_driver.h"


#define MAX_CHECKS 10

/****************** FUNCTIONS ***********************/
void delay_ms(uint16_t ms);
void delay_us(uint8_t us);
void add_card(uint8_t type);
void remove_card(void);
void read_password(uint8_t * input, uint8_t * max_len, uint8_t block);
void change_password(uint8_t type);
void admin_menu(void);
void user_menu(void);

//display functions
#define DISPLAY_CARD_SCAN 0
#define DISPLAY_CARD_ADDED 1
#define DISPLAY_CARD_REMOVED 2
#define DISPLAY_CARD_ERR 3
#define DISPLAY_CARD_BAD 4

#define DISPLAY_PASSWORD_GUESS 0
#define DISPLAY_PASSWORD_ADMIN 1
#define DISPLAY_PASSWORD_USER 2

void display_logo(void);
void display_lock_status(uint8_t open);
void display_admin_mode(uint8_t admin);
void display_blocked(uint8_t blocked);
void display_card_notification(uint8_t type);
void display_password(uint8_t digits, uint8_t type);
void clear_info_section(void);


int main(void)
{
    SystemInit();

    //Init components
    USART2_Init();
    lock_init();
    keyboard_init();
    mfrc522_init();
    pass_manager_init();
    lock_init();
    led_init();

    char buffer[50];
    sprintf(buffer, "MFRC522 Version: 0x%02X\r\n", mfrc522_version());
    uint8_t msg_num = strlen(buffer);
    USART2_Send(buffer, msg_num);
    ST7735_FillScreen(ST7735_WHITE);
    while(1)
    {
        if (!keyboard_empty())
        {
            uint8_t input[MAX_PASSWORD_LEN] = {0};
            uint8_t in_cnt = MAX_PASSWORD_LEN;

            read_password(input, &in_cnt, false);

            if (in_cnt != 0)
            {
                if (pass_manager_check_password(input, in_cnt, PASSWORD_ADMIN) == PASSWORD_OK)
                {
                    admin_menu();
                }
                else if (pass_manager_check_password(input, in_cnt, PASSWORD_USER) == PASSWORD_OK)
                {
                    user_menu();
                }
            }
        }

        if (mfrc522_card_present() == MFRC522_OK)
        {
            uint8_t uid[5] = {0};
            uint8_t uid_len = 0;
            if (mfrc522_get_card_uid(uid,  &uid_len) == MFRC522_OK)
            {
                char buffer[50];
                sprintf(buffer, "CARD UID: %x %x %x %x %x\r\n", uid[0], uid[1], uid[2], uid[3], uid[4]);
                uint8_t msg_len = strlen(buffer);
                USART2_Send(buffer, msg_len);

                if (pass_manager_check_card(uid, uid_len, UID_TYPE_ADMIN) == CARD_OK)
                {
                    admin_menu();
                }
                else if (pass_manager_check_card(uid, uid_len, UID_TYPE_USER) == CARD_OK)
                {
                    user_menu();
                }
            }
        }
    }
}


void delay_ms(uint16_t ms)
{
    //SysTick config
    SysTick->LOAD = 16000 - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = (1UL << SysTick_CTRL_CLKSOURCE_Pos) | (1UL << SysTick_CTRL_ENABLE_Pos);

    for (uint16_t i = 0; i < ms; i++)
    {
        while ( !(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) );
    }

    SysTick->CTRL = 0;
}

void delay_us(uint8_t us)
{
    //SysTick config
    SysTick->LOAD = 16 - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = (1UL << SysTick_CTRL_CLKSOURCE_Pos) | (1UL << SysTick_CTRL_ENABLE_Pos);

    for (uint16_t i = 0; i < us; i++)
    {
        while ( !(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) );
    }
    SysTick->CTRL = 0;
}

void add_card(uint8_t type)
{
    const char * msg = "Add Card!\r\n";
    uint8_t msg_len = strlen(msg);
    USART2_Send(msg, msg_len);

    key_t key = {.key = '\0', .long_press = 0};
    uint8_t card_status = mfrc522_card_present();
    while (card_status != MFRC522_OK) {
        if (keyboard_getchar(&key, false))
        {
            if (key.key == '0')
            {
                break;
            }
        }
        card_status = mfrc522_card_present();
    }

    if (card_status == MFRC522_OK)
    {
        uint8_t uid[5] = {0};
        uint8_t uid_len = 0;
        if (mfrc522_get_card_uid(uid,  &uid_len) == MFRC522_OK)
        {
            char buffer[50];
            sprintf(buffer, "Adding CARD UID: %x %x %x %x %x\r\n", uid[0], uid[1], uid[2], uid[3], uid[4]);
            uint8_t msg_len = strlen(buffer);
            USART2_Send(buffer, msg_len);

            if (pass_manager_add_card(uid, uid_len, type) == CARD_OK)
            {
                sprintf(buffer,"Card add success!\r\n");
                msg_len = strlen(buffer);
                USART2_Send(buffer, msg_len);
            }
            else
            {
                sprintf(buffer,"Card add failed!\r\n");
                msg_len = strlen(buffer);
                USART2_Send(buffer, msg_len);
            }
        }
    }
}

void remove_card(void)
{
    const char * msg = "Remove Card!\r\n";
    uint8_t msg_len = strlen(msg);
    USART2_Send(msg, msg_len);

    key_t key = {.key = '\0', .long_press = 0};
    uint8_t card_status = mfrc522_card_present();
    while (card_status != MFRC522_OK) {
        if (keyboard_getchar(&key, false))
        {
            if (key.key == '0')
            {
                break;
            }
        }
        card_status = mfrc522_card_present();
    }

    if (card_status == MFRC522_OK)
    {
        uint8_t uid[5] = {0};
        uint8_t uid_len = 0;
        if (mfrc522_get_card_uid(uid,  &uid_len) == MFRC522_OK)
        {
            char buffer[50];
            sprintf(buffer, "Removing CARD UID: %x %x %x %x %x\r\n", uid[0], uid[1], uid[2], uid[3], uid[4]);
            uint8_t msg_len = strlen(buffer);
            USART2_Send(buffer, msg_len);

            if (pass_manager_remove_card(uid, uid_len) == CARD_OK)
            {
                sprintf(buffer,"Card remove success!\r\n");
                msg_len = strlen(buffer);
                USART2_Send(buffer, msg_len);
            }
            else
            {
                sprintf(buffer,"Card remove failed!\r\n");
                msg_len = strlen(buffer);
                USART2_Send(buffer, msg_len);
            }
        }
    }
}

void read_password(uint8_t * input, uint8_t * max_len, uint8_t block)
{
    *max_len = 0;

    key_t key = {.key = '\0', .long_press = 0};
    for (uint8_t checks = 0; checks < MAX_CHECKS; checks++)
    {
        if (keyboard_getchar(&key, block))
        {
            if (key.key == '#')
            {
                break;
            }

            input[*max_len] = key.key;
            *max_len = *max_len + 1;
            if (*max_len >= MAX_PASSWORD_LEN)
            {
                break;
            }
        }
        delay_ms(500);
    }
}

void change_password(uint8_t type)
{
    const char * pass_type = "";
    if (type == PASSWORD_ADMIN)
    {
        pass_type = "[admin]";
    }
    else if (type == PASSWORD_USER)
    {
        pass_type = "[user]";
    }
    else
    {
        return;
    }

    char buffer[50];
    sprintf(buffer, "Change password! %s\r\n", pass_type);
    uint8_t msg_len = strlen(buffer);
    USART2_Send(buffer, msg_len);

    uint8_t input[MAX_PASSWORD_LEN] = {0};
    uint8_t in_cnt = MAX_PASSWORD_LEN;

    read_password(input, &in_cnt, true);

    if (pass_manager_set_password(input, in_cnt, type) == PASSWORD_OK)
    {
        sprintf(buffer, "Password for %s change success!\r\n", pass_type);
        msg_len = strlen(buffer);
        USART2_Send(buffer, msg_len);
    }
    else
    {
        sprintf(buffer, "Password for %s change failed!\r\n", pass_type);
        msg_len = strlen(buffer);
        USART2_Send(buffer, msg_len);
    }
}

void admin_menu(void)
{
    const char * msg = "Admin actions!\r\n";
    uint8_t msg_len = strlen(msg);
    USART2_Send(msg, msg_len);

    key_t key = {.key = '\0', .long_press = 0};
    while (1)
    {
        if (keyboard_getchar(&key, false))
        {
            uint8_t exit_menu = false;
            switch (key.key) {
                case '1':
                    add_card(UID_TYPE_USER);
                    break;

                case '2':
                    add_card(UID_TYPE_ADMIN);
                    break;
                
                case '3':
                    remove_card();
                    break;
                
                case 'A':
                    change_password(PASSWORD_USER);
                    break;

                case  'B':
                    change_password(PASSWORD_ADMIN);
                    break;

                case '0':
                    exit_menu = true;
                    break;

                default:
                    break;
            }

            if (exit_menu == true)
            {
                break;
            }
        }
    }

    msg = "Admin actions end!\r\n";
    msg_len = strlen(msg);
    USART2_Send(msg, msg_len);
}

void user_menu(void)
{
    const char * msg = "User actions!\r\n";
    uint8_t msg_len = strlen(msg);
    USART2_Send(msg, msg_len);

    lock_open();
    delay_ms(3000);
    lock_close();

    msg = "User actions end!\r\n";
    msg_len = strlen(msg);
    USART2_Send(msg, msg_len);
}

void display_logo(void)
{
    ST7735_WriteString(16, 0, "SECURITY", Font_16x26, ST7735_BLUE, ST7735_WHITE);
    ST7735_WriteString(48, 26, "LOCK", Font_16x26, ST7735_BLUE, ST7735_WHITE);
}

void display_lock_status(uint8_t open)
{
    ST7735_FillRectangle(47, 2*26 + 4, 52 + 6* 11, 2*26 + 4 + 18, uint16_t color);
    if (open == true)
    {
        ST7735_WriteString(47 + 11, 2*26 + 4, "OPEN", Font_11x18, ST7735_GREEN, ST7735_WHITE);
    }
    else
    {
        ST7735_WriteString(47, 2*26 + 4, "CLOSED", Font_11x18, ST7735_RED, ST7735_WHITE);
    }
}

void display_admin_mode(uint8_t admin)
{
    if (admin == true)
    {
        ST7735_WriteString(0, 2*26, "A", Font_16x26, ST7735_MAGENTA, ST7735_WHITE);
    }
    else
    {
        ST7735_WriteString(0, 2*26, " ", Font_16x26, ST7735_MAGENTA, ST7735_WHITE);
    }
}

void display_blocked(uint8_t blocked)
{
    if (blocked == true)
    {
        ST7735_WriteString(160 - 27, 2*26, "B", Font_16x26, ST7735_RED, ST7735_WHITE);
    }
    else
    {
        ST7735_WriteString(160 - 27, 2*26, " ", Font_16x26, ST7735_MAGENTA, ST7735_WHITE);
    }
}

void display_card_notification(uint8_t type)
{
    
}

void display_password(uint8_t digits, uint8_t type)
{

}

void clear_info_section(void)
{
    ST7735_FillRectangle(0, 2*26 + 4 + 24, ST7735_WIDTH, ST7735_HEIGHT, ST7735_WHITE);
}