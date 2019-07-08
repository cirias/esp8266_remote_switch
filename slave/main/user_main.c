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

/* #include "freertos/FreeRTOS.h" */
/* #include "freertos/timers.h" */

/* #include "esp_log.h" */
#include "esp_now.h"
#include "esp_system.h"
/* #include "esp_wifi.h" */

#include "driver/pwm.h"

#include "rcp.h"

#define PWM_CHANNEL_NUM 1

// GPIO14 D5
#define PWM_0_OUT_IO_NUM 14

// PWM period 20ms(50hz), same as depth
#define PWM_PERIOD (20000)

#define PWM_STALL_DUTY 2000
#define PWM_ON_DUTY 2400
#define PWM_OFF_DUTY 1600

static const char *TAG = "remote_switch_slave";
static uint8_t peer_mac[ESP_NOW_ETH_ALEN] = {0xa0, 0x20, 0xa6, 0xb, 0x4d, 0x10};

// pwm pin number
const uint32_t pin_num[PWM_CHANNEL_NUM] = {
    PWM_0_OUT_IO_NUM,
};

// duties table, real_duty = duties[x]/PERIOD
uint32_t duties[PWM_CHANNEL_NUM] = {
    PWM_STALL_DUTY,
};

// phase table, delay = (phase[x]/360)*PERIOD
int16_t phase[PWM_CHANNEL_NUM] = {0};

static void rcp_cmd_recv_cb(const rcp_cmd_t *cmd) {
  printf("received cmd: %d, %d\n", cmd->servo_index, cmd->servo_state);

  ESP_ERROR_CHECK(pwm_set_duty(
      cmd->servo_index, cmd->servo_state == 0 ? PWM_OFF_DUTY : PWM_ON_DUTY));
  ESP_ERROR_CHECK(pwm_start());
}

void app_main(void) {
  printf("SDK version: %s\n", esp_get_idf_version());

  ESP_ERROR_CHECK(pwm_init(PWM_PERIOD, duties, PWM_CHANNEL_NUM, pin_num));
  ESP_ERROR_CHECK(pwm_set_phases(phase));
  printf("pwm inited\n");

  rcp_slave_init(peer_mac, rcp_cmd_recv_cb);
}
