#ifndef IMAGE_QUEUE_TASK_H
#define IMAGE_QUEUE_TASK_H

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdint.h>

// 图像缓冲区配置
#define IMAGE_WIDTH 96
#define IMAGE_HEIGHT 240
#define IMAGE_SIZE (IMAGE_WIDTH * IMAGE_HEIGHT)
#define IMAGE_BUFFER_SIZE 23040
#define MAX_QUEUE_SIZE 10 // 最大队列深度为5张图像

// 静态缓冲区结构
typedef struct {
    uint8_t data[IMAGE_BUFFER_SIZE];  // 图像数据
    uint16_t size;                    // 实际数据大小
    uint8_t valid;                    // 数据有效标志
    uint8_t inUse;                    // 使用中标志
} StaticImageFrame_t;

// 队列项（只存索引，节省队列内存）
typedef struct {
    uint8_t bufferIndex;  // 缓冲区索引
    uint16_t size;        // 数据大小
} QueueItem_t;

// 队列状态枚举
typedef enum {
    QUEUE_STOPPED = 0,
    QUEUE_RUNNING = 1,
    QUEUE_PAUSED = 2,
    QUEUE_STOPPING = 3  // 正在停止，清空队列中
} QueueState_t;

// 图像队列控制结构
typedef struct {
    QueueHandle_t imageQueue;       // FreeRTOS队列句柄
    SemaphoreHandle_t queueMutex;   // 队列保护互斥锁
    QueueState_t state;             // 队列状态
    uint32_t totalReceived;         // 总接收图像数
    uint32_t totalProcessed;        // 总处理图像数
    uint32_t droppedFrames;         // 丢弃的帧数
} ImageQueueControl_t;




HAL_StatusTypeDef ImageQueue_Init(void);  // 改为返回状态
void ImageQueue_Task(void *argument);
HAL_StatusTypeDef ImageQueue_AddFrame(uint8_t* imageData, uint16_t size);
uint8_t ImageQueue_GetStatus(void);
HAL_StatusTypeDef ImageQueue_Start(void);
HAL_StatusTypeDef ImageQueue_Stop(void);
uint32_t ImageQueue_GetCount(void);
HAL_StatusTypeDef ImageQueue_DeInit(void);  


#endif /* IMAGE_QUEUE_TASK_H */ 