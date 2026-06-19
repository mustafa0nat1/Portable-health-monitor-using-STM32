#ifndef MAX30100_H_
#define MAX30100_H_

#include "stm32g0xx_hal.h"

#define MAX30100_I2C_ADDR       0xAE
#define MAX30100_REG_MODE_CFG   0x06
#define MAX30100_REG_SPO2_CFG   0x07
#define MAX30100_REG_LED_CFG    0x09
#define MAX30100_REG_FIFO_WR    0x02
#define MAX30100_REG_FIFO_OVF   0x03
#define MAX30100_REG_FIFO_RD    0x04
#define MAX30100_REG_FIFO_DATA  0x05

typedef struct {
    uint16_t IR_Raw;
    uint16_t RED_Raw;
    I2C_HandleTypeDef *hi2c;
} MAX30100_HandleTypeDef;

HAL_StatusTypeDef MAX30100_Init(MAX30100_HandleTypeDef *sensor, I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef MAX30100_Read_Raw(MAX30100_HandleTypeDef *sensor);

#endif