#include "command_handler.h"
#include "main.h"
#include "../tasks/MP8865_task.h"
#include "tasks/ads1220_task.h"  
#include "pc_to_stm_command_handler/handler_spi.h" 
#include "pc_to_stm_command_handler/handler_iic.h" 
#include "handler_gpio.h" 


extern UART_HandleTypeDef huart1;

/**
 * @brief 添加参数到参数缓冲区
 * 
 * @param buffer 目标缓冲区
 * @param pos 当前位置指针
 * @param data 参数数据
 * @param len 参数长度
 * @return int 添加后的位置
 */
int Add_Parameter(uint8_t* buffer, int pos, void* data, uint16_t len) {

    PARAM_HEADER header;
    header.param_len = len;
    
    // 复制参数头
    memcpy(buffer + pos, &header, sizeof(PARAM_HEADER));
    pos += sizeof(PARAM_HEADER);
    
    // 复制参数数据
    memcpy(buffer + pos, data, len);
    pos += len;
    
    return pos;
}

/**
 * @brief 从参数缓冲区获取参数
 * 
 * @param buffer 源缓冲区
 * @param pos 当前位置指针
 * @param data 参数数据目标缓冲区
 * @param max_len 最大参数长度
 * @return int 处理后的位置，-1表示错误
 */
int Get_Parameter(uint8_t* buffer, int pos, void* data, uint16_t max_len) {
    // 获取参数头
    PARAM_HEADER header;
    memcpy(&header, buffer + pos, sizeof(PARAM_HEADER));
    pos += sizeof(PARAM_HEADER);
    
    // 检查缓冲区大小
    if (header.param_len > max_len) {
        // 如果参数长度超过缓冲区大小，返回错误
        return -1;
    }
    
    // 复制参数数据
    memcpy(data, buffer + pos, header.param_len);
    pos += header.param_len;
    
    return pos;
}

/**
 * @brief 处理SPI初始化命令
 * 
 * @param spi_index SPI索引
 * @param pConfig SPI配置结构体指针
 */
static void Process_SPI_Init(uint8_t spi_index, PSPI_CONFIG pConfig) {
   

    printf("----------------SPI_CMD_INIT----------------\r\n");
    char buffer[128];

    sprintf(buffer, "STM32_SPI Init: Index=%d, Mode=%d, Master=%d, CPOL=%d, CPHA=%d, LSB=%d, SelPol=%d, Clock=%lu\r\n", 
            spi_index, pConfig->Mode, pConfig->Master, pConfig->CPOL, pConfig->CPHA, 
            pConfig->LSBFirst, pConfig->SelPolarity, pConfig->ClockSpeedHz);
    
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
    
    // 调用handler_spi中的初始化函数
    HAL_StatusTypeDef status = Handler_SPI_Init(spi_index, pConfig);
    
    if (status == HAL_OK) {
        sprintf(buffer, "SPI init SUCCESSFUL: %d\r\n", spi_index);
    } else {
        sprintf(buffer, "SPI init fail: %d, eer: %d\r\n", spi_index, status);
    }
    
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
}

/**
 * @brief 处理电压设置命令
 * 
 * @param channel 电源通道
 * @param voltage_mv 电压值(mV)
 */
static void Process_Power_SetVoltage(uint8_t channel, uint16_t voltage_mv) {

    char buffer[64];
    uint8_t voltage_reg_value = (uint8_t)voltage_mv;
    if (voltage_reg_value < 12) voltage_reg_value = 12;
    if (voltage_reg_value > 255) voltage_reg_value = 255;
    HAL_StatusTypeDef status = MP8865_SetVoltage(voltage_reg_value);
    if (status == HAL_OK) {
        sprintf(buffer, "[MP8865] CH:%d, Set voltage success, reg value: 0x%02X\r\n", channel, voltage_reg_value);
    } else {
        sprintf(buffer, "[MP8865] CH:%d, Set voltage failed, error code: %d\r\n", channel, status);
    }
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
}


