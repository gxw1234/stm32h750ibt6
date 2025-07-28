#include "tasks/usb_command_pc_to_st_task.h"
#include "usbd_cdc_if.h"
#include <stdio.h>
#include <string.h>
#include "pc_to_stm_command_handler/command_handler.h"

// 定义帧头和结束标记
#define FRAME_START_MARKER  0x5AA55AA5 // 帧开始标记
#define CMD_END_MARKER      0xA5A5A5A5 // 命令包结束符


// 定义大缓冲区大小  最大就是一张图片的大小 96*248+50
#define BIG_BUFFER_SIZE (96*248+50)  
static uint8_t big_buffer[BIG_BUFFER_SIZE];
static uint32_t accumulated_length = 0;
static uint8_t packet_complete = 0;


QueueHandle_t usbMessageQueueHandle = NULL;

void usb_command_pc_to_st_task(void *argument)
{
    accumulated_length = 0;
    packet_complete = 0;
    memset(big_buffer, 0, BIG_BUFFER_SIZE);
    usbMessageQueueHandle = xQueueCreate(100, sizeof(USB_Data_TypeDef));
    if (usbMessageQueueHandle == NULL) {
        printf("Queue create failed!\r\n");
        vTaskDelete(NULL);
        return;
    }
    
    printf("--------Queue created----\r\n");
    USB_Data_TypeDef usbData;
    uint16_t expected_total_length = 0;  // 期望的总长度
    uint8_t frame_found = 0;             // 是否已找到帧头
    
    while (1) {
        if (xQueueReceive(usbMessageQueueHandle, &usbData, portMAX_DELAY) == pdPASS) {
            uint32_t len = usbData.Length;
            
            // 检查数据长度的合理性（0长度数据无意义）
            if (len == 0) {
                continue;
            }
            
            // 检查缓冲区是否有足够空间
            if (accumulated_length + len <= BIG_BUFFER_SIZE) {
                memcpy(big_buffer + accumulated_length, usbData.Buf, len);
                accumulated_length += len;
                
                // 如果还没找到帧头，尝试寻找帧头（只在前面几个字节中寻找）
                if (!frame_found && accumulated_length >= sizeof(uint32_t)) {
                    uint32_t marker;
                    memcpy(&marker, big_buffer, sizeof(uint32_t));
                    if (marker == FRAME_START_MARKER) {
                        frame_found = 1;
                        // 检查是否有足够数据读取协议头
                        if (accumulated_length >= sizeof(GENERIC_CMD_HEADER)) {
                            GENERIC_CMD_HEADER* header = (GENERIC_CMD_HEADER*)big_buffer;
                            expected_total_length = header->total_packets;
                            
                            // 验证协议头的合理性
                            if (expected_total_length < sizeof(GENERIC_CMD_HEADER) + sizeof(uint32_t) || 
                                expected_total_length > BIG_BUFFER_SIZE) {
                                printf("Invalid packet length: %d, reset buffer\r\n", expected_total_length);
                                accumulated_length = 0;
                                frame_found = 0;
                                expected_total_length = 0;
                                memset(big_buffer, 0, BIG_BUFFER_SIZE);
                                continue;
                            }
                        }
                    } else {
                        // 前4个字节不是帧头，丢弃这些数据
                        printf("No frame header found at start, reset buffer\r\n");
                        accumulated_length = 0;
                        memset(big_buffer, 0, BIG_BUFFER_SIZE);
                        continue;
                    }
                }
                
                // 如果已找到帧头且已知期望长度，检查数据包是否完整
                if (frame_found && expected_total_length > 0 && accumulated_length >= expected_total_length) {
                    // 验证结束标记
                    uint32_t end_marker;
                    memcpy(&end_marker, big_buffer + expected_total_length - sizeof(uint32_t), sizeof(uint32_t));
                    
                    if (end_marker == CMD_END_MARKER) {
                        // 数据包完整且结束标记正确，处理命令
                        uint32_t process_length = expected_total_length;
                        Process_Command(big_buffer, &process_length);
                        
                        // 重置状态，准备接收下一个数据包
                        accumulated_length = 0;
                        frame_found = 0;
                        expected_total_length = 0;
                        memset(big_buffer, 0, BIG_BUFFER_SIZE);
                    } else {
                        // 结束标记错误，重置缓冲区
                        printf("Invalid end marker: 0x%08X, expected: 0x%08X\r\n", end_marker, CMD_END_MARKER);
                        accumulated_length = 0;
                        frame_found = 0;
                        expected_total_length = 0;
                        memset(big_buffer, 0, BIG_BUFFER_SIZE);
                    }
                }
                
            } else {
                printf("Buffer overflow! Reset buffer\r\n");
                accumulated_length = 0;
                frame_found = 0;
                expected_total_length = 0;
                memset(big_buffer, 0, BIG_BUFFER_SIZE);
            }
        }
    }
}
