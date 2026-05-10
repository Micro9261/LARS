#include "lcd_driver.h"
#include "stm32f446xx.h"
#include <stdint.h>
#include <string.h>

#define DELAY 0x80

static const uint8_t
  init_cmds1[] = {            // Init for 7735R, part 1 (red or green tab)
    15,                       // 15 commands in list:
    ST7735_SWRESET,   DELAY,  //  1: Software reset, 0 args, w/delay
      150,                    //     150 ms delay
    ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, 0 args, w/delay
      255,                    //     500 ms delay
    ST7735_FRMCTR1, 3      ,  //  3: Frame rate ctrl - normal mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR2, 3      ,  //  4: Frame rate control - idle mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR3, 6      ,  //  5: Frame rate ctrl - partial mode, 6 args:
      0x01, 0x2C, 0x2D,       //     Dot inversion mode
      0x01, 0x2C, 0x2D,       //     Line inversion mode
    ST7735_INVCTR , 1      ,  //  6: Display inversion ctrl, 1 arg, no delay:
      0x07,                   //     No inversion
    ST7735_PWCTR1 , 3      ,  //  7: Power control, 3 args, no delay:
      0xA2,
      0x02,                   //     -4.6V
      0x84,                   //     AUTO mode
    ST7735_PWCTR2 , 1      ,  //  8: Power control, 1 arg, no delay:
      0xC5,                   //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
    ST7735_PWCTR3 , 2      ,  //  9: Power control, 2 args, no delay:
      0x0A,                   //     Opamp current small
      0x00,                   //     Boost frequency
    ST7735_PWCTR4 , 2      ,  // 10: Power control, 2 args, no delay:
      0x8A,                   //     BCLK/2, Opamp current small & Medium low
      0x2A,  
    ST7735_PWCTR5 , 2      ,  // 11: Power control, 2 args, no delay:
      0x8A, 0xEE,
    ST7735_VMCTR1 , 1      ,  // 12: Power control, 1 arg, no delay:
      0x0E,
    ST7735_INVOFF , 0      ,  // 13: Don't invert display, no args, no delay
    ST7735_MADCTL , 1      ,  // 14: Memory access control (directions), 1 arg:
      ST7735_ROTATION,        //     row addr/col addr, bottom to top refresh
    ST7735_COLMOD , 1      ,  // 15: set color mode, 1 arg, no delay:
      0x05 },                 //     16-bit color

    init_cmds2[] = {            // Init for 7735S, part 2 (160x80 display)
    3,                        //  3 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x4F,             //     XEND = 79
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x9F ,            //     XEND = 159
    ST7735_INVON, 0 },        //  3: Invert colors

    init_cmds3[] = {            // Init for 7735R, part 3 (red or green tab)
    4,                        //  4 commands in list:
    ST7735_GMCTRP1, 16      , //  1: Gamma Adjustments (pos. polarity), 16 args, no delay:
      0x02, 0x1c, 0x07, 0x12,
      0x37, 0x32, 0x29, 0x2d,
      0x29, 0x25, 0x2B, 0x39,
      0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16      , //  2: Gamma Adjustments (neg. polarity), 16 args, no delay:
      0x03, 0x1d, 0x07, 0x06,
      0x2E, 0x2C, 0x29, 0x2D,
      0x2E, 0x2E, 0x37, 0x3F,
      0x00, 0x00, 0x02, 0x10,
    ST7735_NORON  ,    DELAY, //  3: Normal display on, no args, w/delay
      10,                     //     10 ms delay
    ST7735_DISPON ,    DELAY, //  4: Main screen turn on, no args w/delay
      100 };                  //     100 ms delay

static void spi_slave_select(void)
{
    GPIOC->BSRR = GPIO_BSRR_BR9; // Set PC9 low to select slave
}

static void spi_slave_deselect(void)
{
    GPIOC->BSRR = GPIO_BSRR_BS9; // Set PC9 high to deselect slave
}

static void spi_set_command(void)
{
    GPIOC->BSRR = GPIO_BSRR_BR11; // Set PC9 high to indicate command transfer
}

static void spi_set_data(void)
{
    GPIOC->BSRR = GPIO_BSRR_BS11; // Set PC9 high to indicate data transfer
}