/**
 * @brief 处理开始读取电流命令
 * 
 * @param device_index 设备索引
 */
static void Process_Power_StartCurrentReading(uint8_t device_index) {
    printf("Receive Command:POWER_CMD_START_CURRENT_READING,device_index:%d\r\n", device_index);
    Enable_Current_Data_Sending();
}

/**
 * @brief 处理停止读取电流命令
 * 
 * @param device_index 设备索引
 */
static void Process_Power_StopCurrentReading(uint8_t device_index) {
    printf("Receive Command:POWER_CMD_STOP_CURRENT_READING,device_index:%d\r\n", device_index);
    Disable_Current_Data_Sending();
}

/**
 * @brief 处理IIC初始化命令
 * 
 * @param iic_index IIC索引
 * @param pConfig IIC配置结构体指针
 */
static void Process_IIC_Init(uint8_t iic_index, PIIC_CONFIG pConfig) {
    char buffer[128];

    sprintf(buffer, "STM32_IIC Init: Index=%d, ClockSpeed=%lu, OwnAddr=0x%04X, Master=%d, AddrBits=%d, EnablePu=%d\r\n", 
            iic_index, pConfig->ClockSpeedHz, pConfig->OwnAddr, pConfig->Master, pConfig->AddrBits, pConfig->EnablePu);
    
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
    
    // 调用handler_iic中的初始化函数
    HAL_StatusTypeDef status = Handler_IIC_Init(iic_index, pConfig);
    
    if (status == HAL_OK) {
        sprintf(buffer, "IIC init SUCCESSFUL: %d\r\n", iic_index);
    } else {
        sprintf(buffer, "IIC init fail: %d, error: %d\r\n", iic_index, status);
    }
    
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
}

/**
 * @brief 处理IIC从机写数据命令
 * 
 * @param iic_index IIC索引
 * @param pData 数据缓冲区
 * @param data_len 数据长度
 * @param timeout 超时时间
 */
static void Process_IIC_SlaveWrite(uint8_t iic_index, uint8_t* pData, uint16_t data_len, uint32_t timeout) {
    char buffer[128];

    sprintf(buffer, "STM32_IIC SlaveWrite: Index=%d, DataLen=%d, Timeout=%lu\r\n", 
            iic_index, data_len, timeout);
    HAL_StatusTypeDef status = Handler_IIC_SlaveWriteBytes(iic_index, pData, data_len, timeout);
    
}

/**
 * @brief 处理读取电流数据命令
 * 
 * @param channel 电流通道 (POWER_CHANNEL_UA 或 POWER_CHANNEL_MA)
 * @param response_buf 响应缓冲区
 * @param max_len 最大响应长度
 * @return int 响应数据长度
 */
static int Process_Power_ReadCurrentData(uint8_t channel, uint8_t* response_buf, int max_len) {
    char buffer[80];
    sprintf(buffer, "[Current] Received command: Read current data, channel: %d\r\n", channel);
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
    
    // 使用测试值
    float current_value = 123.456f;
    int response_len = 0;
    
    // 构造响应数据的基本框架
    if (max_len >= sizeof(GENERIC_CMD_HEADER) + sizeof(float)) {
        GENERIC_CMD_HEADER* response_header = (GENERIC_CMD_HEADER*)response_buf;
        response_header->protocol_type = PROTOCOL_POWER;
        response_header->cmd_id = POWER_CMD_READ_CURRENT_DATA;
        response_header->device_index = channel;
        response_header->param_count = 0;
        response_header->data_len = sizeof(float);
        
        // 使用测试值应答
        memcpy(response_buf + sizeof(GENERIC_CMD_HEADER), &current_value, sizeof(float));
        response_len = sizeof(GENERIC_CMD_HEADER) + sizeof(float);
        
        // 打印返回值
        sprintf(buffer, "[Current] Returning test value: %.3f\r\n", current_value);
        HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
    }
    
    return response_len;
}

