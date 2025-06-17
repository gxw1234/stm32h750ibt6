#include "tasks/MP8865_task.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "usbd_cdc_if.h"

#define DATA_SIZE 40960  // 恢复到原始工作值
uint8_t myData[DATA_SIZE];

I2C_HandleTypeDef hi2c4;



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
        

    }

    while (1)
    {

      vTaskDelay(pdMS_TO_TICKS(1000));




    }
}
