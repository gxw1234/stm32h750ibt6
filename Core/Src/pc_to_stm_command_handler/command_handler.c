#include "command_handler.h"
#include "main.h"
#include "../tasks/MP8865_task.h"
#include "tasks/ads1220_task.h"  
#include "pc_to_stm_command_handler/handler_spi.h" 
#include "pc_to_stm_command_handler/handler_iic.h" 
#include "handler_gpio.h" 
#include "pc_to_stm_command_handler/handler_reset_usb3300_stm32.h" 
#include "usbd_cdc_if.h"
#include "tasks/image_queue_task.h"

int Get_Parameter(uint8_t* buffer, int pos, void* data, uint16_t max_len) {
    PARAM_HEADER header;
    memcpy(&header, buffer + pos, sizeof(PARAM_HEADER));
    pos += sizeof(PARAM_HEADER);
    if (header.param_len > max_len) {
        return -1;
    }
    memcpy(data, buffer + pos, header.param_len);
    pos += header.param_len;
    return pos;
}


static void Process_SPI_Init(uint8_t spi_index, PSPI_CONFIG pConfig) {
    printf("STM32_SPI Init: Index=%d, Mode=%d, Master=%d, CPOL=%d, CPHA=%d, LSB=%d, SelPol=%d, Clock=%lu\r\n", 
       spi_index, pConfig->Mode, pConfig->Master, pConfig->CPOL, pConfig->CPHA, 
       pConfig->LSBFirst, pConfig->SelPolarity, pConfig->ClockSpeedHz);
       
    HAL_StatusTypeDef status = Handler_SPI_Init(spi_index, pConfig);
    // if (status == HAL_OK) {

    //     printf("SPI init SUCCESSFUL: %d\r\n", spi_index);
    // } else {

    //     printf("SPI init fail: %d, eer: %d\r\n", spi_index, status);
    // }

}


static void Process_Power_SetVoltage(uint8_t channel, uint16_t voltage_mv) {
    // 将毫伏转换为伏特
    float voltage_v = voltage_mv / 1000.0f;
    
    // printf("Power Set Voltage: Channel=%d, Target=%.3fV (%dmV)\r\n", 
    //        channel, voltage_v, voltage_mv);
    
    HAL_StatusTypeDef status = MP8865_SetVoltageV(voltage_v);
    
    // if (status == HAL_OK) {
    //     printf("MP8865 Set voltage success: %.3fV\r\n", voltage_v);
    // } else {
    //     printf("MP8865 Set voltage failed, error code: %d\r\n", status);
    // }
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

    printf("IIC init: %d, ClockSpeed=%lu, OwnAddr=0x%04X, Master=%d, AddrBits=%d, EnablePu=%d\r\n", 
            iic_index, pConfig->ClockSpeedHz, pConfig->OwnAddr, pConfig->Master, pConfig->AddrBits, pConfig->EnablePu);
    
    // 调用handler_iic中的初始化函数
    HAL_StatusTypeDef status = Handler_IIC_Init(iic_index, pConfig);
    
    if (status == HAL_OK) {

        printf("IIC init SUCCESSFUL: %d\r\n", iic_index);
    } else {

        printf("IIC init fail: %d, error: %d\r\n", iic_index, status);
    }
    


}


static void Process_IIC_SlaveWrite(uint8_t iic_index, uint8_t* pData, uint16_t data_len, uint32_t timeout) {

    HAL_StatusTypeDef status = Handler_IIC_SlaveWriteBytes(iic_index, pData, data_len, timeout);
    
}


static int Process_Power_ReadCurrentData(uint8_t channel, uint8_t* response_buf, int max_len) {

    

    float current_value = 123.456f;
    int response_len = 0;
    

    if (max_len >= sizeof(GENERIC_CMD_HEADER) + sizeof(float)) {
        GENERIC_CMD_HEADER* response_header = (GENERIC_CMD_HEADER*)response_buf;
        response_header->protocol_type = PROTOCOL_POWER;
        response_header->cmd_id = POWER_CMD_READ_CURRENT_DATA;
        response_header->device_index = channel;
        response_header->param_count = 0;
        response_header->data_len = sizeof(float); 
        memcpy(response_buf + sizeof(GENERIC_CMD_HEADER), &current_value, sizeof(float));
        response_len = sizeof(GENERIC_CMD_HEADER) + sizeof(float);
        printf("Current data: %.3f\r\n", current_value);
    }
    
    return response_len;
}


static void Process_SPI_Write(uint8_t spi_index, uint8_t* data, uint16_t data_len) {
    HAL_StatusTypeDef status = Handler_SPI_Transmit(spi_index, data, NULL, data_len, 1000);
}

static void Process_SPI_Queue_start(uint8_t spi_index) {
    // 启动图像队列处理
    HAL_StatusTypeDef status = ImageQueue_Start();
    
    // if (status == HAL_OK) {
    //     printf("SPI Queue started successfully for index: %d\r\n", spi_index);
    // } else {
    //     printf("Failed to start SPI Queue for index: %d, error: %d\r\n", spi_index, status);
    // }
}


