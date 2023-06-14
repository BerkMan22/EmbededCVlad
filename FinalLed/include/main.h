#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "time.h"
#include "sdkconfig.h"
#include "freertos/projdefs.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include "esp_rom_gpio.h"
#include "esp_app_trace.h"
#include <freertos/semphr.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "nvs_flash.h"
#include "nvs.h"
#include <inttypes.h>

#define LED_MAX_RESOLUTION 1024
#define LED_PIN 4
#define BOOT_BUTTON_PIN GPIO_NUM_0
#define BUF_SIZE (1024)
#define STORAGE_NAMESPACE "storage"
