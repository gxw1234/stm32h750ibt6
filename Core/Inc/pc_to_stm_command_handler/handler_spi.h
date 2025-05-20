#ifndef HANDLER_SPI_H
#define HANDLER_SPI_H

#include "stm32h7xx_hal.h"
#include "command_handler.h"

// SPI接口索引定义
#define SPI_INDEX_0     0   // SPI5
#define SPI_INDEX_1     1   // 其他SPI接口...

/**
 * @brief 初始化SPI接口
 * 
 * @param spi_index SPI接口索引
 * @param pConfig SPI配置参数
 * @return HAL_StatusTypeDef 初始化状态
 */
HAL_StatusTypeDef Handler_SPI_Init(uint8_t spi_index, PSPI_CONFIG pConfig);

/**
 * @brief SPI数据传输函数
 * 
 * @param spi_index SPI接口索引
 * @param pTxData 发送数据缓冲区
 * @param pRxData 接收数据缓冲区
 * @param DataSize 数据大小
 * @param Timeout 超时时间
 * @return HAL_StatusTypeDef 传输状态
 */
HAL_StatusTypeDef Handler_SPI_Transmit(uint8_t spi_index, uint8_t *pTxData, uint8_t *pRxData, uint16_t DataSize, uint32_t Timeout);

#endif /* HANDLER_SPI_H */