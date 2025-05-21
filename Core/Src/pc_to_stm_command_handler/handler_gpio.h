/**
 * @file handler_gpio.h
 * @brief GPIO处理相关函数头文件
 */
#ifndef HANDLER_GPIO_H
#define HANDLER_GPIO_H

#include "main.h"
#include "stm32h7xx_hal.h"

// GPIO索引定义
#define GPIO_PORT0       0x00    // GPIO端口0
#define GPIO_PORT1       0x01    // GPIO端口1
#define GPIO_PORT2       0x02    // GPIO端口2

// GPIO方向定义
#define GPIO_DIR_OUTPUT  0x01    // 输出模式
#define GPIO_DIR_INPUT   0x00    // 输入模式

/**
 * @brief 设置GPIO为输出模式
 * 
 * @param gpio_index GPIO索引
 * @param output_mask 输出引脚掩码，每个位对应一个引脚，1表示设置为输出
 * @return HAL_StatusTypeDef 操作结果
 */
HAL_StatusTypeDef Handler_GPIO_SetOutput(uint8_t gpio_index, uint8_t output_mask);

/**
 * @brief 写入GPIO输出值
 * 
 * @param gpio_index GPIO索引
 * @param write_value 写入的值，每个位对应一个引脚
 * @return HAL_StatusTypeDef 操作结果
 */
HAL_StatusTypeDef Handler_GPIO_Write(uint8_t gpio_index, uint8_t write_value);

#endif // HANDLER_GPIO_H
