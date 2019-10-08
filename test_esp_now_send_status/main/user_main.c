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

static const char *TAG = "test_esp_now_send_status";
static uint8_t peer_mac[ESP_NOW_ETH_ALEN] = {0x5c, 0xcf, 0x7f,
                                             0xb7, 0x22, 0x58};

static void rcp_cmd_send_cb(esp_now_send_status_t status) {}

void app_main(void) {
  printf("SDK version: %s\n", esp_get_idf_version());

  // configure rcp
  rcp_master_init(peer_mac, rcp_cmd_send_cb);

  rcp_cmd_t cmd = {.op0 = op_no, .op1 = op_on};

  for (;;) {
    rcp_send(&cmd);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
