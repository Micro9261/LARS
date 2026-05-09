#include "password_manager.h"
#include <stdint.h>

typedef struct
{
    uint8_t buffer[MAX_PASSWORD_LEN];
    uint8_t len;
} password_t;

static password_t password[MAX_PASSWORD_TYPES] = {0};

static uint8_t check_params_password(uint8_t type, uint8_t pass_len)
{
    uint8_t status = PASSWORD_OK;

    if (type != PASSWORD_ADMIN && type != PASSWORD_USER)
    {
        status =  PASSWORD_ERR;
    }

    if (pass_len > MAX_PASSWORD_LEN)
    {
        status = PASSWORD_ERR;
    }

    return status;
}

void pass_manager_init(void)
{
    password[PASSWORD_USER].buffer[0] = '1';
    password[PASSWORD_USER].buffer[1] = '2';
    password[PASSWORD_USER].buffer[2] = '3';
    password[PASSWORD_USER].buffer[3] = '4';
    password[PASSWORD_USER].len = 4;

    password[PASSWORD_ADMIN].buffer[0] = '1';
    password[PASSWORD_ADMIN].buffer[1] = '1';
    password[PASSWORD_ADMIN].buffer[2] = '1';
    password[PASSWORD_ADMIN].buffer[3] = '1';
    password[PASSWORD_ADMIN].len = 4;
}

uint8_t pass_manager_set_password(uint8_t * pass, uint8_t pass_len, uint8_t type)
{
    uint8_t status = check_params_password(type, pass_len);
    if (status != PASSWORD_OK)
    {
        return status;
    }

    for (uint8_t i = 0; i < pass_len; i++)
    {
        password[type].buffer[i] = pass[i];
    }
    password[type].len = pass_len;

    return status;
}

uint8_t pass_manager_check_password(uint8_t * pass, uint8_t pass_len, uint8_t type)
{
    uint8_t status = check_params_password(type, pass_len);
    if (status != PASSWORD_OK)
    {
        return status;
    }

    if (pass_len != password[type].len)
    {
        status = PASSWORD_WRONG;
    }
    else
    {
        for (uint8_t i = 0; i < pass_len; i++)
        {
            if (password[type].buffer[i] != pass[i])
            {
                status = PASSWORD_WRONG;
                break;
            }
        }
    }

    return status;
}

uint8_t pass_manager_get_password(uint8_t * pass, uint8_t * pass_len, uint8_t type)
{
    uint8_t status = PASSWORD_OK;
    if (type != PASSWORD_ADMIN || type != PASSWORD_USER)
    {
        return PASSWORD_ERR;
    }

    *pass_len = password[type].len;
    for (uint8_t i = 0; i < password[type].len; i++)
    {
        pass[i] = password[type].buffer[i];
    }

    return status;
}

/******************* CARDS  **************************/

static uint8_t check_params_card(uint8_t uid_len, uint8_t type)
{
    uint8_t status = CARD_OK;
    
    if (uid_len > UID_LEN)
    {
        status = CARD_ERR;
    }

    if (type != UID_TYPE_ADMIN && type != UID_TYPE_USER )
    {
        status = CARD_ERR;
    }

    return status;
}

typedef struct
{
    uint8_t uid[UID_LEN];
    uint8_t type;
} card_t;

static card_t cards[UID_STORE_MAX] = {0};

uint8_t pass_manager_add_card(uint8_t * uid, uint8_t uid_len, uint8_t type)
{
    uint8_t status = check_params_card(uid_len, type);

    int8_t card_found = -1;
    int8_t free_card_place = -1;
    for (uint8_t i = 0; i < UID_STORE_MAX; i++)
    {
        if (card_found != -1)
        {
            break;
        }
        
        if (cards[i].type == UID_TYPE_EMPTY)
        {
            free_card_place = i;
            continue;
        }

        for (uint8_t j = 0; j < uid_len; i++)
        {
            if (cards[i].uid[j] != uid[j])
            {
                break;
            }

            if (j == uid_len - 1)
            {
                card_found = i;
            }
        }
    }

    if (card_found != -1 || free_card_place == -1)
    {
        status =  CARD_ERR;
    }
    else 
    {
        for (uint8_t i = 0; i < uid_len; i++)
        {
            cards[free_card_place].uid[i] = uid[i];
        }
        cards[free_card_place].type = type;
    }
    
    return status;
}

uint8_t pass_manager_check_card(uint8_t * uid, uint8_t uid_len, uint8_t type)
{
    uint8_t status = check_params_card(uid_len, type);

    int8_t card_found = -1;
    for (uint8_t i = 0; i < UID_STORE_MAX; i++)
    {
        if (card_found != -1)
        {
            break;
        }

        if (cards[i].type == UID_TYPE_EMPTY)
        {
            continue;
        }

        for (uint8_t j = 0; j < uid_len; j++)
        {
            if (cards[i].uid[j] != uid[j])
            {
                break;
            }
            
            if (j == uid_len - 1 && cards[i].type == type)
            {
                card_found = i;
            }
        }
    }

    if (card_found == -1)
    {
        status = CARD_BAD;
    }

    return  status;
}

uint8_t pass_manager_remove_card(uint8_t * uid, uint8_t uid_len)
{
    uint8_t status = check_params_card(uid_len, UID_TYPE_USER);

    int8_t card_found = -1;
    for (uint8_t i = 0; i < UID_STORE_MAX; i++)
    {
        if (card_found != -1)
        {
            break;
        }

        if (cards[i].type == UID_TYPE_EMPTY)
        {
            continue;
        }

        for (uint8_t j = 0; j < uid_len; j++)
        {
            if (cards[i].uid[j] != uid[j])
            {
                break;
            }
            
            if (j == uid_len - 1)
            {
                card_found = i;
            }
        }
    }

    if (card_found == -1)
    {
        status = CARD_BAD;
    }
    else 
    {
        cards[card_found].type = UID_TYPE_EMPTY;
    }

    return status;
}