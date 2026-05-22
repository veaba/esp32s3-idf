#ifndef __UART_H_
#define __UART_H_

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/uart_select.h"
#include "driver/gpio.h"


#define UART_UX UART_NUM_0 // on pc
#define UART_TX_GPIO_PIN GPIO_NUM_43
#define UART_RX_GPIO_PIN GPIO_NUM_44

// 缓冲区
#define RX_BUF_SIZE 1024

void uart_init(uint32_t baud_rate);
#endif