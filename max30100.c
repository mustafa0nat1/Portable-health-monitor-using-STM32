#include "max30100.h"

// Sensörün içine veri yazma fonksiyonu
static HAL_StatusTypeDef WriteRegister(MAX30100_HandleTypeDef *sensor, uint8_t reg, uint8_t value) {
    return HAL_I2C_Mem_Write(sensor->hi2c, MAX30100_I2C_ADDR, reg, 1, &value, 1, 100);
}

// Sensörü başlatma ve yapılandırma
HAL_StatusTypeDef MAX30100_Init(MAX30100_HandleTypeDef *sensor, I2C_HandleTypeDef *hi2c) {
    sensor->hi2c = hi2c;
    
    if (HAL_I2C_IsDeviceReady(sensor->hi2c, MAX30100_I2C_ADDR, 3, 100) != HAL_OK) {
        return HAL_ERROR;
    }

    WriteRegister(sensor, MAX30100_REG_MODE_CFG, 0x03);
    WriteRegister(sensor, MAX30100_REG_SPO2_CFG, 0x43);
    WriteRegister(sensor, MAX30100_REG_LED_CFG, 0x77);
    
    WriteRegister(sensor, MAX30100_REG_FIFO_WR, 0x00);
    WriteRegister(sensor, MAX30100_REG_FIFO_OVF, 0x00);
    WriteRegister(sensor, MAX30100_REG_FIFO_RD, 0x00);

    return HAL_OK;
}

// Ham veriyi çekme
HAL_StatusTypeDef MAX30100_Read_Raw(MAX30100_HandleTypeDef *sensor) {
    uint8_t buffer[4];
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(sensor->hi2c, MAX30100_I2C_ADDR, MAX30100_REG_FIFO_DATA, 1, buffer, 4, 100);
    
    if (status == HAL_OK) {
        sensor->IR_Raw = (buffer[0] << 8) | buffer[1];
        sensor->RED_Raw = (buffer[2] << 8) | buffer[3];
    }
    return status;
}