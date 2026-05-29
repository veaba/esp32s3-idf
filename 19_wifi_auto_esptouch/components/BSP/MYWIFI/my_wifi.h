#ifndef __MY_WIFI_H
#define __MY_WIFI_H

#include <stdint.h>

#define DEFAULT_SCAN_LIST_SIZE 12

#define DEFAULT_SSID "iPhone17"
#define DEFAULT_PASSWORD "123456789"
#define CONNECTED_BIT BIT0     // 连接配网
#define ESPTOUCH_DONE_BIT BIT1 // 配网结束

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