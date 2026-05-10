#include "mfrc522_driver.h"
#include "MFRC522.h"
#include "stm32f446xx.h"
#include <stdint.h>

#define MAX_LEN 16

static uint8_t cardstr[17] = {0};


static void spi_slave_select(void)
{
    GPIOB->BSRR = GPIO_BSRR_BR9; // Set PB9 low to select slave
}

static void spi_slave_deselect(void)
{
    GPIOB->BSRR = GPIO_BSRR_BS9; // Set PB9 high to deselect slave
}

static void delay_ms(uint16_t ms)
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

static void write_register(uint8_t reg, uint8_t val)
{
    spi_slave_select();
    uint8_t address = (reg << 1) & 0x7E; // Address format for MFRC522
    SPI2->DR = address; // Send address
    while ( !(SPI2->SR & SPI_SR_RXNE) ); // Wait until transmit buffer is empty
    SPI2->DR;
    SPI2->DR = val; // Send value
    while ( !(SPI2->SR & SPI_SR_RXNE) ); // Wait until transmit buffer is empty
    SPI2->DR;
    spi_slave_deselect();
}

static uint8_t read_register(uint8_t reg)
{
    spi_slave_select();

    uint8_t address = ((reg << 1) & 0x7E) | 0x80; // Address format for MFRC522 with read bit
    SPI2->DR = address; // Send address
    while ( !(SPI2->SR & SPI_SR_RXNE) ); // Wait until transmit buffer is empty
    SPI2->DR; //dummy read

    SPI2->DR = 0x00;
    while ( !(SPI2->SR & SPI_SR_RXNE) ); // Wait until transmit buffer is empty

    spi_slave_deselect();

    return SPI2->DR; // Read and return value
}

static void set_bit_mask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp;
    tmp = read_register(reg);
    write_register(reg, tmp | mask);
}

static void clear_bit_mask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp;
    tmp = read_register(reg);
    write_register(reg, tmp & (~mask));
}

static void mfrc522_registers_init(void)
{
    //Reset baud rates
    write_register(MFRC522_TXMODE_REG, 0x0);
    write_register(MFRC522_RXMODE_REG, 0x0);
    //Reset ModWidthReg
    write_register(MFRC522_MODWIDTH_REG, 0x26);

    write_register(MFRC522_TMODE_REG, 0x80);
    write_register(MFRC522_TPRESCALER_REG, 0xA9);
    write_register(MFRC522_TRELOAD_REG_LSB, 0x03);
    write_register(MFRC522_TRELOAD_REG_MSB, 0xE8);

    write_register(MFRC522_TXASK_REG, 0x40); // Force 100% ASK modulation
    write_register(MFRC522_MODE_REG, 0x30); // CRC initial value 0x6363

    //Enable antenna
    mfrc522_antenna_on();
}

void mfrc522_antenna_on(void)
{
    set_bit_mask(MFRC522_TXCONTROL_REG, 0x03);
}

void mfrc522_antenna_off(void)
{
    clear_bit_mask(MFRC522_TXCONTROL_REG, 0x03);
}

void mfrc522_reset(void)
{
    write_register(MFRC522_COMMAND_REG, MFRC522_SOFTRESET);
}

