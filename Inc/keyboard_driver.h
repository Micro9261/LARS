#ifndef __KEYBOARD_DRIVER_H
#define __KEYBOARD_DRIVER_H

#include <stm32f4xx.h>

/*
Columns: PC4, PC5, PC6, PC7 (Output)
Rows: PC0, PC1, PC2, PC3 (Input with pull-up)

Uses TIM6 for debouncing and long-press detection.

Key Mapping:
Row\Col | PC4 | PC5 | PC6 | PC7
--------|-----|-----|-----|-----
PC0     | 1   | 2   | 3   | A
PC1     | 4   | 5   | 6   | B
PC2     | 7   | 8   | 9   | C
PC3     | *   | 0   | #   | D
*/

#define KEYBOARD_BUFFER_SIZE 16
#define true 1
#define false 0

typedef struct {
    uint8_t key;
    uint8_t long_press;
} key_t;

void keyboard_init(void);
uint8_t keyboard_empty(void);
void keyboard_clear(void);
uint8_t keyboard_getchar(key_t * key, uint8_t blocking);
uint8_t keyboard_get(char * buffer, uint8_t input_size, char terminator);


#endif /* __KEYBOARD_DRIVER_H */