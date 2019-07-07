/* pwm example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "esp8266/gpio_register.h"
#include "esp8266/pin_mux_register.h"

#include "driver/pwm.h"

#define CHANNEL_NUM 1

// GPIO14 D5
#define PWM_0_OUT_IO_NUM 14

// PWM period 20ms(50hz), same as depth
#define PWM_PERIOD (20000)

static const char *TAG = "pwm_example";

// pwm pin number
const uint32_t pin_num[CHANNEL_NUM] = {
    PWM_0_OUT_IO_NUM,
};

// duties table, real_duty = duties[x]/PERIOD
uint32_t duties[CHANNEL_NUM] = {
    700,
};

// phase table, delay = (phase[x]/360)*PERIOD
int16_t phase[CHANNEL_NUM] = {0};

void app_main() {
  ESP_ERROR_CHECK(pwm_init(PWM_PERIOD, duties, CHANNEL_NUM, pin_num));
  ESP_ERROR_CHECK(pwm_set_phases(phase));
  printf("pwm inited\n");

  uint32_t duty = 700;
  while (1) {
    if (duty == 700) {
      printf("on\n");
      duty = 2600;
    } else {
      printf("off\n");
      duty = 700;
    }

    ESP_ERROR_CHECK(pwm_set_duty(0, duty));
    ESP_ERROR_CHECK(pwm_start());

    vTaskDelay(3000 / portTICK_RATE_MS);
  }
}

