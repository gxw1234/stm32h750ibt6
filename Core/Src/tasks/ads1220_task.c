#include "tasks/ads1220_task.h"
#include "tasks/lcd_task.h"  /* 包含 LCD 函数声明 */
#include <stdio.h>

#define SPI_CS1_LOW()       HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_RESET)
#define SPI_CS1_HIGH()      HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_SET)

/* CS2引脚控制宏定义 */
#define SPI_CS2_LOW()       HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_RESET)
#define SPI_CS2_HIGH()      HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_SET)

/* SPI句柄定义 */
SPI_HandleTypeDef hspi4;


uint32_t sample_count = 0;
float voltage_sum = 0;
float raw_voltage_sum = 0; // 用于存储原始电压累加值

uint32_t sample_count_2 = 0;
float voltage_sum_2 = 0;


uint32_t adc_value = 0;

/* ADS1220参考电压 */
#define VREF              2.048f    // 参考电压2.048V
#define ADC_FSR           8388608.0f // 2^23, ADC满量程范围


/* 将24位ADC数据转换为电压值 */
static float Convert_ADC_To_Voltage(uint32_t adc_value)
{
    int32_t signed_value;
    float voltage;
    uint32_t temp = 0;
    
    // 如果是负数（最高位为1）
    if(adc_value & 0x800000) {
        signed_value = (int32_t)(adc_value| 0xFF000000);
    } else {
        signed_value = (int32_t)adc_value;
    }
    
    // 将补码转换为电压值
    voltage = ((float)signed_value * VREF) / ADC_FSR;

    return voltage;

}

void ADS1220_Task(void *argument)
{

    vTaskDelay(3000 / portTICK_PERIOD_MS);
    ADS1220_Init();

    SPI_CS1_LOW();
    /* 发送复位命令 */
    SPI_Transmit(0x06);  // RESET命令

    vTaskDelay(pdMS_TO_TICKS(1)); 


    SPI_Transmit(0x43);  // WREG命令，写寄存器3
    SPI_Transmit(0x00);  // 配置数据 - 寄存器0：PGA=1, AIN0/AIN1
    SPI_Transmit(0xD4);  // 配置数据 - 寄存器1：DR=20SPS, 连续转换模式
    SPI_Transmit(0x17);  // 配置数据 - 寄存器2：IDAC关闭
    SPI_Transmit(0xA0);  // 配置数据 - 寄存器3：默认设置
    vTaskDelay(pdMS_TO_TICKS(1)); 
    SPI_Transmit(0x23);
    vTaskDelay(pdMS_TO_TICKS(1)); 
    SPI_Transmit(0x08);  
    vTaskDelay(pdMS_TO_TICKS(1)); 

    SPI_CS1_HIGH();

    vTaskDelay(pdMS_TO_TICKS(1)); 
    printf("--------System Start!\r\n");



    SPI_CS2_LOW();
    /* 发送复位命令 */
    SPI_Transmit(0x06);  // RESET命令

    vTaskDelay(pdMS_TO_TICKS(1)); 


    SPI_Transmit(0x43);  // WREG命令，写寄存器3
    SPI_Transmit(0x00);  // 配置数据 - 寄存器0：PGA=1, AIN0/AIN1
    SPI_Transmit(0xD4);  // 配置数据 - 寄存器1：DR=20SPS, 连续转换模式
    SPI_Transmit(0x17);  // 配置数据 - 寄存器2：IDAC关闭
    SPI_Transmit(0xA0);  // 配置数据 - 寄存器3：默认设置
    vTaskDelay(pdMS_TO_TICKS(1)); 

    SPI_Transmit(0x23);
    vTaskDelay(pdMS_TO_TICKS(1)); 

    SPI_Transmit(0x08);  
    vTaskDelay(pdMS_TO_TICKS(1));

    SPI_CS2_HIGH();

    vTaskDelay(pdMS_TO_TICKS(1));

    /* EXTI interrupt init for PH9 */
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    
    /* EXTI interrupt init for PH10 */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);



    printf("ADS1220_Task\n");
    while(1) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void ADS1220_Init(void)
{
        /* SPI4 参数配置 */
        hspi4.Instance = SPI4;
        hspi4.Init.Mode = SPI_MODE_MASTER;
        hspi4.Init.Direction = SPI_DIRECTION_2LINES;
        hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
        hspi4.Init.CLKPolarity = SPI_POLARITY_LOW;    // CPOL = 0
        hspi4.Init.CLKPhase = SPI_PHASE_2EDGE;        // CPHA = 1
        hspi4.Init.NSS = SPI_NSS_SOFT;
        hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;  // 确保SCLK周期>150ns
        hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
        hspi4.Init.TIMode = SPI_TIMODE_DISABLE;
        hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
        hspi4.Init.CRCPolynomial = 7;
        hspi4.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
        hspi4.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
        hspi4.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;
    
    
    
      if (HAL_SPI_Init(&hspi4) != HAL_OK)
      {
        Error_Handler();
      }
    
    
    
    
    
    
      /* USER CODE BEGIN SPI4_Init 2 */
      /* CS引脚配置 */
      GPIO_InitTypeDef GPIO_InitStruct = {0};
      
    
    
    
    
      /* CS1引脚配置 */
      GPIO_InitStruct.Pin = GPIO_PIN_11;
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
      GPIO_InitStruct.Pull = GPIO_PULLUP;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
      HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
      /* CS1默认高电平 */
      HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_SET);
      
      /* CS2引脚配置 */
      GPIO_InitStruct.Pin = GPIO_PIN_15;
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
      GPIO_InitStruct.Pull = GPIO_PULLUP;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
      HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
      /* CS2默认高电平 */
      HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_SET);
    
      /* 所有初始化完成后，启用EXTI中断 */
      printf("ADS1220 initialization completed, enabling EXTI interrupts\r\n");
      

}

