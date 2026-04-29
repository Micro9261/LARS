#include "usart_driver.h"
#include "stm32f446xx.h"
#include <string.h>

typedef struct
{
    uint8_t data[USART2_TX_BUFFER_SIZE];
    uint16_t transfer_start_pointer;
    uint16_t transfer_end_pointer;
    uint16_t new_data_pointer;
} USART2_TxBuffer;

static USART2_TxBuffer usart2_tx_buffer;

/****************** Helper functions *******************************/

static uint8_t get_available_space()
{
    uint8_t space = 0;
    if (usart2_tx_buffer.transfer_start_pointer == usart2_tx_buffer.transfer_end_pointer)
    {
        space = USART2_TX_BUFFER_SIZE;
    }
    else
    {
        if (usart2_tx_buffer.new_data_pointer > usart2_tx_buffer.transfer_start_pointer)
        {
            space = (USART2_TX_BUFFER_SIZE - usart2_tx_buffer.new_data_pointer) + usart2_tx_buffer.transfer_start_pointer;
        }
        else
        {
            space = usart2_tx_buffer.transfer_start_pointer - usart2_tx_buffer.new_data_pointer;
        }
    }
    return space;
}

/******************* API functions *********************************/

void USART2_Init()
{
    //USART2 TX (PA2) init
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; // Enable GPIOA clock
    GPIOA->MODER &= ~GPIO_MODER_MODER2; // Clear mode
    GPIOA->MODER |= GPIO_MODER_MODER2_1; // Alternate function mode
    GPIOA->AFR[0] &= ~GPIO_AFRL_AFRL2; // Clear AF
    GPIOA->AFR[0] |= (0x7 << GPIO_AFRL_AFSEL2_Pos); // AF7 for USART2

    //USART2 init
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN; // Enable USART2 clock
    USART2->BRR = 16000000 / 115200;
    USART2->CR3 = USART_CR3_DMAT; // Enable DMA transmitter
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE; // Enable USART and transmitter

    //Struct init
    usart2_tx_buffer.transfer_start_pointer = 0;
    usart2_tx_buffer.transfer_end_pointer = 0;
    usart2_tx_buffer.new_data_pointer = 0;

    //DMA1 Stream 6 init for USART2 TX (Channel 4)
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN; // Enable DMA1 clock
    DMA1_Stream6->CR = 0x0; // Disable stream and clear config
    while(DMA1_Stream6->CR & DMA_SxCR_EN); // Wait until disabled
    DMA1->HIFCR = DMA_HIFCR_CFEIF4 | DMA_HIFCR_CDMEIF4 | DMA_HIFCR_CTEIF4 | DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTCIF4; // Clear flags
    DMA1_Stream6->CR = (0x4 << DMA_SxCR_CHSEL_Pos) | //Channel 4 for USART2 TX
                        (0x0 << DMA_SxCR_MBURST_Pos) | // Single memory transfer
                        (0x0 << DMA_SxCR_PBURST_Pos) | // Single peripheral transfer
                        (0x0 << DMA_SxCR_DBM_Pos) | // No double buffer
                        (0x0 << DMA_SxCR_PL_Pos) | // Low priority
                        (0x0 << DMA_SxCR_PINCOS_Pos) | // Peripheral increment mode disabled
                        (0x0 << DMA_SxCR_MSIZE_Pos) | // 8-bit memory size
                        (0x0 << DMA_SxCR_PSIZE_Pos) | // 8-bit peripheral size
                        (0x1 << DMA_SxCR_MINC_Pos) | // Memory increment mode enabled
                        (0x0 << DMA_SxCR_PINC_Pos) | // Peripheral increment mode disabled
                        (0x0 << DMA_SxCR_CIRC_Pos) | // Circular mode disabled
                        (0x1 << DMA_SxCR_DIR_Pos) | // Memory to peripheral
                        (0x0 << DMA_SxCR_PFCTRL_Pos) | // Peripheral flow controller drisbled
                        (0x1 << DMA_SxCR_TCIE_Pos); // Transfer complete interrupt enable

    DMA1_Stream6->PAR = (uint32_t)&(USART2->DR); // Peripheral address
    NVIC_EnableIRQ(DMA1_Stream6_IRQn); // Enable DMA1 Stream 6 interrupt
}

void USART2_Send(const char * msg, uint8_t msg_num)
{
    uint8_t available_space = get_available_space();
    if (msg_num == 0)
    {
        return; // No data to send
    }
    else if (msg_num > available_space)
    {
        msg_num = available_space; // Limit to available space
    }


}

void DMA1_Stream6_IRQHandler(void)
{
    if (DMA1->HISR & DMA_HISR_TCIF6) // Transfer complete
    {
        DMA1->HIFCR |= DMA_HIFCR_CTCIF6; // Clear transfer complete flag
        DMA1_Stream6->CR &= ~DMA_SxCR_EN; // Disable stream
    }

    if (DMA1->HISR & (DMA_HISR_TEIF6 | DMA_HISR_DMEIF6 | DMA_HISR_FEIF6)) // Error flags
    {
        DMA1->HIFCR |= (DMA_HIFCR_CTEIF6 | DMA_HIFCR_CDMEIF6 | DMA_HIFCR_CFEIF6); // Clear error flags
        DMA1_Stream6->CR &= ~DMA_SxCR_EN; // Disable stream
    }
}