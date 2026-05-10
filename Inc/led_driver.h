#ifndef __LED_DRIVER_H__
#define __LED_DRIVER_H__

#include "stm32f4xx.h"

#define LED_NUM 2
#define LED_PATTERN_CYCLES 3

#define LED_GREEN 0
#define LED_RED 1

typedef enum {
    LED_STATUS_OFF = 0,
    LED_STATUS_ON = 1,
    LED_STATUS_PATTERN = 2
} led_status_t;

//PA12 red led
//PA11 green led

void led_init(void);
void led_turn_on(uint8_t led);
void led_turn_off(uint8_t led);
void led_play_pattern(uint8_t led);
led_status_t get_led_status(uint8_t led);


#endif // __LED_DRIVER_H__