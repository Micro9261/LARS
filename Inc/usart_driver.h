#ifndef __USART_DRIVER_H
#define __USART_DRIVER_H

#include <stm32f4xx.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#define USART2_TX_BUFFER_SIZE 200

void USART2_Init(void);
void USART2_Send(const char * msg, uint8_t msg_num);


#endif // __USART_DRIVER_H