#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "led.h"
#include "lcd.h"
#include "adc.h"
#include "spi.h"
#include <stdio.h>

i2c_obj_t i2c0_master;

void app_main(void)
{
  uint16_t adc_data = 0; // 量程
  float adc_vol = 0;     // 伏特数

  led_init();                        // 初始化 LED
  i2c0_master = iic_init(I2C_NUM_0); // 初始化 I2C0
  spi2_init();                       // 初始化 24CXX EEPROM
  xl9555_init(i2c0_master);          // 初始化 XL9555
  lcd_init();
  adc_init();

  // printf("============ ↓ ============\n");

  // 正常
  while (1)
  {

    // LCD SHOW，默认 dmeo
    lcd_show_string(10, 40, 240, 32, 32, "ESP32", RED);
    lcd_show_string(10, 80, 240, 24, 24, "SPI LCD TEST", RED);
    lcd_show_string(10, 110, 240, 16, 16, "HELLO", RED);
    lcd_show_string(10, 150, 240, 12, 12, "WORLD", RED);
    lcd_show_string(10, 180, 240, 16, 16, "Tang Minjie, where is your 450MT ?", RED);
    lcd_show_string(10, 200, 240, 16, 16, "ADC_VAL:", BLUE);
    lcd_show_string(10, 230, 240, 16, 16, "ADC_VOL: 0.000v ", BLUE);
    while (1)
    {
      adc_data = adc_get_result_average(ADC_CHAN, 10); // 获取 ADC 值

      lcd_show_xnum(74, 200, adc_data, 4, 16, 0, RED);

      adc_vol = (float)adc_data * 3.3 / 4095;          // 量程
      adc_data = (uint16_t)adc_vol;                    // 确整
      lcd_show_xnum(82, 230, adc_data, 1, 16, 0, RED); // 显示电压取整部分

      adc_vol -= adc_data;
      adc_vol *= 1000; // 小数放大1000倍数方便显示
      lcd_show_xnum(98, 230, adc_vol, 3, 16, 0x80, RED);
      LED_TOGGLE();
    }

    vTaskDelay(pdMS_TO_TICKS(500)); // 延时 10ms
  }
}
