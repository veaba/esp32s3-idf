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

static const char *TAG = "scan";

// 像素绘制适配器（用于中文库的回调）
static void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
  spilcd_draw_point(x, y, color);
}

// ASCII字符绘制适配器（用于中文库的回调）
static void draw_ascii_char(uint16_t x, uint16_t y, char c, uint16_t color,
                            uint16_t bg_color) {
  spilcd_show_char(x, y, (uint8_t)c, 16, 0, color);
}

static void print_auth_mode(int authmode) {
  switch (authmode) {
  case WIFI_AUTH_OPEN:
    ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_OPEN");
    break;
  case WIFI_AUTH_OWE:
    ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_OWE");
    break;
  case WIFI_AUTH_WEP:
    ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WEP");
    break;
  case WIFI_AUTH_WPA_PSK:
    ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA_PSK");
    break;
  case WIFI_AUTH_WPA2_PSK:
    ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_PSK");
    break;
  case WIFI_AUTH_WPA_WPA2_PSK:
    ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA_WPA2_PSK");
    break;
  case WIFI_AUTH_WPA2_ENTERPRISE:
    ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_ENTERPRISE");
    break;
  case WIFI_AUTH_WPA3_PSK:
    ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA3_PSK");
    break;
  case WIFI_AUTH_WPA2_WPA3_PSK:
    ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_WPA3_PSK");
    break;
  default:
    ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_UNKNOWN");
    break;
  }
}

static void print_cipher_type(int pairwise_cipher, int group_cipher) {
  switch (pairwise_cipher) {
  case WIFI_CIPHER_TYPE_NONE:
    ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_NONE");
    break;
  case WIFI_CIPHER_TYPE_WEP40:
    ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP40");
    break;
  case WIFI_CIPHER_TYPE_WEP104:
    ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP104");
    break;
  case WIFI_CIPHER_TYPE_TKIP:
    ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP");
    break;
  case WIFI_CIPHER_TYPE_CCMP:
    ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_CCMP");
    break;
  case WIFI_CIPHER_TYPE_TKIP_CCMP:
    ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
    break;
  default:
    ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
    break;
  }

  switch (group_cipher) {
  case WIFI_CIPHER_TYPE_NONE:
    ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_NONE");
    break;
  case WIFI_CIPHER_TYPE_WEP40:
    ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP40");
    break;
  case WIFI_CIPHER_TYPE_WEP104:
    ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP104");
    break;
  case WIFI_CIPHER_TYPE_TKIP:
    ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP");
    break;
  case WIFI_CIPHER_TYPE_CCMP:
    ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_CCMP");
    break;
  case WIFI_CIPHER_TYPE_TKIP_CCMP:
    ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
    break;
  default:
    ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
    break;
  }
}

void wifi_scan(void) {
  printf("Let us start scan wifi.\n");

  char lcd_buff[100] = {0};
  ESP_ERROR_CHECK(esp_netif_init());                // 初始化网卡, esp_wifi
  ESP_ERROR_CHECK(esp_event_loop_create_default()); // esp_event
  esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta(); // STA
  assert(sta_netif);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // wifi init
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  uint16_t number = DEFAULT_SCAN_LIST_SIZE;
  wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
  uint16_t ap_count = 0;
  memset(ap_info, 0, sizeof(ap_info));

  /* 设置WIFI为STA模式 */
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

  /* 启动WIFI */
  ESP_ERROR_CHECK(esp_wifi_start());
  vTaskDelay(pdMS_TO_TICKS(100)); // ✅ 等待WiFi完全启动
  /* 开始扫描附件的WIFI */
  esp_err_t ret = esp_wifi_scan_start(NULL, true);
  ESP_LOGI(TAG, "esp_wifi_scan_start: %s", esp_err_to_name(ret));
  if (ret != ESP_OK) {
    ESP_LOGI(TAG, "Scan failed, check WiFi config");
    return;
  }
  /* 获取上次扫描中找到的AP数量 */
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

  /* 获取上次扫描中找到的AP列表 */
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));

  ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);
  ESP_LOGI(TAG, "扫描数量= %d\n", DEFAULT_SCAN_LIST_SIZE);

  /* 下面是打印附件的WIFI信息 */
  for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
    ESP_LOGI(TAG, "==============%d===========", i);

    sprintf(lcd_buff, "scan ssid=%s", ap_info[i].ssid);

    show_chinese_string(150, 20 * i, lcd_buff, BLUE, WHITE, lcd_draw_pixel,
                        draw_ascii_char);

    ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
    ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
    print_auth_mode(ap_info[i].authmode);

    if (ap_info[i].authmode != WIFI_AUTH_WEP) {
      print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
    }

    ESP_LOGI(TAG, "Channel \t\t%d\n", ap_info[i].primary);
  }
}