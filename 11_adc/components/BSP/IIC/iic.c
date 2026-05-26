#include "iic.h"

i2c_obj_t iic_master[I2C_NUM_MAX];

/**
 * @brief Initialize the I2C master interface
 */
i2c_obj_t iic_init(uint8_t iic_port)
{
  uint8_t i;
  i2c_config_t iic_config_struct = {0};

  // 根据引脚配置数组索引
  if (iic_port == I2C_NUM_0)
  {
    i = 0;
  }
  else
  {
    i = 1;
  }

  iic_master[i].port = iic_port;
  iic_master[i].init_flag = ESP_FAIL; // 默认初始化失败

  if (iic_master[i].port == I2C_NUM_0)
  {
    iic_master[i].scl = IIC0_SCL_GPIO_PIN;
    iic_master[i].sda = IIC0_SDA_GPIO_PIN;
  }
  else
  {
    iic_master[i].scl = IIC1_SCL_GPIO_PIN;
    iic_master[i].sda = IIC1_SDA_GPIO_PIN;
  }

  iic_config_struct.mode = I2C_MODE_MASTER;
  iic_config_struct.sda_io_num = iic_master[i].sda;
  iic_config_struct.scl_io_num = iic_master[i].scl;
  iic_config_struct.sda_pullup_en = GPIO_PULLUP_ENABLE;
  iic_config_struct.scl_pullup_en = GPIO_PULLUP_ENABLE;
  iic_config_struct.master.clk_speed = IIC_FREQ;
  i2c_param_config(iic_master[i].port, &iic_config_struct); // 配置I2C参数

  // 安装 I2C 驱动
  iic_master[i].init_flag = i2c_driver_install(iic_master[i].port,        // 端口
                                               iic_config_struct.mode,    // 主机模式
                                               IIC_MASTER_RX_BUF_DISABLE, // 从机接收缓冲区禁用
                                               IIC_MASTER_TX_BUF_DISABLE, // 从机发送缓冲区禁用
                                               0);

  if (iic_master[i].init_flag != ESP_OK)
  {
    while (1)
    {
      // 初始化失败，进入死循环
      printf("%s,port=%d, ret=%d, initialization failed!\n", __func__, iic_master[i].port, iic_master[i].init_flag);
    }
  }

  return iic_master[i];
}

/**
 * @brief Perform an I2C transfer (read/write) with the specified device address and buffers
 */
esp_err_t i2c_transfer(i2c_obj_t *self, uint16_t addr, size_t n, i2c_buf_t *bufs, unsigned int flags)
{
  int data_len = 0;
  esp_err_t ret = ESP_FAIL;

  i2c_cmd_handle_t cmd = i2c_cmd_link_create(); // 创建 I2C 命令链接

  if (flags & I2C_FLAG_WRITE)
  {
    i2c_master_start(cmd);                                     // 发送起始信号
    i2c_master_write_byte(cmd, (addr << 1), ACK_CHECK_EN);     // 发送设备地址和写标志
    i2c_master_write(cmd, bufs->buf, bufs->len, ACK_CHECK_EN); // 发送数据
    data_len += bufs->len;
    --n;
    ++bufs;
  }

  i2c_master_start(cmd);                                                           // 发送起始信号
  i2c_master_write_byte(cmd, (addr << 1) | (flags & I2C_FLAG_READ), ACK_CHECK_EN); // 发送设备地址和读标志

  for (; n--; ++bufs)
  {
    if (flags & I2C_FLAG_READ)
    {

      i2c_master_read(cmd, bufs->buf, bufs->len, n == 0 ? I2C_MASTER_LAST_NACK : I2C_MASTER_ACK); // 最后一个字节发送 NACK
    }
    else
    {
      if (bufs->len != 0)
      {
        i2c_master_write(cmd, bufs->buf, bufs->len, ACK_CHECK_EN); // 发送数据
      }
    }
    data_len += bufs->len;
  }

  if (flags & I2C_FLAG_STOP)
  {
    i2c_master_stop(cmd); // 发送停止信号
  }

  ret = i2c_master_cmd_begin(self->port, cmd, 100 * (1 + data_len)); // 执行 I2C 命令
  i2c_cmd_link_delete(cmd);                                          // 删除 I2C 命令链接

  return ret;
}