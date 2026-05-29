#include "my_wifi.h"
#include "chinese16_3k.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "spilcd.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <inttypes.h>

static const char *TAG = "【smartconfig_task】";
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static char s_ssid[33] = {0};
static void smartconfig_task(void *parm);
static char lcd_buff[100] = {0};

void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color) { spilcd_draw_point(x, y, color); }

void draw_ascii_char(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg_color) {
  spilcd_show_char(x, y, (uint8_t)c, 16, 0, color);
}

static void udp_notify_send(int status, const char *ip, uint8_t reason) {
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  char json[256] = {0};
  if (status == 0) {
    snprintf(json, sizeof(json),
             "{\"status\":0,\"ip\":\"%s\",\"mac\":\"%02x:%02x:%02x:%02x:%02x:%02x\",\"ssid\":\"%s\"}",
             ip, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], s_ssid);
  } else {
    snprintf(json, sizeof(json),
             "{\"status\":%d,\"reason\":%d,\"ssid\":\"%s\"}",
             status, reason, s_ssid);
  }

  ESP_LOGI(TAG, "UDP notify: %s", json);

  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    ESP_LOGE(TAG, "UDP socket create failed");
    return;
  }

  int broadcast_enable = 1;
  setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));

  struct sockaddr_in dest_addr = {0};
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(WIFI_NOTIFY_PORT);
  dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

  for (int i = 0; i < WIFI_NOTIFY_COUNT; i++) {
    sendto(sock, json, strlen(json), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    vTaskDelay(pdMS_TO_TICKS(WIFI_NOTIFY_INTERVAL_MS));
  }

  closesocket(sock);
}

static void udp_notify_task(void *parm) {
  int val = (int)(intptr_t)parm;
  int status = val >> 8;
  uint8_t reason = (uint8_t)(val & 0xFF);

  if (status == 0) {
    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
      char ip_str[16] = {0};
      snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
      udp_notify_send(0, ip_str, 0);
    }
  } else {
    udp_notify_send(status, "", reason);
  }

  vTaskDelete(NULL);
}

static void wifi_start_udp_notify(int status, uint8_t reason) {
  int parm = (status << 8) | reason;
  xTaskCreate(udp_notify_task, "udp_notify", 4096, (void *)parm, 3, NULL);
}

