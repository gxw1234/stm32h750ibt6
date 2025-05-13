#ifndef __MP8865_TASK_H
#define __MP8865_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief 设置MP8865输出电压
 * @param voltage_value: 寄存器0x00的值，用于设置输出电压
 * @return HAL_StatusTypeDef: 操作状态
 */
HAL_StatusTypeDef MP8865_SetVoltage(uint8_t voltage_value);

#ifdef __cplusplus
}
#endif

#endif /* __MP8865_TASK_H */
