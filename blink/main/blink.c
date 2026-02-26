#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void blink_task(void *pvParameters) {
    gpio_reset_pin(2);
    gpio_set_direction(2, GPIO_MODE_OUTPUT);
    while (1) {
        gpio_set_level(2, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(2, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
    xTaskCreate(blink_task, "blink_task", 2048, NULL, 5, NULL);
}
