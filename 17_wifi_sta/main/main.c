#include "esp_event.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "my_spi.h"
#include "my_wifi.h"
#include "myiic.h"
#include "nvs_flash.h"
#include "spilcd.h"
#include "xl9555.h"
#include <stdio.h>

void flash_init() {
  esp_err_t ret;
  ret = nvs_flash_init(); /* 初始化NVS */
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
  }
}
/**
 * @brief       程序入口
 * @param       无
 * @retval      无
 */
void app_main(void) {

  flash_init();  /* Flash初始化*/
  led_init();    /* LED初始化 */
  my_spi_init(); /* SPI初始化 */
  myiic_init();  /* IIC初始化 */
  xl9555_init(); /* 初始化按键 */
  spilcd_init(); /* LCD屏初始化 */

  spilcd_show_string(0, 0, 240, 32, 32, "ESP32-S3", RED);
  spilcd_show_string(0, 40, 240, 24, 24, "WiFi STA", RED);

  wifi_sta_init();

  while (1) {
    LED_TOGGLE();
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
