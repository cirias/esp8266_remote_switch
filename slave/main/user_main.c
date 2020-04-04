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
//
// ESP01M
// mac: d8:f1:5b:b9:9f:7e

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "esp_log.h"
#include "esp_now.h"
#include "esp_sleep.h"
#include "esp_system.h"
/* #include "esp_wifi.h" */

#include "driver/gpio.h"
#include "driver/pwm.h"

#include "rcp.h"

#define SLEEP_DURATION_US 3000000

#define PWM_CHANNEL_NUM 2

// GPIO14 D5
#define PWM_0_OUT_IO_NUM 14
// GPIO12 D6
#define PWM_1_OUT_IO_NUM 12

// PWM period 20ms(50hz), same as depth
#define PWM_PERIOD (20000)

#define PWM_0_STALL_DUTY 2075
#define PWM_1_STALL_DUTY 2000

#define PWM_0_ON_DUTY 1675
#define PWM_1_ON_DUTY 2400

#define PWM_0_OFF_DUTY 2475
#define PWM_1_OFF_DUTY 1600

#define GPIO_OUTPUT_IO_LED 2
#define GPIO_OUTPUT_PIN_SEL ((1 << GPIO_OUTPUT_IO_LED))

void system_deep_sleep_instant(uint32_t us);

static const char *TAG = "remote_switch_slave";
static uint8_t peer_mac[ESP_NOW_ETH_ALEN] = {0xa0, 0x20, 0xa6, 0xb, 0x4d, 0x10};

// pwm pin number
const uint32_t pin_num[PWM_CHANNEL_NUM] = {
    PWM_0_OUT_IO_NUM,
    PWM_1_OUT_IO_NUM,
};

// duties table, real_duty = duties[x]/PERIOD
uint32_t duties[PWM_CHANNEL_NUM] = {PWM_0_STALL_DUTY, PWM_1_STALL_DUTY};

// phase table, delay = (phase[x]/360)*PERIOD
int16_t phase[PWM_CHANNEL_NUM] = {0, 0};

static TaskHandle_t sleep_task_handle = NULL;
static QueueHandle_t command_queue = NULL;

static void gpio_init(void) {
  gpio_config_t io_conf;
  // disable interrupt
  io_conf.intr_type = GPIO_INTR_DISABLE;
  // set as output mode
  io_conf.mode = GPIO_MODE_OUTPUT;
  // bit mask of the pins that you want to set,e.g.GPIO15/16
  io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
  // disable pull-down mode
  io_conf.pull_down_en = 0;
  // disable pull-up mode
  io_conf.pull_up_en = 0;
  // configure GPIO with the given settings
  ESP_ERROR_CHECK(gpio_config(&io_conf));
}

static void rcp_cmd_recv_cb(const rcp_cmd_t *cmd) {
  printf("received cmd: %d, %d\n", cmd->op0, cmd->op1);

  if (xQueueOverwrite(command_queue, cmd) != pdTRUE) {
    ESP_LOGE(TAG, "could not write command_queue");
  }
}

static void sleep_task(void *p) {
  rcp_cmd_t cmd;

  if (xQueueReceive(command_queue, &cmd, 3000 / portTICK_PERIOD_MS) == pdTRUE) {
    // move to position
    if (cmd.op0 != OP_NO) {
      ESP_ERROR_CHECK(
          pwm_set_duty(0, cmd.op0 == OP_OFF ? PWM_0_OFF_DUTY : PWM_0_ON_DUTY));
    }
    if (cmd.op1 != OP_NO) {
      ESP_ERROR_CHECK(
          pwm_set_duty(1, cmd.op1 == OP_OFF ? PWM_1_OFF_DUTY : PWM_1_ON_DUTY));
    }
    ESP_ERROR_CHECK(pwm_start());
    vTaskDelay(500 / portTICK_PERIOD_MS);

    // move back
    if (cmd.op0 != OP_NO) {
      ESP_ERROR_CHECK(pwm_set_duty(0, PWM_0_STALL_DUTY));
    }
    if (cmd.op1 != OP_NO) {
      ESP_ERROR_CHECK(pwm_set_duty(1, PWM_1_STALL_DUTY));
    }
    ESP_ERROR_CHECK(pwm_start());
    vTaskDelay(500 / portTICK_PERIOD_MS);

    ESP_ERROR_CHECK(
        pwm_stop(0));  // put all output pins to low level after stop
  }

  system_deep_sleep_instant(SLEEP_DURATION_US);
}

void app_main(void) {
  printf("SDK version: %s\n", esp_get_idf_version());

  // doc says put this before esp_wifi_init and esp_deep_sleep
  // 2 means "Radio calibration will not be done after the deep-sleep wakeup"
  // https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/api-reference/system/sleep_modes.html
  esp_deep_sleep_set_rf_option(2);

  gpio_init();
  printf("gpio inited\n");

  ESP_ERROR_CHECK(pwm_init(PWM_PERIOD, duties, PWM_CHANNEL_NUM, pin_num));
  ESP_ERROR_CHECK(pwm_set_phases(phase));
  printf("pwm inited\n");

  command_queue = xQueueCreate(1, sizeof(rcp_cmd_t));

  rcp_slave_init(peer_mac, rcp_cmd_recv_cb);
  printf("rcp inited\n");

  ESP_ERROR_CHECK(gpio_set_level(GPIO_OUTPUT_IO_LED, 0));

  xTaskCreate(sleep_task, "sleep_task", 2048, NULL, 12, &sleep_task_handle);
}
