#ifndef __RMT_NEC_RX_H
#define __RMT_NEC_RX_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "freertos/queue.h"
#include "driver/rmt_rx.h"
#include "esp_err.h"

#define RMT_IN_GPIO_PIN GPIO_NUM_2 // RMT_RX GPIO 口
#define RMT_RESOLUTION_HZ 1000000  // 1Mhz, 1tick = 1us
#define RMT_NEC_DECODE_MARGIN 400  // 时序容差值

#define NEC_LEADING_CODE_DURATION_0 9000
#define NEC_LEADING_CODE_DURATION_1 4500
#define NEC_PAYLOAD_ZERO_DURATION_0 560
#define NEC_PAYLOAD_ZERO_DURATION_1 560
#define NEC_PAYLOAD_ONE_DURATION_0 560
#define NEC_PAYLOAD_ONE_DURATION_1 1690
#define NEC_REPEAT_CODE_DURATION_0 9000
#define NEC_REPEAT_CODE_DURATION_1 2250

extern QueueHandle_t receive_queue;
extern rmt_channel_handle_t rx_channel;
extern rmt_symbol_word_t raw_symbols[64];
extern rmt_receive_config_t receive_config;
extern uint16_t s_nec_code_address;
extern uint16_t s_nec_code_command;

esp_err_t rmt_nec_rx_init(void);
bool rmt_nec_parse_frame(rmt_symbol_word_t *rmt_nec_symbols);
bool rmt_nec_parse_frame_repeat(rmt_symbol_word_t *rmt_nec_symbols);

#endif
