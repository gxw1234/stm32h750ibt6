#ifndef HANDLER_SPI_H
#define HANDLER_SPI_H

#include "stm32h7xx_hal.h"
#include "command_handler.h"


#define SPI_INDEX_1     1   // SPI5
#define SPI_INDEX_2     2   // 其他SPI接口...


HAL_StatusTypeDef Handler_SPI_Init(uint8_t spi_index, PSPI_CONFIG pConfig);


HAL_StatusTypeDef Handler_SPI_Transmit(uint8_t spi_index, uint8_t *pTxData, uint8_t *pRxData, uint16_t DataSize, uint32_t Timeout);

#endif /* HANDLER_SPI_H */