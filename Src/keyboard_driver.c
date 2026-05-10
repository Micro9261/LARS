#include "keyboard_driver.h"

//key state definitions
#define NO_ACTION (0x0)
#define PRESSED (0x1)
#define LONG_PRESS (0x2)
#define SCANNED (0x3)


static const uint8_t key_map[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};



static uint8_t key_states[4];
static uint8_t debounce_counters[4][4] = {0};


typedef struct {
    key_t data[KEYBOARD_BUFFER_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} keyboard_buffer_t;

static keyboard_buffer_t keyboard_buffer;

void keyboard_init()
{
    //GPIOC clock enable
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    //PC0-PC3 as input with pull-up
    GPIOC->MODER &= ~( GPIO_MODER_MODE0_Msk | GPIO_MODER_MODE1_Msk | GPIO_MODER_MODE2_Msk | GPIO_MODER_MODE3_Msk);
    GPIOC->PUPDR |= ( GPIO_PUPDR_PUPD0_0 | GPIO_PUPDR_PUPD1_0 | GPIO_PUPDR_PUPD2_0 | GPIO_PUPDR_PUPD3_0);

    //PC4-PC7 as output
    GPIOC->MODER |= ( GPIO_MODER_MODE4_0 | GPIO_MODER_MODE5_0 | GPIO_MODER_MODE6_0 | GPIO_MODER_MODE7_0);
    GPIOC->BSRR  = ( GPIO_BSRR_BR4 | GPIO_BSRR_BS5 | GPIO_BSRR_BS6 | GPIO_BSRR_BS7);

    //TIM6 clock enable
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;

    //TIM6 config for 5 ms interrupt
    TIM6->CR1 = 0;
    TIM6->CNT = 0;
    TIM6->PSC = 16000 - 1; // 1 ms tick
    TIM6->ARR = 5 - 1;     // 5 ms period
    TIM6->DIER = TIM_DIER_UIE;
    TIM6->CR1 |= TIM_CR1_CEN;

    //Initialize keyboard buffer
    keyboard_buffer.head = 0;
    keyboard_buffer.tail = 0;
    keyboard_buffer.count = 0;


    __NVIC_EnableIRQ(TIM6_DAC_IRQn);
}

uint8_t keyboard_empty()
{
    return keyboard_buffer.count == 0;
}

void keyboard_clear()
{
    keyboard_buffer.head = 0;
    keyboard_buffer.tail = 0;
    keyboard_buffer.count = 0;
}

uint8_t keyboard_getchar(key_t * key, uint8_t blocking)
{
    if ( !blocking && keyboard_empty() )
    {
        return 0; // No key available
    }

    while(keyboard_empty()); // Wait for key

    *key = keyboard_buffer.data[keyboard_buffer.tail];
    keyboard_buffer.tail = (keyboard_buffer.tail + 1) % KEYBOARD_BUFFER_SIZE;
    keyboard_buffer.count--;

    return 1; // Key available
}

uint8_t keyboard_get(char * buffer, uint8_t input_size, char terminator)
{
    uint8_t count = 0;
    key_t key;
    while (count < input_size && keyboard_getchar(&key, true))
    {
        buffer[count++] = key.key;
        if (key.key == terminator)
        {
            break;
        }
    }
    buffer[count] = '\0'; // Null-terminate the string
    return count; // Number of characters read
}

void TIM6_DAC_IRQHandler(void)
{
    if (TIM6->SR & TIM_SR_UIF)
    {
        static uint8_t col = 0; // Current column index (0-3)

        TIM6->SR &= ~TIM_SR_UIF; // Clear interrupt flag

        /*
        1. We read the current state of the rows.
        2. We set next column low.
        3. We check for key state changes and update debounce counters.
        4. If a key is detected as pressed for 5 ticks and less than 25 ticks,
        we add it to the buffer as quick press. If it's pressed for 25 tick or more,
        it's added as long press.
        */

        uint8_t row_states = ( GPIOC->IDR & 0x0F ); // Read rows (PC0-PC3)
        uint8_t col_state = ( GPIOC->ODR & 0xF0 ) >> 4; // Read current column (PC4-PC7)
        col_state |= 0xF0; // only actual column is low, rest are high
        
        // Set current column low, rest high
        uint8_t current_col = col;
        col = (col + 1) % 4; // Move to next column
        uint8_t mask = ~( 1UL << col ) << 4; // Create mask to set current column low
        uint8_t value = ( GPIOC->ODR & (~0xF0)) | ( mask & 0xF0 ); // Shift low column to the next one
        GPIOC->ODR &= ~(0xF0);
        GPIOC->ODR |= value;

        for (uint8_t row = 0; row < 4; row++)
        {
            uint8_t key_state = (row_states >> row) & 0x01; // Get state of current row
            key_t new_key = { .key = key_map[row][current_col], .long_press = 0 };
            uint8_t key_buffer_state = (key_states[current_col] >> (row * 2)) & 0x03; // Get current key state from key_states
            if (key_state == 0) // Key is pressed
            {
                debounce_counters[row][current_col]++;
                

                if (debounce_counters[row][current_col] >= 25 && (key_buffer_state == NO_ACTION)) // Cap debounce counter to prevent overflow
                {
                    key_states[current_col] |= (LONG_PRESS << (row * 2)); // Mark as long press
                    new_key.long_press = 1;
                }

                key_buffer_state = (key_states[current_col] >> (row * 2)) & 0x03; // Get current key state from key_states
                if ( key_buffer_state != NO_ACTION && key_buffer_state != SCANNED) // If key is still pressed, keep state
                {
                    key_states[current_col] &= ~(0x03 << row * 2); // Clear key state
                    key_states[current_col] |= (SCANNED << (row * 2)); // Mark as scanned but buffer full
                    if (keyboard_buffer.count >= 16)
                    {
                        
                        debounce_counters[row][current_col] = 0; // Reset debounce counter
                        continue; // Buffer full, skip adding new key
                    }

                    keyboard_buffer.data[keyboard_buffer.head] = new_key;
                    keyboard_buffer.head = (keyboard_buffer.head + 1) % KEYBOARD_BUFFER_SIZE;
                    keyboard_buffer.count++;
                }
            }
            else // Key is released
            {
                if (debounce_counters[row][current_col] >= 5 && (key_buffer_state == NO_ACTION) && (key_buffer_state != SCANNED)) // Cap debounce counter to prevent overflow
                {
                    key_states[current_col] &= ~(0x03 << row * 2); // Clear key state
                    key_states[current_col] |= (SCANNED << (row * 2)); // Mark as scanned but buffer full
                    if (keyboard_buffer.count >= 16)
                    {
                        
                        debounce_counters[row][current_col] = 0; // Reset debounce counter
                        continue; // Buffer full, skip adding new key
                    }

                    keyboard_buffer.data[keyboard_buffer.head] = new_key;
                    keyboard_buffer.head = (keyboard_buffer.head + 1) % KEYBOARD_BUFFER_SIZE;
                    keyboard_buffer.count++;
                }

                key_states[current_col] &= ~(0x03 << row * 2); // Clear key state, set as NO_ACTION
                debounce_counters[row][current_col] = 0; // Reset debounce counter
            }
        }
    }
}