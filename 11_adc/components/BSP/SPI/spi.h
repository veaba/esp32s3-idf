#ifndef __SPI_H_
#define __SPI_H_

#include <string.h>
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#define SPI_MOSI_GPIO_PIN GPIO_NUM_11 // spi2_mosi
#define SPI_CLK_GPIO_PIN GPIO_NUM_12  // spi2_clk
#define SPI_MISO_GPIO_PIN GPIO_NUM_13 // spi2_ miso

void spi2_init(void);
void spi2_write_cmd(spi_device_handle_t handle, uint8_t cmd);                   // spi send cmd
void spi2_write_data(spi_device_handle_t handle, const uint8_t *data, int len); // spi send data
uint8_t spi2_transfer_byte(spi_device_handle_t handle, uint8_t byte);           // spi handle

#endif