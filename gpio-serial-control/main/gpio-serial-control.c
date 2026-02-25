#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_PIN 2
#define UART_NUM UART_NUM_0
#define BUF_SIZE 256

void app_main(void)
{
    // Configure LED pin
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

    uint8_t data[BUF_SIZE];
    printf("Ready. Type 'led on' or 'led off'\n");

    while(1)
    {
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE - 1, 100 / portTICK_PERIOD_MS);

        if(len > 0)
        {
            data[len] = '\0';
            printf("Received: %s\n", data);

            if(strncmp((char*)data, "led on", 6) == 0)
            {
                gpio_set_level(LED_PIN, 1);
                printf("LED on\n");
            }
            else if(strncmp((char*)data, "led off", 7) == 0)
            {
                gpio_set_level(LED_PIN, 0);
                printf("LED off\n");
            }
            else
            {
                printf("Unknown command\n");
            }
        }
    }
}