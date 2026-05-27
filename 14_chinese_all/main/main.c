#include "chinese16.h"
#include "esp_err.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd.h"
#include "led.h"
#include "nvs_flash.h"

i2c_obj_t i2c0_master;

// ASCII字符绘制适配器（用于中文库的回调）
static void draw_ascii_char(uint16_t x, uint16_t y, char c, uint16_t color,
                            uint16_t bg_color) {
  // 使用LCD驱动绘制16号ASCII字符（8x16像素，与16x16汉字配套）
  lcd_show_char(x, y, (uint8_t)c, 16, 0, color);
}

//
void app_main(void) {
  esp_err_t ret;

  ret = nvs_flash_init(); /* 初始化NVS */

  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }

  led_init();                        // 初始化 LED
  i2c0_master = iic_init(I2C_NUM_0); // 初始化 I2C0
  spi2_init();                       // 初始化 SPI2
  xl9555_init(i2c0_master);          // 初始化 XL9555
  lcd_init();

  // 正常
  while (1) {

    // 显示中文（x, y, 字符串, 前景色, 背景色, 像素绘制回调, ASCII绘制回调）
    show_chinese_string(10, 10, "两万零九百零二全中文字库", RED, WHITE,
                        lcd_draw_pixel, draw_ascii_char);

    // ASCII字符串
    lcd_show_string(10, 80, 240, 32, 32, "ESP32-S3", RED);

    LED_TOGGLE();
    vTaskDelay(pdMS_TO_TICKS(10)); // 延时 10ms
  }
}
