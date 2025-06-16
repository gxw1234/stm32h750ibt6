/**
 * @file test_usb_send.c
 * @brief USB发送测试任务的实现
 */

#include "tasks/test_usb_send.h"
#include "usbd_cdc_if.h"
#include "cmsis_os.h"
#include "main.h"
#include <stdio.h>

/* 用于IIC从机模式的全局变量 */
struct rx_tx {
  uint8_t rx;
  uint8_t tx;
};
__IO uint32_t Transfer_Direction = 0;
__IO uint32_t Xfer_Complete = 0;

struct rx_tx rx_tx_data[] = {
  {0xfc, 0x39},
  {0xfd, 0x01},
};

/* Data buffer */
uint8_t aTxBuffer[4] = {0x39, 0x01, 0, 0};  // Transmit buffer
uint8_t aRxBuffer[4] = {0};  // Receive buffer

TaskHandle_t UsbSendTaskHandle;
I2C_HandleTypeDef hi2c3_test_; 

#define DATA_SIZE 64
static uint8_t myData[DATA_SIZE];

/**
 * @brief Initialize USB send test task
 */
void USB_Send_Task_Init(void)
{
    for(uint16_t i = 0; i < DATA_SIZE; i++)
    {
        myData[i] = (uint8_t)i;
    }
}



/**
 * @brief Initialize I2C3 as slave mode
 * @retval HAL status
 */
HAL_StatusTypeDef Test_I2C3_Slave_Init(void)
{

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    

    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    

    GPIO_InitStruct.Pin = GPIO_PIN_9;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);


    __HAL_RCC_I2C3_CLK_ENABLE();
    

    // hi2c3_test_.Instance = I2C3;
    // hi2c3_test_.Init.Timing = 0x10D0A9FF; // Fast mode (400 KHz)
    // hi2c3_test_.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    // hi2c3_test_.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    // hi2c3_test_.Init.OwnAddress1 = 0x6e<<1; // Slave address 0x6E (7-bit)
    // hi2c3_test_.Init.OwnAddress2 = 0;
    // hi2c3_test_.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    // hi2c3_test_.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    // hi2c3_test_.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    

    hi2c3_test_.Instance = I2C3;
    hi2c3_test_.Init.Timing = 0x00300F38;
    hi2c3_test_.Init.OwnAddress1 = 0xdc;
    hi2c3_test_.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c3_test_.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c3_test_.Init.OwnAddress2 = 0;
    hi2c3_test_.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c3_test_.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c3_test_.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_StatusTypeDef status = HAL_I2C_Init(&hi2c3_test_);
    
    if (status == HAL_OK) {
        printf("I2C3 slave mode init success, address: 0x%02X\r\n", 0x6e);
        status = HAL_I2C_EnableListen_IT(&hi2c3_test_);
        if (status != HAL_OK) {
            printf("Failed to enable I2C3 listen mode, error code: %d\r\n", status);
        } else {
            printf("I2C3 listen mode enabled\r\n");
        }
    } else {
        printf("I2C3 slave mode init failed, error code: %d\r\n", status);
    }

    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3_test_, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
    {
      Error_Handler();
    }
  
    /** Configure Digital filter
    */
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3_test_, 0) != HAL_OK)
    {
      Error_Handler();
    }
    
    return status;
}

/**
 * @brief USB send test task
 * @param argument Not used
 */
void USB_Send_Task(void *argument)
{
    /* Prevent compiler warning for unused argument */
    (void)argument;

    vTaskDelay(pdMS_TO_TICKS(1000));
    USB_Send_Task_Init();

    Test_I2C3_Slave_Init();

    while (1)
    {
        if (Xfer_Complete == 1) {
            while (HAL_I2C_GetState(&hi2c3_test_) != HAL_I2C_STATE_READY){}
            HAL_I2C_EnableListen_IT(&hi2c3_test_);
            Xfer_Complete = 0;

        }
       
    }
}

/**
 * @brief I2C slave transmission complete callback function
 */
void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *I2cHandle) {
    Xfer_Complete = 1;
    HAL_I2C_Slave_Seq_Transmit_IT(&hi2c3_test_, (uint8_t *)&aTxBuffer[1], 1, I2C_NEXT_FRAME);
    printf("I2C slave TX complete callback\r\n");
}

/**
 * @brief I2C slave reception complete callback function
 */
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *I2cHandle) {
    Xfer_Complete = 1;
    HAL_I2C_Slave_Seq_Receive_IT(&hi2c3_test_, &aRxBuffer[1], 1, I2C_NEXT_FRAME);
    printf("I2C slave RX complete callback, received: 0x%02X\r\n", aRxBuffer[0]);
}

/**
 * @brief I2C address match callback function
 */
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode) {
    Transfer_Direction = TransferDirection;
    
    if (Transfer_Direction != I2C_DIRECTION_TRANSMIT) {
        // Master read request, slave sending data
        for (int i = 0; i < sizeof(rx_tx_data) / sizeof(rx_tx_data[0]); i++) {
            if (aRxBuffer[0] == rx_tx_data[i].rx) {
                aTxBuffer[0] = rx_tx_data[i].tx;
            }
        }
        
        printf("Address match callback: Master READ, sending data: 0x%02X\r\n", aTxBuffer[0]);
        
        if (HAL_I2C_Slave_Seq_Transmit_IT(&hi2c3_test_, (uint8_t *)&aTxBuffer[0], 1, I2C_NEXT_FRAME) != HAL_OK) {
            printf("Failed to set slave transmit\r\n");
        }
    }
    else {
        // Master write request, slave receiving data
        printf("Address match callback: Master WRITE, preparing to receive data\r\n");
        
        if (HAL_I2C_Slave_Seq_Receive_IT(&hi2c3_test_, (uint8_t *)&aRxBuffer[0], 1, I2C_NEXT_FRAME) != HAL_OK) {
            printf("Failed to set slave receive\r\n");
        }
    }
}

/**
 * @brief I2C listen complete callback function
 */
void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c) {
    printf("I2C listen complete callback\r\n");
}

