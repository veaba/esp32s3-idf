#ifndef __XL9555_H_
#define __XL9555_H_

#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "iic.h"

#define XL9555_INT_IO GPIO_NUM_40
#define XL9555_INT gpio_get_level(XL9555_INT_IO) // 读取 XL9555_INT 电平

#define XL9555_INPUT_PORT0_REG 0     // 输入端口0寄存器地址
#define XL9555_INPUT_PORT1_REG 1     // 输入端口1寄存器地址
#define XL9555_OUTPUT_PORT0_REG 2    // 输出端口0寄存器地址
#define XL9555_OUTPUT_PORT1_REG 3    // 输出端口1寄存器地址
#define XL9555_INVERSION_PORT0_REG 4 // 输入极性反转端口0寄存器地址
#define XL9555_INVERSION_PORT1_REG 5 // 输入极性反转端口1寄存器地址
#define XL9555_CONFIG_PORT0_REG 6    // 配置端口0寄存器地址
#define XL9555_CONFIG_PORT1_REG 7    // 配置端口1寄存器地址

#define XL9555_ADDR 0x20 // XL9555 I2C 地址，左移1位

#define AP_INT_IO 0x0001
#define QMA_INT_IO 0x0002
#define SPK_EN_IO 0x0004
#define BEEP_IO 0x0008
#define OV_PWDN_IO 0x0010
#define OV_RESET_IO 0x0020
#define GBC_LED_IO 0x0040
#define GBC_KEY_IO 0x0080
#define LCD_BL_IO 0x0100
#define CT_RST_IO 0x0200
#define SLCD_RST_IO 0x0400
#define SLCD_PWR_IO 0x0800
#define KEY3_IO 0x1000
#define KEY2_IO 0x2000
#define KEY1_IO 0x4000
#define KEY0_IO 0x8000

#define KEY0 xl9555_pin_read(KEY0_IO) // KEY0 引脚
#define KEY1 xl9555_pin_read(KEY1_IO) // KEY1 引脚
#define KEY2 xl9555_pin_read(KEY2_IO) // KEY2 引脚
#define KEY3 xl9555_pin_read(KEY3_IO) // KEY3 引脚

#define KEY0_PRES 1 // KEY0 按下状态
#define KEY1_PRES 2 // KEY1 按下状态
#define KEY2_PRES 3 // KEY2 按下状态
#define KEY3_PRES 4 // KEY3 按下状态

esp_err_t xl9555_read_byte(uint8_t *data, size_t len);
esp_err_t xl9555_write_bytes(uint8_t reg, uint8_t *data, size_t len);
uint16_t xl9555_pin_write(uint16_t pin, int val);
int xl9555_pin_read(uint16_t pin);
uint16_t xl9555_ioconfig(uint16_t config_value);
void xl9555_init(i2c_obj_t self);
uint8_t xl9555_key_scan(uint8_t mode);

#endif