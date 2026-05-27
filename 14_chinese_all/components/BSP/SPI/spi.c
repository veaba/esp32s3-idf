

#include "spi.h"

void spi2_init(void)
{
  esp_err_t ret = 0;
  spi_bus_config_t spi_bus_conf = {0};

  // SPI 总线配置
  spi_bus_conf.miso_io_num = SPI_MISO_GPIO_PIN;
  spi_bus_conf.mosi_io_num = SPI_MOSI_GPIO_PIN;
  spi_bus_conf.sclk_io_num = SPI_CLK_GPIO_PIN;
  spi_bus_conf.quadwp_io_num = -1;              // 禁用写保护引脚
  spi_bus_conf.quadhd_io_num = -1;              //
  spi_bus_conf.max_transfer_sz = 320 * 240 * 2; // 传输大小，2.4寸的 RGB565 的一帧的字节数

  // 初始化总线
  ret = spi_bus_initialize(SPI2_HOST, &spi_bus_conf, SPI_DMA_CH_AUTO);
  ESP_ERROR_CHECK(ret); // 校验
}

void spi2_write_cmd(spi_device_handle_t handle, uint8_t cmd)
{
  esp_err_t ret;
  spi_transaction_t t = {0};
  t.length = 8;                                  // 一个字节 8 位
  t.tx_buffer = &cmd;                            // 命令填充
  ret = spi_device_polling_transmit(handle, &t); // 开始传输
  ESP_ERROR_CHECK(ret);                          // 校验
}

/**
 * SPI 写入数据
 */
void spi2_write_data(spi_device_handle_t handle, const uint8_t *data, int len)
{
  esp_err_t ret;
  spi_transaction_t t = {0};

  if (len == 0)
  {
    return;
  }

  t.length = len * 8; // *8 变为位长度
  t.tx_buffer = data;
  ret = spi_device_polling_transmit(handle, &t);
  ESP_ERROR_CHECK(ret);
}
uint8_t spi2_transfer_byte(spi_device_handle_t handle, uint8_t data)
{
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));

  t.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
  t.length = 8;
  t.tx_data[0] = data;
  spi_device_transmit(handle, &t);

  return t.rx_data[0];
}