/**
 * @brief 处理SPI写数据命令
 * 
 * @param spi_index SPI索引
 * @param data 数据缓冲区
 * @param data_len 数据长度
 */
static void Process_SPI_Write(uint8_t spi_index, uint8_t* data, uint16_t data_len) {


    HAL_StatusTypeDef status = Handler_SPI_Transmit(spi_index, data, NULL, data_len, 1000);
    char status_buffer[64];
    if (status == HAL_OK) {
        sprintf(status_buffer, "SPI Write Success\r\n");
        HAL_UART_Transmit(&huart1, (uint8_t*)status_buffer, strlen(status_buffer), 100);
    } else {
        sprintf(status_buffer, "SPI Write Failed, error code: %d\r\n", status);
        HAL_UART_Transmit(&huart1, (uint8_t*)status_buffer, strlen(status_buffer), 100);
    }
}

/**
 * @brief 处理接收到的命令
 * 
 * @param Buf 接收到的数据缓冲区
 * @param Len 数据长度
 * @return int8_t 处理结果，0表示成功
 */
/**
 * @brief 处理GPIO设置输出命令
 * 
 * @param gpio_index GPIO索引
 * @param output_mask 输出引脚掩码
 */
static void Process_GPIO_SetOutput(uint8_t gpio_index, uint8_t output_mask) {
    char buffer[128];

    // 调用handler_gpio中的设置函数
    HAL_StatusTypeDef status = Handler_GPIO_SetOutput(gpio_index, output_mask);
    
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
    
}

/**
 * @brief 处理GPIO写数据命令
 * 
 * @param gpio_index GPIO索引
 * @param write_value 写入的值
 */
static void Process_GPIO_Write(uint8_t gpio_index, uint8_t write_value) {
    char buffer[128];
    
   
    
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
    
    // 调用handler_gpio中的写函数
    HAL_StatusTypeDef status = Handler_GPIO_Write(gpio_index, write_value);
    

    
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
}

/**
 * @brief 处理接收到的命令
 * 
 * @param Buf 接收到的数据缓冲区
 * @param Len 数据长度
 * @return int8_t 处理结果，0表示成功
 */
