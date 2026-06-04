#include "xl9555.h"

i2c_obj_t xl9555_i2c_master;
static uint16_t xl9555_failed = 0;

esp_err_t xl9555_read_byte(uint8_t *data, size_t len)
{
  uint8_t memaddr_buf[1];
  memaddr_buf[0] = XL9555_INPUT_PORT0_REG;

  i2c_buf_t bufs[2] = {
      {.len = 1, .buf = memaddr_buf},
      {.len = len, .buf = data}};

  return i2c_transfer(&xl9555_i2c_master, XL9555_ADDR, 2, bufs, I2C_FLAG_WRITE | I2C_FLAG_READ | I2C_FLAG_STOP);
}

/**
 * 写 16 位 IO 值
 *
 */
esp_err_t xl9555_write_bytes(uint8_t reg, uint8_t *data, size_t len)
{

  i2c_buf_t bufs[2] = {
      {.len = 1, .buf = &reg},
      {.len = len, .buf = data}};

  return i2c_transfer(&xl9555_i2c_master, XL9555_ADDR, 2, bufs, I2C_FLAG_STOP);
}

uint16_t xl9555_pin_write(uint16_t pin, int val)
{
  uint8_t w_data[2];
  uint16_t temp = 0x0000;

  xl9555_read_byte(w_data, 2);

  if (pin <= GBC_KEY_IO)
  {
    if (val)
    {
      w_data[0] |= (uint8_t)(0xFF & pin);
    }
    else
    {
      w_data[0] &= ~(uint8_t)(0xFF & pin);
    }
  }
  else
  {

    if (val)
    {
      w_data[1] |= (uint8_t)(0xFF & (pin >> 8));
    }
    else
    {
      w_data[1] &= ~(uint8_t)(0xFF & (pin >> 8));
    }
  }

  temp = (uint16_t)(w_data[1] << 8) | w_data[0];
  xl9555_write_bytes(XL9555_OUTPUT_PORT0_REG, w_data, 2);
  return temp;
}

/**
 * 获取某个 IO 状态
 */
int xl9555_pin_read(uint16_t pin)
{
  uint16_t ret;
  uint8_t r_data[2];

  xl9555_read_byte(r_data, 2);

  ret = r_data[1] << 8 | r_data[0];

  return (ret & pin) ? 1 : 0;
}

uint16_t xl9555_ioconfig(uint16_t config_value)
{
  uint8_t data[2];
  esp_err_t err;
  int retry = 3;

  data[0] = (uint8_t)(0xFF & config_value);
  data[1] = (uint8_t)(0XFF & (config_value >> 8));

  do
  {
    err = xl9555_write_bytes(XL9555_CONFIG_PORT0_REG, data, 2);

    if (err != ESP_OK)
    {
      retry--;
      vTaskDelay(pdMS_TO_TICKS(100)); // 等待 100ms 后重试
      xl9555_failed++;
      ESP_LOGE("XL9555", "Failed to write config, retrying... (%d)", retry);

      xl9555_failed = 1;

      if ((retry <= 0) && xl9555_failed)
      {
        vTaskDelay(pdMS_TO_TICKS(5000)); // 等待 5s 后重试
        esp_restart();                   // 重启设备
      }
    }
    else
    {
      xl9555_failed = 0;
      break;
    }
  } while (retry);

  return config_value;
}

void xl9555_init(i2c_obj_t self)
{
  uint8_t r_data[2];

  if (self.init_flag == ESP_FAIL)
  {
    ESP_LOGE("XL9555", "I2C initialization failed");
    iic_init(I2C_NUM_0); // 初始化 I2C0
  }

  xl9555_i2c_master = self;
  gpio_config_t gpio_init_struct = {0};

  gpio_init_struct.intr_type = GPIO_INTR_DISABLE;
  gpio_init_struct.mode = GPIO_MODE_INPUT;
  gpio_init_struct.pin_bit_mask = (1ull << XL9555_INT_IO);
  gpio_init_struct.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_init_struct.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&gpio_init_struct);

  // 清除上一次中断状态
  xl9555_read_byte(r_data, 2);

  xl9555_ioconfig(0xF003);
  xl9555_pin_write(BEEP_IO, 1);   // 蜂鸣器
  xl9555_pin_write(SPK_EN_IO, 1); // 扬声器使能
}

uint8_t xl9555_key_scan(uint8_t mode)
{
  uint8_t key_val = 0;
  static uint8_t key_up = 1; // 按键空开标志

  if (mode)
  {
    key_up = 1; // 重新开启按键扫描
  }
  if (key_up && (KEY0 == 0 || KEY1 == 0 || KEY2 == 0 || KEY3 == 0))
  {
    vTaskDelay(pdMS_TO_TICKS(10)); // 消抖延时
    key_up = 0;                    // 按键按下，关闭扫描
    if (KEY0 == 0)
    {
      key_val = KEY0_PRES;
    }
    if (KEY1 == 0)
    {
      key_val = KEY1_PRES;
    }
    if (KEY2 == 0)
    {
      key_val = KEY2_PRES;
    }
    if (KEY3 == 0)
    {
      key_val = KEY3_PRES;
    }
  }
  else if (KEY0 == 1 && KEY1 == 1 && KEY2 == 1 && KEY3 == 1) // 没有按键按下，标记按键松开
  {
    key_up = 1; // 按键释放，开启扫描
  }
  return key_val;
}