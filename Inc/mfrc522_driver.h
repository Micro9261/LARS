#ifndef __MFRC522_DRIVER_H
#define __MFRC522_DRIVER_H

#include "MFRC522.h"
#include "stm32f4xx.h"

/*
Uses SPI2
SCK  -> PB13
MISO -> PB14
MOSI -> PB15
SS   -> PB9
RST  -> PB2
IRQ  -> PB1
8MHz SPI clock, mode 0, MSB first
TIM2 for delays and timeouts
*/

void mfrc522_init(void);
uint8_t mfrc522_version(void);
uint8_t mfrc522_self_test(void);
uint8_t mfrc522_card_present(void);

#endif /* __MFRC522_DRIVER_H */