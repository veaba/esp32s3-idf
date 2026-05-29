#ifndef __MY_WIFI_H
#define __MY_WIFI_H

#include <stdint.h>

#define CONNECTED_BIT BIT0     // 连接配网
#define ESPTOUCH_DONE_BIT BIT1 // 配网结束

#define WIFI_NOTIFY_PORT    7001    // UDP 反馈端口
#define WIFI_NOTIFY_COUNT   3       // UDP 广播次数
#define WIFI_NOTIFY_INTERVAL_MS 500 // UDP 广播间隔(ms)
#define WIFI_RETRY_MAX      5       // WiFi 断线最大重试次数

#define WIFI_NOTIFY_STATUS_SUCCESS    0   // WiFi 连接成功
#define WIFI_NOTIFY_STATUS_FAIL       1   // WiFi 连接失败
#define WIFI_NOTIFY_STATUS_DISCONNECT 2   // 用户主动断开

void wifi_smart_init(void);
void wifi_forget_and_reconfig(void);
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void draw_ascii_char(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg_color);
#endif