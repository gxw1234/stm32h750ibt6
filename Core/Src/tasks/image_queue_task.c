#include "tasks/image_queue_task.h"
#include "pc_to_stm_command_handler/handler_spi.h"
#include <string.h>
#include <stdio.h>
#include "cmsis_os.h"

// 静态缓冲区池
__attribute__((section(".ram_buffers"))) static StaticImageFrame_t g_imageBuffers[MAX_QUEUE_SIZE];

// 队列控制结构
static ImageQueueControl_t g_imageQueueControl = {0};

// FreeRTOS任务句柄
static TaskHandle_t g_imageQueueTaskHandle = NULL;

/**
 * @brief 图像队列处理任务（简化版本）
 * @param argument 任务参数（未使用）
 */
void ImageQueue_Task(void *argument)
{

    
    (void)argument; 
    QueueItem_t queueItem;  // 只有3字节，安全
    static TickType_t lastFrameStartTime = 0;  // 记录上一帧开始时间
    const TickType_t frameInterval = pdMS_TO_TICKS(4);  // 4ms帧间隔
    
    for (;;) {
        // 从队列中接收缓冲区索引
        if (xQueueReceive(g_imageQueueControl.imageQueue, &queueItem, portMAX_DELAY) == pdTRUE) {
            // 获取对应的静态缓冲区
            StaticImageFrame_t* frame = &g_imageBuffers[queueItem.bufferIndex];
            // 直接传输数据，不检查复杂状态
            HAL_StatusTypeDef status = Handler_SPI_Transmit(SPI_INDEX_1, frame->data, NULL, queueItem.size, 1000);
            // 释放缓冲区
            frame->inUse = 0;
            frame->valid = 0;
        }
    }
}

/**
 * @brief 查找空闲缓冲区
 */
static int8_t FindFreeBuffer(void)
{
    for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
        if (!g_imageBuffers[i].inUse) {
            return i;
        }
    }
    return -1;
}

// ============ 零拷贝相关接口实现 ============

/**
 * @brief 分配空闲缓冲区（零拷贝接口）
 * @return int8_t 缓冲区索引，-1表示无可用缓冲区
 */
int8_t ImageQueue_AllocateBuffer(void)
{
    if (g_imageQueueControl.imageQueue == NULL) {
        return -1;
    }

    int8_t bufferIndex = FindFreeBuffer();
    if (bufferIndex >= 0) {
        // 标记缓冲区为使用中，但尚未有效
        g_imageBuffers[bufferIndex].inUse = 1;
        g_imageBuffers[bufferIndex].valid = 0;
        g_imageBuffers[bufferIndex].size = 0;
    }
    return bufferIndex;
}

/**
 * @brief 获取缓冲区数据指针（零拷贝接口）
 * @param bufferIndex 缓冲区索引
 * @return uint8_t* 缓冲区数据指针，NULL表示无效索引
 */
uint8_t* ImageQueue_GetBufferPtr(int8_t bufferIndex)
{
    if (bufferIndex < 0 || bufferIndex >= MAX_QUEUE_SIZE) {
        return NULL;
    }
    
    if (!g_imageBuffers[bufferIndex].inUse) {
        return NULL;
    }
    
    return g_imageBuffers[bufferIndex].data;
}

/**
 * @brief 提交缓冲区到队列（零拷贝接口）
 * @param bufferIndex 缓冲区索引
 * @param size 实际数据大小
 * @return HAL_StatusTypeDef 返回状态
 */
