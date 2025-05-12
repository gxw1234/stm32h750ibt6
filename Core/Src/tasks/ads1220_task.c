#include "tasks/ads1220_task.h"
#include <stdio.h>

#define SPI_CS1_LOW()       HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_RESET)
#define SPI_CS1_HIGH()      HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_SET)

/* CS2引脚控制宏定义 */
#define SPI_CS2_LOW()       HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_RESET)
#define SPI_CS2_HIGH()      HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_SET)

/* SPI句柄定义 */
SPI_HandleTypeDef hspi4;

void ADS1220_Task(void *argument)
{
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