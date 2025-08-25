#include "tasks/test_iic_send.h"
#include "stm32h7xx_hal_def.h"
#include "usbd_cdc_if.h"
#include "cmsis_os.h"
#include "main.h"
#include <stdio.h>
#include "pc_to_stm_command_handler/command_handler.h"

struct rx_tx {
  uint8_t rx;
  uint8_t tx;
};
__IO uint32_t Transfer_Direction = 0;
__IO uint32_t Xfer_Complete = 0;

struct rx_tx rx_tx_data[] = {
  {0xfc, 0x39},
  {0xfd, 0x01},
  {0x18, 48},
  {0x1a, 120},
  {0x09, 0x10},
  {0x09, 0x00}
};

/* Data buffer */
uint8_t aTxBuffer[4] = {0x39, 0x01, 0, 0};  // Transmit buffer
uint8_t aRxBuffer[4] = {0};  // Receive buffer

TaskHandle_t UsbSendTaskHandle;
I2C_HandleTypeDef hi2c3_test_; 

#define DATA_SIZE 64
static uint8_t myData[DATA_SIZE];

static uint8_t gpio_pressed_flag = 0;
static uint8_t usb_send_flag = 0;

static SemaphoreHandle_t IIC_complete_semaphore = NULL;  //iic信号量
static SemaphoreHandle_t IIC_print_semaphore = NULL;     //打印信号量

HAL_StatusTypeDef Test_I2C3_Slave_Init(void)
{
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
    if (status != HAL_OK)
    {
        Error_Handler();
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
void IIC_interruption_Task(void *argument)
{
    HAL_StatusTypeDef status = HAL_ERROR;
    /* Prevent compiler warning for unused argument */
    (void)argument;
    
    printf("[I2C] Task starting...\r\n");

    if (Test_I2C3_Slave_Init() == HAL_OK) {
        status = HAL_I2C_EnableListen_IT(&hi2c3_test_);
        if (status != HAL_OK) {
            printf("[I2C] Failed to enable listen mode, error: %d\r\n", status);
        } else {
            printf("[I2C] Listen mode enabled successfully\r\n");
        }
    } else {
        printf("[I2C] Slave init failed, error: %d\r\n", status);
    }

    // 信号量
    if (IIC_complete_semaphore == NULL) {
        IIC_complete_semaphore = xSemaphoreCreateBinary();
        if (IIC_complete_semaphore != NULL){
            printf("[I2C] Semaphore created successfully\r\n");
        } else {
            printf("[I2C] Failed to create semaphore, will use polling mode\r\n");
        }
    }

    // 打印信号量
    if (IIC_print_semaphore == NULL) {
        IIC_print_semaphore = xSemaphoreCreateBinary();
        if (IIC_print_semaphore != NULL){
            printf("[I2C] Print semaphore created successfully\r\n");
        } else {
            printf("[I2C] Failed to create print semaphore\r\n");
        }
    }

    while (1)
    {
        if (IIC_complete_semaphore != NULL) {
            if (xSemaphoreTake(IIC_complete_semaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
                while (HAL_I2C_GetState(&hi2c3_test_) != HAL_I2C_STATE_READY){}
                HAL_I2C_EnableListen_IT(&hi2c3_test_);
            }
        }

        if (IIC_print_semaphore != NULL) {
            if (xSemaphoreTake(IIC_print_semaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
                typedef struct {
                    GENERIC_CMD_HEADER header;
                    uint8_t status;  
                } Scan_gpio_Response;
                
                Scan_gpio_Response response;
                response.header.protocol_type = PROTOCOL_SPI;
                response.header.cmd_id = GPIO_SCAN_MODE_WRITE;
                response.header.device_index = 1;
                response.header.param_count = 0;
                response.header.data_len = sizeof(uint8_t);
                response.header.total_packets = sizeof(Scan_gpio_Response);
                response.status = 0;
                USB_Sender((uint8_t*)&response, sizeof(response));
                
                


            }
        }
    }
}



void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *I2cHandle) {
    Xfer_Complete = 1;
    if (IIC_complete_semaphore != NULL) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(IIC_complete_semaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    HAL_I2C_Slave_Seq_Transmit_IT(&hi2c3_test_, (uint8_t *)&aTxBuffer[1], 1, I2C_NEXT_FRAME);

}


void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *I2cHandle) {
    Xfer_Complete = 1;

    if (IIC_complete_semaphore != NULL) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(IIC_complete_semaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    HAL_I2C_Slave_Seq_Receive_IT(&hi2c3_test_, &aRxBuffer[1], 1, I2C_NEXT_FRAME);
    // printf("I2C slave RX complete callback, received: 0x%02X\r\n", aRxBuffer[0]);
    if (aRxBuffer[0] == 0x09) {
        if (aRxBuffer[1] == 0x10) {
            
        }
        else if (aRxBuffer[1] == 0x00) {
            if (IIC_print_semaphore != NULL) {
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                xSemaphoreGiveFromISR(IIC_print_semaphore, &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
    }
}


void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode) {
    Transfer_Direction = TransferDirection;
    
    if (Transfer_Direction != I2C_DIRECTION_TRANSMIT) {
        // Master read request, slave sending data
        for (int i = 0; i < sizeof(rx_tx_data) / sizeof(rx_tx_data[0]); i++) {
            if (aRxBuffer[0] == rx_tx_data[i].rx) {
                aTxBuffer[0] = rx_tx_data[i].tx;

            }
        }
        // printf("Address match callback: Master READ, sending data: 0x%02X\r\n", aTxBuffer[0]);
        if (HAL_I2C_Slave_Seq_Transmit_IT(&hi2c3_test_, (uint8_t *)&aTxBuffer[0], 1, I2C_NEXT_FRAME) != HAL_OK) {
            printf("Failed to set slave transmit\r\n");
        }
    }
    else {
        memset(aRxBuffer, 0xFF, sizeof(aRxBuffer));
        if (HAL_I2C_Slave_Seq_Receive_IT(&hi2c3_test_, (uint8_t *)&aRxBuffer[0], 1, I2C_NEXT_FRAME) != HAL_OK) {
            printf("Failed to set slave receive\r\n");

        }
    }
}


void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c) {
    // printf("I2C listen complete callback\r\n");
}


void Set_GPIO_Press_Flag(void) {
    gpio_pressed_flag = 1;
}
