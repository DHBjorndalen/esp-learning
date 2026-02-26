#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define LED_PIN 2
#define UART_NUM UART_NUM_0
#define BUF_SIZE 256
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL    LEDC_CHANNEL_0
#define LEDC_DUTY_RES   LEDC_TIMER_13_BIT
#define LEDC_FREQUENCY  2500

QueueHandle_t led_queue;

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
                    index = 0;
                }
            } else {
                if (index < BUF_SIZE - 1) {
                    data[index++] = byte;
                }
            }
        }
    }
}

void led_task(void *pvParameters) {
    uint8_t data[BUF_SIZE];
    while (1) {
        if (xQueueReceive(led_queue, data, portMAX_DELAY)) {
            if (strncmp((char*)data, "pulse", 5) == 0) {
                for(int i = 0; i<5; i++) {
                    printf("Pulsing LED...\n");
                    for(int i = 0; i <8192; i++) {
                        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, i);
                        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
                        vTaskDelay(7 / portTICK_PERIOD_MS);
                    }
                    for(int i = 8191; i >=0; i--) {
                        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, i);
                        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
                        vTaskDelay(7 / portTICK_PERIOD_MS);
                    }
                }
            }else if(strncmp((char*)data, "on", 2) == 0) {
                printf("Turning LED on...\n");
                ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 8192);
                ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
            }else if(strncmp((char*)data, "off", 3) == 0) {
                printf("Turning LED off...\n");
                ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
                ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
            }else if(strncmp((char*)data, "25", 2) == 0) {
                printf("Turning LED on...\n");
                ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 2048);  // 25% duty cycle
                ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
            }else if(strncmp((char*)data, "75", 2) == 0) {
                printf("Turning LED off...\n");
                ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 6144);  // 75% duty cycle
                ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
            }
            else {
                printf("Unknown command\n");
            }
        }
    }
}

void app_main(void) {
    // Configure GPIO
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

    // Configure LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode      = LEDC_MODE,
        .timer_num       = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz         = LEDC_FREQUENCY,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&ledc_timer);

    // Configure LEDC channel
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CHANNEL,
        .timer_sel  = LEDC_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = LED_PIN,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config(&ledc_channel);

    led_queue = xQueueCreate(10, BUF_SIZE);

    printf("Ready. Type 'pulse'\n");

    xTaskCreate(uart_task, "uart_task", 2048, NULL, 5, NULL);
    xTaskCreate(led_task, "led_task", 2048, NULL, 5, NULL);
}