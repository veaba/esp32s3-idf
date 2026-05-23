

#include "24cxx.h"

i2c_obj_t at24cxx_master;

void at24cxx_init(i2c_obj_t self)
{
  at24cxx_master = self;
  if (self.init_flag == ESP_FAIL)
  {
    ESP_LOGE("24CXX", "I2C initialization failed");
    iic_init(I2C_NUM_0); // 初始化 IIC
  }
}

uint8_t at24cxx_read_one_byte(uint16_t addr)
{
  uint8_t data = 0;

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);

  // 根据 24cxx 做匹配

  if (EE_TYPE > AT24C16)
  {
    i2c_master_write_byte(cmd, (AT_ADDR << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN); // send write
    i2c_master_write_byte(cmd, addr >> 8, ACK_CHECK_EN);                         // 高地址
  }
  else
  {
    i2c_master_write_byte(cmd, 0XA0 + ((addr / 256) << 1), ACK_CHECK_EN); // send write + 高地址
  }

  i2c_master_write_byte(cmd, addr % 256, ACK_CHECK_EN); // 低地址
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (AT_ADDR << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
  i2c_master_read_byte(cmd, &data, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  i2c_master_cmd_begin(at24cxx_master.port, cmd, 1000);
  i2c_cmd_link_delete(cmd);
  vTaskDelay(pdMS_TO_TICKS(10)); // 等待 10ms，确保 EEPROM 内部处理完成

  return data;
}
void at24cxx_write_one_byte(uint16_t addr, uint8_t data)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);

  if (EE_TYPE > AT24C16)
  {
    i2c_master_write_byte(cmd, (AT_ADDR << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN); // send write
    i2c_master_write_byte(cmd, addr >> 8, ACK_CHECK_EN);                         // 高地址
  }
  else
  {
    i2c_master_write_byte(cmd, 0XA0 + ((addr / 256) << 1), ACK_CHECK_EN); // send write + 高地址
  }
  i2c_master_write_byte(cmd, addr % 256, ACK_CHECK_EN); // 低地址
  i2c_master_write_byte(cmd, data, ACK_CHECK_EN);       // 写入数据
  i2c_master_stop(cmd);
  i2c_master_cmd_begin(at24cxx_master.port, cmd, 1000);
  i2c_cmd_link_delete(cmd);
  vTaskDelay(pdMS_TO_TICKS(10)); // 等待 10ms，确保 EEPROM 内部处理完成
}

uint8_t at24cxx_check(void)
{
  uint8_t temp;
  uint16_t addr = EE_TYPE;

  temp = at24cxx_read_one_byte(addr); // 避免每次开机都写 AT24CXX

  if (temp == 0X55)
  {

    return 0; // EEPROM 空白，未写入过数据
  }
  else
  {
    at24cxx_write_one_byte(addr, 0X55); // 写入 0xFF，标记 EEPROM 已经被写入过数据
    temp = at24cxx_read_one_byte(255);  // 读取回写的数据

    if (temp == 0X55)
    {
      return 0;
    }
  }

  return 1; // EEPROM 已经被写入过数据
}
void at24cxx_read(uint16_t addr, uint8_t *pbuf, uint16_t datalen)
{
  while (datalen--)
  {
    *pbuf++ = at24cxx_read_one_byte(addr++);
  }
}
void at24cxx_write(uint16_t addr, uint8_t *pbuf, uint16_t datalen)
{
  while (datalen--)
  {
    at24cxx_write_one_byte(addr, *pbuf);
    addr++;
    pbuf++;
  }
}