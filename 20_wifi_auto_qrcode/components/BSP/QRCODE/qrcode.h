#ifndef __QRCODE_H__
#define __QRCODE_H__

#include <stdint.h>

#define QRCODE_VERSION_MAX 6
#define QRCODE_SIZE_MAX 45

typedef struct {
  uint8_t modules[QRCODE_SIZE_MAX][QRCODE_SIZE_MAX];
  uint8_t width;
} QRCode;

typedef enum { QR_ECLEVEL_L = 0, QR_ECLEVEL_M = 1, QR_ECLEVEL_Q = 2, QR_ECLEVEL_H = 3 } QRErrorCorrectLevel;

typedef enum { QR_MODE_NUL = -1, QR_MODE_NUM = 0, QR_MODE_AN = 1, QR_MODE_8 = 2, QR_MODE_KAN = 3 } QRMode;

void qrcode_init(void);

int qrcode_encode_string(const char *data, QRErrorCorrectLevel ecl, QRCode *outQr);

int qrcode_get_module(int row, int col);

void qrcode_show_wifi(const char *ssid, const char *password, uint8_t module_size);
void draw_qrcode(QRCode *qr, uint16_t x, uint16_t y, uint8_t module_size);
void qrcode_show_hello_world(void);

#endif