#ifndef __ST7789_H
#define __ST7789_H

#include "fonts.h"
#include "main.h"

/* Kullanılan SPI Portu */
#define ST7789_SPI_PORT hspi1
extern SPI_HandleTypeDef ST7789_SPI_PORT;

/* DMA ayari */
//#define USE_DMA

/* CS pini olmayan ekranlar icin */
#define CFG_NO_CS

/* Pin Baglantilari (STM32G071RB) */
#define ST7789_RST_PORT GPIOA
#define ST7789_RST_PIN  GPIO_PIN_9

#define ST7789_DC_PORT  GPIOC
#define ST7789_DC_PIN   GPIO_PIN_7

#define BLK_PORT GPIOB
#define BLK_PIN  GPIO_PIN_0

/* Ekran Tipi */
#define USING_240X240
#define ST7789_ROTATION 2

#ifdef USING_240X240
    #define ST7789_WIDTH  240
    #define ST7789_HEIGHT 240
    #define X_SHIFT 0
    #define Y_SHIFT 0
#endif

/* Renk Tanimlari */
#define WHITE       0xFFFF
#define BLACK       0x0000
#define BLUE        0x001F
#define RED         0xF800
#define MAGENTA     0xF81F
#define GREEN       0x07E0
#define CYAN        0x7FFF
#define YELLOW      0xFFE0
#define GBLUE       0x07FF

/* Komut Registers */
#define ST7789_NOP     0x00
#define ST7789_SWRESET 0x01
#define ST7789_SLPIN   0x10
#define ST7789_SLPOUT  0x11
#define ST7789_INVOFF  0x20
#define ST7789_INVON   0x21
#define ST7789_DISPOFF 0x28
#define ST7789_DISPON  0x29
#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_RAMWR   0x2C
#define ST7789_MADCTL  0x36
#define ST7789_COLMOD  0x3A
#define ST7789_NORON   0x13

/* MADCTL Bit Tanimlari */
#define ST7789_MADCTL_MY  0x80
#define ST7789_MADCTL_MX  0x40
#define ST7789_MADCTL_MV  0x20
#define ST7789_MADCTL_ML  0x10
#define ST7789_MADCTL_RGB 0x00

/* Renk Modu */
#define ST7789_COLOR_MODE_16bit 0x55

/* Donanim Kontrol Makrolari */
#define ST7789_RST_Clr() HAL_GPIO_WritePin(ST7789_RST_PORT, ST7789_RST_PIN, GPIO_PIN_RESET)
#define ST7789_RST_Set() HAL_GPIO_WritePin(ST7789_RST_PORT, ST7789_RST_PIN, GPIO_PIN_SET)
#define ST7789_DC_Clr()  HAL_GPIO_WritePin(ST7789_DC_PORT,  ST7789_DC_PIN,  GPIO_PIN_RESET)
#define ST7789_DC_Set()  HAL_GPIO_WritePin(ST7789_DC_PORT,  ST7789_DC_PIN,  GPIO_PIN_SET)

#ifdef CFG_NO_CS
    #define ST7789_Select()   __NOP()
    #define ST7789_UnSelect() __NOP()
#else
    #define ST7789_Select()   HAL_GPIO_WritePin(ST7789_CS_PORT, ST7789_CS_PIN, GPIO_PIN_RESET)
    #define ST7789_UnSelect() HAL_GPIO_WritePin(ST7789_CS_PORT, ST7789_CS_PIN, GPIO_PIN_SET)
#endif

/* Fonksiyonlar */
void ST7789_Init(void);
void ST7789_SetRotation(uint8_t m);
void ST7789_Fill_Color(uint16_t color);
void ST7789_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void ST7789_WriteString(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bgcolor);
void ST7789_Test(void);

#endif
