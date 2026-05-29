#ifndef __MY_WIFI_H
#define __MY_WIFI_H

#include <stdint.h>

#define DEFAULT_SCAN_LIST_SIZE 12

#define DEFAULT_SSID "iPhone17"
#define DEFAULT_PASSWORD "123456789"
#define CONNECTED_BIT BIT0     // 连接配网
#define ESPTOUCH_DONE_BIT BIT1 // 配网结束

#define WIFI_NOTIFY_PORT    7001    // UDP 反馈端口
#define WIFI_NOTIFY_COUNT   3       // UDP 广播次数
#define WIFI_NOTIFY_INTERVAL_MS 500 // UDP 广播间隔(ms)
#define WIFI_RETRY_MAX      5       // WiFi 断线最大重试次数

#define WIFI_CONFIG()                                                                                                  \
  {                                                                                                                    \
    .sta = {                                                                                                           \
      .ssid = DEFAULT_SSID,                                                                                            \
      .password = DEFAULT_PASSWORD,                                                                                    \
      .threshold.authmode = WIFI_AUTH_WPA2_PSK,                                                                        \
    }                                                                                                                  \
  }

void wifi_smart_init(void);
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void draw_ascii_char(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg_color);
#endif