HAL_StatusTypeDef ImageQueue_CommitBuffer(int8_t bufferIndex, uint16_t size)
{
    if (bufferIndex < 0 || bufferIndex >= MAX_QUEUE_SIZE) {
        return HAL_ERROR;
    }
    
    if (!g_imageBuffers[bufferIndex].inUse || size == 0 || size > IMAGE_BUFFER_SIZE) {
        return HAL_ERROR;
    }
    
    if (g_imageQueueControl.imageQueue == NULL) {
        return HAL_BUSY;
    }

    // 设置缓冲区为有效
    g_imageBuffers[bufferIndex].size = size;
    g_imageBuffers[bufferIndex].valid = 1;
    
    // 创建队列项并发送到队列
    QueueItem_t queueItem;
    queueItem.bufferIndex = bufferIndex;
    queueItem.size = size;
    
    if (xQueueSend(g_imageQueueControl.imageQueue, &queueItem, 0) == pdTRUE) {
        g_imageQueueControl.totalReceived++;
        return HAL_OK;
    } else {
        // 队列满，释放缓冲区
        g_imageBuffers[bufferIndex].inUse = 0;
        g_imageBuffers[bufferIndex].valid = 0;
        g_imageQueueControl.droppedFrames++;
        return HAL_BUSY;
    }
}

/**
 * @brief 释放缓冲区（零拷贝接口）
 * @param bufferIndex 缓冲区索引
 * @return HAL_StatusTypeDef 返回状态
 */
HAL_StatusTypeDef ImageQueue_ReleaseBuffer(int8_t bufferIndex)
{
    if (bufferIndex < 0 || bufferIndex >= MAX_QUEUE_SIZE) {
        return HAL_ERROR;
    }
    
    // 释放缓冲区
    g_imageBuffers[bufferIndex].inUse = 0;
    g_imageBuffers[bufferIndex].valid = 0;
    g_imageBuffers[bufferIndex].size = 0;
    
    return HAL_OK;
}

HAL_StatusTypeDef ImageQueue_AddFrame(uint8_t* imageData, uint16_t size)
{
    if (imageData == NULL || size == 0 || size > IMAGE_BUFFER_SIZE) {
        printf("ImageQueue_AddFrame: Invalid params\r\n");
        return HAL_ERROR;
    }

    if (g_imageQueueControl.imageQueue == NULL) {
        printf("ImageQueue_AddFrame: Queue not available\r\n");
        return HAL_BUSY;
    }

    // 查找空闲缓冲区
    int8_t bufferIndex = FindFreeBuffer();
    if (bufferIndex < 0) {
        printf("ImageQueue_AddFrame: No free buffer, dropped=%lu\r\n", g_imageQueueControl.droppedFrames);
        g_imageQueueControl.droppedFrames++;
        return HAL_BUSY;
    }

    // 填充缓冲区
    StaticImageFrame_t* frame = &g_imageBuffers[bufferIndex];
    frame->size = size;
    frame->valid = 1;
    frame->inUse = 1;
    memcpy(frame->data, imageData, size);

    g_imageQueueControl.totalReceived++;
    
    // 发送索引到队列
    QueueItem_t queueItem = {.bufferIndex = bufferIndex, .size = size};
    BaseType_t result = xQueueSend(g_imageQueueControl.imageQueue, &queueItem, 0);
    
    if (result != pdTRUE) {
        printf("ImageQueue_AddFrame: Queue send failed, queue_count=%d\r\n", 
               (int)uxQueueMessagesWaiting(g_imageQueueControl.imageQueue));
        frame->inUse = 0;
        frame->valid = 0;
        g_imageQueueControl.droppedFrames++;
        return HAL_BUSY;
    }
    

    return HAL_OK;
}

/**
 * @brief 获取队列状态（返回队列中的帧数量）
 * @return uint8_t 队列中的帧数量
 */
uint8_t ImageQueue_GetStatus(void)
{
    if (g_imageQueueControl.imageQueue == NULL) {
        return 0;
    }
    return (uint8_t)uxQueueMessagesWaiting(g_imageQueueControl.imageQueue);
}

/**
 * @brief 启动队列处理
 * @return HAL_StatusTypeDef 返回状态
 */
