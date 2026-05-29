#include "my_wifi.h"
#include "chinese16_3k.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif_types.h"
#include "esp_smartconfig.h"
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#include "spilcd.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h> // 系统库，支持：printf
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static const char *TAG = "【smartconfig_task】";
static EventGroupHandle_t s_wifi_event_group;
static void smartconfig_task(void *parm);
static char lcd_buff[100] = {0};

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

    spilcd_fill(0, 110, 320, 240, WHITE);
    sprintf(lcd_buff, "【ssid】:%s", DEFAULT_SSID);
    spilcd_show_string(0, 150, 240, 16, 16, lcd_buff, WHITE);
    sprintf(lcd_buff, "【psw】:%s", DEFAULT_PASSWORD);
    spilcd_show_string(0, 150, 240, 16, 16, lcd_buff, WHITE);
    // 失败
  } else if (flag == 1) {
    spilcd_show_string(0, 150, 240, 16, 16, "wifi connect fail", RED);
  } else {
    // 连接中
    spilcd_show_string(0, 150, 240, 16, 16, "wifi connecting...", BLUE);
  }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {

  if (event_base == WIFI_EVENT) {
    switch (event_id) {
      // STA 模式开始
      case WIFI_EVENT_STA_START: {
        connect_display(0);
        xTaskCreate(smartconfig_task, "smartconfig_task", 4096, NULL, 3, NULL);
        break;
      }
        // 断开
      case WIFI_EVENT_STA_DISCONNECTED: {

        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
        ESP_LOGI(TAG, "connect to the AP fail");
        break;
      }
        // 连接成功
      case WIFI_EVENT_STA_CONNECTED: {
        connect_display(2);
        break;
      }

      default:
        break;
    }
  } else if (event_base == IP_EVENT) {
    switch (event_id) {
      // 成功，工作站从连接 AP 获得 IP
      case IP_EVENT_STA_GOT_IP: {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "static ip:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
        show_chinese_string(0, 150, "WIFI 连接成功！", BLUE, WHITE, lcd_draw_pixel, draw_ascii_char);
        break;
      }

      default:
        break;
    }
  } else if (event_base == SC_EVENT) {

    switch (event_id) {

      // 扫描结束
      case SC_EVENT_SCAN_DONE:
        ESP_LOGI(TAG, "scan done.");
        spilcd_show_string(10, 110, 320, 16, 16, "In the distribution network...", BLUE);
        break;

        // 查找 channel
      case SC_EVENT_FOUND_CHANNEL: {

        ESP_LOGI(TAG, "Got SSID adn password");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;
        uint8_t ssid[33] = {0};
        uint8_t password[65] = {0};
        uint8_t rvd_data[65] = {0};

        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;

        if (wifi_config.sta.bssid_set == true) {
          memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));
        ESP_LOGI(TAG, "SSID:%s", ssid);
        ESP_LOGI(TAG, "PASSWORD:%s", password);

        spilcd_fill(0, 110, 320, 240, WHITE);
        sprintf(lcd_buff, "%s", ssid);
        spilcd_show_string(10, 110, 320, 16, 16, lcd_buff, BLUE);
        sprintf(lcd_buff, "%s", password);
        spilcd_show_string(10, 110, 320, 16, 16, lcd_buff, BLUE);
        spilcd_show_string(10, 130, 320, 16, 16, "Distribution network is successful.", BLUE);

        // 手机 APP 通过 EspTouch v2 模式，执行配网
        if (evt->type == SC_TYPE_ESPTOUCH_V2) {
          ESP_ERROR_CHECK(esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)));
          ESP_LOGI(TAG, "RVD_DATA");

          for (int i = 0; i < 32; i++) {
            printf("%02x", rvd_data[i]);
          }
          printf("\n");
        }

        ESP_ERROR_CHECK(esp_wifi_disconnect());                          // 断开链接
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config)); // 重写入密码
        esp_wifi_connect();                                              // 重新链接
        break;
      }

      case SC_EVENT_SEND_ACK_DONE: {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
      }
    }
  }
}

static void smartconfig_task(void *parm) {
  parm = parm;
  EventBits_t uxBits;
  ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));         // 配网协议
  smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT(); // 配网参数
  ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));

  while (1) {
    uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);

    // 配网成功
    if (uxBits & CONNECTED_BIT) {
      ESP_LOGI(TAG, "Wifi connected to AP");
    }

    // 智能配网结束
    if (uxBits & ESPTOUCH_DONE_BIT) {
      ESP_LOGI(TAG, "smartconfig over");
      esp_smartconfig_stop(); // 释放资源+删除配网任务
      vTaskDelete(NULL);
    }
  }
}

/** WIFI 初始化 */
void wifi_smart_init(void) {

  show_chinese_string(0, 80, "WIFI 实验", BLUE, WHITE, lcd_draw_pixel, draw_ascii_char);
  s_wifi_event_group = xEventGroupCreate(); // 创建事件组

  ESP_ERROR_CHECK(esp_netif_init());                // 1. 网卡初始化
  ESP_ERROR_CHECK(esp_event_loop_create_default()); // 2. 事件循环
  esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
  assert(sta_netif);

  // wifi 初始化默认设置 +=
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  // 注册 wifi 事件
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); // 设置 STA 模式
  ESP_ERROR_CHECK(esp_wifi_start());                 // 启动 wifi

  // 等待连接成功后，ip 生成
  EventBits_t bits =
      xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

  // 判断连接事件
  if (bits & CONNECTED_BIT) {
    ESP_LOGI(TAG, "Connected to ap SSID: %s | password: %s", DEFAULT_SSID, DEFAULT_PASSWORD);
  } else if (bits & ESPTOUCH_DONE_BIT) {
    connect_display(1); // 失败
    ESP_LOGI(TAG, "Failed to connect to SSID: %s | password: %s", DEFAULT_SSID, DEFAULT_PASSWORD);
  } else {
    ESP_LOGI(TAG, "Unexpected event");
  }
}