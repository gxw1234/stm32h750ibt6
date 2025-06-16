#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "main.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// 协议类型定义
#define PROTOCOL_SPI        0x01    // SPI协议
#define PROTOCOL_IIC        0x02    // IIC协议
#define PROTOCOL_UART       0x03    // UART协议
#define PROTOCOL_GPIO       0x04    // GPIO协议
#define PROTOCOL_POWER      0x05    // 电源协议

// 通用命令ID定义
#define CMD_INIT            0x01    // 初始化命令
#define CMD_WRITE           0x02    // 写数据命令
#define CMD_READ            0x03    // 读数据命令
#define CMD_TRANSFER        0x04    // 读写数据命令
#define CMD_SET_DIR         0x04    // 设置GPIO方向

// 电源命令ID定义
#define POWER_CMD_SET_VOLTAGE       0x01  // 设置电压命令
#define POWER_CMD_START_CURRENT_READING 0x02  // 开始读取电流命令
#define POWER_CMD_STOP_CURRENT_READING  0x03  // 停止读取电流命令
#define POWER_CMD_READ_CURRENT_DATA     0x04  // 读取电流数据命令

// 电源通道定义
#define POWER_CHANNEL_1         0x01  // 电源通道1
#define POWER_CHANNEL_UA        0x02  // 微安电流通道
#define POWER_CHANNEL_MA        0x03  // 毫安电流通道

// 为了支持大数据包和多包传输的定义
#define CMD_END_MARKER 0xA5A5A5A5  // 命令结束标记符

// 通用命令包头结构
typedef struct _GENERIC_CMD_HEADER {
  uint8_t protocol_type;  // 协议类型：SPI/IIC/UART等
  uint8_t cmd_id;         // 命令ID：初始化/读/写等
  uint8_t device_index;   // 设备索引
  uint8_t param_count;    // 参数数量
  uint32_t data_len;      // 数据部分长度
  uint16_t total_packets; // 整个数据包的总大小，包括头部和数据
  uint32_t end_marker;    // 固定结束标记，值为0xA5A5A5A5
} GENERIC_CMD_HEADER, *PGENERIC_CMD_HEADER;

// 简化的参数头结构
typedef struct _PARAM_HEADER {
  uint16_t param_len;     // 参数长度
} PARAM_HEADER, *PPARAM_HEADER;

// SPI索引定义（保留向后兼容性）
#define SPI1_CS0    0
#define SPI1_CS1    1
#define SPI1_CS2    2
#define SPI2_CS0    3
#define SPI2_CS1    4
#define SPI2_CS2    5

// SPI配置结构体（保留向后兼容性）
typedef struct _SPI_CONFIG {
  uint8_t   Mode;            // SPI控制方式:0-硬件控制（全双工模式）,1-硬件控制（半双工模式），2-软件控制（半双工模式）,3-单总线模式，数据线输入输出都为MOSI,4-软件控制（全双工模式）  
  uint8_t   Master;          // 主从选择控制:0-从机，1-主机  
  uint8_t   CPOL;            // 时钟极性控制:0-SCK空闲时为低电平，1-SCK空闲时为高电平  
  uint8_t   CPHA;            // 时钟相位控制:0-第一个SCK时钟采样，1-第二个SCK时钟采样  
  uint8_t   LSBFirst;        // 数据移位方式:0-MSB在前，1-LSB在前  
  uint8_t   SelPolarity;     // 片选信号极性:0-低电平选中，1-高电平选中  
  uint32_t  ClockSpeedHz;    // SPI时钟频率:单位为HZ，硬件模式下最处50000000，最小390625，频率按2的倍数改变  
} SPI_CONFIG, *PSPI_CONFIG;

// 电压配置结构体
typedef struct _VOLTAGE_CONFIG {
  uint8_t   channel;     // 电源通道
  uint16_t  voltage;     // 电压值（单位：mV）
} VOLTAGE_CONFIG, *PVOLTAGE_CONFIG;

/**
 * @brief 添加参数到参数缓冲区
 * 
 * @param buffer 目标缓冲区
 * @param pos 当前位置指针
 * @param data 参数数据
 * @param len 参数长度
 * @return int 添加后的位置
 */
int Add_Parameter(uint8_t* buffer, int pos, void* data, uint16_t len);

/**
 * @brief 从参数缓冲区获取参数
 * 
 * @param buffer 源缓冲区
 * @param pos 当前位置指针
 * @param data 参数数据目标缓冲区
 * @param max_len 最大参数长度
 * @return int 处理后的位置，-1表示错误
 */
int Get_Parameter(uint8_t* buffer, int pos, void* data, uint16_t max_len);

/**
 * @brief 处理接收到的命令
 * 
 * @param Buf 接收到的数据缓冲区
 * @param Len 数据长度
 * @return int8_t 处理结果，0表示成功
 */
int8_t Process_Command(uint8_t* Buf, uint32_t *Len);

#endif /* COMMAND_HANDLER_H */
