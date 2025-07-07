#include "tasks/debug_task.h"
#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "usbd_cdc_if.h"

void debug_task(void *argument)
{

    char data[1] = {        1};
    uint8_t ret = 0;
    vTaskDelay(pdMS_TO_TICKS(1000));
    while (1)
    {






        // ret = CDC_Transmit_HS(data,1);
        // printf("ret: %d\r\n", ret);

        vTaskDelay(pdMS_TO_TICKS(1000));




    }
}   