static void Process_SPI_Queue_stop(uint8_t spi_index) {
    // 停止图像队列处理
    HAL_StatusTypeDef status = ImageQueue_Stop();
    
    // if (status == HAL_OK) {
    //     printf("SPI Queue stopped successfully for index: %d\r\n", spi_index);
    // } else {
    //     printf("Failed to stop SPI Queue for index: %d, error: %d\r\n", spi_index, status);
    // }
}

static void Process_SPI_Queue_Status(uint8_t spi_index) {
    // 获取实际的队列状态
    uint8_t queue_count = ImageQueue_GetStatus();

    // printf("SPI Queue status: %d\r\n", queue_count);
    
    
    // 定义队列状态响应数据包
    typedef struct {
        GENERIC_CMD_HEADER header;
        uint8_t queue_status;  // 队列状态数据
    } Queue_Status_Response;
    
    Queue_Status_Response response;
    
    // 设置协议头
    response.header.protocol_type = PROTOCOL_SPI;        // SPI协议
    response.header.cmd_id = CMD_READ;           // 队列状态命令
    response.header.device_index = spi_index;            // 使用传入的索引
    response.header.param_count = 0;                     // 无参数
    response.header.data_len = sizeof(uint8_t);          // 数据长度1字节
    response.header.total_packets = sizeof(Queue_Status_Response);  // 总包大小
    
    response.queue_status = queue_count;
    
    uint8_t ret = CDC_Transmit_HS((uint8_t*)&response, sizeof(response));
}


static void Process_GPIO_SetOutput(uint8_t gpio_index, uint8_t output_mask) {

    HAL_StatusTypeDef status = Handler_GPIO_SetOutput(gpio_index, output_mask);
    

}


static void Process_GPIO_Write(uint8_t gpio_index, uint8_t write_value) {
    HAL_StatusTypeDef status = Handler_GPIO_Write(gpio_index, write_value);
}


