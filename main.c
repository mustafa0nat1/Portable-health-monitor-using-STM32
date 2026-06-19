/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Tasinabilir Saglik Monitoru - Final
  ******************************************************************************
  */
/* USER CODE END Header */
#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "st7789.h"
#include "fonts.h"
#include "max30100.h"
/* USER CODE END Includes */

I2C_HandleTypeDef hi2c1;
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);

/* USER CODE BEGIN 0 */
MAX30100_HandleTypeDef hmax30100;

#define MLX90614_ADDR  0xB4
#define MLX90614_TOBJ1 0x07

#define GRAPH_W      240
#define GRAPH_H      138
#define GRAPH_MID    69
#define PANEL_Y      141
#define WAVE_LEN     240

#define BUZZER_PORT  GPIOB
#define BUZZER_PIN   GPIO_PIN_1
#define BPM_HIGH     80

int16_t  wave_buf[WAVE_LEN];
uint16_t wave_idx = 0;

int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

float Read_Temp(void) {
    uint8_t buf[3];
    if (HAL_I2C_Mem_Read(&hi2c1, MLX90614_ADDR, MLX90614_TOBJ1,
                          1, buf, 3, 100) == HAL_OK) {
        uint16_t raw = (buf[1] << 8) | buf[0];
        return (raw * 0.02f) - 273.15f;
    }
    return -99.0f;
}

void Draw_Column(uint16_t col, int16_t y_new, int16_t y_prev) {
    if (y_new  < 2)           y_new  = 2;
    if (y_new  > GRAPH_H - 2) y_new  = GRAPH_H - 2;
    if (y_prev < 2)           y_prev = 2;
    if (y_prev > GRAPH_H - 2) y_prev = GRAPH_H - 2;

    int16_t ymin = y_prev < y_new ? y_prev : y_new;
    int16_t ymax = y_prev > y_new ? y_prev : y_new;

    for (int16_t y = 0; y < GRAPH_H; y++) {
        uint16_t color;
        if (y >= ymin && y <= ymax) {
            color = GREEN;
        } else if (y == GRAPH_MID) {
            color = 0x0300;
        } else if (y % 35 == 0) {
            color = 0x0180;
        } else if (col % 60 == 0) {
            color = 0x0180;
        } else {
            color = BLACK;
        }
        ST7789_DrawPixel(col, y, color);
    }
}

void Update_Number(uint16_t x, uint16_t y, const char *str, uint16_t color) {
    for (uint16_t row = y; row < y + 26; row++)
        for (uint16_t col = x; col < x + 75; col++)
            ST7789_DrawPixel(col, row, BLACK);
    ST7789_WriteString(x, y, str, Font_16x26, color, BLACK);
}

void Draw_Static_UI(void) {
    ST7789_Fill_Color(BLACK);

    for (uint16_t x = 0; x < 240; x++) {
        ST7789_DrawPixel(x, 139, 0x8410);
        ST7789_DrawPixel(x, 140, 0x4228);
    }

    for (uint16_t y = PANEL_Y; y < 240; y++) {
        ST7789_DrawPixel(80,  y, 0x4228);
        ST7789_DrawPixel(160, y, 0x4228);
    }

    ST7789_WriteString(8,   PANEL_Y + 3, "NABIZ", Font_7x10, 0x8410, BLACK);
    ST7789_WriteString(88,  PANEL_Y + 3, "SpO2",  Font_7x10, 0x8410, BLACK);
    ST7789_WriteString(168, PANEL_Y + 3, "ATES",  Font_7x10, 0x8410, BLACK);

    ST7789_WriteString(24,  228, "bpm", Font_7x10, 0x4228, BLACK);
    ST7789_WriteString(108, 228, "%",   Font_7x10, 0x4228, BLACK);
    ST7789_WriteString(183, 228, "C",   Font_7x10, 0x4228, BLACK);

    Update_Number(2,   PANEL_Y + 16, "---", 0x4228);
    Update_Number(82,  PANEL_Y + 16, "--",  0x4228);
    Update_Number(162, PANEL_Y + 16, "--",  0x4228);

    for (uint16_t col = 0; col < GRAPH_W; col++)
        Draw_Column(col, GRAPH_MID, GRAPH_MID);
}

