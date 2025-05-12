#ifndef ADS1220_TASK_H
#define ADS1220_TASK_H

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_spi.h"

extern SPI_HandleTypeDef hspi4;

void ADS1220_Task(void *argument);
void ADS1220_Init(void);
uint8_t SPI_TransmitReceive(uint8_t data);
void SPI_Transmit(uint8_t data);



#endif /* ADS1220_TASK_H */