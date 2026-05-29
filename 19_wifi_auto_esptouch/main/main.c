#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "key.h"
#include "led.h"
#include "my_spi.h"
#include "my_wifi.h"
#include "myiic.h"
#include "nvs_flash.h"
#include "spilcd.h"
#include "xl9555.h"

static const char *TAG = "main";

static volatile bool s_key_boot_pressed = false;

static void IRAM_ATTR keyBoot_isr_handler(void *arg) { s_key_boot_pressed = true; }

void flash_init(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
  }
}

void app_main(void) {
  flash_init();
  led_init();
  my_spi_init();
  myiic_init();
  xl9555_init();
  spilcd_init();

  spilcd_show_string(0, 0, 320, 32, 32, "ESP32-S3", RED);
  spilcd_show_string(0, 40, 320, 24, 24, "WiFi Smart Config Network", RED);

  key_init();
  gpio_set_direction(BOOT_GPIO_PIN, GPIO_MODE_INPUT);
  gpio_set_intr_type(BOOT_GPIO_PIN, GPIO_INTR_NEGEDGE);
  gpio_install_isr_service(0);
  gpio_isr_handler_add(BOOT_GPIO_PIN, keyBoot_isr_handler, NULL);

  wifi_smart_init();

  uint8_t key;
  while (1) {

    key = key_scan(0); // 扫描按键状态

    if (key == BOOT_PRES) {

      wifi_forget_and_reconfig();
      ESP_LOGE(TAG, "wifi disconnect failed: %d", key == BOOT_PRES);
      vTaskDelay(pdMS_TO_TICKS(50));
    }

    LED_TOGGLE();
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}