#include "led.h"
#include "my_spi.h"
#include "myiic.h"
#include "nvs_flash.h"
#include "qrcode.h"
#include "spilcd.h"
#include "xl9555.h"

void flash_init() {
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

  qrcode_show_hello_world();
  while (1) {
    LED_TOGGLE();
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