int8_t Process_Command(uint8_t* Buf, uint32_t *Len) {
   
    if (*Len >= sizeof(GENERIC_CMD_HEADER)) {
        GENERIC_CMD_HEADER* header = (GENERIC_CMD_HEADER*)Buf;

        switch (header->protocol_type) {
            case PROTOCOL_SPI: {

                switch (header->cmd_id) {
                    case CMD_INIT: {
                       
                        char buffer[256];
                        sprintf(buffer, "SPI Init Command: Device=%d, ParamCount=%d\r\n", 
                                header->device_index, header->param_count);
                        HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
                        if (header->param_count > 0) {
                            int pos = sizeof(GENERIC_CMD_HEADER);
                            SPI_CONFIG spi_config;
                            pos = Get_Parameter(Buf, pos, &spi_config, sizeof(SPI_CONFIG));
                            if (pos > 0) {
                                Process_SPI_Init(header->device_index, &spi_config);
                            } else {
                                char* error_msg = "Error: Invalid parameter format for SPI_INIT\r\n";
                                HAL_UART_Transmit(&huart1, (uint8_t*)error_msg, strlen(error_msg), 100);
                            }
                        } else {
                            char* error_msg = "Error: No parameters for SPI_INIT command\r\n";
                            HAL_UART_Transmit(&huart1, (uint8_t*)error_msg, strlen(error_msg), 100);
                        }
                        break;
                    }
                    case CMD_WRITE: {
                        // SPI写命令
                        if (header->data_len > 0) {
                            // 数据部分开始位置：命令头 + 参数区
                            int data_pos = sizeof(GENERIC_CMD_HEADER);
                            for (int i = 0; i < header->param_count; i++) {
                                PARAM_HEADER* param_header = (PARAM_HEADER*)(Buf + data_pos);
                                data_pos += sizeof(PARAM_HEADER) + param_header->param_len;
                            }
                            uint8_t* write_data = Buf + data_pos;
                            Process_SPI_Write(header->device_index, write_data, header->data_len);
                        } else {
                            char* error_msg = "Error: No data for SPI_WRITE command\r\n";
                            HAL_UART_Transmit(&huart1, (uint8_t*)error_msg, strlen(error_msg), 100);
                        }
                        break;
                    }
                    case CMD_READ:
                    case CMD_TRANSFER:
                        {
                            char* msg = "Received other SPI command (not implemented yet)\r\n";
                            HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
                        }
                        break;
                    default: {
                        char buffer[64];
                        sprintf(buffer, "Unknown SPI command ID: 0x%02X\r\n", header->cmd_id);
                        HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
                        break;
                    }
                }
                break;
            }
            case PROTOCOL_IIC: {
                switch (header->cmd_id) {
                    case CMD_INIT: {
                        IIC_CONFIG iic_config;
                        int param_pos = sizeof(GENERIC_CMD_HEADER);
                        param_pos = Get_Parameter(Buf, param_pos, &iic_config, sizeof(IIC_CONFIG));
                        if (param_pos > 0) {
                            Process_IIC_Init(header->device_index, &iic_config);
                        } else {
                            char* error_msg = "Error: Failed to parse IIC_CONFIG\r\n";
                            HAL_UART_Transmit(&huart1, (uint8_t*)error_msg, strlen(error_msg), 100);
                        }
                        break;
                    }
                    case CMD_WRITE: {
                        // IIC从机写数据命令
                        char buffer[64];
                        sprintf(buffer, "Received IIC WRITE command: Index=%d, DataLen=%d\r\n", 
                                header->device_index, header->data_len);
                        HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
                        
                        // 检查是否有数据
                        if (header->data_len > 0) {
                            // 获取数据部分起始位置
                            int data_pos = sizeof(GENERIC_CMD_HEADER);
                            // 提取超时参数(如果有)
                            uint32_t timeout = 3000;  // 默认超时时间为3000ms
                            
                            // 读取数据并处理
                            Process_IIC_SlaveWrite(header->device_index, Buf + data_pos, header->data_len, timeout);
                        } else {
                            char* error_msg = "Error: No data for IIC WRITE command\r\n";
                            HAL_UART_Transmit(&huart1, (uint8_t*)error_msg, strlen(error_msg), 100);
                        }
                        break;
                    }
                    default: {
                        char buffer[64];
                        sprintf(buffer, "Unknown IIC command ID: 0x%02X\r\n", header->cmd_id);
                        HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
                        break;
                    }
                }
                break;
            }
            
            case PROTOCOL_UART: {
                // 处理UART协议命令
                char* msg = "Received UART command (not implemented yet)\r\n";
                HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
                break;
            }
            
            case PROTOCOL_GPIO: {
                // 处理GPIO协议命令
                switch (header->cmd_id) {
                    case CMD_SET_DIR: {
                        // GPIO设置方向
                        if (header->data_len > 0) {
                            uint8_t output_mask = Buf[sizeof(GENERIC_CMD_HEADER)];
                            Process_GPIO_SetOutput(header->device_index, output_mask);
                        } else {
                            char* error_msg = "Error: No mask for GPIO set direction command\r\n";
                            HAL_UART_Transmit(&huart1, (uint8_t*)error_msg, strlen(error_msg), 100);
                        }
                        break;
                    }
                    case CMD_INIT: {
                        // GPIO初始化/设置方向
                        int param_pos = sizeof(GENERIC_CMD_HEADER);
                        if (header->param_count > 0) {
                            // 获取GPIO方向配置参数
                            typedef struct {
                                uint8_t dir;     // 方向：输入/输出
                                uint8_t mask;    // 引脚掩码
                            } GPIO_DIR_CONFIG;
                            
                            GPIO_DIR_CONFIG dir_config;
                            param_pos = Get_Parameter(Buf, param_pos, &dir_config, sizeof(GPIO_DIR_CONFIG));
                            if (param_pos < 0) {
                                char* error_msg = "Error: Invalid parameter for GPIO init command\r\n";
                                HAL_UART_Transmit(&huart1, (uint8_t*)error_msg, strlen(error_msg), 100);
                                break;
                            }
                            
                            // 调用GPIO方向设置函数
                            Process_GPIO_SetOutput(header->device_index, dir_config.mask);
                        } else {
                            char* error_msg = "Error: No parameters for GPIO init command\r\n";
                            HAL_UART_Transmit(&huart1, (uint8_t*)error_msg, strlen(error_msg), 100);
                        }
                        break;
                    }
                    case CMD_WRITE: {
                        // GPIO写数据
                        if (header->data_len > 0) {
                            uint8_t gpio_value = Buf[sizeof(GENERIC_CMD_HEADER)];
                            Process_GPIO_Write(header->device_index, gpio_value);
                        } else {
                            char* error_msg = "Error: No data for GPIO write command\r\n";
                            HAL_UART_Transmit(&huart1, (uint8_t*)error_msg, strlen(error_msg), 100);
                        }
                        break;
                    }
                    default: {
                        char buffer[64];
                        sprintf(buffer, "Unknown GPIO command ID: 0x%02X\r\n", header->cmd_id);
                        HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
                        break;
                    }
                }
                break;
            }
            
            case PROTOCOL_POWER: {
                switch (header->cmd_id) {
                    case POWER_CMD_SET_VOLTAGE: {
                        uint16_t voltage_mv = 0;
                        if (header->data_len >= sizeof(uint16_t)) {
                            memcpy(&voltage_mv, Buf + sizeof(GENERIC_CMD_HEADER), sizeof(uint16_t));
                            Process_Power_SetVoltage(header->device_index, voltage_mv);
                        } else {
                            char* error_msg = "Error: No voltage value for POWER_SET_VOLTAGE command\r\n";
                            HAL_UART_Transmit(&huart1, (uint8_t*)error_msg, strlen(error_msg), 100);
                        }
                        break;
                    }
                    
                    case POWER_CMD_START_CURRENT_READING: {
                        Process_Power_StartCurrentReading(header->device_index);
                        break;
                    }
                    
                    case POWER_CMD_STOP_CURRENT_READING: {
                        Process_Power_StopCurrentReading(header->device_index);
                        break;
                    }
                    
                    case POWER_CMD_READ_CURRENT_DATA: {
                        uint8_t response_buffer[128];  // 响应缓冲区
                        int response_len = Process_Power_ReadCurrentData(header->device_index, response_buffer, sizeof(response_buffer));
                        if (response_len > 0) {
                            char debug_msg[64];
                            sprintf(debug_msg, "[Debug] Response data ready, length: %d bytes\r\n", response_len);
                            HAL_UART_Transmit(&huart1, (uint8_t*)debug_msg, strlen(debug_msg), 100);
                        }
                        break;
                    }
                    
                    default: {
                        char buffer[64];
                        sprintf(buffer, "Unknown POWER command ID: 0x%02X\r\n", header->cmd_id);
                        HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
                        break;
                    }
                }
                break;
            }
            default: {
                // 未知协议类型
                char buffer[64];
                sprintf(buffer, "Unknown protocol type: 0x%02X\r\n", header->protocol_type);
                HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
                break;
            }
        }
    } else {
        // 数据过短，直接打印原始数据
        HAL_UART_Transmit(&huart1, Buf, *Len, 100);
    }
    
    return 0;
}
