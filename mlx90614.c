#include "mlx90614.h"

/* * Sensörden veri okumak için ortak fonksiyon.
 * MLX90614 SMBus protokolü kullanır, bu da I2C Memory Read işlemine eşdeğerdir.
 * Cihaz 3 bayt veri döndürür: LSB, MSB ve PEC (Hata Kontrol Kodu).
 */
static float MLX90614_ReadTemp(I2C_HandleTypeDef *hi2c, uint16_t reg) {
    uint8_t buffer[3]; // LSB, MSB, PEC
    uint16_t rawData;
    float temperature;

    // Sensörden 3 bayt okuyoruz.
    if (HAL_I2C_Mem_Read(hi2c, MLX90614_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buffer, 3, 100) == HAL_OK) {
        
        // LSB ve MSB'yi birleştirip 16-bit ham veriyi elde ediyoruz (PEC baytını hesaplamaya katmıyoruz)
        rawData = (buffer[1] << 8) | buffer[0];

        // Sensör formülü: Sıcaklık (Kelvin) = rawData * 0.02
        // Santigrat (Celsius) = Kelvin - 273.15
        temperature = (rawData * 0.02f) - 273.15f;
        
        return temperature;
    }

    // İletişim hatası durumunda dönülecek değer
    return -999.0f; 
}

float MLX90614_ReadAmbientTemp(I2C_HandleTypeDef *hi2c) {
    return MLX90614_ReadTemp(hi2c, MLX90614_TA);
}

float MLX90614_ReadObjectTemp(I2C_HandleTypeDef *hi2c) {
    return MLX90614_ReadTemp(hi2c, MLX90614_TOBJ1);
}