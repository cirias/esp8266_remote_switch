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
#include "freertos/timers.h"

#include "driver/uart.h"

/* #include "esp_log.h" */
#include "esp_now.h"
#include "esp_system.h"
#include "esp_vfs_dev.h"
/* #include "esp_wifi.h" */

#include "rcp.h"

static const char *TAG = "remote_switch_master";
static uint8_t peer_mac[ESP_NOW_ETH_ALEN] = {0x5c, 0xcf, 0x7f,
                                             0xb7, 0x22, 0x58};

void app_main(void) {
  printf("SDK version: %s\n", esp_get_idf_version());

  /* Disable buffering on stdin */
  setvbuf(stdin, NULL, _IONBF, 0);

  /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
  esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
  /* Move the caret to the beginning of the next line on '\n' */
  esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

  /* Configure UART. Note that REF_TICK is used so that the baud rate remains
   * correct while APB frequency is changing in light sleep mode.
   */
  uart_config_t uart_config = {
      .baud_rate = CONFIG_CONSOLE_UART_BAUDRATE,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
  };
  ESP_ERROR_CHECK(uart_param_config(CONFIG_CONSOLE_UART_NUM, &uart_config));

  /* Install UART driver for interrupt-driven reads and writes */
  ESP_ERROR_CHECK(
      uart_driver_install(CONFIG_CONSOLE_UART_NUM, 256, 0, 0, NULL));

  /* Tell VFS to use UART driver */
  esp_vfs_dev_uart_use_driver(CONFIG_CONSOLE_UART_NUM);

  rcp_master_init(peer_mac);

  rcp_cmd_t cmd = {.servo_index = 0, .servo_state = 1};

  int state = 0;
  int c = 0;
  int nread = 0;

  printf("ready\n");
  while ((c = getchar()) != EOF) {
    putc(c, stdout);

    if (c == '1') {
      cmd.servo_state = 1;
    } else if (c == '0') {
      cmd.servo_state = 0;
    } else {
      continue;
    }

    printf("sending\n");
    rcp_send(&cmd);
  }
}