void SPI_Transmit(uint8_t data) {
    uint8_t txData[] = {data};  // 发送数据缓冲区
    uint8_t rxData[1];          // 接收数据缓冲区

    /* 同时发送和接收数据 */
    if(HAL_SPI_TransmitReceive(&hspi4, txData, rxData, sizeof(txData), 100) != HAL_OK)
    {
        Error_Handler();
    }

    /* 打印发送和接收的数据 */
    printf("SPI: TX=0x%02X, RX=0x%02X\r\n", txData[0], rxData[0]);
}

uint8_t SPI_TransmitReceive(uint8_t data) {
    uint8_t txData[] = {data}; 
    uint8_t rxData[1];         


    if(HAL_SPI_TransmitReceive(&hspi4, txData, rxData, sizeof(txData), 100) != HAL_OK)
    {
        Error_Handler();
    }
    return rxData[0];
}

/**
  * @brief GPIO EXTI callback function
  * @param GPIO_Pin: Pin connected to EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == GPIO_PIN_10)
    {
   
       SPI_CS2_LOW();
       uint32_t adc_value = 0;
       uint8_t msb = SPI_TransmitReceive(0xFF);  // 读取高8位
       uint8_t mid = SPI_TransmitReceive(0xFF);  // 读取中8位
       uint8_t lsb = SPI_TransmitReceive(0xFF);  // 读取低8位
       SPI_CS2_HIGH();
       adc_value = (uint32_t)msb << 16 | (uint32_t)mid << 8 | lsb;
        float voltage = Convert_ADC_To_Voltage(adc_value);  
        float current =  voltage * 4000+  0.296;

        voltage_sum += current;
        raw_voltage_sum += voltage; // 累加原始电压值
        sample_count++;

        /* 每1000次采样计算一次平均值并显示 */
        if(sample_count >= 1000)
        {
            float avg_current = voltage_sum / sample_count;
            float avg_voltage = raw_voltage_sum / sample_count; // 计算平均电压值
            
            // 发送到串口
            char buffer1[80];
            sprintf(buffer1, "current_uA:%.6f uA, avg_voltage:%.6f V\r\n", avg_current, avg_voltage);
            printf(buffer1);
            
            // 定义LCD显示区域的位置
            uint16_t text_y = (LCD_HEIGHT - 12)/2; // 使用屏幕中心
            uint16_t value_x = 90;  // 值显示的起始x坐标
            uint16_t current_y = text_y + 20;  // 电流显示的y坐标，往下移动
            uint16_t voltage_y = text_y - 10;  // 电压显示的y坐标，往下移动
        

            // 显示标签
            LCD_Show_String(5, current_y, "Current:", COLOR_WHITE, COLOR_BLACK, FONT_1608);
            LCD_Show_String(5, voltage_y, "Voltage:", COLOR_WHITE, COLOR_BLACK, FONT_1608);
            
            // 清除上一次显示的值
            LCD_Fill_Rect(value_x, current_y - 2, LCD_WIDTH - 10, current_y + 18, COLOR_BLACK);
            LCD_Fill_Rect(value_x, voltage_y - 2, LCD_WIDTH - 10, voltage_y + 18, COLOR_BLACK);
            
            // 格式化显示电流值 (uA)
            char current_str[30];
            sprintf(current_str, "%.3f uA", avg_current);
            // 格式化显示电压值 (V)
            char voltage_str[30];
            sprintf(voltage_str, "%.6f V", avg_voltage);
            
            // 显示到LCD
            LCD_Show_String(value_x, current_y, current_str, COLOR_GREEN, COLOR_BLACK, FONT_1608);
            LCD_Show_String(value_x, voltage_y, voltage_str, COLOR_YELLOW, COLOR_BLACK, FONT_1608);
            
            /* 重置计数器和累加器 */
            voltage_sum = 0;
            raw_voltage_sum = 0; // 重置原始电压累加值
            sample_count = 0;
        }
       
     }
     else if(GPIO_Pin == GPIO_PIN_9)
     {


    //    SPI_CS1_LOW();
    //    uint32_t adc_value = 0;
    //    uint8_t msb = SPI_TransmitReceive(0xFF);  // 读取高8位
    //    uint8_t mid = SPI_TransmitReceive(0xFF);  // 读取中8位
    //    uint8_t lsb = SPI_TransmitReceive(0xFF);  // 读取低8位
    //    SPI_CS1_HIGH();
    //    adc_value = (uint32_t)msb << 16 | (uint32_t)mid << 8 | lsb;
   

    //      float voltage = Convert_ADC_To_Voltage(adc_value);  
    //      float current =  (voltage * 797 )*3+  0.202;
   
    //      voltage_sum_2 += current;
   
   
    //      sample_count_2++;
   
    //      if(sample_count_2 >= 1000)
    //      {
    //        float avg_voltage = voltage_sum_2 / sample_count_2;
    //        char buffer1[50];
    //        sprintf(buffer1, "current_mA :%.6f  mA\r\n", avg_voltage); // 格式化为两位小数
    //        printf(buffer1); // 发送到串口
    //        /* 重置计数器和累加器 */
    //        voltage_sum_2 = 0;
    //        sample_count_2 = 0;
    //      }
   
   
   
     }
   
     
}