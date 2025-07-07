/**
 * @file handler_gpio.c
 * @brief GPIO处理相关函数实现
 */
#include "handler_gpio.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart1;


typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
} GPIO_PIN_MAP;

static const GPIO_PIN_MAP gpio_map[] = {
    {NULL, 0},         // 0: 未用
    {GPIOH, GPIO_PIN_4}, // 1: H4
    {GPIOH, GPIO_PIN_5}, // 2: H5
    {GPIOF, GPIO_PIN_11},// 3: F11
    {GPIOH, GPIO_PIN_6}, // 4: H6
    {GPIOH, GPIO_PIN_7}, // 5: H7
    {GPIOH, GPIO_PIN_8}, // 6: H8
    {NULL, 0},         // 7: 未用
    {NULL, 0},         // 8: 未用
    {GPIOE, GPIO_PIN_10},// 9: E10
    {GPIOE, GPIO_PIN_7}, // 10: E7
};




HAL_StatusTypeDef Handler_GPIO_SetOutput(uint8_t gpio_index, uint8_t output_mask) {
    if (gpio_index >= sizeof(gpio_map)/sizeof(gpio_map[0]) || gpio_map[gpio_index].port == NULL) {
        return HAL_ERROR;
    }
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = gpio_map[gpio_index].pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(gpio_map[gpio_index].port, &GPIO_InitStruct);
    // 可选：根据output_mask设置初始电平
    HAL_GPIO_WritePin(
        gpio_map[gpio_index].port,
        gpio_map[gpio_index].pin,
        (output_mask & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET
    );
    printf("GPIO set output: index=%d, pin=0x%04X, mask=0x%02X, state=%d\r\n", gpio_index, gpio_map[gpio_index].pin, output_mask, (output_mask & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return HAL_OK;
}


HAL_StatusTypeDef Handler_GPIO_Write(uint8_t gpio_index, uint8_t write_value) {
    if (gpio_index >= sizeof(gpio_map)/sizeof(gpio_map[0]) || gpio_map[gpio_index].port == NULL) {
        return HAL_ERROR;
    }
    GPIO_PinState pin_state = (write_value & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(gpio_map[gpio_index].port, gpio_map[gpio_index].pin, pin_state);
    printf("---GPIO write: index=%d, pin=0x%04X, value=0x%02X, state=%d\r\n", gpio_index, gpio_map[gpio_index].pin, write_value, pin_state);
    return HAL_OK;
}
