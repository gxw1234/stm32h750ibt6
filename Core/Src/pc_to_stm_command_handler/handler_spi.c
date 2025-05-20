#include "handler_spi.h"
#include "main.h"

// SPI句柄定义
static SPI_HandleTypeDef hspi5;

/**
 * @brief 初始化SPI GPIO引脚
 * 
 * @param spi_index SPI接口索引
 * @return HAL_StatusTypeDef 初始化状态
 */
//index 1 --> SPI5      {PF11 :SPI5_MOSI ,  PH6 :SPI5_SCK   ,PH7 :SPI5_MISO}



static HAL_StatusTypeDef SPI_GPIO_Init(uint8_t spi_index)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    if (spi_index == SPI_INDEX_0) { 
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI5;
        PeriphClkInitStruct.Spi45ClockSelection = RCC_SPI45CLKSOURCE_D2PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
        {
            return HAL_ERROR;
        }

        __HAL_RCC_SPI5_CLK_ENABLE();
        __HAL_RCC_GPIOF_CLK_ENABLE();
        __HAL_RCC_GPIOH_CLK_ENABLE();
        /**SPI5 GPIO Configuration
        PF11    ------> SPI5_MOSI
        PH6     ------> SPI5_SCK
        PH7     ------> SPI5_MISO
        */
        // 配置PH6和PH7
        GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI5;
        HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
        
        // 配置PF11
        GPIO_InitStruct.Pin = GPIO_PIN_11;
        HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
        
        return HAL_OK;
    }
    return HAL_ERROR; 
}

/**
 * @brief 初始化SPI接口
 * 
 * @param spi_index SPI接口索引
 * @param pConfig SPI配置参数
 * @return HAL_StatusTypeDef 初始化状态
 */
HAL_StatusTypeDef Handler_SPI_Init(uint8_t spi_index, PSPI_CONFIG pConfig)
{
    HAL_StatusTypeDef status;
    if (spi_index == SPI_INDEX_0) { // SPI5
        status = SPI_GPIO_Init(spi_index);
        if (status != HAL_OK) {
            return status;
        }
        __HAL_RCC_SPI5_CLK_ENABLE();
        hspi5.Instance = SPI5;
        hspi5.Init.Mode = pConfig->Master ? SPI_MODE_MASTER : SPI_MODE_SLAVE;
        hspi5.Init.Direction = SPI_DIRECTION_2LINES;
        hspi5.Init.DataSize = SPI_DATASIZE_8BIT; // 默认8位数据宽度
        hspi5.Init.CLKPolarity = pConfig->CPOL ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
        hspi5.Init.CLKPhase = pConfig->CPHA ? SPI_PHASE_2EDGE : SPI_PHASE_1EDGE;
        hspi5.Init.NSS = SPI_NSS_SOFT; // 软件NSS管理
        uint32_t spi_clock = HAL_RCC_GetPCLK2Freq(); // 获取PCLK2频率
        uint32_t prescaler = SPI_BAUDRATEPRESCALER_256; // 默认最低速度
        if (pConfig->ClockSpeedHz >= spi_clock / 2)
            prescaler = SPI_BAUDRATEPRESCALER_2;
        else if (pConfig->ClockSpeedHz >= spi_clock / 4)
            prescaler = SPI_BAUDRATEPRESCALER_4;
        else if (pConfig->ClockSpeedHz >= spi_clock / 8)
            prescaler = SPI_BAUDRATEPRESCALER_8;
        else if (pConfig->ClockSpeedHz >= spi_clock / 16)
            prescaler = SPI_BAUDRATEPRESCALER_16;
        else if (pConfig->ClockSpeedHz >= spi_clock / 32)
            prescaler = SPI_BAUDRATEPRESCALER_32;
        else if (pConfig->ClockSpeedHz >= spi_clock / 64)
            prescaler = SPI_BAUDRATEPRESCALER_64;
        else if (pConfig->ClockSpeedHz >= spi_clock / 128)
            prescaler = SPI_BAUDRATEPRESCALER_128;
        hspi5.Init.BaudRatePrescaler = prescaler;
        hspi5.Init.FirstBit = pConfig->LSBFirst ? SPI_FIRSTBIT_LSB : SPI_FIRSTBIT_MSB;
        hspi5.Init.TIMode = SPI_TIMODE_DISABLE;
        hspi5.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
        hspi5.Init.CRCPolynomial = 7;
        return HAL_SPI_Init(&hspi5);
    }
    
    
    return HAL_ERROR; // 未知的SPI索引
}

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
HAL_StatusTypeDef Handler_SPI_Transmit(uint8_t spi_index, uint8_t *pTxData, uint8_t *pRxData, uint16_t DataSize, uint32_t Timeout)
{
    if (spi_index == SPI_INDEX_0) { // SPI5
        if (pRxData != NULL) {
            return HAL_SPI_TransmitReceive(&hspi5, pTxData, pRxData, DataSize, Timeout);
        } else {
            return HAL_SPI_Transmit(&hspi5, pTxData, DataSize, Timeout);
        }
    }
    
    
    return HAL_ERROR; // 未知的SPI索引
}