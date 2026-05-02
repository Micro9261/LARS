#ifndef __LOCK_DRIVER_H
#define __LOCK_DRIVER_H

#include "stm32f4xx.h"
#include <inttypes.h>

typedef enum uint8_t{
    LOCK_STATUS_CLOSED = 0,
    LOCK_STATUS_OPEN = 1,
    LOCK_STATUS_BLOCKED = 2
} lock_status_t;

void lock_init(void);
void lock_open(void);
void lock_close(void);
lock_status_t get_lock_status(void);

void lock_block(void);
void lock_unblock(void);

#endif /* __LOCK_DRIVER_H */