#ifndef __USB_COMMAND_PC_TO_ST_TASK_H
#define __USB_COMMAND_PC_TO_ST_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdint.h>

// 定义缓冲区大小
#define USB_RX_BUFFER_SIZE 64

// 定义USB数据结构体
typedef struct {
    uint8_t* Buf;      // 数据缓冲区指针
    uint32_t* Len;     // 数据长度指针
} USB_Data_TypeDef;

// 使用静态缓冲区
extern volatile uint8_t usb_rx_buffer[USB_RX_BUFFER_SIZE];
extern volatile uint8_t usb_rx_length;
extern volatile uint8_t usb_rx_flag;

// 声明消息队列句柄
extern QueueHandle_t usbMessageQueueHandle;

void usb_command_pc_to_st_task(void *argument);

#endif /* __USB_COMMAND_PC_TO_ST_TASK_H */