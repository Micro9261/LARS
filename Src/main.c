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
void read_password(uint8_t * input, uint8_t * max_len, uint8_t block, uint8_t type);
void change_password(uint8_t type);
void admin_menu(void);
void user_menu(void);

//display functions
#define DISPLAY_CARD_SCAN 0
#define DISPLAY_CARD_ADDED 1
#define DISPLAY_CARD_REMOVED 2
#define DISPLAY_CARD_ERR 3
#define DISPLAY_CARD_BAD 4
#define DISPLAY_PASSWORD_SAVED 5
#define DISPLAY_PASSWORD_ERR 6

#define DISPLAY_PASSWORD_GUESS 0
#define DISPLAY_PASSWORD_ADMIN 1
#define DISPLAY_PASSWORD_USER 2

void display_logo(void);
void display_lock_status(uint8_t open);
void display_admin_mode(uint8_t admin);
void display_blocked(uint8_t blocked);
void display_notification(uint8_t type);
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
    ST7735_Init();
    led_init();

    char buffer[50];
    sprintf(buffer, "MFRC522 Version: 0x%02X\r\n", mfrc522_version());
    uint8_t msg_num = strlen(buffer);
    USART2_Send(buffer, msg_num);
    ST7735_FillScreen(ST7735_WHITE);
    display_logo();
    display_lock_status(false);
    led_turn_on(LED_RED);
    led_turn_off(LED_GREEN);
    while(1)
    {
        if (!keyboard_empty())
        {
            uint8_t input[MAX_PASSWORD_LEN] = {0};
            uint8_t in_cnt = MAX_PASSWORD_LEN;

            read_password(input, &in_cnt, false, PASSWORD_UNKOWN);
            clear_info_section();
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
    display_notification(DISPLAY_CARD_SCAN);
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
                display_notification(DISPLAY_CARD_ADDED);
                led_turn_on(LED_GREEN);
                sprintf(buffer,"Card add success!\r\n");
                msg_len = strlen(buffer);
                USART2_Send(buffer, msg_len);
                delay_ms(1000);
                led_turn_off(LED_GREEN);
            }
            else
            {
                led_play_pattern(LED_RED);
                display_notification(DISPLAY_CARD_ERR);
                sprintf(buffer,"Card add failed!\r\n");
                msg_len = strlen(buffer);
                USART2_Send(buffer, msg_len);
                delay_ms(1000);
            }
        }
    }
}

void remove_card(void)
{
    display_notification(DISPLAY_CARD_SCAN);
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
                led_turn_on(LED_GREEN);
                display_notification(DISPLAY_CARD_REMOVED);
                sprintf(buffer,"Card remove success!\r\n");
                msg_len = strlen(buffer);
                USART2_Send(buffer, msg_len);
                delay_ms(1000);
                led_turn_off(LED_GREEN);
            }
            else
            {
                led_play_pattern(LED_RED);
                display_notification(DISPLAY_CARD_ERR);
                sprintf(buffer,"Card remove failed!\r\n");
                msg_len = strlen(buffer);
                USART2_Send(buffer, msg_len);
                delay_ms(1000);
            }
        }
    }
}

void read_password(uint8_t * input, uint8_t * max_len, uint8_t block, uint8_t type)
{
    *max_len = 0;
    switch (type) 
    {
        case PASSWORD_ADMIN:
            display_password(0, DISPLAY_PASSWORD_ADMIN);
            break;
        
        case PASSWORD_USER:
            display_password(0, DISPLAY_PASSWORD_USER);
            break;

        default:
            display_password(0, DISPLAY_PASSWORD_GUESS);
            break;
    }
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
            switch (type) 
            {
                case PASSWORD_ADMIN:
                    display_password(*max_len, DISPLAY_PASSWORD_ADMIN);
                    break;
                
                case PASSWORD_USER:
                    display_password(*max_len, DISPLAY_PASSWORD_USER);
                    break;

                default:
                    display_password(*max_len, DISPLAY_PASSWORD_GUESS);
                    break;
            }
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

    read_password(input, &in_cnt, true, type);

    if (pass_manager_set_password(input, in_cnt, type) == PASSWORD_OK)
    {
        led_turn_on(LED_GREEN);
        display_notification(DISPLAY_PASSWORD_SAVED);
        sprintf(buffer, "Password for %s change success!\r\n", pass_type);
        msg_len = strlen(buffer);
        USART2_Send(buffer, msg_len);
        delay_ms(1000);
        led_turn_off(LED_GREEN);
    }
    else
    {
        led_play_pattern(LED_RED);
        display_notification(DISPLAY_PASSWORD_ERR);
        sprintf(buffer, "Password for %s change failed!\r\n", pass_type);
        msg_len = strlen(buffer);
        USART2_Send(buffer, msg_len);
        delay_ms(1000);
    }
}