void mfrc522_init()
{
    //GPIOB clock enable
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    //Configure PB13, PB14, PB15 as alternate function (SPI2)
    GPIOB->MODER &= ~(GPIO_MODER_MODE13 | GPIO_MODER_MODE14 | GPIO_MODER_MODE15);
    GPIOB->MODER |= (GPIO_MODER_MODE13_1 | GPIO_MODER_MODE14_1 | GPIO_MODER_MODE15_1);
    GPIOB->AFR[1] &= ~(GPIO_AFRH_AFSEL13 | GPIO_AFRH_AFSEL14 | GPIO_AFRH_AFSEL15);
    GPIOB->AFR[1] |= (0x5 << GPIO_AFRH_AFSEL13_Pos) | (0x5 << GPIO_AFRH_AFSEL14_Pos) | (0x5 << GPIO_AFRH_AFSEL15_Pos);

    //Configure PB2 and PB1 as input (RST and IRQ)
    GPIOB->MODER &= ~(GPIO_MODER_MODE2 | GPIO_MODER_MODE1);
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD2 | GPIO_PUPDR_PUPD1);
    GPIOB->PUPDR |= (GPIO_PUPDR_PUPD2_0 | GPIO_PUPDR_PUPD1_0);

    //Configure PB9 as output
    GPIOB->MODER &= ~GPIO_MODER_MODE9_Msk;
    GPIOB->MODER |= GPIO_MODER_MODE9_0;

    spi_slave_deselect();

    //MFRC522 Reset
    if ( (GPIOB->IDR & GPIO_IDR_ID2) == 0 ) // If reset pin is low, set it high
    {
        GPIOB->MODER |= GPIO_MODER_MODE2_0; // Set PB2 as output
        GPIOB->BSRR = GPIO_BSRR_BR2;
        delay_ms(1);
        GPIOB->BSRR = GPIO_BSRR_BS2; // Set PB2 high
        delay_ms(50); // Wait for MFRC522 to reset
    }

    //SPI2 clock enable
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

    //SPI2 configuration: Master mode, 8-bit data, CPOL=0, CPHA=0, MSB first, baud rate = fPCLK/2
    SPI2->CR1 = SPI_CR1_MSTR; // Master mode, software slave management, baud rate fPCLK/2
    SPI2->CR2 = SPI_CR2_SSOE; // Enable SS output for master mode
    SPI2->CR1 |= SPI_CR1_SPE; // Enable SPI2

    mfrc522_registers_init();
}

uint8_t mfrc522_version(void)
{
    uint8_t version = read_register(MFRC522_VERSION_REG); // Read VersionReg
    return version;
}

//Find cards, read card type number
// Input parameters: reqMode - find cards way
//  *   TagType - Return Card Type
//  *    0x4400 = Mifare_UltraLight
//  *    0x0400 = Mifare_One(S50)
//  *    0x0200 = Mifare_One(S70)
//  *    0x0800 = Mifare_Pro(X)
//  *    0x4403 = Mifare_DESFire
//  * Return value: the successful return MI_OK

uint8_t mfrc522_request(uint8_t req_mode, uint8_t * tag_type)
{
    uint8_t status = MFRC522_OK;
    uint16_t data_rx_len = 0;

    write_register(MFRC522_BITFRAMING_REG, 0x07);
    tag_type[0] = req_mode;

    status = mfrc522_to_card(MFRC522_TRANSCEIVE, tag_type, 
    1, tag_type, &data_rx_len);
    
    if ( (status != MFRC522_OK) || (data_rx_len != 0x10))
    {
        status = MFRC522_ERR;
    }
    
    return status;
}