void Buzzer_Alarm(void) {
    for (uint8_t i = 0; i < 3; i++) {
        HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
        HAL_Delay(100);
        HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
        HAL_Delay(100);
    }
}
/* USER CODE END 0 */

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART2_UART_Init();
    MX_SPI1_Init();

    /* USER CODE BEGIN 2 */
    printf("Saglik Monitoru Baslatiliyor...\r\n");

    HAL_GPIO_WritePin(BLK_PORT, BLK_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
    ST7789_Init();
    Draw_Static_UI();

    if (MAX30100_Init(&hmax30100, &hi2c1) == HAL_OK)
        printf("MAX30100 OK\r\n");
    else
        printf("MAX30100 HATA!\r\n");

    for (uint16_t i = 0; i < WAVE_LEN; i++)
        wave_buf[i] = GRAPH_MID;

    float ir_dc = 0.0f, red_dc = 0.0f;
    float ir_ac = 0.0f, red_ac = 0.0f;
    float ir_ac_max = 0.0f, red_ac_max = 0.0f;
    float bpm = 0.0f, spo2 = 0.0f;
    uint8_t beat_detected = 0;
    uint32_t last_beat_time = HAL_GetTick();
    const float alpha = 0.90f;

    float bpm_sum = 0.0f, spo2_sum = 0.0f;
    uint16_t sample_count = 0;
    uint32_t window_start   = HAL_GetTick();
    uint32_t last_temp_time = HAL_GetTick();
    int16_t  last_pixel_y   = GRAPH_MID;

    float temp = Read_Temp();

    printf("Olcum basliyor...\r\n");
    /* USER CODE END 2 */

    while (1)
    {
        /* USER CODE BEGIN 3 */

        if (MAX30100_Read_Raw(&hmax30100) != HAL_OK) {
            HAL_Delay(10);
            continue;
        }

        /* DC filtre */
        ir_dc  = (alpha * ir_dc)  + ((1.0f - alpha) * hmax30100.IR_Raw);
        ir_ac  = hmax30100.IR_Raw  - ir_dc;
        red_dc = (alpha * red_dc) + ((1.0f - alpha) * hmax30100.RED_Raw);
        red_ac = hmax30100.RED_Raw - red_dc;

        if (ir_ac > 5000.0f || ir_ac < -5000.0f) {
            ir_dc = hmax30100.IR_Raw;
            ir_ac = 0.0f;
        }

        /* AC -> piksel */
        float ac_clamped = ir_ac;
        if (ac_clamped >  800.0f) ac_clamped =  800.0f;
        if (ac_clamped < -800.0f) ac_clamped = -800.0f;
        int16_t pixel_y = (int16_t)(GRAPH_MID - (ac_clamped * (GRAPH_MID - 5) / 800.0f));

        if (pixel_y < 2)           pixel_y = 2;
        if (pixel_y > GRAPH_H - 2) pixel_y = GRAPH_H - 2;

        /* Sutunu ciz */
        uint16_t col = wave_idx % GRAPH_W;
        Draw_Column(col, pixel_y, last_pixel_y);
        last_pixel_y = pixel_y;

        wave_buf[wave_idx] = pixel_y;
        wave_idx = (wave_idx + 1) % WAVE_LEN;

        /* AC tepe */
        if (ir_ac  > ir_ac_max)  ir_ac_max  = ir_ac;
        if (red_ac > red_ac_max) red_ac_max = red_ac;

        /* Nabiz algila */
        if (ir_ac > 0.5f && beat_detected == 0) {
            beat_detected = 1;
            uint32_t now = HAL_GetTick();
            uint32_t dt  = now - last_beat_time;
            last_beat_time = now;

            if (dt > 300 && dt < 1500) {
                float new_bpm = 60000.0f / dt;
                if (bpm < 1.0f) bpm = new_bpm;
                else            bpm = (bpm * 0.7f) + (new_bpm * 0.3f);

                if (ir_dc > 0 && red_dc > 0 && ir_ac_max > 0) {
                    float r = (red_ac_max / red_dc) / (ir_ac_max / ir_dc);
                    float s = 104.0f - (17.0f * r);
                    if (s > 100.0f) s = 100.0f;
                    if (s <  70.0f) s =  70.0f;
                    if (spo2 < 1.0f) spo2 = s;
                    else             spo2 = (spo2 * 0.7f) + (s * 0.3f);
                }

                if (bpm > 40 && bpm < 160 && spo2 > 70) {
                    bpm_sum  += bpm;
                    spo2_sum += spo2;
                    sample_count++;
                }
            }
            ir_ac_max  = 0.0f;
            red_ac_max = 0.0f;
        }
        if (ir_ac < -1.0f) beat_detected = 0;

        /* 1 saniyede sayilari guncelle */
        if (HAL_GetTick() - window_start >= 1000) {

            if (sample_count > 0) {
                bpm  = bpm_sum  / sample_count;
                spo2 = spo2_sum / sample_count;
            }

            printf("BPM:%d SpO2:%d Temp:%d Samples:%d\r\n",
                   (int)bpm, (int)spo2, (int)temp, sample_count);

            char buf[12];

            /* Nabiz yuksekse kirmizi goster */
            uint16_t bpm_color = (bpm > BPM_HIGH) ? RED : GREEN;

            if (bpm > 1.0f) {
                snprintf(buf, sizeof(buf), "%3d", (int)bpm);
                Update_Number(2, PANEL_Y + 16, buf, bpm_color);
                snprintf(buf, sizeof(buf), "%3d", (int)spo2);
                Update_Number(82, PANEL_Y + 16, buf, YELLOW);
            } else {
                Update_Number(2,  PANEL_Y + 16, "---", 0x4228);
                Update_Number(82, PANEL_Y + 16, "---", 0x4228);
            }

            if (temp > -50.0f) {
                snprintf(buf, sizeof(buf), "%2d", (int)temp);
                Update_Number(162, PANEL_Y + 16, buf, RED);
            }

            /* BUZZER ALARM - nabiz 100 BPM uzerinde */
            if (bpm > BPM_HIGH && bpm < 200) {
                printf("ALARM! Nabiz yuksek: %d BPM\r\n", (int)bpm);
                Buzzer_Alarm();
            } else {
                HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
            }

            bpm_sum      = 0.0f;
            spo2_sum     = 0.0f;
            sample_count = 0;
            window_start = HAL_GetTick();
        }

        /* Sicaklik 3sn */
        if (HAL_GetTick() - last_temp_time > 3000) {
            float t = Read_Temp();
            if (t > -50.0f) temp = t;
            last_temp_time = HAL_GetTick();
        }

        HAL_Delay(20);
        /* USER CODE END 3 */
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSIDiv              = RCC_HSI_DIV1;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM            = RCC_PLLM_DIV1;
    RCC_OscInitStruct.PLL.PLLN            = 8;
    RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ            = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR            = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) Error_Handler();
}

