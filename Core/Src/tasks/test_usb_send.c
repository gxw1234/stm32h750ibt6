/**
 * @file test_usb_send.c
 * @brief USB发送测试任务的实现
 */

#include "tasks/test_usb_send.h"
#include "usbd_cdc_if.h"
#include "cmsis_os.h"


TaskHandle_t UsbSendTaskHandle;


#define DATA_SIZE 64
static uint8_t myData[DATA_SIZE];

/**
 * @brief 初始化USB发送测试任务
 */
void USB_Send_Task_Init(void)
{
    for(uint16_t i = 0; i < DATA_SIZE; i++)
    {
        myData[i] = (uint8_t)i;
    }
}
/**
 * @brief USB发送测试任务
 * @param argument 未使用
 */
void USB_Send_Task(void *argument)
{

    vTaskDelay(pdMS_TO_TICKS(1000));
    USB_Send_Task_Init();

    // uint8_t result = CDC_Transmit_HS(myData, DATA_SIZE);
    // printf("---------result-----------: %d\n", result);
    while (1)
    {
        // uint8_t result = CDC_Transmit_HS(myData, DATA_SIZE);
        // printf("---------result-----------: %d\n", result);
       
        osDelay(1000);
    }
}