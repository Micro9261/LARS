#ifndef __PASSWORD_MANAGER_H__
#define __PASSWORD_MANAGER_H__
#include <stdint.h>

//Config
#define MAX_PASSWORD_LEN 4
#define MAX_PASSWORD_TYPES 2

#define PASSWORD_OK 0
#define PASSWORD_WRONG 1
#define PASSWORD_ERR 2

//Password types
#define PASSWORD_ADMIN 0
#define PASSWORD_USER 1

//Card UIDs
#define UID_LEN 5
#define UID_STORE_MAX 4

//Card types
#define UID_TYPE_ADMIN 2
#define UID_TYPE_USER 1
#define UID_TYPE_EMPTY 0

//Card Config
#define CARD_OK 0
#define CARD_BAD 1
#define CARD_ERR 2

void pass_manager_init(void);
uint8_t pass_manager_set_password(uint8_t * pass, uint8_t pass_len, uint8_t type);
uint8_t pass_manager_check_password(uint8_t * pass, uint8_t pass_len, uint8_t type);
uint8_t pass_manager_get_password(uint8_t * pass, uint8_t * pass_len, uint8_t type);

uint8_t pass_manager_add_card(uint8_t * uid, uint8_t uid_len, uint8_t type);
uint8_t pass_manager_check_card(uint8_t * uid, uint8_t uid_len, uint8_t type);
uint8_t pass_manager_remove_card(uint8_t * uid, uint8_t uid_len);

#endif // __PASSWORD_MANAGER_H__