static void MX_I2C1_Init(void)
{
    hi2c1.Instance              = I2C1;
    hi2c1.Init.Timing           = 0x10B17DB5;
    hi2c1.Init.OwnAddress1      = 0;
    hi2c1.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2      = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) Error_Handler();
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) Error_Handler();
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) Error_Handler();
}

static void MX_SPI1_Init(void)
{
    hspi1.Instance               = SPI1;
    hspi1.Init.Mode              = SPI_MODE_MASTER;
    hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity       = SPI_POLARITY_HIGH;
    hspi1.Init.CLKPhase          = SPI_PHASE_2EDGE;
    hspi1.Init.NSS               = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial     = 7;
    hspi1.Init.CRCLength         = SPI_CRC_LENGTH_DATASIZE;
    hspi1.Init.NSSPMode          = SPI_NSS_PULSE_DISABLE;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) Error_Handler();
}

static void MX_USART2_UART_Init(void)
{
    huart2.Instance            = USART2;
    huart2.Init.BaudRate       = 115200;
    huart2.Init.WordLength     = UART_WORDLENGTH_8B;
    huart2.Init.StopBits       = UART_STOPBITS_1;
    huart2.Init.Parity         = UART_PARITY_NONE;
    huart2.Init.Mode           = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl      = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling   = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart2) != HAL_OK) Error_Handler();
    if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) Error_Handler();
    if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) Error_Handler();
    if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK) Error_Handler();
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOA, LED_GREEN_Pin | TFT_RES_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TFT_BLK_GPIO_Port, TFT_BLK_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TFT_DC_GPIO_Port,  TFT_DC_Pin,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);

    /* LED_GREEN */
    GPIO_InitStruct.Pin   = LED_GREEN_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LED_GREEN_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    /* TFT_RES (PA9) */
    GPIO_InitStruct.Pin = TFT_RES_Pin;
    HAL_GPIO_Init(TFT_RES_GPIO_Port, &GPIO_InitStruct);

    /* TFT_DC (PC7) */
    GPIO_InitStruct.Pin = TFT_DC_Pin;
    HAL_GPIO_Init(TFT_DC_GPIO_Port, &GPIO_InitStruct);

    /* TFT_BLK (PB0) */
    GPIO_InitStruct.Pin = TFT_BLK_Pin;
    HAL_GPIO_Init(TFT_BLK_GPIO_Port, &GPIO_InitStruct);

    /* BUZZER (PB1) */
    GPIO_InitStruct.Pin = BUZZER_PIN;
    HAL_GPIO_Init(BUZZER_PORT, &GPIO_InitStruct);
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
