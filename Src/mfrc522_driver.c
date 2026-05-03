#include "mfrc522_driver.h"
#include "MFRC522.h"
#include "stm32f446xx.h"

static volatile uint16_t tick_cnt = 0;

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
    tick_cnt = 0;
    while (tick_cnt < ms);
}

static void write_register(uint8_t reg, uint8_t val)
{
    SPI2->DR;

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

// static void write_registers(uint8_t reg, uint8_t count,const uint8_t * buffer)
// {
//     uint8_t address = (reg << 1) & 0x7E; // Address format for MFRC522
//     SPI2->DR = address; // Send address
//     while ( !(SPI2->SR & SPI_SR_TXE) ); // Wait until transmit buffer is empty

//     for (uint8_t i = 0; i < count; i++)
//     {
//         SPI2->DR = buffer[i]; // Send value
//         while ( !(SPI2->SR & SPI_SR_TXE) ); // Wait until transmit buffer is empty
//     }
// }

static uint8_t read_register(uint8_t reg)
{
    SPI2->DR;

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

static void mrfc_registers_init(void)
{
    //Reset baud rates
    write_register(MFRC522_TXMODE_REG, 0x0);
    write_register(MFRC522_RXMODE_REG, 0x0);
    //Reset ModWidthReg
    write_register(MFRC522_MODWIDTH_REG, 0x26);

    write_register(MFRC522_TMODE_REG, 0x80);
    write_register(MFRC522_TPRESCALER_REG, 0xA9);
    write_register(MFRC522_TRELOAD_REG_LSB, 0xE8);
    write_register(MFRC522_TRELOAD_REG_MSB, 0x03);

    write_register(MFRC522_TXASK_REG, 0x40); // Force 100% ASK modulation
    write_register(MFRC522_MODE_REG, 0x30); // CRC initial value 0x6363

    //Enable antenna
    uint8_t value = read_register(MFRC522_TXCONTROL_REG);
    if ( (value & 0x03) == 0 )
    {
        write_register(MFRC522_TXCONTROL_REG, value | 0x03);
    }
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

    //TIM2 clock enable
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    //TIM2 configuration 1ms tick
    TIM2->PSC = 16000 - 1; // Prescaler for 1MHz timer clock
    TIM2->ARR = 1000 - 1; // Auto-reload for 1ms tick
    TIM2->CR1 |= TIM_CR1_CEN; // Enable TIM2

    NVIC_EnableIRQ(TIM2_IRQn); // Enable TIM2 interrupt

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
    SPI2->CR1 = SPI_CR1_MSTR | (0x01 << SPI_CR1_BR_Pos); // Master mode, software slave management, baud rate fPCLK/2
    SPI2->CR2 = SPI_CR2_SSOE; // Enable SS output for master mode
    SPI2->CR1 |= SPI_CR1_SPE; // Enable SPI2

    mrfc_registers_init();
}

uint8_t mfrc522_version(void)
{
    uint8_t version = read_register(MFRC522_VERSION_REG); // Read VersionReg
    return version;
}

void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF) // Check for update interrupt
    {
        TIM2->SR &= ~TIM_SR_UIF; // Clear interrupt flag
        tick_cnt++; // Increment tick counter
    }
}