void connect_display(uint8_t flag) {
  if (flag == 2) {
    spilcd_fill(0, 110, 320, 240, WHITE);
    sprintf(lcd_buff, "SSID:%s", s_ssid);
    spilcd_show_string(10, 150, 320, 16, 16, lcd_buff, WHITE);
    spilcd_show_string(10, 170, 320, 16, 16, "WiFi connected!", WHITE);
  } else if (flag == 1) {
    spilcd_show_string(10, 150, 320, 16, 16, "wifi connect fail", RED);
  } else {
    spilcd_show_string(10, 150, 320, 16, 16, "wifi connecting...", BLUE);
  }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {

  if (event_base == WIFI_EVENT) {
    switch (event_id) {
    case WIFI_EVENT_STA_START: {
      connect_display(0);
      xTaskCreate(smartconfig_task, "smartconfig_task", 4096, NULL, 3, NULL);
      break;
    }
    case WIFI_EVENT_STA_DISCONNECTED: {
      s_retry_num++;
      if (s_retry_num > WIFI_RETRY_MAX) {
        ESP_LOGE(TAG, "WiFi connect failed after %d retries", WIFI_RETRY_MAX);
        connect_display(1);
        wifi_start_udp_notify(1, ((wifi_event_sta_disconnected_t *)event_data)->reason);
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
      } else {
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
        ESP_LOGI(TAG, "retry %d/%d to connect AP", s_retry_num, WIFI_RETRY_MAX);
      }
      break;
    }
    case WIFI_EVENT_STA_CONNECTED: {
      s_retry_num = 0;
      break;
    }
    default:
      break;
    }
  } else if (event_base == IP_EVENT) {
    switch (event_id) {
    case IP_EVENT_STA_GOT_IP: {
      ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
      ESP_LOGI(TAG, "static ip:" IPSTR, IP2STR(&event->ip_info.ip));
      xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
      show_chinese_string(0, 150, "WIFI 连接成功！", BLUE, WHITE, lcd_draw_pixel, draw_ascii_char);
      connect_display(2);
      wifi_start_udp_notify(0, 0);
      break;
    }
    default:
      break;
    }
  } else if (event_base == SC_EVENT) {
    switch (event_id) {
    case SC_EVENT_SCAN_DONE:
      ESP_LOGI(TAG, "scan done.");
      spilcd_show_string(10, 110, 320, 16, 16, "In the distribution network...", BLUE);
      break;

    case SC_EVENT_GOT_SSID_PSWD: {
      ESP_LOGI(TAG, "Got SSID and password");
      smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
      wifi_config_t wifi_config;
      uint8_t password[65] = {0};
      uint8_t rvd_data[65] = {0};

      bzero(&wifi_config, sizeof(wifi_config_t));
      memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
      memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
      wifi_config.sta.bssid_set = evt->bssid_set;

      if (wifi_config.sta.bssid_set == true) {
        memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
      }

      memcpy(s_ssid, evt->ssid, sizeof(evt->ssid));
      memcpy(password, evt->password, sizeof(evt->password));
      ESP_LOGI(TAG, "SSID:%s", s_ssid);
      ESP_LOGI(TAG, "PASSWORD:%s", password);

      if (strlen((char *)password) == 0) {
        ESP_LOGE(TAG, "Password is empty! APP v2 encoding error");
        spilcd_fill(0, 110, 320, 240, WHITE);
        spilcd_show_string(10, 110, 320, 16, 16, "Password is empty!", RED);
        spilcd_show_string(10, 130, 320, 16, 16, "Check APP encoding", RED);
        break;
      }

      spilcd_fill(0, 110, 320, 240, WHITE);
      sprintf(lcd_buff, "SSID:%s", s_ssid);
      spilcd_show_string(10, 110, 320, 16, 16, lcd_buff, BLUE);
      spilcd_show_string(10, 130, 320, 16, 16, "WiFi connecting...", BLUE);

      if (evt->type == SC_TYPE_ESPTOUCH_V2) {
        ESP_ERROR_CHECK(esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)));
        ESP_LOGI(TAG, "RVD_DATA");
        for (int i = 0; i < 32; i++) {
          printf("%02x", rvd_data[i]);
        }
        printf("\n");
      }

      s_retry_num = 0;
      ESP_ERROR_CHECK(esp_wifi_disconnect());
      ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
      esp_wifi_connect();
      break;
    }

    case SC_EVENT_SEND_ACK_DONE: {
      xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
      break;
    }
    }
  }
}

static void smartconfig_task(void *parm) {
  EventBits_t uxBits;
  ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_V2));
  smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));

  while (1) {
    uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);

    if (uxBits & CONNECTED_BIT) {
      ESP_LOGI(TAG, "Wifi connected to AP");
    }

    if (uxBits & ESPTOUCH_DONE_BIT) {
      ESP_LOGI(TAG, "smartconfig over");
      esp_smartconfig_stop();
      vTaskDelete(NULL);
    }
  }
}

void wifi_smart_init(void) {

  show_chinese_string(0, 80, "WIFI 实验", BLUE, WHITE, lcd_draw_pixel, draw_ascii_char);
  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
  assert(sta_netif);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());

  EventBits_t bits =
      xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

  if (bits & CONNECTED_BIT) {
    ESP_LOGI(TAG, "Connected to ap SSID: %s", s_ssid);
  } else if (bits & ESPTOUCH_DONE_BIT) {
    connect_display(1);
    ESP_LOGI(TAG, "Failed to connect to SSID: %s", s_ssid);
  } else {
    ESP_LOGI(TAG, "Unexpected event");
  }
}