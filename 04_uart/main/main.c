#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "uart.h"

void app_main(void)
{
    uint8_t len = 0;
    uint16_t times = 0;
    unsigned char data[RX_BUF_SIZE] = {0};

    led_init();
    uart_init(115200);

    while (1)
    {
        uart_get_buffered_data_len(UART_UX, (size_t *)&len);

        if (len > 0)
        {
            memset(data, 0, RX_BUF_SIZE); // 清空缓存
            printf("\n esp32s3 >: ↓ \n ");
            uart_read_bytes(UART_UX, data, len, 100);                                  // 读取数据
            uart_write_bytes(UART_UX, (const char *)data, strlen((const char *)data)); // 串口回显
        }
        // 没有信息
        else
        {
            times++;

            if (times % 5000 == 0)
            {
                printf("\n ------esp32s3 examples------ ");
            }

            if (times % 200 == 0)
            {
                printf("\n 领导！你发下话好吗？求你了");
            }

            // 每隔一段时间反转 LED
            if (times % 20 == 0)
            {
                LED_TOGGLE();
            }

            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}
