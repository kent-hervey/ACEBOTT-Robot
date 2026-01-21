#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void hello_task(void *pvParameters)
{
    (void)pvParameters;
    while (1) {
          printf("Hello from ESP32! Tick: %lu\n", xTaskGetTickCount());
        vTaskDelay(pdMS_TO_TICKS(2000));  // print every 2 seconds
    }
}

void app_main(void)
{
    printf("Starting Hello World task...\n");
    xTaskCreate(hello_task, "hello_task", 2048, NULL, 5, NULL);
}