// static void write_register(uint8_t reg, uint8_t val)
// {
//     spi_slave_select();
//     uint8_t address = (reg << 1) & 0x7E; // Address format for MFRC522
//     SPI3->DR = address; // Send address
//     while ( !(SPI3->SR & SPI_SR_TXE) ); // Wait until transmit buffer is empty
//     SPI3->DR = val; // Send value
//     while ( !(SPI3->SR & SPI_SR_TXE) ); // Wait until transmit buffer is empty
//     spi_slave_deselect();
// }

static void write_byte(uint8_t byte)
{
    SPI3->DR = byte; // Send address
    while ( !(SPI3->SR & SPI_SR_TXE) ); // Wait until transmit buffer is empty
}

static void write_cmd(uint8_t val)
{
    spi_set_command();
    // spi_slave_select();
    write_byte(val);
    // spi_slave_deselect();
}

// static void write_data(uint8_t val)
// {
//     spi_set_data();
//     spi_slave_select();
//     write_byte(val);
//     spi_slave_deselect();
// }

static void write_data_package(const uint8_t * buff, uint32_t buff_size)
{
    spi_set_data();
    // spi_slave_select();
    for (uint32_t i = 0 ;i < buff_size; i++)
    {
        write_byte(buff[i]);
    }
    // spi_slave_deselect();
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

static void lcd_execute_command_list(const uint8_t * addr)
{
    uint8_t commands_num, args_num;
    uint16_t ms;

    commands_num = *addr++;
    while(commands_num--)
    {
        uint8_t cmd = *addr++;
        write_cmd(cmd);

        args_num = *addr++;
        ms = args_num & DELAY;
        args_num &= ~DELAY;
        if (args_num)
        {
            write_data_package(addr, args_num);
            addr += args_num;
        }

        if (ms) 
        {
            ms = *addr++;
            if (ms == 255)
            {
                ms = 500;
            }
            delay_ms(ms);
        }
    }
}

static void lcd_set_address_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    write_cmd(ST7735_CASET);
    uint8_t data[] = {0x00, x0 + ST7735_XSTART, 0x00, x1 + ST7735_XSTART};
    write_data_package(data, sizeof(data));

    write_cmd(ST7735_RASET);
    data[1] = y0 + ST7735_YSTART;
    data[3] = y1 + ST7735_YSTART;
    write_data_package(data, sizeof(data));

    write_cmd(ST7735_RAMWR);
}

void ST7735_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    GPIOC->MODER &= ~(GPIO_MODER_MODE12 | GPIO_MODER_MODE10);
    GPIOC->MODER |= (GPIO_MODER_MODE12_1 | GPIO_MODER_MODE10_1);
    GPIOC->AFR[1] &= ~(GPIO_AFRH_AFSEL10 | GPIO_AFRH_AFSEL12);
    GPIOC->AFR[1] |= (0x6 << GPIO_AFRH_AFSEL10_Pos) | (0x6 << GPIO_AFRH_AFSEL12_Pos);

    //Configure PC8 (Reset), PC9 (CS), PC11 (D/C) as output
    GPIOC->MODER &= ~(GPIO_MODER_MODE8 | GPIO_MODER_MODE9 | GPIO_MODER_MODE11);
    GPIOC->MODER |= (GPIO_MODER_MODE8_0 | GPIO_MODER_MODE9_0 | GPIO_MODER_MODE11_0);

    //SPI3 clock enable
    RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;

    //SPI3 configuration: Master mode, 8-bit data, CPOL=0, CPHA=0, MSB first, baud rate = fPCLK/2
    SPI3->CR1 = SPI_CR1_MSTR;
    SPI3->CR2 = SPI_CR2_SSOE;
    SPI3->CR1 |= SPI_CR1_SPE;

    spi_slave_select();
    //Reset lcd module
    GPIOC->BSRR = GPIO_BSRR_BR8;
    delay_ms(5);
    GPIOC->BSRR = GPIO_BSRR_BS8;
    delay_ms(50);

    //init lcd module
    lcd_execute_command_list(init_cmds1);
    lcd_execute_command_list(init_cmds2);
    lcd_execute_command_list(init_cmds3);
    spi_slave_deselect();
}

void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if ( (x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT) )
    {
        return;
    }

    spi_slave_select();
    lcd_set_address_window(x, y, x + 1, y + 1);
    uint8_t data[] = {color >> 8, color & 0xFF };
    write_data_package(data, sizeof(data));
    spi_slave_deselect();
}

