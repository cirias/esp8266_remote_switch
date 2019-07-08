#include <stdio.h>
#include <string.h>

#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include "rcp.h"

#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF ESP_IF_WIFI_STA
#define ESPNOW_CHANNEL 1

static const char *TAG = "remove_switch_rcp";

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

static void espnow_init(const uint8_t *peer_addr, esp_now_send_cb_t send_cb,
                        esp_now_recv_cb_t recv_cb) {
  /* Initialize ESPNOW and register sending and receiving callback function. */
  ESP_ERROR_CHECK(esp_now_init());
  if (send_cb != NULL) {
    ESP_ERROR_CHECK(esp_now_register_send_cb(send_cb));
  }
  if (recv_cb != NULL) {
    ESP_ERROR_CHECK(esp_now_register_recv_cb(recv_cb));
  }

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
  memcpy(peer->peer_addr, peer_addr, ESP_NOW_ETH_ALEN);
  ESP_ERROR_CHECK(esp_now_add_peer(peer));
  free(peer);
}

static const uint8_t *_peer_addr = NULL;
static rcp_recv_cb_t _rcp_recv_cb = NULL;

static void rcp_espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data,
                               int len) {
  if (mac_addr == NULL || data == NULL || len <= 0) {
    ESP_LOGE(TAG, "invalid receive cb arg");
    return;
  }

  printf("received %d bytes of data: %02x\n", len, data[0]);
  // TODO verify the mac_addr

  rcp_cmd_t *rcp_cmd = (rcp_cmd_t *)data;

  _rcp_recv_cb(rcp_cmd);
}

void rcp_master_init(const uint8_t *peer_addr) {
  _peer_addr = peer_addr;

  wifi_init();
  espnow_init(peer_addr, NULL, NULL);
}

void rcp_slave_init(const uint8_t *peer_addr, rcp_recv_cb_t recv_cb) {
  _peer_addr = peer_addr;
  _rcp_recv_cb = recv_cb;

  wifi_init();
  espnow_init(peer_addr, NULL, rcp_espnow_recv_cb);
}

void rcp_send(const rcp_cmd_t *cmd) {
  if (_peer_addr == NULL) {
    return;
  }

  ESP_ERROR_CHECK(
      esp_now_send(_peer_addr, (uint8_t const *)cmd, sizeof(rcp_cmd_t)));
}
