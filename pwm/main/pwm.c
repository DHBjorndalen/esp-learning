#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// ─── Pin / peripheral config ────────────────────────────────────────────────
#define LED_PIN         2
#define UART_NUM        UART_NUM_0
#define BUF_SIZE        256
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL    LEDC_CHANNEL_0
#define LEDC_DUTY_RES   LEDC_TIMER_13_BIT
#define LEDC_FREQUENCY  2500
#define LEDC_DUTY_MAX   ((1 << 13) - 1)   // 8191 for 13-bit resolution

// ─── Service status ──────────────────────────────────────────────────────────
typedef enum {
    SERVICE_STATUS_UNINITIALIZED,
    SERVICE_STATUS_OK,
    SERVICE_STATUS_ERROR,
} ServiceStatus_t;

// ─── Message types ───────────────────────────────────────────────────────────
typedef enum {
    MSG_LED_ON,
    MSG_LED_OFF,
    MSG_LED_PULSE,
    MSG_LED_DUTY_25,
    MSG_LED_DUTY_75,
    MSG_LED_UNKNOWN,
} LedCommand_t;

typedef struct {
    LedCommand_t command;
} LedMessage_t;

// ─── UART Service ────────────────────────────────────────────────────────────
typedef struct {
    QueueHandle_t   outbound;        // sends to LED service's inbound queue
    ServiceStatus_t status;
    uint32_t        messages_sent;   // log: how many commands dispatched
    uint32_t        parse_errors;    // log: how many unknown commands seen
} UartService_t;

static UartService_t uart_service = {0};

static LedCommand_t uart_service_parse(const char *raw) {
    if (strcmp(raw, "on")    == 0) return MSG_LED_ON;
    if (strcmp(raw, "off")   == 0) return MSG_LED_OFF;
    if (strcmp(raw, "pulse") == 0) return MSG_LED_PULSE;
    if (strcmp(raw, "25")    == 0) return MSG_LED_DUTY_25;
    if (strcmp(raw, "75")    == 0) return MSG_LED_DUTY_75;
    return MSG_LED_UNKNOWN;
}

static void uart_service_log(const char *event) {
    printf("[UART SERVICE] %s | sent: %lu | errors: %lu\n",
           event, uart_service.messages_sent, uart_service.parse_errors);
}

void uart_service_task(void *pvParameters) {
    uart_service.status = SERVICE_STATUS_OK;
    uint8_t raw[BUF_SIZE];
    int index = 0;

    while (1) {
        uint8_t byte;
        int len = uart_read_bytes(UART_NUM, &byte, 1, 100 / portTICK_PERIOD_MS);
        if (len > 0) {
            if (byte == '\n' || byte == '\r') {
                if (index > 0) {
                    raw[index] = '\0';
                    index = 0;

                    LedMessage_t msg = { .command = uart_service_parse((char*)raw) };

                    if (msg.command == MSG_LED_UNKNOWN) {
                        uart_service.parse_errors++;
                        uart_service.status = SERVICE_STATUS_ERROR;
                        uart_service_log("Unknown command received");
                        uart_service.status = SERVICE_STATUS_OK;
                    } else {
                        xQueueSend(uart_service.outbound, &msg, portMAX_DELAY);
                        uart_service.messages_sent++;
                        uart_service_log("Command dispatched");
                    }
                }
            } else {
                if (index < BUF_SIZE - 1) raw[index++] = byte;
            }
        }
    }
}

// ─── LED Service ─────────────────────────────────────────────────────────────
typedef struct {
    QueueHandle_t   inbound;         // receives LedMessage_t from UART service
    ServiceStatus_t status;
    uint32_t        commands_handled; // log
    LedCommand_t    last_command;     // log
} LedService_t;

static LedService_t led_service = {0};

static void led_service_log(const char *event) {
    printf("[LED SERVICE] %s | handled: %lu | last cmd: %d\n",
           event, led_service.commands_handled, led_service.last_command);
}

static void led_service_set_duty(uint32_t duty) {
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

void led_service_task(void *pvParameters) {
    led_service.status = SERVICE_STATUS_OK;
    LedMessage_t msg;

    while (1) {
        if (xQueueReceive(led_service.inbound, &msg, portMAX_DELAY)) {
            led_service.last_command = msg.command;
            led_service.commands_handled++;

            switch (msg.command) {
                case MSG_LED_ON:
                    led_service_log("ON");
                    led_service_set_duty(LEDC_DUTY_MAX);
                    break;

                case MSG_LED_OFF:
                    led_service_log("OFF");
                    led_service_set_duty(0);
                    break;

                case MSG_LED_DUTY_25:
                    led_service_log("25% duty");
                    led_service_set_duty(LEDC_DUTY_MAX / 4);
                    break;

                case MSG_LED_DUTY_75:
                    led_service_log("75% duty");
                    led_service_set_duty((LEDC_DUTY_MAX * 3) / 4);
                    break;

                case MSG_LED_PULSE:
                    led_service_log("PULSE");
                    for (int p = 0; p < 5; p++) {
                        for (int i = 0; i <= LEDC_DUTY_MAX; i++) {
                            led_service_set_duty(i);
                            vTaskDelay(1 / portTICK_PERIOD_MS);
                        }
                        for (int i = LEDC_DUTY_MAX; i >= 0; i--) {
                            led_service_set_duty(i);
                            vTaskDelay(1 / portTICK_PERIOD_MS);
                        }
                    }
                    break;

                default:
                    led_service.status = SERVICE_STATUS_ERROR;
                    led_service_log("Unhandled command");
                    led_service.status = SERVICE_STATUS_OK;
                    break;
            }
        }
    }
}

// ─── App entry point ─────────────────────────────────────────────────────────
void app_main(void) {
    // GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask   = (1ULL << LED_PIN),
        .mode           = GPIO_MODE_OUTPUT,
        .pull_up_en     = GPIO_PULLUP_DISABLE,
        .pull_down_en   = GPIO_PULLDOWN_DISABLE,
        .intr_type      = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // UART
    uart_config_t uart_config = {
        .baud_rate  = 115200,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM, &uart_config);

    // LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode      = LEDC_MODE,
        .timer_num       = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz         = LEDC_FREQUENCY,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&ledc_timer);

    // LEDC channel
    ledc_channel_config_t ledc_channel_cfg = {
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CHANNEL,
        .timer_sel  = LEDC_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = LED_PIN,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config(&ledc_channel_cfg);

    // Wire services together: UART's outbound IS LED's inbound
    QueueHandle_t uart_to_led = xQueueCreate(10, sizeof(LedMessage_t));
    uart_service.outbound = uart_to_led;
    led_service.inbound   = uart_to_led;

    printf("System ready. Commands: on | off | pulse | 25 | 75\n");

    xTaskCreate(uart_service_task, "uart_service", 2048, NULL, 5, NULL);
    xTaskCreate(led_service_task,  "led_service",  2048, NULL, 5, NULL);
}