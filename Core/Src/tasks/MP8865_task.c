#include "tasks/MP8865_task.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"


I2C_HandleTypeDef hi2c4;
I2C_HandleTypeDef hi2c3_test_; // 用于测试I2C3从机模式


#define I2C_DEVICE_ADDR    0x60    
#define I2C_REG_ADDR_01       0x01   
#define I2C_REG_ADDR_00       0x00    
#define I2C_DATA_VALUE     0b11111111   


/**
 * @brief 设置MP8865输出电压
 * @param voltage_value: 寄存器0x00的值，用于设置输出电压
 * @return HAL_StatusTypeDef: 操作状态
 */
HAL_StatusTypeDef MP8865_SetVoltage(uint8_t voltage_value)
{
    
    printf("MP8865_SetVoltage: Value: 0x%02X (Bin: %d%d%d%d%d%d%d%d)\r\n", 
           voltage_value,
           (voltage_value & 0x80) ? 1 : 0,
           (voltage_value & 0x40) ? 1 : 0,
           (voltage_value & 0x20) ? 1 : 0,
           (voltage_value & 0x10) ? 1 : 0,
           (voltage_value & 0x08) ? 1 : 0,
           (voltage_value & 0x04) ? 1 : 0,
           (voltage_value & 0x02) ? 1 : 0,
           (voltage_value & 0x01) ? 1 : 0);
    
    HAL_StatusTypeDef status;
    uint8_t data[2];
    data[0] = 0x01;
    data[1] = 0xE0; // 11100000 - 启用GO_BIT
    status = HAL_I2C_Master_Transmit(&hi2c4, I2C_DEVICE_ADDR << 1, data, 2, HAL_MAX_DELAY);
    if (status != HAL_OK)
        return status;
    data[0] = 0x00;
    data[1] = voltage_value;
    status = HAL_I2C_Master_Transmit(&hi2c4, I2C_DEVICE_ADDR << 1, data, 2, HAL_MAX_DELAY);
    return status;

}

void MP8865_Init(void)
{

    hi2c4.Instance = I2C4;
    hi2c4.Init.Timing = 0x00707CBB;
    hi2c4.Init.OwnAddress1 = 0;
    hi2c4.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c4.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c4.Init.OwnAddress2 = 0;
    hi2c4.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c4.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c4.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c4) != HAL_OK)
    {
      Error_Handler();
    }
  
    /** Configure Analogue filter
    */
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c4, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
    {
      Error_Handler();
    }
  
    /** Configure Digital filter
    */
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c4, 0) != HAL_OK)
    {
      Error_Handler();
    }

}
// 测试I2C3从机模式初始化函数
HAL_StatusTypeDef Test_I2C3_Slave_Init(void)
{
    // 使能 GPIO 时钟
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    // 配置GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // 配置PA8作为I2C3 SCL
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // 配置PC9作为I2C3 SDA
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    // 使能I2C3时钟
    __HAL_RCC_I2C3_CLK_ENABLE();
    
    // 配置I2C参数
    hi2c3_test_.Instance = I2C3;
    hi2c3_test_.Init.Timing = 0x10D0A9FF; // 快速模式 (400 KHz)
    hi2c3_test_.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c3_test_.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c3_test_.Init.OwnAddress1 = 0x6e; // 从机地址设置为0x30
    hi2c3_test_.Init.OwnAddress2 = 0;
    hi2c3_test_.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c3_test_.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c3_test_.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    
    return HAL_I2C_Init(&hi2c3_test_);
}

// 测试I2C3从机模式发送函数
HAL_StatusTypeDef Test_I2C3_Slave_Transmit(void)
{
    printf("Starting I2C3 slave transmit test...\r\n");
    
    uint8_t test_data[4] = {0x11, 0x22, 0x33, 0x44};
    HAL_StatusTypeDef status;
    
    printf("HAL_I2C_Slave_Transmit will be called, timeout = 1000ms\r\n");
    status = HAL_I2C_Slave_Transmit(&hi2c3_test_, test_data, 4, 1000);
    printf("HAL_I2C_Slave_Transmit completed, status = %d\r\n", status);
    
    return status;
}

void MP8865_Task(void *pvParameters)
{
    MP8865_Init();
  
    vTaskDelay(pdMS_TO_TICKS(1000)); 
    HAL_StatusTypeDef status = MP8865_SetVoltage(0b110111010);
    
    if (status != HAL_OK)
    {
        Error_Handler();
    }
    else
    {
        printf("MP8865 Init Success!\r\n");
        
        // 在MP8865初始化成功后，测试I2C3从机模式
        printf("Starting I2C3 slave mode test...\r\n");
        
        // 初始化I2C3从机模式
        status = Test_I2C3_Slave_Init();
        if (status != HAL_OK)
        {
            printf("I2C3 slave initialization failed, status = %d\r\n", status);
        }
        else
        {
            printf("I2C3 slave initialized successfully, trying to transmit data...\r\n");
            vTaskDelay(pdMS_TO_TICKS(500));
            

        }
    }
    
    while (1)
    {

        // 测试从机发送，这里可能会导致系统崩溃
        // status = Test_I2C3_Slave_Transmit();
        // // 如果程序执行到这里，说明没有崩溃
        // printf("I2C3 slave transmit test completed, status = %d\r\n", status);
        
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}
