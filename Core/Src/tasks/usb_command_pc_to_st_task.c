#include "tasks/usb_command_pc_to_st_task.h"
#include "usbd_cdc_if.h"
#include <stdio.h>
#include <string.h>
#include "pc_to_stm_command_handler/command_handler.h"

// 定义结束标记
#define CMD_END_MARKER      0xA5A5A5A5 // 命令包结束符





// 定义大缓冲区用于累积数据
#define BIG_BUFFER_SIZE (96*248+50)
static uint8_t big_buffer[BIG_BUFFER_SIZE];
static uint32_t accumulated_length = 0;
static uint8_t packet_complete = 0;

// 定义消息队列句柄
QueueHandle_t usbMessageQueueHandle = NULL;

void usb_command_pc_to_st_task(void *argument)
{

    
    // 初始化累积缓冲区状态
    accumulated_length = 0;
    packet_complete = 0;
    memset(big_buffer, 0, BIG_BUFFER_SIZE);
    
    usbMessageQueueHandle = xQueueCreate(60, sizeof(USB_Data_TypeDef));
    if (usbMessageQueueHandle == NULL) {
        printf("Queue create failed!\r\n");
        vTaskDelete(NULL);
        return;
    }
    
    printf("--------Queue created----\r\n");
    USB_Data_TypeDef usbData;
    while (1) {
        if (xQueueReceive(usbMessageQueueHandle, &usbData, portMAX_DELAY) == pdPASS) {
            uint32_t len = usbData.Length;
            
            if (accumulated_length + len <= BIG_BUFFER_SIZE) {
                memcpy(big_buffer + accumulated_length, usbData.Buf, len);
                accumulated_length += len;
                if (accumulated_length >= sizeof(uint32_t)) {
                    uint32_t end_marker;
                    memcpy(&end_marker, big_buffer + accumulated_length - sizeof(uint32_t), sizeof(uint32_t));
                    if (end_marker == CMD_END_MARKER) {
                        // printf("Total data: %lu bytes\r\n", accumulated_length);
                        Process_Command(big_buffer, &accumulated_length);
                        accumulated_length = 0;
                        memset(big_buffer, 0, BIG_BUFFER_SIZE);
                    }
                }
            } else {
                printf("Buffer overflow! Reset buffer\r\n");
                accumulated_length = 0;
                memset(big_buffer, 0, BIG_BUFFER_SIZE);
            }
        }
    }
}