uint8_t mfrc522_to_card(uint8_t cmd,
                        const uint8_t * data_tx,
                        uint8_t data_tx_num,
                        uint8_t * data_rx,
                        uint16_t * data_rx_num)
{
    uint8_t status = MFRC522_ERR;
    uint8_t irq_en = 0x00;
    uint8_t wait_irq = 0x00;
    uint8_t last_bits = 0x00;
    uint8_t n = 0x00;
    uint16_t i = 0x00;

    switch (cmd) 
    {
        case MFRC522_MFAUTHENT: //Certification cards close
            {
                irq_en = 0x12;
                wait_irq = 0x10;
                break;
            }
        case MFRC522_TRANSCEIVE: //transmit FIFO data
            {
                irq_en = 0x77;
                wait_irq = 0x30;
                break;
            }
        default:
            break;
    }

    write_register(MFRC522_COMMIEN_REG, irq_en | 0x80);
    clear_bit_mask(MFRC522_COMMIRQ_REG, 0x80);
    set_bit_mask(MFRC522_FIFOLEVEL_REG, 0x80);

    write_register(MFRC522_COMMAND_REG, MFRC522_IDLE);

    for (i = 0; i < data_tx_num; i++)
    {
        write_register(MFRC522_FIFODATA_REG, data_tx[i]);
    }

    write_register(MFRC522_COMMAND_REG, cmd);
    if (MFRC522_TRANSCEIVE == cmd)
    {
        set_bit_mask(MFRC522_BITFRAMING_REG, 0x80);
    }

    i = 20000;
    do 
    {
        n = read_register(MFRC522_COMMIRQ_REG);
        i--;
    }
    while ( (i != 0) && !(n & 0x01) && !(n & wait_irq) );

    clear_bit_mask(MFRC522_BITFRAMING_REG, 0x80);

    if ( 0 != i )
    {
        uint8_t test = read_register(MFRC522_ERROR_REG);
        if ( !(test & 0x1B) )
        {
            status = MFRC522_OK;
            if ( n & irq_en & 0x01 )
            {
                status = MFRC522_NOTAGERR;
            }

            if ( MFRC522_TRANSCEIVE == cmd)
            {
                n = read_register(MFRC522_FIFOLEVEL_REG);
                last_bits = read_register(MFRC522_CONTROL_REG) & 0x07;
                
                if (last_bits)
                {
                    *data_rx_num = (n-1) * 8 + last_bits;
                }
                else
                {
                    *data_rx_num = n * 8;
                }

                if ( n == 0)
                {
                    n = 1;
                }

                if ( n > MAX_LEN )
                {
                    n = MAX_LEN;
                }

                for (i = 0; i < n; i++)
                {
                    data_rx[i] = read_register(MFRC522_FIFODATA_REG);
                }
            }
        }
        else
        {
            status = MFRC522_ERR;
        }
    }
    else
    {
        //request timed out
    }

    return  status;
}

uint8_t mfrc522_anticoll(uint8_t * serial_num)
{
    uint8_t status = MFRC522_OK;
    uint8_t i = 0;
    uint8_t serial_num_check = 0;
    uint16_t un_len = 0;

    write_register(MFRC522_BITFRAMING_REG, 0x00);

    serial_num[0] = MFRC522_PICC_ANTICOLL;
    serial_num[1] = 0x20;
    status = mfrc522_to_card(MFRC522_TRANSCEIVE, serial_num,
        2, serial_num, &un_len);
    
    if (status == MFRC522_OK)
    {
        for (i = 0; i < 4; i++)
        {
            serial_num_check ^= serial_num[i];
        }

        if (serial_num_check != serial_num[i])
        {
            status = MFRC522_ERR;
        }
    }
    
    return  status;
}

uint8_t mfrc522_read(uint8_t block_addr, uint8_t * data_rx)
{
    uint8_t status = MFRC522_OK;
    uint16_t data_rx_num = 0;

    data_rx[0] = MFRC522_PICC_READ;
    data_rx[1] = block_addr;
    mfrc522_calculate_crc(data_rx, 2, &data_rx[2]);
    status = mfrc522_to_card(MFRC522_TRANSCEIVE, data_rx,
        4, data_rx, &data_rx_num);
    
    if ( (status != MFRC522_OK) || (data_rx_num != 0x09))
    {
        status = MFRC522_ERR;
    }

    return status;
}

uint8_t mfrc522_write(uint8_t block_addr, uint8_t * data_tx)
{
    uint8_t status = MFRC522_OK;
    uint16_t data_rx_num = 0;
    uint8_t i = 0;
    uint8_t buffer[18];

    buffer[0] = MFRC522_PICC_WRITE;
    buffer[1] = block_addr;
    mfrc522_calculate_crc(buffer, 2, &buffer[2]);
    status = mfrc522_to_card(MFRC522_TRANSCEIVE, buffer,
        4, buffer, &data_rx_num);

    if ( (status != MFRC522_OK) )
    {
        status = MFRC522_ERR;
    }

    if ( status == MFRC522_OK )
    {
        for ( i = 0; i < 16; i++ )
        {
            buffer[i] = data_tx[i];
        }
        mfrc522_calculate_crc(buffer, 16, &buffer[16]);
        status = mfrc522_to_card(MFRC522_TRANSCEIVE, buffer, 
            18, buffer, &data_rx_num);
        
        if ( (status != MFRC522_OK) )
        {
            status = MFRC522_ERR;
        }
    }

    return status;
}

