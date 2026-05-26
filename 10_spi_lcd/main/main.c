#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "led.h"
#include "lcd.h"
// #include <time.h>

i2c_obj_t i2c0_master;

// #define TEXT_MAX_SIZE 64 // 定义最大长度

// uint8_t g_text_buf[TEXT_MAX_SIZE] = {"ESP32s3 LCD"};

// void display_info(void)
// {

//   printf("\n");
//   printf(" ******************************** \n");
//   printf("LCD esp32s3 \n");
// }

// 格式化当前时间到字符串
// void get_current_time_str(char *time_str, size_t len)
// {
//   time_t now = time(NULL);
//   struct tm *tm_info = localtime(&now);
//   strftime(time_str, len, "%Y-%m-%d %H:%M:%S", tm_info);
// }
void app_main(void)
{
  uint8_t x = 0;
  esp_err_t ret;

  ret = nvs_flash_init(); /* 初始化NVS */

  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }

  led_init();                        // 初始化 LED
  i2c0_master = iic_init(I2C_NUM_0); // 初始化 I2C0
  spi2_init();                       // 初始化 24CXX EEPROM
  xl9555_init(i2c0_master);          // 初始化 XL9555
  lcd_init();
  // display_info();

  // printf("============ ↓ ============\n");

  // 正常
  while (1)
  {

    switch (x)
    {
    case 0:
    {
      lcd_clear(WHITE);
      break;
    }
    case 1:
    {
      lcd_clear(BLACK);
      break;
    }

    case 2:
    {
      lcd_clear(RED);
      break;
    }

    case 3:
    {
      lcd_clear(GREEN);
      break;
    }
    case 4:
    {
      lcd_clear(BLUE);
      break;
    }
    case 5:
    {
      lcd_clear(MAGENTA);
      break;
    }
    case 6:
    {
      lcd_clear(YELLOW);
      break;
    }
    case 7:
    {
      lcd_clear(CYAN);
      break;
    }
    default:
    {
      break;
    }
    }

    // LCD SHOW，默认 dmeo
    // lcd_show_string(10, 40, 240, 32, 32, "ESP32", RED);
    // lcd_show_string(10, 80, 240, 24, 24, "SPI LCD TEST", RED);
    // lcd_show_string(10, 110, 240, 16, 16, "HELLO", RED);
    // lcd_show_string(10, 150, 240, 12, 12, "WORLD", RED);
    // lcd_show_string(10, 180, 240, 16, 16, "Tang Minjie, where is your 450MT ?", RED);

    // 改进换行
    TextBox box;
    // 初始化文本框：从(10,10)开始，宽300像素，高200像素，字体16px
    text_box_init(&box, 10, 10, 240, 320, 16);
    // 设置颜色（可选）
    text_box_set_color(&box, RED);

    // 方式1：直接打印（自动换行）
    text_box_print(&box, "Tang Minjie, where is your:", RED);
    box.y += 30;
    box.font_size = 24;
    text_box_print(&box, "CF 450 MT", RED);

    x++;

    if (x == 12)
    {
      x = 0;
    }

    LED_TOGGLE();
    vTaskDelay(pdMS_TO_TICKS(2)); // 延时 10ms
  }
}
