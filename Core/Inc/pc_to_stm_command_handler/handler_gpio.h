/**
 * @file handler_gpio.h
 * @brief GPIO处理相关函数头文件
 */
#ifndef HANDLER_GPIO_H
#define HANDLER_GPIO_H

#include "main.h"
#include "stm32h7xx_hal.h"



HAL_StatusTypeDef Handler_GPIO_SetOutput(uint8_t gpio_index, uint8_t pull_mode);

//@param pull_mode 上拉下拉模式：0=无上拉下拉，1=上拉，2=下拉
HAL_StatusTypeDef Handler_GPIO_Write(uint8_t gpio_index, uint8_t write_value);

//@param pull_mode 上拉下拉模式：0=无上拉下拉，1=上拉，2=下拉
HAL_StatusTypeDef Handler_GPIO_SetInput(uint8_t gpio_index, uint8_t pull_mode);

#endif // HANDLER_GPIO_H