HAL_StatusTypeDef ImageQueue_Start(void)
{
    if (xSemaphoreTake(g_imageQueueControl.queueMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        // 启动前先重置统计信息
        g_imageQueueControl.totalReceived = 0;
        g_imageQueueControl.totalProcessed = 0;
        g_imageQueueControl.droppedFrames = 0;
        
        // 清空队列和静态缓冲区
        xQueueReset(g_imageQueueControl.imageQueue);
        for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
            g_imageBuffers[i].inUse = 0;
            g_imageBuffers[i].valid = 0;
            g_imageBuffers[i].size = 0;
        }
        // 状态管理已简化，无需设置运行状态
        xSemaphoreGive(g_imageQueueControl.queueMutex);
        return HAL_OK;
    }
    return HAL_ERROR;
}



/**
 * @brief 获取队列中的帧数量
 * @return uint32_t 队列中的帧数量
 */
uint32_t ImageQueue_GetCount(void)
{
    if (g_imageQueueControl.imageQueue == NULL) {
        return 0;
    }
    
    return uxQueueMessagesWaiting(g_imageQueueControl.imageQueue);
}

/**
 * @brief 清理图像队列系统资源
 * @return HAL_StatusTypeDef 返回状态
 * @note 用于系统关闭时的资源清理
 */
HAL_StatusTypeDef ImageQueue_DeInit(void)
{
    // 状态管理已简化，直接清理资源
    
    // 删除任务
    if (g_imageQueueTaskHandle != NULL) {
        vTaskDelete(g_imageQueueTaskHandle);
        g_imageQueueTaskHandle = NULL;
        printf("Image queue task deleted\r\n");
    }
    
    // 清理队列
    if (g_imageQueueControl.imageQueue != NULL) {
        vQueueDelete(g_imageQueueControl.imageQueue);
        g_imageQueueControl.imageQueue = NULL;
        printf("Image queue deleted\r\n");
    }
    
    // 清理互斥锁
    if (g_imageQueueControl.queueMutex != NULL) {
        vSemaphoreDelete(g_imageQueueControl.queueMutex);
        g_imageQueueControl.queueMutex = NULL;
        printf("Image queue mutex deleted\r\n");
    }
    
    // 重置控制结构
    memset(&g_imageQueueControl, 0, sizeof(ImageQueueControl_t));
    
    printf("Image queue system deinitialized\r\n");
    return HAL_OK;
}

/**
 * @brief 初始化图像队列系统
 * @return HAL_StatusTypeDef 返回状态
 */
HAL_StatusTypeDef ImageQueue_Init(void)
{
    // 初始化静态缓冲区
    for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
        g_imageBuffers[i].inUse = 0;
        g_imageBuffers[i].valid = 0;
        g_imageBuffers[i].size = 0;
    }

    // 创建小队列（每项只有3字节）
    g_imageQueueControl.imageQueue = xQueueCreate(MAX_QUEUE_SIZE, sizeof(QueueItem_t));
    if (g_imageQueueControl.imageQueue == NULL) {
        printf("Failed to create image queue\r\n");
        return HAL_ERROR;
    }

    printf("Image queue created successfully\r\n");

    // 创建互斥锁
    g_imageQueueControl.queueMutex = xSemaphoreCreateMutex();
    if (g_imageQueueControl.queueMutex == NULL) {
        printf("Failed to create queue mutex\r\n");
        return HAL_ERROR;
    }

    // 初始化统计信息
    g_imageQueueControl.totalReceived = 0;
    g_imageQueueControl.totalProcessed = 0;
    g_imageQueueControl.droppedFrames = 0;

    // 创建任务
    BaseType_t result = xTaskCreate(
        ImageQueue_Task, "ImageQueueTask", 
        1024, NULL, 5, &g_imageQueueTaskHandle
    );
    if (result != pdPASS) {
        printf("Failed to create image queue task\r\n");
        return HAL_ERROR;
    }
    printf("Image queue created successfully\r\n");
    return HAL_OK;
} 