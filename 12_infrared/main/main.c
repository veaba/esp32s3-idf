#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "led.h"
#include "spilcd.h"
#include "myiic.h"
#include "my_spi.h"
#include "xl9555.h"
#include "rmt_nec_rx.h"
#include <stdio.h>

const char *TAG = "rmt_rx";

/**
 * @brief       根据NEC编码解析红外协议并打印指令结果
 * @param       rmt_nec_symbols : 数据帧
 * @param       symbol_num      : 数据帧大小
 * @retval      无
 */
void rmt_rx_scan(rmt_symbol_word_t *rmt_nec_symbols, size_t symbol_num)
{
  uint8_t rmt_data = 0;
  uint8_t tbuf[40];
  char *str = 0;

  printf("\n[old 扫描帧] = %d\n", symbol_num);
  switch (symbol_num) /* 解码RMT接收数据 */
  {
  case 34: /* 正常NEC数据帧 */
  {

    printf("\n[old 解析帧] = %d\n", rmt_nec_parse_frame(rmt_nec_symbols));
    if (rmt_nec_parse_frame(rmt_nec_symbols))
      rmt_data = (s_nec_code_command >> 8);
    {
      switch (rmt_data)
      {
      case 0xBA:
      {
        str = "POWER";
        break;
      }

      case 0xB9:
      {
        str = "UP";
        break;
      }

      case 0xB8:
      {
        str = "ALIENTEK";
        break;
      }

      case 0xBB:
      {
        str = "BACK";
        break;
      }

      case 0xBF:
      {
        str = "PLAY/PAUSE";
        break;
      }

      case 0xBC:
      {
        str = "FORWARD";
        break;
      }

      case 0xF8:
      {
        str = "VOL-";
        break;
      }

      case 0xEA:
      {
        str = "DOWN";
        break;
      }

      case 0xF6:
      {
        str = "VOL+";
        break;
      }

      case 0xE9:
      {
        str = "1";
        break;
      }

      case 0xE6:
      {
        str = "2";
        break;
      }

      case 0xF2:
      {
        str = "3";
        break;
      }

      case 0xF3:
      {
        str = "4";
        break;
      }

      case 0xE7:
      {
        str = "5";
        break;
      }

      case 0xA1:
      {
        str = "6";
        break;
      }

      case 0xF7:
      {
        str = "7";
        break;
      }

      case 0xE3:
      {
        str = "8";
        break;
      }

      case 0xA5:
      {
        str = "9";
        break;
      }

      case 0xBD:
      {
        str = "0";
        break;
      }

      case 0xB5:
      {
        str = "DELETE";
        break;
      }
      }
      spilcd_fill(86, 110, 286, 150, WHITE);
      sprintf((char *)tbuf, "%d", rmt_data);
      spilcd_show_string(86, 110, 200, 16, 16, (char *)tbuf, BLUE);
      sprintf((char *)tbuf, "%s", str);
      spilcd_show_string(86, 130, 200, 16, 16, (char *)tbuf, BLUE);

      ESP_LOGI(TAG, "KEYVAL = %d, Command=%04X", rmt_data, s_nec_code_command);
    }
    break;
  }

  case 2: /* 重复NEC数据帧 */
  {
    if (rmt_nec_parse_frame_repeat(rmt_nec_symbols))
    {
      ESP_LOGI(TAG, "KEYVAL = %d, Command = %04X, repeat", rmt_data, s_nec_code_command);
    }
    break;
  }

  default: /* 未知NEC数据帧 */
  {
    ESP_LOGI(TAG, "Unknown NEC frame, symbol_num=%d", symbol_num);
    for (int i = 0; i < symbol_num && i < 8; i++)
    {
      ESP_LOGI(TAG, "  symbol[%d]: dur0=%d, level0=%d, dur1=%d, level1=%d",
               i, rmt_nec_symbols[i].duration0, rmt_nec_symbols[i].level0,
               rmt_nec_symbols[i].duration1, rmt_nec_symbols[i].level1);
    }
    break;
  }
  }
}

/**
 * @brief       程序入口
 * @param       无
 * @retval      无
 */
void app_main(void)
{
  esp_err_t ret;
  rmt_rx_done_event_data_t rx_data;

  ret = nvs_flash_init(); /* 初始化NVS */
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
  }

  led_init();        /* LED初始化 */
  my_spi_init();     /* SPI初始化 */
  myiic_init();      /* IIC初始化 */
  xl9555_init();     /* 初始化按键 */
  spilcd_init();     /* LCD屏初始化 */
  rmt_nec_rx_init(); /* 红外接收初始化 */

  spilcd_show_string(30, 50, 200, 16, 16, "ESP32-S3", RED);
  spilcd_show_string(30, 70, 200, 16, 16, "RMT RX TEST", RED);
  spilcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
  spilcd_show_string(30, 110, 200, 16, 16, "KEYVAL:", RED);
  spilcd_show_string(30, 130, 200, 16, 16, "SYMBOL:", RED);

  while (1)
  {
    if (xQueueReceive(receive_queue, &rx_data, pdMS_TO_TICKS(1000)) == pdPASS)
    {
      rmt_rx_scan(rx_data.received_symbols, rx_data.num_symbols);                                  /* 解析接收符号并打印结果 */
      ESP_ERROR_CHECK(rmt_receive(rx_channel, raw_symbols, sizeof(raw_symbols), &receive_config)); /* 重新开始接收 */
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
