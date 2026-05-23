#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "led.h"
#include "iic.h"
#include "xl9555.h"
#include "24cxx.h"
#include <time.h>

i2c_obj_t i2c0_master;

#define TEXT_MAX_SIZE 64 // 定义最大长度

uint8_t g_text_buf[TEXT_MAX_SIZE] = {"ESP32s3 EEPROM"};

void display_info(void)
{

  printf("\n");
  printf(" ******************************** \n");
  printf("GBC-ESP32 Mini\n");
}

// 格式化当前时间到字符串
void get_current_time_str(char *time_str, size_t len)
{
  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);
  strftime(time_str, len, "%Y-%m-%d %H:%M:%S", tm_info);
}
void app_main(void)
{

  uint16_t i = 0;
  uint8_t err;
  uint8_t key;
  uint8_t datatemp[TEXT_MAX_SIZE];

  led_init();                        // 初始化 LED
  i2c0_master = iic_init(I2C_NUM_0); // 初始化 I2C0
  xl9555_init(i2c0_master);          // 初始化 XL9555
  at24cxx_init(i2c0_master);         // 初始化 24CXX EEPROM
  display_info();
  err = at24cxx_check(); // 检测 24CXX 是否存在

  // 错误
  if (err != 0)
  {
    while (1)
    {
      printf("24cxx check failed ,please check the connection!\n");
      vTaskDelay(pdMS_TO_TICKS(500)); // 延时 100ms
      LED_TOGGLE();                   // LED 翻转
    }
  }

  printf("24CXX Ready!\n");
  printf("============ ↓ ============\n");

  // 正常
  while (1)
  {

    key = xl9555_key_scan(0); // 扫描按键状态
    switch (key)
    {
    // key 3,将 g_text_buf  写
    case KEY3_PRES:
    {
      snprintf((char *)g_text_buf, TEXT_MAX_SIZE, "ESP32s3 EEPROM");
      printf("[write] key3 pressed, the data write is: %s\n", g_text_buf);
      at24cxx_write(0, (uint8_t *)g_text_buf, strlen((char *)g_text_buf) + 1);
      vTaskDelay(10);
      break;
    }

    // key1 写入时间戳
    case KEY1_PRES:
    {
      char time_str[32];
      get_current_time_str(time_str, sizeof(time_str)); // 获取当前时间

      snprintf((char *)g_text_buf, TEXT_MAX_SIZE, "dynamic add system time： %s", time_str);

      printf("[write] key1 pressed, the data write is: %s\n", g_text_buf);
      at24cxx_write(0, (uint8_t *)g_text_buf, strlen((char *)g_text_buf) + 1);
      vTaskDelay(10);
      break;
    }
    case KEY0_PRES:
    {
      at24cxx_read(0, datatemp, TEXT_MAX_SIZE); // 读取 EEPROM 中的数据
      printf("[read] key0 pressed, the datatemp read is: %s\n", datatemp);
      break;
    }
    default:
    {
      break;
    }
    }

    i++;
    if (i == 20)
    {
      LED_TOGGLE(); // LED 翻转
      i = 0;
    }

    vTaskDelay(pdMS_TO_TICKS(10)); // 延时 100ms
  }
}
