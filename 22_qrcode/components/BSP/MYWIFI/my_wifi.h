#ifndef __MY_WIFI_H
#define __MY_WIFI_H

#include <stdint.h>

#define MAX_STA_COUNT 5                                           // 最大 STA（wifi client） 连接数
#define DEFAULT_SSID "esp32-s3-wifi"                              // wifi 名
#define DEFAULT_PASSWORD "123456789"                              // wifi 密码
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5] // mac 地址
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"                    // mac 地址字符

// TODO remove
#define WIFI_CONNECTING_BIT BIT0
#define WIFI_FAIL_BIT BIT1
static const int RETRY_LIMIT = 5; // 最大重试次数

#define WIFI_CONFIG()                                                                                                  \
  {                                                                                                                    \
    .ap = {                                                                                                            \
      .ssid = DEFAULT_SSID,                                                                                            \
      .ssid_len = strlen(DEFAULT_SSID),                                                                                \
      .password = DEFAULT_PASSWORD,                                                                                    \
      .max_connection = MAX_STA_COUNT,                                                                                 \
      .authmode = WIFI_AUTH_WPA_WPA2_PSK,                                                                              \
    }                                                                                                                  \
  }

void wifi_ap_init(void);
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void draw_ascii_char(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg_color);
#endif