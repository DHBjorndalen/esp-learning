#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define LED_PIN 2
#define UART_NUM UART_NUM_0
#define BUF_SIZE 256

// Queue handle - this is how the two tasks communicate
QueueHandle_t led_queue;

// Task 1: Reads UART and sends commands to the queue
void uart_task(void *pvParameters) {
    uint8_t data[BUF_SIZE];
    int index = 0;
    
    while (1) {
        uint8_t byte;
        int len = uart_read_bytes(UART_NUM, &byte, 1, 100 / portTICK_PERIOD_MS);
        if (len > 0) {
            if (byte == '\n' || byte == '\r') {
                if (index > 0) {
                    data[index] = '\0';
                    printf("Received: %s\n", data);
                    xQueueSend(led_queue, data, portMAX_DELAY);
                    index = 0;  // reset buffer for next command
                }
            } else {
                if (index < BUF_SIZE - 1) {
                    data[index++] = byte;
                }
            }
        }
    }
}

// Task 2: Receives commands from the queue and controls the LED
void led_task(void *pvParameters) {
    uint8_t data[BUF_SIZE];
    while (1) {
        // Block here until something arrives in the queue
        if (xQueueReceive(led_queue, data, portMAX_DELAY)) {
            if (strncmp((char*)data, "led on", 6) == 0) {
                gpio_set_level(LED_PIN, 1);
                printf("LED on\n");
            } else if (strncmp((char*)data, "led off", 7) == 0) {
                gpio_set_level(LED_PIN, 0);
                printf("LED off\n");
            } else if (strncmp((char*)data, "blink", 5) == 0) {
                for (int i = 0; i < 5; i++) {
                    gpio_set_level(LED_PIN, 1);
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                    gpio_set_level(LED_PIN, 0);
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                }
                printf("LED blinked\n");
            } else {
                printf("Unknown command\n");
            }
        }
    }
}

void app_main(void) {
    // Configure LED
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Configure UART
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM, &uart_config);

    // Create the queue - holds up to 10 messages of BUF_SIZE bytes each
    led_queue = xQueueCreate(10, BUF_SIZE);

    printf("Ready. Type 'led on' or 'led off'\n");

    // Create the tasks
    xTaskCreate(uart_task, "uart_task", 2048, NULL, 5, NULL);
    xTaskCreate(led_task, "led_task", 2048, NULL, 5, NULL);
}