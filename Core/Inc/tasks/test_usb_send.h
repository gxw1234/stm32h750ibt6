#ifndef TEST_USB_SEND_H
#define TEST_USB_SEND_H

#include <stdint.h>
#include "cmsis_os.h"

/* 任务相关定义 */
extern TaskHandle_t UsbSendTaskHandle;

/* 函数声明 */
void USB_Send_Task_Init(void);
void USB_Send_Task(void *argument);

#endif /* TEST_USB_SEND_H */