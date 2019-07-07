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
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_system.h"
#include "esp_wifi.h"

#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF ESP_IF_WIFI_STA
#define ESPNOW_CHANNEL 1

static const char *TAG = "remote_switch_master";
/* static uint8_t remote_mac[ESP_NOW_ETH_ALEN] = {0x5c, 0xcf, 0x7f, */
/* 0xb7, 0x22, 0x58}; */
static uint8_t remote_mac[ESP_NOW_ETH_ALEN] = {0xa0, 0x20, 0xa6,
                                               0xb,  0x4d, 0x10};

static esp_err_t event_handler(void *ctx, system_event_t *event) {
  switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
      ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
      break;
    default:
      break;
  }
  return ESP_OK;
}

static void wifi_init(void) {
  /* tcpip_adapter_init(); */
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(ESPNOW_WIFI_MODE));
  ESP_ERROR_CHECK(esp_wifi_start());

  /* In order to simplify example, channel is set after WiFi started.
   * This is not necessary in real application if the two devices have
   * been already on the same channel.
   */
  ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, 0));
}

static void espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data,
                           int len) {
  if (mac_addr == NULL || data == NULL || len <= 0) {
    ESP_LOGE(TAG, "invalid receive cb arg");
    return;
  }

  printf("received %d bytes of data: %02x\n", len, data[0]);
}

static void espnow_init(void) {
  /* Initialize ESPNOW and register sending and receiving callback function. */
  ESP_ERROR_CHECK(esp_now_init());
  /* ESP_ERROR_CHECK(esp_now_register_send_cb(example_espnow_send_cb)); */
  ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));

  /* Set primary master key. */
  /* ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK)); */

  /* Add broadcast peer information to peer list. */
  esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
  if (peer == NULL) {
    ESP_LOGE(TAG, "malloc peer information fail");
    ESP_ERROR_CHECK(ESP_FAIL);
  }

  memset(peer, 0, sizeof(esp_now_peer_info_t));
  peer->channel = ESPNOW_CHANNEL;
  peer->ifidx = ESPNOW_WIFI_IF;
  peer->encrypt = false;
  memcpy(peer->peer_addr, remote_mac, ESP_NOW_ETH_ALEN);
  ESP_ERROR_CHECK(esp_now_add_peer(peer));
  free(peer);
}

/******************************************************************************
 * FunctionName : app_main
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void app_main(void) {
  printf("SDK version:%s\n", esp_get_idf_version());

  wifi_init();
  espnow_init();

  printf("listening");
  vTaskDelay(40000 / portTICK_RATE_MS);

  /* while (1) { */

  /* uint8_t data[10] = {0x5c, 0xcf, 0x7f}; */

  /* printf("sending\n"); */
  /* ESP_ERROR_CHECK(esp_now_send(remote_mac, data, 3)); */
  /* printf("sended\n"); */
  /* } */
}
