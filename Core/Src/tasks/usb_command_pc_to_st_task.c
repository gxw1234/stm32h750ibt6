#include "tasks/usb_command_pc_to_st_task.h"
#include "usbd_cdc_if.h"
#include <stdio.h>



// 定义消息队列句柄
QueueHandle_t usbMessageQueueHandle = NULL;

void usb_command_pc_to_st_task(void *argument)
{
    printf("USB task starting...\r\n");
    
    
    usbMessageQueueHandle = xQueueCreate(1, sizeof(USB_Data_TypeDef));
    if (usbMessageQueueHandle == NULL) {
        printf("Queue create failed!\r\n");
        vTaskDelete(NULL);
        return;
    }
    printf("--------Queue created----\r\n");
    
    USB_Data_TypeDef usbData;
    uint8_t buffer[USB_RX_BUFFER_SIZE];
    uint32_t length = 0;
    
    // 初始化结构体
    usbData.Buf = buffer;
    usbData.Len = &length;
    
    while (1) {
        // 等待通知
        if (xQueueReceive(usbMessageQueueHandle, &usbData, portMAX_DELAY) == pdPASS) {
            printf("--len: %d\r\n", (int)(*usbData.Len));
            }
        }
    }
