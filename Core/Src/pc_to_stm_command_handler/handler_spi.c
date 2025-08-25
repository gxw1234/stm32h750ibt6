#include "handler_spi.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "cmsis_os.h"
#include "stdio.h"
#include "usbd_cdc_if.h"
#include "string.h"

static SPI_HandleTypeDef hspi5;
static DMA_HandleTypeDef hdma_spi5_tx;

// DMA传输完成标志  // DMA错误状态标志
static __IO uint8_t spi_transfer_complete = 0;
static __IO uint32_t spi_error_code = 0;
static __IO uint32_t dma_error_code = 0;
static SemaphoreHandle_t spi_complete_semaphore = NULL;
#define SPI_RX_BUFFER_SIZE 512
static uint8_t spi_rx_buffer[SPI_RX_BUFFER_SIZE];

typedef struct {
    GENERIC_CMD_HEADER header;
    uint8_t data[SPI_RX_BUFFER_SIZE];  
} SPI_Data_Packet;

static void WaitForSPITransmissionComplete(void)
{
    if (spi_complete_semaphore != NULL) {
        if (xSemaphoreTake(spi_complete_semaphore, pdMS_TO_TICKS(5000)) != pdTRUE) {
            HAL_SPI_DMAStop(&hspi5);
            spi_transfer_complete = 1;
        }
    }
    
    if(spi_error_code != HAL_SPI_ERROR_NONE) {
        spi_error_code = 0;
    }
    
    spi_transfer_complete = 0;
}

/**Sindex 1:SPI5   对应的引脚是PH6,PH7,PF11,H8
CS          对应 P6  (H8)
MOSI        对应 P3  （F11)
SCK         对应 P4   (H6)
MISO        对应 P5   (H7)

{H6: P4,H7: P5,F11: P3}
*/
static HAL_StatusTypeDef SPI_GPIO_Init(uint8_t spi_index)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    if (spi_index == SPI_INDEX_1) { 
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
        GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI5;
        HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = GPIO_PIN_11;
        HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

        /* CS0引脚配置 */
        GPIO_InitStruct.Pin = GPIO_PIN_8;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
        /* CS0默认高电平 */
        HAL_GPIO_WritePin(GPIOH, GPIO_PIN_8, GPIO_PIN_SET);

        return HAL_OK;
    }
    return HAL_ERROR; 
}

HAL_StatusTypeDef Handler_SPI_Init(uint8_t spi_index, PSPI_CONFIG pConfig)
{
    HAL_StatusTypeDef status;
    if (spi_index == SPI_INDEX_1) { // SPI5
        status = SPI_GPIO_Init(spi_index);
        if (status != HAL_OK) {
            return status;
        }
        __HAL_RCC_DMA1_CLK_ENABLE();
        hdma_spi5_tx.Instance = DMA1_Stream4;
        hdma_spi5_tx.Init.Request = DMA_REQUEST_SPI5_TX;
        hdma_spi5_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_spi5_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_spi5_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_spi5_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi5_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_spi5_tx.Init.Mode = DMA_NORMAL;
        hdma_spi5_tx.Init.Priority = DMA_PRIORITY_HIGH;
        hdma_spi5_tx.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
        if (HAL_DMA_Init(&hdma_spi5_tx) != HAL_OK) {
            return HAL_ERROR;
        }
        // 链接DMA到SPI
        __HAL_LINKDMA(&hspi5, hdmatx, hdma_spi5_tx);
        HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
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
        prescaler = SPI_BAUDRATEPRESCALER_4;
        hspi5.Init.BaudRatePrescaler = prescaler;
        hspi5.Init.FirstBit = pConfig->LSBFirst ? SPI_FIRSTBIT_LSB : SPI_FIRSTBIT_MSB;
        hspi5.Init.TIMode = SPI_TIMODE_DISABLE;
        hspi5.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
        hspi5.Init.CRCPolynomial = 7;
        status = HAL_SPI_Init(&hspi5);
        if (status == HAL_OK) {
            // 信号量
            if (spi_complete_semaphore == NULL) {
                spi_complete_semaphore = xSemaphoreCreateBinary();
            }
            HAL_NVIC_SetPriority(SPI5_IRQn, 5, 0);
            HAL_NVIC_EnableIRQ(SPI5_IRQn);
            if (!pConfig->Master) {
                HAL_SPI_Receive_IT(&hspi5, spi_rx_buffer, SPI_RX_BUFFER_SIZE);
            }
        }
        return status;
    }
    return HAL_ERROR; 
}


