/**
  ******************************************************************************
  * @file           : gpio_init.c
  * @brief          : GPIO initialization function implementations
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "init/gpio_init.h"
#include <stdio.h>

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();  // LCD使用的GPIOG
  __HAL_RCC_GPIOI_CLK_ENABLE();  // LCD使用的GPIOI

  __HAL_RCC_GPIOE_CLK_ENABLE();

/* USER CODE BEGIN MX_GPIO_Init_2 */
  // LCD引脚初始化（ST7789）- 与原始工作项目保持一致
  // 基于软件SPI，所有引脚配置为输出模式
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;  // 改为无上拉下拉，与原始项目一致
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  
  // GPIOG引脚初始化 - LCD信号引脚
  GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
  
  // GPIOI引脚初始化 - LCD背光控制
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);
  

  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_8, GPIO_PIN_SET);   // CS拉高，不选中
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_12, GPIO_PIN_SET);  // RST拉高
  HAL_GPIO_WritePin(GPIOI, GPIO_PIN_6, GPIO_PIN_RESET); // 背光打开（低电平激活）
  
  /* 配置外部中断引脚 */
  /*Configure GPIO pin : PH10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /*Configure GPIO pin : PH9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
  
  /* EXTI中断将在ADS1220_Init()函数结束时启用 */


  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
  
  printf("GPIO initialization completed with LCD pin settings and interrupts\r\n");
/* USER CODE END MX_GPIO_Init_2 */
}
