#include "tasks/usb_command_pc_to_st_task.h"
#include "usbd_cdc_if.h"
#include <stdio.h>
#include <string.h>

// 定义大缓冲区用于累积数据
static uint8_t large_buffer[USB_LARGE_BUFFER_SIZE];
static uint32_t accumulated_length = 0;
static uint8_t packet_complete = 0;

// 定义消息队列句柄
QueueHandle_t usbMessageQueueHandle = NULL;

void usb_command_pc_to_st_task(void *argument)
{
    printf("USB task starting...\r\n");
    
    // 初始化累积缓冲区状态
    accumulated_length = 0;
    packet_complete = 0;
    memset(large_buffer, 0, USB_LARGE_BUFFER_SIZE);
    
    usbMessageQueueHandle = xQueueCreate(10, sizeof(USB_Data_TypeDef));
    if (usbMessageQueueHandle == NULL) {
        printf("Queue create failed!\r\n");
        vTaskDelete(NULL);
        return;
    }
    printf("--------Queue created----\r\n");
    USB_Data_TypeDef usbData;
    while (1) {
        if (xQueueReceive(usbMessageQueueHandle, &usbData, portMAX_DELAY) == pdPASS) {


            
           
        }
    }
}
