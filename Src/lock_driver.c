#include "lock_driver.h"
#include "stm32f446xx.h"

static lock_status_t lock_status = LOCK_STATUS_CLOSED;

void lock_init()
{
    //GPIOA and GPIOC clock enable
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN;

    //PA5 as output
    GPIOA->MODER |= GPIO_MODER_MODER5_0;
    GPIOA->BSRR = GPIO_BSRR_BR5; // Set PA5 low (lock closed)

    //PC13 as input
    GPIOC->MODER &= ~GPIO_MODER_MODER13_Msk;
    GPIOC->PUPDR &= ~GPIO_PUPDR_PUPD13_Msk;
    GPIOC->PUPDR |= GPIO_PUPDR_PUPD13_0;

    lock_status = LOCK_STATUS_CLOSED;
}

void lock_open()
{
    if (lock_status == LOCK_STATUS_BLOCKED)
        return; // Cannot open if blocked

    GPIOA->BSRR = GPIO_BSRR_BS5; // Set PA5 high (lock open)
    lock_status = LOCK_STATUS_OPEN;
}

void lock_close()
{
    if (lock_status == LOCK_STATUS_BLOCKED)
        return; // Cannot close if blocked
    
    while (GPIOC->IDR & GPIO_IDR_ID13); //wait until doors opened
    GPIOA->BSRR = GPIO_BSRR_BR5; // Set PA5 low (lock closed)
    lock_status = LOCK_STATUS_CLOSED;
}

lock_status_t get_lock_status()
{
    return lock_status;
}

void lock_block()
{
    if (lock_status != LOCK_STATUS_BLOCKED)
    {
        lock_close(); // Ensure lock is closed when blocking
        lock_status = LOCK_STATUS_BLOCKED;
    }
}

void lock_unblock()
{
    if (lock_status == LOCK_STATUS_BLOCKED)
    {
        lock_status = LOCK_STATUS_CLOSED;
    }
}