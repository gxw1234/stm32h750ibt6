#include "tasks/ads1220_task.h"
#include "tasks/lcd_task.h"  /* 包含 LCD 函数声明 */
#include <stdio.h>
#include "usb_device.h"
#include "usbd_cdc_if.h"

#define SPI_CS1_LOW()       HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_RESET)
#define SPI_CS1_HIGH()      HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_SET)

/* CS2引脚控制宏定义 */
#define SPI_CS2_LOW()       HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_RESET)
#define SPI_CS2_HIGH()      HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_SET)

/* SPI句柄定义 */
SPI_HandleTypeDef hspi4;


static uint32_t sample_count = 0;
static float voltage_sum = 0;
static float raw_voltage_sum = 0; // 用于存储原始电压累加值

static uint32_t sample_count_2 = 0;
static float voltage_sum_2 = 0;

// 电压数据发送相关变量
#define DATA_BUFFER_SIZE 20000  // 足够存储1000个浮点数和换行符
static uint8_t data_buffer[DATA_BUFFER_SIZE];  // 大的数据缓冲区
static uint8_t sending_enabled = 0;  // 是否启用数据发送
static uint32_t buffer_pos = 0;  // 数据缓冲区当前位置

/**
 * @brief 启用电压数据发送
 */
void Enable_Current_Data_Sending(void) {
    buffer_pos = 0;  // 重置缓冲区位置
    sending_enabled = 1;  // 启用发送开关
}

/**
 * @brief 禁用电压数据发送
 */
void Disable_Current_Data_Sending(void) {
    sending_enabled = 0;  // 关闭发送开关
}

// uint32_t adc_value = 0;

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

    vTaskDelay(pdMS_TO_TICKS(1));

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
        float current = 2008*voltage +  0.27 ;
        voltage_sum += current;
        raw_voltage_sum += voltage; // 累加原始电压值
        sample_count++;
        if(sending_enabled && (buffer_pos + 20) < DATA_BUFFER_SIZE) {
            buffer_pos += sprintf((char*)data_buffer + buffer_pos, "%.6f\r\n", current);
        }
        if(sample_count >= 1000)
        {
            // 定义静态变量保存屏幕参数
            static uint16_t text_y = (LCD_HEIGHT - 12)/2; // 使用屏幕中心
            static uint16_t value_x = 90;  // 值显示的起始x坐标
            static uint16_t current_y = 0;  // 动态计算
            static uint16_t voltage_y = 0;  // 动态计算
            static uint8_t first_display = 1; // 第一次显示标志
            
            // 计算平均值
            float avg_current = voltage_sum / sample_count;
            float avg_voltage = raw_voltage_sum / sample_count; // 计算平均电压值
            
            // // 输出到串口
            // char buffer1[80];
            // sprintf(buffer1, "current_uA:%.6f uA, avg_voltage:%.6f V\r\n", avg_current, avg_voltage);
            // printf(buffer1);
            
            // 首次显示时初始化坐标和绘制标签
            if (first_display) {
                current_y = text_y + 20;  // 电流显示的y坐标，往下移动
                voltage_y = text_y - 10;  // 电压显示的y坐标，往下移动
                
                // 绘制固定标签，只绘制一次
                LCD_Show_String(5, current_y, "Current:", COLOR_WHITE, COLOR_BLACK, FONT_1608);
                LCD_Show_String(5, voltage_y, "Voltage:", COLOR_WHITE, COLOR_BLACK, FONT_1608);
                
                first_display = 0; // 清除首次显示标志
            }
            
            // 格式化数值字符串
            char current_str[30] = {0};
            sprintf(current_str, "%.3f uA", avg_current);
            char voltage_str[30] = {0};
            sprintf(voltage_str, "%.6f V", avg_voltage);
            
            // 测量显示函数的执行时间
            uint32_t start_time = HAL_GetTick();
            
            // 使用LCD显示函数
            LCD_Show_String(value_x, current_y, current_str, COLOR_GREEN, COLOR_BLACK, FONT_1608);
            LCD_Show_String(value_x, voltage_y, voltage_str, COLOR_YELLOW, COLOR_BLACK, FONT_1608);
            
            uint32_t end_time = HAL_GetTick();
            uint32_t elapsed_time = end_time - start_time;
            
            // 输出执行时间
            char time_str[30];
            sprintf(time_str, "LCD time: %lu ms", elapsed_time);
            printf("%s\r\n", time_str);
            
            // 如果启用了发送，则将所有积累的数据一次性发送
            if (sending_enabled && buffer_pos > 0) {
                printf("send data\n");
                // 发送缓冲区中的所有数据
                CDC_Transmit_HS(data_buffer, buffer_pos);
                
                // 重置缓冲区位置
                buffer_pos = 0;
            }
            /* 重置计数器和累加器 */
            voltage_sum = 0;
            raw_voltage_sum = 0; // 重置原始电压累加值
            sample_count = 0;
        }
       
     }
     else if(GPIO_Pin == GPIO_PIN_9)
     {

       SPI_CS1_LOW();
       uint32_t adc_value = 0;
       uint8_t msb = SPI_TransmitReceive(0xFF);  // 读取高8位
       uint8_t mid = SPI_TransmitReceive(0xFF);  // 读取中8位
       uint8_t lsb = SPI_TransmitReceive(0xFF);  // 读取低8位
       SPI_CS1_HIGH();
       adc_value = (uint32_t)msb << 16 | (uint32_t)mid << 8 | lsb;
        float voltage = Convert_ADC_To_Voltage(adc_value);  
        float current =  voltage *1189.7  +  0.222;
        voltage_sum_2 += current;
        sample_count_2++;

        if(sample_count_2 >= 1000)
        {
        // 定义静态变量保存屏幕参数
        static uint16_t text_y = (LCD_HEIGHT - 12)/2; // 使用屏幕中心
        static uint16_t value_x = 90;  // 值显示的起始x坐标
        static uint16_t current_mA_y = 0; // 毫安电流显示的y坐标
        static uint8_t first_display_mA = 1; // 毫安电流首次显示标志
        
        // 计算平均毫安电流
        float avg_current_mA = voltage_sum_2 / sample_count_2;
        
        // 输出到串口
        char buffer1[50];
        sprintf(buffer1, "current_mA :%.6f  mA\r\n", avg_current_mA); // 格式化为两位小数
        printf(buffer1); // 发送到串口
        
        // 首次显示时初始化坐标和绘制标签
        if (first_display_mA) {
            // 计算毫安电流显示位置（位置与GPIO_PIN_10不同，避免重叠）
            current_mA_y = text_y + 50;  // 毫安电流显示的y坐标，比微安更下方
            
            // 绘制固定标签，只绘制一次
            LCD_Show_String(5, current_mA_y, "Current:", COLOR_WHITE, COLOR_BLACK, FONT_1608);
            
            first_display_mA = 0; // 清除首次显示标志
        }
        
        // 格式化毫安电流值字符串
        char current_str[30] = {0};
        sprintf(current_str, "%.3f mA", avg_current_mA);
        
        // 使用无背景显示函数，只绘制字符前景部分
        LCD_Show_String(value_x, current_mA_y, current_str, COLOR_CYAN, COLOR_BLACK, FONT_1608);
        
        /* 重置计数器和累加器 */
        voltage_sum_2 = 0;
        sample_count_2 = 0;
        }
     }
   
     
}