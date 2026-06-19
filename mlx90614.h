#ifndef MLX90614_H
#define MLX90614_H

#include "stm32g0xx_hal.h"

/* * MLX90614 varsayılan 7-bit I2C Adresi: 0x5A
 * STM32 HAL kütüphanesi 8-bit adresleme beklediği için 1 bit sola kaydırıyoruz (0x5A << 1) = 0xB4
 */
#define MLX90614_I2C_ADDR 0xB4

// Okunacak Register Adresleri (RAM)
#define MLX90614_TA       0x06 // Ortam Sıcaklığı (Ambient Temperature)
#define MLX90614_TOBJ1    0x07 // Cisim Sıcaklığı 1 (Object Temperature)

// Fonksiyon Prototipleri
float MLX90614_ReadAmbientTemp(I2C_HandleTypeDef *hi2c);
float MLX90614_ReadObjectTemp(I2C_HandleTypeDef *hi2c);

#endif /* MLX90614_H */