void admin_menu(void)
{
    display_admin_mode(true);
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
                
                case '7':
                    lock_block();
                    if (LOCK_STATUS_BLOCKED == get_lock_status())
                    {
                        display_blocked(true);
                    }
                    break;

                case '9':
                    lock_unblock();
                    if (LOCK_STATUS_BLOCKED != get_lock_status())
                    {
                        display_blocked(false);
                    }
                    break;

                case '#':
                    user_menu();
                    exit_menu = true;
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
        clear_info_section();
    }

    msg = "Admin actions end!\r\n";
    msg_len = strlen(msg);
    USART2_Send(msg, msg_len);
    display_admin_mode(false);
}

void user_menu(void)
{
    const char * msg = "User actions!\r\n";
    uint8_t msg_len = strlen(msg);
    USART2_Send(msg, msg_len);

    lock_open();
    if (LOCK_STATUS_OPEN == get_lock_status())
    {
        led_turn_off(LED_RED);
        led_play_pattern(LED_GREEN);
        display_lock_status(true);
    }

    delay_ms(1000);

    lock_close();
    if (LOCK_STATUS_CLOSED == get_lock_status())
    {
        led_turn_on(LED_RED);
        display_lock_status(false);
    }

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
    ST7735_FillRectangle(47, 2*26 + 4, 52 + 6* 11, 2*26 + 4 + 18, ST7735_WHITE);
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

void display_notification(uint8_t type)
{
    clear_info_section();
    switch (type) 
    {
        case DISPLAY_CARD_SCAN:
            ST7735_WriteString(0, 2*26 + 4 + 24, "Card scan...", Font_7x10,ST7735_BLACK, ST7735_WHITE);
            break;
        
        case DISPLAY_CARD_ADDED:
            ST7735_WriteString(0, 2*26 + 4 + 24, "Card added!", Font_7x10, ST7735_BLACK, ST7735_WHITE);
            break;

        case DISPLAY_CARD_REMOVED:
            ST7735_WriteString(0, 2*26 + 4 + 24, "Card removed!", Font_7x10, ST7735_BLACK, ST7735_WHITE);
            break;

        case DISPLAY_CARD_ERR:
            ST7735_WriteString(0, 2*26 + 4 + 24, "Card error!", Font_7x10, ST7735_BLACK, ST7735_WHITE);
            break;

        case DISPLAY_CARD_BAD:
            ST7735_WriteString(0, 2*26 + 4 + 24, "Card wrong!", Font_7x10, ST7735_BLACK, ST7735_WHITE);
            break;

        case DISPLAY_PASSWORD_SAVED:
            ST7735_WriteString(0, 2*26 + 4 + 24, "PASS changed!", Font_7x10, ST7735_BLACK, ST7735_WHITE);
            break;

        case DISPLAY_PASSWORD_ERR:
            ST7735_WriteString(0, 2*26 + 4 + 24, "PASS error!", Font_7x10, ST7735_BLACK, ST7735_WHITE);
            break;

        default:
            break;
    }
}

void display_password(uint8_t digits, uint8_t type)
{
    clear_info_section();
    char buffer[MAX_PASSWORD_LEN + 10 + 1];
    uint8_t msg_len;
    switch (type) 
    {
        case DISPLAY_PASSWORD_GUESS:
            msg_len = 5;
            strncpy(buffer, "PASS:",msg_len);
            while (digits--)
            {
                buffer[msg_len++] = '*';
            }
            buffer[msg_len] = '\0';
            ST7735_WriteString(0, 2*26 + 4 + 24, buffer, Font_7x10,ST7735_BLACK, ST7735_WHITE);
            break;
        
        case DISPLAY_PASSWORD_ADMIN:
            msg_len = 10;
            strncpy(buffer, "N PASS[A]:",msg_len);
            while (digits--)
            {
                buffer[msg_len++] = '*';
            }
            buffer[msg_len] = '\0';
            ST7735_WriteString(0, 2*26 + 4 + 24, buffer, Font_7x10, ST7735_BLACK, ST7735_WHITE);
            break;

        case DISPLAY_PASSWORD_USER:
            msg_len = 7;
            strncpy(buffer, "N PASS:",msg_len);
            while (digits--)
            {
                buffer[msg_len++] = '*';
            }
            buffer[msg_len] = '\0';
            ST7735_WriteString(0, 2*26 + 4 + 24, buffer, Font_7x10, ST7735_BLACK, ST7735_WHITE);
            break;

        default:
            break;
    }
}

void clear_info_section(void)
{
    ST7735_FillRectangle(0, 2*26 + 4 + 24, ST7735_WIDTH, ST7735_HEIGHT, ST7735_WHITE);
}