void mfrc522_calculate_crc(uint8_t * data_in, uint8_t data_num, uint8_t * data_out)
{
    uint8_t i = 0;
    uint8_t n = 0;

    clear_bit_mask(MFRC522_DIVIRQ_REG, 0x04);
    set_bit_mask(MFRC522_FIFOLEVEL_REG, 0x80);

    for (i = 0; i < data_num; i++)
    {
        write_register(MFRC522_FIFODATA_REG, data_in[i]);
    }
    write_register(MFRC522_COMMAND_REG, MFRC522_CALC_CRC);

    i = 0xFF;
    do
    {
        n = read_register(MFRC522_DIVIRQ_REG);
        // i--;
    }
    while ( (i != 0) && !(n & 0x04));

    data_out[0] = read_register(MFRC522_CRCRESULT_REG_LSB);
    data_out[1] = read_register(MFRC522_CRCRESULT_REG_MSB);
}

uint8_t mfrc522_auth(uint8_t auth_mode, uint8_t block_addr, uint8_t * sector_key, uint8_t * serial_num)
{
    uint8_t status = MFRC522_OK;
    uint16_t data_rx_num = 0;
    uint8_t i = 0;
    uint8_t buffer[12];

    buffer[0] = auth_mode;
    buffer[1] = block_addr;

    for ( i = 0; i < 6 ; i++ )
    {
        buffer[i + 2] = sector_key[i];
    }

    for ( i = 0; i < 4 ; i++ )
    {
        buffer[i + 8] = serial_num[i];
    }

    status = mfrc522_to_card(MFRC522_MFAUTHENT, buffer,
        12, buffer, &data_rx_num);

    uint8_t data = read_register(MFRC522_STATUS2_REG);
    if ( (status != MFRC522_OK) || ( !(data & 0x08) ))
    {
        status = MFRC522_ERR;
    }
    
    return status;
}

uint8_t mfrc522_select_tag(uint8_t * serial_num)
{
    uint8_t status = MFRC522_OK;
    uint8_t i = 0;
    uint8_t size = 0;
    uint16_t data_rx_num = 0;
    uint8_t buffer[9];

    buffer[0] = MFRC522_PICC_SElECTTAG;
    buffer[1] = 0x70;
    for (i = 0; i < 5 ; i++)
    {
        buffer[i + 2] = serial_num[i];
    }
    mfrc522_calculate_crc(buffer, 7, &buffer[7]);
    status = mfrc522_to_card(MFRC522_TRANSCEIVE, buffer,
        9, buffer, &data_rx_num);

    if ( (status == MFRC522_OK) && (data_rx_num == 0x18))
    {
        size = buffer[0];
    }
    else
    {
        size = 0;
    }

    return size;
}

uint8_t mfrc522_halt(void)
{
    uint8_t status = MFRC522_OK;
    uint16_t data_rx_num = 0;
    uint8_t buffer[4];

    buffer[0] = MFRC522_PICC_HALT;
    buffer[1] = 0x0;
    mfrc522_calculate_crc(buffer, 2, &buffer[2]);

    status = mfrc522_to_card(MFRC522_TRANSCEIVE, buffer,
        4, buffer, &data_rx_num);
    return status;
}

void mfrc522_stop_crypto1(void)
{
    clear_bit_mask(MFRC522_STATUS2_REG, 0x08);
}

uint8_t mfrc522_card_present(void)
{
    uint8_t status = MFRC522_ERR;
    status = mfrc522_request(MFRC522_PICC_REQIDL, cardstr);
    return status;
}

uint8_t mfrc522_get_card_uid(uint8_t * uid, uint8_t * uid_len)
{
    uint8_t status = MFRC522_ERR;
    status = mfrc522_anticoll(cardstr);
    if (status == MFRC522_OK)
    {
        for (uint8_t i = 0; i < 5; i++)
        {
            uid[i] = cardstr[i];
        }
        *uid_len = 5;
    }
    mfrc522_halt();
    return status;
}