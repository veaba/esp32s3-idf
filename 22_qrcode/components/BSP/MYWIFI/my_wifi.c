#include "my_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_types.h"
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "qrcode.h"
#include "spilcd.h"
#include <stdio.h> // 系统库，支持：printf
#include <stdlib.h>
#include <string.h>
#include <sys/_intsup.h>

static const char *TAG = "AP";
char lcd_buff[100] = {0};

// 像素绘制适配器（用于中文库的回调）
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color) { spilcd_draw_point(x, y, color); }

// ASCII字符绘制适配器（用于中文库的回调）
void draw_ascii_char(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg_color) {
  spilcd_show_char(x, y, (uint8_t)c, 16, 0, color);
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {

  // 设备连接
  if (event_id == WIFI_EVENT_AP_STACONNECTED) {
    spilcd_fill(0, 90, 320, 240, WHITE);
    wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR, "join, AID=%d", MAC2STR(event->mac), event->aid);

    sprintf(lcd_buff, "MACSTR:" MACSTR, MAC2STR(event->mac));
    spilcd_show_string(0, 90, 320, 16, 16, lcd_buff, BLUE);
    spilcd_show_string(0, 110, 320, 16, 16, "with device connection", BLUE);

  } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR, " level, ADI=%d", MAC2STR(event->mac), event->aid);
    spilcd_fill(0, 90, 320, 320, WHITE);
    sprintf(lcd_buff, "device disconnected:" MACSTR, MAC2STR(event->mac));
    spilcd_show_string(0, 90, 320, 16, 16, lcd_buff, BLUE);
  }
}

/** WIFI 初始化 */

void wifi_ap_init(void) {

  // ESP_ERROR_CHECK(esp_netif_init());                // 网卡初始化
  esp_err_t ret = esp_netif_init(); // 网卡初始化
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "esp_netif_init failed: %s", esp_err_to_name(ret));
    return;
  }
  ESP_ERROR_CHECK(esp_event_loop_create_default()); // 事件循环
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // wifi 初始化默认设置
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

  wifi_config_t wifi_config = WIFI_CONFIG(); // 宏定义配置

  if (strlen(DEFAULT_PASSWORD) == 0) {
    wifi_config.ap.authmode = WIFI_AUTH_OPEN; // 无密码则 open
  }
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));                   // 设置 AP 模式
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config)); // 设置配置
  ESP_ERROR_CHECK(esp_wifi_start());                                  // 启动 wifi

  esp_netif_ip_info_t ip_info;

  esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

  char ip_addr[16];

  inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);
  ESP_LOGI(TAG, "setup AP with IP:%s", ip_addr);
  ESP_LOGI(TAG, "wifi_init_ap finished, ssid:'%s' | password: '%s'", DEFAULT_SSID, DEFAULT_PASSWORD);

  qrcode_show_wifi(DEFAULT_SSID, DEFAULT_PASSWORD, 4);
  // qrcode_show_hello_world();
  spilcd_show_string(0, 200, 240, 16, 16, "wifi connecting", BLUE);
}