int8_t Process_Command(uint8_t* Buf, uint32_t *Len) {
   
    if (*Len >= sizeof(GENERIC_CMD_HEADER)) {
        GENERIC_CMD_HEADER* header = (GENERIC_CMD_HEADER*)Buf;


        // printf("SPI Init Command: Device=%d, ParamCount=%d, ProtocolType=%d, CmdId=%d\r\n", 
        // header->device_index, header->param_count, header->protocol_type, header->cmd_id);

        switch (header->protocol_type) {
            case PROTOCOL_SPI: {

                switch (header->cmd_id) {
                    case CMD_INIT: {
                        if (header->param_count > 0) {
                            int pos = sizeof(GENERIC_CMD_HEADER);
                            SPI_CONFIG spi_config;
                            pos = Get_Parameter(Buf, pos, &spi_config, sizeof(SPI_CONFIG));
                            if (pos > 0) {
                                Process_SPI_Init(header->device_index, &spi_config);
                            } else {
                                printf("Error: Invalid parameter format for SPI_INIT\r\n");
                            }
                        } else {
                            printf("Error: No parameters for SPI_INIT command\r\n");
                        }
                        break;
                    }
                    case CMD_QUEUE_WRITE: {
                        uint8_t status = 0x00;  // 0x00=整包数据完整, 0x01=数据不完整, 0xFF=其他错误
                        
                        // 检查整包数据完整性
                        if (header->data_len > 0) {
                            int data_pos = sizeof(GENERIC_CMD_HEADER);
                            for (int i = 0; i < header->param_count; i++) {
                                PARAM_HEADER* param_header = (PARAM_HEADER*)(Buf + data_pos);
                                data_pos += sizeof(PARAM_HEADER) + param_header->param_len;
                            }
                            
                            // 验证数据长度是否匹配
                            int expected_total_len = sizeof(GENERIC_CMD_HEADER) + header->data_len + sizeof(uint32_t);
                            if (header->total_packets != expected_total_len) {
                                status = 0x01;  // 整包长度不匹配
                                printf("Packet length mismatch: expected=%d, actual=%d\r\n", 
                                       expected_total_len, header->total_packets);
                            } else {
                                // 数据包完整，添加到图像队列
                                uint8_t* write_data = Buf + data_pos;
                                HAL_StatusTypeDef result = ImageQueue_AddFrame(write_data, header->data_len);
                                if (result != HAL_OK) {
                                    // 图像队列操作失败，但整包数据是完整的
                                    printf("ImageQueue_AddFrame failed: %d\r\n", result);
                                }
                                // 注意：不管图像队列是否成功，只要整包数据完整就返回成功
                            }
                        } else {
                            status = 0xFF;  // 数据长度错误
                            printf("Error: No data for SPI_WRITE command\r\n");
                        }
                        
                        // 发送带协议头的状态响应
                        typedef struct {
                            GENERIC_CMD_HEADER header;
                            uint8_t status;  // 状态数据
                        } Queue_Write_Response;
                        
                        Queue_Write_Response response;
                        
                        // 设置协议头
                        response.header.protocol_type = PROTOCOL_SPI;
                        response.header.cmd_id = CMD_QUEUE_WRITE;
                        response.header.device_index = header->device_index;
                        response.header.param_count = 0;
                        response.header.data_len = sizeof(uint8_t);
                        response.header.total_packets = sizeof(Queue_Write_Response);
                        response.status = status;
                        
                        CDC_Transmit_HS((uint8_t*)&response, sizeof(response));
                        break;
                    }

                    case CMD_WRITE: {
                        if (header->data_len > 0) {
                            int data_pos = sizeof(GENERIC_CMD_HEADER);
                            for (int i = 0; i < header->param_count; i++) {
                                PARAM_HEADER* param_header = (PARAM_HEADER*)(Buf + data_pos);
                                data_pos += sizeof(PARAM_HEADER) + param_header->param_len;
                            }
                            uint8_t* write_data = Buf + data_pos;

                            Process_SPI_Write(header->device_index, write_data, header->data_len);

                            
                        } else {

                            printf("Error: No data for SPI_WRITE command\r\n");
                        }
                        break;
                    }



                    case CMD_QUEUE_STATUS: {
                        Process_SPI_Queue_Status(header->device_index);
                        break;
                    }
                    case CMD_QUEUE_START: {
                        Process_SPI_Queue_start(header->device_index);
                        break;
                    }
                    case CMD_QUEUE_STOP: {
                        Process_SPI_Queue_stop(header->device_index);
                        break;
                    }
                    
                    
                    case CMD_READ:
                    case CMD_TRANSFER:
                        {
                            printf("Unknown SPI command ID: 0x%02X\r\n", header->cmd_id);
                        }
                        break;
                    default: {
                        printf("Unknown SPI command ID: 0x%02X\r\n", header->cmd_id);
                        break;
                    }
                }
                break;
            }
            case PROTOCOL_IIC: {
                switch (header->cmd_id) {
                    case CMD_INIT: {

                        break;
                    }
                    case CMD_WRITE: {

                        break;
                    }
                    default: {

                        break;
                    }
                }
                break;
            }         
            case PROTOCOL_UART: {

                break;
            }
            case PROTOCOL_RESETSTM32: {

                switch (header->cmd_id) {

                    case CMD_INIT:
                    {

                        handler_reset_usb3300_stm32();


                        break;
                    }
                }


                break;
            }
            
            case PROTOCOL_GPIO: {
                switch (header->cmd_id) {
                    case GPIO_DIR_OUTPUT: {
                        int pos = sizeof(GENERIC_CMD_HEADER);
                        uint8_t output_mask;
                        pos = Get_Parameter(Buf, pos, &output_mask, sizeof(uint8_t));
                        if (pos > 0) {
                            Process_GPIO_SetOutput(header->device_index, output_mask);
                        } else {
                            printf("Error: Invalid parameter format for GPIO_DIR_OUTPUT\r\n");
                        }
                        break;
                    }
                    case GPIO_DIR_OUTPUT_OD: {
                        int pos = sizeof(GENERIC_CMD_HEADER);
                        uint8_t output_mask;
                        pos = Get_Parameter(Buf, pos, &output_mask, sizeof(uint8_t));
                        if (pos > 0) {
                            Handler_GPIO_SetOpenDrain(header->device_index, output_mask);
                        } else {
                            printf("Error: Invalid parameter format for GPIO_DIR_OUTPUT_OD\r\n");
                        }
                        break;
                    }
                    case GPIO_DIR_INPUT: {
 
                        break;
                    }
                    case GPIO_DIR_WRITE: {
                        if (header->param_count > 0) {
                            int pos = sizeof(GENERIC_CMD_HEADER);
                            uint8_t gpio_value;
                            pos = Get_Parameter(Buf, pos, &gpio_value, sizeof(uint8_t));
                            if (pos > 0) {
                                Process_GPIO_Write(header->device_index, gpio_value);
                            } else {


                                printf("Error: Invalid parameter format for GPIO_WRITE\r\n");
                            }
                        } else {

                            printf("Error: No parameters for GPIO_WRITE command\r\n");
                        }
                        break;
                    }
                    default: {


                        printf("Unknown GPIO command ID: 0x%02X\r\n", header->cmd_id);
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
   
                            printf("Error: No voltage value for POWER_SET_VOLTAGE command\r\n");
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
    
                            printf("Response data ready, length: %d bytes\r\n", response_len);
                        }
                        break;
                    }
                    
                    default: {
 
                        printf("Unknown POWER command ID: 0x%02X\r\n", header->cmd_id);
                        break;
                    }
                }
                break;
            }


            default: {

                printf("Unknown protocol type: 0x%02X\r\n", header->protocol_type);
                break;
            }
        }
    } else {
        // 数据过短，直接打印原始数据

        printf("Data too short, length: %d\r\n", *Len);
    }
    
    return 0;
}
