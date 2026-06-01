#include "blue_tooth.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "led.h"
#include "nvs_flash.h"

/**
 * @brief       程序入口
 * @param       无
 * @retval      无
 */
void app_main(void) {
  esp_err_t ret;

  led_init(); /* LED初始化 */

  ret = nvs_flash_init(); /* 初始化NVS */
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  blue_tooth_init();
}
