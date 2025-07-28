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
    
    for (;;) {
        // 从队列中接收缓冲区索引
        if (xQueueReceive(g_imageQueueControl.imageQueue, &queueItem, portMAX_DELAY) == pdTRUE) {
            
            // 获取对应的静态缓冲区
            StaticImageFrame_t* frame = &g_imageBuffers[queueItem.bufferIndex];
            
            // 检查队列状态和数据有效性
            if (g_imageQueueControl.state == QUEUE_RUNNING || g_imageQueueControl.state == QUEUE_STOPPING) {
                if (frame->valid && frame->inUse) {
                    // SPI传输
                    HAL_StatusTypeDef status = Handler_SPI_Transmit(SPI_INDEX_1, frame->data, NULL, queueItem.size, 1000);
                    if (status == HAL_OK) {
                        
                        g_imageQueueControl.totalProcessed++;
                    } else {
                        printf("SPI FAIL: %d\r\n", status);
                    }
                } else {
                    printf("Frame invalid\r\n");
                }
            } else {
                printf("Queue stopped, drop frame\r\n");
            }
            
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

HAL_StatusTypeDef ImageQueue_AddFrame(uint8_t* imageData, uint16_t size)
{
    if (imageData == NULL || size == 0 || size > IMAGE_BUFFER_SIZE) {
        printf("ImageQueue_AddFrame: Invalid params\r\n");
        return HAL_ERROR;
    }

    if (g_imageQueueControl.imageQueue == NULL || g_imageQueueControl.state != QUEUE_RUNNING) {
        printf("ImageQueue_AddFrame: Queue not running, state=%d\r\n", g_imageQueueControl.state);
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
        // 设置为运行状态
        g_imageQueueControl.state = QUEUE_RUNNING;
        xSemaphoreGive(g_imageQueueControl.queueMutex);
        return HAL_OK;
    }
    return HAL_ERROR;
}

/**
 * @brief 停止队列处理
 * @return HAL_StatusTypeDef 返回状态
 */
HAL_StatusTypeDef ImageQueue_Stop(void)
{
    if (xSemaphoreTake(g_imageQueueControl.queueMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // 设置为正在停止状态，不再接收新图像，但继续处理队列中的图像
        g_imageQueueControl.state = QUEUE_STOPPING;
       
        
        xSemaphoreGive(g_imageQueueControl.queueMutex);
        
        // 等待队列为空
        uint32_t timeout = 0;
        while (uxQueueMessagesWaiting(g_imageQueueControl.imageQueue) > 0 && timeout < 1000) {
            vTaskDelay(pdMS_TO_TICKS(5)); // 等待5ms
            timeout += 5;
        }
        
        if (xSemaphoreTake(g_imageQueueControl.queueMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // 真正停止
            g_imageQueueControl.state = QUEUE_STOPPED;
            xSemaphoreGive(g_imageQueueControl.queueMutex);
     
            return HAL_OK;
        }
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
    // 先停止队列处理
    if (g_imageQueueControl.state != QUEUE_STOPPED) {
        ImageQueue_Stop();
    }
    
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

    // 初始化状态
    g_imageQueueControl.state = QUEUE_STOPPED;
    g_imageQueueControl.totalReceived = 0;
    g_imageQueueControl.totalProcessed = 0;
    g_imageQueueControl.droppedFrames = 0;

    // 创建任务
    BaseType_t result = xTaskCreate(
        ImageQueue_Task, "ImageQueueTask", 
        1024, NULL, 9, &g_imageQueueTaskHandle
    );

    if (result != pdPASS) {
        printf("Failed to create image queue task\r\n");
        return HAL_ERROR;
    }

    printf("Image queue created successfully\r\n");
    return HAL_OK;
} 