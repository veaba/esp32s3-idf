#include "uart.h"

void uart_init(uint32_t baud_rate)
{
    printf("Hello, ESP32! uart_init \n");
    uart_config_t uart_config = {0};

    uart_config.baud_rate = baud_rate; // 波特率
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.rx_flow_ctrl_thresh = 122;
    uart_config.source_clk = UART_SCLK_APB;

    uart_param_config(UART_UX, &uart_config);

    uart_set_pin(UART_UX, UART_TX_GPIO_PIN, UART_RX_GPIO_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_driver_install(UART_UX, RX_BUF_SIZE * 2, RX_BUF_SIZE * 2, 20, NULL, 0);
}
