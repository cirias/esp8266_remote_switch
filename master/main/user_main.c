/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// Board 01
// mac: a0:20:a6:b:4d:10
//
// Board 02
// mac: 5c:cf:7f:b7:22:58

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "driver/uart.h"

#include "esp_log.h"
#include "esp_now.h"
#include "esp_system.h"

#include "rcp.h"

#define MESSAGE_LEN (2)
#define EX_UART_NUM UART_NUM_0

typedef struct {
  uint8_t index;
  uint8_t op;
} message_t;

static const char *TAG = "remote_switch_master";
static uint8_t peer_mac[ESP_NOW_ETH_ALEN] = {0x5c, 0xcf, 0x7f,
                                             0xb7, 0x22, 0x58};

static QueueHandle_t uart0_queue;

static TaskHandle_t rcp_send_task_handle = NULL;
static SemaphoreHandle_t command_semaphore = NULL;
static message_t current_message = {.index = 0, .op = OP_NO};
static QueueHandle_t rcp_cmd_send_callback_queue = NULL;

static void uart_event_task(void *pvParameters) {
  uart_event_t event;
  uint8_t msgbuf[4];  // ' 0o'
  char *ok = "OK\n";

  for (;;) {
    // Waiting for UART event.
    if (xQueueReceive(uart0_queue, &event, portMAX_DELAY)) {
      ESP_LOGI(TAG, "uart[%d] event:", EX_UART_NUM);

      switch (event.type) {
        // Event of UART receving data
        // We'd better handler data event fast, there would be much more data
        // events than other types of events. If we take too much time on data
        // event, the queue might be full.
        case UART_DATA:
          ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
          uart_read_bytes(EX_UART_NUM, msgbuf, event.size, portMAX_DELAY);

          char c = msgbuf[event.size - 1];
          uint8_t index;
          uint8_t op;
          if (c == 'a') {
            index = 0;
            op = OP_ON;
          } else if (c == 'b') {
            index = 0;
            op = OP_OFF;
          } else if (c == 'A') {
            index = 1;
            op = OP_ON;
          } else if (c == 'B') {
            index = 1;
            op = OP_OFF;
          } else {
            ESP_LOGE(TAG, "invalid uart message");
            continue;
          }

          if (xSemaphoreTake(command_semaphore, portMAX_DELAY) != pdTRUE) {
            ESP_LOGE(TAG, "could not take command semaphore");
            break;
          }

          current_message.index = index;
          current_message.op = op;

          if (xSemaphoreGive(command_semaphore) != pdTRUE) {
            ESP_LOGE(TAG, "could not release command semaphore");
            break;
          }

          vTaskResume(rcp_send_task_handle);

          ESP_LOGI(TAG, "[DATA EVT]:");
          uart_write_bytes(EX_UART_NUM, ok, sizeof(ok));
          break;

        // Event of HW FIFO overflow detected
        case UART_FIFO_OVF:
          ESP_LOGE(TAG, "hw fifo overflow");
          // If fifo overflow happened, you should consider adding flow control
          // for your application. The ISR has already reset the rx FIFO, As an
          // example, we directly flush the rx buffer here in order to read more
          // data.
          uart_flush_input(EX_UART_NUM);
          xQueueReset(uart0_queue);
          break;

        // Event of UART ring buffer full
        case UART_BUFFER_FULL:
          ESP_LOGE(TAG, "uart ring buffer full");
          // If buffer full happened, you should consider encreasing your buffer
          // size As an example, we directly flush the rx buffer here in order
          // to read more data.
          uart_flush_input(EX_UART_NUM);
          xQueueReset(uart0_queue);
          break;

        case UART_PARITY_ERR:
          ESP_LOGE(TAG, "uart parity error");
          break;

        // Event of UART frame error
        case UART_FRAME_ERR:
          ESP_LOGE(TAG, "uart frame error");
          break;

        // Others
        default:
          ESP_LOGI(TAG, "uart event type: %d", event.type);
          break;
      }
    }
  }

  vTaskDelete(NULL);
}

static void rcp_send_task(void *pvParameters) {
  rcp_cmd_t cmd;
  esp_now_send_status_t send_status;

  for (;;) {
    if (xSemaphoreTake(command_semaphore, portMAX_DELAY) != pdTRUE) {
      ESP_LOGE(TAG, "could not take command semaphore");
      break;
    }

    if (current_message.index == 0) {
      cmd.op0 = current_message.op;
    } else if (current_message.index == 1) {
      cmd.op1 = current_message.op;
    } else {
      ESP_LOGE(TAG, "current_message index overflow");
    }

    if (xSemaphoreGive(command_semaphore) != pdTRUE) {
      ESP_LOGE(TAG, "could not release command semaphore");
      break;
    }

    if (cmd.op0 == OP_NO && cmd.op1 == OP_NO) {
      ESP_LOGI(TAG, "ignore empty command");
      vTaskSuspend(NULL);
    }

    rcp_send(&cmd);

    if (xQueueReceive(rcp_cmd_send_callback_queue, &send_status,
                      portMAX_DELAY) != pdTRUE) {
      ESP_LOGE(TAG, "could not get rcp send callback");
      continue;
    }

    if (send_status == ESP_NOW_SEND_FAIL) {
      vTaskDelay(200 / portTICK_PERIOD_MS);
      continue;
    }

    ESP_LOGI(TAG, "message is received by client");
    cmd.op0 = OP_NO;
    cmd.op1 = OP_NO;
    vTaskSuspend(NULL);
  }

  vTaskDelete(NULL);
}

static void rcp_cmd_send_cb(esp_now_send_status_t status) {
  if (xQueueOverwrite(rcp_cmd_send_callback_queue, &status) != pdTRUE) {
    ESP_LOGE(TAG, "could not write rcp_cmd_send_callback_queue");
  }
}

void app_main(void) {
  printf("SDK version: %s\n", esp_get_idf_version());

  // configure rcp
  rcp_master_init(peer_mac, rcp_cmd_send_cb);

  /* Configure UART. Note that REF_TICK is used so that the baud rate remains
   * correct while APB frequency is changing in light sleep mode.
   */
  uart_config_t uart_config = {.baud_rate = CONFIG_CONSOLE_UART_BAUDRATE,
                               .data_bits = UART_DATA_8_BITS,
                               .parity = UART_PARITY_EVEN,
                               .stop_bits = UART_STOP_BITS_1,
                               .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
  ESP_ERROR_CHECK(uart_param_config(EX_UART_NUM, &uart_config));

  /* Install UART driver for interrupt-driven reads and writes */
  ESP_ERROR_CHECK(uart_driver_install(EX_UART_NUM, 256, 256, 20, &uart0_queue));

  command_semaphore = xSemaphoreCreateMutex();

  rcp_cmd_send_callback_queue = xQueueCreate(1, sizeof(esp_now_send_status_t));

  // Create a task to handler UART event from ISR
  xTaskCreate(uart_event_task, "uart_event_task", 1024, NULL, 12, NULL);

  xTaskCreate(rcp_send_task, "rcp_send_task", 1024, NULL, 12,
              &rcp_send_task_handle);
}
