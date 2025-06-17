/**
 * @file handler_gpio.c
 * @brief GPIO处理相关函数实现
 */
#include "handler_gpio.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart1;

// 定义GPIO端口和引脚映射
typedef struct {
    GPIO_TypeDef* port;
    uint16_t pins[8];  // 每个端口最多8个引脚
} GPIO_MAP;

// GPIO端口映射表
static const GPIO_MAP gpio_map[] = {
    // GPIO_PORT0 - 映射到PH7引脚
    {
        GPIOH,
        {GPIO_PIN_7, GPIO_PIN_7, GPIO_PIN_7, GPIO_PIN_7, GPIO_PIN_7, GPIO_PIN_7, GPIO_PIN_7, GPIO_PIN_7}
    },
    // GPIO_PORT1
    {
        GPIOB,
        {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7}
    },
    // GPIO_PORT2
    {
        GPIOC,
        {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7}
    }
};

// 记录当前GPIO输出状态
static uint8_t gpio_output_state[3] = {0, 0, 0};

/**
 * @brief 设置GPIO引脚为输出模式
 * 
 * @param gpio_index GPIO索引
 * @param output_mask 输出引脚掩码，每个位对应一个引脚，1表示设置为输出
 * @return HAL_StatusTypeDef 操作结果
 */
HAL_StatusTypeDef Handler_GPIO_SetOutput(uint8_t gpio_index, uint8_t output_mask) {


    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    // 当gpio_index为0时，直接控制PH7引脚
    if (gpio_index == 0) {
        GPIO_InitStruct.Pin = GPIO_PIN_7;         // PH7引脚
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);  // 在GPIOH端口上初始化
        
        // 默认设置为高电平
        HAL_GPIO_WritePin(GPIOH, GPIO_PIN_7, GPIO_PIN_SET);
        
        // 保存输出状态
        gpio_output_state[gpio_index] = output_mask;


        GPIO_InitStruct.Pin = GPIO_PIN_8;         // PH7引脚
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);  // 在GPIOH端口上初始化


        printf("PH7 configured as output and set HIGH, mask: 0x%02X\r\n", output_mask);
        
        return HAL_OK;
    }
    else {
        // 其他端口保持原有逻辑

      
    }
    
    return HAL_ERROR;
}

/**
 * @brief 写入GPIO输出值
 * 
 * @param gpio_index GPIO索引
 * @param write_value 写入的值，每个位对应一个引脚
 * @return HAL_StatusTypeDef 操作结果
 */
HAL_StatusTypeDef Handler_GPIO_Write(uint8_t gpio_index, uint8_t write_value) {
    // 检查索引是否有效
    if (gpio_index >= sizeof(gpio_map) / sizeof(gpio_map[0])) {
        printf("Invalid GPIO index: %d\r\n", gpio_index);
        return HAL_ERROR;
    }
    
    // 当gpio_index为0时，直接控制PH7引脚
    if (gpio_index == 0) {
        // 使用写入值的第一位(LSB)来确定引脚状态
        GPIO_PinState pin_state = (write_value & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET;
        
        HAL_GPIO_WritePin(GPIOH, GPIO_PIN_7, pin_state);
        
        printf("PH7 write value: 0x%02X, pin state: %d\r\n", write_value, pin_state);
        return HAL_OK;
    }
    else {

        return HAL_OK;
    }
}