static void ST7735_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor) {
    uint32_t i, b, j;

    lcd_set_address_window(x, y, x+font.width-1, y+font.height-1);

    for(i = 0; i < font.height; i++) {
        b = font.data[(ch - 32) * font.height + i];
        for(j = 0; j < font.width; j++) {
            if((b << j) & 0x8000)  {
                uint8_t data[] = { color >> 8, color & 0xFF };
                write_data_package(data, sizeof(data));
            } else {
                uint8_t data[] = { bgcolor >> 8, bgcolor & 0xFF };
                write_data_package(data, sizeof(data));
            }
        }
    }
}

void ST7735_WriteString(uint16_t x, uint16_t y, const char* str, FontDef font, uint16_t color, uint16_t bgcolor)
{
    spi_slave_select();
    while(*str) {
        if(x + font.width >= ST7735_WIDTH) {
            x = 0;
            y += font.height;
            if(y + font.height >= ST7735_HEIGHT) {
                break;
            }

            if(*str == ' ') {
                // skip spaces in the beginning of the new line
                str++;
                continue;
            }
        }

        ST7735_WriteChar(x, y, *str, font, color, bgcolor);
        x += font.width;
        str++;
    }
    spi_slave_deselect();
}

void ST7735_FillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    // clipping
    if((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT)) return;
    if((x + w - 1) >= ST7735_WIDTH) w = ST7735_WIDTH - x;
    if((y + h - 1) >= ST7735_HEIGHT) h = ST7735_HEIGHT - y;

    spi_slave_select();
    lcd_set_address_window(x, y, x+w-1, y+h-1);

    uint8_t data[] = { color >> 8, color & 0xFF };
    spi_set_data();
    for(y = h; y > 0; y--) {
        for(x = w; x > 0; x--) {
            // HAL_SPI_Transmit(&ST7735_SPI_PORT, data, sizeof(data), HAL_MAX_DELAY);
            for (uint16_t i = 0; i < sizeof(data); i++)
            {
                write_byte(data[i]);
            }
        }
    }

    spi_slave_deselect();
}

void ST7735_FillRectangleFast(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    // clipping
    if((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT)) return;
    if((x + w - 1) >= ST7735_WIDTH) w = ST7735_WIDTH - x;
    if((y + h - 1) >= ST7735_HEIGHT) h = ST7735_HEIGHT - y;

    spi_slave_select();
    lcd_set_address_window(x, y, x+w-1, y+h-1);

    // Prepare whole line in a single buffer
    uint8_t pixel[] = { color >> 8, color & 0xFF };
    uint8_t line[ST7735_WIDTH];
    for(x = 0; x < w; ++x)
    	memcpy(line + x * sizeof(pixel), pixel, sizeof(pixel));

    spi_set_data();
    for(y = h; y > 0; y--)
        // HAL_SPI_Transmit(&ST7735_SPI_PORT, line, w * sizeof(pixel), HAL_MAX_DELAY);
    {
        for (uint32_t i = 0; i < w * sizeof(pixel); i++)
        {
            write_byte(line[i]);
        }
    }

    spi_slave_deselect();
}

void ST7735_FillScreen(uint16_t color)
{
    ST7735_FillRectangle(0, 0, ST7735_WIDTH, ST7735_HEIGHT, color);
}

void ST7735_FillScreenFast(uint16_t color)
{
    ST7735_FillRectangleFast(0, 0, ST7735_WIDTH, ST7735_HEIGHT, color);
}

void ST7735_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data)
{
    if((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT)) return;
    if((x + w - 1) >= ST7735_WIDTH) return;
    if((y + h - 1) >= ST7735_HEIGHT) return;

    spi_slave_select();
    lcd_set_address_window(x, y, x+w-1, y+h-1);
    write_data_package((uint8_t*)data, sizeof(uint16_t)*w*h);
    spi_slave_deselect();
}

void ST7735_InvertColors(uint8_t invert)
{
    spi_slave_select();
    write_cmd(invert ? ST7735_INVON : ST7735_INVOFF);
    spi_slave_deselect();
}

void ST7735_SetGamma(GammaDef gamma)
{
    spi_slave_select();
	write_cmd(ST7735_GAMSET);
	write_data_package((uint8_t *) &gamma, sizeof(gamma));
	spi_slave_deselect();
}