#ifndef __IIC_H_
#define __IIC_H_

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"

// IIC control
typedef struct _i2c_obj_t
{
  i2c_port_t port;
  gpio_num_t scl;
  gpio_num_t sda;
  esp_err_t init_flag;
} i2c_obj_t;

// Read buffer struct
typedef struct _i2c_buf_t
{
  size_t len;
  uint8_t *buf;
} i2c_buf_t;

extern i2c_obj_t iic_master[I2C_NUM_MAX];

// 读写标志
#define I2C_FLAG_READ (0x01)
#define I2C_FLAG_STOP (0x02)
#define I2C_FLAG_WRITE (0x04)

// 引脚
#define IIC0_SDA_GPIO_PIN GPIO_NUM_41
#define IIC0_SCL_GPIO_PIN GPIO_NUM_42
#define IIC1_SDA_GPIO_PIN GPIO_NUM_5
#define IIC1_SCL_GPIO_PIN GPIO_NUM_4
#define IIC_FREQ 400000             // 400 kHz
#define IIC_MASTER_TX_BUF_DISABLE 0 // 接收
#define IIC_MASTER_RX_BUF_DISABLE 0 // 发送
#define ACK_CHECK_EN 0x1            // 主机发送ACK

// 函数声明
i2c_obj_t iic_init(uint8_t iic_port);
esp_err_t i2c_transfer(i2c_obj_t *self, uint16_t addr, size_t n, i2c_buf_t *bufs, unsigned int flags);

#endif