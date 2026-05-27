#include "my_wifi.h"
#include "chinese16_3k.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "spilcd.h"
#include <stdio.h> // 系统库，支持：printf
#include <stdlib.h>
#include <string.h>

static const char *TAG = "static_ip";
static EventGroupHandle_t wifi_event;
char lcd_buff[100] = {0};

// 像素绘制适配器（用于中文库的回调）
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color) { spilcd_draw_point(x, y, color); }

// ASCII字符绘制适配器（用于中文库的回调）
void draw_ascii_char(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg_color) {
  spilcd_show_char(x, y, (uint8_t)c, 16, 0, color);
}

// 连接显示
void connect_display(uint8_t flag) {
  // 连接成功
  if (flag == 2) {

    spilcd_fill(0, 90, 320, 240, WHITE);
    sprintf(lcd_buff, "【ssid】:%s", DEFAULT_SSID);
    spilcd_show_string(0, 150, 240, 16, 16, lcd_buff, WHITE);
    sprintf(lcd_buff, "【psw】:%s", DEFAULT_PASSWORD);
    spilcd_show_string(0, 150, 240, 16, 16, lcd_buff, WHITE);
    // 失败
  } else if (flag == 1) {
    spilcd_show_string(0, 150, 240, 16, 16, "wifi connet fail", RED);
  } else {
    // 连接中
    spilcd_show_string(0, 150, 240, 16, 16, "wifi connecting...", BLUE);
  }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  // 重试次数
  static int s_retry_num = 0;

  if (event_base == WIFI_EVENT) {
    switch (event_id) {
      case WIFI_EVENT_STA_START: {
        connect_display(0);
        esp_wifi_connect();
        break;
      }
        // 连接成功
      case WIFI_EVENT_STA_CONNECTED: {
        connect_display(2);
        break;
      }

      case WIFI_EVENT_STA_DISCONNECTED: {

        if (s_retry_num < RETRY_LIMIT) {
          esp_wifi_connect();
          s_retry_num++;
          ESP_LOGI(TAG, "retry to connect to the AP\n");
        } else {
          xEventGroupSetBits(wifi_event, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
        break;
      }
      default:
        break;
    }
  } else if (event_base == IP_EVENT) {
    switch (event_id) {
      // 工作站从连接 AP 获得 IP
      case IP_EVENT_STA_GOT_IP: {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "static ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(wifi_event, WIFI_CONNECTING_BIT);

        show_chinese_string(0, 150, "WIFI 连接成功！", BLUE, WHITE, lcd_draw_pixel, draw_ascii_char);
        break;
      }

      default:
        break;
    }
  }
}

/** WIFI 初始化 */

void wifi_sta_init(void) {
  static esp_netif_t *sta_netif = NULL;
  wifi_event = xEventGroupCreate();

  show_chinese_string(0, 80, "WIFI 实验", BLUE, WHITE, lcd_draw_pixel, draw_ascii_char);
  ESP_ERROR_CHECK(esp_netif_init());                // 网卡初始化
  ESP_ERROR_CHECK(esp_event_loop_create_default()); // 时间循环
  sta_netif = esp_netif_create_default_wifi_sta();
  assert(sta_netif);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // wifi 初始化默认设置
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  wifi_config_t wifi_config = WIFI_CONFIG();
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));                   // 设置 STA 模式
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config)); // 设置配置
  ESP_ERROR_CHECK(esp_wifi_start());                                   // 启动 wifi

  // 等待连接成功后，ip 生成
  EventBits_t bits =
      xEventGroupWaitBits(wifi_event, WIFI_CONNECTING_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

  // 判断连接事件

  if (bits & WIFI_CONNECTING_BIT) {
    ESP_LOGI(TAG, "Connected to ap SSID: %s | password: %s", DEFAULT_SSID, DEFAULT_PASSWORD);
  } else if (bits & WIFI_FAIL_BIT) {
    connect_display(1); // 失败
    ESP_LOGI(TAG, "Failed to connect to SSID: %s | password: %s", DEFAULT_SSID, DEFAULT_PASSWORD);
  } else {
    ESP_LOGI(TAG, "Unexpected event");
  }
}