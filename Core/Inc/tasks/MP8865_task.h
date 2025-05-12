#ifndef MP8865_TASK_H
#define MP8865_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

void MP8865_Task(void *pvParameters);

void MP8865_Init(void);

/**
 * @brief 设置MP8865输出电压
 * @param voltage_value: 寄存器0x00的值，用于设置输出电压
 * @return HAL_StatusTypeDef: 操作状态
 */
HAL_StatusTypeDef MP8865_SetVoltage(uint8_t voltage_value);

#endif /* MP8865_TASK_H */