HAL_StatusTypeDef Handler_SPI_Transmit(uint8_t spi_index, uint8_t *pTxData, uint8_t *pRxData, uint16_t DataSize, uint32_t Timeout)
{
    if (spi_index != SPI_INDEX_1) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef status;
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_8, GPIO_PIN_RESET);
    uint32_t addr = (uint32_t)pTxData;
    if (addr >= 0x20000000 && addr < 0x20020000) {
        HAL_GPIO_WritePin(GPIOH, GPIO_PIN_8, GPIO_PIN_SET);
        return HAL_ERROR;
    }
    if (pRxData != NULL) {
        status = HAL_SPI_TransmitReceive(&hspi5, pTxData, pRxData, DataSize, Timeout);
    } else if (DataSize >= 96*240+10) {
        printf("[SPI] DataSize >= 96*240+10\r\n");
        status = HAL_SPI_Transmit(&hspi5, pTxData, DataSize, Timeout);
    } else {        

        

        // printf("1111[SPI] DataSize < 96*240+10\r\n");
        spi_transfer_complete = 0;
        spi_error_code = 0;
        dma_error_code = 0;
        // 缓存清理（仅在DCache启用时）
        if (SCB->CCR & SCB_CCR_DC_Msk) {
            SCB_CleanDCache_by_Addr((uint32_t*)pTxData, DataSize);
        }
        status = HAL_SPI_Transmit_DMA(&hspi5, (uint8_t*)pTxData, DataSize);
        if (status == HAL_OK) {
            WaitForSPITransmissionComplete();
        }
    }
    
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_8, GPIO_PIN_SET);
    return status;
}

void SPI5_IRQHandler(void)
{
    HAL_SPI_IRQHandler(&hspi5);
}

// DMA中断处理函数
//因为没有使用DMA的错误处理，所以这个函数没有用
void DMA1_Stream4_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_spi5_tx);
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &hspi5) {
        spi_transfer_complete = 1;
        // printf("SPI transmission completed successfully\n");
        // 从中断中释放信号量
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (spi_complete_semaphore != NULL) {
            xSemaphoreGiveFromISR(spi_complete_semaphore, &xHigherPriorityTaskWoken);
        }
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &hspi5) {
        spi_error_code = hspi->ErrorCode;
        spi_transfer_complete = 1;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (spi_complete_semaphore != NULL) {
            xSemaphoreGiveFromISR(spi_complete_semaphore, &xHigherPriorityTaskWoken);
        }
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void HAL_DMA_ErrorCallback(DMA_HandleTypeDef *hdma)
{
    if (hdma == &hdma_spi5_tx) {
        dma_error_code = hdma->ErrorCode;
        spi_transfer_complete = 1;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (spi_complete_semaphore != NULL) {
            // xSemaphoreGiveFromISR(spi_complete_semaphore, &xHigherPriorityTaskWoken);
        }
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &hspi5) {
        SPI_Data_Packet packet;
        packet.header.protocol_type = PROTOCOL_SPI;    // SPI协议
        packet.header.cmd_id = CMD_READ;              // 读数据命令
        packet.header.device_index = SPI_INDEX_1;     // SPI设备索引
        packet.header.param_count = 0;                // 无参数
        packet.header.data_len = SPI_RX_BUFFER_SIZE;  // 数据长度
        packet.header.total_packets = sizeof(SPI_Data_Packet); // 总包大小
        memcpy(packet.data, spi_rx_buffer, SPI_RX_BUFFER_SIZE);
        USB_Sender((uint8_t*)&packet, sizeof(packet));
        HAL_SPI_Receive_IT(&hspi5, spi_rx_buffer, SPI_RX_BUFFER_SIZE);
    }
}