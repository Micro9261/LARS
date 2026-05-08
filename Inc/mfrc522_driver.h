#ifndef __MFRC522_DRIVER_H
#define __MFRC522_DRIVER_H

#include "stm32f4xx.h"

#define MFRC522_OK          0
#define MFRC522_NOTAGERR    1
#define MFRC522_ERR         2

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
void mfrc522_antenna_on(void);
void mfrc522_antenna_off(void);
void mfrc522_reset(void);
uint8_t mfrc522_version(void);
// uint8_t mfrc522_self_test(void);
// uint8_t mfrc522_card_present(void);
uint8_t mfrc522_request(uint8_t req_mode, uint8_t * tag_type);
uint8_t mfrc522_to_card(uint8_t cmd, const uint8_t * data_tx, uint8_t data_tx_num,
                        uint8_t * data_rx, uint16_t * data_rx_num);
uint8_t mfrc522_anticoll(uint8_t * serial_num);
uint8_t mfrc522_read(uint8_t block_addr, uint8_t * data_rx);
uint8_t mfrc522_write(uint8_t block_addr, uint8_t * data_tx);
void mfrc522_calculate_crc(uint8_t * data_in, uint8_t data_num, uint8_t * data_out);
uint8_t mfrc522_auth(uint8_t auth_mode, uint8_t block_addr, uint8_t * sector_key, uint8_t * serial_num);
uint8_t mfrc522_select_tag(uint8_t * serial_num);
uint8_t mfrc522_halt(void);
void mfrc522_stop_crypto1(void);

#endif /* __MFRC522_DRIVER_H */