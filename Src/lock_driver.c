#include "lock_driver.h"

static lock_status_t lock_status = LOCK_STATUS_CLOSED;

void lock_init()
{
    //GPIOA clock enable
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    //PA5 as output
    GPIOA->MODER |= GPIO_MODER_MODER5_0;
    GPIOA->BSRR = GPIO_BSRR_BR5; // Set PA5 low (lock closed)

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